#include "sessionmodel.h"
#include "irc/ircclient.h"
#include "config/keychainhelper.h"
#include "net/networkmonitor.h"

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
{
    m_netMonitor = new NetworkMonitor(this);
    connect(m_netMonitor, &NetworkMonitor::onlineAgain, this, [this]{
        for (IrcClient *cl : std::as_const(m_clients))
            cl->onNetworkOnline();
    });
}

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

// The pseudo-buffer that holds server notices and status traffic.
static BufferId serverBufferId() { return BufferId{QStringLiteral("(server)")}; }

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
    auto *cl = clientFor(host);
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
    if (auto *cl = clientFor(host))
        cl->sendRedact(target.str(), msgid, reason);
}

void SessionModel::monitorAdd(ServerId host, const QString &nick)
{
    if (auto *cl = clientFor(host))
        cl->monitorAdd(nick);
}

void SessionModel::monitorRemove(ServerId host, const QString &nick)
{
    if (auto *cl = clientFor(host))
        cl->monitorRemove(nick);
}

void SessionModel::monitorClear(ServerId host)
{
    if (auto *cl = clientFor(host))
        cl->monitorClear();
}

void SessionModel::monitorStatus(ServerId host)
{
    if (auto *cl = clientFor(host))
        cl->monitorStatus();
}

// ---------------------------------------------------------------------------
// Logging
// ---------------------------------------------------------------------------



void SessionModel::logMessage(ServerId host, BufferId target, const Message &msg)
{
    if (msg.isHistory) return;

    const QString logsDir = logsRootPath() + sanitizeFilename(host.str()) + "/";
    const QString filePath = logsDir + sanitizeFilename(target.str()) + ".log";

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
    return logsRootPath()
           + sanitizeFilename(host.str()) + "/"
           + sanitizeFilename(target.str()) + ".log";
}

QString SessionModel::logsRootPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
           + "/.config/uplink/logs/";
}

// Maps a log path's sanitized "<server>/<buffer>" components back to a live
// buffer. Sanitizing is lossy, so re-sanitize the current names and compare
// instead of trying to parse the path.
bool SessionModel::resolveLogBuffer(const QString &serverPart, const QString &bufferPart,
                                    ServerId &host, BufferId &buffer) const
{
    for (const auto &s : m_sessions) {
        if (sanitizeFilename(s.name) != serverPart) continue;
        for (const auto &ch : s.channels) {
            if (sanitizeFilename(ch.name) == bufferPart) {
                host   = ServerId{s.name};
                buffer = BufferId{ch.name};
                return true;
            }
        }
    }
    return false;
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
            // Late socket signals from the dying client must not reach a
            // re-added session with the same name.
            m_clients[i]->disconnect(this);
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
            // Late socket signals from the dying client must not reach a
            // re-added session with the same name.
            m_clients[i]->disconnect(this);
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
    auto *sess = session(host);
    if (!sess) return;

    if (isChannelName(target.str())) {
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
    auto *s = session(host);
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
    auto *sess = session(host);
    if (!sess || nick.isEmpty() || isChannelName(nick)) return;
    const bool isNew = !sess->get(nick);
    sess->getOrCreate(nick);
    if (isNew)
        emit channelAdded(host, BufferId{nick});
}

void SessionModel::sendMessage(ServerId host, BufferId target, const QString &text,
                               const QString &replyToMsgid)
{
    auto *cl = clientFor(host);
    if (!cl) return;
    if (text.contains('\n'))
        cl->sendMultiline(target.str(), text, replyToMsgid);
    else
        cl->privmsg(target.str(), text, replyToMsgid);
    // Open a PM tab for outgoing private messages
    const bool isPM = !isChannelName(target.str()) && target.str() != "(server)";
    if (isPM) openPM(host, target.str());
    // If echo-message is acked, the server will echo back the PRIVMSG with the
    // server-assigned msgid — use that echo as the display so msgid is set correctly.
    // Without echo-message, display immediately with no msgid (old behaviour).
    if (!cl->hasCap("echo-message")) {
        auto *sess = session(host);
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
            postMessage(host, target, Message::make(MessageType::Privmsg, sess->nick, display, {}, false, {}, replyToMsgid));
        }
    }
}

void SessionModel::sendRaw(ServerId host, const QString &line)
{
    auto *cl = clientFor(host);
    if (cl) cl->sendRaw(line);
}

void SessionModel::localMessage(ServerId host, BufferId target, const QString &text)
{
    postMessage(host, target, Message::make(MessageType::Server, "", text));
}

QString SessionModel::selfNick(ServerId host)
{
    auto *sess = session(host);
    return sess ? sess->nick : QString{};
}

bool SessionModel::hasMention(ServerId host, BufferId ch)
{
    auto *c = channel(host, ch);
    return c && c->mentions > 0;
}

void SessionModel::sendJoin(ServerId host, BufferId channel, const QString &key)
{
    auto *cl = clientFor(host);
    if (cl) cl->join(channel.str(), key);
}

void SessionModel::sendPart(ServerId host, BufferId channel, const QString &reason)
{
    auto *cl = clientFor(host);
    if (cl) cl->part(channel.str(), reason);
}

void SessionModel::sendNick(ServerId host, const QString &nick)
{
    auto *cl = clientFor(host);
    if (cl) cl->setNick(nick);
}

void SessionModel::sendAction(ServerId host, BufferId target, const QString &text)
{
    auto *cl = clientFor(host);
    if (!cl) return;
    cl->privmsg(target.str(), "\x01""ACTION " + text + "\x01");
    if (!cl->hasCap("echo-message")) {
        if (auto *sess = session(host))
            postMessage(host, target, Message::make(MessageType::Action, sess->nick, text));
    }
}

void SessionModel::sendTyping(ServerId host, BufferId channel, const QString &state)
{
    if (auto *cl = clientFor(host))
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

void SessionModel::markRead(ServerId host, BufferId ch)
{
    auto *c = channel(host, ch);
    if (!c || (c->unread == 0 && c->mentions == 0)) return;
    c->unread         = 0;
    c->mentions       = 0;
    c->firstUnreadIdx = -1;
    emit unreadChanged(host, ch, 0);
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
            [this](const QString &h, const QString &id,
                   const QString &name, bool connected){
        Q_UNUSED(id)
        // live state-change update (not the initial listing)
        postMessage(ServerId{h}, serverBufferId(),
            Message::make(MessageType::Server, "",
                QString("Bouncer network: %1 [%2]").arg(name, connected ? "connected" : "offline")));
    });
    connect(cl, &IrcClient::bouncerNetworksListed, this,
            [this](const QString &h, const QList<QStringList> &networks) {
        if (networks.isEmpty()) {
            postMessage(ServerId{h}, serverBufferId(),
                Message::make(MessageType::Server, "", "Bouncer: no networks configured."));
            return;
        }
        postMessage(ServerId{h}, serverBufferId(),
            Message::make(MessageType::Server, "", "Bouncer networks:"));
        for (const QStringList &n : networks) {
            const QString name  = n.value(1).isEmpty() ? n.value(0) : n.value(1);
            const QString state = n.value(2);
            const QString line  = QString("  %1  [%2]").arg(name, -24).arg(state);
            postMessage(ServerId{h}, serverBufferId(), Message::make(MessageType::Server, "", line));
        }
    });
    connect(cl, &IrcClient::readMarkerReceived, this,
            [this](const QString &h, const QString &target, const QDateTime &ts){
        if (auto *ch = channel(ServerId{h}, BufferId{target}))
            ch->lastRead = ts;
    });
    connect(cl, &IrcClient::userJoined,      this, &SessionModel::onUserJoined);
    connect(cl, &IrcClient::metaLookupFailed, this,
            [this](const QString &h, const QString &nick){
        if (auto *sess = session(ServerId{h}))
            sess->metaRequested.remove(nick.toLower());
    });
    connect(cl, &IrcClient::userParted,      this, &SessionModel::onUserParted);
    connect(cl, &IrcClient::userQuit,        this, &SessionModel::onUserQuit);
    connect(cl, &IrcClient::netsplitDetected, this, &SessionModel::onNetsplitDetected);
    connect(cl, &IrcClient::netjoinDetected,  this, &SessionModel::onNetjoinDetected);
    connect(cl, &IrcClient::standardReply,    this, &SessionModel::onStandardReply);
    connect(cl, &IrcClient::historyBatchDone, this, &SessionModel::onHistoryBatchDone);
    connect(cl, &IrcClient::nickChanged,     this, &SessionModel::onNickChanged);
    connect(cl, &IrcClient::kicked,          this, &SessionModel::onKicked);
    connect(cl, &IrcClient::topicReceived,    this, &SessionModel::onTopicReceived);
    connect(cl, &IrcClient::topicSetByReceived, this,
            [this](const QString &h, const QString &channel,
                   const QString &setter, quint64 ts) {
        auto *sess = session(ServerId{h});
        if (!sess) return;
        auto &ch = sess->getOrCreate(channel);
        ch.topicSetBy = setter;
        ch.topicSetAt = ts;
        emit topicSetByChanged(ServerId{h}, BufferId{channel}, setter, ts);
    });
    connect(cl, &IrcClient::awayChanged, this,
            [this](const QString &h, bool away) {
        auto *sess = session(ServerId{h});
        if (sess) sess->away = away;
        emit awayStatusChanged(ServerId{h}, away);
    });
    connect(cl, &IrcClient::modesReceived,   this, &SessionModel::onModesReceived);
    connect(cl, &IrcClient::namesReceived,   this, &SessionModel::onNamesReceived);
    connect(cl, &IrcClient::whoEntryReceived,this, &SessionModel::onWhoEntry);
    connect(cl, &IrcClient::serverMessage,     this, &SessionModel::onServerMessage);
    connect(cl, &IrcClient::errorMessage,      this, &SessionModel::onErrorMessage);
    connect(cl, &IrcClient::contextualMessage, this, &SessionModel::onContextualMessage);
    connect(cl, &IrcClient::wallopsReceived, this,
            [this](const QString &h, const QString &nick, const QString &text){
        const QString line = "[" + (nick.isEmpty() ? h : nick) + "] " + text;
        postMessage(ServerId{h}, serverBufferId(), Message::make(MessageType::Wallops, nick, line));
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

void SessionModel::postMessage(ServerId host, BufferId target, const Message &msg)
{
    auto *sess = session(host);
    if (!sess) return;

    const QString key = host.str() + '\t' + target.str().toLower();

    if (msg.isHistory && m_pendingHistoryBefore.contains(key)) {
        m_historyBeforeBuf[key].append(msg);
        return;
    }

    auto &ch = sess->getOrCreate(target.str());
    if (ch.name.isEmpty()) ch.name = target.str();
    ch.addMessage(msg);
    if (m_config.ui.logMessages)
        logMessage(host, target, msg);

    const bool isActive = (host == m_activeHost && target.str().compare(m_activeChannel.str(), Qt::CaseInsensitive) == 0);
    const bool countsAsUnread = msg.type == MessageType::Privmsg
        || (msg.type == MessageType::Notice && target == serverBufferId());
    if (!isActive && !msg.isHistory && countsAsUnread) {
        if (ch.unread == 0)
            ch.firstUnreadIdx = static_cast<int>(ch.messages.size()) - 1;
        ++ch.unread;
        const bool nickHit    = !sess->mentionRe.pattern().isEmpty() && sess->mentionRe.match(msg.text).hasMatch();
        const bool keywordHit = !sess->highlightRe.pattern().isEmpty() && sess->highlightRe.match(msg.text).hasMatch();
        if (nickHit || keywordHit)
            ++ch.mentions;
    }

    // Read before the emit: a directly-connected slot that inserts into the
    // session's channel hash would invalidate the ch reference (Qt 6 QHash
    // rehash). No current handler does, but don't rely on that.
    const int unread = ch.unread;
    emit messageAdded(host, target, msg);
    if (!isActive && !msg.isHistory && countsAsUnread)
        emit unreadChanged(host, target, unread);
}

BufferId SessionModel::activeOrServer(ServerId host) const
{
    return (host == m_activeHost && !m_activeChannel.isEmpty())
        ? m_activeChannel : serverBufferId();
}

void SessionModel::requestOlderHistory(ServerId host, BufferId channel)
{
    const QString key = host.str() + '\t' + channel.str().toLower();
    if (m_pendingHistoryBefore.contains(key)) return;

    auto *ch = this->channel(host, channel);
    if (!ch || ch->messages.isEmpty()) return;

    const QDateTime oldest = ch->messages.first().timestamp;
    if (!oldest.isValid()) return;

    auto *cl = clientFor(host);
    if (!cl) return;

    if (!cl->requestHistoryBefore(channel.str(), oldest, 100)) {
        // No chathistory cap on this connection — nothing will ever arrive.
        emit olderHistoryLoaded(host, channel, 0);
        return;
    }

    // Completed by onHistoryBatchDone when the reply batch closes. The
    // timeout is only a safety net for a server that never answers.
    m_pendingHistoryBefore.insert(key);
    QTimer::singleShot(10000, this, [this, host, channel, key]{
        if (!m_pendingHistoryBefore.remove(key)) return;  // batch already landed
        m_historyBeforeBuf.remove(key);
        emit olderHistoryLoaded(host, channel, 0);
    });
}

void SessionModel::onHistoryBatchDone(const QString &hostStr, const QString &target)
{
    const ServerId host{hostStr};
    const QString key = hostStr + '\t' + target.toLower();
    if (!m_pendingHistoryBefore.remove(key)) return;  // join-time LATEST etc.

    const QList<Message> msgs = m_historyBeforeBuf.take(key);
    if (!msgs.isEmpty()) {
        if (auto *ch = this->channel(host, BufferId{target}))
            ch->prependMessages(msgs);
    }
    emit olderHistoryLoaded(host, BufferId{target}, static_cast<int>(msgs.size()));
}

// ---------------------------------------------------------------------------
// Handlers
// ---------------------------------------------------------------------------

void SessionModel::onConnected(const QString &hostStr)
{
    const ServerId host{hostStr};
    auto *sess = session(host);
    if (!sess) return;
    sess->connected = true;
    emit serverConnected(host);

    IrcClient *cl = clientFor(host);
    if (!cl) return;

    // Join configured channels (with keys)
    QSet<QString> configChans;
    for (const ServerConfig &sc : m_config.servers) {
        if (sc.name != hostStr) continue;
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

void SessionModel::onDisconnected(const QString &hostStr)
{
    const ServerId host{hostStr};
    auto *sess = session(host);
    if (!sess) return;
    sess->connected = false;
    // Clear all nick lists
    for (auto &ch : sess->channels) {
        ch.nicks.clear();
        ch.nickIndex.clear();
    }
    // Unanswered metadata requests died with the connection — allow re-asks.
    sess->metaRequested.clear();
    emit serverDisconnected(host);
    postMessage(host, serverBufferId(), Message::make(MessageType::Server, "", "Disconnected."));
}

void SessionModel::onSocketError(const QString &host, const QString &error)
{
    postMessage(ServerId{host}, serverBufferId(), Message::make(MessageType::Error, "", "Error: " + error));
}

void SessionModel::onMessage(const QString &hostStr, const QString &target,
                             const QString &nick, const QString &text,
                             const QDateTime &serverTime, bool isHistory,
                             const QString &msgid, const QString &replyTo)
{
    const ServerId host{hostStr};
    auto *sess = session(host);
    const bool isSelf = sess && (nick.toLower() == sess->nick.toLower());
    const bool isPM = !isChannelName(target);
    if (isPM && !isSelf && isIgnoredFor(nick, IgnoreType::PM)) return;
    const QString pmNick = isSelf ? target : nick;
    const QString buf = isPM ? pmNick : target;
    if (isPM && !isHistory) openPM(host, pmNick);
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
    postMessage(host, BufferId{buf}, Message::make(MessageType::Privmsg, nick, display, serverTime, isHistory, msgid, replyTo, account));
}

void SessionModel::onNotice(const QString &hostStr, const QString &target,
                            const QString &nick, const QString &text,
                            const QDateTime &serverTime, bool isHistory,
                            const QString &msgid, const QString &replyTo)
{
    const ServerId host{hostStr};
    auto *sess2 = session(host);
    const bool isChannelNotice = isChannelName(target);
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
    postMessage(host, BufferId{dest}, Message::make(MessageType::Notice, nick, text, serverTime, isHistory, msgid, replyTo, noticeAccount));
}

void SessionModel::onAction(const QString &hostStr, const QString &target,
                            const QString &nick, const QString &text,
                            const QDateTime &serverTime, bool isHistory,
                            const QString &msgid)
{
    const ServerId host{hostStr};
    const bool isPrivateAction = !isChannelName(target);
    if (isPrivateAction && isIgnoredFor(nick, IgnoreType::PM)) return;
    auto *sessA = session(host);
    QString actionAccount;
    if (auto *ch = sessA ? sessA->get(target) : nullptr) {
        const auto it = ch->nickIndex.constFind(nick.toLower());
        if (it != ch->nickIndex.constEnd())
            actionAccount = ch->nicks[it.value()].account;
    }
    postMessage(host, BufferId{target}, Message::make(MessageType::Action, nick, text, serverTime, isHistory, msgid, {}, actionAccount));
}

void SessionModel::onUserJoined(const QString &hostStr, const QString &channel, const QString &nick, const QString &user, const QString &hostAddr)
{
    const ServerId host{hostStr};
    auto *sess = session(host);
    if (!sess) return;
    auto &ch = sess->getOrCreate(channel);
    if (ch.name.isEmpty()) ch.name = channel;

    const bool isSelf = sess->nick.toLower() == nick.toLower();
    if (isSelf) {
        ch.joined = true;
        emit channelAdded(host, BufferId{channel});
    }
    ch.addNick(nick);
    emit nickAdded(host, BufferId{channel}, nick);

    if (!isSelf) {
        // A rejoining user may have changed metadata while offline (SUB only
        // pushes while both sides are connected), and a GET that failed while
        // they were gone must not block retries forever — forget what we had
        // so the next hover fetches fresh.
        sess->metaRequested.remove(nick.toLower());
        sess->nickMeta.remove(nick.toLower());
        auto *cl = clientFor(host);
        if (cl) {
            if (cl->supportsWhox())
                sendRaw(host, "WHO " + nick + " %tcnfa,42");
            else
                sendRaw(host, "WHO " + nick);
        }
    }

    const QString mask = (!user.isEmpty() && !hostAddr.isEmpty())
        ? " (" + user + "@" + hostAddr + ")" : QString();
    postMessage(host, BufferId{channel}, Message::make(MessageType::Join, nick,
        isSelf ? "You joined " + channel : nick + mask + " has joined the channel"));
}

void SessionModel::onUserParted(const QString &hostStr, const QString &channel,
                                const QString &nick, const QString &user, const QString &hostAddr, const QString &reason)
{
    const ServerId host{hostStr};
    auto *sess = session(host);
    if (!sess) return;
    auto *ch = sess->get(channel);

    const bool isSelf = sess->nick.toLower() == nick.toLower();
    if (isSelf && ch) {
        ch->joined = false;
        ch->nicks.clear();
        ch->nickIndex.clear();
        emit nickListChanged(host, BufferId{channel});
    } else if (ch) {
        ch->removeNick(nick);
        emit nickRemoved(host, BufferId{channel}, nick);
    }
    const QString mask = (!user.isEmpty() && !hostAddr.isEmpty())
        ? " (" + user + "@" + hostAddr + ")" : QString();
    const QString text = nick + mask + (reason.isEmpty() ? " has left the channel" : " has left the channel (" + reason + ")");
    postMessage(host, BufferId{channel}, Message::make(MessageType::Part, nick, text));
}

void SessionModel::onUserQuit(const QString &hostStr, const QString &nick, const QString &user, const QString &hostAddr, const QString &reason)
{
    const ServerId host{hostStr};
    auto *sess = session(host);
    if (!sess) return;
    const QString mask = (!user.isEmpty() && !hostAddr.isEmpty())
        ? " (" + user + "@" + hostAddr + ")" : QString();
    const QString text = nick + mask + (reason.isEmpty() ? " has quit" : " has quit (" + reason + ")");
    for (auto &ch : sess->channels) {
        if (!ch.nickIndex.contains(nick.toLower())) continue;
        ch.removeNick(nick);
        emit nickRemoved(host, BufferId{ch.name}, nick);
        postMessage(host, BufferId{ch.name}, Message::make(MessageType::Quit, nick, text));
    }
}

void SessionModel::onNetsplitDetected(const QString &hostStr, const QString &servers, const QStringList &nicks)
{
    const ServerId host{hostStr};
    auto *sess = session(host);
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
        emit nickListChanged(host, BufferId{ch.name});
        const QString text = QString("Netsplit: %1 user%2 lost (%3)")
            .arg(lost).arg(lost == 1 ? "" : "s").arg(servers);
        postMessage(host, BufferId{ch.name}, Message::make(MessageType::Quit, QString(), text));
    }
}

void SessionModel::onNetjoinDetected(const QString &hostStr, const QString &servers,
                                     const QStringList &channels, const QStringList &nicks)
{
    const ServerId host{hostStr};
    auto *sess = session(host);
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
        emit nickListChanged(host, BufferId{it.key()});
        const int n = static_cast<int>(it.value().size());
        const QString text = QString("Netjoin: %1 user%2 returned (%3)")
            .arg(n).arg(n == 1 ? "" : "s").arg(servers);
        postMessage(host, BufferId{it.key()}, Message::make(MessageType::Join, QString(), text));
    }
}

void SessionModel::onNickChanged(const QString &hostStr, const QString &oldNick, const QString &newNick)
{
    const ServerId host{hostStr};
    auto *sess = session(host);
    if (!sess) return;
    for (auto &ch : sess->channels) {
        if (!ch.nickIndex.contains(oldNick.toLower())) continue;
        ch.renameNick(oldNick, newNick);
        emit nickRenamed(host, BufferId{ch.name}, oldNick, newNick);
        postMessage(host, BufferId{ch.name}, Message::make(MessageType::Nick, oldNick,
            oldNick + " is now known as " + newNick, {}, false, {}, newNick));
    }
}

void SessionModel::onKicked(const QString &hostStr, const QString &channel,
                            const QString &nick, const QString &by, const QString &reason)
{
    const ServerId host{hostStr};
    auto *sess = session(host);
    if (!sess) return;
    auto *ch = sess->get(channel);
    if (ch) {
        ch->removeNick(nick);
        emit nickRemoved(host, BufferId{channel}, nick);
        postMessage(host, BufferId{channel}, Message::make(MessageType::Kick, nick,
            nick + " was kicked by " + by + (reason.isEmpty() ? "" : " (" + reason + ")")));
    }
}

void SessionModel::onTopicReceived(const QString &hostStr, const QString &channel, const QString &topic)
{
    const ServerId host{hostStr};
    auto *sess = session(host);
    if (!sess) return;
    auto &ch = sess->getOrCreate(channel);
    ch.topic = topic;
    emit topicChanged(host, BufferId{channel}, topic);
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

void SessionModel::onModesReceived(const QString &hostStr, const QString &channel, const QString &modes)
{
    const ServerId host{hostStr};
    auto *sess = session(host);
    if (!sess) return;

    const bool isChannel = isChannelName(channel);

    if (isChannel) {
        auto *ch = sess->get(channel);
        if (ch) {
            ch->modes = modes;
            parseBotModes(modes, ch->botNicks);

            // Update per-nick prefixes for privilege mode changes (+o/-o etc.)
            static const QString argModes = QStringLiteral("ovhaqBe");
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
                        if (adding) e.addPrefix(pre);
                        else        e.removePrefix(pre);
                    }
                }
                if (argModes.contains(c)) ++argIdx;
            }
            std::sort(ch->nicks.begin(), ch->nicks.end());
            ch->rebuildNickIndex();
            emit nickListChanged(host, BufferId{channel});
        }
        postMessage(host, BufferId{channel}, Message::make(MessageType::Server, "", "Mode " + channel + " " + modes));
        emit modesChanged(host, BufferId{channel});
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
            emit nickListChanged(host, BufferId{ch.name});
    }
}

void SessionModel::onNamesReceived(const QString &hostStr, const QString &channel, const QStringList &nicks)
{
    const ServerId host{hostStr};
    auto *sess = session(host);
    if (!sess) return;
    auto &ch = sess->getOrCreate(channel);
    ch.setNicks(nicks);
    emit nickListChanged(host, BufferId{channel});
}

void SessionModel::onWhoEntry(const QString &hostStr, const QString &channel,
                              const QString &nick, const QString &flags)
{
    const ServerId host{hostStr};
    auto *sess = session(host);
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
                    emit nickListChanged(host, BufferId{c.name});
            }
        }
        return;
    }

    const bool changed = isBot ? !ch->botNicks.contains(key)
                                :  ch->botNicks.contains(key);
    if (!changed) return;

    if (isBot) ch->botNicks.insert(key);
    else        ch->botNicks.remove(key);

    emit nickListChanged(host, BufferId{channel});
}

void SessionModel::onServerMessage(const QString &host, const QString &text)
{
    postMessage(ServerId{host}, serverBufferId(), Message::make(MessageType::Server, "", text));
}

void SessionModel::onErrorMessage(const QString &hostStr, const QString &text)
{
    const ServerId host{hostStr};
    postMessage(host, activeOrServer(host), Message::make(MessageType::Error, "", text));
}

void SessionModel::onStandardReply(const QString &hostStr, const QString &channel,
                                   const QString &severity, const QString &text)
{
    const ServerId host{hostStr};
    auto *sess = session(host);
    if (!sess) return;
    // Post to the named channel if we're in it, else active channel / server buffer
    const BufferId target = (!channel.isEmpty() && sess->get(channel))
        ? BufferId{channel} : activeOrServer(host);
    const MessageType type = (severity == "FAIL") ? MessageType::Error : MessageType::Server;
    postMessage(host, target, Message::make(type, "", text));
}

void SessionModel::onContextualMessage(const QString &hostStr, const QString &text)
{
    const ServerId host{hostStr};
    postMessage(host, activeOrServer(host), Message::make(MessageType::Reply, "", text));
}

void SessionModel::onCtcpPingReply(const QString &hostStr, const QString &nick, qint64 rttMs)
{
    const QString text = rttMs >= 0
        ? QString("Ping reply from %1: %2ms").arg(nick).arg(rttMs)
        : QString("Ping reply from %1").arg(nick);
    const ServerId host{hostStr};
    postMessage(host, activeOrServer(host), Message::make(MessageType::Server, "", text));
}

void SessionModel::onCtcpTimeReply(const QString &hostStr, const QString &nick, const QString &timeStr)
{
    const QString text = QString("Time reply from %1: %2").arg(nick, timeStr);
    const ServerId host{hostStr};
    postMessage(host, activeOrServer(host), Message::make(MessageType::Server, "", text));
}

void SessionModel::onSelfNickChanged(const QString &hostStr, const QString &nick)
{
    const ServerId host{hostStr};
    auto *sess = session(host);
    if (sess) {
        sess->nick = nick;
        sess->mentionRe = nick.isEmpty() ? QRegularExpression{}
            : QRegularExpression("\\b" + QRegularExpression::escape(nick) + "\\b",
                                 QRegularExpression::CaseInsensitiveOption);
    }
    emit selfNickChanged(host, nick);
}

void SessionModel::onHostChanged(const QString &hostStr, const QString &nick,
                                  const QString &newUser, const QString &newHost)
{
    const ServerId host{hostStr};
    auto *sess = session(host);
    if (!sess) return;
    const QString text = nick + " changed host (" + newUser + "@" + newHost + ")";
    for (auto &ch : sess->channels) {
        if (!ch.nickIndex.contains(nick.toLower())) continue;
        postMessage(host, BufferId{ch.name}, Message::make(MessageType::Server, "", text));
    }
}

void SessionModel::onReactReceived(const QString &hostStr, const QString &target,
                                    const QString &nick, const QString &msgid,
                                    const QString &emoji)
{
    if (msgid.isEmpty() || emoji.isEmpty()) return;
    const ServerId host{hostStr};
    const bool isChannel = isChannelName(target);
    const QString buf = isChannel ? target : nick;
    auto *ch = channel(host, BufferId{buf});
    if (!ch) return;
    // Only react to messages we actually hold — fabricated msgids would
    // grow the reactions map without bound (eviction never prunes them).
    if (!ch->hasMessage(msgid)) return;

    auto &perEmoji = ch->reactions[msgid];
    static constexpr int kMaxEmojis = 16;
    static constexpr int kMaxNicks  = 50;
    if (!perEmoji.contains(emoji) && perEmoji.size() >= kMaxEmojis) return;
    if (perEmoji[emoji].size() >= kMaxNicks) return;
    perEmoji[emoji].insert(nick);
    emit reactionsChanged(host, BufferId{buf}, msgid);
}

void SessionModel::onAccountChanged(const QString &hostStr, const QString &nick,
                                     const QString &account)
{
    const ServerId host{hostStr};
    auto *sess = session(host);
    if (!sess) return;
    for (auto &ch : sess->channels) {
        ch.setNickAccount(nick, account);
        if (ch.nickIndex.contains(nick.toLower()))
            emit nickListChanged(host, BufferId{ch.name});
    }
}

// Metadata is only ever shown in hover tooltips, so it's fetched on demand
// rather than roster-wide: account-notify/WHOX used to trigger one GET per
// nick on every join, and Ergo's fakelag drains such a burst at ~2 commands/s,
// queueing real traffic behind minutes of metadata chatter. SUB (sent at
// registration) still pushes changes for nicks we've fetched once.
void SessionModel::requestNickMeta(ServerId host, const QString &nick)
{
    auto *sess = session(host);
    if (!sess || nick.isEmpty()) return;
    const QString lower = nick.toLower();
    if (sess->metaRequested.contains(lower) || sess->nickMeta.contains(lower))
        return;
    auto *cl = clientFor(host);
    if (!cl || !cl->hasCap("draft/metadata-2")) return;
    sess->metaRequested.insert(lower);
    sendRaw(host, "METADATA " + nick + " GET avatar display-name");
}

void SessionModel::onUserMetaChanged(ServerId host, const QString &nick,
                                      const QString &key,  const QString &value)
{
    auto *sess = session(host);
    if (!sess) return;
    const QString lower = nick.toLower();
    sess->setNickMeta(lower, key, value);
    emit userMetaChanged(host, nick, key, value);
    for (const auto &ch : std::as_const(sess->channels))
        if (ch.nickIndex.contains(lower))
            emit nickListChanged(host, BufferId{ch.name});
}

void SessionModel::onMessageRedacted(const QString &hostStr, const QString &senderNick,
                                      const QString &target, const QString &msgid,
                                      const QString &reason)
{
    Q_UNUSED(reason) // reason is not surfaced in the UI; keep parameter for signal compat
    const ServerId host{hostStr};
    auto *sess = session(host);
    if (!sess) return;
    const bool isChannel = isChannelName(target);
    const QString bufName = isChannel ? target : senderNick;
    auto *ch = sess->get(bufName);
    if (!ch) return;
    for (auto &msg : ch->messages) {
        if (msg.msgid == msgid) {
            msg.redacted = true;
            break;
        }
    }
    emit messageRedacted(host, BufferId{bufName}, msgid);
}

void SessionModel::onInviteNotify(const QString &hostStr, const QString &inviter,
                                   const QString &channel, const QString &targetNick)
{
    const ServerId host{hostStr};
    auto *sess = session(host);
    if (!sess) return;
    if (isIgnoredFor(inviter, IgnoreType::Invite)) return;
    if (targetNick.toLower() == sess->nick.toLower()) {
        postMessage(host, serverBufferId(), Message::make(MessageType::Server, "",
            "You were invited to " + channel + " by " + inviter));
    } else {
        auto *ch = sess->get(channel);
        if (ch)
            postMessage(host, BufferId{channel}, Message::make(MessageType::Server, "",
                inviter + " invited " + targetNick + " to " + channel));
    }
}

void SessionModel::onSetNameReceived(const QString &hostStr, const QString &nick,
                                      const QString &realname)
{
    const ServerId host{hostStr};
    auto *sess = session(host);
    if (!sess) return;
    const QString text = nick + " changed their realname to \"" + realname + "\"";
    for (auto &ch : sess->channels) {
        if (!ch.nickIndex.contains(nick.toLower())) continue;
        postMessage(host, BufferId{ch.name}, Message::make(MessageType::Server, "", text));
    }
}

void SessionModel::onMonitorOnline(const QString &host, const QStringList &nicks)
{
    postMessage(ServerId{host}, serverBufferId(), Message::make(MessageType::Server, "",
        "Now online: " + nicks.join(", ")));
}

void SessionModel::onMonitorOffline(const QString &host, const QStringList &nicks)
{
    postMessage(ServerId{host}, serverBufferId(), Message::make(MessageType::Server, "",
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
    if (auto *cl = clientFor(host)) {
        cl->setPinnedFingerprint(fingerprint);
        cl->reconnect();
    }
}

void SessionModel::acceptCertificateOnce(ServerId host, const QString &fingerprint)
{
    if (auto *cl = clientFor(host)) {
        cl->setPinnedFingerprint(fingerprint);
        cl->reconnect();
    }
}
