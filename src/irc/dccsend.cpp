#include "dccsend.h"

#include <QFileInfo>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QtEndian>

DccSend::DccSend(const QString &filepath, QObject *parent)
    : QObject(parent)
    , m_file(filepath)
{}

bool DccSend::listen(const QHostAddress &bindAddr, std::optional<QHostAddress> expectedPeer)
{
    m_expectedPeer = expectedPeer;
    if (!m_file.open(QIODevice::ReadOnly)) {
        emit error("Cannot open: " + m_file.fileName());
        return false;
    }
    m_filesize = QFileInfo(m_file.fileName()).size();

    const qint64 size = m_filesize;
    if (size <= 0 || size > static_cast<qint64>(UINT32_MAX)) {
        emit error(size <= 0 ? "File is empty" : "File too large for DCC (max 4 GiB)");
        m_file.close();
        return false;
    }

    m_server = new QTcpServer(this);
    if (!m_server->listen(bindAddr, 0)) {
        emit error("Cannot bind listen port");
        m_file.close();
        return false;
    }
    connect(m_server, &QTcpServer::newConnection, this, &DccSend::onNewConnection);

    QTimer::singleShot(60000, this, [this]{
        if (!m_socket) {
            cancel();
            emit error("No connection received (timeout)");
        }
    });
    return true;
}

QString DccSend::initPassive()
{
    if (!m_file.open(QIODevice::ReadOnly)) {
        emit error("Cannot open: " + m_file.fileName());
        return {};
    }
    m_filesize = QFileInfo(m_file.fileName()).size();
    const qint64 size = m_filesize;
    if (size <= 0 || size > static_cast<qint64>(UINT32_MAX)) {
        emit error(size <= 0 ? "File is empty" : "File too large for DCC (max 4 GiB)");
        m_file.close();
        return {};
    }
    // 128-bit token (4 × 32-bit words → 32 hex chars) — replaces the prior 8-digit decimal.
    auto *rng = QRandomGenerator::global();
    m_token = QString("%1%2%3%4")
        .arg(rng->generate(), 8, 16, QLatin1Char('0'))
        .arg(rng->generate(), 8, 16, QLatin1Char('0'))
        .arg(rng->generate(), 8, 16, QLatin1Char('0'))
        .arg(rng->generate(), 8, 16, QLatin1Char('0'));
    return m_token;
}

void DccSend::connectOut(quint32 ip, quint16 port)
{
    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::connected,          this, [this]{
        armStallGuard();
        sendNextChunk();
    });
    connect(m_socket, &QTcpSocket::readyRead,          this, &DccSend::onReadyRead);
    connect(m_socket, &QTcpSocket::bytesWritten,       this, &DccSend::onBytesWritten);
    connect(m_socket, &QAbstractSocket::errorOccurred, this, [this]{
        if (!m_finished) emit error(m_socket->errorString());
    });
    m_socket->connectToHost(QHostAddress(ip), port);
}

void DccSend::cancel()
{
    if (m_finished) return;
    m_finished = true;
    if (m_server) m_server->close();
    if (m_socket) m_socket->abort();
    m_file.close();
}

quint16 DccSend::port() const
{
    return m_server ? m_server->serverPort() : 0;
}

QString DccSend::filename() const
{
    QString name = QFileInfo(m_file).fileName();
    name.replace(' ', '_');
    name.remove(QRegularExpression(QStringLiteral("[\\x00-\\x1f\\x7f]")));
    if (name.isEmpty()) name = QStringLiteral("file");
    return name.left(180);
}

qint64 DccSend::filesize() const
{
    return m_filesize;
}

void DccSend::onNewConnection()
{
    QTcpSocket *incoming = m_server->nextPendingConnection();
    if (m_expectedPeer && incoming->peerAddress() != *m_expectedPeer) {
        incoming->abort();
        incoming->deleteLater();
        return;
    }
    m_socket = incoming;
    m_server->close();
    connect(m_socket, &QTcpSocket::readyRead,     this, &DccSend::onReadyRead);
    connect(m_socket, &QTcpSocket::bytesWritten,  this, &DccSend::onBytesWritten);
    connect(m_socket, &QAbstractSocket::errorOccurred, this, [this]{
        if (!m_finished) emit error(m_socket->errorString());
    });
    armStallGuard();
    sendNextChunk();
}

// Stall guard: abort only when the peer's cumulative ACK count has not
// advanced between two 60s checks — a one-shot deadline would kill any
// healthy transfer that simply takes longer than 60s.
void DccSend::armStallGuard()
{
    auto *stall = new QTimer(this);
    connect(stall, &QTimer::timeout, this, [this, stall]{
        if (m_finished || !m_socket) {
            stall->stop();
            stall->deleteLater();
            return;
        }
        if (m_acked == m_lastAckSeen) {
            stall->stop();
            stall->deleteLater();
            cancel();
            emit error("Transfer stalled: no ACK received");
            return;
        }
        m_lastAckSeen = m_acked;
    });
    stall->start(60000);
}

void DccSend::onReadyRead()
{
    // ACKs are 4-byte big-endian cumulative byte counts
    while (m_socket->bytesAvailable() >= 4) {
        quint32 raw;
        m_socket->read(reinterpret_cast<char *>(&raw), 4);
        m_acked = qMax(m_acked, static_cast<qint64>(qFromBigEndian(raw)));
        if (m_acked >= filesize()) {
            m_finished = true;
            emit finished();
            return;
        }
    }
}

void DccSend::onBytesWritten(qint64)
{
    sendNextChunk();
}

void DccSend::sendNextChunk()
{
    if (m_finished) return;
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) return;
    if (m_socket->bytesToWrite() > kChunk * 4) return;
    const QByteArray chunk = m_file.read(kChunk);
    if (chunk.isEmpty()) return;  // EOF — wait for final ACK
    m_socket->write(chunk);
    m_sent += chunk.size();
    emit progress(m_sent, filesize());
}
