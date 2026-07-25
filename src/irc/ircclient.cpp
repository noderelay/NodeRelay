#include "ircclient.h"
#include "ircparser.h"
#include "stsstore.h"
#include "config/config.h"
#include "model/ids.h" // for isChannelName()
#include "gitversion.h"
#include "logging.h"

#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QFile>
#include <QNetworkProxy>
#include <QSslCertificate>
#include <QSslKey>
#include <QSslSocket>
#include <QWebSocket>
#include <QTimer>

static QString stripCrlf(QString s)
{
    s.remove('\r');
    s.remove('\n');
    return s;
}

static bool validIrcToken(const QString &s)
{
    if (s.isEmpty()) return false;
    for (QChar c : s)
        if (c.isSpace() || c == '\r' || c == '\n' || c == '\0')
            return false;
    return true;
}

static QString ircv3TagEscape(QString s)
{
    s.replace("\\", "\\\\");
    s.replace(";", "\\:");
    s.replace(" ", "\\s");
    s.replace("\r", "\\r");
    s.replace("\n", "\\n");
    return s;
}

IrcClient::IrcClient(QObject *parent)
    : QObject(parent)
    , m_socket(new QSslSocket(this))
{
    connect(m_socket, &QSslSocket::connected,       this, &IrcClient::onConnected);
    connect(m_socket, &QSslSocket::disconnected,    this, &IrcClient::onDisconnected);
    connect(m_socket, &QSslSocket::readyRead,       this, &IrcClient::onReadyRead);
    connect(m_socket, &QSslSocket::sslErrors,       this, &IrcClient::onSslErrors);
    connect(m_socket, &QAbstractSocket::errorOccurred, this, &IrcClient::onErrorOccurred);

    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, &IrcClient::doReconnect);

    m_pingTimer = new QTimer(this);
    m_pingTimer->setInterval(30000);
    connect(m_pingTimer, &QTimer::timeout, this, &IrcClient::sendPing);
}

IrcClient::~IrcClient() = default;

void IrcClient::connectToServer(const ServerConfig &cfg)
{
    m_reconnectTimer->stop();
    m_intentionalDisconnect = false;
    m_reconnectDelay = 5;

    m_serverName      = cfg.name;
    m_host            = cfg.host;
    m_port            = cfg.port;
    m_ssl             = cfg.ssl;
    m_nick            = cfg.nick;
    // Registration sends the sanitized nick — reclaim must compare against
    // and send the same form, or the two never converge.
    m_wantedNick      = stripCrlf(cfg.nick).remove(' ').remove('\0');
    m_nickFallback    = false;
    m_saslAuthed      = false;
    m_user            = cfg.user;
    m_realname        = cfg.realname;
    m_password        = cfg.password;
    m_saslUser        = cfg.saslUser;
    m_saslPassword    = cfg.saslPassword;
    m_saslExternal    = cfg.saslExternal;
    m_saslPending     = false;
    m_utf8Only        = false;
    m_nickservPassword = cfg.nickservPassword;
    m_bouncerType     = cfg.bouncerType;
    m_metadataEnabled = cfg.metadata;
    m_bouncerNetwork  = cfg.bouncerNetwork;
    m_proxyHost           = cfg.proxyHost;
    m_proxyPort           = cfg.proxyPort;
    m_proxyUser           = cfg.proxyUser;
    m_proxyPass           = cfg.proxyPass;
    m_pinnedFingerprint   = cfg.pinnedFingerprint;
    m_useWs               = cfg.websocket;
    m_quitMessage         = cfg.quitMessage;

    if (m_useWs) {
        if (!m_wsSocket) {
            m_wsSocket = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);
            connect(m_wsSocket, &QWebSocket::connected,           this, &IrcClient::onConnected);
            connect(m_wsSocket, &QWebSocket::disconnected,        this, &IrcClient::onDisconnected);
            connect(m_wsSocket, &QWebSocket::textMessageReceived, this, &IrcClient::onWsTextReceived);
            connect(m_wsSocket, &QWebSocket::sslErrors,           this, &IrcClient::onSslErrors);
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
            connect(m_wsSocket, &QWebSocket::errorOccurred,       this, &IrcClient::onErrorOccurred);
#else
            connect(m_wsSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
                    this, &IrcClient::onErrorOccurred);
#endif
        } else if (m_wsSocket->state() != QAbstractSocket::UnconnectedState) {
            m_wsSocket->abort();
            // abort() can fire onDisconnected() synchronously, re-arming the
            // reconnect timer that was stopped above.
            m_reconnectTimer->stop();
        }
        applyProxy();
        QUrl url;
        url.setScheme(m_ssl ? "wss" : "ws");
        url.setHost(m_host);
        url.setPort(m_port);
        url.setPath("/");
        qCDebug(lcIrc) << m_serverName << "connecting to" << url.toString();
        m_wsSocket->open(url);
        return;
    }

    // Apply cached STS policy — upgrade plain to TLS if one exists
    if (!m_ssl) {
        StsStore::Policy sts;
        if (StsStore::lookup(m_host, sts)) {
            m_ssl  = true;
            m_port = sts.port;
        }
    }

    if (m_saslExternal && !cfg.clientCertFile.isEmpty() && !cfg.clientKeyFile.isEmpty()) {
        QFile certFile(cfg.clientCertFile);
        if (certFile.open(QIODevice::ReadOnly)) {
            const QSslCertificate cert(certFile.readAll(), QSsl::Pem);
            if (!cert.isNull())
                m_socket->setLocalCertificate(cert);
        }
        QFile keyFile(cfg.clientKeyFile);
        if (keyFile.open(QIODevice::ReadOnly)) {
            const QByteArray keyData = keyFile.readAll();
            QSslKey key(keyData, QSsl::Rsa, QSsl::Pem);
            if (key.isNull()) key = QSslKey(keyData, QSsl::Ec, QSsl::Pem);
            if (!key.isNull())
                m_socket->setPrivateKey(key);
        }
    }

    if (sockState() != QAbstractSocket::UnconnectedState) {
        sockAbort();
        // abort() can fire onDisconnected() synchronously, re-arming the
        // reconnect timer that was stopped above.
        m_reconnectTimer->stop();
    }

    applyProxy();
    qCDebug(lcIrc) << m_serverName << "connecting to" << m_host << m_port
                   << (m_ssl ? "(tls)" : "(plain)");
    if (m_ssl)
        m_socket->connectToHostEncrypted(m_host, m_port);
    else
        m_socket->connectToHost(m_host, m_port);
}

void IrcClient::applyProxy()
{
    QNetworkProxy proxy = m_proxyHost.isEmpty()
        ? QNetworkProxy(QNetworkProxy::NoProxy)
        : QNetworkProxy(QNetworkProxy::Socks5Proxy, m_proxyHost, m_proxyPort,
                        m_proxyUser, m_proxyPass);
    if (m_useWs && m_wsSocket)
        m_wsSocket->setProxy(proxy);
    else
        m_socket->setProxy(proxy);
}

// ---------------------------------------------------------------------------
// Socket abstraction helpers
// ---------------------------------------------------------------------------

void IrcClient::sockWrite(const QString &line)
{
    if (m_useWs) {
        m_wsSocket->sendTextMessage(line);
    } else {
        QByteArray buf = line.toUtf8();
        buf.append("\r\n", 2);
        m_socket->write(buf);
    }
}

void IrcClient::sockDisconnect()
{
    if (m_useWs) m_wsSocket->close();
    else         m_socket->disconnectFromHost();
}

void IrcClient::sockAbort()
{
    if (m_useWs) m_wsSocket->abort();
    else         m_socket->abort();
}

QAbstractSocket::SocketState IrcClient::sockState() const
{
    return m_useWs ? m_wsSocket->state() : m_socket->state();
}

QHostAddress IrcClient::sockLocalAddress() const
{
    return m_useWs ? m_wsSocket->localAddress() : m_socket->localAddress();
}

QString IrcClient::sockErrorString() const
{
    return m_useWs ? m_wsSocket->errorString() : m_socket->errorString();
}

bool IrcClient::isConnected() const
{
    return sockState() == QAbstractSocket::ConnectedState;
}

quint32 IrcClient::localIpv4() const
{
    bool ok = false;
    const quint32 v4 = sockLocalAddress().toIPv4Address(&ok);
    return ok ? v4 : 0;
}

void IrcClient::quit(const QString &reason)
{
    m_intentionalDisconnect = true;
    m_reconnectTimer->stop();
    const QString msg = !reason.isEmpty() ? reason
                        : (!m_quitMessage.isEmpty() ? m_quitMessage : QStringLiteral("Uplink"));
    sendRaw("QUIT :" + msg);
    sockDisconnect();
}

void IrcClient::join(const QString &channel, const QString &key)
{
    if (!validIrcToken(channel)) return;
    sendRaw(key.isEmpty() ? "JOIN " + channel : "JOIN " + channel + " " + stripCrlf(key));
}

void IrcClient::part(const QString &channel, const QString &reason)
{
    if (!validIrcToken(channel)) return;
    sendRaw(reason.isEmpty() ? "PART " + channel : "PART " + channel + " :" + stripCrlf(reason));
}

void IrcClient::privmsg(const QString &target, const QString &text, const QString &replyToMsgid)
{
    if (!validIrcToken(target)) return;
    if (sockState() != QAbstractSocket::ConnectedState) {
        emit errorMessage(m_serverName, "Not connected — message not sent");
        return;
    }
    if (m_utf8Only && text.contains(QChar::ReplacementCharacter))
        emit serverMessage(m_serverName,
            "Warning: message contains invalid UTF-8 characters — server requires UTF-8 (UTF8ONLY)");

    const QString tagStr = replyToMsgid.isEmpty()
        ? QString{}
        : "@+draft/reply=" + ircv3TagEscape(replyToMsgid) + " ";
    const int baseOverhead = 10 + static_cast<int>(target.toUtf8().size()); // "PRIVMSG target :"
    const QByteArray tagBytes = tagStr.toUtf8();

    const QByteArray utf8 = stripCrlf(text).toUtf8();
    if (utf8.isEmpty()) return;

    int offset = 0;
    bool first = true;
    while (offset < utf8.size()) {
        const int tagLen  = first ? static_cast<int>(tagBytes.size()) : 0;
        const int maxText = 510 - baseOverhead - tagLen;
        int chunkLen = qMin(maxText, static_cast<int>(utf8.size()) - offset);
        // back off to a valid UTF-8 boundary
        int end = offset + chunkLen;
        while (end > offset && end < utf8.size()
               && (static_cast<unsigned char>(utf8[end]) & 0xC0) == 0x80)
            --end;
        chunkLen = end - offset;
        if (chunkLen <= 0) break;
        const QString chunk = QString::fromUtf8(utf8.mid(offset, chunkLen));
        sendRaw((first ? tagStr : QString{}) + "PRIVMSG " + target + " :" + chunk);
        offset += chunkLen;
        first = false;
    }
}

void IrcClient::sendMultiline(const QString &target, const QString &text,
                               const QString &replyToMsgid)
{
    if (!validIrcToken(target)) return;
    if (sockState() != QAbstractSocket::ConnectedState) {
        emit errorMessage(m_serverName, "Not connected — message not sent");
        return;
    }

    const QStringList lines = text.split('\n', Qt::KeepEmptyParts);

    if (!m_ackedCaps.contains("draft/multiline") || lines.size() <= 1) {
        // Fall back to separate PRIVMSGs
        for (const QString &line : lines)
            if (!line.isEmpty())
                privmsg(target, line, replyToMsgid);
        return;
    }

    const QString ref = QStringLiteral("ml%1")
        .arg(QRandomGenerator::global()->generate(), 8, 16, QLatin1Char('0'));

    sendRaw("BATCH +" + ref + " draft/multiline " + target);
    bool first = true;
    for (const QString &line : lines) {
        QString tags = "@batch=" + ref;
        if (first && !replyToMsgid.isEmpty())
            tags += ";+draft/reply=" + ircv3TagEscape(replyToMsgid);
        sendRaw(tags + " PRIVMSG " + target + " :" + stripCrlf(line));
        first = false;
    }
    sendRaw("BATCH -" + ref);
}

void IrcClient::notice(const QString &target, const QString &text)
{
    if (!validIrcToken(target)) return;
    if (sockState() != QAbstractSocket::ConnectedState) {
        emit errorMessage(m_serverName, "Not connected — message not sent");
        return;
    }
    sendRaw("NOTICE " + target + " :" + stripCrlf(text));
}

void IrcClient::setNick(const QString &nick)
{
    const QString clean = stripCrlf(nick).remove(' ').remove('\0');
    m_nick = clean;
    sendRaw("NICK " + clean);
}

void IrcClient::sendTyping(const QString &channel, const QString &state)
{
    if (!validIrcToken(channel)) return;
    const bool hasFinal = m_ackedCaps.contains("typing");
    const bool hasDraft = m_ackedCaps.contains("draft/typing");
    const bool hasTags  = m_ackedCaps.contains("message-tags");
    if (!hasFinal && !hasDraft && !hasTags) return;
    const QString tag = hasFinal ? "typing=" : "+typing=";
    sendRaw("@" + tag + ircv3TagEscape(state) + " TAGMSG " + channel);
}

bool IrcClient::sendReact(const QString &target, const QString &msgid, const QString &emoji)
{
    if (!validIrcToken(target)) return false;
    if (msgid.isEmpty() || emoji.isEmpty()) return false;
    if (!m_ackedCaps.contains("message-tags")) return false;
    sendRaw("@+draft/react=" + ircv3TagEscape(emoji)
            + ";+draft/reply=" + ircv3TagEscape(msgid)
            + " TAGMSG " + stripCrlf(target));
    return true;
}

void IrcClient::requestHistory(const QString &target, int limit)
{
    if (!m_ackedCaps.contains("chathistory") && !m_ackedCaps.contains("draft/chathistory"))
        return;
    sendRaw(QString("CHATHISTORY LATEST %1 * %2").arg(target).arg(limit));
}

bool IrcClient::requestHistoryBefore(const QString &target, const QDateTime &before, int limit)
{
    if (!m_ackedCaps.contains("chathistory") && !m_ackedCaps.contains("draft/chathistory"))
        return false;
    // Timestamp bound, not msgid: soju rejects msgid bounds with
    // "Invalid first bound"; every implementation takes timestamps.
    sendRaw(QString("CHATHISTORY BEFORE %1 timestamp=%2 %3")
            .arg(target, before.toUTC().toString(Qt::ISODateWithMs)).arg(limit));
    return true;
}

void IrcClient::markRead(const QString &target, const QDateTime &ts)
{
    if (!m_ackedCaps.contains("soju.im/read") &&
        !m_ackedCaps.contains("draft/read-marker")) return;
    sendRaw("MARKREAD " + target + " timestamp=" + ts.toUTC().toString(Qt::ISODateWithMs));
}

void IrcClient::sendRedact(const QString &target, const QString &msgid, const QString &reason)
{
    if (!m_ackedCaps.contains("draft/message-redaction")) return;
    if (target.isEmpty() || msgid.isEmpty()) return;
    sendRaw("REDACT " + stripCrlf(target) + " " + stripCrlf(msgid)
            + (reason.isEmpty() ? QString() : " :" + stripCrlf(reason)));
}

void IrcClient::setMonitorList(const QStringList &nicks)
{
    m_monitorList = nicks;
    m_monitorSet.clear();
    for (const QString &n : nicks)
        m_monitorSet.insert(n.toLower());
}

void IrcClient::monitorAdd(const QString &nick)
{
    if (nick.isEmpty()) return;
    if (!m_monitorSet.contains(nick.toLower())) {
        m_monitorList.append(nick);
        m_monitorSet.insert(nick.toLower());
    }
    sendRaw("MONITOR + " + stripCrlf(nick));
}

void IrcClient::monitorRemove(const QString &nick)
{
    m_monitorSet.remove(nick.toLower());
    m_monitorList.removeIf([&](const QString &n){ return n.compare(nick, Qt::CaseInsensitive) == 0; });
    sendRaw("MONITOR - " + stripCrlf(nick));
}

void IrcClient::monitorClear()
{
    m_monitorList.clear();
    m_monitorSet.clear();
    sendRaw("MONITOR C");
}

void IrcClient::monitorStatus()
{
    sendRaw("MONITOR S");
}

static QString redactRawForLog(const QString &line)
{
    const QString cmd = line.section(' ', 0, 0).toUpper();
    if (cmd == "PASS")
        return "PASS :<redacted>";
    if (cmd == "AUTHENTICATE")
        return "AUTHENTICATE <redacted>";
    if (cmd == "OPER")
        return "OPER " + line.section(' ', 1, 1) + " :<redacted>";
    if (line.contains("NickServ", Qt::CaseInsensitive) &&
        line.contains("IDENTIFY", Qt::CaseInsensitive))
        return "<NickServ IDENTIFY redacted>";
    return line;
}

void IrcClient::sendRaw(const QString &line)
{
    if (sockState() != QAbstractSocket::ConnectedState) return;
    const QString stripped = stripCrlf(line);
    if (stripped.startsWith("AWAY :", Qt::CaseInsensitive))
        m_awayMsg = stripped.mid(6);
    else if (stripped.compare("AWAY", Qt::CaseInsensitive) == 0)
        m_awayMsg.clear();
    // Enforce 510-byte wire limit (RFC 1459: 512 bytes including CRLF).
    // left(510) counts code-points, not bytes — multi-byte chars could exceed the limit.
    QByteArray utf8 = stripped.toUtf8();
    if (utf8.size() > 510) {
        int i = 510;
        while (i > 0 && (static_cast<unsigned char>(utf8[i]) & 0xC0) == 0x80) --i;
        utf8.truncate(i);
    }
    const QString clean = QString::fromUtf8(utf8);
    const QString logLine = redactRawForLog(clean);
    emit rawReceived(">> " + logLine);
    qCDebug(lcIrc).noquote() << m_serverName << ">>" << logLine;
    sockWrite(clean);
}

// ---------------------------------------------------------------------------
// Socket callbacks
// ---------------------------------------------------------------------------

void IrcClient::onConnected()
{
    // For SSL connections, defer until TLS handshake completes. UniqueConnection:
    // a single-shot connection survives a failed handshake (it is only removed
    // when the signal fires), so without it each failed attempt stacks another
    // onConnected invocation — registration would then run N+1 times.
    if (m_ssl && !m_useWs && !m_socket->isEncrypted()) {
        connect(m_socket, &QSslSocket::encrypted, this, &IrcClient::onConnected,
                static_cast<Qt::ConnectionType>(Qt::SingleShotConnection | Qt::UniqueConnection));
        return;
    }

    m_reconnectTimer->stop();
    m_reconnectDelay = 5;
    m_pingPending = false;
    m_pingTimer->start();

    qCDebug(lcIrc) << m_serverName << "connected, registering";
    sendRaw("CAP LS 302");

    // For ZNC with bouncer_network + SASL credentials, assemble the ZNC PASS format
    // so users don't have to embed it manually as "user/network:password".
    const bool zncAssembled = m_bouncerType == BouncerType::ZNC
                              && !m_bouncerNetwork.isEmpty()
                              && !m_saslUser.isEmpty()
                              && !m_saslPassword.isEmpty();
    if (zncAssembled) {
        if (!m_ssl)
            emit serverMessage(m_serverName, "Warning: sending server password over unencrypted connection");
        sendRaw("PASS :" + stripCrlf(m_saslUser) + "/"
                         + stripCrlf(m_bouncerNetwork) + ":"
                         + stripCrlf(m_saslPassword));
    } else if (!m_password.isEmpty()) {
        if (!m_ssl)
            emit serverMessage(m_serverName, "Warning: sending server password over unencrypted connection");
        sendRaw("PASS :" + stripCrlf(m_password));
    }

    const QString safeNick = stripCrlf(m_nick).remove(' ').remove('\0');
    const QString safeUser = stripCrlf(m_user).remove(' ').remove('\0');
    sendRaw("NICK " + safeNick);
    sendRaw("USER " + safeUser + " 0 * :" + stripCrlf(m_realname));
}

void IrcClient::onDisconnected()
{
    qCDebug(lcIrc) << m_serverName << "disconnected";
    m_pingTimer->stop();
    m_pingPending = false;
    m_buffer.clear();
    m_namesBuffer.clear();
    m_requestedCaps.clear();
    m_ackedCaps.clear();
    m_capLsBuffer.clear();
    m_batches.clear();
    m_saslPending    = false;
    m_saslAuthed     = false;
    m_nickFallback   = false;
    m_registered     = false;
    m_supportsWhox   = false;
    m_bouncerListing = false;
    m_bouncerNetBuf.clear();
    emit disconnected(m_serverName);
    if (!m_stsUpgrade)
        scheduleReconnect();
    m_stsUpgrade = false;
    // m_intentionalDisconnect is reset by connectToServer() on next connection
}

static constexpr qsizetype kMaxPendingBuffer = qsizetype{64} * 1024;
static constexpr qsizetype kMaxIrcLine       = 8192;
static constexpr int       kMaxOpenBatches   = 8;
static constexpr int       kMaxBatchMessages = 1000;

void IrcClient::onWsTextReceived(const QString &message)
{
    // Each WebSocket text frame is one complete IRC line — no buffering needed.
    const QString line = message.trimmed();
    if (line.isEmpty() || line.size() > kMaxIrcLine) return;
    emit rawReceived(line);
    qCDebug(lcIrc).noquote() << m_serverName << "<<" << line;
    processLine(line);
}

void IrcClient::onReadyRead()
{
    const QByteArray raw = m_socket->readAll();
    if (m_utf8Only && raw.contains('\xff')) {
        // Cheap non-UTF-8 sniff: 0xFF never appears in valid UTF-8
        emit serverMessage(m_serverName, "Warning: server sent non-UTF-8 data (UTF8ONLY is set)");
    }
    m_buffer += raw;

    // Extract complete lines and trim the buffer BEFORE dispatching: a slot
    // reached from processLine may spin a nested event loop (e.g. a dialog),
    // and a reentrant onReadyRead must not rescan lines already taken.
    QList<QByteArray> lines;
    qsizetype start = 0, idx;
    while ((idx = m_buffer.indexOf('\n', start)) != -1) {
        QByteArray lineBytes = m_buffer.mid(start, idx - start);
        start = idx + 1;
        if (!lineBytes.isEmpty() && lineBytes.back() == '\r')
            lineBytes.chop(1);
        if (!lineBytes.isEmpty())
            lines.append(lineBytes);
    }
    if (start > 0)
        m_buffer = m_buffer.mid(start);

    for (const QByteArray &lineBytes : lines) {
        if (lineBytes.size() > kMaxIrcLine) {
            emit socketError(m_serverName, "Dropped oversized IRC line from server");
            continue;
        }
        if (Q_UNLIKELY(!lineBytes.isValidUtf8()))
            qCDebug(lcIrc) << m_serverName << "non-UTF-8 line, decoding as Latin-1";
        const QString line = IrcParser::decodeLine(lineBytes);
        emit rawReceived(line);
        qCDebug(lcIrc).noquote() << m_serverName << "<<" << line;
        processLine(line);
    }

    // Only disconnect if the *remaining* partial line (no newline yet) is absurdly large
    if (m_buffer.size() > kMaxPendingBuffer) {
        emit socketError(m_serverName, "Server sent oversized unterminated data; disconnecting");
        m_socket->disconnectFromHost();
        m_buffer.clear();
    }
}

void IrcClient::onSslErrors(const QList<QSslError> &errors)
{
    bool allPinnable = true;
    QStringList errorMsgs;
    for (const auto &e : errors) {
        errorMsgs << e.errorString();
        if (e.error() != QSslError::SelfSignedCertificate &&
            e.error() != QSslError::SelfSignedCertificateInChain &&
            e.error() != QSslError::HostNameMismatch) {
            allPinnable = false;
        }
    }

    if (!allPinnable) {
        emit socketError(m_serverName, "TLS error: " + errorMsgs.join("; "));
        sockDisconnect();
        return;
    }

    const auto chain = m_useWs
        ? m_wsSocket->sslConfiguration().peerCertificateChain()
        : m_socket->peerCertificateChain();
    if (chain.isEmpty()) {
        emit socketError(m_serverName, "TLS error: no peer certificate");
        sockDisconnect();
        return;
    }
    const QString fp = QString::fromLatin1(
        chain.first().digest(QCryptographicHash::Sha256).toHex(':').toUpper());

    if (!m_pinnedFingerprint.isEmpty()) {
        if (m_pinnedFingerprint.compare(fp, Qt::CaseInsensitive) == 0) {
            qCDebug(lcIrc) << m_serverName << "pinned fingerprint matched, ignoring ssl errors:"
                           << errorMsgs;
            if (m_useWs) m_wsSocket->ignoreSslErrors(errors);
            else         m_socket->ignoreSslErrors(errors);
            return;
        }
        emit socketError(m_serverName,
            QString("TLS: certificate fingerprint mismatch!\nPinned: %1\nGot:    %2")
                .arg(m_pinnedFingerprint, fp));
        sockDisconnect();
        return;
    }

    // No pin — abort before any credentials are sent, then ask the user.
    // The connection is re-established after the user accepts the fingerprint.
    m_intentionalDisconnect = true;
    sockAbort();
    emit sslFingerprintPrompt(m_serverName, fp);
}

void IrcClient::abort()
{
    m_intentionalDisconnect = true;
    if (sockState() != QAbstractSocket::UnconnectedState)
        sockDisconnect();
}

void IrcClient::reconnect()
{
    m_intentionalDisconnect = false;
    m_reconnectTimer->stop();
    m_reconnectDelay = 5;
    doReconnect();
}

// Network reachability regained (laptop resume, Wi-Fi roam). Unlike
// reconnect(), never clears m_intentionalDisconnect, so a server the
// user /disconnect'ed stays down.
void IrcClient::onNetworkOnline()
{
    if (m_intentionalDisconnect || m_host.isEmpty())
        return;
    if (m_reconnectTimer->isActive()) {
        // Backoff wait in progress: retry now on the fresh network.
        m_reconnectTimer->stop();
        m_reconnectDelay = 5;
        doReconnect();
    } else if (isConnected()) {
        // Socket may be stale after the network change; probe it now
        // instead of waiting up to 30s for the ping watchdog tick.
        sendPing();
    }
    // else: a connect attempt is already in flight; leave it alone.
}

void IrcClient::onErrorOccurred(QAbstractSocket::SocketError)
{
    emit socketError(m_serverName, sockErrorString());
    // Qt emits disconnected() (→ onDisconnected → scheduleReconnect) only when a
    // socket that reached ConnectedState loses it. A failed *connection attempt*
    // (server down: ConnectionRefused/timeout) never emits disconnected(), so we
    // must reschedule here or the retry loop dies after the first attempt.
    // scheduleReconnect() is idempotent, so this is safe if both fire.
    if (sockState() == QAbstractSocket::UnconnectedState)
        scheduleReconnect();
}

void IrcClient::scheduleReconnect()
{
    if (m_intentionalDisconnect || m_reconnectTimer->isActive() || m_host.isEmpty()) return;
    emit serverMessage(m_serverName, QString("Reconnecting in %1s…").arg(m_reconnectDelay));
    m_reconnectTimer->start(m_reconnectDelay * 1000);
    m_reconnectDelay = qMin(m_reconnectDelay * 2, 60);
}

void IrcClient::sendPing()
{
    if (m_pingPending) {
        if (m_pingClock.elapsed() > 90000) {
            emit socketError(m_serverName, "Ping timeout");
            sockAbort();
        }
        return;
    }
    m_pingPending = true;
    m_pingClock.start();
    sendRaw("PING :uplink_rtt");
}

void IrcClient::doReconnect()
{
    emit reconnecting(m_serverName);
    emit serverMessage(m_serverName, "Reconnecting…");
    m_saslPending = false;
    if (sockState() != QAbstractSocket::UnconnectedState)
        sockAbort();
    // abort() can invoke onDisconnected() synchronously (see the STS comment
    // below), which re-arms the reconnect timer; left running, its next fire
    // would abort a TLS handshake slower than the 5s delay.
    m_reconnectTimer->stop();
    applyProxy();
    if (m_useWs) {
        QUrl url;
        url.setScheme(m_ssl ? "wss" : "ws");
        url.setHost(m_host);
        url.setPort(m_port);
        url.setPath("/");
        m_wsSocket->open(url);
    } else if (m_ssl) {
        m_socket->connectToHostEncrypted(m_host, m_port);
    } else {
        m_socket->connectToHost(m_host, m_port);
    }
}

// ---------------------------------------------------------------------------
// Line processing
// ---------------------------------------------------------------------------

void IrcClient::processLine(const QString &line)
{
    IrcMessage msg = IrcParser::parse(line);
    if (!msg.isValid()) return;

    const QString &cmd = msg.command;

    if (cmd == "PING") {
        sendRaw("PONG :" + (msg.params.isEmpty() ? QString{} : msg.params.last()));
        return;
    }

    if (cmd == "PONG") {
        if (m_pingPending && !msg.params.isEmpty() && msg.params.last() == "uplink_rtt") {
            m_pingPending = false;
            emit pingRtt(m_serverName, static_cast<int>(m_pingClock.elapsed()));
        }
        return;
    }

    if (cmd == "CAP") {
        handleCap(msg.params, msg.trailing);
        return;
    }

    if (cmd == "AUTHENTICATE") {
        if (!msg.params.isEmpty() && msg.params[0] == "+") {
            if (m_saslExternal) {
                sendRaw("AUTHENTICATE +");
            } else {
                if (!m_ssl)
                    emit serverMessage(m_serverName, "Warning: sending SASL credentials over unencrypted connection");
                // Bouncers select the network via the SASL username: both soju
                // and ZNC take "user/network". Without it ZNC authenticates the
                // user but attaches no network, parking the client in a status
                // query (the PASS fallback never applies once SASL succeeds).
                QString saslUser = m_saslUser;
                if ((m_bouncerType == BouncerType::Soju || m_bouncerType == BouncerType::ZNC)
                        && !m_bouncerNetwork.isEmpty())
                    saslUser += "/" + m_bouncerNetwork;
                const QByteArray payload =
                    QByteArray("\0", 1) + QString(saslUser).remove(QChar(0)).toUtf8() +
                    QByteArray("\0", 1) + QString(m_saslPassword).remove(QChar(0)).toUtf8();
                sendRaw("AUTHENTICATE " + QString::fromLatin1(payload.toBase64()));
            }
        }
        return;
    }

    if (cmd == "BATCH") {
        handleBatch(msg.params);
        return;
    }

    if (cmd == "BOUNCER") {
        handleBouncer(msg.params, msg.trailing);
        return;
    }

    if (cmd == "MARKREAD" && msg.params.size() >= 2) {
        const QString target = msg.params[0];
        const QString tsTag  = msg.params[1]; // "timestamp=<iso>"
        if (tsTag.startsWith("timestamp=")) {
            const QDateTime ts = QDateTime::fromString(tsTag.mid(10), Qt::ISODateWithMs);
            emit readMarkerReceived(m_serverName, target, ts);
        }
        return;
    }

    // PERSISTENCE STATUS <client-setting> <effective-setting> — sent in the
    // registration burst and in reply to PERSISTENCE GET/SET (draft/persistence)
    if (cmd == "PERSISTENCE" && msg.params.value(0).toUpper() == "STATUS" &&
        msg.params.size() >= 3) {
        emit contextualMessage(m_serverName,
            QString("Persistence: %1 (your setting: %2)")
                .arg(msg.params.value(2).toLower(), msg.params.value(1).toLower()));
        return;
    }

    // Standard replies are command responses — handle immediately, never
    // buffer them into batches (metadata-3 sends FAIL METADATA inside one)
    if (cmd == "FAIL" || cmd == "WARN" || cmd == "NOTE") {
        // params: [0]=command [1]=code [2..n-1]=context  trailing=description
        const QString triggeredBy = msg.params.value(0);
        const QString code        = msg.params.value(1);
        // Silently drop benign probe failures — these are expected on servers that
        // advertise the capability but don't support our specific keys/subcommands
        if (triggeredBy == "BOUNCER" && code == "UNKNOWN_COMMAND") {
            m_bouncerListingUnsupported = true;
            return;
        }
        // KEY_INVALID renamed to INVALID_KEY in metadata-3
        if (triggeredBy == "METADATA" && (code == "KEY_INVALID" || code == "INVALID_KEY"))
            return;
        // Background hover-GETs race the target going offline — expected,
        // not user-facing. Tell the model so it can allow a later retry.
        if (triggeredBy == "METADATA" && code == "INVALID_TARGET") {
            emit metaLookupFailed(m_serverName, msg.params.value(2));
            return;
        }
        // ACCOUNT_REQUIRED is informational — route to server buffer only, not
        // channels. PERSISTENCE is the exception: it's the reply to something
        // the user just typed, so dropping it would look like nothing happened.
        if (code == "ACCOUNT_REQUIRED" && triggeredBy != "PERSISTENCE")
            return;
        // The description is always the last param — the parser appends
        // trailing into params, so trailing or not, it sits at the end.
        const QString desc = msg.params.size() > 2 ? msg.params.value(msg.params.size() - 1)
                                                   : QString();
        // Look for a channel name in the context parameters
        QString channel;
        for (int i = 2; i < msg.params.size() - 1; ++i) {
            const QString &p = msg.params[i];
            if (isChannelName(p)) { channel = p; break; }
        }
        // Context params (channel, key, …) say what was refused — without
        // them a FAIL from the wrong buffer is undebuggable.
        const qsizetype ctxEnd = msg.params.size() - 1;
        const QString context = ctxEnd > 2 ? msg.params.mid(2, ctxEnd - 2).join(' ')
                                           : QString();
        const QString prefix  = "[" + cmd + "] ";
        QString text = triggeredBy.isEmpty() || triggeredBy == "*"
            ? prefix + code
            : prefix + triggeredBy + " " + code;
        if (!context.isEmpty())
            text += " [" + context + "]";
        if (!desc.isEmpty())
            text += ": " + desc;
        if (!channel.isEmpty())
            emit standardReply(m_serverName, channel, cmd, text);
        else if (cmd == "FAIL")
            emit errorMessage(m_serverName, text);
        else
            emit serverMessage(m_serverName, text);
        return;
    }

    bool isNumeric = false;
    cmd.toInt(&isNumeric);
    if (isNumeric) {
        handleNumeric(cmd, msg.params, msg.trailing);
        return;
    }

    // If this message belongs to an active batch, buffer it
    const QString batchRef = msg.tags.value("batch");
    if (!batchRef.isEmpty() && m_batches.contains(batchRef)) {
        auto &batch = m_batches[batchRef];
        if (batch.msgs.size() >= kMaxBatchMessages) {
            emit errorMessage(m_serverName, "IRC batch too large; dropping extra messages");
            return;
        }
        BatchInfo::Msg bm;
        bm.command    = cmd;
        bm.nick       = msg.nick;
        bm.params     = msg.params;
        bm.trailing   = msg.trailing;
        bm.serverTime = msg.serverTime;
        bm.msgid      = msg.tags.value("msgid");
        bm.replyTo    = msg.tags.value("+draft/reply");
        bm.concat     = msg.tags.contains("draft/multiline-concat");
        batch.msgs.append(bm);
        return;
    }

    const QDateTime serverTime = msg.serverTime;

    // PRIVMSG / CTCP
    if (cmd == "PRIVMSG" && msg.params.size() >= 1) {
        const QString target = msg.params[0];
        const QString text   = msg.params.size() >= 2 ? msg.params.last() : msg.trailing;

        const QString msgid   = msg.tags.value("msgid");
        const QString replyTo = msg.tags.value("+draft/reply");
        const QString account = msg.tags.value("account");
        if (!account.isEmpty())
            emit accountChanged(m_serverName, msg.nick, account);
        if (text.startsWith('\x01') && text.endsWith('\x01')) {
            const QString ctcp    = text.mid(1, text.size() - 2);
            const QString ctcpCmd = ctcp.section(' ', 0, 0).toUpper();
            const bool selfEcho   = (msg.nick == m_nick);
            if (ctcpCmd == "ACTION") {
                emit actionReceived(m_serverName, target,
                                    msg.nick, ctcp.mid(7), serverTime, false, msgid);
            } else if (!selfEcho && ctcpCmd == "VERSION") {
                const QString rkey = msg.nick + ":VERSION";
                const qint64  now  = QDateTime::currentMSecsSinceEpoch();
                if (now - m_ctcpTimestamps.value(rkey, 0) >= 5000) {
                    if (m_ctcpTimestamps.size() >= 500)
                        m_ctcpTimestamps.erase(m_ctcpTimestamps.begin());
                    m_ctcpTimestamps.insert(rkey, now);
                    sendRaw("NOTICE " + msg.nick + " :\x01VERSION Uplink " UPLINK_VERSION_FULL "\x01");
                    emit serverMessage(m_serverName, "CTCP VERSION from " + msg.nick);
                }
            } else if (!selfEcho && ctcpCmd == "PING") {
                const QString rkey = msg.nick + ":PING";
                const qint64  now  = QDateTime::currentMSecsSinceEpoch();
                if (now - m_ctcpTimestamps.value(rkey, 0) >= 5000) {
                    if (m_ctcpTimestamps.size() >= 500)
                        m_ctcpTimestamps.erase(m_ctcpTimestamps.begin());
                    m_ctcpTimestamps.insert(rkey, now);
                    QString payload = stripCrlf(ctcp.section(' ', 1).left(32));
                    payload.remove(QChar(1)).remove('\0'); // strip CTCP delimiters and nulls
                    sendRaw("NOTICE " + msg.nick + " :\x01PING " + payload + "\x01");
                }
            } else if (!selfEcho && ctcpCmd == "TIME") {
                const QString rkey = msg.nick + ":TIME";
                const qint64  now  = QDateTime::currentMSecsSinceEpoch();
                if (now - m_ctcpTimestamps.value(rkey, 0) >= 5000) {
                    if (m_ctcpTimestamps.size() >= 500)
                        m_ctcpTimestamps.erase(m_ctcpTimestamps.begin());
                    m_ctcpTimestamps.insert(rkey, now);
                    const QString localTime = QDateTime::currentDateTime().toString(Qt::RFC2822Date);
                    sendRaw("NOTICE " + msg.nick + " :\x01TIME " + localTime + "\x01");
                    emit serverMessage(m_serverName, "CTCP TIME from " + msg.nick);
                }
            } else if (ctcpCmd == "DCC" && msg.nick != m_nick) {
                const QString dccType = ctcp.section(' ', 1, 1).toUpper();
                if (dccType == "SEND") {
                    // Support quoted filenames: DCC SEND "my file.txt" ip port size [token]
                    const QString afterSend = ctcp.section(' ', 2).trimmed();
                    QString fn;
                    QString remainder;
                    if (afterSend.startsWith('"')) {
                        const qsizetype closeQuote = afterSend.indexOf('"', 1);
                        if (closeQuote > 1) {
                            fn = afterSend.mid(1, closeQuote - 1);
                            remainder = afterSend.mid(closeQuote + 1).trimmed();
                        }
                    }
                    if (fn.isEmpty()) {
                        fn = afterSend.section(' ', 0, 0);
                        remainder = afterSend.section(' ', 1).trimmed();
                    }
                    static const QRegularExpression kCtrlChars(QStringLiteral("[\\x00-\\x1f\\x7f]"));
                    fn.remove(kCtrlChars);
                    bool ok1, ok2, ok3;
                    const quint32 ip       = remainder.section(' ', 0, 0).toUInt(&ok1);
                    const quint16 port     = remainder.section(' ', 1, 1).toUShort(&ok2);
                    const qint64  filesize = remainder.section(' ', 2, 2).toLongLong(&ok3);
                    const QString token    = remainder.section(' ', 3, 3);
                    if (ok1 && ok2 && ok3 && !fn.isEmpty() && filesize > 0) {
                        if (port == 0 && !token.isEmpty())
                            emit dccPassiveOfferReceived(m_serverName, msg.nick, fn, ip, filesize, token);
                        else if (port != 0 && !token.isEmpty())
                            emit dccPassiveSendReply(m_serverName, msg.nick, fn, ip, port, filesize, token);
                        else if (port != 0)
                            emit dccSendReceived(m_serverName, msg.nick, fn, ip, port, filesize);
                        else
                            emit serverMessage(m_serverName, "DCC SEND from " + msg.nick + " (malformed)");
                    } else
                        emit serverMessage(m_serverName, "DCC SEND from " + msg.nick + " (malformed)");
                } else {
                    emit serverMessage(m_serverName, "DCC " + dccType + " from " + msg.nick + " (unsupported)");
                }
            } else {
                emit serverMessage(m_serverName, "CTCP " + ctcpCmd + " from " + msg.nick);
            }
        } else {
            emit messageReceived(m_serverName, target, msg.nick, text, serverTime, false, msgid, replyTo);
        }
        return;
    }

    if (cmd == "NOTICE" && msg.params.size() >= 1) {
        const QString text = msg.params.size() >= 2 ? msg.params.last() : msg.trailing;
        const QString noticeAccount = msg.tags.value("account");
        if (!noticeAccount.isEmpty())
            emit accountChanged(m_serverName, msg.nick, noticeAccount);
        if (text.startsWith('\x01') && text.endsWith('\x01')) {
            const QString ctcp    = text.mid(1, text.size() - 2);
            const QString ctcpCmd = ctcp.section(' ', 0, 0).toUpper();
            if (ctcpCmd == "PING") {
                bool ok = false;
                const qint64 sent = ctcp.section(' ', 1).toLongLong(&ok);
                const qint64 rtt  = QDateTime::currentMSecsSinceEpoch() - sent;
                emit ctcpPingReply(m_serverName, msg.nick, ok ? rtt : -1);
            } else if (ctcpCmd == "VERSION") {
                emit ctcpVersionReply(m_serverName, msg.nick, ctcp.section(' ', 1));
            } else if (ctcpCmd == "TIME") {
                emit ctcpTimeReply(m_serverName, msg.nick, ctcp.section(' ', 1));
            } else {
                emit noticeReceived(m_serverName, msg.params[0], msg.nick,
                                    "CTCP reply: " + ctcp, serverTime, false, {}, {});
            }
        } else {
            emit noticeReceived(m_serverName, msg.params[0], msg.nick, text, serverTime, false,
                                msg.tags.value("msgid"), msg.tags.value("+draft/reply"));
        }
        return;
    }

    if (cmd == "TAGMSG" && !msg.params.isEmpty()) {
        const QString target = msg.params[0];
        QString typing = msg.tags.value("typing");
        if (typing.isEmpty()) typing = msg.tags.value("+typing");
        if (!typing.isEmpty() && msg.nick != m_nick)
            emit typingReceived(m_serverName, target, msg.nick, typing);
        const QString emoji = msg.tags.value("+draft/react");
        if (!emoji.isEmpty()) {
            const QString replyTo = msg.tags.value("+draft/reply");
            emit reactReceived(m_serverName, target, msg.nick, replyTo, emoji);
        }
        return;
    }

    if (cmd == "CHGHOST" && msg.params.size() >= 2) {
        emit hostChanged(m_serverName, msg.nick, msg.params[0], msg.params[1]);
        return;
    }

    if (cmd == "ACCOUNT" && !msg.params.isEmpty()) {
        emit accountChanged(m_serverName, msg.nick, msg.params[0]);
        return;
    }

    // draft/metadata-2 — server-push notification: METADATA <target> <key> <visibility> :<value>
    // (metadata-3 drops this verb; its pushes arrive as numeric 761 instead)
    // ircmsg only adds trailing colon when value has spaces — plain URLs land in params[3]
    if (cmd == "METADATA" && msg.params.size() >= 2) {
        const QString value = !msg.trailing.isEmpty() ? msg.trailing : msg.params.value(3);
        routeMetadata(msg.params[0], msg.params[1], value);
        return;
    }

    if (cmd == "SETNAME" && !msg.trailing.isEmpty()) {
        emit setNameReceived(m_serverName, msg.nick, msg.trailing);
        return;
    }

    if (cmd == "REDACT" && msg.params.size() >= 2) {
        emit messageRedacted(m_serverName, msg.nick, msg.params[0], msg.params[1], msg.trailing);
        return;
    }

    if (cmd == "INVITE" && msg.params.size() >= 1) {
        const QString targetNick = msg.params[0];
        const QString channel    = msg.params.size() > 1 ? msg.params[1] : msg.trailing;
        emit inviteNotify(m_serverName, msg.nick, channel, targetNick);
        return;
    }

    if (cmd == "JOIN") {
        const QString channel = msg.params.isEmpty() ? msg.trailing : msg.params[0];
        if (msg.nick == m_nick) {
            emit serverMessage(m_serverName, "Joined " + channel);
            if (m_ackedCaps.contains("no-implicit-names") ||
                m_ackedCaps.contains("soju.im/no-implicit-names"))
                sendRaw("NAMES " + channel);
        }
        emit userJoined(m_serverName, channel, msg.nick, msg.user, msg.host);
        // extended-join: params[1] = account, trailing = realname
        if (msg.params.size() > 1) {
            const QString account = msg.params[1];
            if (!account.isEmpty() && account != "*")
                emit accountChanged(m_serverName, msg.nick, account);
        }
        return;
    }

    if (cmd == "PART" && !msg.params.isEmpty()) {
        emit userParted(m_serverName, msg.params[0], msg.nick, msg.user, msg.host, msg.trailing);
        return;
    }

    if (cmd == "QUIT") {
        emit userQuit(m_serverName, msg.nick, msg.user, msg.host, msg.trailing);
        return;
    }

    if (cmd == "NICK") {
        const QString newNick = msg.params.isEmpty() ? msg.trailing : msg.params[0];
        if (newNick.toLower() == m_nick.toLower() || msg.nick.toLower() == m_nick.toLower()) {
            m_nick = newNick;
            emit selfNickChanged(m_serverName, newNick);
        }
        emit nickChanged(m_serverName, msg.nick, newNick);
        return;
    }

    if (cmd == "KICK" && msg.params.size() >= 2) {
        emit kicked(m_serverName, msg.params[0], msg.params[1], msg.nick, msg.trailing);
        return;
    }

    if (cmd == "MODE" && !msg.params.isEmpty()) {
        QStringList modeParts = msg.params.mid(1);
        if (!msg.trailing.isEmpty()) modeParts << msg.trailing;
        emit modesReceived(m_serverName, msg.params[0], modeParts.join(' '));
        return;
    }

    if (cmd == "TOPIC" && !msg.params.isEmpty()) {
        emit topicReceived(m_serverName, msg.params[0], msg.trailing);
        return;
    }

    if (cmd == "WALLOPS") {
        const QString text = msg.trailing.isEmpty() ? msg.params.value(0) : msg.trailing;
        emit wallopsReceived(m_serverName, msg.nick, text);
        return;
    }

}

// ---------------------------------------------------------------------------
// BATCH
// ---------------------------------------------------------------------------

void IrcClient::handleBatch(const QStringList &params)
{
    if (params.isEmpty()) return;
    const QString &refArg = params[0];

    if (refArg.startsWith('+')) {
        if (m_batches.size() >= kMaxOpenBatches) {
            emit errorMessage(m_serverName, "Too many open IRC batches; ignoring");
            return;
        }
        const QString ref   = refArg.mid(1);
        const QString type  = params.size() > 1 ? params[1] : QString();
        const QString param = params.size() > 2 ? params.mid(2).join(' ') : QString();
        BatchInfo bi;
        bi.type  = type;
        bi.param = param;
        m_batches.insert(ref, bi);
    } else if (refArg.startsWith('-')) {
        deliverBatch(refArg.mid(1));
    }
}

void IrcClient::deliverBatch(const QString &ref)
{
    if (!m_batches.contains(ref)) return;
    const BatchInfo batch = m_batches.take(ref);
    if (batch.type == "draft/multiline") {
        // Assemble lines into a single multi-line message.
        // Lines tagged draft/multiline-concat are appended without a newline prefix.
        const QString target = batch.param;
        QString assembled;
        QString nick;
        QDateTime serverTime;
        QString msgid;
        QString replyTo;
        for (const BatchInfo::Msg &bm : batch.msgs) {
            if (bm.command != "PRIVMSG") continue;
            if (nick.isEmpty()) {
                nick       = bm.nick;
                serverTime = bm.serverTime;
                msgid      = bm.msgid;
                replyTo    = bm.replyTo;
            }
            if (!assembled.isEmpty())
                assembled += bm.concat ? QString{} : QStringLiteral("\n");
            assembled += bm.trailing;
        }
        if (!assembled.isEmpty())
            emit messageReceived(m_serverName, target, nick, assembled, serverTime, false, msgid, replyTo);
        return;
    }

    if (batch.type == "netsplit") {
        QStringList nicks;
        for (const auto &bm : batch.msgs)
            if (!bm.nick.isEmpty()) nicks.append(bm.nick);
        emit netsplitDetected(m_serverName, batch.param, nicks);
        return;
    }
    if (batch.type == "netjoin") {
        QStringList channels, nicks;
        for (const auto &bm : batch.msgs) {
            if (bm.command == "JOIN" && !bm.params.isEmpty()) {
                channels.append(bm.params[0]);
                nicks.append(bm.nick);
            }
        }
        if (!channels.isEmpty())
            emit netjoinDetected(m_serverName, batch.param, channels, nicks);
        return;
    }

    const bool isHistory = (batch.type == "chathistory" ||
                            batch.type == "draft/chathistory" ||
                            batch.type == "znc.in/batch/playback");

    for (const BatchInfo::Msg &bm : batch.msgs) {
        if (bm.command == "PRIVMSG" && bm.params.size() >= 1) {
            const QString target = bm.params[0];
            const QString text   = bm.params.size() >= 2 ? bm.params.last() : bm.trailing;
            if (text.startsWith('\x01') && text.endsWith('\x01')) {
                const QString ctcp = text.mid(1, text.size() - 2);
                if (ctcp.startsWith("ACTION "))
                    emit actionReceived(m_serverName, target, bm.nick,
                                        ctcp.mid(7), bm.serverTime, isHistory, bm.msgid);
            } else {
                emit messageReceived(m_serverName, target, bm.nick,
                                     text, bm.serverTime, isHistory, bm.msgid, bm.replyTo);
            }
        } else if (bm.command == "NOTICE" && bm.params.size() >= 1) {
            const QString btext = bm.params.size() >= 2 ? bm.params.last() : bm.trailing;
            emit noticeReceived(m_serverName, bm.params[0], bm.nick,
                                btext, bm.serverTime, isHistory, bm.msgid, bm.replyTo);
        }
    }

    // Reply batches to our CHATHISTORY requests only — znc.in/batch/playback
    // is unsolicited push playback, not a completion of anything we asked for.
    if (batch.type == "chathistory" || batch.type == "draft/chathistory")
        emit historyBatchDone(m_serverName, batch.param);
}

// ---------------------------------------------------------------------------
// BOUNCER (soju)
// ---------------------------------------------------------------------------

void IrcClient::handleBouncer(const QStringList &params, const QString &trailing)
{
    Q_UNUSED(trailing)
    if (params.isEmpty()) return;
    const QString sub = params[0].toUpper();

    if (sub == "NETWORK" && params.size() >= 2) {
        QHash<QString,QString> attrs;
        for (const QString &part : params[1].split(';', Qt::SkipEmptyParts)) {
            const qsizetype eq = part.indexOf('=');
            if (eq != -1) attrs.insert(part.left(eq), part.mid(eq+1));
        }
        const QString id    = attrs.value("id");
        const QString name  = attrs.value("name").isEmpty() ? id : attrs.value("name");
        const QString state = attrs.value("state");
        if (!id.isEmpty())
            m_bouncerNetBuf.append({id, name, state});
        if (!m_bouncerListing) {
            // live state-change notify (not the initial listing)
            emit bouncerNetworkReceived(m_serverName, id, name, state == "connected");
        }
    } else if (sub == "LISTNETWORKS" && params.size() >= 2
               && params[1].toUpper() == "END") {
        m_bouncerListing = false;
        emit bouncerNetworksListed(m_serverName, m_bouncerNetBuf);
        m_bouncerNetBuf.clear();
    }
}

// ---------------------------------------------------------------------------
// CAP negotiation
// ---------------------------------------------------------------------------

// Route a metadata push/reply (METADATA verb, numeric 761, numeric 766) to
// the right signal. Channel targets carry only the avatar key; everything
// else is per-user profile metadata.
void IrcClient::routeMetadata(QString target, const QString &key, const QString &value)
{
    if (target == QLatin1String("*")) target = m_nick;
    if (isChannelName(target)) {
        if (key == "avatar")
            emit channelMetaChanged(m_serverName, target, key, value);
    } else if (key == "display-name" || key == "avatar" || key == "status") {
        emit userMetaChanged(m_serverName, target, key, value);
    }
}

// Capabilities we REQ when the server offers them — shared by the CAP LS
// and CAP NEW paths so the list can't drift between them.
QStringList IrcClient::desiredCaps() const
{
    QStringList desired = {
        "multi-prefix", "away-notify", "server-time",
        "message-tags", "batch", "labeled-response", "typing", "draft/typing",
        "echo-message",
        "chathistory", "draft/chathistory", "chghost", "draft/react",
        "account-notify", "account-tag", "extended-join", "invite-notify", "setname",
        "userhost-in-names", "draft/message-redaction", "draft/multiline",
        "draft/metadata-2", "draft/metadata-3", "draft/read-marker",
        "no-implicit-names", "cap-notify", "standard-replies",
        "draft/persistence",
    };
    // Metadata off for this server: never REQ the caps, so hasMetadataCap()
    // stays false and every SET/GET/SUB site below it goes quiet on its own.
    if (!m_metadataEnabled) {
        desired.removeAll("draft/metadata-2");
        desired.removeAll("draft/metadata-3");
    }
    if (m_bouncerType == BouncerType::ZNC)
        desired << "znc.in/playback" << "znc.in/self-message" << "znc.in/batch";
    if (m_bouncerType == BouncerType::Soju)
        desired << "soju.im/bouncer-networks" << "soju.im/bouncer-networks-notify"
                << "soju.im/read" << "soju.im/no-implicit-names";
    if ((!m_saslUser.isEmpty() && !m_saslPassword.isEmpty()) || m_saslExternal)
        desired << "sasl";
    return desired;
}

void IrcClient::handleCap(const QStringList &params, const QString &trailing)
{
    if (params.size() < 2) return;
    const QString subCmd = params[1].toUpper();

    if (subCmd == "LS") {
        // Buffer multi-line responses (params[2] == "*" means more lines coming).
        // Cap the total: a hostile server could stream "LS *" continuations forever.
        constexpr qsizetype kMaxCapTokens = 512;
        const bool more = (params.size() >= 3 && params[2] == "*");
        if (m_capLsBuffer.size() < kMaxCapTokens)
            m_capLsBuffer += trailing.split(' ', Qt::SkipEmptyParts);
        if (more) return;

        const QStringList available = m_capLsBuffer;
        m_capLsBuffer.clear();

        // Servers may advertise caps as "name=value" (e.g. "sasl=PLAIN,EXTERNAL")
        auto hasAvailable = [&](const QString &cap) {
            for (const QString &a : available)
                if (a == cap || a.startsWith(cap + "="))
                    return true;
            return false;
        };

        // STS (Strict Transport Security)
        for (const QString &a : available) {
            if (a != "sts" && !a.startsWith("sts=")) continue;
            const QString val = a.startsWith("sts=") ? a.mid(4) : QString();
            quint16 stsPort     = 0;
            qint64  stsDuration = -1;
            for (const QString &kv : val.split(',', Qt::SkipEmptyParts)) {
                if (kv.startsWith("port="))
                    stsPort = kv.mid(5).toUShort();
                else if (kv.startsWith("duration="))
                    // Clamp to a year: an absurd duration would overflow the
                    // epoch addition in StsStore and corrupt the pin's expiry.
                    stsDuration = qMin(kv.mid(9).toLongLong(),
                                       qint64(365) * 24 * 3600);
            }
            if (stsDuration == 0) {
                StsStore::remove(m_host);
            } else if (stsDuration > 0) {
                if (m_ssl) {
                    // Already on TLS — refresh the stored policy
                    StsStore::store(m_host, m_port, stsDuration);
                } else if (stsPort > 0 && !m_useWs) {
                    // Plain connection — store policy and reconnect over TLS.
                    // Skipped on WebSocket: the upgrade path drives m_socket,
                    // which would leave a zombie TCP connection beside the ws.
                    StsStore::store(m_host, stsPort, stsDuration);
                    m_ssl  = true;
                    m_port = stsPort;
                    // m_stsUpgrade must be set BEFORE abort() — abort() may call
                    // onDisconnected() synchronously on some platforms, and that slot
                    // checks m_stsUpgrade to suppress the normal reconnect schedule.
                    // The singleShot lambda fires after onDisconnected() clears the flag,
                    // so connectToHostEncrypted() always runs in UnconnectedState.
                    m_stsUpgrade = true;
                    emit serverMessage(m_serverName, QString("STS: upgrading to TLS on port %1").arg(stsPort));
                    QTimer::singleShot(0, this, [this]() {
                        if (sockState() != QAbstractSocket::UnconnectedState)
                            sockAbort();
                        applyProxy();
                        m_socket->connectToHostEncrypted(m_host, m_port);
                    });
                }
            }
            break;
        }

        const QStringList desired = desiredCaps();

        QStringList want;
        for (const QString &cap : std::as_const(desired)) {
            // metadata-3 supersedes metadata-2 — request only the newer revision
            if (cap == "draft/metadata-2" && hasAvailable("draft/metadata-3"))
                continue;
            if (hasAvailable(cap))
                want << cap;
        }

        if (!want.isEmpty()) {
            m_requestedCaps = QSet<QString>(want.begin(), want.end());
            sendRaw("CAP REQ :" + want.join(' '));
        } else {
            sendRaw("CAP END");
        }
        return;
    }

    if (subCmd == "ACK") {
        const QStringList acked = trailing.split(' ', Qt::SkipEmptyParts);
        const bool hadMetadata = hasMetadataCap();
        for (const QString &cap : acked) {
            // An ACKed "-cap" confirms the cap was disabled, not enabled
            if (cap.startsWith('-'))
                m_ackedCaps.remove(cap.mid(1));
            else
                m_ackedCaps.insert(cap);
        }
        // Metadata gained mid-session (CAP NEW → REQ → ACK) — the 001
        // handler already ran, so subscribe here or pushes never arrive.
        if (m_registered && !hadMetadata && hasMetadataCap())
            sendRaw("METADATA * SUB display-name avatar status");

        if (!m_registered) {
            if (acked.contains("sasl") &&
                ((!m_saslUser.isEmpty() && !m_saslPassword.isEmpty()) || m_saslExternal)) {
                m_saslPending = true;
                sendRaw(m_saslExternal ? "AUTHENTICATE EXTERNAL" : "AUTHENTICATE PLAIN");
            } else if (!m_requestedCaps.contains("sasl") || m_ackedCaps.contains("sasl")) {
                sendRaw("CAP END");
            }
            // else: sasl was requested but not yet acked — wait for the next ACK
        }


        // Request soju network list once caps are acked and SASL is not in-flight.
        // Skip if BOUNCER LISTNETWORKS was already rejected this session.
        if (m_bouncerType == BouncerType::Soju &&
            m_ackedCaps.contains("soju.im/bouncer-networks") &&
            !m_saslPending && !m_bouncerListing && !m_bouncerListingUnsupported) {
            m_bouncerListing = true;
            sendRaw("BOUNCER LISTNETWORKS");
        }
        return;
    }

    if (subCmd == "NAK") {
        if (!m_registered)
            sendRaw("CAP END");
        return;
    }

    if (subCmd == "NEW") {
        // Server added capabilities mid-session — request any we want
        const QStringList newCaps = trailing.split(' ', Qt::SkipEmptyParts);
        const QStringList desired = desiredCaps();

        QStringList want;
        for (const QString &cap : newCaps) {
            const QString name = cap.section('=', 0, 0);
            if (name == "draft/metadata-2" && m_ackedCaps.contains("draft/metadata-3"))
                continue;
            if (desired.contains(name) && !m_ackedCaps.contains(name))
                want << name;
        }
        // Both metadata drafts advertised in one CAP NEW — take only v3
        if (want.contains("draft/metadata-3"))
            want.removeAll("draft/metadata-2");
        if (!want.isEmpty())
            sendRaw("CAP REQ :" + want.join(' '));
        return;
    }

    if (subCmd == "DEL") {
        // Server removed capabilities mid-session
        const QStringList delCaps = trailing.split(' ', Qt::SkipEmptyParts);
        for (const QString &cap : delCaps)
            m_ackedCaps.remove(cap);
        return;
    }
}

// ---------------------------------------------------------------------------
// Numeric handlers
// ---------------------------------------------------------------------------

void IrcClient::handleNumeric(const QString &cmd, const QStringList &params, const QString &trailing)
{
    const int n = cmd.toInt();

    switch (n) {
    case 1:   // RPL_WELCOME
        m_registered = true;
        emit selfNickChanged(m_serverName, m_nick);
        emit connected(m_serverName);
        emit serverMessage(m_serverName, trailing);
        if (!m_nickservPassword.isEmpty()) {
            if (!m_ssl)
                emit serverMessage(m_serverName, "Warning: sending NickServ password over unencrypted connection");
            sendRaw("PRIVMSG NickServ :IDENTIFY " + m_nickservPassword);
            emit serverMessage(m_serverName, "Sent NickServ IDENTIFY");
        }
        // ZNC: request playback of all buffers since last seen
        if (m_bouncerType == BouncerType::ZNC &&
            m_ackedCaps.contains("znc.in/playback")) {
            sendRaw("PRIVMSG *playback :PLAY * 0");
        }
        if (!m_monitorList.isEmpty())
            sendRaw("MONITOR + " + m_monitorList.join(','));
        if (hasMetadataCap())
            sendRaw("METADATA * SUB display-name avatar status");
        // Registration bumped us off the configured nick (433 fallback). If
        // SASL says we own it, take it back — once; a refusal just leaves the
        // fallback nick and the normal in-use error. Raw send on purpose:
        // m_nick must only change via the NICK echo if the server agrees.
        if (m_nickFallback) {
            m_nickFallback = false;
            if (m_saslAuthed && m_nick.compare(m_wantedNick, Qt::CaseInsensitive) != 0)
                sendRaw("NICK " + m_wantedNick);
        }
        break;

    case 2:   // RPL_YOURHOST
    case 3:   // RPL_CREATED
    case 4:   // RPL_MYINFO
    case 5: { // RPL_ISUPPORT
        // params[0] = our nick, params[1..n-1] = tokens, trailing = "are supported..."
        for (int i = 1; i < params.size(); ++i) {
            if (params[i].compare("UTF8ONLY", Qt::CaseInsensitive) == 0) {
                m_utf8Only = true;
                emit serverMessage(m_serverName, "UTF8ONLY: server enforces UTF-8 encoding");
            } else if (params[i].compare("WHOX", Qt::CaseInsensitive) == 0) {
                m_supportsWhox = true;
            }
        }
        emit serverMessage(m_serverName, trailing);
        break;
    }
    case 251: case 252: case 253: case 254: case 255:
    case 265: case 266:
    case 372: // RPL_MOTD
    case 375: // RPL_MOTDSTART
    case 376: // RPL_ENDOFMOTD
    case 422: // ERR_NOMOTD
        emit serverMessage(m_serverName, trailing);
        break;

    case 324: // RPL_CHANNELMODEIS
        if (params.size() >= 3) {
            QStringList modeParts = params.mid(2);
            emit modesReceived(m_serverName, params[1], modeParts.join(' '));
        }
        break;

    case 332: // RPL_TOPIC
        if (params.size() >= 2)
            emit topicReceived(m_serverName, params[1], trailing);
        break;

    case 315: break; // RPL_ENDOFWHO — suppress

    case 333: // RPL_TOPICWHOTIME
        if (params.size() >= 4) {
            bool ok = false;
            const quint64 ts = params[3].toULongLong(&ok);
            if (ok)
                emit topicSetByReceived(m_serverName, params[1], params[2], ts);
        }
        break;

    case 353: { // RPL_NAMREPLY
        if (params.size() >= 3) {
            // Bound server-driven growth: a hostile server could stream 353s
            // for unlimited distinct channels, or endless nicks for one.
            constexpr qsizetype kMaxNamesChannels = 64;
            constexpr qsizetype kMaxNamesNicks    = 50000;
            const QString key = params[2].toLower();
            if (m_namesBuffer.contains(key) || m_namesBuffer.size() < kMaxNamesChannels) {
                QStringList &buf = m_namesBuffer[key];
                if (buf.size() < kMaxNamesNicks)
                    buf += trailing.split(' ', Qt::SkipEmptyParts);
            }
        }
        break;
    }

    case 352: { // RPL_WHOREPLY — <client> <channel> <user> <host> <server> <nick> <flags> :<hop> <real>
        if (params.size() >= 7) {
            const QString &channel = params[1];
            const QString &nick    = params[5];
            const QString &flags   = params[6];
            if (isChannelName(channel))
                emit whoEntryReceived(m_serverName, channel, nick, flags);
        }
        break;
    }

    case 354: { // RPL_WHOSPCRPL — server may or may not include querytype token
        if (params.size() < 4) break;
        const bool hasToken = params.size() >= 6 && params[1] == QLatin1String("42");
        const int b = hasToken ? 2 : 1;
        if (params.size() >= b + 3) {
            const QString &channel = params[b];
            const QString &nick    = params[b + 1];
            const QString &flags   = params[b + 2];
            const QString account  = (params.size() > b + 3) ? params[b + 3] : QString();
            emit whoEntryReceived(m_serverName, channel, nick, flags);
            if (!account.isEmpty() && account != "0" && account != "*")
                emit accountChanged(m_serverName, nick, account);
        }
        break;
    }

    case 366: { // RPL_ENDOFNAMES
        if (params.size() >= 2) {
            const QString &channel = params[1];
            emit namesReceived(m_serverName, channel, m_namesBuffer.take(channel.toLower()));
            emit namesDone(m_serverName, channel);
            sendRaw("MODE " + channel);
            if (m_supportsWhox)
                sendRaw("WHO " + channel + " %tcnfa,42");
            else
                sendRaw("WHO " + channel);
            // Request chat history on join
            if (m_ackedCaps.contains("chathistory") || m_ackedCaps.contains("draft/chathistory"))
                requestHistory(channel, 100);
        }
        break;
    }

    case 900: // RPL_LOGGEDIN
        emit serverMessage(m_serverName, trailing);
        break;

    case 903: // RPL_SASLSUCCESS
        emit serverMessage(m_serverName, "SASL authentication successful");
        m_saslAuthed  = true;
        m_saslPending = false;
        sendRaw("CAP END");
        break;

    case 904: case 905: case 906:
        emit serverMessage(m_serverName, "SASL authentication failed: " + trailing);
        m_saslPending = false;
        sendRaw("CAP END");
        break;

    case 433: // ERR_NICKNAMEINUSE
        if (!m_registered) {
            m_nickFallback = true;
            setNick(m_nick + "_");
        } else {
            emit errorMessage(m_serverName, "Nickname " + params.value(1, m_nick) + " is already in use");
        }
        break;

    case 431: case 432:
        emit errorMessage(m_serverName, "Nick error: " + trailing);
        break;

    case 730: { // RPL_MONONLINE
        QStringList nicks;
        for (const QString &part : trailing.split(',', Qt::SkipEmptyParts)) {
            const QString nick = part.section('!', 0, 0).trimmed();
            if (!nick.isEmpty()) nicks << nick;
        }
        if (!nicks.isEmpty()) emit monitorOnline(m_serverName, nicks);
        break;
    }
    case 731: { // RPL_MONOFFLINE
        QStringList nicks;
        for (const QString &part : trailing.split(',', Qt::SkipEmptyParts)) {
            const QString nick = part.section('!', 0, 0).trimmed();
            // offline nicks have no !user@host, but handle both forms
            if (!nick.isEmpty()) nicks << nick;
        }
        if (!nicks.isEmpty()) emit monitorOffline(m_serverName, nicks);
        break;
    }
    case 732: // RPL_MONLIST
        emit serverMessage(m_serverName, "Monitor: " + trailing);
        break;
    case 733: // RPL_ENDOFMONLIST
        emit serverMessage(m_serverName, "End of monitor list.");
        break;
    case 734: // ERR_MONLISTFULL
        emit errorMessage(m_serverName, "Monitor list full: " + trailing);
        break;

    case 761: { // RPL_KEYVALUE — response to METADATA SET/GET: <client> <target> <key> <visibility> :<value>
        // params: [client, target, key, visibility, ?value], trailing = value
        // Ergo echoes target as "*" when you SET your own metadata; resolve to own nick
        // ircmsg only adds trailing colon when value has spaces — plain URLs land in params[4]
        if (params.size() >= 3)
            routeMetadata(params[1], params[2],
                          !trailing.isEmpty() ? trailing : params.value(4));
        break;
    }

    case 321: // RPL_LISTSTART
        m_listBuffer.clear();
        break;

    case 322: // RPL_LIST — <client> <channel> <count> :<topic>
        if (params.size() >= 3) {
            m_listBuffer.append({params[1], params[2], trailing});
            emit channelListEntry(m_serverName, params[1], params[2].toInt(), trailing);
        }
        break;

    case 323: // RPL_LISTEND
        emit channelListEnd(m_serverName, static_cast<int>(m_listBuffer.size()));
        emit channelListReceived(m_serverName, m_listBuffer);
        m_listBuffer.clear();
        break;

    case 391: { // RPL_TIME — <client> <server> [<timestamp>] :<human-readable>
        const QString srv  = params.size() >= 2 ? params[1] : m_host;
        const QString time = trailing.isEmpty() ? "(no time returned)" : trailing;
        emit contextualMessage(m_serverName, "Time on " + srv + ": " + time);
        break;
    }

    case 211: case 212: case 213: case 214: case 215:
    case 216: case 217: case 218: case 241: case 242:
    case 243: case 244: case 249: case 250: { // STATS reply lines
        QStringList parts = params.mid(1); // skip client nick
        if (!trailing.isEmpty()) parts << trailing;
        if (!parts.isEmpty()) emit contextualMessage(m_serverName, parts.join(' '));
        break;
    }
    case 219: { // RPL_ENDOFSTATS
        const QString query = params.size() >= 2 ? params[1] : QString();
        const QString msg   = trailing.isEmpty() ? "End of STATS" : trailing;
        emit contextualMessage(m_serverName, query.isEmpty() ? msg : msg + " (" + query + ")");
        break;
    }

    case 314: { // RPL_WHOWASUSER — <client> <nick> <user> <host> * :<realname>
        if (params.size() >= 4) {
            const QString line = params[1] + " was " + params[2] + "@" + params[3]
                + (trailing.isEmpty() ? QString() : " (" + trailing + ")");
            emit contextualMessage(m_serverName, line);
        }
        break;
    }
    case 305: // RPL_UNAWAY
        m_away = false;
        emit awayChanged(m_serverName, false);
        emit contextualMessage(m_serverName, "You are no longer away");
        break;

    case 306: // RPL_NOWAWAY
        m_away = true;
        emit awayChanged(m_serverName, true);
        emit contextualMessage(m_serverName, m_awayMsg.isEmpty()
            ? QString("You are now away")
            : "You are now away: " + m_awayMsg);
        break;

    case 301: // RPL_AWAY — <client> <nick> :<away message>
        if (params.size() >= 2 && !trailing.isEmpty())
            emit contextualMessage(m_serverName, params[1] + " is away: " + trailing);
        break;

    // WHOIS/WHOWAS replies — route to active channel/window
    case 307: // RPL_WHOISREGNICK
    case 313: // RPL_WHOISOPERATOR
    case 320: // RPL_WHOISSPECIAL
    case 671: // RPL_WHOISSECURE
        if (params.size() >= 2 && !trailing.isEmpty())
            emit contextualMessage(m_serverName, params[1] + " " + trailing);
        break;
    case 311: { // RPL_WHOISUSER: <client> <nick> <user> <host> * :<realname>
        if (params.size() >= 4)
            emit contextualMessage(m_serverName,
                params[1] + " (" + params[2] + "@" + params[3] + ") — " + trailing);
        break;
    }
    case 312: { // RPL_WHOISSERVER: <client> <nick> <server> :<info>
        if (params.size() >= 3)
            emit contextualMessage(m_serverName,
                params[1] + " on " + params[2] + " (" + trailing + ")");
        break;
    }
    case 317: { // RPL_WHOISIDLE: <client> <nick> <idle_secs> <signon_ts>
        if (params.size() >= 3) {
            const int secs = params[2].toInt();
            QString idle;
            if (secs >= 3600)
                idle = QString("%1h %2m idle").arg(secs/3600).arg((secs%3600)/60);
            else if (secs >= 60)
                idle = QString("%1m %2s idle").arg(secs/60).arg(secs%60);
            else
                idle = QString("%1s idle").arg(secs);
            if (params.size() >= 4) {
                const QDateTime dt = QDateTime::fromSecsSinceEpoch(params[3].toLongLong());
                idle += ", signed on " + dt.toString("ddd hh:mm");
            }
            emit contextualMessage(m_serverName, params[1] + " — " + idle);
        }
        break;
    }
    case 319: { // RPL_WHOISCHANNELS: <client> <nick> :<channels>
        if (params.size() >= 2 && !trailing.isEmpty())
            emit contextualMessage(m_serverName, params[1] + " is on: " + trailing);
        break;
    }
    case 330: { // RPL_WHOISACCOUNT: <client> <nick> <account> :is logged in as
        if (params.size() >= 3)
            emit contextualMessage(m_serverName, params[1] + " is logged in as " + params[2]);
        break;
    }
    case 318: // RPL_ENDOFWHOIS — suppress, replies already shown
    case 369: // RPL_ENDOFWHOWAS
        break;

    case 766: { // ERR_NOMATCHINGKEY — key not set; Ergo also pushes this to
                // subscribers when a key is deleted, so treat it as "cleared"
        if (params.size() >= 3)
            routeMetadata(params[1], params[2], QString());
        break;
    }
    case 768: // ERR_KEYNOTSET
        break;

    case 770: // RPL_METADATASUBOK — subscription confirmed, silent
    case 771: // RPL_METADATAUNSUBOK
        break;

    case 774: // RPL_METADATASYNCLATER — sync deferred. Ergo 2.19 sends this
              // after every SUB yet rejects the METADATA SYNC subcommand
              // (FAIL INVALID_PARAMS), so retrying is useless; the on-demand
              // hover fetch covers anything a missed push would have carried
        break;

    default:
        if (!trailing.isEmpty()) {
            if (n >= 400)
                emit errorMessage(m_serverName, trailing);
            else
                emit serverMessage(m_serverName, trailing);
        }
        break;
    }
}
