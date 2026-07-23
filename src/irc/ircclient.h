#pragma once

#include "config/config.h"
#include "irc/stsstore.h"
#include <QDateTime>
#include <QHash>
#include <QList>
#include <QObject>
#include <QSet>
#include <QHostAddress>
#include <QSslSocket>
#include <QElapsedTimer>
#include <QTimer>

class QWebSocket;
struct ServerConfig;

class IrcClient : public QObject
{
    Q_OBJECT

public:
    explicit IrcClient(QObject *parent = nullptr);
    ~IrcClient() override;

    void connectToServer(const ServerConfig &cfg);
    void quit(const QString &reason = {});
    void abort();
    void reconnect();
    void onNetworkOnline();   // reachability regained; respects intentional disconnects
    void setPinnedFingerprint(const QString &fp) { m_pinnedFingerprint = fp; }

    void join(const QString &channel, const QString &key = {});
    void part(const QString &channel, const QString &reason = {});
    void privmsg      (const QString &target, const QString &text, const QString &replyToMsgid = {});
    void sendMultiline(const QString &target, const QString &text, const QString &replyToMsgid = {});
    void notice(const QString &target, const QString &text);
    void setNick(const QString &nick);
    void sendTyping(const QString &channel, const QString &state);
    bool sendReact (const QString &target,  const QString &msgid, const QString &emoji);
    void sendRaw(const QString &line);
    void sendRedact(const QString &target, const QString &msgid, const QString &reason = {});

    // Bouncer helpers
    void requestHistory(const QString &target, int limit = 100);
    // Returns false when the server negotiated no chathistory cap (nothing sent).
    bool requestHistoryBefore(const QString &target, const QDateTime &before, int limit = 100);
    void markRead(const QString &target, const QDateTime &ts);

    // Monitor
    void setMonitorList(const QStringList &nicks);
    void monitorAdd   (const QString &nick);
    void monitorRemove(const QString &nick);
    void monitorClear ();
    void monitorStatus();

    QString currentNick() const { return m_nick; }
    QString serverName()  const { return m_serverName; }
    QString host()        const { return m_host; }
    bool    isConnected() const;
    bool    hasCap(const QString &cap) const { return m_ackedCaps.contains(cap); }
    bool    hasMetadataCap() const { return m_ackedCaps.contains("draft/metadata-3")
                                         || m_ackedCaps.contains("draft/metadata-2"); }
    bool    supportsWhox() const { return m_supportsWhox; }
    QStringList ackedCaps() const { auto l = m_ackedCaps.values(); l.sort(); return l; }
    quint32 localIpv4()  const;

signals:
    void connected(const QString &server);
    void disconnected(const QString &server);
    void socketError(const QString &server, const QString &error);

    void messageReceived(const QString &server, const QString &target,
                         const QString &nick,   const QString &text,
                         const QDateTime &serverTime, bool isHistory,
                         const QString &msgid, const QString &replyTo);
    void noticeReceived (const QString &server, const QString &target,
                         const QString &nick,   const QString &text,
                         const QDateTime &serverTime, bool isHistory,
                         const QString &msgid, const QString &replyTo);
    void actionReceived (const QString &server, const QString &target,
                         const QString &nick,   const QString &text,
                         const QDateTime &serverTime, bool isHistory,
                         const QString &msgid);

    void userJoined   (const QString &server, const QString &channel, const QString &nick, const QString &user, const QString &host);
    void userParted   (const QString &server, const QString &channel, const QString &nick, const QString &user, const QString &host, const QString &reason);
    void userQuit     (const QString &server, const QString &nick,    const QString &user, const QString &host, const QString &reason);
    void nickChanged  (const QString &server, const QString &oldNick, const QString &newNick);
    void kicked       (const QString &server, const QString &channel, const QString &nick,
                       const QString &by,     const QString &reason);

    void topicReceived    (const QString &server, const QString &channel, const QString &topic);
    void topicSetByReceived(const QString &server, const QString &channel,
                            const QString &setter, quint64 timestamp);
    void awayChanged      (const QString &server, bool away);
    void modesReceived    (const QString &server, const QString &channel, const QString &modes);
    void namesReceived    (const QString &server, const QString &channel, const QStringList &nicks);
    void namesDone        (const QString &server, const QString &channel);
    void whoEntryReceived (const QString &server, const QString &channel,
                           const QString &nick,   const QString &flags);

    void serverMessage     (const QString &server, const QString &text);
    void errorMessage      (const QString &server, const QString &text);
    void contextualMessage (const QString &server, const QString &text);
    void wallopsReceived   (const QString &server, const QString &nick, const QString &text);
    void pingRtt        (const QString &host, int ms);
    void reconnecting   (const QString &host);
    void ctcpPingReply  (const QString &server, const QString &nick, qint64 rttMs);
    void ctcpTimeReply  (const QString &server, const QString &nick, const QString &timeStr);
    void rawReceived  (const QString &line);
    void selfNickChanged(const QString &server, const QString &newNick);
    void sslFingerprintPrompt(const QString &host, const QString &fingerprint);
    void dccSendReceived(const QString &server, const QString &fromNick,
                         const QString &filename, quint32 ip, quint16 port, qint64 filesize);
    void dccPassiveOfferReceived(const QString &server, const QString &fromNick,
                                  const QString &filename, quint32 ip,
                                  qint64 filesize, const QString &token);
    void dccPassiveSendReply(const QString &server, const QString &fromNick,
                              const QString &filename, quint32 ip, quint16 port,
                              qint64 filesize, const QString &token);
    void typingReceived(const QString &server, const QString &channel,
                        const QString &nick,   const QString &state);
    void hostChanged   (const QString &server, const QString &nick,
                        const QString &newUser, const QString &newHost);
    void reactReceived (const QString &server, const QString &target,
                        const QString &nick,   const QString &msgid,
                        const QString &emoji);
    void accountChanged(const QString &server, const QString &nick, const QString &account);
    void messageRedacted(const QString &server, const QString &senderNick,
                         const QString &target, const QString &msgid, const QString &reason);
    void inviteNotify  (const QString &server, const QString &inviter,
                        const QString &channel, const QString &targetNick);
    void setNameReceived(const QString &server, const QString &nick, const QString &realname);
    void monitorOnline (const QString &server, const QStringList &nicks);
    void monitorOffline(const QString &server, const QStringList &nicks);
    void userMetaChanged    (const QString &server, const QString &nick,
                             const QString &key,    const QString &value);
    void metaLookupFailed   (const QString &server, const QString &nick);
    void channelListEntry   (const QString &server, const QString &channel, int users, const QString &topic);
    void channelListEnd     (const QString &server, int total);
    void channelListReceived(const QString &server, const QList<QStringList> &entries);
    void bouncerNetworksListed(const QString &server, const QList<QStringList> &networks);
    void netsplitDetected(const QString &server, const QString &splitServers, const QStringList &nicks);
    void netjoinDetected (const QString &server, const QString &splitServers,
                          const QStringList &channels, const QStringList &nicks);
    void standardReply   (const QString &server, const QString &channel,
                          const QString &severity, const QString &text);

    // Bouncer-specific
    void bouncerNetworkReceived(const QString &server, const QString &netId,
                                const QString &netName, bool isConnected);
    void readMarkerReceived(const QString &server, const QString &target,
                            const QDateTime &ts);
    // A chathistory batch for `target` closed (fires even when it carried
    // zero messages — that's how "no more history" is detected).
    void historyBatchDone(const QString &server, const QString &target);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onWsTextReceived(const QString &message);
    void onSslErrors(const QList<QSslError> &errors);
    void onErrorOccurred(QAbstractSocket::SocketError error);
    void doReconnect();
    void sendPing();

private:
    void processLine(const QString &line);
    void applyProxy();
    void sockWrite(const QString &line);
    void sockDisconnect();
    void sockAbort();
    QAbstractSocket::SocketState sockState() const;
    QHostAddress                 sockLocalAddress() const;
    QString                      sockErrorString() const;
    void handleCap     (const QStringList &params, const QString &trailing);
    void handleNumeric (const QString &cmd, const QStringList &params, const QString &trailing);
    void handleBatch   (const QStringList &params);
    void handleBouncer (const QStringList &params, const QString &trailing);
    void deliverBatch  (const QString &ref);
    void scheduleReconnect();

    QSslSocket  *m_socket;
    QWebSocket  *m_wsSocket{nullptr};
    bool         m_useWs{false};
    QString      m_serverName;
    QString      m_host;
    quint16      m_port{6697};
    bool         m_ssl{true};
    QString      m_nick;
    QString      m_user;
    QString      m_realname;
    QString      m_password;
    QString      m_saslUser;
    QString      m_saslPassword;
    bool         m_saslExternal{false};
    bool         m_saslPending{false};
    bool         m_away{false};
    QString      m_awayMsg;
    QString      m_nickservPassword;
    QByteArray   m_buffer;

    BouncerType  m_bouncerType{BouncerType::None};
    QString      m_bouncerNetwork;
    QString      m_pinnedFingerprint;
    QString      m_proxyHost;
    quint16      m_proxyPort{1080};
    QString      m_proxyUser;
    QString      m_proxyPass;
    QString      m_quitMessage;

    QTimer      *m_reconnectTimer{nullptr};
    QTimer      *m_pingTimer{nullptr};
    QElapsedTimer m_pingClock;
    bool         m_pingPending{false};
    bool         m_intentionalDisconnect{false};
    bool         m_stsUpgrade{false};
    bool         m_utf8Only{false};
    bool         m_supportsWhox{false};
    int          m_reconnectDelay{5};

    QSet<QString>               m_requestedCaps;
    QSet<QString>               m_ackedCaps;
    QStringList                 m_capLsBuffer;
    bool                        m_registered{false};
    QHash<QString, QStringList> m_namesBuffer;
    QList<QStringList>          m_listBuffer;         // [channel, count, topic] per entry
    QList<QStringList>          m_bouncerNetBuf;      // [id, name, state] per soju network
    bool                        m_bouncerListing{false};
    bool                        m_bouncerListingUnsupported{false}; // persists across reconnects

    // Batch tracking
    struct BatchInfo {
        QString type;
        QString param; // e.g., channel for chathistory
        struct Msg {
            QString     command;
            QString     nick;
            QStringList params;
            QString     trailing;
            QDateTime   serverTime;
            QString     msgid;
            QString     replyTo;
            bool        concat{false}; // draft/multiline-concat: append without newline
        };
        QList<Msg> msgs;
    };
    QHash<QString, BatchInfo> m_batches;

    QStringList   m_monitorList;
    QSet<QString> m_monitorSet; // lowercase mirror for O(1) membership check

    // CTCP reply rate-limit: key = "nick:cmd", value = last-reply ms timestamp
    QHash<QString, qint64> m_ctcpTimestamps;
};
