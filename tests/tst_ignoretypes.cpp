#include <QtTest/QtTest>
#include "config/config.h"
#include <QHash>

class TstIgnoreTypes : public QObject
{
    Q_OBJECT

private slots:
    void kIgnoreAllHasAllFlags()
    {
        QVERIFY(kIgnoreAll & IgnoreType::PM);
        QVERIFY(kIgnoreAll & IgnoreType::Notice);
        QVERIFY(kIgnoreAll & IgnoreType::Invite);
    }

    void singleFlagIsolation()
    {
        IgnoreTypes flags = IgnoreType::PM;
        QVERIFY(flags & IgnoreType::PM);
        QVERIFY(!(flags & IgnoreType::Notice));
        QVERIFY(!(flags & IgnoreType::Invite));
    }

    void flagCombination()
    {
        IgnoreTypes flags = IgnoreType::PM | IgnoreType::Notice;
        QVERIFY(flags & IgnoreType::PM);
        QVERIFY(flags & IgnoreType::Notice);
        QVERIFY(!(flags & IgnoreType::Invite));
    }

    void caseInsensitiveLookup()
    {
        QHash<QString, IgnoreTypes> map;
        map.insert("baduser", IgnoreType::PM);
        QVERIFY(map.contains(QString("BadUser").toLower()));
        QCOMPARE(map.value(QString("BADUSER").toLower()), IgnoreTypes(IgnoreType::PM));
    }

    void ignoreEntryDefaults()
    {
        IgnoreEntry entry;
        entry.nick = "someone";
        QCOMPARE(entry.flags, kIgnoreAll);
    }

    void flagsOverwrite()
    {
        QHash<QString, IgnoreTypes> map;
        map.insert("nick", IgnoreType::PM);
        map.insert("nick", IgnoreType::Notice);
        QCOMPARE(map.value("nick"), IgnoreTypes(IgnoreType::Notice));
        QVERIFY(!(map.value("nick") & IgnoreType::PM));
    }
};

QTEST_GUILESS_MAIN(TstIgnoreTypes)
#include "tst_ignoretypes.moc"
