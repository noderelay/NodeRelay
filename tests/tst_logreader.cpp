#include <QtTest/QtTest>
#include "model/logreader.h"

#include <QTemporaryDir>

// Mirrors SessionModel::logMessage()'s output format.
static QString logLine(const QDateTime &ts, MessageType type,
                       const QString &nick, const QString &text)
{
    const QString t = ts.toString("yyyy-MM-dd hh:mm:ss");
    switch (type) {
    case MessageType::Privmsg: return "[" + t + "] <" + nick + "> " + text + "\n";
    case MessageType::Action:  return "[" + t + "] * " + nick + " " + text + "\n";
    case MessageType::Notice:  return "[" + t + "] -" + nick + "- " + text + "\n";
    default:                   return "[" + t + "] -- " + text + "\n";
    }
}

class TstLogReader : public QObject
{
    Q_OBJECT

    QTemporaryDir m_dir;
    QDateTime m_base; // 12:00:00 local

    QString writeLog(const QStringList &lines, const QString &name = "test.log")
    {
        const QString path = m_dir.filePath(name);
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return {};
        for (const QString &l : lines)
            f.write(l.toUtf8());
        return path;
    }

private slots:
    void initTestCase()
    {
        QVERIFY(m_dir.isValid());
        m_base = QDateTime(QDate(2026, 7, 20), QTime(12, 0, 0));
    }

    void parsesAllLineTypes()
    {
        Message m;
        QVERIFY(LogReader::parseLine("[2026-07-20 12:00:01] <alice> hi there", m));
        QCOMPARE(m.type, MessageType::Privmsg);
        QCOMPARE(m.nick, "alice");
        QCOMPARE(m.text, "hi there");
        QCOMPARE(m.timestamp, m_base.addSecs(1));
        QVERIFY(m.isHistory);

        QVERIFY(LogReader::parseLine("[2026-07-20 12:00:02] * bob waves", m));
        QCOMPARE(m.type, MessageType::Action);
        QCOMPARE(m.nick, "bob");
        QCOMPARE(m.text, "waves");

        QVERIFY(LogReader::parseLine("[2026-07-20 12:00:03] -ChanServ- mode set", m));
        QCOMPARE(m.type, MessageType::Notice);
        QCOMPARE(m.nick, "ChanServ");
        QCOMPARE(m.text, "mode set");

        QVERIFY(LogReader::parseLine("[2026-07-20 12:00:04] -- carol has joined", m));
        QCOMPARE(m.type, MessageType::Server);
        QVERIFY(m.nick.isEmpty());
        QCOMPARE(m.text, "carol has joined");
    }

    void rejectsGarbage()
    {
        Message m;
        QVERIFY(!LogReader::parseLine("", m));
        QVERIFY(!LogReader::parseLine("no timestamp here", m));
        QVERIFY(!LogReader::parseLine("[not a date stamp!] <a> b", m));
        QVERIFY(!LogReader::parseLine("[2026-07-20 12:00:01] unknown shape", m));
    }

    void emptyTextTolerated()
    {
        // Trailing spaces can be lost in transit; parse the trimmed shapes too.
        Message m;
        QVERIFY(LogReader::parseLine("[2026-07-20 12:00:01] <alice>", m));
        QVERIFY(m.text.isEmpty());
        QVERIFY(LogReader::parseLine("[2026-07-20 12:00:01] * bob", m));
        QCOMPARE(m.nick, "bob");
        QVERIFY(m.text.isEmpty());
    }

    void readsChronologicalBeforeBoundary()
    {
        QStringList lines;
        for (int i = 0; i < 10; ++i)
            lines << logLine(m_base.addSecs(i), MessageType::Privmsg,
                             "nick", QString("msg %1").arg(i));
        const QString path = writeLog(lines);

        // Boundary at second 7, one in-memory message in that second.
        const auto msgs = LogReader::readBefore(path, m_base.addSecs(7), 1, 100);
        QCOMPARE(msgs.size(), 7);
        QCOMPARE(msgs.first().text, "msg 0");
        QCOMPARE(msgs.last().text, "msg 6");
    }

    void limitKeepsNewest()
    {
        QStringList lines;
        for (int i = 0; i < 20; ++i)
            lines << logLine(m_base.addSecs(i), MessageType::Privmsg,
                             "nick", QString("msg %1").arg(i));
        const QString path = writeLog(lines);

        const auto msgs = LogReader::readBefore(path, m_base.addSecs(15), 1, 5);
        QCOMPARE(msgs.size(), 5);
        QCOMPARE(msgs.first().text, "msg 10");
        QCOMPARE(msgs.last().text, "msg 14");
    }

    void sameSecondTiesSkipped()
    {
        // Three messages in the boundary second; two are still in memory.
        QStringList lines;
        lines << logLine(m_base, MessageType::Privmsg, "n", "old")
              << logLine(m_base.addSecs(5), MessageType::Privmsg, "n", "tie A")
              << logLine(m_base.addSecs(5), MessageType::Privmsg, "n", "tie B")
              << logLine(m_base.addSecs(5), MessageType::Privmsg, "n", "tie C");
        const QString path = writeLog(lines);

        const auto msgs = LogReader::readBefore(path, m_base.addSecs(5), 2, 100);
        QCOMPARE(msgs.size(), 2);
        QCOMPARE(msgs.first().text, "old");
        QCOMPARE(msgs.last().text, "tie A");
    }

    void smallBlocksMatchLargeBlocks()
    {
        // Lines with multibyte text spanning block boundaries.
        QStringList lines;
        for (int i = 0; i < 50; ++i)
            lines << logLine(m_base.addSecs(i), MessageType::Privmsg,
                             "émile", QString("héllo 🚀 %1").arg(i));
        const QString path = writeLog(lines);

        const auto big   = LogReader::readBefore(path, m_base.addSecs(40), 1, 100);
        const auto small = LogReader::readBefore(path, m_base.addSecs(40), 1, 100, 16);
        QCOMPARE(big.size(), 40);
        QCOMPARE(small.size(), big.size());
        for (qsizetype i = 0; i < big.size(); ++i) {
            QCOMPARE(small[i].text, big[i].text);
            QCOMPARE(small[i].nick, big[i].nick);
            QCOMPARE(small[i].timestamp, big[i].timestamp);
        }
    }

    void crlfTolerated()
    {
        const QString path = writeLog({
            "[2026-07-20 11:59:00] <a> one\r\n",
            "[2026-07-20 11:59:01] <a> two\r\n",
        }, "crlf.log");
        const auto msgs = LogReader::readBefore(path, m_base, 0, 100);
        QCOMPARE(msgs.size(), 2);
        QCOMPARE(msgs.last().text, "two");
    }

    void garbageLinesSkipped()
    {
        const QString path = writeLog({
            "corrupted line\n",
            logLine(m_base, MessageType::Privmsg, "a", "kept"),
            "\n",
        }, "garbage.log");
        const auto msgs = LogReader::readBefore(path, m_base.addSecs(60), 0, 100);
        QCOMPARE(msgs.size(), 1);
        QCOMPARE(msgs.first().text, "kept");
    }

    void missingFileReturnsEmpty()
    {
        QVERIFY(LogReader::readBefore(m_dir.filePath("nope.log"),
                                      m_base, 0, 100).isEmpty());
    }

    void exhaustionReturnsEmpty()
    {
        const QString path = writeLog({logLine(m_base.addSecs(5),
                                               MessageType::Privmsg, "a", "only")},
                                      "exhausted.log");
        // Everything in the file is at or after the boundary.
        QVERIFY(LogReader::readBefore(path, m_base, 0, 100).isEmpty());
        QVERIFY(LogReader::readBefore(path, m_base.addSecs(5), 1, 100).isEmpty());
    }
};

QTEST_GUILESS_MAIN(TstLogReader)
#include "tst_logreader.moc"
