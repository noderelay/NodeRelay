#include <QtTest/QtTest>
#include "config/config.h"
#include <QTemporaryFile>

#define LOAD(varName, tomlStr) \
    QTemporaryFile varName##_tmp; \
    varName##_tmp.setAutoRemove(true); \
    QVERIFY(varName##_tmp.open()); \
    varName##_tmp.write(tomlStr); \
    varName##_tmp.flush(); \
    const Config varName = Config::load(varName##_tmp.fileName())

class TstConfig : public QObject
{
    Q_OBJECT

private slots:

    void loadMinimalServer()
    {
        LOAD(cfg, R"(
[[server]]
host = "irc.example.org"
nick = "joe"
)");
        QCOMPARE(cfg.servers.size(), 1);
        const auto &s = cfg.servers[0];
        QCOMPARE(s.host, "irc.example.org");
        QCOMPARE(s.nick, "joe");
        QCOMPARE(s.port, quint16(6697));
        QVERIFY(s.ssl);
        QCOMPARE(s.user, "uplink");
        QCOMPARE(s.realname, "Uplink User");
        QCOMPARE(s.bouncerType, BouncerType::None);
        QVERIFY(!s.websocket);
        QVERIFY(!s.disabled);
    }

    void loadUiSection()
    {
        LOAD(cfg, R"(
[ui]
theme = "dracula"
show_nick_prefix = false
colored_nicks = false
font_chat = 14
nick_brackets = "[]"
typing_indicator = false
log_messages = true
show_timestamps = false
highlight_words = "alert,urgent"
)");
        QCOMPARE(cfg.ui.theme, "dracula");
        QVERIFY(!cfg.ui.showNickPrefix);
        QVERIFY(!cfg.ui.coloredNicks);
        QCOMPARE(cfg.ui.fontSizes.chat, 14.0);
        QCOMPARE(cfg.ui.nickBrackets, "[]");
        QVERIFY(!cfg.ui.typingIndicator);
        QVERIFY(cfg.ui.logMessages);
        QVERIFY(!cfg.ui.showTimestamps);
        QCOMPARE(cfg.ui.highlightWords, "alert,urgent");
    }

    void loadMultipleServers()
    {
        LOAD(cfg, R"(
[[server]]
host = "irc.one.org"
nick = "joe"
port = 6667
ssl = false

[[server]]
host = "irc.two.org"
nick = "joe2"
)");
        QCOMPARE(cfg.servers.size(), 2);
        QCOMPARE(cfg.servers[0].host, "irc.one.org");
        QCOMPARE(cfg.servers[0].port, quint16(6667));
        QVERIFY(!cfg.servers[0].ssl);
        QCOMPARE(cfg.servers[1].host, "irc.two.org");
        QCOMPARE(cfg.servers[1].nick, "joe2");
        QCOMPARE(cfg.servers[1].port, quint16(6697));
    }

    void emptyHostSkipped()
    {
        LOAD(cfg, R"(
[[server]]
nick = "joe"

[[server]]
host = "irc.real.org"
nick = "joe"
)");
        QCOMPARE(cfg.servers.size(), 1);
        QCOMPARE(cfg.servers[0].host, "irc.real.org");
    }

    void loadOldStyleChannels()
    {
        LOAD(cfg, R"(
[[server]]
host = "irc.example.org"
nick = "joe"
channels = "#foo, #bar, #baz"
)");
        QCOMPARE(cfg.servers[0].channels.size(), 3);
        QCOMPARE(cfg.servers[0].channels[0].name, "#foo");
        QCOMPARE(cfg.servers[0].channels[1].name, "#bar");
        QCOMPARE(cfg.servers[0].channels[2].name, "#baz");
        QVERIFY(cfg.servers[0].channels[0].password.isEmpty());
    }

    void loadNewStyleChannels()
    {
        LOAD(cfg, R"(
[[server]]
host = "irc.example.org"
nick = "joe"

[[server.channel]]
name = "#secret"
key = "mypass"

[[server.channel]]
name = "#open"
)");
        QCOMPARE(cfg.servers[0].channels.size(), 2);
        QCOMPARE(cfg.servers[0].channels[0].name, "#secret");
        QCOMPARE(cfg.servers[0].channels[0].password, "mypass");
        QCOMPARE(cfg.servers[0].channels[1].name, "#open");
        QVERIFY(cfg.servers[0].channels[1].password.isEmpty());
    }

    void loadIgnoreOldFormat()
    {
        LOAD(cfg, R"(
[ignore]
nicks = ["BadUser", "Spammer"]
)");
        QCOMPARE(cfg.ignoreList.size(), 2);
        QCOMPARE(cfg.ignoreList[0].nick, "baduser");
        QCOMPARE(cfg.ignoreList[0].flags, kIgnoreAll);
        QCOMPARE(cfg.ignoreList[1].nick, "spammer");
    }

    void loadIgnoreNewFormat()
    {
        LOAD(cfg, R"(
[[ignore.entry]]
nick = "Troll"
flags = ["pm", "notice"]

[[ignore.entry]]
nick = "Pest"
)");
        QCOMPARE(cfg.ignoreList.size(), 2);
        QCOMPARE(cfg.ignoreList[0].nick, "troll");
        QVERIFY(cfg.ignoreList[0].flags & IgnoreType::PM);
        QVERIFY(cfg.ignoreList[0].flags & IgnoreType::Notice);
        QVERIFY(!(cfg.ignoreList[0].flags & IgnoreType::Invite));
        QCOMPARE(cfg.ignoreList[1].nick, "pest");
        QCOMPARE(cfg.ignoreList[1].flags, kIgnoreAll);
    }

    void appIconMigration()
    {
        LOAD(cfg, R"(
[ui]
app_icon = "dark"
)");
        QCOMPARE(cfg.ui.appIcon, "flat-black");

        LOAD(cfg2, R"(
[ui]
app_icon = "light"
)");
        QCOMPARE(cfg2.ui.appIcon, "original-flat-shine");
    }

    void bouncerTypeParsing()
    {
        LOAD(cfg, R"(
[[server]]
host = "znc.example.org"
nick = "joe"
bouncer = "znc"

[[server]]
host = "soju.example.org"
nick = "joe"
bouncer = "soju"

[[server]]
host = "plain.example.org"
nick = "joe"
)");
        QCOMPARE(cfg.servers[0].bouncerType, BouncerType::ZNC);
        QCOMPARE(cfg.servers[1].bouncerType, BouncerType::Soju);
        QCOMPARE(cfg.servers[2].bouncerType, BouncerType::None);
    }

    void saveLoadRoundTrip()
    {
        Config orig;
        orig.ui.theme = "nord";
        orig.ui.coloredNicks = false;
        orig.ui.fontSizes.chat = 14.0;
        orig.ui.nickBrackets = "[]";
        orig.ui.logMessages = true;
        orig.ui.linkPreviews = true;
        orig.profileDisplayName = "Joe";
        orig.profileAvatarUrl = "https://example.com/avatar.png";

        ServerConfig sc;
        sc.name = "TestNet";
        sc.host = "irc.test.org";
        sc.port = 6667;
        sc.ssl = false;
        sc.nick = "testjoe";
        sc.user = "tj";
        sc.realname = "Test Joe";
        sc.bouncerType = BouncerType::Soju;
        sc.bouncerNetwork = "libera";
        sc.quitMessage = "bye";
        sc.channels.append(ChannelConfig{"#dev", QString()});
        sc.channels.append(ChannelConfig{"#chat", QString()});
        orig.servers.append(sc);

        orig.ignoreList.append({"troll", IgnoreType::PM | IgnoreType::Notice});
        orig.monitorList.append("friend1");
        orig.monitorList.append("friend2");

        QTemporaryFile tmp;
        tmp.setAutoRemove(true);
        QVERIFY(tmp.open());
        const QString path = tmp.fileName();
        tmp.close();

        Config::save(orig, path, false);
        Config loaded = Config::load(path);

        QCOMPARE(loaded.ui.theme, "nord");
        QVERIFY(!loaded.ui.coloredNicks);
        QCOMPARE(loaded.ui.fontSizes.chat, 14.0);
        QCOMPARE(loaded.ui.nickBrackets, "[]");
        QVERIFY(loaded.ui.logMessages);
        QVERIFY(loaded.ui.linkPreviews);
        QCOMPARE(loaded.profileDisplayName, "Joe");
        QCOMPARE(loaded.profileAvatarUrl, "https://example.com/avatar.png");

        QCOMPARE(loaded.servers.size(), 1);
        const auto &ls = loaded.servers[0];
        QCOMPARE(ls.name, "TestNet");
        QCOMPARE(ls.host, "irc.test.org");
        QCOMPARE(ls.port, quint16(6667));
        QVERIFY(!ls.ssl);
        QCOMPARE(ls.nick, "testjoe");
        QCOMPARE(ls.user, "tj");
        QCOMPARE(ls.realname, "Test Joe");
        QCOMPARE(ls.bouncerType, BouncerType::Soju);
        QCOMPARE(ls.bouncerNetwork, "libera");
        QCOMPARE(ls.quitMessage, "bye");
        QCOMPARE(ls.channels.size(), 2);
        QCOMPARE(ls.channels[0].name, "#dev");
        QCOMPARE(ls.channels[1].name, "#chat");

        QCOMPARE(loaded.ignoreList.size(), 1);
        QCOMPARE(loaded.ignoreList[0].nick, "troll");
        QVERIFY(loaded.ignoreList[0].flags & IgnoreType::PM);
        QVERIFY(loaded.ignoreList[0].flags & IgnoreType::Notice);
        QVERIFY(!(loaded.ignoreList[0].flags & IgnoreType::Invite));

        QCOMPARE(loaded.monitorList.size(), 2);
        QCOMPARE(loaded.monitorList[0], "friend1");
    }

    void needsNickSetup()
    {
        Config cfg;
        ServerConfig sc;
        sc.host = "irc.example.org";

        sc.nick = "yournick";
        cfg.servers = {sc};
        QVERIFY(cfg.needsNickSetup());

        sc.nick = "";
        cfg.servers = {sc};
        QVERIFY(cfg.needsNickSetup());

        sc.nick = "joe";
        cfg.servers = {sc};
        QVERIFY(!cfg.needsNickSetup());
    }
};

int main(int argc, char *argv[])
{
    fprintf(stderr, "tst_config: main() entered\n");
    fflush(stderr);
    QCoreApplication app(argc, argv);
    fprintf(stderr, "tst_config: QCoreApplication created\n");
    fflush(stderr);
    TstConfig tc;
    int ret = QTest::qExec(&tc, argc, argv);
    fprintf(stderr, "tst_config: QTest::qExec returned %d\n", ret);
    fflush(stderr);
    return ret;
}

#include "tst_config.moc"
