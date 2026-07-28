#pragma once

#include <QFlags>
#include <QString>
#include <QStringList>
#include <QList>

#if defined(Q_OS_WIN)
inline const char *kDefaultFontFamily = "Consolas";
#else
inline const char *kDefaultFontFamily = "IBM Plex Mono";
#endif

enum class BouncerType { None, ZNC, Soju };

enum class IgnoreType : quint8 {
    PM     = 0x01,   // private PRIVMSG and /me actions
    Notice = 0x02,   // private NOTICEs
    Invite = 0x04,   // INVITE requests
};
Q_DECLARE_FLAGS(IgnoreTypes, IgnoreType)
Q_DECLARE_OPERATORS_FOR_FLAGS(IgnoreTypes)

inline constexpr IgnoreTypes kIgnoreAll{IgnoreType::PM | IgnoreType::Notice | IgnoreType::Invite};

struct IgnoreEntry {
    QString     nick;
    IgnoreTypes flags{kIgnoreAll};
};

struct ScriptBinding {
    QString command;    // e.g. "music" (no leading slash)
    QString path;       // absolute path to executable
    bool    enabled{true};
};

struct ChannelConfig {
    QString name;
    QString password;
};

struct ServerConfig {
    QString            name;
    QString            host;
    quint16            port{6697};
    bool               ssl{true};
    QString            nick;
    QString            user;
    QString            realname;
    QString            password;         // PASS / bouncer
    QString            saslUser;
    QString            saslPassword;
    bool               saslExternal{false};
    QString            clientCertFile;
    QString            clientKeyFile;
    QString            nickservPassword;
    BouncerType        bouncerType{BouncerType::None};
    QString            bouncerNetwork;   // soju network name, or empty
    QString            proxyHost;        // SOCKS5 proxy hostname (empty = no proxy)
    quint16            proxyPort{1080};  // SOCKS5 proxy port
    QString            proxyUser;        // optional proxy username
    QString            proxyPass;        // optional proxy password
    QString            pinnedFingerprint; // SHA-256 hex of pinned TLS cert (empty = verify normally)
    bool               websocket{false};  // connect via WebSocket (ws:// or wss://)
    bool               metadata{true};    // negotiate draft/metadata (profiles, avatars)
    bool               disabled{false};  // skip on startup; keep in config
    QString            quitMessage;      // QUIT message (empty = "Uplink")
    QString            awayMessage;      // default /away message when no arg given
    QList<ChannelConfig> channels;

    bool operator==(const ServerConfig &o) const {
        return host == o.host && port == o.port && ssl == o.ssl
            && nick == o.nick && user == o.user && realname == o.realname
            && name == o.name && password == o.password
            && saslUser == o.saslUser && saslPassword == o.saslPassword
            && saslExternal == o.saslExternal
            && clientCertFile == o.clientCertFile && clientKeyFile == o.clientKeyFile
            && nickservPassword == o.nickservPassword
            && pinnedFingerprint == o.pinnedFingerprint;
    }
    bool operator!=(const ServerConfig &o) const { return !(*this == o); }
};

struct FontSizes {
    double toolbar{10};      // hamburger button
    double serverHeader{9};  // network name section label
    double sidebar{10};      // channel list
    double chat{10};         // message area
    double nickList{10};     // user list panel
#if defined(Q_OS_MAC)
    double nickDock{13};     // "Users (N)" dock title
#else
    double nickDock{9};      // "Users (N)" dock title
#endif
    double topicBar{11};     // #channel label in topic bar
    double topicText{11};    // actual topic text
    double inputNick{10};    // your nick label
    double input{10};        // message typing area
    double typing{9};        // "nick is typing..." indicator
    double emoji{16};        // emoji in chat messages
};

struct UiConfig {
    QString   theme{"default"};
    bool      themeAuto{false};      // follow the OS light/dark scheme (Qt 6.5+)
    QString   themeLight{"light"};   // applied when the OS scheme is light
    QString   themeDark{"dark"};     // applied when the OS scheme is dark
    bool      showNickPrefix{true};
    bool      showTopic{true};
    bool      showEmojiButton{false};
    bool      showSendButton{true};
    bool      coloredNicks{true};
    QString   fontFamily{kDefaultFontFamily};
    FontSizes fontSizes;
    bool      typingIndicator{true};
    bool      notifications{true};
    bool      hangingIndent{true};
    bool      logMessages{false};
    bool      linkPreviews{false};
    bool      showTimestamps{true};
    QString   highlightWords{};   // comma-separated extra highlight keywords
    bool      showUnreadCounts{true};
    bool      showAvatars{true};      // fetch and show user/channel avatar images
    QString   appIcon{"flat-black"};
    QString   nickBrackets{"<>"};
    // "auto" (default) picks the split axis from the shape of the pane area,
    // "columns" and "rows" force it either way
    QString   paneSplitAxis{"auto"};
    bool      panelCards{true};       // side panels use own theme colors + rounded-top cards
    QString   menuStyle{"menubar"};   // "menubar" (File/Edit/... bar), "hidden" (shortcuts only)
};

struct DccConfig {
    QString externalIp;   // IPv4 advertised in DCC offers (empty = socket address)
    quint16 portMin{0};   // listen range for DCC servers; 0 = ephemeral port
    quint16 portMax{0};
    bool    allowLan{false};  // permit private peer addresses for DCC transfers
};

inline const QString kKeychainSentinel = QStringLiteral("<keychain>");

struct Config {
    QList<ServerConfig> servers;
    UiConfig            ui;
    DccConfig           dcc;
    QList<IgnoreEntry>  ignoreList;
    QStringList         monitorList;   // nicks to watch with MONITOR
    QList<ScriptBinding> scripts;
    QString             profileDisplayName; // draft/metadata display-name
    QString             profileAvatarUrl;   // draft/metadata avatar URL
    QString             profileStatusText;  // draft/metadata status

    bool needsNickSetup() const;

    static Config    load(const QString &path);
    static void      save(const Config &cfg, const QString &path, bool migratePasswords = false);
    static QString   defaultPath();
    static void      ensureExists(const QString &path);
    static QString   defaultScriptsPath();
    static void      installDefaultScripts(QList<ScriptBinding> &scripts);
};
