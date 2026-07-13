#pragma once

#include <QFile>
#include <QObject>

class QTcpServer;
class QTcpSocket;

class DccReceive : public QObject
{
    Q_OBJECT
public:
    DccReceive(const QString &savePath, quint32 ip, quint16 port, qint64 filesize,
               QObject *parent = nullptr);
    ~DccReceive();

    void    start();
    bool    listenPassive(quint32 expectedIp = 0);
    quint16 listenPort() const;
    void    cancel();

signals:
    void progress(qint64 received, qint64 total);
    void finished(const QString &path);
    void error(const QString &msg);

private slots:
    void onReadyRead();
    void onSocketError();

private:
    void armStallGuard();

    QString     m_savePath;
    QTcpServer *m_server{nullptr};
    QTcpSocket *m_socket{nullptr};
    QFile       m_file;
    quint32     m_ip;
    quint16     m_port;
    qint64      m_total;
    qint64      m_received{0};
    qint64      m_lastReceivedSeen{0};
    bool        m_done{false};
};
