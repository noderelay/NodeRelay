#pragma once

#include "ids.h"
#include "serversession.h"
#include "config/config.h"
#include <QDateTime>
#include <QFile>
#include <QHash>
#include <QObject>
#include <QList>

class IrcClient;
class NetworkMonitor;

class SessionModel : public QObject
{
    Q_OBJECT

public:
    explicit SessionModel(QObject *parent = nullptr);

    // OS network state (reachability, metered). Never null.
    NetworkMonitor *networkMonitor() const { return m_netMonitor; }

    // Create a client for each server in config and start connecting
    void loadConfig(const Config &cfg);
    void addServer(const ServerConfig &sc);
    void removeServer(ServerId host);
    void closeServer(ServerId host);
    bool connectServer(ServerId host);
    void updateServer(ServerId oldHost, const ServerConfig &sc);
    void closeBuffer(ServerId host, BufferId target);

    // Read access for UI
    const QList<ServerSession> &sessions() const { return m_sessions; }
    ServerSession *session(ServerId host);
    Channel       *channel(ServerId host, BufferId name);

    // Active selection — UI drives this
    void    setActive      (ServerId host, BufferId channel);
    // Clear unread state without changing the active buffer — used for
    // channels the user is already watching in a docked pane or pop-out
    // window, which never become active.
    void    markRead       (ServerId host, BufferId channel);
    ServerId activeHost()    const { return m_activeHost; }
    BufferId activeChannel() const { return m_activeChannel; }

    // Log-file location for a buffer. The path is computed even when logging
    // is currently off, so previously written logs stay searchable. Empty if
    // either id is blank.
    QString logFilePath(ServerId host, BufferId target) const;
    static QString logsRootPath();
    // Reverse of logFilePath: sanitized "<server>/<buffer>" path components →
    // live buffer. False when no open buffer matches (e.g. logs of a closed one).
    bool    resolveLogBuffer(const QString &serverPart, const QString &bufferPart,
                             ServerId &host, BufferId &buffer) const;
    bool    messageLoggingEnabled() const { return m_config.ui.logMessages; }

    // Send on behalf of a session
    void sendMessage(ServerId host, BufferId target, const QString &text,
                     const QString &replyToMsgid = {});
    void sendRaw    (ServerId host, const QString &line);
    void localMessage(ServerId host, BufferId target, const QString &text);
    QString selfNick  (ServerId host);
    bool    hasMention(ServerId host, BufferId channel);
    void sendJoin   (ServerId host, BufferId channel, const QString &key = {});
    void sendPart   (ServerId host, BufferId channel, const QString &reason = {});
    void sendNick   (ServerId host, const QString &nick);
    void sendAction (ServerId host, BufferId target, const QString &text);
    void sendTyping (ServerId host, BufferId channel, const QString &state);
    void openPM     (ServerId host, const QString &nick);
    IrcClient *clientFor(ServerId host);

    void setIgnore         (const QString &nick, IgnoreTypes flags = kIgnoreAll);
    void clearIgnore       (const QString &nick);
    void setHighlightWords (const QString &words);
    // Comma-separated words → case-insensitive \b-anchored regex with one
    // capture group (the renderer highlights group 1). Invalid = no matches.
    static QRegularExpression buildHighlightRe(const QString &words)
    {
        QStringList parts;
        for (const QString &w : words.split(',', Qt::SkipEmptyParts)) {
            const QString t = w.trimmed();
            if (!t.isEmpty())
                parts << "\\b" + QRegularExpression::escape(t) + "\\b";
        }
        if (parts.isEmpty()) return {};
        // Group 1 wraps the whole alternation — the renderer highlights group 1.
        return QRegularExpression("(" + parts.join('|') + ")", QRegularExpression::CaseInsensitiveOption);
    }
    bool isIgnored   (const QString &nick) const;
    bool isIgnoredFor(const QString &nick, IgnoreType type) const;
    IgnoreTypes ignoreFlags(const QString &nick) const;

    void requestOlderHistory(ServerId host, BufferId channel);

    void sendReact (ServerId host, BufferId target,
                    const QString &msgid, const QString &emoji);
    void sendRedact(ServerId host, BufferId target,
                    const QString &msgid, const QString &reason = {});

    void requestNickMeta(ServerId host, const QString &nick); // on-demand, deduped

    void monitorAdd   (ServerId host, const QString &nick);
    void monitorRemove(ServerId host, const QString &nick);
    void monitorClear (ServerId host);
    void monitorStatus(ServerId host);
    void pinCertificate      (ServerId host, const QString &fingerprint);
    void acceptCertificateOnce(ServerId host, const QString &fingerprint);
    void onUserMetaChanged   (ServerId host, const QString &nick,
                              const QString &key, const QString &value);

signals:
    // Structural changes — sidebar needs a repaint
    void serverAdded       (ServerId host);
    void serverConnected   (ServerId host);
    void serverDisconnected(ServerId host);
    void serverClosed      (ServerId host);
    void channelAdded  (ServerId host, BufferId channel);
    void channelRemoved(ServerId host, BufferId channel);

    // Content changes — chat view needs updating
    void messageAdded   (ServerId host, BufferId channel, const Message &msg);
    void topicChanged      (ServerId host, BufferId channel, const QString &topic);
    void topicSetByChanged (ServerId host, BufferId channel,
                            const QString &setter, quint64 ts);
    void awayStatusChanged (ServerId host, bool away);
    void modesChanged   (ServerId host, BufferId channel);
    void nickListChanged(ServerId host, BufferId channel);
    void nickAdded      (ServerId host, BufferId channel, const QString &nick);
    void nickRemoved    (ServerId host, BufferId channel, const QString &nick);
    void nickRenamed    (ServerId host, BufferId channel,
                         const QString &oldNick, const QString &newNick);
    void unreadChanged   (ServerId host, BufferId channel, int count);
    void reactionsChanged(ServerId host, BufferId channel, const QString &msgid);
    void messageRedacted (ServerId host, BufferId channel, const QString &msgid);
    void olderHistoryLoaded(ServerId host, BufferId channel, int count);

    // Self
    void selfNickChanged(ServerId host, const QString &nick);

    // Connection quality
    void pingRtt          (ServerId host, int ms);
    void serverReconnecting(ServerId host);

    // Typing
    void typingReceived(ServerId host, BufferId channel,
                        const QString &nick, const QString &state);

    // Channel list
    void channelListEntry(ServerId host, BufferId channel, int users, const QString &topic);
    void channelListEnd  (ServerId host, int total);

    // User metadata
    void userMetaChanged(ServerId host, const QString &nick,
                         const QString &key, const QString &value);

    // TLS cert pin
    void sslFingerprintPrompt(ServerId host, const QString &fingerprint);

    // DCC
    void dccSendReceived(ServerId server, const QString &fromNick,
                         const QString &filename, quint32 ip, quint16 port, qint64 filesize);
    void dccPassiveOfferReceived(ServerId server, const QString &fromNick,
                                  const QString &filename, quint32 ip,
                                  qint64 filesize, const QString &token);
    void dccPassiveSendReply(ServerId server, const QString &fromNick,
                              const QString &filename, quint32 ip, quint16 port,
                              qint64 filesize, const QString &token);

private:
    void attachClient(IrcClient *client, const ServerConfig &cfg);
    void spawnSession(const ServerConfig &sc, bool addToConfig);

    // IrcClient signal handlers
    void onConnected      (const QString &host);
    void onDisconnected   (const QString &host);
    void onSocketError    (const QString &host, const QString &error);
    void onMessage        (const QString &host, const QString &target,
                           const QString &nick, const QString &text,
                           const QDateTime &serverTime, bool isHistory,
                           const QString &msgid, const QString &replyTo);
    void onNotice         (const QString &host, const QString &target,
                           const QString &nick, const QString &text,
                           const QDateTime &serverTime, bool isHistory,
                           const QString &msgid, const QString &replyTo);
    void onAction         (const QString &host, const QString &target,
                           const QString &nick, const QString &text,
                           const QDateTime &serverTime, bool isHistory,
                           const QString &msgid);
    void onUserJoined     (const QString &host, const QString &channel, const QString &nick, const QString &user, const QString &hostAddr);
    void onUserParted     (const QString &host, const QString &channel,
                           const QString &nick, const QString &user, const QString &hostAddr, const QString &reason);
    void onUserQuit       (const QString &host, const QString &nick, const QString &user, const QString &hostAddr, const QString &reason);
    void onNetsplitDetected(const QString &host, const QString &servers, const QStringList &nicks);
    void onNetjoinDetected (const QString &host, const QString &servers,
                            const QStringList &channels, const QStringList &nicks);
    void onStandardReply   (const QString &host, const QString &channel,
                            const QString &severity, const QString &text);
    void onNickChanged    (const QString &host, const QString &oldNick, const QString &newNick);
    void onKicked         (const QString &host, const QString &channel, const QString &nick,
                           const QString &by,   const QString &reason);
    void onTopicReceived  (const QString &host, const QString &channel, const QString &topic);
    void onModesReceived  (const QString &host, const QString &channel, const QString &modes);
    void onNamesReceived  (const QString &host, const QString &channel, const QStringList &nicks);
    void onWhoEntry       (const QString &host, const QString &channel,
                           const QString &nick, const QString &flags);
    void onServerMessage     (const QString &host, const QString &text);
    void onErrorMessage      (const QString &host, const QString &text);
    void onContextualMessage (const QString &host, const QString &text);
    void onCtcpPingReply     (const QString &host, const QString &nick, qint64 rttMs);
    void onCtcpTimeReply  (const QString &host, const QString &nick, const QString &timeStr);
    void onSelfNickChanged(const QString &host, const QString &nick);
    void onHostChanged    (const QString &host, const QString &nick,
                           const QString &newUser, const QString &newHost);
    void onReactReceived  (const QString &host, const QString &target,
                           const QString &nick,  const QString &msgid,
                           const QString &emoji);
    void onAccountChanged (const QString &host, const QString &nick, const QString &account);
    void onMessageRedacted(const QString &host, const QString &senderNick,
                           const QString &target, const QString &msgid, const QString &reason);
    void onInviteNotify   (const QString &host, const QString &inviter,
                           const QString &channel, const QString &targetNick);
    void onSetNameReceived(const QString &host, const QString &nick, const QString &realname);
    void onMonitorOnline  (const QString &host, const QStringList &nicks);
    void onMonitorOffline (const QString &host, const QStringList &nicks);

    void postMessage(const QString &host, const QString &target, const Message &msg);
    void logMessage (const QString &host, const QString &target, const Message &msg);

    QList<ServerSession> m_sessions;
    QList<IrcClient *>   m_clients;
    NetworkMonitor      *m_netMonitor{nullptr};
    Config               m_config;
    QHash<QString, IgnoreTypes> m_ignoredNicks;
    QHash<QString, QFile*> m_logFiles;

    ServerId m_activeHost;
    BufferId m_activeChannel;

    QSet<QString> m_pendingHistoryBefore; // "host\tchannel" keys awaiting CHATHISTORY BEFORE
    QHash<QString, QList<Message>> m_historyBeforeBuf; // collected prepend messages
};
