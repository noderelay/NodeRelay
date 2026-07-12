#include "themeloader.h"

#include <QApplication>
#include <QPalette>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDebug>

#include <toml++/toml.hpp>

QString ThemeLoader::themesDir()
{
    return QDir::homePath() + "/.config/uplink/themes";
}

void ThemeLoader::ensureUserThemesDir()
{
    const QString userDir = themesDir();
    QDir().mkpath(userDir);

    // Seed from the bundled themes directory (next to binary or installed path).
    const QStringList sources = {
        QCoreApplication::applicationDirPath() + "/themes",
        QCoreApplication::applicationDirPath() + "/../Resources/themes",
        QCoreApplication::applicationDirPath() + "/../share/uplink/themes",
        QCoreApplication::applicationDirPath() + "/../themes",
    };
    for (const QString &src : sources) {
        const QDir srcDir(src);
        if (!srcDir.exists()) continue;
        for (const QString &f : srcDir.entryList({"*.toml"}, QDir::Files)) {
            const QString dest = userDir + "/" + f;
            if (!QFile::exists(dest))
                QFile::copy(srcDir.filePath(f), dest);
        }
        break; // use first source that exists
    }
}

QStringList ThemeLoader::availableThemes()
{
    QStringList names;
    const QDir dir(themesDir());
    for (const QString &f : dir.entryList({"*.toml"}, QDir::Files, QDir::Name))
        names << QFileInfo(f).baseName();
    return names;
}

// ---------------------------------------------------------------------------
// Parse
// ---------------------------------------------------------------------------

static QString str(const toml::table &tbl, std::string_view key, const char *def = "#ffffff")
{
    return QString::fromStdString(tbl[key].value_or<std::string>(def));
}

Theme ThemeLoader::load(const QString &name)
{
    const QString safeName = QFileInfo(name).fileName();
    const QString path = themesDir() + "/" + safeName + ".toml";
    Theme t;

    try {
        auto tbl = toml::parse_file(path.toStdString());

        auto get = [&](std::string_view section) -> const toml::table * {
            auto n = tbl[section];
            return n.is_table() ? n.as_table() : nullptr;
        };

        if (auto *g = get("general")) {
            t.background = str(*g, "background", "#1e1e2e");
            t.text       = str(*g, "text",       "#cdd6f4");
            t.border     = str(*g, "border",     "#313244");
            t.accent     = str(*g, "accent",     "#89b4fa");
        }
        if (auto *s = get("sidebar")) {
            t.sidebarBg      = str(*s, "background", "#181825");
            t.sidebarText    = str(*s, "text",       "#6c7086");
            t.sidebarActive  = str(*s, "active",     "#cdd6f4");
            t.sidebarUnread  = str(*s, "unread",     "#89b4fa");
            t.sidebarMention = str(*s, "mention",    "#f38ba8");
            t.sidebarServer  = str(*s, "server",     "#585b70");
        }
        if (auto *b = get("buffer")) {
            t.bufferBg   = str(*b, "background",  "#1e1e2e");
            t.timestamp  = str(*b, "timestamp",   "#6c7086");
            t.serverLine = str(*b, "server_line", "#585b70");
            t.action     = str(*b, "action",      "#cba6f7");
            t.nickSelf   = str(*b, "nick_self",   "#a6e3a1");
            const auto sepNode = (*b)["separator"];
            t.separator  = sepNode.is_string()
                ? QString::fromStdString(*sepNode.value<std::string>())
                : t.border;
        }
        if (auto *h = get("highlights")) {
            t.mentionBg   = str(*h, "mention_bg",   "#2a1a2e");
            t.mentionText = str(*h, "mention_text",  "#f38ba8");
            t.keyword     = str(*h, "keyword",       "#fab387");
        }
        if (auto *n = get("nicklist")) {
            t.nicklistBg   = str(*n, "background", "#181825");
            t.nicklistText = str(*n, "text",       "#6c7086");
            t.op           = str(*n, "op",         "#89b4fa");
            t.halfop       = str(*n, "halfop",     "#cba6f7");
            t.voice        = str(*n, "voice",      "#a6e3a1");
            t.away         = str(*n, "away",       "#45475a");
        }
        if (auto *i = get("input")) {
            t.inputBg    = str(*i, "background", "#313244");
            t.inputText  = str(*i, "text",       "#cdd6f4");
            t.placeholder= str(*i, "placeholder","#6c7086");
            t.inputNick  = str(*i, "nick_color", "#a6e3a1");
        }

        t.valid = true;

    } catch (const toml::parse_error &e) {
        qWarning() << "ThemeLoader: failed to parse" << path << ":" << e.what();
    }

    return t;
}

// ---------------------------------------------------------------------------
// QSS generation — uses named {{key}} substitution to avoid Qt arg() mangling
// ---------------------------------------------------------------------------

static QString fill(QString tpl, const QHash<QString, QString> &vars)
{
    for (auto it = vars.constBegin(); it != vars.constEnd(); ++it)
        tpl.replace("{{" + it.key() + "}}", it.value());
    return tpl;
}

QString ThemeLoader::toStyleSheet(const Theme &t)
{
    static const QString tpl = R"(
/* ── Base ── */
QMainWindow, QDialog, QWidget {
    background-color: {{bg}};
    color: {{text}};
}
QToolBar {
    background-color: {{sidebarBg}};
    border: none;
    spacing: 0px;
    margin: 0px;
    padding: 0px;
}
QToolBar::handle {
    width: 0px;
    height: 0px;
    image: none;
}
QStatusBar {
    background-color: {{sidebarBg}};
    color: {{srvText}};
    border: none;
    font-size: 7pt;
    padding: 0px 4px;
}
/* ── Dock title bars ── */
QWidget#dockTitleBar {
    background-color: {{sidebarBg}};
    max-height: 16px;
}
QToolButton#dockFloatBtn {
    background: transparent;
    color: {{placeholder}};
    border: none;
    font-size: 9pt;
    padding: 0px;
}
QToolButton#dockFloatBtn:hover { color: {{text}}; }
QWidget#nickContainer { background-color: {{nicklistBg}}; }
QToolButton#hamburger {
    background-color: {{inputBg}};
    color: {{text}};
    border: none;
    padding: 0px 6px;
}
QToolButton#hamburger:hover { color: {{accent}}; }
QToolButton#sidebarToggleBtn {
    background-color: {{inputBg}};
    color: {{text}};
    border: none;
    padding: 0px 2px;
}
QToolButton#sidebarToggleBtn:hover { color: {{accent}}; }
/* Sidebar card: the panel is the window-bg backdrop the rounded corners
   are cut against; header carries the top rounding, the tree below stays
   flush/square at the bottom. Radii are painted by QSS — never use widget
   masks for rounding (they mis-clip at fractional display scale). */
QWidget#sidebarPanel {
    background-color: {{bg}};
}
QWidget#sidebarHeader {
    background-color: {{bg}};
}

QSizeGrip {
    width: 0px;
    height: 0px;
    image: none;
}
QMenuBar {
    background-color: {{bg}};
    color: {{text}};
    padding: 2px 0px;
}
QMenuBar::item {
    padding: 4px 12px;
    border-radius: 6px;
}
QMenuBar::item:selected {
    background-color: {{accent}};
    color: {{text}};
}
QMenu {
    background-color: {{bg}};
    color: {{text}};
    border: 1px solid {{border}};
    border-radius: 12px;
    padding: 6px 0px;
}
QMenu::item {
    padding: 7px 24px 7px 16px;
    border-radius: 8px;
    margin: 1px 6px;
}
QMenu::item:selected {
    background-color: {{accent}};
}
QMenu::separator {
    height: 1px;
    background: {{border}};
    margin: 6px 10px;
}

/* ── Sidebar ── */
QTreeWidget {
    background-color: {{sidebarBg}};
    color: {{sidebarText}};
    border: none;
    outline: none;
    border-top-left-radius: 10px;
    border-top-right-radius: 10px;
}
QTreeWidget::item {
    padding: 2px 6px;
    height: 22px;
}
QTreeWidget::item:selected {
    background: transparent;
    color: {{sidebarActive}};
}
QTreeWidget::item:hover {
    background: transparent;
}
QTreeWidget::branch {
    background-color: {{sidebarBg}};
    border: none;
    image: none;
    width: 0px;
}

/* ── Chat view ── */
QTextEdit {
    background-color: {{bufferBg}};
    color: {{text}};
    border: none;
    selection-background-color: {{accent}};
}

/* ── Nick list ── */
QListWidget {
    background-color: {{nicklistBg}};
    color: {{nicklistText}};
    border: none;
    outline: none;
}
QListWidget::item {
    padding: 2px 8px;
    height: 22px;
}
QListWidget::item:selected {
    background-color: {{accent}};
    color: {{sidebarActive}};
}
QListWidget::item:hover:!selected {
    background-color: {{border}};
}

/* ── Channel pane (docked + popped-out): compose strip on the chat colour ── */
QWidget#channelPane,
QWidget#paneWindow {
    background-color: {{bufferBg}};
}

/* ── Input bar ── */
QWidget#inputBar {
    background-color: {{bufferBg}};
    border: none;
    padding: 4px 6px;
}
QWidget#inputBar QLabel {
    background-color: transparent;
    color: {{inputNick}};
    border: none;
}
QWidget#inputBar QLineEdit,
QWidget#inputBar QPlainTextEdit {
    background-color: {{inputBg}};
    color: {{inputText}};
    border: none;
    border-radius: 8px;
    padding: 6px 14px;
    selection-background-color: {{accent}};
}

/* ── Line edit (dialogs) ── */
QLineEdit {
    background-color: {{inputBg}};
    color: {{inputText}};
    border: 1px solid {{border}};
    border-radius: 10px;
    padding: 6px 12px;
    selection-background-color: {{accent}};
}
QLineEdit:focus {
    border-color: {{accent}};
}
QLineEdit::placeholder-text,
QPlainTextEdit::placeholder-text {
    color: {{placeholder}};
}

/* ── Combo box ── */
QComboBox {
    background-color: {{inputBg}};
    color: {{inputText}};
    border: 1px solid {{border}};
    border-radius: 10px;
    padding: 5px 12px;
    min-width: 80px;
}
QComboBox:focus {
    border-color: {{accent}};
}
QComboBox::drop-down {
    subcontrol-origin: border;
    subcontrol-position: center right;
    border: none;
    width: 24px;
}
QComboBox::down-arrow {
    image: url(:/icons/mi-arrow-drop-down.svg);
    width: 10px;
    height: 6px;
}
QComboBox QAbstractItemView {
    background-color: {{bg}};
    color: {{text}};
    border: 1px solid {{border}};
    selection-background-color: {{accent}};
    selection-color: {{text}};
    outline: none;
}
QComboBox QFrame {
    background-color: {{bg}};
}

/* ── Spin box ── */
QSpinBox {
    background-color: {{inputBg}};
    color: {{inputText}};
    border: 1px solid {{border}};
    border-radius: 10px;
    padding: 5px 10px;
}
QSpinBox:focus {
    border-color: {{accent}};
}
QSpinBox::up-button, QSpinBox::down-button {
    border: none;
    background: transparent;
    width: 14px;
}

/* ── Check box ── */
QCheckBox {
    spacing: 8px;
    color: {{text}};
    font-size: 10pt;
    padding: 3px 0;
}
QCheckBox::indicator {
    width: 16px;
    height: 16px;
    border: 1px solid {{border}};
    border-radius: 4px;
    background-color: {{inputBg}};
}
QCheckBox::indicator:checked {
    background-color: {{accent}};
    border-color: {{accent}};
}
QCheckBox::indicator:hover {
    border-color: {{accent}};
}

/* ── Radio button ── */
QRadioButton {
    spacing: 8px;
    color: {{text}};
    font-size: 10pt;
    padding: 3px 0;
}
QRadioButton::indicator {
    width: 16px;
    height: 16px;
    border: 1px solid {{border}};
    border-radius: 8px;
    background-color: {{inputBg}};
}
QRadioButton::indicator:checked {
    background-color: {{accent}};
    border-color: {{accent}};
}
QRadioButton::indicator:hover {
    border-color: {{accent}};
}

/* ── Tab widget ── */
QTabWidget::pane {
    border: none;
}
QTabBar::tab {
    background-color: transparent;
    color: {{placeholder}};
    padding: 8px 20px;
    border: none;
    border-bottom: 2px solid transparent;
}
QTabBar::tab:selected {
    color: {{text}};
    border-bottom: 2px solid {{accent}};
}
QTabBar::tab:hover:!selected {
    color: {{text}};
    background-color: {{border}};
    border-radius: 6px 6px 0px 0px;
}

/* ── Tooltip ── */
QToolTip {
    background-color: {{sidebarBg}};
    color: {{text}};
    border: 1px solid {{border}};
    border-radius: 8px;
    padding: 6px 12px;
}

/* ── Dock widgets ── */
QDockWidget {
    color: {{text}};
    titlebar-close-icon: none;
}
QDockWidget::title {
    background-color: {{sidebarBg}};
    padding: 0px 2px;
}

/* ── Scroll bars ── */
QScrollBar:vertical {
    background: transparent;
    width: 8px;
    border: none;
    margin: 2px;
}
QScrollBar::handle:vertical {
    background: {{border}};
    border-radius: 4px;
    min-height: 28px;
}
QScrollBar::handle:vertical:hover {
    background: {{placeholder}};
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar:horizontal { height: 0; }

/* ── Splitter ── */
QSplitter::handle {
    background: {{border}};
    width: 1px;
    height: 1px;
}

/* ── Embedded nick panel ── */
/* Mirrors the sidebar card's rounded top. Both the panel and its header
   carry the radii — the header fills the panel's top, so both must curve
   for the splitter's window-bg backdrop to show through the corners. */
QWidget#nickPanel {
    background-color: {{nicklistBg}};
    border-top-left-radius: 10px;
    border-top-right-radius: 10px;
}
QWidget#nickPanelHeader {
    background-color: {{nicklistBg}};
    border-top-left-radius: 10px;
    border-top-right-radius: 10px;
}
QWidget#nickPanel QListWidget {
    background-color: {{nicklistBg}};
    color: {{text}};
}
QWidget#nickPanel QListWidget::item:selected {
    background: transparent;
    color: {{sidebarActive}};
}
QWidget#nickPanel QListWidget::item:hover {
    background: transparent;
}
QWidget#nickPanelHeader QToolButton {
    background-color: transparent;
    color: {{text}};
}
QWidget#nickPanelHeader QLabel {
    background-color: transparent;
    color: {{text}};
}
QLineEdit#nickFilter {
    background-color: {{nicklistBg}};
    color: {{text}};
    border: none;
    border-bottom: 1px solid {{border}};
    padding: 2px 6px;
    font-size: 9pt;
}

/* ── Tool buttons ── */
QToolButton {
    background-color: {{sidebarBg}};
    color: {{sidebarText}};
    border: none;
    padding: 4px 8px;
}
QToolButton:hover {
    background-color: {{border}};
    border-radius: 8px;
}

/* ── Labels ── */
QLabel {
    color: {{text}};
    background: transparent;
}

/* ── Push buttons ── */
QPushButton {
    background: {{border}};
    color: {{text}};
    border: none;
    border-radius: 20px;
    padding: 6px 22px;
    font-weight: 500;
    outline: 0;
}
QPushButton:hover   { background: {{accent}}; border-color: {{accent}}; }
QPushButton:pressed { background: {{sidebarBg}}; border-color: {{sidebarBg}}; }

/* ── Dialog buttons ── */
QDialogButtonBox QPushButton {
    min-width: 88px;
}

/* ── Right content area — window-bg backdrop behind panel margins/corners ── */
QWidget#rightContent {
    background-color: {{bg}};
}

QLabel#channelLabel {
    color: {{accent}};
    font-weight: bold;
}
QLabel#modesLabel {
    color: {{placeholder}};
}
QLabel#userInfoLabel {
    color: {{placeholder}};
}
QWidget#topicDisplay {
    background-color: {{bufferBg}};
    border-bottom: 1px solid {{border}};
}
QLabel#topicText {
    color: {{placeholder}};
    background: transparent;
    padding: 0px;
}

/* ── Topic toggle button ── */
QToolButton#topicToggle {
    background: transparent;
    border: none;
    padding: 2px;
}
QToolButton#topicToggle:hover {
    background: {{border}};
    border-radius: 6px;
}

/* ── Typing indicator ── */
QLabel#typingLabel {
    color: {{placeholder}};
    background: transparent;
    padding: 0;
}
)";

    return fill(tpl, {
        {"bg",           t.background},
        {"text",         t.text},
        {"border",       t.border},
        {"accent",       t.accent},
        {"sidebarBg",    t.sidebarBg},
        {"sidebarText",  t.sidebarText},
        {"sidebarActive",t.sidebarActive},
        {"bufferBg",     t.bufferBg},
        {"nicklistBg",   t.nicklistBg},
        {"nicklistText", t.nicklistText},
        {"inputBg",      t.inputBg},
        {"inputText",    t.inputText},
        {"placeholder",  t.placeholder},
        {"inputNick",    t.inputNick},
        {"srvText",      t.sidebarServer},
    });
}

// ---------------------------------------------------------------------------
// Apply to QApplication
// ---------------------------------------------------------------------------

void ThemeLoader::apply(const QString &name)
{
#if defined(Q_OS_WIN)
    // On Windows, let the native style handle the default look.
    // Only apply QSS when the user explicitly picks a custom theme.
    if (name.isEmpty() || name == "default") {
        qApp->setStyleSheet({});
        return;
    }
#endif

    const Theme t = load(name);
    if (!t.valid) {
        qWarning() << "ThemeLoader: could not load theme" << name;
        return;
    }

    // Keep QPalette in sync so palette() references in stylesheets and
    // autoFillBackground on popup widgets resolve to the correct theme colors.
    QPalette pal;
    pal.setColor(QPalette::Window,          QColor(t.background));
    pal.setColor(QPalette::WindowText,      QColor(t.text));
    pal.setColor(QPalette::Base,            QColor(t.background));
    pal.setColor(QPalette::AlternateBase,   QColor(t.sidebarBg));
    pal.setColor(QPalette::Text,            QColor(t.text));
    pal.setColor(QPalette::Button,          QColor(t.inputBg));
    pal.setColor(QPalette::ButtonText,      QColor(t.inputText));
    pal.setColor(QPalette::Highlight,       QColor(t.accent));
    pal.setColor(QPalette::HighlightedText, QColor(t.text));
    pal.setColor(QPalette::PlaceholderText, QColor(t.placeholder));
    qApp->setPalette(pal);

    qApp->setStyleSheet(toStyleSheet(t));
}
