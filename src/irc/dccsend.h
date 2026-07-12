#pragma once

#include <QFile>
#include <QHostAddress>
#include <QObject>
#include <optional>

class QTcpServer;
class QTcpSocket;

class DccSend : public QObject
{
    Q_OBJECT
public:
    explicit DccSend(const QString &filepath, QObject *parent = nullptr);

    bool    listen(const QHostAddress &bindAddr = QHostAddress::Any,
                   std::optional<QHostAddress> expectedPeer = std::nullopt);
    QString initPassive();
    void    connectOut(quint32 ip, quint16 port);
    void    cancel();
    quint16 port()     const;
    QString filename() const;
    qint64  filesize() const;
    const QString &token() const { return m_token; }

signals:
    void progress(qint64 sent, qint64 total);
    void finished();
    void error(const QString &msg);

private slots:
    void onNewConnection();
    void onReadyRead();
    void onBytesWritten(qint64);

private:
    void sendNextChunk();
    void armStallGuard();

    QTcpServer *m_server{nullptr};
    QTcpSocket *m_socket{nullptr};
    QFile       m_file;
    QString     m_token;
    qint64      m_sent{0};
    qint64      m_acked{0};       // highest cumulative ACK from the peer
    qint64      m_lastAckSeen{0}; // m_acked at the previous stall check
    qint64      m_filesize{0};
    std::optional<QHostAddress> m_expectedPeer;

    bool        m_finished{false};

    static constexpr qint32 kChunk = 65536;
};
