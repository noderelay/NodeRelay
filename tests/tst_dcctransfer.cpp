// Real localhost DCC transfers between DccSend and DccReceive, active and
// passive, including the controller's teardown pattern (deleteLater from the
// finished handler while socket events are still in flight). Written while
// chasing a post-transfer crash on both ends of a LAN transfer.
#include <QtTest>
#include <QHostAddress>
#include <QSignalSpy>
#include <QTemporaryDir>

#include "irc/dccsend.h"
#include "irc/dccreceive.h"

class TestDccTransfer : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_dir;
    QString m_srcPath;
    QByteArray m_payload;

    QString makeSource(qint64 size)
    {
        m_payload.clear();
        m_payload.reserve(size);
        while (m_payload.size() < size)
            m_payload.append("0123456789abcdef", 16);
        m_payload.truncate(size);
        const QString path = m_dir.filePath("source.bin");
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly)) return {};
        f.write(m_payload);
        return path;
    }

private slots:
    void initTestCase()
    {
        QVERIFY(m_dir.isValid());
        // Big enough for several 16 KB chunks and multiple 64 KB ACK boundaries.
        m_srcPath = makeSource(300 * 1024 + 7);
        QVERIFY(!m_srcPath.isEmpty());
    }

    void activeTransfer()
    {
        auto *send = new DccSend(m_srcPath, this);
        QVERIFY(send->listen(QHostAddress(QHostAddress::LocalHost), std::nullopt, 0, 0));
        QVERIFY(send->port() != 0);

        const QString savePath = m_dir.filePath("active.bin");
        auto *recv = new DccReceive(savePath,
                                    QHostAddress("127.0.0.1").toIPv4Address(),
                                    send->port(), send->filesize(), this);
        recv->setAllowPrivatePeer(true);

        QSignalSpy sendDone(send, &DccSend::finished);
        QSignalSpy recvDone(recv, &DccReceive::finished);
        QSignalSpy sendErr(send, &DccSend::error);
        QSignalSpy recvErr(recv, &DccReceive::error);

        // Mirror DccController: both ends deleteLater'd from their finished
        // handlers while their sockets still have disconnect events pending.
        connect(send, &DccSend::finished, send, &QObject::deleteLater);
        connect(recv, &DccReceive::finished, recv, &QObject::deleteLater);

        recv->start();

        QTRY_COMPARE_WITH_TIMEOUT(recvDone.count(), 1, 10000);
        QTRY_COMPARE_WITH_TIMEOUT(sendDone.count(), 1, 10000);

        // Let disconnects, stray errors and the deferred deletes land.
        QTest::qWait(300);

        QCOMPARE(sendErr.count(), 0);
        QCOMPARE(recvErr.count(), 0);

        QFile out(savePath);
        QVERIFY(out.open(QIODevice::ReadOnly));
        QCOMPARE(out.readAll(), m_payload);
    }

    void passiveTransfer()
    {
        auto *send = new DccSend(m_srcPath, this);
        const QString token = send->initPassive();
        QVERIFY(!token.isEmpty());

        const QString savePath = m_dir.filePath("passive.bin");
        auto *recv = new DccReceive(savePath, 0, 0, send->filesize(), this);
        QVERIFY(recv->listenPassive(QHostAddress("127.0.0.1").toIPv4Address(), 0, 0));
        QVERIFY(recv->listenPort() != 0);

        QSignalSpy sendDone(send, &DccSend::finished);
        QSignalSpy recvDone(recv, &DccReceive::finished);
        QSignalSpy sendErr(send, &DccSend::error);
        QSignalSpy recvErr(recv, &DccReceive::error);

        connect(send, &DccSend::finished, send, &QObject::deleteLater);
        connect(recv, &DccReceive::finished, recv, &QObject::deleteLater);

        send->connectOut(QHostAddress("127.0.0.1").toIPv4Address(), recv->listenPort());

        QTRY_COMPARE_WITH_TIMEOUT(recvDone.count(), 1, 10000);
        QTRY_COMPARE_WITH_TIMEOUT(sendDone.count(), 1, 10000);

        QTest::qWait(300);

        QCOMPARE(sendErr.count(), 0);
        QCOMPARE(recvErr.count(), 0);

        QFile out(savePath);
        QVERIFY(out.open(QIODevice::ReadOnly));
        QCOMPARE(out.readAll(), m_payload);
    }
};

QTEST_MAIN(TestDccTransfer)
#include "tst_dcctransfer.moc"
