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
    void removeServer(const ServerId &host);
    void closeServer(const ServerId &host);
    bool connectServer(const ServerId &host);
    void updateServer(const ServerId &oldHost, const ServerConfig &sc);
    void closeBuffer(const ServerId &host, const BufferId &target);

    // Read access for UI
    const QList<ServerSession> &sessions() const { return m_sessions; }
    ServerSession *session(const ServerId &host);
    Channel       *channel(const ServerId &host, const BufferId &name);

    // Active selection — UI drives this
    void    setActive      (const ServerId &host, const BufferId &channel);
    // Clear unread state without changing the active buffer — used for
    // channels the user is already watching in a docked pane or pop-out
    // window, which never become active.
    void    markRead       (const ServerId &host, const BufferId &channel);
    ServerId activeHost()    const { return m_activeHost; }
    BufferId activeChannel() const { return m_activeChannel; }

    // Log-file location for a buffer. The path is computed even when logging
    // is currently off, so previously written logs stay searchable. Empty if
    // either id is blank.
    QString logFilePath(const ServerId &host, const BufferId &target) const;
    static QString logsRootPath();
    // Reverse of logFilePath: sanitized "<server>/<buffer>" path components →
    // live buffer. False when no open buffer matches (e.g. logs of a closed one).
    bool    resolveLogBuffer(const QString &serverPart, const QString &bufferPart,
                             ServerId &host, BufferId &buffer) const;
    bool    messageLoggingEnabled() const { return m_config.ui.logMessages; }

    // Send on behalf of a session
    void sendMessage(const ServerId &host, const BufferId &target, const QString &text,
                     const QString &replyToMsgid = {});
    void sendRaw    (const ServerId &host, const QString &line);
    void localMessage(const ServerId &host, const BufferId &target, const QString &text);
    QString selfNick  (const ServerId &host);
    bool    hasMention(const ServerId &host, const BufferId &channel);
    void sendJoin   (const ServerId &host, const BufferId &channel, const QString &key = {});
    void sendPart   (const ServerId &host, const BufferId &channel, const QString &reason = {});
    void sendNick   (const ServerId &host, const QString &nick);
    void sendAction (const ServerId &host, const BufferId &target, const QString &text);
    void sendTyping (const ServerId &host, const BufferId &channel, const QString &state);
    void openPM     (const ServerId &host, const QString &nick);
    IrcClient *clientFor(const ServerId &host);

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

    void requestOlderHistory(const ServerId &host, const BufferId &channel);

    void sendReact (const ServerId &host, const BufferId &target,
                    const QString &msgid, const QString &emoji);
    void sendRedact(const ServerId &host, const BufferId &target,
                    const QString &msgid, const QString &reason = {});

    void requestNickMeta(const ServerId &host, const QString &nick); // on-demand, deduped

    void monitorAdd   (const ServerId &host, const QString &nick);
    void monitorRemove(const ServerId &host, const QString &nick);
    void monitorClear (const ServerId &host);
    void monitorStatus(const ServerId &host);
    void pinCertificate      (const ServerId &host, const QString &fingerprint);
    void acceptCertificateOnce(const ServerId &host, const QString &fingerprint);
    void onUserMetaChanged   (const ServerId &host, const QString &nick,
                              const QString &key, const QString &value);

signals:
    // Structural changes — sidebar needs a repaint
    void serverAdded       (const ServerId &host);
    void serverConnected   (const ServerId &host);
    void serverDisconnected(const ServerId &host);
    void serverClosed      (const ServerId &host);
    void channelAdded  (const ServerId &host, const BufferId &channel);
    void channelRemoved(const ServerId &host, const BufferId &channel);

    // Content changes — chat view needs updating
    void messageAdded   (const ServerId &host, const BufferId &channel, const Message &msg);
    void topicChanged      (const ServerId &host, const BufferId &channel, const QString &topic);
    void topicSetByChanged (const ServerId &host, const BufferId &channel,
                            const QString &setter, quint64 ts);
    void awayStatusChanged (const ServerId &host, bool away);
    void modesChanged   (const ServerId &host, const BufferId &channel);
    void nickListChanged(const ServerId &host, const BufferId &channel);
    void nickAdded      (const ServerId &host, const BufferId &channel, const QString &nick);
    void nickRemoved    (const ServerId &host, const BufferId &channel, const QString &nick);
    void nickRenamed    (const ServerId &host, const BufferId &channel,
                         const QString &oldNick, const QString &newNick);
    void unreadChanged   (const ServerId &host, const BufferId &channel, int count);
    void reactionsChanged(const ServerId &host, const BufferId &channel, const QString &msgid);
    void messageRedacted (const ServerId &host, const BufferId &channel, const QString &msgid);
    void olderHistoryLoaded(const ServerId &host, const BufferId &channel, int count);

    // Self
    void selfNickChanged(const ServerId &host, const QString &nick);

    // Connection quality
    void pingRtt          (const ServerId &host, int ms);
    void serverReconnecting(const ServerId &host);

    // Typing
    void typingReceived(const ServerId &host, const BufferId &channel,
                        const QString &nick, const QString &state);

    // Channel list
    void channelListEntry(const ServerId &host, const BufferId &channel, int users, const QString &topic);
    void channelListEnd  (const ServerId &host, int total);

    // User metadata
    void channelAvatarChanged(const ServerId &host, const BufferId &channel,
                              const QString &url);
    void userMetaChanged(const ServerId &host, const QString &nick,
                         const QString &key, const QString &value);

    // TLS cert pin
    void sslFingerprintPrompt(const ServerId &host, const QString &fingerprint);

    // DCC
    void dccSendReceived(const ServerId &server, const QString &fromNick,
                         const QString &filename, quint32 ip, quint16 port, qint64 filesize);
    void dccPassiveOfferReceived(const ServerId &server, const QString &fromNick,
                                  const QString &filename, quint32 ip,
                                  qint64 filesize, const QString &token);
    void dccPassiveSendReply(const ServerId &server, const QString &fromNick,
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
    void onHistoryBatchDone(const QString &host, const QString &target);
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

    void postMessage(const ServerId &host, const BufferId &target, const Message &msg);
    void logMessage (const ServerId &host, const BufferId &target, const Message &msg);
    // Route unaddressed replies: active buffer if this server is focused,
    // else the "(server)" buffer.
    BufferId activeOrServer(const ServerId &host) const;

    QList<ServerSession> m_sessions;
    QList<IrcClient *>   m_clients;
    NetworkMonitor      *m_netMonitor{nullptr};
    Config               m_config;
    QHash<QString, IgnoreTypes> m_ignoredNicks;
    QHash<QString, QFile*> m_logFiles;

    ServerId m_activeHost;
    BufferId m_activeChannel;

    void seedFromLog(const ServerId &host, const BufferId &target);

    QSet<QString> m_logSeeded;            // buffers already seeded from local logs this run
    QSet<QString> m_pendingHistoryBefore; // "host\tchannel" keys awaiting CHATHISTORY BEFORE
    QHash<QString, QList<Message>> m_historyBeforeBuf; // collected prepend messages

    void queueReadMark(const ServerId &host, const BufferId &channel);
    void flushReadMarks();
    QHash<QString, QPair<ServerId, BufferId>> m_pendingReadMarks; // bufferKey → buffer
    bool m_readMarkQueued{false};
};
