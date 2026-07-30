#include "dccreceive.h"
#include "dccports.h"
#include "logging.h"
#include "net/addresscheck.h"

#include <QFileInfo>
#include <QHostAddress>
#include <QStorageInfo>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QtEndian>

// Maximum file size accepted via DCC receive (2 GiB).
static constexpr qint64 kMaxDccReceiveBytes = 2LL * 1024 * 1024 * 1024;

DccReceive::DccReceive(const QString &savePath, quint32 ip, quint16 port, qint64 filesize,
                       QObject *parent)
    : QObject(parent)
    , m_savePath(savePath)
    , m_file(savePath + ".part")
    , m_ip(ip)
    , m_port(port)
    , m_total(filesize)
{}

DccReceive::~DccReceive()
{
    // Remove any partial file that was never renamed to the final path.
    if (m_file.isOpen()) {
        const QString path = m_file.fileName();
        m_file.close();
        QFile::remove(path);
    }
}

static bool checkTransferPrecon(qint64 total, const QString &savePath, DccReceive *dcc)
{
    if (total <= 0) {
        emit dcc->error("Invalid file size");
        return false;
    }
    if (total > kMaxDccReceiveBytes) {
        emit dcc->error(QString("File too large (%1 MB); max is %2 MB")
            .arg(total / (1024 * 1024))
            .arg(kMaxDccReceiveBytes / (1024 * 1024)));
        return false;
    }
    const QStorageInfo storage(QFileInfo(savePath).absolutePath());
    if (storage.isValid() && storage.bytesAvailable() < total) {
        emit dcc->error(QString("Not enough disk space (need %1 MB, have %2 MB)")
            .arg(total / (1024 * 1024))
            .arg(storage.bytesAvailable() / (1024 * 1024)));
        return false;
    }
    return true;
}

void DccReceive::start()
{
    const QHostAddress peerAddr(m_ip);
    if (!m_allowPrivatePeer && isPrivateAddress(peerAddr)) {
        emit error("DCC blocked: peer address " + peerAddr.toString()
                   + " is private or reserved (Preferences > File Transfers permits LAN transfers)");
        return;
    }

    if (!checkTransferPrecon(m_total, m_savePath, this))
        return;

    if (!m_file.open(QIODevice::WriteOnly)) {
        emit error("Cannot create: " + m_file.fileName());
        return;
    }
    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::readyRead,          this, &DccReceive::onReadyRead);
    connect(m_socket, &QAbstractSocket::errorOccurred, this, &DccReceive::onSocketError);
    connect(m_socket, &QTcpSocket::connected,          this, &DccReceive::armStallGuard);

    QTimer::singleShot(30000, this, [this]{
        if (m_socket && m_socket->state() != QAbstractSocket::ConnectedState) {
            cancel();
            emit error("Connection timeout");
        }
    });

    qCDebug(lcDcc) << "recv: connecting to" << peerAddr.toString() << m_port
                   << "expecting" << m_total << "bytes";
    m_socket->connectToHost(peerAddr, m_port);
}

bool DccReceive::listenPassive(quint32 expectedIp, quint16 portMin, quint16 portMax)
{
    if (!checkTransferPrecon(m_total, m_savePath, this))
        return false;

    if (!m_file.open(QIODevice::WriteOnly)) {
        emit error("Cannot create: " + m_file.fileName());
        return false;
    }
    m_server = new QTcpServer(this);
    if (!dccListen(m_server, QHostAddress::Any, portMin, portMax)) {
        emit error("Cannot bind listen port");
        cancel(); // closes and removes the freshly created .part file
        return false;
    }

    // Only validate peer IP when the expected address is a known global (non-private) address.
    // With NAT/bouncers the advertised IP may differ from the real source, so skip validation
    // for private addresses rather than blocking legitimate transfers.
    const QHostAddress expected(expectedIp);
    const bool validatePeer = expectedIp != 0 && !isPrivateAddress(expected);

    connect(m_server, &QTcpServer::newConnection, this, [this, expected, validatePeer] {
        QTcpSocket *incoming = m_server->nextPendingConnection();
        if (m_socket) {
            // Already have our peer — a second queued connection must not
            // replace the socket mid-transfer.
            incoming->abort();
            incoming->deleteLater();
            return;
        }
        if (validatePeer) {
            // Reject connections from unexpected peers (race/injection protection).
            if (incoming->peerAddress().toIPv4Address() != expected.toIPv4Address()) {
                qCDebug(lcDcc) << "recv: rejected unexpected peer"
                               << incoming->peerAddress().toString();
                incoming->abort();
                incoming->deleteLater();
                return;
            }
        }
        m_socket = incoming;
        m_server->close();
        qCDebug(lcDcc) << "recv: peer connected from" << m_socket->peerAddress().toString();
        connect(m_socket, &QTcpSocket::readyRead,          this, &DccReceive::onReadyRead);
        connect(m_socket, &QAbstractSocket::errorOccurred, this, &DccReceive::onSocketError);
        // Stall guard: if no data arrives within 30s of the peer connecting, abort.
        // Matters when expectedIp is private and peer validation was skipped (NAT case).
        QTimer::singleShot(30000, this, [this]{
            if (m_socket && m_received == 0) {
                m_socket->disconnect(this);
                cancel();
                emit error("Transfer stalled: no data received after connect");
            }
        });
        armStallGuard();
    });
    QTimer::singleShot(60000, this, [this]{
        if (!m_socket) {
            cancel();
            emit error("No connection received (timeout)");
        }
    });
    qCDebug(lcDcc) << "recv (passive): listening on port" << m_server->serverPort()
                   << "expecting" << m_total << "bytes";
    return true;
}

quint16 DccReceive::listenPort() const
{
    return m_server ? m_server->serverPort() : 0;
}

// Stall guard: abort only when the received byte count has not advanced
// between two 60s checks — a one-shot deadline would kill any healthy
// transfer that simply takes longer than 60s. Mirrors DccSend's ACK guard.
void DccReceive::armStallGuard()
{
    auto *stall = new QTimer(this);
    connect(stall, &QTimer::timeout, this, [this, stall]{
        if (m_done || !m_socket || m_received >= m_total) {
            stall->stop();
            stall->deleteLater();
            return;
        }
        if (m_received == m_lastReceivedSeen) {
            stall->stop();
            stall->deleteLater();
            m_socket->disconnect(this);
            cancel();
            emit error("Transfer stalled: peer stopped sending");
            return;
        }
        m_lastReceivedSeen = m_received;
    });
    stall->start(60000);
}

void DccReceive::cancel()
{
    if (m_done) return;
    m_done = true;
    if (m_server) m_server->close();
    if (m_socket) m_socket->abort();
    if (m_file.isOpen()) {
        const QString path = m_file.fileName();
        m_file.close();
        QFile::remove(path);
    }
}

void DccReceive::onReadyRead()
{
    if (m_done) return;
    const qint64 remaining = m_total - m_received;
    if (remaining <= 0) {
        m_socket->disconnectFromHost();
        return;
    }
    const QByteArray data = m_socket->read(qMin(m_socket->bytesAvailable(), remaining));
    if (m_file.write(data) != data.size()) {
        m_socket->disconnect(this); // non-null: this slot only fires on its readyRead
        cancel();
        emit error("Write failed");
        return;
    }
    const qint64 prevAck = m_received;
    m_received += data.size();

    // Send cumulative ACK every 64 KB boundary and always on completion.
    // Coalescing avoids a 4-byte write syscall for every readyRead on fast links.
    static constexpr qint64 kAckInterval = 65536;
    const bool crossedBoundary = (m_received / kAckInterval) > (prevAck / kAckInterval);
    if (crossedBoundary || m_received >= m_total) {
        const quint32 ack = qToBigEndian(static_cast<quint32>(m_received));
        m_socket->write(reinterpret_cast<const char *>(&ack), 4);
    }

    emit progress(m_received, m_total);

    if (m_received >= m_total) {
        qCDebug(lcDcc) << "recv: complete," << m_received << "bytes";
        const QString partPath = m_file.fileName();
        m_file.close();
        m_socket->disconnectFromHost();
        if (QFile::rename(partPath, m_savePath))
            emit finished(m_savePath);
        else
            emit error("Download complete but rename failed: " + m_savePath);
    }
}

void DccReceive::onSocketError()
{
    if (m_done) return;
    const QString msg = m_socket ? m_socket->errorString() : QString("Unknown socket error");
    if (m_socket)
        m_socket->disconnect(this);
    cancel();
    emit error(msg);
}
