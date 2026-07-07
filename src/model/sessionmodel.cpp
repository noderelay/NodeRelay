#include "sessionmodel.h"
#include "irc/ircclient.h"
#include "config/keychainhelper.h"

#include <memory>
#include <QPointer>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QSet>
#include <QStandardPaths>
#include <QTimer>

SessionModel::SessionModel(QObject *parent)
    : QObject(parent)
{}

// Resolve any "<keychain>" sentinel fields asynchronously, then connect.
// If no sentinels are present the connection is started immediately.
static void resolveAndConnect(IrcClient *client, ServerConfig sc)
{
    static const QLatin1String kSentinel("<keychain>");

    auto shared    = std::make_shared<ServerConfig>(std::move(sc));
    auto remaining = std::make_shared<int>(0);
    QPointer<IrcClient> guard(client);

    auto done = [shared, remaining, guard]() {
        if (guard && --(*remaining) == 0)
            guard->connectToServer(*shared);
    };

    // All 4 server-level passwords share one bundle keychain item → one macOS prompt per server.
    const bool needBundle = (shared->password         == kSentinel ||
                             shared->saslPassword     == kSentinel ||
                             shared->nickservPassword == kSentinel ||
                             shared->proxyPass        == kSentinel);
    if (needBundle) {
        ++(*remaining);
        const QString bundleKey = shared->name + QLatin1String(":bundle");
        KeychainHelper::readAsync(bundleKey, [shared, guard, done](const QString &val) {
            if (!guard) return;
            if (!val.isEmpty()) {
                static const QLatin1String kSent("<keychain>");
                const QStringList parts = val.split(QChar('\x1F'));
                auto fill = [&](QString ServerConfig::*fp, int i) {
                    if (shared.get()->*fp == kSent)
                        shared.get()->*fp = parts.value(i);
                };
                fill(&ServerConfig::password,         0);
                fill(&ServerConfig::saslPassword,     1);
                fill(&ServerConfig::nickservPassword, 2);
                fill(&ServerConfig::proxyPass,        3);
            } else {
                emit guard->socketError(shared->name,
                    "Keychain: no credentials stored for \"" + shared->name +
                    "\" — open Edit Server and re-save");
            }
            done();
        });
    }

    for (int i = 0; i < shared->channels.size(); ++i) {
        if (shared->channels[i].password != kSentinel)
            continue;
        ++(*remaining);
        const QString key = shared->name + ":channel:" + shared->channels[i].name + ":key";
        KeychainHelper::readAsync(key, [shared, i, guard, done](const QString &val) {
            if (!guard) return;
            shared->channels[i].password = val;
            done();
        });
    }

    if (*remaining == 0)
        client->connectToServer(*shared);
}

static QRegularExpression buildHighlightRe(const QString &words)
{
    QStringList parts;
    for (const QString &w : words.split(',', Qt::SkipEmptyParts)) {
        const QString t = w.trimmed();
        if (!t.isEmpty())
            parts << "\\b" + QRegularExpression::escape(t) + "\\b";
    }
    if (parts.isEmpty()) return {};
    return QRegularExpression(parts.join('|'), QRegularExpression::CaseInsensitiveOption);
}

static QString sanitizeFilename(QString s)
{
    const QString bad = QStringLiteral("/\\:*?\"<>|");
    for (QChar c : bad)
        s.replace(c, '_');
    return s;
}

void SessionModel::spawnSession(const ServerConfig &sc, bool addToConfig)
{
    if (addToConfig)
        m_config.servers.append(sc);

    ServerSession sess;
    sess.name        = sc.name;
    sess.host        = sc.host;
    sess.nick        = sc.nick;
    sess.highlightRe = buildHighlightRe(m_config.ui.highlightWords);
    m_sessions.append(sess);
    emit serverAdded(ServerId{sc.name});

    auto *client = new IrcClient(this);
    attachClient(client, sc);
    m_clients.append(client);
    resolveAndConnect(client, sc);
}

void SessionModel::loadConfig(const Config &cfg)
{
    m_config = cfg;
    for (const auto &entry : cfg.ignoreList)
        m_ignoredNicks.insert(entry.nick, entry.flags);

    // Migrate log dirs from hostname-based to name-based layout
    const QString logsBase = QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
                             + "/.config/uplink/logs/";
    for (const ServerConfig &sc : cfg.servers) {
        if (sc.name.isEmpty() || sc.name == sc.host) continue;
        const QString oldDir = logsBase + sanitizeFilename(sc.host);
        const QString newDir = logsBase + sanitizeFilename(sc.name);
        if (QDir(oldDir).exists() && !QDir(newDir).exists())
            QDir().rename(oldDir, newDir);
    }

    for (const ServerConfig &sc : cfg.servers)
        if (!sc.disabled)
            spawnSession(sc, false);
}

void SessionModel::setHighlightWords(const QString &words)
{
    m_config.ui.highlightWords = words;
    const QRegularExpression re = buildHighlightRe(words);
    for (auto &sess : m_sessions)
        sess.highlightRe = re;
}

void SessionModel::setIgnore(const QString &nick, IgnoreTypes flags)
{
    m_ignoredNicks.insert(nick.toLower(), flags);
}

void SessionModel::clearIgnore(const QString &nick)
{
    m_ignoredNicks.remove(nick.toLower());
}

bool SessionModel::isIgnored(const QString &nick) const
{
    return m_ignoredNicks.contains(nick.toLower());
}

bool SessionModel::isIgnoredFor(const QString &nick, IgnoreType type) const
{
    return m_ignoredNicks.value(nick.toLower()) & type;
}

IgnoreTypes SessionModel::ignoreFlags(const QString &nick) const
{
    return m_ignoredNicks.value(nick.toLower());
}

void SessionModel::sendReact(ServerId host, BufferId target,
                              const QString &msgid, const QString &emoji)
{
    auto *cl = clientFor(ServerId{host});
    if (!cl) return;
    // No echo-message: locally apply only if the TAGMSG was actually sent.
    if (!cl->sendReact(target.str(), msgid, emoji)) {
        localMessage(host, target, "Cannot send reaction (message-tags cap not active)");
        return;
    }
    const QString nick = selfNick(host);
    if (!nick.isEmpty())
        onReactReceived(host.str(), target.str(), nick, msgid, emoji);
}

void SessionModel::sendRedact(ServerId host, BufferId target,
                               const QString &msgid, const QString &reason)
{
    if (auto *cl = clientFor(ServerId{host}))
        cl->sendRedact(target.str(), msgid, reason);
}

void SessionModel::monitorAdd(ServerId host, const QString &nick)
{
    if (auto *cl = clientFor(ServerId{host}))
        cl->monitorAdd(nick);
}

void SessionModel::monitorRemove(ServerId host, const QString &nick)
{
    if (auto *cl = clientFor(ServerId{host}))
        cl->monitorRemove(nick);
}

void SessionModel::monitorClear(ServerId host)
{
    if (auto *cl = clientFor(ServerId{host}))
        cl->monitorClear();
}

void SessionModel::monitorStatus(ServerId host)
{
    if (auto *cl = clientFor(ServerId{host}))
        cl->monitorStatus();
}

// ---------------------------------------------------------------------------
// Logging
// ---------------------------------------------------------------------------



void SessionModel::logMessage(const QString &host, const QString &target, const Message &msg)
{
    if (msg.isHistory) return;

    const QString logsDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
                            + "/.config/uplink/logs/"
                            + sanitizeFilename(host) + "/";
    const QString filePath = logsDir + sanitizeFilename(target) + ".log";

    QFile *f = m_logFiles.value(filePath, nullptr);
    if (!f) {
        QDir().mkpath(logsDir);
        QFile::setPermissions(logsDir, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
        f = new QFile(filePath, this);
        if (!f->open(QIODevice::Append | QIODevice::Text)) {
            delete f;
            return;
        }
        f->setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        m_logFiles.insert(filePath, f);
    }

    const QString ts = msg.timestamp.toLocalTime().toString("yyyy-MM-dd hh:mm:ss");

    const auto sanitize = [](const QString &s) {
        QString r = s;
        r.replace('\n', ' ');
        r.replace('\r', ' ');
        return r;
    };
    const QString safeNick = sanitize(msg.nick);
    const QString safeText = sanitize(msg.text);

    QString line;
    switch (msg.type) {
    case MessageType::Privmsg:
        line = "[" + ts + "] <" + safeNick + "> " + safeText + "\n";
        break;
    case MessageType::Action:
        line = "[" + ts + "] * " + safeNick + " " + safeText + "\n";
        break;
    case MessageType::Notice:
        line = "[" + ts + "] -" + safeNick + "- " + safeText + "\n";
        break;
    default:
        line = "[" + ts + "] -- " + safeText + "\n";
        break;
    }
    f->write(line.toUtf8());
}

QString SessionModel::logFilePath(ServerId host, BufferId target) const
{
    if (host.str().isEmpty() || target.str().isEmpty())
        return {};
    return QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
           + "/.config/uplink/logs/"
           + sanitizeFilename(host.str()) + "/"
           + sanitizeFilename(target.str()) + ".log";
}

void SessionModel::addServer(const ServerConfig &sc)
{
    if (sc.disabled) {
        m_config.servers.append(sc);
        return;
    }
    spawnSession(sc, true);
}

void SessionModel::removeServer(ServerId host)
{
    for (int i = 0; i < m_clients.size(); ++i) {
        if (m_clients[i]->serverName() == host.str()) {
            m_clients[i]->quit("Removed");
            m_clients[i]->deleteLater();
            m_clients.removeAt(i);
            break;
        }
    }
    m_sessions.removeIf([&](const ServerSession &s){ return s.name == host.str(); });
    m_config.servers.removeIf([&](const ServerConfig &s){ return s.name == host.str(); });
    emit serverDisconnected(host);

    const QString hostSeg = "/" + sanitizeFilename(host.str()) + "/";
    for (auto it = m_logFiles.begin(); it != m_logFiles.end(); ) {
        if (it.key().contains(hostSeg)) {
            it.value()->close();
            delete it.value();
            it = m_logFiles.erase(it);
        } else {
            ++it;
        }
    }
}

void SessionModel::closeServer(ServerId host)
{
    for (int i = 0; i < m_clients.size(); ++i) {
        if (m_clients[i]->serverName() == host.str()) {
            m_clients[i]->quit("Closing");
            m_clients[i]->deleteLater();
            m_clients.removeAt(i);
            break;
        }
    }
    m_sessions.removeIf([&](const ServerSession &s){ return s.name == host.str(); });

    const QString hostSeg = "/" + sanitizeFilename(host.str()) + "/";
    for (auto it = m_logFiles.begin(); it != m_logFiles.end(); ) {
        if (it.key().contains(hostSeg)) {
            it.value()->close();
            delete it.value();
            it = m_logFiles.erase(it);
        } else {
            ++it;
        }
    }

    emit serverClosed(host);
}

bool SessionModel::connectServer(ServerId host)
{
    if (session(host)) return true;
    for (const auto &sc : std::as_const(m_config.servers)) {
        if (sc.name == host.str() || sc.host == host.str()) {
            spawnSession(sc, false);
            return true;
        }
    }
    return false;
}

void SessionModel::updateServer(ServerId oldHost, const ServerConfig &sc)
{
    removeServer(oldHost);
    addServer(sc);
}

void SessionModel::closeBuffer(ServerId host, BufferId target)
{
    auto *sess = session(ServerId{host});
    if (!sess) return;

    const bool isChannel = target.str().startsWith('#') || target.str().startsWith('&');
    if (isChannel) {
        for (IrcClient *cl : m_clients)
            if (cl->serverName() == host.str()) { cl->part(target.str()); break; }
    }

    sess->channels.remove(target.str().toLower());

    const QString logPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
                            + "/.config/uplink/logs/"
                            + sanitizeFilename(host.str()) + "/"
                            + sanitizeFilename(target.str()) + ".log";
    if (auto *f = m_logFiles.take(logPath)) {
        f->close();
        delete f;
    }

    emit channelRemoved(host, target);
}

ServerSession *SessionModel::session(ServerId host)
{
    for (auto &s : m_sessions)
        if (s.name == host.str()) return &s;
    return nullptr;
}

Channel *SessionModel::channel(ServerId host, BufferId name)
{
    auto *s = session(ServerId{host});
    return s ? s->get(name.str()) : nullptr;
}

IrcClient *SessionModel::clientFor(ServerId host)
{
    for (IrcClient *cl : m_clients)
        if (cl->serverName() == host.str()) return cl;
    return nullptr;
}

void SessionModel::openPM(ServerId host, const QString &nick)
{
    auto *sess = session(ServerId{host});
    if (!sess || nick.isEmpty() || nick.startsWith('#') || nick.startsWith('&')) return;
    const bool isNew = !sess->get(nick);
    sess->getOrCreate(nick);
    if (isNew)
        emit channelAdded(host, BufferId{nick});
}

void SessionModel::sendMessage(ServerId host, BufferId target, const QString &text,
                               const QString &replyToMsgid)
{
    auto *cl = clientFor(ServerId{host});
    if (!cl) return;
    if (text.contains('\n'))
        cl->sendMultiline(target.str(), text, replyToMsgid);
    else
        cl->privmsg(target.str(), text, replyToMsgid);
    // Open a PM tab for outgoing private messages
    const bool isPM = !target.str().startsWith('#') && !target.str().startsWith('&')
                      && !target.str().startsWith('!') && target.str() != "(server)";
    if (isPM) openPM(host, target.str());
    // If echo-message is acked, the server will echo back the PRIVMSG with the
    // server-assigned msgid — use that echo as the display so msgid is set correctly.
    // Without echo-message, display immediately with no msgid (old behaviour).
    if (!cl->hasCap("echo-message")) {
        auto *sess = session(ServerId{host});
        if (sess) {
            QString display = text;
            if (target.str().compare("NickServ", Qt::CaseInsensitive) == 0) {
                const QString svcCmd = text.section(' ', 0, 0).toUpper();
                static const QStringList pwdCmds = {
                    "IDENTIFY", "REGISTER", "GHOST", "RECOVER", "RELEASE", "REGAIN", "SETPASS"
                };
                if (pwdCmds.contains(svcCmd))
                    display = svcCmd + " <redacted>";
            }
            postMessage(host.str(), target.str(), Message::make(MessageType::Privmsg, sess->nick, display, {}, false, {}, replyToMsgid));
        }
    }
}

void SessionModel::sendRaw(ServerId host, const QString &line)
{
    auto *cl = clientFor(ServerId{host});
    if (cl) cl->sendRaw(line);
}

void SessionModel::localMessage(ServerId host, BufferId target, const QString &text)
{
    postMessage(host.str(), target.str(), Message::make(MessageType::Server, "", text));
}

QString SessionModel::selfNick(ServerId host)
{
    auto *sess = session(ServerId{host});
    return sess ? sess->nick : QString{};
}

bool SessionModel::hasMention(ServerId host, BufferId ch)
{
    auto *c = channel(host, ch);
    return c && c->mentions > 0;
}

void SessionModel::sendJoin(ServerId host, BufferId channel, const QString &key)
{
    auto *cl = clientFor(ServerId{host});
    if (cl) cl->join(channel.str(), key);
}

void SessionModel::sendPart(ServerId host, BufferId channel, const QString &reason)
{
    auto *cl = clientFor(ServerId{host});
    if (cl) cl->part(channel.str(), reason);
}

void SessionModel::sendNick(ServerId host, const QString &nick)
{
    auto *cl = clientFor(ServerId{host});
    if (cl) cl->setNick(nick);
}

void SessionModel::sendAction(ServerId host, BufferId target, const QString &text)
{
    auto *cl = clientFor(ServerId{host});
    if (!cl) return;
    cl->privmsg(target.str(), "\x01""ACTION " + text + "\x01");
    if (!cl->hasCap("echo-message")) {
        if (auto *sess = session(ServerId{host}))
            postMessage(host.str(), target.str(), Message::make(MessageType::Action, sess->nick, text));
    }
}

void SessionModel::sendTyping(ServerId host, BufferId channel, const QString &state)
{
    if (auto *cl = clientFor(ServerId{host}))
        cl->sendTyping(channel.str(), state);
}

void SessionModel::setActive(ServerId host, BufferId ch)
{
    m_activeHost    = host;
    m_activeChannel = ch;
    // Clear unread on the newly active channel
    if (auto *c = channel(host, ch)) {
        c->unread         = 0;
        c->mentions       = 0;
        c->firstUnreadIdx = -1;
        emit unreadChanged(host, ch, 0);
    }
}

// ---------------------------------------------------------------------------
// Wire up a client to all our handler slots
// ---------------------------------------------------------------------------

void SessionModel::attachClient(IrcClient *cl, const ServerConfig &cfg)
{
    connect(cl, &IrcClient::connected,       this, &SessionModel::onConnected);
    connect(cl, &IrcClient::disconnected,    this, &SessionModel::onDisconnected);
    connect(cl, &IrcClient::socketError,     this, &SessionModel::onSocketError);
    connect(cl, &IrcClient::messageReceived, this, &SessionModel::onMessage);
    connect(cl, &IrcClient::noticeReceived,  this, &SessionModel::onNotice);
    connect(cl, &IrcClient::actionReceived,  this, &SessionModel::onAction);
    connect(cl, &IrcClient::bouncerNetworkReceived, this,
            [this](const QString &host, const QString &id,
                   const QString &name, bool connected){
        Q_UNUSED(id)
        // live state-change update (not the initial listing)
        postMessage(host, "(server)",
            Message::make(MessageType::Server, "",
                QString("Bouncer network: %1 [%2]").arg(name, connected ? "connected" : "offline")));
    });
    connect(cl, &IrcClient::bouncerNetworksListed, this,
            [this](const QString &host, const QList<QStringList> &networks) {
        if (networks.isEmpty()) {
            postMessage(host, "(server)",
                Message::make(MessageType::Server, "", "Bouncer: no networks configured."));
            return;
        }
        postMessage(host, "(server)",
            Message::make(MessageType::Server, "", "Bouncer networks:"));
        for (const QStringList &n : networks) {
            const QString name  = n.value(1).isEmpty() ? n.value(0) : n.value(1);
            const QString state = n.value(2);
            const QString line  = QString("  %1  [%2]").arg(name, -24).arg(state);
            postMessage(host, "(server)", Message::make(MessageType::Server, "", line));
        }
    });
    connect(cl, &IrcClient::readMarkerReceived, this,
            [this](const QString &host, const QString &target, const QDateTime &ts){
        if (auto *ch = channel(ServerId{host}, BufferId{target}))
            ch->lastRead = ts;
    });
    connect(cl, &IrcClient::userJoined,      this, &SessionModel::onUserJoined);
    connect(cl, &IrcClient::userParted,      this, &SessionModel::onUserParted);
    connect(cl, &IrcClient::userQuit,        this, &SessionModel::onUserQuit);
    connect(cl, &IrcClient::netsplitDetected, this, &SessionModel::onNetsplitDetected);
    connect(cl, &IrcClient::netjoinDetected,  this, &SessionModel::onNetjoinDetected);
    connect(cl, &IrcClient::standardReply,    this, &SessionModel::onStandardReply);
    connect(cl, &IrcClient::nickChanged,     this, &SessionModel::onNickChanged);
    connect(cl, &IrcClient::kicked,          this, &SessionModel::onKicked);
    connect(cl, &IrcClient::topicReceived,    this, &SessionModel::onTopicReceived);
    connect(cl, &IrcClient::topicSetByReceived, this,
            [this](const QString &host, const QString &channel,
                   const QString &setter, quint64 ts) {
        auto *sess = session(ServerId{host});
        if (!sess) return;
        auto &ch = sess->getOrCreate(channel);
        ch.topicSetBy = setter;
        ch.topicSetAt = ts;
        emit topicSetByChanged(ServerId{host}, BufferId{channel}, setter, ts);
    });
    connect(cl, &IrcClient::awayChanged, this,
            [this](const QString &host, bool away) {
        auto *sess = session(ServerId{host});
        if (sess) sess->away = away;
        emit awayStatusChanged(ServerId{host}, away);
    });
    connect(cl, &IrcClient::modesReceived,   this, &SessionModel::onModesReceived);
    connect(cl, &IrcClient::namesReceived,   this, &SessionModel::onNamesReceived);
    connect(cl, &IrcClient::whoEntryReceived,this, &SessionModel::onWhoEntry);
    connect(cl, &IrcClient::serverMessage,     this, &SessionModel::onServerMessage);
    connect(cl, &IrcClient::errorMessage,      this, &SessionModel::onErrorMessage);
    connect(cl, &IrcClient::contextualMessage, this, &SessionModel::onContextualMessage);
    connect(cl, &IrcClient::wallopsReceived, this,
            [this](const QString &host, const QString &nick, const QString &text){
        const QString line = "[" + (nick.isEmpty() ? host : nick) + "] " + text;
        postMessage(host, "(server)", Message::make(MessageType::Wallops, nick, line));
    });
    connect(cl, &IrcClient::ctcpPingReply,     this, &SessionModel::onCtcpPingReply);
    connect(cl, &IrcClient::ctcpTimeReply,   this, &SessionModel::onCtcpTimeReply);
    connect(cl, &IrcClient::selfNickChanged, this, &SessionModel::onSelfNickChanged);
    connect(cl, &IrcClient::typingReceived, this,
            [this](const QString &h, const QString &ch, const QString &nick, const QString &state){
        emit typingReceived(ServerId{h}, BufferId{ch}, nick, state);
    });
    connect(cl, &IrcClient::dccSendReceived, this,
            [this](const QString &h, const QString &nick, const QString &fn,
                   quint32 ip, quint16 port, qint64 fs){
        emit dccSendReceived(ServerId{h}, nick, fn, ip, port, fs);
    });
    connect(cl, &IrcClient::dccPassiveOfferReceived, this,
            [this](const QString &h, const QString &nick, const QString &fn,
                   quint32 ip, qint64 fs, const QString &tok){
        emit dccPassiveOfferReceived(ServerId{h}, nick, fn, ip, fs, tok);
    });
    connect(cl, &IrcClient::dccPassiveSendReply, this,
            [this](const QString &h, const QString &nick, const QString &fn,
                   quint32 ip, quint16 port, qint64 fs, const QString &tok){
        emit dccPassiveSendReply(ServerId{h}, nick, fn, ip, port, fs, tok);
    });
    connect(cl, &IrcClient::sslFingerprintPrompt, this,
            [this](const QString &h, const QString &fp){
        emit sslFingerprintPrompt(ServerId{h}, fp);
    });
    connect(cl, &IrcClient::pingRtt, this,
            [this](const QString &h, int ms){ emit pingRtt(ServerId{h}, ms); });
    connect(cl, &IrcClient::reconnecting, this,
            [this](const QString &h){ emit serverReconnecting(ServerId{h}); });
    connect(cl, &IrcClient::channelListEntry, this,
            [this](const QString &h, const QString &ch, int users, const QString &topic){
        emit channelListEntry(ServerId{h}, BufferId{ch}, users, topic);
    });
    connect(cl, &IrcClient::channelListEnd, this,
            [this](const QString &h, int total){ emit channelListEnd(ServerId{h}, total); });
    connect(cl, &IrcClient::hostChanged,     this, &SessionModel::onHostChanged);
    connect(cl, &IrcClient::reactReceived,   this, &SessionModel::onReactReceived);
    connect(cl, &IrcClient::accountChanged,  this, &SessionModel::onAccountChanged);
    connect(cl, &IrcClient::messageRedacted, this, &SessionModel::onMessageRedacted);
    connect(cl, &IrcClient::inviteNotify,    this, &SessionModel::onInviteNotify);
    connect(cl, &IrcClient::setNameReceived, this, &SessionModel::onSetNameReceived);
    connect(cl, &IrcClient::monitorOnline,   this, &SessionModel::onMonitorOnline);
    connect(cl, &IrcClient::monitorOffline,  this, &SessionModel::onMonitorOffline);
    connect(cl, &IrcClient::userMetaChanged, this,
            [this](const QString &h, const QString &nick, const QString &key, const QString &val){
        onUserMetaChanged(ServerId{h}, nick, key, val);
    });

    if (!m_config.monitorList.isEmpty())
        cl->setMonitorList(m_config.monitorList);

    // Pre-create server buffer and configured channels in the session
    auto *sess = session(ServerId{cfg.name});
    if (!sess) return;
    sess->serverBuffer(); // ensure "(server)" exists
    for (const auto &ch : cfg.channels) {
        auto &c  = sess->getOrCreate(ch.name);
        c.name   = ch.name;
        emit channelAdded(ServerId{cfg.name}, BufferId{ch.name});
    }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void SessionModel::postMessage(const QString &host, const QString &target, const Message &msg)
{
    auto *sess = session(ServerId{host});
    if (!sess) return;

    const QString key = host + '\t' + target.toLower();

    if (msg.isHistory && m_pendingHistoryBefore.contains(key)) {
        m_historyBeforeBuf[key].append(msg);
        return;
    }

    auto &ch = sess->getOrCreate(target);
    if (ch.name.isEmpty()) ch.name = target;
    ch.addMessage(msg);
    if (m_config.ui.logMessages)
        logMessage(host, target, msg);

    const bool isActive = (host == m_activeHost.str() && target.compare(m_activeChannel.str(), Qt::CaseInsensitive) == 0);
    const bool countsAsUnread = msg.type == MessageType::Privmsg
        || (msg.type == MessageType::Notice && target == "(server)");
    if (!isActive && !msg.isHistory && countsAsUnread) {
        if (ch.unread == 0)
            ch.firstUnreadIdx = static_cast<int>(ch.messages.size()) - 1;
        ++ch.unread;
        const bool nickHit    = !sess->mentionRe.pattern().isEmpty() && sess->mentionRe.match(msg.text).hasMatch();
        const bool keywordHit = !sess->highlightRe.pattern().isEmpty() && sess->highlightRe.match(msg.text).hasMatch();
        if (nickHit || keywordHit)
            ++ch.mentions;
    }

    emit messageAdded(ServerId{host}, BufferId{target}, msg);
    if (!isActive && !msg.isHistory && countsAsUnread)
        emit unreadChanged(ServerId{host}, BufferId{target}, ch.unread);
}

void SessionModel::requestOlderHistory(ServerId host, BufferId channel)
{
    const QString key = host.str() + '\t' + channel.str().toLower();
    if (m_pendingHistoryBefore.contains(key)) return;

    auto *ch = this->channel(host, channel);
    if (!ch || ch->messages.isEmpty()) return;

    QString oldestMsgid;
    for (const auto &msg : ch->messages) {
        if (!msg.msgid.isEmpty()) { oldestMsgid = msg.msgid; break; }
    }
    if (oldestMsgid.isEmpty()) return;

    auto *cl = clientFor(host);
    if (!cl) return;

    m_pendingHistoryBefore.insert(key);
    cl->requestHistoryBefore(channel.str(), oldestMsgid, 100);

    QTimer::singleShot(0, this, [this, host, channel, key]{
        m_pendingHistoryBefore.remove(key);
        const QList<Message> msgs = m_historyBeforeBuf.take(key);
        if (!msgs.isEmpty()) {
            auto *target = this->channel(host, channel);
            if (target) target->prependMessages(msgs);
        }
        emit olderHistoryLoaded(host, channel, static_cast<int>(msgs.size()));
    });
}

// ---------------------------------------------------------------------------
// Handlers
// ---------------------------------------------------------------------------

void SessionModel::onConnected(const QString &host)
{
    auto *sess = session(ServerId{host});
    if (!sess) return;
    sess->connected = true;
    emit serverConnected(ServerId{host});

    IrcClient *cl = nullptr;
    for (IrcClient *c : m_clients)
        if (c->serverName() == host) { cl = c; break; }
    if (!cl) return;

    // Join configured channels (with keys)
    QSet<QString> configChans;
    for (const ServerConfig &sc : m_config.servers) {
        if (sc.name != host) continue;
        for (const ChannelConfig &cc : sc.channels) {
            cl->join(cc.name, cc.password);
            configChans.insert(cc.name.toLower());
        }
        break;
    }

    // Re-join any channels the user had open that aren't covered by config
    static const QString kChanPrefixes = QStringLiteral("#&!+");
    for (const auto &ch : std::as_const(sess->channels)) {
        if (ch.name.isEmpty() || ch.name == "(server)") continue;
        if (!kChanPrefixes.contains(ch.name[0]))         continue; // skip PMs
        if (configChans.contains(ch.name.toLower()))      continue; // already joining
        cl->join(ch.name);
    }
}

void SessionModel::onDisconnected(const QString &host)
{
    auto *sess = session(ServerId{host});
    if (!sess) return;
    sess->connected = false;
    // Clear all nick lists
    for (auto &ch : sess->channels) {
        ch.nicks.clear();
        ch.nickIndex.clear();
    }
    emit serverDisconnected(ServerId{host});
    postMessage(host, "(server)", Message::make(MessageType::Server, "", "Disconnected."));
}

void SessionModel::onSocketError(const QString &host, const QString &error)
{
    postMessage(host, "(server)", Message::make(MessageType::Error, "", "Error: " + error));
}

void SessionModel::onMessage(const QString &host, const QString &target,
                             const QString &nick, const QString &text,
                             const QDateTime &serverTime, bool isHistory,
                             const QString &msgid, const QString &replyTo)
{
    auto *sess = session(ServerId{host});
    const bool isSelf = sess && (nick.toLower() == sess->nick.toLower());
    const bool isPM = !target.startsWith('#') && !target.startsWith('&')
                      && !target.startsWith('!');
    if (isPM && !isSelf && isIgnoredFor(nick, IgnoreType::PM)) return;
    const QString pmNick = isSelf ? target : nick;
    const QString buf = isPM ? pmNick : target;
    if (isPM && !isHistory) openPM(ServerId{host}, pmNick);
    QString account;
    if (auto *ch = sess ? sess->get(buf) : nullptr) {
        const auto it = ch->nickIndex.constFind(nick.toLower());
        if (it != ch->nickIndex.constEnd())
            account = ch->nicks[it.value()].account;
    }
    QString display = text;
    if (isSelf && isPM) {
        const QString svcCmd = text.section(' ', 0, 0).toUpper();
        static const QStringList pwdCmds = {
            "IDENTIFY", "REGISTER", "GHOST", "RECOVER", "RELEASE", "REGAIN", "SETPASS"
        };
        if (target.compare("NickServ", Qt::CaseInsensitive) == 0 && pwdCmds.contains(svcCmd))
            display = svcCmd + " <redacted>";
    }
    postMessage(host, buf, Message::make(MessageType::Privmsg, nick, display, serverTime, isHistory, msgid, replyTo, account));
}

void SessionModel::onNotice(const QString &host, const QString &target,
                            const QString &nick, const QString &text,
                            const QDateTime &serverTime, bool isHistory,
                            const QString &msgid, const QString &replyTo)
{
    auto *sess2 = session(ServerId{host});
    const bool isChannelNotice = target.startsWith('#') || target.startsWith('&');
    if (!isChannelNotice && isIgnoredFor(nick, IgnoreType::Notice)) return;
    QString dest;
    if (isChannelNotice) {
        dest = target;
    } else if (sess2 && sess2->get(nick)) {
        dest = nick;   // route reply into the open PM tab for this sender
    } else {
        dest = "(server)";
    }
    QString noticeAccount;
    if (auto *ch = sess2 ? sess2->get(dest) : nullptr) {
        const auto it = ch->nickIndex.constFind(nick.toLower());
        if (it != ch->nickIndex.constEnd())
            noticeAccount = ch->nicks[it.value()].account;
    }
    postMessage(host, dest, Message::make(MessageType::Notice, nick, text, serverTime, isHistory, msgid, replyTo, noticeAccount));
}

void SessionModel::onAction(const QString &host, const QString &target,
                            const QString &nick, const QString &text,
                            const QDateTime &serverTime, bool isHistory,
                            const QString &msgid)
{
    const bool isPrivateAction = !target.startsWith('#') && !target.startsWith('&')
                                 && !target.startsWith('!');
    if (isPrivateAction && isIgnoredFor(nick, IgnoreType::PM)) return;
    auto *sessA = session(ServerId{host});
    QString actionAccount;
    if (auto *ch = sessA ? sessA->get(target) : nullptr) {
        const auto it = ch->nickIndex.constFind(nick.toLower());
        if (it != ch->nickIndex.constEnd())
            actionAccount = ch->nicks[it.value()].account;
    }
    postMessage(host, target, Message::make(MessageType::Action, nick, text, serverTime, isHistory, msgid, {}, actionAccount));
}

void SessionModel::onUserJoined(const QString &host, const QString &channel, const QString &nick, const QString &user, const QString &hostAddr)
{
    auto *sess = session(ServerId{host});
    if (!sess) return;
    auto &ch = sess->getOrCreate(channel);
    if (ch.name.isEmpty()) ch.name = channel;

    const bool isSelf = sess->nick.toLower() == nick.toLower();
    if (isSelf) {
        ch.joined = true;
        emit channelAdded(ServerId{host}, BufferId{channel});
    }
    ch.addNick(nick);
    emit nickAdded(ServerId{host}, BufferId{channel}, nick);

    if (!isSelf) {
        auto *cl = clientFor(ServerId{host});
        if (cl) {
            if (cl->supportsWhox())
                sendRaw(ServerId{host}, "WHO " + nick + " %tcnfa,42");
            else
                sendRaw(ServerId{host}, "WHO " + nick);
        }
    }

    const QString mask = (!user.isEmpty() && !hostAddr.isEmpty())
        ? " (" + user + "@" + hostAddr + ")" : QString();
    postMessage(host, channel, Message::make(MessageType::Join, nick,
        isSelf ? "You joined " + channel : nick + mask + " has joined the channel"));
}

void SessionModel::onUserParted(const QString &host, const QString &channel,
                                const QString &nick, const QString &user, const QString &hostAddr, const QString &reason)
{
    auto *sess = session(ServerId{host});
    if (!sess) return;
    auto *ch = sess->get(channel);

    const bool isSelf = sess->nick.toLower() == nick.toLower();
    if (isSelf && ch) {
        ch->joined = false;
        ch->nicks.clear();
        ch->nickIndex.clear();
        emit nickListChanged(ServerId{host}, BufferId{channel});
    } else if (ch) {
        ch->removeNick(nick);
        emit nickRemoved(ServerId{host}, BufferId{channel}, nick);
    }
    const QString mask = (!user.isEmpty() && !hostAddr.isEmpty())
        ? " (" + user + "@" + hostAddr + ")" : QString();
    const QString text = nick + mask + (reason.isEmpty() ? " has left the channel" : " has left the channel (" + reason + ")");
    postMessage(host, channel, Message::make(MessageType::Part, nick, text));
}

void SessionModel::onUserQuit(const QString &host, const QString &nick, const QString &user, const QString &hostAddr, const QString &reason)
{
    auto *sess = session(ServerId{host});
    if (!sess) return;
    const QString mask = (!user.isEmpty() && !hostAddr.isEmpty())
        ? " (" + user + "@" + hostAddr + ")" : QString();
    const QString text = nick + mask + (reason.isEmpty() ? " has quit" : " has quit (" + reason + ")");
    for (auto &ch : sess->channels) {
        if (!ch.nickIndex.contains(nick.toLower())) continue;
        ch.removeNick(nick);
        emit nickRemoved(ServerId{host}, BufferId{ch.name}, nick);
        postMessage(host, ch.name, Message::make(MessageType::Quit, nick, text));
    }
}

void SessionModel::onNetsplitDetected(const QString &host, const QString &servers, const QStringList &nicks)
{
    auto *sess = session(ServerId{host});
    if (!sess) return;

    QSet<QString> lowerNicks;
    lowerNicks.reserve(nicks.size());
    for (const QString &n : nicks)
        lowerNicks.insert(n.toLower());

    for (auto &ch : sess->channels) {
        int lost = 0;
        for (const QString &ln : std::as_const(lowerNicks))
            if (ch.nickIndex.contains(ln)) ++lost;
        if (lost == 0) continue;
        ch.removeNicks(lowerNicks);
        emit nickListChanged(ServerId{host}, BufferId{ch.name});
        const QString text = QString("Netsplit: %1 user%2 lost (%3)")
            .arg(lost).arg(lost == 1 ? "" : "s").arg(servers);
        postMessage(host, ch.name, Message::make(MessageType::Quit, QString(), text));
    }
}

void SessionModel::onNetjoinDetected(const QString &host, const QString &servers,
                                     const QStringList &channels, const QStringList &nicks)
{
    auto *sess = session(ServerId{host});
    if (!sess) return;
    // Group returning nicks by channel so each nick list is rebuilt once.
    QHash<QString, QStringList> perChannel;
    for (int i = 0; i < channels.size(); ++i) {
        const QString &channel = channels[i];
        const QString &nick    = i < nicks.size() ? nicks[i] : QString();
        if (nick.isEmpty() || !sess->get(channel)) continue;
        perChannel[channel].append(nick);
    }
    for (auto it = perChannel.cbegin(); it != perChannel.cend(); ++it) {
        auto *ch = sess->get(it.key());
        if (!ch) continue;
        ch->addNicks(it.value());
        emit nickListChanged(ServerId{host}, BufferId{it.key()});
        const int n = static_cast<int>(it.value().size());
        const QString text = QString("Netjoin: %1 user%2 returned (%3)")
            .arg(n).arg(n == 1 ? "" : "s").arg(servers);
        postMessage(host, it.key(), Message::make(MessageType::Join, QString(), text));
    }
}

void SessionModel::onNickChanged(const QString &host, const QString &oldNick, const QString &newNick)
{
    auto *sess = session(ServerId{host});
    if (!sess) return;
    for (auto &ch : sess->channels) {
        if (!ch.nickIndex.contains(oldNick.toLower())) continue;
        ch.renameNick(oldNick, newNick);
        emit nickRenamed(ServerId{host}, BufferId{ch.name}, oldNick, newNick);
        postMessage(host, ch.name, Message::make(MessageType::Nick, oldNick,
            oldNick + " is now known as " + newNick, {}, false, {}, newNick));
    }
}

void SessionModel::onKicked(const QString &host, const QString &channel,
                            const QString &nick, const QString &by, const QString &reason)
{
    auto *sess = session(ServerId{host});
    if (!sess) return;
    auto *ch = sess->get(channel);
    if (ch) {
        ch->removeNick(nick);
        emit nickRemoved(ServerId{host}, BufferId{channel}, nick);
        postMessage(host, channel, Message::make(MessageType::Kick, nick,
            nick + " was kicked by " + by + (reason.isEmpty() ? "" : " (" + reason + ")")));
    }
}

void SessionModel::onTopicReceived(const QString &host, const QString &channel, const QString &topic)
{
    auto *sess = session(ServerId{host});
    if (!sess) return;
    auto &ch = sess->getOrCreate(channel);
    ch.topic = topic;
    emit topicChanged(ServerId{host}, BufferId{channel}, topic);
}

static void parseBotModes(const QString &modeStr, QSet<QString> &botSet)
{
    // Parse channel +B/-B mode changes, e.g. "+oB nick1 nick2".
    // argModes = chars that consume a nick argument.
    const QStringList parts = modeStr.split(' ', Qt::SkipEmptyParts);
    if (parts.isEmpty()) return;

    static const QString argModes = QStringLiteral("ovhaqBe");

    bool adding = true;
    int  argIdx = 1;
    for (QChar c : parts[0]) {
        if (c == '+') { adding = true;  continue; }
        if (c == '-') { adding = false; continue; }
        if (c == 'B' && argIdx < parts.size()) {
            const QString nick = parts[argIdx].toLower();
            if (adding) botSet.insert(nick);
            else        botSet.remove(nick);
        }
        if (argModes.contains(c))
            ++argIdx;
    }
}

static QChar modeToPrefix(QChar m)
{
    switch (m.unicode()) {
    case 'q': return '~';
    case 'a': return '&';
    case 'o': return '@';
    case 'h': return '%';
    case 'v': return '+';
    default:  return ' ';
    }
}

void SessionModel::onModesReceived(const QString &host, const QString &channel, const QString &modes)
{
    auto *sess = session(ServerId{host});
    if (!sess) return;

    const bool isChannel = channel.startsWith('#') || channel.startsWith('&');

    if (isChannel) {
        auto *ch = sess->get(channel);
        if (ch) {
            ch->modes = modes;
            parseBotModes(modes, ch->botNicks);

            // Update per-nick prefixes for privilege mode changes (+o/-o etc.)
            static const QString prefixModes = QStringLiteral("qaohv");
            static const QString argModes    = QStringLiteral("ovhaqBe");
            const QStringList parts = modes.split(' ', Qt::SkipEmptyParts);
            bool adding = true;
            int  argIdx = 1;
            for (QChar c : parts.value(0)) {
                if (c == '+') { adding = true;  continue; }
                if (c == '-') { adding = false; continue; }
                const QChar pre = modeToPrefix(c);
                if (pre != ' ' && argIdx < parts.size()) {
                    const QString target = parts[argIdx];
                    const auto it = ch->nickIndex.constFind(target.toLower());
                    if (it != ch->nickIndex.constEnd()) {
                        auto &e = ch->nicks[it.value()];
                        if (adding) e.prefixes.insert(pre);
                        else        e.prefixes.remove(pre);
                        e.recomputePrefix();
                    }
                }
                if (argModes.contains(c)) ++argIdx;
            }
            std::sort(ch->nicks.begin(), ch->nicks.end());
            ch->rebuildNickIndex();
            emit nickListChanged(ServerId{host}, BufferId{channel});
        }
        postMessage(host, channel, Message::make(MessageType::Server, "", "Mode " + channel + " " + modes));
        emit modesChanged(ServerId{host}, BufferId{channel});
    } else {
        // User mode — check for +B/-B on this nick
        const QString &modeStr = modes;
        bool adding = true;
        for (QChar c : modeStr.split(' ').value(0)) {
            if (c == '+') { adding = true;  continue; }
            if (c == '-') { adding = false; continue; }
            if (c == 'B') {
                if (adding) sess->botNicks.insert(channel.toLower());
                else        sess->botNicks.remove(channel.toLower());
            }
        }
        // Also update all channel nick lists so the display refreshes
        for (auto &ch : sess->channels)
            emit nickListChanged(ServerId{host}, BufferId{ch.name});
    }
}

void SessionModel::onNamesReceived(const QString &host, const QString &channel, const QStringList &nicks)
{
    auto *sess = session(ServerId{host});
    if (!sess) return;
    auto &ch = sess->getOrCreate(channel);
    ch.setNicks(nicks);
    emit nickListChanged(ServerId{host}, BufferId{channel});
}

void SessionModel::onWhoEntry(const QString &host, const QString &channel,
                              const QString &nick, const QString &flags)
{
    auto *sess = session(ServerId{host});
    if (!sess) return;

    const bool isBot = flags.contains('B');
    const QString key = nick.toLower();
    // Update session-wide bot set (covers per-nick WHO returning channel "*")
    if (isBot) sess->botNicks.insert(key);
    else       sess->botNicks.remove(key);

    auto *ch = sess->get(channel);
    if (!ch) {
        // Per-nick WHO returned "*" — refresh all channels containing this nick
        if (isBot) {
            for (auto &c : sess->channels) {
                if (c.nickIndex.contains(key))
                    emit nickListChanged(ServerId{host}, BufferId{c.name});
            }
        }
        return;
    }

    const bool changed = isBot ? !ch->botNicks.contains(key)
                                :  ch->botNicks.contains(key);
    if (!changed) return;

    if (isBot) ch->botNicks.insert(key);
    else        ch->botNicks.remove(key);

    emit nickListChanged(ServerId{host}, BufferId{channel});
}

void SessionModel::onServerMessage(const QString &host, const QString &text)
{
    postMessage(host, "(server)", Message::make(MessageType::Server, "", text));
}

void SessionModel::onErrorMessage(const QString &host, const QString &text)
{
    const QString target = (host == m_activeHost.str() && !m_activeChannel.isEmpty())
        ? m_activeChannel.str() : "(server)";
    postMessage(host, target, Message::make(MessageType::Error, "", text));
}

void SessionModel::onStandardReply(const QString &host, const QString &channel,
                                   const QString &severity, const QString &text)
{
    auto *sess = session(ServerId{host});
    if (!sess) return;
    // Post to the named channel if we're in it, else active channel / server buffer
    QString target;
    if (!channel.isEmpty() && sess->get(channel))
        target = channel;
    else
        target = (host == m_activeHost.str() && !m_activeChannel.isEmpty())
                 ? m_activeChannel.str() : "(server)";
    const MessageType type = (severity == "FAIL") ? MessageType::Error : MessageType::Server;
    postMessage(host, target, Message::make(type, "", text));
}

void SessionModel::onContextualMessage(const QString &host, const QString &text)
{
    const QString target = (host == m_activeHost.str() && !m_activeChannel.isEmpty())
        ? m_activeChannel.str() : "(server)";
    postMessage(host, target, Message::make(MessageType::Reply, "", text));
}

void SessionModel::onCtcpPingReply(const QString &host, const QString &nick, qint64 rttMs)
{
    const QString text = rttMs >= 0
        ? QString("Ping reply from %1: %2ms").arg(nick).arg(rttMs)
        : QString("Ping reply from %1").arg(nick);
    const QString target = (host == m_activeHost.str() && !m_activeChannel.isEmpty())
        ? m_activeChannel.str() : "(server)";
    postMessage(host, target, Message::make(MessageType::Server, "", text));
}

void SessionModel::onCtcpTimeReply(const QString &host, const QString &nick, const QString &timeStr)
{
    const QString text = QString("Time reply from %1: %2").arg(nick, timeStr);
    const QString target = (host == m_activeHost.str() && !m_activeChannel.isEmpty())
        ? m_activeChannel.str() : "(server)";
    postMessage(host, target, Message::make(MessageType::Server, "", text));
}

void SessionModel::onSelfNickChanged(const QString &host, const QString &nick)
{
    auto *sess = session(ServerId{host});
    if (sess) {
        sess->nick = nick;
        sess->mentionRe = nick.isEmpty() ? QRegularExpression{}
            : QRegularExpression("\\b" + QRegularExpression::escape(nick) + "\\b",
                                 QRegularExpression::CaseInsensitiveOption);
    }
    emit selfNickChanged(ServerId{host}, nick);
}

void SessionModel::onHostChanged(const QString &host, const QString &nick,
                                  const QString &newUser, const QString &newHost)
{
    auto *sess = session(ServerId{host});
    if (!sess) return;
    const QString text = nick + " changed host (" + newUser + "@" + newHost + ")";
    for (auto &ch : sess->channels) {
        if (!ch.nickIndex.contains(nick.toLower())) continue;
        postMessage(host, ch.name, Message::make(MessageType::Server, "", text));
    }
}

void SessionModel::onReactReceived(const QString &host, const QString &target,
                                    const QString &nick, const QString &msgid,
                                    const QString &emoji)
{
    if (msgid.isEmpty() || emoji.isEmpty()) return;
    const bool isChannel = target.startsWith('#') || target.startsWith('&');
    const QString buf = isChannel ? target : nick;
    auto *ch = channel(ServerId{host}, BufferId{buf});
    if (!ch) return;

    auto &perEmoji = ch->reactions[msgid];
    static constexpr int kMaxEmojis = 16;
    static constexpr int kMaxNicks  = 50;
    if (!perEmoji.contains(emoji) && perEmoji.size() >= kMaxEmojis) return;
    if (perEmoji[emoji].size() >= kMaxNicks) return;
    perEmoji[emoji].insert(nick);
    emit reactionsChanged(ServerId{host}, BufferId{buf}, msgid);
}

void SessionModel::onAccountChanged(const QString &host, const QString &nick,
                                     const QString &account)
{
    auto *sess = session(ServerId{host});
    if (!sess) return;
    for (auto &ch : sess->channels) {
        ch.setNickAccount(nick, account);
        if (ch.nickIndex.contains(nick.toLower()))
            emit nickListChanged(ServerId{host}, BufferId{ch.name});
    }
    // If this nick has no avatar yet and the server supports metadata, request it.
    const QString lower = nick.toLower();
    const bool noAvatar = !sess->nickMeta.contains(lower)
                       || sess->nickMeta[lower].avatarUrl.isEmpty();
    if (noAvatar) {
        if (auto *cl = clientFor(ServerId{host})) {
            if (cl->hasCap("draft/metadata-2"))
                sendRaw(ServerId{host}, "METADATA " + nick + " GET avatar display-name");
        }
    }
}

void SessionModel::onUserMetaChanged(ServerId host, const QString &nick,
                                      const QString &key,  const QString &value)
{
    auto *sess = session(ServerId{host});
    if (!sess) return;
    const QString lower = nick.toLower();
    sess->setNickMeta(lower, key, value);
    emit userMetaChanged(ServerId{host}, nick, key, value);
    for (const auto &ch : std::as_const(sess->channels))
        if (ch.nickIndex.contains(lower))
            emit nickListChanged(ServerId{host}, BufferId{ch.name});
}

void SessionModel::onMessageRedacted(const QString &host, const QString &senderNick,
                                      const QString &target, const QString &msgid,
                                      const QString &reason)
{
    Q_UNUSED(reason) // reason is not surfaced in the UI; keep parameter for signal compat
    auto *sess = session(ServerId{host});
    if (!sess) return;
    const bool isChannel = target.startsWith('#') || target.startsWith('&');
    const QString bufName = isChannel ? target : senderNick;
    auto *ch = sess->get(bufName);
    if (!ch) return;
    for (auto &msg : ch->messages) {
        if (msg.msgid == msgid) {
            msg.redacted = true;
            break;
        }
    }
    emit messageRedacted(ServerId{host}, BufferId{bufName}, msgid);
}

void SessionModel::onInviteNotify(const QString &host, const QString &inviter,
                                   const QString &channel, const QString &targetNick)
{
    auto *sess = session(ServerId{host});
    if (!sess) return;
    if (isIgnoredFor(inviter, IgnoreType::Invite)) return;
    if (targetNick.toLower() == sess->nick.toLower()) {
        postMessage(host, "(server)", Message::make(MessageType::Server, "",
            "You were invited to " + channel + " by " + inviter));
    } else {
        auto *ch = sess->get(channel);
        if (ch)
            postMessage(host, channel, Message::make(MessageType::Server, "",
                inviter + " invited " + targetNick + " to " + channel));
    }
}

void SessionModel::onSetNameReceived(const QString &host, const QString &nick,
                                      const QString &realname)
{
    auto *sess = session(ServerId{host});
    if (!sess) return;
    const QString text = nick + " changed their realname to \"" + realname + "\"";
    for (auto &ch : sess->channels) {
        if (!ch.nickIndex.contains(nick.toLower())) continue;
        postMessage(host, ch.name, Message::make(MessageType::Server, "", text));
    }
}

void SessionModel::onMonitorOnline(const QString &host, const QStringList &nicks)
{
    postMessage(host, "(server)", Message::make(MessageType::Server, "",
        "Now online: " + nicks.join(", ")));
}

void SessionModel::onMonitorOffline(const QString &host, const QStringList &nicks)
{
    postMessage(host, "(server)", Message::make(MessageType::Server, "",
        "Now offline: " + nicks.join(", ")));
}

void SessionModel::pinCertificate(ServerId host, const QString &fingerprint)
{
    for (auto &sc : m_config.servers) {
        if (sc.name == host.str()) {
            sc.pinnedFingerprint = fingerprint;
            Config::save(m_config, Config::defaultPath());
            break;
        }
    }
    if (auto *cl = clientFor(ServerId{host})) {
        cl->setPinnedFingerprint(fingerprint);
        cl->reconnect();
    }
}

void SessionModel::acceptCertificateOnce(ServerId host, const QString &fingerprint)
{
    if (auto *cl = clientFor(ServerId{host})) {
        cl->setPinnedFingerprint(fingerprint);
        cl->reconnect();
    }
}
