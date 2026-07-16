#include <QtTest/QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include "plugins/pluginprotocol.h"

using namespace PluginProtocol;

static QJsonObject parseLine(const QByteArray &line)
{
    return QJsonDocument::fromJson(line).object();
}

class TstPluginProtocol : public QObject
{
    Q_OBJECT

private slots:
    // events
    void helloShape();
    void privmsgEvent();
    void actionAndNoticeKinds();
    void mentionsAndAccount();
    void joinPartQuitKick();
    void nickCarriesOldAndNew();
    void syntheticNetsplitSkipped();
    void serverTypesSkipped();
    void topicAndServerEvents();
    void oneObjectPerLine();
    // actions
    void sayAction();
    void meAndPrintActions();
    void commandUsesLineField();
    void malformedJson();
    void unknownAction();
    void missingFields();
    void utf8RoundTrip();
    void oversizedLineStillParses();
};

static Message mkMsg(MessageType t, const QString &nick, const QString &text,
                     const QString &msgid = {}, const QString &replyTo = {},
                     const QString &account = {})
{
    return Message::make(t, nick, text, QDateTime::fromString("2026-07-16T19:00:00Z", Qt::ISODate),
                         false, msgid, replyTo, account);
}

void TstPluginProtocol::helloShape()
{
    const auto o = parseLine(helloEvent("2026.7.6"));
    QCOMPARE(o.value("event").toString(),   QStringLiteral("hello"));
    QCOMPARE(o.value("version").toString(), QStringLiteral("2026.7.6"));
}

void TstPluginProtocol::privmsgEvent()
{
    const auto line = messageEvent("LiberaChat", "#uplinkirc",
                                   mkMsg(MessageType::Privmsg, "alice", "hi joe", "m1"),
                                   false, false);
    const auto o = parseLine(line);
    QCOMPARE(o.value("event").toString(),  QStringLiteral("message"));
    QCOMPARE(o.value("kind").toString(),   QStringLiteral("message"));
    QCOMPARE(o.value("server").toString(), QStringLiteral("LiberaChat"));
    QCOMPARE(o.value("buffer").toString(), QStringLiteral("#uplinkirc"));
    QCOMPARE(o.value("nick").toString(),   QStringLiteral("alice"));
    QCOMPARE(o.value("text").toString(),   QStringLiteral("hi joe"));
    QCOMPARE(o.value("msgid").toString(),  QStringLiteral("m1"));
    QCOMPARE(o.value("self").toBool(),     false);
    QVERIFY(o.value("time").toString().startsWith("2026-07-16T"));
}

void TstPluginProtocol::actionAndNoticeKinds()
{
    auto o = parseLine(messageEvent("s", "#c", mkMsg(MessageType::Action, "a", "waves"), false, false));
    QCOMPARE(o.value("kind").toString(), QStringLiteral("action"));
    o = parseLine(messageEvent("s", "#c", mkMsg(MessageType::Notice, "a", "psst"), false, false));
    QCOMPARE(o.value("kind").toString(), QStringLiteral("notice"));
}

void TstPluginProtocol::mentionsAndAccount()
{
    const auto o = parseLine(messageEvent("s", "#c",
        mkMsg(MessageType::Privmsg, "a", "joe: hi", {}, {}, "alice_acct"), false, true));
    QCOMPARE(o.value("mentions_you").toBool(), true);
    QCOMPARE(o.value("account").toString(), QStringLiteral("alice_acct"));
    // absent msgid must be omitted, not empty
    QVERIFY(!o.contains("msgid"));
}

void TstPluginProtocol::joinPartQuitKick()
{
    auto o = parseLine(messageEvent("s", "#c", mkMsg(MessageType::Join, "bob", "bob has joined the channel"), false, false));
    QCOMPARE(o.value("event").toString(), QStringLiteral("join"));
    QCOMPARE(o.value("nick").toString(),  QStringLiteral("bob"));
    o = parseLine(messageEvent("s", "#c", mkMsg(MessageType::Part, "bob", "bob has left the channel"), false, false));
    QCOMPARE(o.value("event").toString(), QStringLiteral("part"));
    o = parseLine(messageEvent("s", "#c", mkMsg(MessageType::Quit, "bob", "bob has quit"), false, false));
    QCOMPARE(o.value("event").toString(), QStringLiteral("quit"));
    o = parseLine(messageEvent("s", "#c", mkMsg(MessageType::Kick, "bob", "bob was kicked by op"), false, false));
    QCOMPARE(o.value("event").toString(), QStringLiteral("kick"));
    // join/part/quit/kick carry no message-only fields
    QVERIFY(!o.contains("kind"));
    QVERIFY(!o.contains("mentions_you"));
}

void TstPluginProtocol::nickCarriesOldAndNew()
{
    const auto o = parseLine(messageEvent("s", "#c",
        mkMsg(MessageType::Nick, "oldnick", "oldnick is now known as newnick", {}, "newnick"), false, false));
    QCOMPARE(o.value("event").toString(), QStringLiteral("nick"));
    QCOMPARE(o.value("old").toString(),   QStringLiteral("oldnick"));
    QCOMPARE(o.value("new").toString(),   QStringLiteral("newnick"));
}

void TstPluginProtocol::syntheticNetsplitSkipped()
{
    QVERIFY(messageEvent("s", "#c", mkMsg(MessageType::Quit, QString(), "Netsplit: 3 users lost"), false, false).isEmpty());
    QVERIFY(messageEvent("s", "#c", mkMsg(MessageType::Join, QString(), "Netjoin: 3 users returned"), false, false).isEmpty());
}

void TstPluginProtocol::serverTypesSkipped()
{
    for (auto t : { MessageType::Server, MessageType::Reply, MessageType::Wallops,
                    MessageType::Topic, MessageType::Error })
        QVERIFY(messageEvent("s", "#c", mkMsg(t, "n", "text"), false, false).isEmpty());
}

void TstPluginProtocol::topicAndServerEvents()
{
    auto o = parseLine(topicEvent("s", "#c", "new topic"));
    QCOMPARE(o.value("event").toString(), QStringLiteral("topic"));
    QCOMPARE(o.value("text").toString(),  QStringLiteral("new topic"));
    o = parseLine(serverEvent("connected", "LiberaChat"));
    QCOMPARE(o.value("event").toString(),  QStringLiteral("connected"));
    QCOMPARE(o.value("server").toString(), QStringLiteral("LiberaChat"));
}

void TstPluginProtocol::oneObjectPerLine()
{
    const auto line = helloEvent("v");
    QVERIFY(line.endsWith('\n'));
    QVERIFY(!line.chopped(1).contains('\n'));
}

void TstPluginProtocol::sayAction()
{
    const auto a = parseAction(R"({"action":"say","server":"s","buffer":"#c","text":"hello"})");
    QCOMPARE(a.kind,   PluginAction::Say);
    QCOMPARE(a.server, QStringLiteral("s"));
    QCOMPARE(a.buffer, QStringLiteral("#c"));
    QCOMPARE(a.text,   QStringLiteral("hello"));
    QVERIFY(a.error.isEmpty());
}

void TstPluginProtocol::meAndPrintActions()
{
    QCOMPARE(parseAction(R"({"action":"me","server":"s","buffer":"#c","text":"waves"})").kind,
             PluginAction::Me);
    QCOMPARE(parseAction(R"({"action":"print","server":"s","buffer":"#c","text":"note"})").kind,
             PluginAction::Print);
}

void TstPluginProtocol::commandUsesLineField()
{
    const auto a = parseAction(R"({"action":"command","server":"s","line":"/join #test"})");
    QCOMPARE(a.kind, PluginAction::Command);
    QCOMPARE(a.text, QStringLiteral("/join #test"));
    // buffer is optional for commands
    QVERIFY(a.error.isEmpty());
}

void TstPluginProtocol::malformedJson()
{
    QCOMPARE(parseAction("not json at all").kind,      PluginAction::Invalid);
    QCOMPARE(parseAction(R"({"action":"say")").kind,   PluginAction::Invalid);
    QCOMPARE(parseAction(R"([1,2,3])").kind,           PluginAction::Invalid);
    QCOMPARE(parseAction("").kind,                     PluginAction::Invalid);
    QCOMPARE(parseAction("   \n").kind,                PluginAction::Invalid);
    QVERIFY(!parseAction("not json").error.isEmpty());
}

void TstPluginProtocol::unknownAction()
{
    const auto a = parseAction(R"({"action":"reboot","server":"s","buffer":"#c","text":"x"})");
    QCOMPARE(a.kind, PluginAction::Invalid);
    QVERIFY(a.error.contains("unknown action"));
    QVERIFY(parseAction(R"({"server":"s"})").error.contains("missing \"action\""));
}

void TstPluginProtocol::missingFields()
{
    QCOMPARE(parseAction(R"({"action":"say","buffer":"#c","text":"x"})").kind,  PluginAction::Invalid);
    QCOMPARE(parseAction(R"({"action":"say","server":"s","text":"x"})").kind,   PluginAction::Invalid);
    QCOMPARE(parseAction(R"({"action":"say","server":"s","buffer":"#c"})").kind, PluginAction::Invalid);
}

void TstPluginProtocol::utf8RoundTrip()
{
    const auto line = messageEvent("s", "#c",
        mkMsg(MessageType::Privmsg, "ünïcode", QString::fromUtf8("héllo 世界 🎉")), false, false);
    const auto o = parseLine(line);
    QCOMPARE(o.value("nick").toString(), QString::fromUtf8("ünïcode"));
    QCOMPARE(o.value("text").toString(), QString::fromUtf8("héllo 世界 🎉"));

    const auto a = parseAction(QString(R"({"action":"say","server":"s","buffer":"#c","text":"héllo 世界"})").toUtf8());
    QCOMPARE(a.text, QString::fromUtf8("héllo 世界"));
}

void TstPluginProtocol::oversizedLineStillParses()
{
    const QString big(100000, QChar('x'));
    const auto a = parseAction(QString(R"({"action":"say","server":"s","buffer":"#c","text":"%1"})").arg(big).toUtf8());
    QCOMPARE(a.kind, PluginAction::Say);
    QCOMPARE(a.text.size(), big.size());
}

QTEST_MAIN(TstPluginProtocol)
#include "tst_pluginprotocol.moc"
