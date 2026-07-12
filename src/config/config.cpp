#include "config.h"
#include "keychainhelper.h"

#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QStandardPaths>
#include <QDebug>

#include <toml++/toml.hpp>

static QString kcKey(const QString &serverName, const char *field)
{
    return serverName + QLatin1Char(':') + QLatin1String(field);
}

// Write a password to keychain and return the sentinel, or return empty.
static QString storePassword(const QString &value, const QString &serverName, const char *field)
{
    if (value.isEmpty()) {
        KeychainHelper::remove(kcKey(serverName, field));
        return {};
    }
    if (value == kKeychainSentinel)
        return kKeychainSentinel;
    if (KeychainHelper::write(kcKey(serverName, field), value))
        return kKeychainSentinel;

    qWarning() << "Uplink: keychain write failed for" << serverName << field
               << "— password will not be persisted";
    return {};
}

static QString tomlQuote(QString s)
{
    s.replace("\\", "\\\\");
    s.replace("\"", "\\\"");
    s.replace("\n", "\\n");
    s.replace("\r", "\\r");
    s.replace("\t", "\\t");
    return "\"" + s + "\"";
}

static const char *kDefaultConfig = R"(
[ui]
theme             = "default"
show_nick_prefix  = true
show_topic        = true
show_emoji_button = false
colored_nicks     = true
typing_indicator  = true
hanging_indent    = true
show_timestamps   = true
highlight_words   = ""           # comma-separated words that trigger highlight (e.g. "myproject,alert")
nick_brackets     = "<>"         # "<>" angle, "[]" square, "::::" double-colon, "" none
pane_stack_rows   = false        # false = stack panes in columns, true = stack in rows
panel_cards       = true         # side panels use their own theme colors + rounded tops; false = classic flat look
menu_style        = "menubar"    # "menubar" File/Edit/... bar (joins the KDE global menu), "hidden" shortcuts only
font_family       = "IBM Plex Mono"
font_toolbar    = 10
font_sidebar    = 10
font_chat       = 10
font_nick_list  = 10
font_nick_dock  = 10
font_topic_bar  = 10
font_topic_text = 10
font_input_nick = 10
font_input      = 10

[[server]]
name     = "LinuxDojo"
host     = "irc.linuxdojo.org"
port     = 6697
ssl      = true
nick     = "yournick"
user     = "yournick"
realname = "Uplink User"
channels = "#uplink"
)";

// ---------------------------------------------------------------------------

QString Config::defaultPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
           + "/.config/uplink/config.toml";
}

void Config::ensureExists(const QString &path)
{
    QFileInfo fi(path);
    if (fi.exists() && fi.size() > 0) return;

    QDir().mkpath(fi.absolutePath());
    QFile f(path);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        f.write(kDefaultConfig);
        f.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    } else {
        qWarning() << "Uplink: could not create config at" << path;
    }
}

Config Config::load(const QString &path)
{
    ensureExists(path);

    Config cfg;

    try {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "Uplink: could not open config at" << path;
            return cfg;
        }
        const std::string content = f.readAll().toStdString();
        auto tbl = toml::parse(content, path.toStdString());

        // [ui]
        if (auto ui = tbl["ui"].as_table()) {
            auto ustr = [&](const char *key, std::string def = "") -> QString {
                return QString::fromStdString((*ui)[key].value_or<std::string>(std::move(def)));
            };
            cfg.ui.theme           = ustr("theme", "default");
            cfg.ui.showNickPrefix  = (*ui)["show_nick_prefix"].value_or(true);
            cfg.ui.showTopic       = (*ui)["show_topic"].value_or(true);
            cfg.ui.showEmojiButton = (*ui)["show_emoji_button"].value_or(false);
            cfg.ui.showSendButton  = (*ui)["show_send_button"].value_or(true);
            cfg.ui.coloredNicks    = (*ui)["colored_nicks"].value_or(true);
            cfg.ui.typingIndicator = (*ui)["typing_indicator"].value_or(true);
            cfg.ui.hangingIndent   = (*ui)["hanging_indent"].value_or(true);
            cfg.ui.logMessages       = (*ui)["log_messages"].value_or(false);
            cfg.ui.showUnreadCounts  = (*ui)["show_unread_counts"].value_or(true);
            cfg.ui.showTimestamps    = (*ui)["show_timestamps"].value_or(true);
            cfg.ui.highlightWords    = ustr("highlight_words", "");
            cfg.ui.appIcon           = ustr("app_icon", "flat-black");
            if (cfg.ui.appIcon == "dark")  cfg.ui.appIcon = "flat-black";
            if (cfg.ui.appIcon == "light") cfg.ui.appIcon = "original-flat-shine";
            cfg.ui.nickBrackets    = ustr("nick_brackets", "<>");
            cfg.ui.paneStackRows   = (*ui)["pane_stack_rows"].value_or(false);
            cfg.ui.panelCards      = (*ui)["panel_cards"].value_or(true);
            cfg.ui.menuStyle       = ustr("menu_style", "menubar");
            if (cfg.ui.menuStyle != "hidden")
                cfg.ui.menuStyle = "menubar";
            cfg.ui.notifications   = (*ui)["notifications"].value_or(true);
            cfg.ui.fontFamily      = ustr("font_family", kDefaultFontFamily);
            cfg.ui.fontSizes.toolbar      = (*ui)["font_toolbar"].value_or(10.0);
            cfg.ui.fontSizes.serverHeader = (*ui)["font_server_header"].value_or(9.0);
            cfg.ui.fontSizes.sidebar      = (*ui)["font_sidebar"].value_or(10.0);
            cfg.ui.fontSizes.chat         = (*ui)["font_chat"].value_or(10.0);
            cfg.ui.fontSizes.nickList     = (*ui)["font_nick_list"].value_or(10.0);
#if defined(Q_OS_MAC)
            cfg.ui.fontSizes.nickDock     = (*ui)["font_nick_dock"].value_or(13.0);
#else
            cfg.ui.fontSizes.nickDock     = (*ui)["font_nick_dock"].value_or(9.0);
#endif
            cfg.ui.fontSizes.topicBar     = (*ui)["font_topic_bar"].value_or(10.0);
            cfg.ui.fontSizes.topicText    = (*ui)["font_topic_text"].value_or(10.0);
            cfg.ui.fontSizes.inputNick    = (*ui)["font_input_nick"].value_or(10.0);
            cfg.ui.fontSizes.input        = (*ui)["font_input"].value_or(10.0);
            cfg.ui.fontSizes.typing       = (*ui)["font_typing"].value_or(9.0);
            cfg.ui.fontSizes.emoji        = (*ui)["font_emoji"].value_or(16.0);
        }

        // [privacy]
        if (auto priv = tbl["privacy"].as_table())
            cfg.ui.linkPreviews = (*priv)["link_previews"].value_or(false);

        // [profile]
        if (auto prof = tbl["profile"].as_table()) {
            cfg.profileDisplayName = QString::fromStdString((*prof)["display_name"].value_or<std::string>(""));
            cfg.profileAvatarUrl   = QString::fromStdString((*prof)["avatar_url"].value_or<std::string>(""));
        }

        // [ignore]
        if (auto ign = tbl["ignore"].as_table()) {
            // Backwards compat: old nicks = [...] array
            if (auto nicks = (*ign)["nicks"].as_array()) {
                for (auto &n : *nicks) {
                    if (auto v = n.value<std::string>()) {
                        const QString nick = QString::fromStdString(*v).trimmed().toLower();
                        if (!nick.isEmpty())
                            cfg.ignoreList.append({nick, kIgnoreAll});
                    }
                }
            }
            // New format: [[ignore.entry]]
            if (auto entries = (*ign)["entry"].as_array()) {
                for (auto &enode : *entries) {
                    auto *e = enode.as_table();
                    if (!e) continue;
                    const QString nick = QString::fromStdString(
                        (*e)["nick"].value_or<std::string>("")).trimmed().toLower();
                    if (nick.isEmpty()) continue;
                    IgnoreTypes flags;
                    if (auto fa = (*e)["flags"].as_array()) {
                        for (auto &fn : *fa) {
                            if (auto fv = fn.value<std::string>()) {
                                if (*fv == "pm")     flags |= IgnoreType::PM;
                                if (*fv == "notice") flags |= IgnoreType::Notice;
                                if (*fv == "invite") flags |= IgnoreType::Invite;
                            }
                        }
                    } else {
                        flags = kIgnoreAll;
                    }
                    if (flags)
                        cfg.ignoreList.append({nick, flags});
                }
            }
        }

        // [monitor]
        if (auto mon = tbl["monitor"].as_table()) {
            if (auto nicks = (*mon)["nicks"].as_array()) {
                for (auto &n : *nicks) {
                    if (auto v = n.value<std::string>()) {
                        const QString nick = QString::fromStdString(*v).trimmed();
                        if (!nick.isEmpty())
                            cfg.monitorList.append(nick);
                    }
                }
            }
        }

        // [[script]]
        if (auto scripts = tbl["script"].as_array()) {
            for (auto &node : *scripts) {
                auto *s = node.as_table();
                if (!s) continue;
                ScriptBinding sb;
                sb.command = QString::fromStdString((*s)["command"].value_or<std::string>("")).trimmed().toLower();
                sb.path    = QString::fromStdString((*s)["path"].value_or<std::string>(""));
                sb.enabled = (*s)["enabled"].value_or(true);
                if (!sb.command.isEmpty() && !sb.path.isEmpty())
                    cfg.scripts.append(sb);
            }
        }

        // [[server]]
        if (auto servers = tbl["server"].as_array()) {
            for (auto &node : *servers) {
                auto *s = node.as_table();
                if (!s) continue;

                auto sstr = [&](const char *key, std::string def = "") -> QString {
                    return QString::fromStdString((*s)[key].value_or<std::string>(std::move(def)));
                };

                ServerConfig sc;
                sc.name     = sstr("name");
                sc.host     = sstr("host");
                sc.port     = static_cast<quint16>((*s)["port"].value_or(6697));
                sc.ssl      = (*s)["ssl"].value_or(true);
                sc.nick     = sstr("nick");
                sc.user     = sstr("user", "uplink");
                sc.realname = sstr("realname", "Uplink User");
                sc.password         = sstr("password");
                sc.saslUser         = sstr("sasl_user");
                sc.saslPassword     = sstr("sasl_password");
                sc.saslExternal     = (*s)["sasl_external"].value_or(false);
                sc.clientCertFile   = sstr("client_cert");
                sc.clientKeyFile    = sstr("client_key");
                sc.nickservPassword = sstr("nickserv_password");
                const std::string bt = (*s)["bouncer"].value_or<std::string>("none");
                if      (bt == "znc")  sc.bouncerType = BouncerType::ZNC;
                else if (bt == "soju") sc.bouncerType = BouncerType::Soju;
                else                   sc.bouncerType = BouncerType::None;
                sc.bouncerNetwork    = sstr("bouncer_network");
                sc.proxyHost         = sstr("proxy_host");
                sc.proxyPort         = static_cast<quint16>((*s)["proxy_port"].value_or(1080));
                sc.proxyUser         = sstr("proxy_user");
                sc.proxyPass         = sstr("proxy_pass");
                sc.pinnedFingerprint = sstr("ssl_fingerprint");
                sc.websocket         = (*s)["websocket"].value_or(false);
                sc.disabled          = (*s)["disabled"].value_or(false);
                sc.quitMessage       = sstr("quit_message");
                sc.awayMessage       = sstr("away_message");

                if (auto chans = (*s)["channel"].as_array()) {
                    for (auto &cnode : *chans) {
                        auto *c = cnode.as_table();
                        if (!c) continue;
                        ChannelConfig cc;
                        cc.name     = QString::fromStdString((*c)["name"].value_or<std::string>(""));
                        cc.password = QString::fromStdString((*c)["key"].value_or<std::string>(""));
                        if (!cc.name.isEmpty())
                            sc.channels.append(cc);
                    }
                } else {
                    // backward compat: old comma-separated string
                    const std::string chanStr = (*s)["channels"].value_or<std::string>("");
                    for (const QString &part : QString::fromStdString(chanStr).split(',', Qt::SkipEmptyParts)) {
                        const QString name = part.trimmed();
                        if (!name.isEmpty())
                            sc.channels.append({name, {}});
                    }
                }

                if (!sc.host.isEmpty())
                    cfg.servers.append(sc);
            }
        }

    } catch (const toml::parse_error &e) {
        qWarning() << "Uplink: config parse error:" << e.what();
    }

    installDefaultScripts(cfg.scripts);

    return cfg;
}

void Config::save(const Config &cfg, const QString &path, bool migratePasswords)
{
    QDir().mkpath(QFileInfo(path).absolutePath());

    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Uplink: could not open config for writing at" << path;
        return;
    }

    QTextStream out(&f);

    auto boolStr = [](bool b) -> const char * { return b ? "true" : "false"; };

    out << "[ui]\n";
    out << "theme = " << tomlQuote(cfg.ui.theme) << "\n";
    out << "show_nick_prefix = " << boolStr(cfg.ui.showNickPrefix) << "\n";
    out << "show_topic = " << boolStr(cfg.ui.showTopic) << "\n";
    out << "show_emoji_button = " << boolStr(cfg.ui.showEmojiButton) << "\n";
    out << "show_send_button = " << boolStr(cfg.ui.showSendButton) << "\n";
    out << "colored_nicks = " << boolStr(cfg.ui.coloredNicks) << "\n";
    out << "typing_indicator = " << boolStr(cfg.ui.typingIndicator) << "\n";
    out << "hanging_indent = " << boolStr(cfg.ui.hangingIndent) << "\n";
    out << "log_messages = " << boolStr(cfg.ui.logMessages) << "\n";
    out << "show_unread_counts = " << boolStr(cfg.ui.showUnreadCounts) << "\n";
    out << "show_timestamps = " << boolStr(cfg.ui.showTimestamps) << "\n";
    out << "highlight_words = " << tomlQuote(cfg.ui.highlightWords) << "\n";
    out << "app_icon = " << tomlQuote(cfg.ui.appIcon) << "\n";
    out << "nick_brackets = " << tomlQuote(cfg.ui.nickBrackets) << "\n";
    out << "pane_stack_rows = " << boolStr(cfg.ui.paneStackRows) << "\n";
    out << "panel_cards = " << boolStr(cfg.ui.panelCards) << "\n";
    out << "menu_style = " << tomlQuote(cfg.ui.menuStyle) << "\n";
    out << "notifications = " << boolStr(cfg.ui.notifications) << "\n";
    out << "font_family = " << tomlQuote(cfg.ui.fontFamily) << "\n";
    out << "font_toolbar = " << cfg.ui.fontSizes.toolbar << "\n";
    out << "font_server_header = " << cfg.ui.fontSizes.serverHeader << "\n";
    out << "font_sidebar = " << cfg.ui.fontSizes.sidebar << "\n";
    out << "font_chat = " << cfg.ui.fontSizes.chat << "\n";
    out << "font_nick_list = " << cfg.ui.fontSizes.nickList << "\n";
    out << "font_nick_dock = " << cfg.ui.fontSizes.nickDock << "\n";
    out << "font_topic_bar = " << cfg.ui.fontSizes.topicBar << "\n";
    out << "font_topic_text = " << cfg.ui.fontSizes.topicText << "\n";
    out << "font_input_nick = " << cfg.ui.fontSizes.inputNick << "\n";
    out << "font_input = " << cfg.ui.fontSizes.input << "\n";
    out << "font_typing = " << cfg.ui.fontSizes.typing << "\n";
    out << "font_emoji = " << cfg.ui.fontSizes.emoji << "\n\n";

    out << "[privacy]\n";
    out << "link_previews = " << boolStr(cfg.ui.linkPreviews) << "\n\n";

    if (!cfg.profileDisplayName.isEmpty() || !cfg.profileAvatarUrl.isEmpty()) {
        out << "[profile]\n";
        if (!cfg.profileDisplayName.isEmpty())
            out << "display_name = " << tomlQuote(cfg.profileDisplayName) << "\n";
        if (!cfg.profileAvatarUrl.isEmpty())
            out << "avatar_url = " << tomlQuote(cfg.profileAvatarUrl) << "\n";
        out << "\n";
    }

    for (const auto &entry : cfg.ignoreList) {
        out << "[[ignore.entry]]\n";
        out << "nick = " << tomlQuote(entry.nick) << "\n";
        QStringList flagNames;
        if (entry.flags & IgnoreType::PM)     flagNames << "\"pm\"";
        if (entry.flags & IgnoreType::Notice) flagNames << "\"notice\"";
        if (entry.flags & IgnoreType::Invite) flagNames << "\"invite\"";
        out << "flags = [" << flagNames.join(", ") << "]\n\n";
    }

    auto writeNickList = [&](const char *section, const QStringList &nicks) {
        if (nicks.isEmpty()) return;
        QStringList quoted;
        for (const QString &n : nicks)
            quoted << tomlQuote(n);
        out << "[" << section << "]\nnicks = [" << quoted.join(", ") << "]\n\n";
    };
    writeNickList("monitor", cfg.monitorList);

    for (const auto &sb : cfg.scripts) {
        out << "[[script]]\n";
        out << "command = " << tomlQuote(sb.command) << "\n";
        out << "path = " << tomlQuote(sb.path) << "\n";
        out << "enabled = " << boolStr(sb.enabled) << "\n\n";
    }

    for (const auto &s : cfg.servers) {
        // Bundle all 4 server-level passwords into one keychain item → one macOS prompt per server.
        QString savedPw, savedSasl, savedNs, savedProxyPass;
        if (migratePasswords) {
            const QString bundleKey = s.name + QLatin1String(":bundle");
            QString existing = KeychainHelper::read(bundleKey);
            if (existing.isEmpty()) {
                // First-time migration: preserve old individual items in the new bundle.
                const QString op  = KeychainHelper::read(s.name + QLatin1String(":password"));
                const QString osa = KeychainHelper::read(s.name + QLatin1String(":sasl_password"));
                const QString ons = KeychainHelper::read(s.name + QLatin1String(":nickserv_password"));
                const QString opp = KeychainHelper::read(s.name + QLatin1String(":proxy_pass"));
                existing = QStringList{op, osa, ons, opp}.join(QChar('\x1F'));
            }
            QStringList cur = existing.split(QChar('\x1F'));
            while (cur.size() < 4) cur.append(QString{});
            auto upd = [&](int i, const QString &val) -> QString {
                if (val.isEmpty())            { cur[i] = {}; return {}; }
                if (val != kKeychainSentinel) { cur[i] = val; }
                return cur[i].isEmpty() ? QString{} : kKeychainSentinel;
            };
            savedPw        = upd(0, s.password);
            savedSasl      = upd(1, s.saslPassword);
            savedNs        = upd(2, s.nickservPassword);
            savedProxyPass = upd(3, s.proxyPass);
            const bool hasAny = std::any_of(cur.cbegin(), cur.cend(),
                                             [](const QString &v){ return !v.isEmpty(); });
            if (hasAny)
                KeychainHelper::write(bundleKey, cur.join(QChar('\x1F')));
            else
                KeychainHelper::remove(bundleKey);
            KeychainHelper::remove(s.name + QLatin1String(":password"));
            KeychainHelper::remove(s.name + QLatin1String(":sasl_password"));
            KeychainHelper::remove(s.name + QLatin1String(":nickserv_password"));
            KeychainHelper::remove(s.name + QLatin1String(":proxy_pass"));
        } else {
            savedPw        = s.password;
            savedSasl      = s.saslPassword;
            savedNs        = s.nickservPassword;
            savedProxyPass = s.proxyPass;
        }

        out << "[[server]]\n";
        out << "name = " << tomlQuote(s.name) << "\n";
        out << "host = " << tomlQuote(s.host) << "\n";
        out << "port = " << s.port << "\n";
        out << "ssl = " << boolStr(s.ssl) << "\n";
        out << "nick = " << tomlQuote(s.nick) << "\n";
        out << "user = " << tomlQuote(s.user) << "\n";
        out << "realname = " << tomlQuote(s.realname) << "\n";
        if (!savedPw.isEmpty())
            out << "password = " << tomlQuote(savedPw) << "\n";
        if (!s.saslUser.isEmpty())
            out << "sasl_user = " << tomlQuote(s.saslUser) << "\n";
        if (!savedSasl.isEmpty())
            out << "sasl_password = " << tomlQuote(savedSasl) << "\n";
        if (s.saslExternal)
            out << "sasl_external = true\n";
        if (!s.clientCertFile.isEmpty())
            out << "client_cert = " << tomlQuote(s.clientCertFile) << "\n";
        if (!s.clientKeyFile.isEmpty())
            out << "client_key = " << tomlQuote(s.clientKeyFile) << "\n";
        if (!savedNs.isEmpty())
            out << "nickserv_password = " << tomlQuote(savedNs) << "\n";
        if (s.bouncerType == BouncerType::ZNC)
            out << "bouncer = \"znc\"\n";
        else if (s.bouncerType == BouncerType::Soju)
            out << "bouncer = \"soju\"\n";
        if (!s.bouncerNetwork.isEmpty())
            out << "bouncer_network = " << tomlQuote(s.bouncerNetwork) << "\n";
        if (!s.proxyHost.isEmpty()) {
            out << "proxy_host = " << tomlQuote(s.proxyHost) << "\n";
            out << "proxy_port = " << s.proxyPort << "\n";
            if (!s.proxyUser.isEmpty())
                out << "proxy_user = " << tomlQuote(s.proxyUser) << "\n";
            if (!savedProxyPass.isEmpty())
                out << "proxy_pass = " << tomlQuote(savedProxyPass) << "\n";
        }
        if (!s.pinnedFingerprint.isEmpty())
            out << "ssl_fingerprint = " << tomlQuote(s.pinnedFingerprint) << "\n";
        if (s.websocket)
            out << "websocket = true\n";
        if (s.disabled)
            out << "disabled = true\n";
        if (!s.quitMessage.isEmpty())
            out << "quit_message = " << tomlQuote(s.quitMessage) << "\n";
        if (!s.awayMessage.isEmpty())
            out << "away_message = " << tomlQuote(s.awayMessage) << "\n";
        const bool hasKeys = std::any_of(s.channels.begin(), s.channels.end(),
                                          [](const ChannelConfig &c){ return !c.password.isEmpty(); });
        if (hasKeys) {
            for (const auto &ch : s.channels) {
                out << "\n[[server.channel]]\n";
                out << "name = " << tomlQuote(ch.name) << "\n";
                const QString savedKey = migratePasswords
                    ? storePassword(ch.password, s.name + ":channel:" + ch.name, "key")
                    : ch.password;
                if (!savedKey.isEmpty())
                    out << "key = " << tomlQuote(savedKey) << "\n";
            }
        } else {
            QStringList names;
            for (const auto &ch : s.channels)
                names << ch.name;
            if (!names.isEmpty())
                out << "channels = " << tomlQuote(names.join(", ")) << "\n";
        }
        out << "\n";
    }

    if (!f.commit())
        qWarning() << "Uplink: failed to commit config to" << path;
    else
        QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
}

bool Config::needsNickSetup() const
{
    for (const auto &s : servers)
        if (s.nick == "yournick" || s.nick.isEmpty())
            return true;
    return false;
}

QString Config::defaultScriptsPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
           + "/.config/uplink/scripts";
}

struct BundledScript {
    const char *command;
    const char *resource;
    const char *filename;
};

static const BundledScript kBundledScripts[] = {
    { "music",   ":/scripts/music.sh",   "music.sh"   },
    { "weather", ":/scripts/weather.sh", "weather.sh" },
    { "uptime",  ":/scripts/uptime.sh",  "uptime.sh"  },
    { "roll",    ":/scripts/roll.sh",    "roll.sh"    },
};

void Config::installDefaultScripts(QList<ScriptBinding> &scripts)
{
    const QString dir = defaultScriptsPath();
    QDir().mkpath(dir);

    for (const auto &bs : kBundledScripts) {
        const QString dest = dir + "/" + bs.filename;

        // Always overwrite bundled scripts with the version shipped in the app
        QFile src(bs.resource);
        if (src.open(QIODevice::ReadOnly)) {
            QFile out(dest);
            if (out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                out.write(src.readAll());
                out.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner
                                   | QFileDevice::ExeOwner);
            }
        }

        // Add config entry if no matching command exists
        bool found = false;
        for (const auto &sb : std::as_const(scripts)) {
            if (sb.command == QLatin1String(bs.command)) {
                found = true;
                break;
            }
        }
        if (!found)
            scripts.append({QLatin1String(bs.command), dest, true});
    }
}
