#if defined(__linux__) && !defined(__MUSL__)
#include <malloc.h>
#endif
#include "mainwindow.h"
#include "ui/mainwindowdelegates.h"
#include "ui/uistyle.h"
#include "ui/elidedlabel.h"
#include "ui/commanddispatcher.h"
#include "ui/searchbar.h"
#include "ui/nickfilteredit.h"
#include "irc/ircclient.h"
#include "ui/dcccontroller.h"
#include "ui/trayicon.h"
#include "ui/aboutdialog.h"
#include "ui/channellistdialog.h"
#include "ui/logsearchdialog.h"
#include "ui/fontdialog.h"
#include "ui/preferencesdialog.h"
#include "ui/serverdialog.h"
#include "ui/manageserversdialog.h"
#include "ui/appicons.h"
#include "ui/themeloader.h"
#include "ui/previewcontroller.h"
#include "ui/emojipicker.h"
#include "ui/quickswitcher.h"
#include "ui/updatechecker.h"
#include "ui/sidebarcontroller.h"
#include "ui/typingcontroller.h"
#include "ui/emojidata.h"
#include "ui/chromepanel.h"
#include "ui/menuicons.h"
#include "ui/signalbars.h"
#include "ui/fadescrollbar.h"
#include "ui/channelpane.h"
#include "ui/panetree.h"
#include "ui/dropframe.h"
#include "ui/chatrenderer.h"
#include "ui/chatview.h"
#include "config/config.h"

#include <QApplication>
#include <QStyleHints>
#include <QClipboard>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QToolBar>
#include <QToolButton>
#include <QMenu>
#include <QAction>
#include <QTreeWidget>
#include <QListView>
#include <QListWidget>
#include <QListWidgetItem>
#include <QTextBrowser>
#include <QDesktopServices>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStatusBar>
#include <QSplitter>
#include <QTextCharFormat>
#include <QScrollBar>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QInputDialog>
#include <QSysInfo>
#include <QThread>
#include <QTimer>
#include <QSettings>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTextStream>
#include <QProcess>
#include <QPainter>
#include <QPixmap>
#include <QSvgRenderer>
#include <QRegularExpression>
#include <QToolTip>
#include <QCursor>
#include <QMouseEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QTextDocument>
#include <QTextFrame>
#include <QTextCursor>
#include <QTextBlock>
#include <QTextFragment>
#include <QBuffer>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QScreen>
#include <QScroller>
#include <QRandomGenerator>
#include <QShortcut>
#include <QDebug>
#include <QStyledItemDelegate>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QPointer>
#if defined(Q_OS_WIN)
#  include <windows.h>
#endif

class FixedRowDelegate : public QStyledItemDelegate {
    int m_height;
public:
    explicit FixedRowDelegate(int height, QObject *parent = nullptr)
        : QStyledItemDelegate(parent), m_height(height) {}
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override
    {
        QSize s = QStyledItemDelegate::sizeHint(option, index);
        s.setHeight(m_height);
        return s;
    }
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        const QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);
        opt.icon = QIcon();
        opt.decorationSize = QSize(0, 0);
        QStyledItemDelegate::paint(painter, opt, index);
        if (!icon.isNull()) {
            const int sz = 14;
            const QFontMetrics fm(opt.font);
            const int textEnd = opt.rect.left() + opt.fontMetrics.horizontalAdvance(opt.text)
                                + fm.horizontalAdvance(QLatin1Char(' '));
            QRect r(textEnd,
                    opt.rect.top() + (opt.rect.height() - sz) / 2,
                    sz, sz);
            icon.paint(painter, r);
        }
    }
};


// Minimum width of the topic bar left zone — wide enough to always show the
// hamburger (22) + gear (22) + right margin (4) even when the sidebar is closed.
static constexpr int kDefaultWindowW  = 900;
static constexpr int kDefaultWindowH  = 650;
// Sanity ceiling only. What actually stops the splitting is the minimum pane
// size in chooseSplitTarget — a view with no room to halve refuses to split,
// so the limit follows the window rather than a shape table.
static constexpr int kMaxExtraPanes   = 7;

// Layout-tree plumbing, defined with the rest of it further down; the window's
// constructor needs them for the quit handler.
static void captureFractions(PaneNode &node, QSplitter *s);
static QJsonObject paneNodeToJson(const PaneNode &node, const QHash<int, QWidget*> &views,
                                  QWidget *primary);
static constexpr int kMaxPaneWindows  = 4;
static constexpr int kPaneMaxLines    = 800; // pane scrollback retention (main view: 2000)

// Returns the slot index sharing a nested cross-splitter ("stack") with
// `slot`, or -1 if `slot` is a full/lone top-level column with no stack-
// mate. Mirrors the slot shapes rebuildPaneLayout() actually builds for
// n<=2 / n==3 / n==4:
//   n<=2  — every slot is a lone top-level column, no stacking at all.
//   n==3  — slot 0 is lone; slots 1 and 2 share one stack.
//   n==4  — slots (0,1) share one stack, slots (2,3) share the other.


// Mime payload marking a drag of the primary panel. Real pane keys are
// "host|channel", so this can't collide with one.
static const QString kPrimaryDragKey = QStringLiteral("__primary__");

static bool isModifierKey(int key)
{
    return key == Qt::Key_Shift || key == Qt::Key_Control
        || key == Qt::Key_Alt   || key == Qt::Key_Meta;
}


// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

MainWindow::MainWindow(SessionModel *model, const Config &cfg, QWidget *parent)
    : QMainWindow(parent)
    , m_model(model)
    , m_config(cfg)
{
    // Init UI toggles from config
    m_showNickPrefix = cfg.ui.showNickPrefix;
    m_showTopic      = cfg.ui.showTopic;
    m_showEmojiBtn   = cfg.ui.showEmojiButton;
    m_highlightRe = SessionModel::buildHighlightRe(cfg.ui.highlightWords);

    setWindowTitle("Uplink");
    const QIcon appIcon = AppIcons::appIcon(m_config.ui.appIcon);
    QApplication::setWindowIcon(appIcon);
    setWindowIcon(appIcon);
    AppIcons::publishSystemIcon(appIcon);
    resize(kDefaultWindowW, kDefaultWindowH);
    setAcceptDrops(true); // pane drags: whole window accepts (see dragEnterEvent)

    m_appliedThemeName = effectiveThemeName();
    ThemeLoader::apply(m_appliedThemeName, m_config.ui.panelCards);
    m_theme = ThemeLoader::load(m_appliedThemeName);
    setDockOptions(QMainWindow::AllowNestedDocks | QMainWindow::AllowTabbedDocks);

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    // Auto theme mode: recolor live when the OS flips between light and dark.
    connect(qApp->styleHints(), &QStyleHints::colorSchemeChanged,
            this, [this](Qt::ColorScheme){
        if (!m_config.ui.themeAuto)
            return;
        const QString name = effectiveThemeName();
        if (name != m_appliedThemeName)
            applyThemeByName(name);
    });
#endif

    setupSidebar();
    setupNickPanel();
    setupChatArea();
    setupInputBar();
    connectModel();
    applyFontSizes();
    applyPanelChrome();

    m_configWatcher.addPath(Config::defaultPath());
    connect(&m_configWatcher, &QFileSystemWatcher::fileChanged,
            this, &MainWindow::onConfigFileChanged);
    QTimer::singleShot(0, this, [this]{
        if (m_input) {
            const int lineH   = m_input->fontMetrics().lineSpacing();
            const int margins = m_input->contentsMargins().top() + m_input->contentsMargins().bottom() + 8;
            m_input->setFixedHeight(lineH + margins);
        }
    });

    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        m_tray = new TrayIcon(model, this);
        m_tray->setBaseIcon(AppIcons::appIcon(m_config.ui.appIcon));
        m_tray->setNotificationsEnabled(m_config.ui.notifications);
    }

    auto *ctrlW = new QShortcut(QKeySequence("Ctrl+W"), this);
    connect(ctrlW, &QShortcut::activated, this, [this]{
        if (m_tray && m_tray->isVisible()) hide();
    });

    m_quickSwitcher = new QuickSwitcher(model, this);
    connect(m_quickSwitcher, &QuickSwitcher::channelSelected, this, [this](const ServerId &host, const BufferId &channel){
        auto *item = m_sidebarCtl->channelItem(host, channel);
        if (item) {
            m_sidebar->setCurrentItem(item);
            onSidebarSelectionChanged();
        }
    });

    // Ctrl+F / Ctrl+Shift+F / Ctrl+K / Ctrl+Shift+K / Ctrl+Q live on window-
    // level QActions (shared with the menu bar) — see setupMenuActions().
    setupMenuActions();
    applyMenuStyle();

    statusBar()->hide();

    QSettings settings("uplink", "uplink");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    setWindowState(windowState() & ~Qt::WindowMaximized);
    // If the restored normal-mode size fills ≥80% of the screen the right edge
    // ends up at or near the screen boundary — WM can't offer a grab handle.
    // Reset to default so the window always opens with visible resize margins.
    if (auto *scr = QGuiApplication::primaryScreen()) {
        if (width() > scr->availableGeometry().width() * 8 / 10)
            resize(kDefaultWindowW, kDefaultWindowH);
    }
    // Pre-show width cap: limits WM_NORMAL_HINTS.max_width so the WM cannot map
    // the window wider than our target.  Released in the post-show timer.
    // Skipped on Windows — Windows uses max size to gate the maximize button,
    // so setting it this tight would grey out the title-bar maximize control.
#if !defined(Q_OS_WIN)
    setMaximumWidth(width() + 20);
#endif

    // User list width persists as a plain int, same as sidebarWidth below —
    // never via QSplitter::saveState(), whose blob also restores flags like
    // childrenCollapsible and resurrected drag-collapse from old configs.
    int nickW = settings.value("nickWidth", 0).toInt();
    if (nickW <= 0 && settings.contains("nickSplitter")) {
        // Legacy config: pull the width out of the old state blob, then
        // re-assert non-collapsible (restoreState smuggles it back on).
        // The blob itself is dropped on next save.
        m_chatSplitter->restoreState(settings.value("nickSplitter").toByteArray());
        m_chatSplitter->setChildrenCollapsible(false);
        nickW = m_chatSplitter->sizes().value(1);
        if (nickW <= 0)  // blob saved while drag-collapsed to 0 — rescue
            nickW = 180;
    }
    if (nickW > 0) {
        // Also heals legacy drag-collapsed widths (0 or sliver) to the floor.
        nickW = qMax(m_nickPanel->minimumWidth(), nickW);
        if (auto sizes = m_chatSplitter->sizes(); sizes.size() == 2) {
            const int total = sizes[0] + sizes[1];
            m_chatSplitter->setSizes({qMax(1, total - nickW), nickW});
        }
        m_nickExpandedWidth = nickW;
    }
    if (settings.contains("sidebarWidth"))
        m_sidebarExpandedWidth = settings.value("sidebarWidth").toInt();

    const QStringList savedPanes       = settings.value("panes").toStringList();

    QTimer::singleShot(0, this, [this]{
        // Release the pre-show width cap.
        setMaximumWidth(QWIDGETSIZE_MAX);

        auto *scr = screen() ? screen() : QGuiApplication::primaryScreen();
        const bool qtMaximized = windowState() & Qt::WindowMaximized;
        const bool tooWide     = scr && width() > scr->availableGeometry().width() * 8 / 10;

        if (qtMaximized) {
            setWindowState(Qt::WindowNoState);
        } else if (tooWide) {
            // KWin session-restored a wider-than-screen geometry that Qt doesn't
            // surface as Maximized. Cycle state so KWin clears the saved size.
            setWindowState(Qt::WindowMaximized);
            setWindowState(Qt::WindowNoState);
        }

        if (qtMaximized || tooWide) {
            QTimer::singleShot(100, this, &MainWindow::correctStartupGeometry);
            return;
        }

        // Normal path: clamp position/size immediately.
        if (scr) {
            const QRect avail = scr->availableGeometry();
            QRect w = geometry();
            if (w.height() > avail.height() - 60) w.setHeight(avail.height() - 80);
            if (w.right()  > avail.right())  w.moveRight(avail.right());
            if (w.left()   < avail.left())   w.moveLeft(avail.left());
            if (w.bottom() > avail.bottom()) w.moveBottom(avail.bottom());
            if (w.top()    < avail.top())    w.moveTop(avail.top());
            setGeometry(w);
        }
        const int total = m_mainSplitter->width();
        if (total > 0)
            m_mainSplitter->setSizes({m_sidebarExpandedWidth, total - m_sidebarExpandedWidth});
    });

    if (!savedPanes.isEmpty()) {
        const QString savedLayout  = settings.value("paneLayout").toString();
        const bool primaryWasShut  = settings.value("primaryHidden").toBool();
        QTimer::singleShot(0, this, [this, savedPanes, savedLayout, primaryWasShut]{
            for (const QString &k : savedPanes) {
                const qsizetype sep = k.indexOf('|');
                if (sep < 0) continue;
                openChannelPane(ServerId{k.left(sep)}, BufferId{k.mid(sep + 1)});
            }
            // The opens above split whatever had room, which gets the panes on
            // screen; the saved layout then puts them where they were.
            restorePaneLayout(savedLayout);
            // The ✕ on the main view is a real close — it has to survive a
            // restart, or the panes come back with the closed view alongside
            // them. Only honour it while a pane is left to show, so a window
            // with nothing in it is never what starts.
            if (primaryWasShut && !m_orderedPanes.isEmpty())
                m_primaryPanel->hide();
        });
    }

    const QStringList savedPaneWindows = settings.value("paneWindows").toStringList();
    if (!savedPaneWindows.isEmpty()) {
        QTimer::singleShot(0, this, [this, savedPaneWindows]{
            for (const QString &k : savedPaneWindows) {
                const qsizetype sep = k.indexOf('|');
                if (sep < 0) continue;
                popOutChannel(ServerId{k.left(sep)}, BufferId{k.mid(sep + 1)});
            }
        });
    }

    connect(m_mainSplitter, &QSplitter::splitterMoved, this, [this](int, int){
        const int w = m_mainSplitter->sizes().value(0);
        if (m_sidebarExpanded && w > 0)
            m_sidebarExpandedWidth = w;
    });

    connect(m_chatSplitter, &QSplitter::splitterMoved, this, [this](int, int){
        const int w = m_chatSplitter->sizes().value(1);
        if (m_nickExpanded && w > 0)
            m_nickExpandedWidth = w;
    });

    connect(qApp, &QApplication::aboutToQuit, this, [this]{
        QSettings s("uplink", "uplink");
        s.setValue("geometry", saveGeometry());
        s.setValue("windowState", saveState());
        s.setValue("nickWidth", m_nickExpandedWidth);
        s.remove("nickSplitter"); // legacy state blob, replaced by nickWidth
        s.setValue("sidebarWidth", m_sidebarExpandedWidth);
        // Persist original-case host|channel (not the lowercased pane key):
        // the restore path rebuilds BufferIds from these strings, and a
        // lowercased channel breaks case-sensitive lookups (typing state)
        // and window titles.
        QStringList paneList;
        for (auto *p : std::as_const(m_orderedPanes))
            paneList << p->host().str() + "|" + p->channel().str();
        s.setValue("panes", paneList);
        s.setValue("primaryHidden", m_primaryPanel->isHidden());
        s.remove("primarySlot"); // replaced by the layout tree below
        // Sizes as last dragged, then the arrangement itself.
        captureFractions(m_paneTree, m_panesSplitter);
        s.setValue("paneLayout",
                   QString::fromUtf8(QJsonDocument(
                       paneNodeToJson(m_paneTree, m_viewById, m_primaryPanel))
                       .toJson(QJsonDocument::Compact)));
        QStringList winList;
        for (auto it = m_paneWindows.constBegin(); it != m_paneWindows.constEnd(); ++it)
            if (auto *p = m_panes.value(it.key()))
                winList << p->host().str() + "|" + p->channel().str();
        s.setValue("paneWindows", winList);
        for (auto it = m_paneWindows.constBegin(); it != m_paneWindows.constEnd(); ++it)
            s.setValue("paneWinGeom/" + it.key(), it.value()->saveGeometry());
    });

#if defined(__linux__) && !defined(__MUSL__)
    auto *trimTimer = new QTimer(this);
    connect(trimTimer, &QTimer::timeout, this, []{ malloc_trim(0); });
    trimTimer->start(60000);
#endif

    // Track which view the user is typing in, so a sidebar click knows whether
    // to load into a docked pane or the primary. Widgets that belong to
    // neither (the sidebar itself, dialogs) leave the last choice standing —
    // clicking a channel row must not count as leaving the pane.
    connect(qApp, &QApplication::focusChanged, this, [this](QWidget *, QWidget *now){
        for (QWidget *w = now; w; w = w->parentWidget()) {
            if (w == m_primaryPanel) { m_focusedPane = nullptr; return; }
            if (auto *p = qobject_cast<ChannelPane *>(w)) {
                if (m_orderedPanes.contains(p)) m_focusedPane = p; // docked only
                return;
            }
        }
    });

    m_dispatcher = new CommandDispatcher(m_model, &m_config, this, this);
    connect(m_dispatcher, &CommandDispatcher::switchChannel,  this, &MainWindow::showBuffer);
    // Focus follows the same routing showBuffer used — /query answered by a
    // pane must not yank focus back to the (possibly hidden) main input.
    connect(m_dispatcher, &CommandDispatcher::focusInput,     this, [this]{
        if (ChannelPane *p = paneRouteTarget()) p->input()->setFocus();
        else if (m_input)                       m_input->setFocus();
    });
    // Straight to clearBuffer — the dispatcher already knows which buffer
    // /clear was typed in. Deriving it from focus here cleared the main
    // view when the command came from a popped-out pane window.
    connect(m_dispatcher, &CommandDispatcher::clearChat,
            this, &MainWindow::clearBuffer);
    connect(m_dispatcher, &CommandDispatcher::openChannelList,this, &MainWindow::openChannelList);
    connect(m_dispatcher, &CommandDispatcher::replyBarCleared, this, &MainWindow::clearReplyBar);
}

MainWindow::~MainWindow()
{
    // Floating pane windows are parentless top-levels; delete them explicitly.
    for (auto *win : std::as_const(m_paneWindows)) {
        win->removeEventFilter(this);
        delete win;
    }
}

void MainWindow::correctStartupGeometry()
{
    QScreen *scr = screen() ? screen() : QGuiApplication::primaryScreen();
    if (scr) {
        const QRect avail = scr->availableGeometry();
        QRect w = geometry();
        if (w.width() > avail.width() * 8 / 10) w.setWidth(kDefaultWindowW);
        if (w.height() > avail.height() - 60)   w.setHeight(avail.height() - 80);
        if (w.right()  > avail.right())  w.moveRight(avail.right());
        if (w.left()   < avail.left())   w.moveLeft(avail.left());
        if (w.bottom() > avail.bottom()) w.moveBottom(avail.bottom());
        if (w.top()    < avail.top())    w.moveTop(avail.top());
        setMinimumSize(1, 1);
        setGeometry(w);
    }
    const int total = m_mainSplitter->width();
    if (total > 0)
        m_mainSplitter->setSizes({m_sidebarExpandedWidth, total - m_sidebarExpandedWidth});
}


// Panel chrome (the user-list card and its header row) paints its own fill
// via ChromePanel — stylesheet backgrounds (app-wide AND local per-widget
// sheets) are silently dropped for plain QWidgets in some Wayland/KDE
// sessions, but a QPainter paintEvent always renders.
void MainWindow::applyPanelChrome()
{
    if (!m_theme.valid) return;
    // With panel cards off, the nick panel flattens onto the buffer color
    // with square corners and no floating gaps.
    const bool cards = m_config.ui.panelCards;
    const QColor fill(cards ? m_theme.nicklistBg : m_theme.bufferBg);
    // Cards float with a uniform kPanelGap frame, matching the input strip's
    // bottom margin. Each side card carries its OWN gaps (exposing the panel's
    // backdrop) rather than relying on rightContent's outer margins, which
    // don't paint reliably: the sidebar's window-facing edge is its left, the
    // user list's is its right, the inner edges are the splitter handles, and
    // top/bottom are the panels' own insets. rightContent drops its margins in
    // cards mode so the gaps aren't doubled and the chat column stays flush.
    const int gap = cards ? kPanelGap : 0;
    auto *nickPanel = static_cast<ChromePanel *>(m_nickPanel);
    nickPanel->setFill(fill, cards, /*roundedBottom=*/cards);
    nickPanel->setTopInset(gap);
    nickPanel->setBottomInset(gap);
    nickPanel->setRightInset(gap);
    if (m_nickPanel->layout())
        m_nickPanel->layout()->setContentsMargins(0, gap, gap, gap);
    static_cast<ChromePanel *>(m_nickPanelHeader)->setFill(fill, cards);
    if (m_sidebarPanel && m_sidebarPanel->layout())
        m_sidebarPanel->layout()->setContentsMargins(gap, gap, 0, gap);
    if (m_rightContent && m_rightContent->layout())
        m_rightContent->layout()->setContentsMargins(cards ? 0 : 8, cards ? 0 : 8,
                                                      cards ? 0 : 8, 0);
    if (m_mainSplitter) m_mainSplitter->setHandleWidth(gap);
    if (m_chatSplitter) m_chatSplitter->setHandleWidth(gap);
    for (auto *pane : std::as_const(m_panes))
        pane->setNickChrome(fill.name(), cards);
}

void MainWindow::applyFontSizes()
{
    const QString fam = UiStyle::effectiveFontFamily(m_config.ui.fontFamily);
    const FontSizes &fs = m_config.ui.fontSizes;

    auto makeFont = [&](double pt) {
        QFont f;
        f.setFamilies({fam,
                       QStringLiteral("Noto Color Emoji"),
                       QStringLiteral("Segoe UI Emoji"),
                       QStringLiteral("Apple Color Emoji")});
        f.setPointSizeF(pt);
        return f;
    };

    if (m_appLabel) {
        QFont f = makeFont(fs.toolbar);
        f.setBold(true);
        m_appLabel->setFont(f);
    }
    if (m_sidebar) {
        m_sidebar->setFont(makeFont(fs.sidebar));
        // Update server header items with their dedicated font size
        for (int i = 0; i < m_sidebar->topLevelItemCount(); ++i) {
            auto *srv = m_sidebar->topLevelItem(i);
            QFont f = makeFont(fs.serverHeader);
            f.setBold(true);
            srv->setFont(0, f);
        }
    }
    if (m_chatView)       m_chatView->setChatFont(makeFont(fs.chat));
    if (m_nickList)       m_nickList->setFont(makeFont(fs.nickList));
    if (m_nickPanel)      m_nickPanel->setFont(makeFont(fs.nickDock));
    if (m_nickCountLabel) m_nickCountLabel->setFont(makeFont(fs.nickDock));
    if (m_searchBtn)
        m_searchBtn->setIcon(MenuIcons::fromSvg(
            QStringLiteral(":/icons/mi-search.svg"),
            QColor(m_theme.valid ? m_theme.text : "#e3e3e3"), 20));
    if (m_popOutBtn)
        m_popOutBtn->setIcon(MenuIcons::pipEnter(
            QColor(m_theme.valid ? m_theme.text : "#e3e3e3")));
    if (m_nickToggleBtn)
        m_nickToggleBtn->setIcon(MenuIcons::fromSvg(
            QStringLiteral(":/icons/mi-right-panel-close.svg"),
            QColor(m_theme.valid ? m_theme.text : "#e3e3e3"), 20));
    if (m_nickRevealBtn)
        m_nickRevealBtn->setIcon(MenuIcons::fromSvg(
            QStringLiteral(":/icons/mi-left-panel-close.svg"),
            QColor(m_theme.valid ? m_theme.text : "#e3e3e3"), 20));
    if (m_sidebarCloseBtn)
        m_sidebarCloseBtn->setIcon(MenuIcons::fromSvg(
            QStringLiteral(":/icons/mi-left-panel-close.svg"),
            QColor(m_theme.valid ? m_theme.text : "#e3e3e3"), 20));
    if (m_sidebarRevealBtn)
        m_sidebarRevealBtn->setIcon(MenuIcons::fromSvg(
            QStringLiteral(":/icons/mi-right-panel-open.svg"),
            QColor(m_theme.valid ? m_theme.text : "#e3e3e3"), 20));
    if (m_nickGroupsIconLabel)
        m_nickGroupsIconLabel->setPixmap(
            MenuIcons::groups(QColor(m_theme.valid ? m_theme.text : "#e3e3e3"), 20));
    if (m_topicLabel) {
        // Bold set on the font, not via QSS: ElidedLabel measures with
        // fontMetrics(), and a stylesheet-only weight paints wider than
        // the metrics say, clipping the tail of the channel name.
        QFont cf = makeFont(fs.topicBar);
        cf.setBold(true);
        m_topicLabel->setFont(cf);
    }
    if (m_topicText) {
        const QFont tf = makeFont(fs.topicText);
        m_topicText->setFont(tf);
        m_topicText->setStyleSheet(QString("font-size: %1pt;").arg(tf.pointSizeF()));
        // Re-wrap content with the new inline font size (rich text ignores setFont)
        auto *ch = m_model->channel(m_model->activeHost(), m_model->activeChannel());
        if (ch && !ch->topic.isEmpty()) {
            const QString html = ChatRenderer::linkifyTopic(ch->topic);
            m_topicText->setText(QString("<span style='font-size:%1pt;'>%2</span>")
                                 .arg(tf.pointSizeF()).arg(html));
        }
    }
    if (m_userInfoLabel) m_userInfoLabel->setFont(makeFont(fs.topicBar));
    if (m_topicSetByLabel) {
        m_topicSetByLabel->setFont(makeFont(fs.topicBar));
        m_topicSetByLabel->setStyleSheet(
            QString("QLabel { color: %1; }").arg(m_theme.valid ? m_theme.placeholder : "#888888"));
    }
    if (m_nickPrefix)   m_nickPrefix->setFont(makeFont(fs.inputNick));
    if (m_input)        m_input->setFont(makeFont(fs.input));
    for (auto *p : std::as_const(m_panes)) {
        p->setChatFont(makeFont(fs.chat));
        p->setNickListFont(makeFont(fs.nickList));
        p->setNickPanelFont(makeFont(fs.nickDock));
        p->setTopicFont(makeFont(fs.topicText));
        p->setInputFont(makeFont(fs.inputNick), makeFont(fs.input));
        p->setTypingFont(makeFont(fs.typing));
    }
    if (m_typingLabel) {
        QFont f = makeFont(fs.typing);
        f.setItalic(true);
        m_typingLabel->setFont(f);
    }
}


// ---------------------------------------------------------------------------
// Event filter — Tab completion + input history
// ---------------------------------------------------------------------------

bool MainWindow::zoomFont(QObject *target, double delta, const QPoint &pos)
{
    double *field = fontFieldForWidget(target, pos);
    if (!field) return false;
    *field = qBound(6.0, *field + delta, 32.0);
    applyFontSizes();
    saveConfig();
    return true;
}

// /clear acts on the buffer the command was typed in, which is not the
// active one when it came from a pane. The menu entry has no buffer of its
// own, so it follows the view the user is working in.
void MainWindow::clearBuffer(const ServerId &host, const BufferId &channel)
{
    if (auto *ch = m_model->channel(host, channel))
        ch->messages.clear();

    // Every view showing that buffer, not just the one that asked: the main
    // view and a pane can hold the same channel at once.
    if (m_chatView && host == m_model->activeHost()
        && channel.str().compare(m_model->activeChannel().str(), Qt::CaseInsensitive) == 0)
        m_chatView->clear();
    if (auto *pane = m_panes.value(paneKey(host, channel)))
        pane->chatView()->clear();
}

void MainWindow::clearActiveBuffer()
{
    if (auto *pane = m_focusedPane; pane && m_panes.value(pane->key()) == pane) {
        clearBuffer(pane->host(), pane->channel());
        return;
    }
    clearBuffer(m_model->activeHost(), m_model->activeChannel());
}

double *MainWindow::fontFieldForWidget(QObject *obj, const QPoint &pos)
{
    auto *w = qobject_cast<QWidget *>(obj);
    if (!w) return nullptr;

    auto isOrChild = [&](QWidget *parent) {
        for (QWidget *p = w; p; p = p->parentWidget())
            if (p == parent) return true;
        return false;
    };

    // Check pane widgets first
    for (auto *pane : std::as_const(m_panes)) {
        if (isOrChild(pane->chatView()))  return &m_config.ui.fontSizes.chat;
        if (isOrChild(pane->nickList()))   return &m_config.ui.fontSizes.nickList;
        if (isOrChild(pane->input()))      return &m_config.ui.fontSizes.input;
    }

    if (m_topicText    && isOrChild(m_topicText))    return &m_config.ui.fontSizes.topicText;
    if (m_topicDisplay && isOrChild(m_topicDisplay)) return &m_config.ui.fontSizes.topicText;
    if (m_primaryHeader && isOrChild(m_primaryHeader)) return &m_config.ui.fontSizes.topicBar;
    if (m_nickList     && isOrChild(m_nickList))      return &m_config.ui.fontSizes.nickList;
    if (m_nickPanel    && isOrChild(m_nickPanel))     return &m_config.ui.fontSizes.nickDock;
    if (m_chatView     && isOrChild(m_chatView))     return &m_config.ui.fontSizes.chat;
    if (m_sidebar      && isOrChild(m_sidebar)) {
        // Server vs channel: check what item is under the mouse
        if (!pos.isNull()) {
            auto *item = m_sidebar->itemAt(m_sidebar->viewport()->mapFrom(w, pos));
            if (item && !item->parent())
                return &m_config.ui.fontSizes.serverHeader;
        }
        return &m_config.ui.fontSizes.sidebar;
    }
    if (m_nickPrefix   && isOrChild(m_nickPrefix))   return &m_config.ui.fontSizes.inputNick;
    if (m_input        && isOrChild(m_input))        return &m_config.ui.fontSizes.input;

    return nullptr;
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (!m_chatView)
        return QMainWindow::eventFilter(obj, event);

    // Fractional-scale seam guard: at 1.45x-style scale factors the QSS
    // rounded fill and the text fills can round to different device rows,
    // leaving an unpainted hairline that shows the input bar through. QSS
    // and palette fills proved unreliable here (polish resets them), so the
    // background is painted in code, every frame — the ChromePanel doctrine.
    // The filter runs before the widget's own paintEvent, so text and the
    // QSS fill land on top of ours.
    if (m_input && m_theme.valid && event->type() == QEvent::Paint) {
        if (obj == m_input->viewport()) {
            QPainter p(m_input->viewport());
            p.fillRect(m_input->viewport()->rect(), QColor(m_theme.inputBg));
        } else if (obj == m_input) {
            QPainter p(m_input);
            p.setRenderHint(QPainter::Antialiasing);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(m_theme.inputBg));
            // Radius matches the theme template's input rule (border-radius: 8px)
            p.drawRoundedRect(m_input->rect(), 8, 8);
        }
        return false;
    }

    // A popped-out pane window was closed via its OS title-bar button.
    if (event->type() == QEvent::Close && !m_paneWindows.isEmpty()) {
        for (auto it = m_paneWindows.constBegin(); it != m_paneWindows.constEnd(); ++it) {
            if (it.value() == obj) {
                if (auto *pane = m_panes.value(it.key()))
                    closeChannelPane(pane->host(), pane->channel());
                return false; // let the close proceed
            }
        }
    }

    // Primary pane header drag — same gesture ChannelPane implements for its
    // own header (see the long comment there for why the tracking filter is
    // app-wide and installed just-in-time).
    if (m_primaryDragPending || m_primaryDragging) {
        if (event->type() == QEvent::MouseButtonRelease && m_primaryDragPending) {
            m_primaryDragPending = false;
            qApp->removeEventFilter(this);
        } else if (event->type() == QEvent::MouseMove && m_primaryDragPending) {
            auto *me = static_cast<QMouseEvent *>(event);
            const QPoint gp = me->globalPosition().toPoint();
            if (!(me->buttons() & Qt::LeftButton)) {
                m_primaryDragPending = false;
                qApp->removeEventFilter(this);
            } else if ((gp - m_primaryDragStart).manhattanLength()
                       >= QApplication::startDragDistance()) {
                m_primaryDragPending = false;
                m_primaryDragging    = true;
                ChannelPane::execPaneDrag(m_primaryPanel, kPrimaryDragKey,
                                          m_primaryDragStart); // blocks
                m_primaryDragging = false;
                qApp->removeEventFilter(this);
            }
        }
        // fall through — never swallow events meant for other widgets
    }
    if (event->type() == QEvent::MouseButtonPress && m_primaryHeader
        && !m_primaryDragPending && !m_primaryDragging && !m_orderedPanes.isEmpty()) {
        bool inHeader = (obj == m_primaryHeader);
        if (!inHeader)
            for (auto *w : m_primaryHeader->findChildren<QWidget*>(Qt::FindDirectChildrenOnly))
                if (obj == w) { inHeader = true; break; }
        auto *me = static_cast<QMouseEvent *>(event);
        if (inHeader && me->button() == Qt::LeftButton) {
            m_primaryDragPending = true;
            m_primaryDragStart   = me->globalPosition().toPoint();
            qApp->installEventFilter(this);
            // Same as ChannelPane: consume presses on the passive header
            // surfaces so they can't bubble to the QMainWindow and arm
            // Breeze's whole-window empty-area drag against this gesture.
            if (!qobject_cast<QAbstractButton *>(obj))
                return true;
        }
    }

    // Pane header dropped onto the primary view: pane <-> primary swap.
    if (obj == m_primaryPanel) {
        if (event->type() == QEvent::DragEnter || event->type() == QEvent::DragMove) {
            auto *de = static_cast<QDragMoveEvent *>(event);
            const QByteArray fmt = ChannelPane::mimeType().toUtf8();
            if (de->mimeData()->hasFormat(fmt)) {
                // Accept even when the primary is dragging itself — rejecting
                // flips the drag cursor to the forbidden shape (see
                // ChannelPane::dragEnterEvent). Only a real target gets the
                // drop frame; the drop handler below no-ops for self.
                de->acceptProposedAction();
                if (QString::fromUtf8(de->mimeData()->data(fmt)) != kPrimaryDragKey) {
                    if (!m_primaryDropFrame)
                        m_primaryDropFrame = new DropFrame(m_primaryPanel);
                    // Same edge bands a pane uses, so the primary view reads
                    // as a drop target in exactly the same way — including
                    // staying dark for a drop that wouldn't move anything.
                    const auto zone = ChannelPane::zoneFor(m_primaryPanel->size(),
                                                           de->position().toPoint());
                    auto *src = m_panes.value(QString::fromUtf8(de->mimeData()->data(fmt)));
                    if (zone != ChannelPane::DropZone::Center &&
                        !paneDropWouldChange(src, nullptr, zone))
                        m_primaryDropFrame->hide();
                    else
                        m_primaryDropFrame->activate(
                            ChannelPane::zoneRect(m_primaryPanel->size(), zone));
                }
            } else {
                de->ignore();
            }
            return true;
        }
        if (event->type() == QEvent::DragLeave) {
            if (m_primaryDropFrame) m_primaryDropFrame->hide();
            return true;
        }
        if (event->type() == QEvent::Drop) {
            auto *de = static_cast<QDropEvent *>(event);
            const QByteArray fmt = ChannelPane::mimeType().toUtf8();
            const QString sourceKey = QString::fromUtf8(de->mimeData()->data(fmt));
            de->acceptProposedAction();
            if (m_primaryDropFrame) m_primaryDropFrame->hide();

            // Edge drop: place the dragged pane on that side of the primary
            // view and take the axis it implies, instead of the swap below.
            const auto zone = ChannelPane::zoneFor(m_primaryPanel->size(),
                                                   de->position().toPoint());
            if (zone != ChannelPane::DropZone::Center) {
                if (auto *src = m_panes.value(sourceKey);
                    src && paneDropWouldChange(src, nullptr, zone))
                    placePaneBeside(src, nullptr, zone);
                return true;
            }

            // pane dropped on the primary's middle: the two trade places.
            // With the layout a tree, that is simply a leaf swap — no slot
            // arithmetic and no promoting whatever the pane was stacked with.
            if (ChannelPane *source = m_panes.value(sourceKey);
                source && swapLeaves(m_paneTree, viewId(source), viewId(m_primaryPanel)))
                rebuildPaneLayout();
            return true;
        }
    }

    // Ctrl+wheel or Ctrl+Plus/Minus: zoom the focused region's font
    if (event->type() == QEvent::Wheel) {
        auto *we = static_cast<QWheelEvent *>(event);
        if (we->modifiers() & Qt::ControlModifier) {
            if (zoomFont(obj, we->angleDelta().y() > 0 ? 0.5 : -0.5,
                         we->position().toPoint()))
                return true;
        }
    }
    if (event->type() == QEvent::KeyPress) {
        auto *ke = static_cast<QKeyEvent *>(event);
        if (ke->modifiers() & Qt::ControlModifier) {
            double delta = 0;
            if (ke->key() == Qt::Key_Plus || ke->key() == Qt::Key_Equal) delta = 0.5;
            else if (ke->key() == Qt::Key_Minus) delta = -0.5;
            if (delta != 0 && zoomFont(obj, delta))
                return true;
        }
        // Alt-navigation only applies to widgets living in the main window —
        // a floating pane window shouldn't drive the main view's selection.
        auto *kw = qobject_cast<QWidget *>(obj);
        if ((ke->modifiers() & Qt::AltModifier) && kw && kw->window() == this) {
            if (ke->key() == Qt::Key_Up)   { navigateChannel(-1); return true; }
            if (ke->key() == Qt::Key_Down) { navigateChannel(+1); return true; }
            if (ke->key() == Qt::Key_Left) { navigatePane(-1);    return true; }
            if (ke->key() == Qt::Key_Right){ navigatePane(+1);    return true; }
        }
    }



    if (obj == m_chatSection && event->type() == QEvent::Resize &&
        m_sidebarRevealBtn && m_sidebarRevealBtn->isVisible()) {
        auto *re = static_cast<QResizeEvent *>(event);
        m_sidebarRevealBtn->move(4, re->size().height() - m_sidebarRevealBtn->height() - 4);
    }

    if (m_scrollBottomBtn && m_scrollBottomBtn->isVisible() &&
        event->type() == QEvent::Resize && obj == m_chatView->viewport()) {
        auto *re = static_cast<QResizeEvent *>(event);
        m_scrollBottomBtn->move(
            re->size().width() - m_scrollBottomBtn->width() - 12,
            re->size().height() - m_scrollBottomBtn->height() - 12);
    }

    if (obj == m_sidebarPanel && event->type() == QEvent::Resize && m_sidebarCloseBtn) {
        m_sidebarCloseBtn->move(4, m_sidebarPanel->height() - m_sidebarCloseBtn->height() - 4);
        m_sidebarCloseBtn->raise();
    }

    // Check if obj is a pane input bar
    if (event->type() == QEvent::KeyPress) {
        for (auto *pane : std::as_const(m_panes)) {
            if (obj == pane->input()) {
                auto *ke = static_cast<QKeyEvent *>(event);
                // Emoji autocomplete owns the navigation keys while it's up
                if (handleEmojiCompleterKey(obj, ke)) return true;
                // Only a reply pending in THIS pane's buffer — Esc here must
                // not cancel one being composed in another view.
                if (ke->key() == Qt::Key_Escape
                    && !replyMsgidFor(pane->host(), pane->channel()).isEmpty()) {
                    clearReplyBar();
                    return true;
                }
                if (ke->key() == Qt::Key_Tab || ke->key() == Qt::Key_Backtab) {
                    handleTabComplete(pane->input(), pane->host(), pane->channel(),
                                      ke->key() == Qt::Key_Backtab);
                    return true;
                }
                // Bold/italic/underline/strike, same as the main input
                if (applyFormatShortcut(pane->input(), ke)) return true;
                if (ke->modifiers() == Qt::ControlModifier && ke->key() == Qt::Key_O) {
                    pane->input()->setCurrentCharFormat(QTextCharFormat{});
                    updateFormatIndicatorFor(pane->input());
                    return true;
                }
                if (ke->key() == Qt::Key_K
                    && ke->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier)) {
                    showColorPicker();
                    return true;
                }
                // Input history, same rule as the main view: only step once
                // the cursor is on the first (or last) line of what's typed
                QPlainTextEdit *inp = pane->input();
                if (ke->key() == Qt::Key_Up
                    && inp->textCursor().blockNumber() == 0) {
                    handleHistoryUp(inp);
                    return true;
                }
                if (ke->key() == Qt::Key_Down
                    && inp->textCursor().blockNumber() == inp->document()->blockCount() - 1) {
                    handleHistoryDown(inp);
                    return true;
                }
                // Non-Tab resets the completion cycle — but not a bare
                // modifier press, or Shift+Tab could never reverse mid-cycle
                if (!isModifierKey(ke->key())) {
                    m_tabActive = false;
                    m_tabCandidates.clear();
                }
                break;
            }
        }
    }

if (obj == m_input && event->type() == QEvent::Resize) {
        repositionSendBtn();
        return QMainWindow::eventFilter(obj, event);
    }

    if (obj != m_input || event->type() != QEvent::KeyPress)
        return QMainWindow::eventFilter(obj, event);

    auto *ke = static_cast<QKeyEvent *>(event);

    // Emoji autocomplete navigation takes priority when popup is visible
    if (handleEmojiCompleterKey(obj, ke)) return true;

    if (ke->key() == Qt::Key_F && (ke->modifiers() & Qt::ControlModifier)) {
        m_searchBar->open();
        return true;
    }

    if (ke->key() == Qt::Key_Escape
        && !replyMsgidFor(m_model->activeHost(), m_model->activeChannel()).isEmpty()) {
        clearReplyBar();
        return true;
    }

    if (ke->key() == Qt::Key_Tab || ke->key() == Qt::Key_Backtab) {
        handleTabComplete(m_input, m_model->activeHost(), m_model->activeChannel(),
                          ke->key() == Qt::Key_Backtab);
        return true;
    }

    // Any non-Tab key resets nick completion cycle — except a bare modifier
    // press, or Shift+Tab could never reverse mid-cycle
    if (!isModifierKey(ke->key())) {
        m_tabActive = false;
        m_tabCandidates.clear();
    }

    if (ke->key() == Qt::Key_Up && !m_emojiCompleter->isVisible()) {
        if (m_input->textCursor().blockNumber() == 0) {
            handleHistoryUp(m_input);
            return true;
        }
    }
    if (ke->key() == Qt::Key_Down && !m_emojiCompleter->isVisible()) {
        if (m_input->textCursor().blockNumber() == m_input->document()->blockCount() - 1) {
            handleHistoryDown(m_input);
            return true;
        }
    }

    if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
        if (ke->modifiers() & Qt::ShiftModifier)
            return false; // let QPlainTextEdit insert newline
        onInputSubmit();
        return true;
    }

    // Ctrl+Shift+K: mIRC color picker (Ctrl+K is the quick channel switcher).
    if (ke->key() == Qt::Key_K
        && (ke->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier))) {
        showColorPicker();
        return true;
    }

    // mIRC formatting: toggle visual QTextCharFormat only — no control chars
    // in the widget. IRC codes come from the document at send time.
    if (applyFormatShortcut(m_input, ke)) return true;
    if (ke->modifiers() == Qt::ControlModifier && ke->key() == Qt::Key_O) {
        m_input->setCurrentCharFormat(QTextCharFormat{});
        updateFormatIndicator();
        return true;
    }

    return QMainWindow::eventFilter(obj, event);
}

// ---------------------------------------------------------------------------
// Sidebar helpers
// ---------------------------------------------------------------------------

void MainWindow::syncSidebarOrderToConfig()
{
    QList<ServerConfig> reordered;
    for (int i = 0; i < m_sidebar->topLevelItemCount(); ++i) {
        auto *item = m_sidebar->topLevelItem(i);
        if (!item) continue;
        const QString host = item->data(0, Qt::UserRole).toString();
        for (const auto &sc : std::as_const(m_config.servers))
            if (sc.name == host) { reordered.append(sc); break; }
    }
    if (reordered.size() == m_config.servers.size()) {
        m_config.servers = reordered;
        saveConfig(true);
    }
}

void MainWindow::syncSidebarOrderFromConfig()
{
    for (int ci = 0; ci < m_config.servers.size(); ++ci) {
        const QString &name = m_config.servers[ci].name;
        for (int si = ci; si < m_sidebar->topLevelItemCount(); ++si) {
            if (m_sidebar->topLevelItem(si)->data(0, Qt::UserRole).toString() == name) {
                if (si != ci) {
                    auto *item = m_sidebar->takeTopLevelItem(si);
                    m_sidebar->insertTopLevelItem(ci, item);
                    item->setExpanded(true);
                }
                break;
            }
        }
    }
}

void MainWindow::syncChannelOrderToConfig(const ServerId &host)
{
    auto *srvItem = m_sidebarCtl->serverItem(host);
    if (!srvItem) return;
    for (auto &sc : m_config.servers) {
        if (sc.name != host.str()) continue;
        QList<ChannelConfig> reordered;
        for (int i = 0; i < srvItem->childCount(); ++i) {
            const QString ch = srvItem->child(i)->data(0, Qt::UserRole + 1).toString().toLower();
            for (const auto &cc : std::as_const(sc.channels))
                if (cc.name.toLower() == ch) { reordered.append(cc); break; }
        }
        // keep any config channels that aren't in the sidebar (shouldn't happen, but be safe)
        for (const auto &cc : std::as_const(sc.channels)) {
            bool found = false;
            for (const auto &r : std::as_const(reordered))
                if (r.name.toLower() == cc.name.toLower()) { found = true; break; }
            if (!found) reordered.append(cc);
        }
        sc.channels = reordered;
        break;
    }
    saveConfig();
}

// ---------------------------------------------------------------------------
// Model → UI slots
// ---------------------------------------------------------------------------

void MainWindow::onServerAdded(const ServerId &host)
{
    if (m_sidebarCtl->serverItem(host)) return;
    m_sidebarCtl->addServerItem(host);

    if (m_signalBars && (m_model->activeHost().isEmpty() || host == m_model->activeHost()))
        m_signalBars->setState(SignalBars::State::Connecting);
}

void MainWindow::onServerConnected(const ServerId &host)
{
    m_sidebarCtl->markConnected(host);
    if (m_signalBars && host == m_model->activeHost())
        m_signalBars->setState(SignalBars::State::Connected);

    if (!m_config.profileDisplayName.isEmpty() || !m_config.profileAvatarUrl.isEmpty()
        || !m_config.profileStatusText.isEmpty()) {
        auto *cl = m_model->clientFor(host);
        if (cl && cl->hasMetadataCap()) {
            // Only push fields that are actually configured — an empty SET
            // clears the key server-side, clobbering values set elsewhere.
            // (Clearing on purpose goes through the preferences dialog.)
            if (!m_config.profileDisplayName.isEmpty())
                m_model->sendRaw(host, "METADATA * SET display-name :" + m_config.profileDisplayName);
            const bool localPath = m_config.profileAvatarUrl.startsWith('/')
                                   || QUrl(m_config.profileAvatarUrl).isLocalFile();
            if (!localPath && !m_config.profileAvatarUrl.isEmpty())
                m_model->sendRaw(host, "METADATA * SET avatar :" + m_config.profileAvatarUrl);
            if (!m_config.profileStatusText.isEmpty())
                m_model->sendRaw(host, "METADATA * SET status :" + m_config.profileStatusText);
        }
        // Local file avatars are never sent to the server, so seed nickMeta + cache manually.
        if (!m_config.profileAvatarUrl.isEmpty()) {
            const bool localPath = m_config.profileAvatarUrl.startsWith('/')
                                   || QUrl(m_config.profileAvatarUrl).isLocalFile();
            if (localPath) {
                if (auto *sess = m_model->session(host); sess && !sess->nick.isEmpty())
                    m_model->onUserMetaChanged(host, sess->nick, "avatar", m_config.profileAvatarUrl);
                fetchAvatar(m_config.profileAvatarUrl);
            }
        }
    }
}

void MainWindow::onServerDisconnected(const ServerId &host)
{
    m_sidebarCtl->clearConnectionIcon(host);
    if (m_signalBars && host == m_model->activeHost())
        m_signalBars->setState(SignalBars::State::Disconnected);

    // Prune typing state for all channels on this host
    m_typing->forgetHost(host);

    if (!m_model->session(host)) {
        // No session left → the server was removed (Manage Servers), not just
        // dropped. Tear down its panes and floating windows or they linger
        // as zombies pointing at the removed session.
        closePanesForHost(host);
    }
}

// Closes every docked pane and floating window belonging to a server.
void MainWindow::closePanesForHost(const ServerId &host)
{
    const QString prefix = host.str() + "|";
    const QStringList keys = m_panes.keys();
    for (const QString &k : keys) {
        if (!k.startsWith(prefix)) continue;
        if (auto *pane = m_panes.value(k))
            closeChannelPane(pane->host(), pane->channel());
    }
}

void MainWindow::onServerClosed(const ServerId &host)
{
    if (!m_sidebarCtl->serverItem(host)) return;

    // Close any open panes/windows for channels on this server
    closePanesForHost(host);

    // Prune per-channel caches for this server
    const QString prefix = host.str() + '\t';
    for (auto it = m_scrollPositions.begin(); it != m_scrollPositions.end(); )
        it = it.key().startsWith(prefix) ? m_scrollPositions.erase(it) : ++it;
    for (auto it = m_renderStart.begin(); it != m_renderStart.end(); )
        it = it.key().startsWith(prefix) ? m_renderStart.erase(it) : ++it;
    for (auto it = m_inputDrafts.begin(); it != m_inputDrafts.end(); )
        it = it.key().startsWith(prefix) ? m_inputDrafts.erase(it) : ++it;
    for (auto it = m_historyExhausted.begin(); it != m_historyExhausted.end(); )
        it = it->startsWith(prefix) ? m_historyExhausted.erase(it) : ++it;

    m_sidebarCtl->removeServerItem(host);

    if (m_signalBars && host == m_model->activeHost())
        m_signalBars->setState(SignalBars::State::Disconnected);

    onSidebarSelectionChanged();
}

void MainWindow::onChannelAdded(const ServerId &host, const BufferId &channel)
{
    if (m_sidebarCtl->channelItem(host, channel)) return;
    auto *item = m_sidebarCtl->addChannelItem(host, channel);
    if (!item) return;

    // A recreated row starts plain — restore the channel avatar if one is known
    if (auto *chData = m_model->channel(host, channel); chData && !chData->avatarUrl.isEmpty())
        onChannelAvatarChanged(host, channel, chData->avatarUrl);

    // Checked out to a floating window: re-mark, but don't select or raise —
    // (re)joins would otherwise steal focus and misplace the sidebar highlight.
    if (m_paneWindows.contains(paneKey(host, channel))) {
        m_sidebarCtl->setCheckedOut(host, channel, true);
        return;
    }

    // Only a join the user asked for takes the view. Config autojoins and the
    // ones a reconnect replays used to select each row as it arrived, so the
    // last channel to come up stole both the highlight and — with the main
    // view closed — the pane the user was reading, while the view itself was
    // left on the first channel. The first buffer of a session still opens.
    if (!m_model->takeUserJoin(host, channel) && !m_model->activeChannel().isEmpty())
        return;

    m_sidebar->setCurrentItem(item);
    switchToChannel(host, channel);
}

void MainWindow::onChannelRemoved(const ServerId &host, const BufferId &channel)
{
    auto *item = m_sidebarCtl->channelItem(host, channel);
    if (item) delete item;
    closeChannelPane(host, channel);

    const QString key = bufferKey(host, channel);
    m_scrollPositions.remove(key);
    m_renderStart.remove(key);
    m_inputDrafts.remove(key);
    m_historyExhausted.remove(key);

    onSidebarSelectionChanged();
}


void MainWindow::onTopicChanged(const ServerId &host, const BufferId &channel, const QString &topic)
{
    if (host == m_model->activeHost() &&
        channel.str().toLower() == m_model->activeChannel().str().toLower())
        refreshTopicBar(host, channel);

    const QString key = paneKey(host, channel);
    if (auto *pane = m_panes.value(key))
        pane->setTopic(ChatRenderer::linkifyTopic(topic));
}

void MainWindow::onNickListChanged(const ServerId &host, const BufferId &channel)
{
    scheduleNickRefresh(host, channel);
}

void MainWindow::scheduleNickRefresh(const ServerId &host, const BufferId &channel)
{
    const QString key = paneKey(host, channel);
    if (m_nickRefreshPending.contains(key)) return;
    m_nickRefreshPending.insert(key);
    QTimer::singleShot(50, this, [this, host, channel, key] {
        m_nickRefreshPending.remove(key);
        const bool isActive = (host == m_model->activeHost() &&
                               channel.str().toLower() == m_model->activeChannel().str().toLower());
        if (isActive) {
            refreshNickList(host, channel);
            refreshTopicBar(host, channel);
        }
        if (auto *pane = m_panes.value(key))
            refreshPaneNickList(pane);
    });
}

void MainWindow::onSelfNickChanged(const ServerId &host, const QString &nick)
{
    if (host == m_model->activeHost()) {
        m_nickPrefix->setText(nick);
        m_selfNickRe = SessionModel::buildHighlightRe(nick);
    }

    for (auto *pane : std::as_const(m_panes))
        if (pane->host() == host)
            pane->setNick(nick);
}

void MainWindow::updateTypingLabel()
{
    if (!m_config.ui.typingIndicator)
        m_typingLabel->setVisible(false);
    else
        m_typingLabel->setText(m_typing->typingText(m_model->activeHost(), m_model->activeChannel()));

    // Panes (docked and popped-out) carry their own typing indicator.
    for (auto *pane : std::as_const(m_panes))
        pane->setTyping(m_typing->typingText(pane->host(), pane->channel()));
}

// ---------------------------------------------------------------------------
// UI → Model
// ---------------------------------------------------------------------------

void MainWindow::onSidebarSelectionChanged()
{
    auto *item = m_sidebar->currentItem();
    if (!item) return;
    const ServerId host{item->data(0, Qt::UserRole).toString()};
    const BufferId channel{item->data(0, Qt::UserRole + 1).toString()};
    if (host.isEmpty() || channel.isEmpty()) return;
    showBuffer(host, channel);
}

// The pane a loaded buffer should land in, if any. Typing in a docked pane?
// Load it there and leave the primary (and the layout) alone. With the main
// view closed the panes are all there is, so it loads into one of them
// either way — switching into the hidden main view would put a dismissed
// view back on screen. Null means the primary takes it.
ChannelPane *MainWindow::paneRouteTarget() const
{
    if (m_focusedPane && m_orderedPanes.contains(m_focusedPane))
        return m_focusedPane;
    return m_primaryPanel->isHidden() ? currentPaneTarget() : nullptr;
}

// Every route that loads a buffer on the user's behalf — sidebar click,
// /query, /msg, the nick menu's Message — funnels through here so none of
// them can land in the hidden primary.
void MainWindow::showBuffer(const ServerId &host, const BufferId &channel)
{
    if (ChannelPane *target = paneRouteTarget())
        retargetPane(target, host, channel);
    else
        switchToChannel(host, channel);
}

void MainWindow::navigateChannel(int direction)
{
    QList<QTreeWidgetItem*> channels;
    for (int s = 0; s < m_sidebar->topLevelItemCount(); ++s) {
        auto *srv = m_sidebar->topLevelItem(s);
        for (int c = 0; c < srv->childCount(); ++c) {
            auto *child = srv->child(c);
            const QString key = paneKey(child->data(0, Qt::UserRole).toString(),
                                         child->data(0, Qt::UserRole + 1).toString());
            if (m_paneWindows.contains(key)) continue; // checked out to a window
            channels.append(child);
        }
    }
    if (channels.isEmpty()) return;

    const int count = static_cast<int>(channels.size());
    int cur = static_cast<int>(channels.indexOf(m_sidebar->currentItem()));
    int next = (cur < 0) ? 0 : cur + direction;
    if (next < 0) next = count - 1;
    if (next >= count) next = 0;

    m_sidebar->setCurrentItem(channels[next]);
    onSidebarSelectionChanged();
}

// Cycles keyboard focus across the docked panes' inputs and the primary
// input, in layout slot order. Docked panes can't become the active buffer
// (switchToChannel keeps them out of the primary view), so pane navigation
// moves focus rather than sidebar selection.
void MainWindow::navigatePane(int direction)
{
    if (m_orderedPanes.isEmpty()) return;

    // Focus targets follow the layout tree, so Alt+arrow walks the views in
    // the order they appear on screen.
    QList<QWidget*> targets;
    int cur = 0;
    QWidget *fw = QApplication::focusWidget();
    for (QWidget *view : paneViewOrder()) {
        if (view == m_primaryPanel) {
            targets.append(m_input);
        } else if (auto *p = qobject_cast<ChannelPane *>(view)) {
            targets.append(p->input());
        } else {
            continue;
        }
        // Whichever view owns the focus is where the walk starts.
        if (fw && view->isAncestorOf(fw)) cur = static_cast<int>(targets.size()) - 1;
    }
    if (targets.isEmpty()) return;

    const int count = static_cast<int>(targets.size());
    int next = cur + direction;
    if (next < 0) next = count - 1;
    if (next >= count) next = 0;
    targets[next]->setFocus();
}

// ---------------------------------------------------------------------------
// Input dispatch
// ---------------------------------------------------------------------------

// A pending reply only applies to the buffer it was started in.
QString MainWindow::replyMsgidFor(const ServerId &host, const BufferId &channel) const
{
    if (m_pendingReplyMsgid.isEmpty()) return {};
    if (host != m_pendingReplyHost) return {};
    if (channel.str().compare(m_pendingReplyChannel.str(), Qt::CaseInsensitive) != 0) return {};
    return m_pendingReplyMsgid;
}

void MainWindow::dispatchInput(const QString &text, const ServerId &host, const BufferId &channel)
{
    if (text.startsWith('/')) {
        m_dispatcher->dispatch(text, host, channel, replyMsgidFor(host, channel));
        return;
    }

    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty() || channel.str() == "(server)") return;

    // Substitute :shortcode: patterns before sending
    static const QRegularExpression shortcodeRe(R"(:(\w+):)");
    QString outText = trimmed;
    qsizetype offset = 0;
    auto it = shortcodeRe.globalMatch(trimmed);
    while (it.hasNext()) {
        const auto m = it.next();
        const QString emoji = emojiForCode(m.capturedView(1));
        if (!emoji.isEmpty()) {
            outText.replace(m.capturedStart() + offset, m.capturedLength(), emoji);
            offset += emoji.length() - m.capturedLength();
        }
    }

    const QString replyMsgid = replyMsgidFor(host, channel);
    if (!replyMsgid.isEmpty()) clearReplyBar();
    m_model->sendMessage(host, channel, outText, replyMsgid);
}

// ---------------------------------------------------------------------------
// View helpers
// ---------------------------------------------------------------------------

ChannelPane *MainWindow::currentPaneTarget() const
{
    if (m_focusedPane && m_orderedPanes.contains(m_focusedPane)) return m_focusedPane;
    for (QWidget *w : paneViewOrder())
        if (auto *p = qobject_cast<ChannelPane *>(w)) return p;
    return nullptr;
}

// The highlight follows the primary view, never a pane — a checked-out or
// paned row left selected would claim to be what the main view is showing.
// Closed with its ✕, though, the main view shows nothing at all, and leaving
// the highlight on the buffer it still holds marks a row the user can't see.
// The pane they're working in stands in until the main view is back.
void MainWindow::syncSidebarToActive()
{
    ServerId host = m_model->activeHost();
    BufferId chan = m_model->activeChannel();
    if (m_primaryPanel->isHidden())
        if (ChannelPane *p = currentPaneTarget()) { host = p->host(); chan = p->channel(); }

    if (auto *active = m_sidebarCtl->channelItem(host, chan))
        m_sidebar->setCurrentItem(active);
    else
        m_sidebar->clearSelection();
}

void MainWindow::switchToChannel(const ServerId &host, const BufferId &channel)
{
    // Checked out to a floating window — raise it instead of loading in main,
    // and keep the sidebar highlight on what the main view is actually showing.
    const QString key = paneKey(host, channel);
    if (auto *win = m_paneWindows.value(key)) {
        win->show();
        win->raise();
        win->activateWindow();
        syncSidebarToActive();
        return;
    }

    // Already docked in a visible pane — don't also load it into the primary
    // view. Same reasoning as the floating-window case above.
    if (m_panes.contains(key)) {
        syncSidebarToActive();
        return;
    }

    const QString prevKey = bufferKey(m_model->activeHost(), m_model->activeChannel());

    // Save scroll position for the channel we're leaving (only if not at bottom)
    if (m_chatView && !prevKey.startsWith('\t')) {
        if (!m_chatView->isAtBottom())
            m_scrollPositions[prevKey] = m_chatView->verticalScrollBar()->value();
        else
            m_scrollPositions.remove(prevKey);
    }

    // Stash the unsent draft for the buffer we're leaving; restored below.
    if (m_input && !prevKey.startsWith('\t')) {
        const QString draft = m_input->toPlainText();
        if (draft.isEmpty())
            m_inputDrafts.remove(prevKey);
        else
            m_inputDrafts[prevKey] = draft;
        // End the typing indicator on the old buffer — the inactivity timer
        // would otherwise fire "paused" at whatever buffer is active by then.
        m_typing->noteBufferLeft(m_model->activeHost(), m_model->activeChannel());
    }

    // The ✕ on the main view is a real close — loading a buffer into it must
    // not undo that. Panes cover every route here (a sidebar click retargets
    // one, and the pane/primary trade below lands in the hidden view on
    // purpose); closing the last pane is what brings it back.
    if (m_orderedPanes.isEmpty()) {
        // Showing it while it sits outside the layout would open it as a
        // window of its own — put it back in the splitter first.
        if (!m_primaryPanel->parentWidget()) rebuildPaneLayout();
        m_primaryPanel->setVisible(true);
    }

    const bool isChannel = isChannelName(channel.str());

    // Nick panel: only meaningful in channels
    if (m_nickPanel) {
        const bool show = isChannel && m_nickExpanded;
        m_nickPanel->setVisible(show);
        if (m_nickRevealBtn)
            m_nickRevealBtn->setVisible(isChannel && !m_nickExpanded);
    }

    // Topic button + topic bar: only meaningful in channels
    if (m_primaryTopicBtn)
        m_primaryTopicBtn->setVisible(isChannel);
    if (m_topicDisplay && !isChannel)
        m_topicDisplay->setVisible(false);
    else if (m_topicDisplay && isChannel)
        m_topicDisplay->setVisible(m_showTopic && m_primaryTopicBtn && m_primaryTopicBtn->isChecked());

    clearReplyBar();
    m_model->setActive(host, channel);
    refreshChatView(host, channel);
    refreshNickList(host, channel);
    refreshTopicBar(host, channel);

    // Restore this buffer's unsent draft (empty if none)
    if (m_input) {
        m_restoringDraft = true;
        m_input->setPlainText(m_inputDrafts.value(bufferKey(host, channel)));
        m_input->moveCursor(QTextCursor::End);
        m_restoringDraft = false;
    }

    if (auto *sess = m_model->session(host)) {
        m_nickPrefix->setText(sess->nick);
        m_selfNickRe = SessionModel::buildHighlightRe(sess->nick);
        if (m_nickDelegate)
            m_nickDelegate->setSelfAway(sess->nick, sess->away);
    }

    if (m_signalBars) {
        auto *sess = m_model->session(host);
        if (!sess)
            m_signalBars->setState(SignalBars::State::None);
        else if (sess->connected)
            m_signalBars->setState(SignalBars::State::Connected);
        else
            m_signalBars->setState(SignalBars::State::Disconnected);
    }

    setWindowTitle("Uplink — " + channel.str() + " @ " + host.str());
    updateTypingLabel();
    updateLengthIndicator();
}

void MainWindow::openChannelList(const ServerId &host)
{
    if (m_channelListDialog && m_channelListDialog->host() == host) {
        m_channelListDialog->show();
        m_channelListDialog->raise();
        m_channelListDialog->activateWindow();
        return;
    }

    if (m_channelListDialog)
        m_channelListDialog->deleteLater();

    m_channelListDialog = new ChannelListDialog(host, this);

    connect(m_model, &SessionModel::channelListEntry,
            m_channelListDialog, [this, host](const ServerId &h, const BufferId &ch, int u, const QString &t) {
        if (h == host)
            m_channelListDialog->addEntry(ch.str(), u, t);
    });
    connect(m_model, &SessionModel::channelListEnd,
            m_channelListDialog, [this, host](const ServerId &h, int total) {
        if (h == host)
            m_channelListDialog->onListEnd(total);
    });
    connect(m_channelListDialog, &ChannelListDialog::joinRequested,
            this, [this](const ServerId &h, const BufferId &channel) {
        m_model->sendJoin(h, channel); // via sendJoin so the view follows it
    });
    connect(m_channelListDialog, &ChannelListDialog::refreshRequested,
            this, [this](const ServerId &h) {
        m_channelListDialog->reset();
        m_model->sendRaw(h, "LIST");
    });

    m_model->sendRaw(host, "LIST");
    m_channelListDialog->show();
}

// ---------------------------------------------------------------------------
// Channel panes
// ---------------------------------------------------------------------------

// Creates and fully wires a ChannelPane, registering it in m_panes. The caller
// decides where it lives — docked (openChannelPane) or floating (popOutChannel).
ChannelPane *MainWindow::createPane(const ServerId &host, const BufferId &channel)
{
    const QString key = paneKey(host, channel);
    if (m_panes.contains(key)) return nullptr;

    auto *pane = new ChannelPane(host, channel, this);
    // Panes are auxiliary views — cap their scrollback below the main
    // view's so each extra pane costs less memory over a long session.
    pane->chatView()->setMaxLines(kPaneMaxLines);
    pane->setNickModel(new NickListModel(m_model, &m_nickStyle));
    if (m_theme.valid)
        pane->chatView()->setColors(QColor(m_theme.text), QColor(m_theme.background),
                                    QColor(m_theme.accent), QColor(m_theme.background),
                                    QColor(m_theme.border));

    pane->input()->installEventFilter(this);
    // Right-click in the pane's user list, same menu the main one gets —
    // scoped to this pane's channel.
    pane->nickList()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(pane->nickList(), &QListView::customContextMenuRequested,
            this, [this, pane](const QPoint &pos){
        const QModelIndex idx = pane->nickList()->indexAt(pos);
        if (!idx.isValid()) return;
        const QString nick = idx.data(Qt::UserRole).toString();
        if (nick.isEmpty()) return;
        showNickContextMenu(nick, pane->nickList()->viewport()->mapToGlobal(pos),
                            pane->host(), pane->channel());
    });
    // Keep the pane's format badge in step with the cursor, as the main
    // input does — the char format changes as you move through the text.
    connect(pane->input(), &QPlainTextEdit::cursorPositionChanged, this, [this, pane]{
        updateFormatIndicatorFor(pane->input());
    });
    // Panes get the same :shortcode completion as the main view — and the
    // same outgoing typing notifications, which only the main input sent.
    connect(pane->input(), &QPlainTextEdit::textChanged, this, [this, pane]{
        if (m_restoringDraft) return; // buffer switch, not the user typing
        checkEmojiAutocomplete(pane->input(), pane->input()->toPlainText());
        m_typing->noteInputChanged(pane->host(), pane->channel(),
                                   !pane->input()->toPlainText().isEmpty());
    });
    pane->setTypingEnabled(m_config.ui.typingIndicator);
    pane->setTyping(m_typing->typingText(pane->host(), pane->channel())); // seed current typers
    {
        const QColor ic(m_theme.valid ? m_theme.text : QStringLiteral("#e3e3e3"));
        pane->setSearchIcon(MenuIcons::fromSvg(QStringLiteral(":/icons/mi-search.svg"), ic, 20));
        pane->setPopOutIcon(MenuIcons::pipEnter(ic));
        pane->setNickPanelIcons(
            MenuIcons::fromSvg(QStringLiteral(":/icons/mi-right-panel-close.svg"), ic, 20),
            MenuIcons::fromSvg(QStringLiteral(":/icons/mi-left-panel-close.svg"), ic, 20),
            MenuIcons::groups(ic, 20));
        if (m_theme.valid) {
            pane->setNickChrome(m_config.ui.panelCards ? m_theme.nicklistBg : m_theme.bufferBg,
                                m_config.ui.panelCards);
            pane->setInputBase(QColor(m_theme.inputBg));
        }
    }
    connect(pane, &ChannelPane::escapePressed, this, [this, pane]{
        if (!replyMsgidFor(pane->host(), pane->channel()).isEmpty()) clearReplyBar();
    });
    // Seed the unsent text stashed for this buffer, as retargetPane does —
    // a draft typed in the main view survives the channel moving to a pane.
    if (const QString draft = m_inputDrafts.value(bufferKey(host, channel)); !draft.isEmpty()) {
        m_restoringDraft = true; // not the user typing: no completer, no TAGMSG
        pane->input()->setPlainText(draft);
        m_restoringDraft = false;
        pane->input()->moveCursor(QTextCursor::End);
    }
    connect(pane, &ChannelPane::popOutRequested, this, [this, pane]{ floatPane(pane); });
    connect(pane->chatView(), &ChatView::anchorActivated, this,
            [this, pane](const QString &anchor, const QPoint &gp, Qt::MouseButton btn){
        if (anchor.startsWith(QLatin1String("evgrp:"))) {
            toggleEventGroupInView(pane->chatView(), anchor.mid(6),
                                   pane->host(), pane->channel());
        } else if (btn == Qt::LeftButton && anchor.startsWith(QLatin1String("nick:"))) {
            const QString nick = anchor.mid(5);
            QPlainTextEdit *inp = pane->input();
            inp->setFocus();
            const QString cur = inp->toPlainText();
            inp->setPlainText(cur.isEmpty() ? nick + ": " : cur + nick + " ");
            QTextCursor c = inp->textCursor();
            c.movePosition(QTextCursor::End);
            inp->setTextCursor(c);
        } else if (btn == Qt::LeftButton
                   && (anchor.startsWith(QLatin1String("url:"))
                       || anchor.startsWith(QLatin1String("preview:")))) {
            QString href = anchor;
            if (href.startsWith("url:"))     href = href.mid(4);
            if (href.startsWith("preview:")) href = href.mid(8);
            const QUrl u(href);
            const QString s = u.scheme().toLower();
            if (s == "http" || s == "https") QDesktopServices::openUrl(u);
        } else {
            handleChatViewContextMenu(pane->chatView(), anchor, gp,
                                      pane->host(), pane->channel());
        }
    });
    connect(pane->chatView(), &ChatView::anchorHovered, this,
            [this, pane](const QString &anchor){
        if (anchor.startsWith("nick:")) {
            const QString nick = anchor.mid(5);
            const QString tip = nickTooltip(nick, pane->host());
            if (!tip.isEmpty())
                QToolTip::showText(QCursor::pos(), tip, pane->chatView()->viewport());
        } else {
            QToolTip::hideText();
        }
    });

    if (auto *sess = m_model->session(host))
        pane->setNick(sess->nick);
    pane->setNickVisible(m_showNickPrefix);
    {
        const FontSizes &fs = m_config.ui.fontSizes;
        const QString    fam = UiStyle::effectiveFontFamily(m_config.ui.fontFamily);
        auto makeFont = [&](double pt){
            QFont f;
            f.setFamilies({fam,
                           QStringLiteral("Noto Color Emoji"),
                           QStringLiteral("Segoe UI Emoji"),
                           QStringLiteral("Apple Color Emoji")});
            f.setPointSizeF(pt);
            return f;
        };
        pane->setChatFont(makeFont(fs.chat));
        pane->setNickListFont(makeFont(fs.nickList));
        pane->setNickPanelFont(makeFont(fs.nickDock));
        {
            auto *nd = new NickDelegate(pane->nickList());
            if (m_theme.valid)
                nd->setColors(QColor(m_theme.accent),
                              QColor(m_theme.border),
                              QColor(m_theme.text));
            pane->nickList()->setItemDelegate(nd);
        }
        pane->setTopicFont(makeFont(fs.topicText));
        pane->setInputFont(makeFont(fs.inputNick), makeFont(fs.input));
        pane->setTypingFont(makeFont(fs.typing));
    }

    if (auto *ch = m_model->channel(host, channel))
        pane->setTopic(ChatRenderer::linkifyTopic(ch->topic));

    // Read the buffer off the pane, not the values it was created with — a
    // docked pane can be retargeted at a different channel later.
    connect(pane, &ChannelPane::closeRequested, this, [this, pane]{
        closeChannelPane(pane->host(), pane->channel());
    });
    connect(pane, &ChannelPane::inputSubmitted, this, [this, pane](const QString &text){
        hideEmojiAutocomplete();
        // Read the IRC codes off the document — bold/italic/colour live in
        // the char formats, and toPlainText() would drop every one of them.
        const QString raw = inputToIrcText(pane->input());
        // Panes share the main view's history, both ways: what's sent from
        // one is what Up cycles back through in any of them.
        if (const QString trimmed = raw.trimmed(); !trimmed.isEmpty()) {
            pushInputHistory(trimmed);
            noteHistoryTarget(pane->input());
        }
        dispatchInput(raw.isEmpty() ? text : raw, pane->host(), pane->channel());
    });
    pane->setDropZoneFilter([this, pane](const QString &sourceKey, ChannelPane::DropZone zone){
        ChannelPane *source = (sourceKey == kPrimaryDragKey) ? nullptr
                                                             : m_panes.value(sourceKey);
        if (sourceKey != kPrimaryDragKey && !source) return false;
        return paneDropWouldChange(source, pane, zone);
    });
    connect(pane, &ChannelPane::dropReceived, this,
            [this, pane](const QString &sourceKey, ChannelPane::DropZone zone){
        // Dropped on an edge: place the dragged view on that side of this one
        // instead of trading slots, and take the axis the side implies.
        if (zone != ChannelPane::DropZone::Center) {
            ChannelPane *source = (sourceKey == kPrimaryDragKey)
                ? nullptr : m_panes.value(sourceKey);
            if (sourceKey == kPrimaryDragKey || source)
                placePaneBeside(source, pane, zone);
            return;
        }
        // Middle of the pane: the dragged view and this one trade places.
        QWidget *source = (sourceKey == kPrimaryDragKey)
            ? static_cast<QWidget *>(m_primaryPanel)
            : static_cast<QWidget *>(m_panes.value(sourceKey));
        if (!source || source == pane) return;
        if (swapLeaves(m_paneTree, viewId(source), viewId(pane)))
            rebuildPaneLayout();
    });

    m_panes[key] = pane;

    if (m_theme.valid) {
        pane->setTopicIcon(
            MenuIcons::topicBubble(QColor(m_theme.placeholder)),
            MenuIcons::topicBubble(QColor(m_theme.accent)));
    }
    return pane;
}

// Say why something didn't happen, in the buffer the user is looking at —
// a menu entry that does nothing at all reads as broken.
void MainWindow::notifyFocusedBuffer(const QString &text)
{
    ServerId h;
    BufferId c;
    if (m_focusedPane && m_orderedPanes.contains(m_focusedPane)) {
        h = m_focusedPane->host();
        c = m_focusedPane->channel();
    } else {
        h = m_model->activeHost();
        c = m_model->activeChannel();
    }
    if (!h.isEmpty() && !c.isEmpty())
        m_model->localMessage(h, c, text);
}

// Docks a channel as a tiled pane in the main window.
void MainWindow::openChannelPane(const ServerId &host, const BufferId &channel)
{
    // Already open somewhere — go to it rather than doing nothing.
    if (auto *existing = m_panes.value(paneKey(host, channel))) {
        if (auto *win = m_paneWindows.value(existing->key())) {
            win->show(); win->raise(); win->activateWindow();
        } else {
            m_focusedPane = existing;
            existing->input()->setFocus();
        }
        return;
    }
    if (m_orderedPanes.size() >= kMaxExtraPanes) {
        notifyFocusedBuffer(QStringLiteral("Pane limit reached (%1) — close one first.")
                                .arg(kMaxExtraPanes));
        return;
    }
    if (m_viewById.isEmpty()) seedPaneTree();

    // Decide where it goes before creating it, so a refusal costs nothing.
    int targetId = -1;
    Qt::Orientation axis = Qt::Horizontal;
    if (!chooseSplitTarget(targetId, axis)) {
        notifyFocusedBuffer(QStringLiteral(
            "No room for another pane — every view would end up too small. "
            "Widen the window, or close a pane."));
        return;
    }

    auto *pane = createPane(host, channel);
    if (!pane) return;

    const int newId = m_nextViewId++;
    m_viewById.insert(newId, pane);
    splitLeaf(m_paneTree, targetId, newId, axis, false);
    m_orderedPanes.append(pane);
    m_primaryHeader->setVisible(true);
    m_primaryCloseBtn->setVisible(true);

    rebuildPaneLayout();
    refreshPaneChatView(pane);
    refreshPaneNickList(pane);
    m_model->markRead(pane->host(), pane->channel());
    // The same channel never shows in the primary and a pane at once — the
    // pop-out path shifts the primary away, and docking has to match it.
    if (host == m_model->activeHost() &&
        channel.str().compare(m_model->activeChannel().str(), Qt::CaseInsensitive) == 0)
        switchAwayFromChannel(host, channel);
    // Right-clicking the row to open it here moved the selection onto a
    // channel the main view isn't showing; the highlight tracks the primary.
    syncSidebarToActive();
}

// Opens a channel in its own floating top-level window. Closing the window
// (or the pane's ✕) drops it back to the server list without leaving the buffer.
void MainWindow::popOutChannel(const ServerId &host, const BufferId &channel)
{
    const QString key = paneKey(host, channel);
    // Already a window → just raise it. Already a docked pane → float that one.
    if (auto *win = m_paneWindows.value(key)) {
        win->show(); win->raise(); win->activateWindow();
        return;
    }
    if (m_paneWindows.size() >= kMaxPaneWindows) {
        notifyFocusedBuffer(QStringLiteral("Window limit reached (%1) — close one first.")
                                .arg(kMaxPaneWindows));
        return;
    }
    if (auto *existing = m_panes.value(key)) { floatPane(existing); return; }

    if (auto *pane = createPane(host, channel))
        floatPane(pane);
}

// Moves a pane (freshly created or currently docked) into its own floating
// top-level window and checks the channel out of the main view.
void MainWindow::floatPane(ChannelPane *pane)
{
    // Cap check must precede the m_orderedPanes removal so a refused
    // docked pane stays docked untouched.
    if (!pane) return;
    if (m_paneWindows.size() >= kMaxPaneWindows) {
        notifyFocusedBuffer(QStringLiteral("Window limit reached (%1) — close one first.")
                                .arg(kMaxPaneWindows));
        return;
    }
    const QString  key     = pane->key();
    const ServerId host    = pane->host();
    const BufferId channel = pane->channel();

    const bool wasDocked = m_orderedPanes.removeOne(pane);
    if (wasDocked)
        if (const int id = viewId(pane); id >= 0) {
            removeLeaf(m_paneTree, id);
            m_viewById.remove(id);
        }
    if (m_focusedPane == pane) m_focusedPane = nullptr; // no longer a docked target
    if (m_emojiTarget == pane->input()) hideEmojiAutocomplete(); // it changes window

    auto *win = new QWidget(nullptr, Qt::Window);
    win->setObjectName("paneWindow");
    // Belt and suspenders for the pane's styled backgrounds: if any of the
    // plain-QWidget surfaces (#channelPane/#topicDisplay/#inputBar) miss
    // their QSS background after the reparent, the system palette would
    // show through. Paint the window itself in the buffer colour — scoped
    // to #paneWindow so it doesn't cascade into the children.
    if (m_theme.valid)
        win->setStyleSheet(QStringLiteral("QWidget#paneWindow { background-color: %1; }")
                               .arg(m_theme.bufferBg));
    win->setWindowTitle(channel.str() + " — " + host.str());
    auto *lay = new QVBoxLayout(win);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(pane); // reparents the pane into the window
    {
        QSettings settings("uplink", "uplink");
        const QByteArray geom = settings.value("paneWinGeom/" + key).toByteArray();
        if (geom.isEmpty() || !win->restoreGeometry(geom))
            win->resize(820, 620);
    }
    win->installEventFilter(this); // catch the OS close button
    m_paneWindows[key] = win;

    pane->setCloseIcon(MenuIcons::pipExit(
        QColor(m_theme.valid ? m_theme.placeholder : QStringLiteral("#888888"))));
    pane->setPopOutVisible(false); // it's already a window now
    pane->enableSearchShortcut();  // main window's Ctrl+F can't reach this window
    // Freshly created panes get their first style polish under the new
    // window and paint correctly; a reparented docked pane relies on Qt's
    // implicit repolish, which lands asynchronously on Wayland/KDE and can
    // leave the QSS backgrounds (#channelPane, #topicDisplay, #inputBar)
    // unpainted — the window then shows the system palette colour. Force
    // the same full polish a fresh pane gets.
    const auto paneKids = pane->findChildren<QWidget*>();
    for (QWidget *w : paneKids) { w->style()->unpolish(w); w->style()->polish(w); }
    pane->style()->unpolish(pane);
    pane->style()->polish(pane);
    // Repolishing resets programmatic fonts to the app default —
    // re-apply the configured fonts.
    applyFontSizes();
    win->show();

    if (wasDocked) {
        if (m_orderedPanes.isEmpty()) {
            m_primaryCloseBtn->setVisible(false);
            m_primaryPanel->setVisible(true);
        }
        rebuildPaneLayout();
    }

    // Checked out to the window — remove it from the main view and mark it.
    if (host == m_model->activeHost() &&
        channel.str().toLower() == m_model->activeChannel().str().toLower())
        switchAwayFromChannel(host, channel);
    m_sidebarCtl->setCheckedOut(host, channel, true);
    m_model->markRead(host, channel);

    // A previously docked pane is already rendered; reparenting keeps its
    // content, so only freshly created panes need the initial fill.
    if (!wasDocked) {
        refreshPaneChatView(pane);
        refreshPaneNickList(pane);
    }
}

// Italicises/dims a channel's sidebar row while it's checked out to a floating
// window, or restores it to normal.
// Moves the main view off host/channel to another available (non-popped) buffer,
// falling back to the server buffer if that channel was the only one.
void MainWindow::switchAwayFromChannel(const ServerId &host, const BufferId &channel)
{
    const QString skipKey = paneKey(host, channel);
    for (int s = 0; s < m_sidebar->topLevelItemCount(); ++s) {
        auto *srv = m_sidebar->topLevelItem(s);
        for (int c = 0; c < srv->childCount(); ++c) {
            auto *child = srv->child(c);
            const QString key = paneKey(child->data(0, Qt::UserRole).toString(),
                                         child->data(0, Qt::UserRole + 1).toString());
            if (key == skipKey || m_paneWindows.contains(key) || m_panes.contains(key)) continue;
            m_sidebar->setCurrentItem(child);
            onSidebarSelectionChanged();
            return;
        }
    }
    if (auto *srv = m_sidebarCtl->serverItem(host)) {
        m_sidebar->setCurrentItem(srv);
        onSidebarSelectionChanged();
    }
}

void MainWindow::closeChannelPane(const ServerId &host, const BufferId &channel)
{
    const QString key = paneKey(host, channel);
    auto *pane = m_panes.take(key);
    if (!pane) return;
    if (m_focusedPane == pane) m_focusedPane = nullptr;
    if (m_emojiTarget == pane->input()) hideEmojiAutocomplete();
    if (m_pendingReplyPane == pane) clearReplyBar(); // don't outlive the pane
    m_typing->noteBufferLeft(host, channel); // close out an in-flight typing state

    // Keep the unsent text, same as the main view does on a switch — the
    // buffer stays open, only its view is going away. The floating path
    // below immediately reloads the buffer into the main view, which reads
    // this stash back.
    const QString draftKey = bufferKey(host, channel);
    if (const QString draft = pane->input()->toPlainText(); !draft.isEmpty())
        m_inputDrafts[draftKey] = draft;
    else
        m_inputDrafts.remove(draftKey);

    // Floating pane: tear down its window and return to the server list.
    if (auto *win = m_paneWindows.take(key)) {
        QSettings settings("uplink", "uplink");
        settings.setValue("paneWinGeom/" + key, win->saveGeometry());
        win->removeEventFilter(this);
        win->deleteLater(); // deletes the pane it owns
        m_sidebarCtl->setCheckedOut(host, channel, false);
        // Skip the reselect while the server itself is being torn down —
        // no point churning the main view through dying buffers. And with
        // the main view closed, don't load the buffer into it invisibly (or
        // steal a pane the user is reading) — back to the server list it
        // goes, highlight kept on what's actually showing.
        if (m_model->session(host)) {
            if (m_primaryPanel->isHidden()) {
                syncSidebarToActive();
            } else if (auto *item = m_sidebarCtl->channelItem(host, channel)) {
                m_sidebar->setCurrentItem(item);
                switchToChannel(host, channel); // available again → show in main
            }
        }
        return;
    }

    m_orderedPanes.removeOne(pane);
    if (const int id = viewId(pane); id >= 0) {
        removeLeaf(m_paneTree, id);
        m_viewById.remove(id);
    }
    pane->setParent(nullptr); // detach before rebuild
    pane->deleteLater();

    if (m_orderedPanes.isEmpty()) {
        m_primaryCloseBtn->setVisible(false);
        m_primaryPanel->setVisible(true);
    }

    rebuildPaneLayout();
    // The closed channel may still own the highlight — put it back on whatever
    // the main view is showing, as the floating-window path above already does.
    syncSidebarToActive();
}

// Which way panes split. On "auto" the shape of the pane area decides: a wide
// area splits into columns, a tall one into rows, so neither half ends up
// cramped. Evaluated per layout rebuild (open, close, rearrange) and not on
// every resize — re-flowing under the cursor mid-drag would be worse than the
// wrong axis. The pane area can still be unsized during the startup restore,
// so fall back to the chat section and then the window.
bool MainWindow::paneRowsAxis() const
{
    const QString &mode = m_config.ui.paneSplitAxis;
    if (mode == "rows")    return true;
    if (mode == "columns") return false;

    QSize s;
    if (m_panesSplitter && m_panesSplitter->width() > 0 && m_panesSplitter->height() > 0)
        s = m_panesSplitter->size();
    else if (m_chatSection && m_chatSection->width() > 0)
        s = m_chatSection->size();
    else
        s = size();
    return s.height() > s.width();
}

// Builds the widget side of a layout tree: leaves are the views themselves,
// every split becomes a nested QSplitter carrying its own axis.
// Splitter sizes are relative, so the stored fractions can be handed over as
// they are — no need to know the real extent, which isn't settled yet anyway.
static void applyFractions(QSplitter *s, const PaneNode &node)
{
    if (!s || s->count() == 0) return;
    const std::vector<double> f = (node.fractions.size() == node.children.size())
        ? node.fractions : evenFractions(node.children.size());
    QList<int> sizes;
    for (int i = 0; i < s->count(); ++i)
        sizes << qMax(1, int((i < int(f.size()) ? f[size_t(i)] : 0.0) * 10000));
    s->setSizes(sizes);
}

static QWidget *buildPaneWidgets(const PaneNode &node, const QHash<int, QWidget*> &views,
                                 const std::function<void(QWidget*)> &show)
{
    if (node.isLeaf()) {
        QWidget *w = views.value(node.slot);
        if (w) show(w);
        return w;
    }
    auto *split = new QSplitter(node.axis);
    split->setHandleWidth(2);
    split->setChildrenCollapsible(false); // clamp, don't snap panes closed
    for (const PaneNode &child : node.children)
        if (QWidget *w = buildPaneWidgets(child, views, show))
            split->addWidget(w);
    applyFractions(split, node);
    return split;
}

// Reads the splitters back into the tree so a drag isn't lost the next time
// the layout is rebuilt. The widget tree mirrors the node tree exactly —
// buildPaneWidgets adds children in node order — so a parallel walk lines up.
static void captureFractions(PaneNode &node, QSplitter *s)
{
    if (!s || node.isLeaf()) return;
    const QList<int> sizes = s->sizes();
    if (sizes.size() == int(node.children.size())) {
        double total = 0;
        for (int v : sizes) total += v;
        if (total > 0) {
            node.fractions.clear();
            for (int v : sizes) node.fractions.push_back(double(v) / total);
        }
    }
    for (int i = 0; i < s->count() && i < int(node.children.size()); ++i)
        captureFractions(node.children[size_t(i)], qobject_cast<QSplitter *>(s->widget(i)));
}

// ---------------------------------------------------------------------------
// Layout persistence. Leaves are stored by buffer, not by view id: ids are
// handed out per run and mean nothing across a restart.
// ---------------------------------------------------------------------------

static QString paneLayoutKeyFor(QWidget *view, QWidget *primary)
{
    if (view == primary) return QStringLiteral("primary");
    if (auto *p = qobject_cast<ChannelPane *>(view))
        return p->host().str() + "|" + p->channel().str();
    return {};
}

static QJsonObject paneNodeToJson(const PaneNode &node, const QHash<int, QWidget*> &views,
                                  QWidget *primary)
{
    QJsonObject obj;
    if (node.isLeaf()) {
        obj["view"] = paneLayoutKeyFor(views.value(node.slot), primary);
        return obj;
    }
    obj["axis"] = (node.axis == Qt::Horizontal) ? "h" : "v";
    const std::vector<double> f = (node.fractions.size() == node.children.size())
        ? node.fractions : evenFractions(node.children.size());
    QJsonArray sizes;
    for (double v : f) sizes.append(v);
    obj["sizes"] = sizes;
    QJsonArray kids;
    for (const PaneNode &c : node.children)
        kids.append(paneNodeToJson(c, views, primary));
    obj["children"] = kids;
    return obj;
}

// Rebuilds a node from JSON, resolving each stored buffer back to a live view
// id. Leaves whose channel didn't come back are dropped, and a split left with
// nothing is dropped with them.
static bool paneNodeFromJson(const QJsonObject &obj, const QHash<QString, int> &idForKey,
                             PaneNode &out)
{
    if (obj.contains("view")) {
        const int id = idForKey.value(obj["view"].toString(), -1);
        if (id < 0) return false;
        out = PaneNode{};
        out.slot = id;
        return true;
    }
    PaneNode node;
    node.axis = (obj["axis"].toString() == "v") ? Qt::Vertical : Qt::Horizontal;
    const QJsonArray kids  = obj["children"].toArray();
    const QJsonArray sizes = obj["sizes"].toArray();
    for (int i = 0; i < kids.size(); ++i) {
        PaneNode child;
        if (!paneNodeFromJson(kids[i].toObject(), idForKey, child)) continue;
        node.children.push_back(std::move(child));
        node.fractions.push_back(i < sizes.size() ? sizes[i].toDouble() : 0.0);
    }
    if (node.children.empty()) return false;
    double total = 0;
    for (double v : node.fractions) total += v;
    if (total <= 0) node.fractions = evenFractions(node.children.size());
    else for (double &v : node.fractions) v /= total;
    if (node.children.size() == 1) {
        out = std::move(node.children[0]);   // a split of one is just its child
        return true;
    }
    out = std::move(node);
    return true;
}

int MainWindow::viewId(const QWidget *view) const
{
    for (auto it = m_viewById.constBegin(); it != m_viewById.constEnd(); ++it)
        if (it.value() == view) return it.key();
    return -1;
}

QWidget *MainWindow::viewForId(int id) const { return m_viewById.value(id); }

// The tree always holds the primary; panes are split off it.
void MainWindow::seedPaneTree()
{
    m_viewById.clear();
    m_viewById.insert(0, m_primaryPanel);
    m_paneTree = PaneNode{};
    m_paneTree.axis = paneRowsAxis() ? Qt::Vertical : Qt::Horizontal;
    PaneNode primary;
    primary.slot = 0;
    m_paneTree.children.push_back(primary);
}

QList<QWidget*> MainWindow::paneViewOrder() const
{
    QList<QWidget*> out;
    for (int id : leafOrder(m_paneTree))
        if (QWidget *w = m_viewById.value(id)) out.append(w);
    return out;
}

// A new pane splits the roomiest view in half, along whichever side of it is
// longer, so panes stay as square as the window allows. Halloy calls this
// largest-shorter; with the axis forced, only the target is chosen. Views that
// would end up under kMinPaneExtent are left alone, which is what caps the
// number of panes now that the shape table is gone.
bool MainWindow::chooseSplitTarget(int &targetId, Qt::Orientation &axis) const
{
    constexpr int kMinPaneExtent = 260;

    int bestId = -1;
    qint64 bestArea = -1;
    Qt::Orientation bestAxis = Qt::Horizontal;
    for (int id : leafOrder(m_paneTree)) {
        QWidget *w = m_viewById.value(id);
        if (!w || w->isHidden()) continue;
        const QSize sz = w->size();
        const Qt::Orientation split = (m_config.ui.paneSplitAxis == "columns") ? Qt::Horizontal
                                    : (m_config.ui.paneSplitAxis == "rows")    ? Qt::Vertical
                                    : (sz.width() >= sz.height() ? Qt::Horizontal : Qt::Vertical);
        const int extent = (split == Qt::Horizontal) ? sz.width() : sz.height();
        if (extent / 2 < kMinPaneExtent) continue; // both halves would be too thin
        const qint64 area = qint64(sz.width()) * sz.height();
        if (area > bestArea) { bestArea = area; bestId = id; bestAxis = split; }
    }
    if (bestId < 0) return false;
    targetId = bestId;
    axis     = bestAxis;
    return true;
}

// Puts the panes back where they were, at the sizes they were. Falls back to
// whatever opening them produced if the saved layout no longer describes the
// views that came back — a channel may have been removed from the config, or
// failed to rejoin.
void MainWindow::restorePaneLayout(const QString &json)
{
    if (json.isEmpty()) { rebuildPaneLayout(); return; }

    QHash<QString, int> idForKey;
    for (auto it = m_viewById.constBegin(); it != m_viewById.constEnd(); ++it) {
        const QString key = paneLayoutKeyFor(it.value(), m_primaryPanel);
        if (!key.isEmpty()) idForKey.insert(key, it.key());
    }

    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    PaneNode restored;
    if (!doc.isObject() || !paneNodeFromJson(doc.object(), idForKey, restored)) {
        rebuildPaneLayout();
        return;
    }

    // Anything on screen that the saved layout doesn't mention still needs a
    // home: give it half of whichever view has the most room.
    PaneNode tree = restored;
    if (tree.isLeaf()) {
        PaneNode root;
        root.axis = paneRowsAxis() ? Qt::Vertical : Qt::Horizontal;
        root.children.push_back(tree);
        tree = root;
    }
    m_paneTree = tree;
    for (auto it = m_viewById.constBegin(); it != m_viewById.constEnd(); ++it) {
        if (containsLeaf(m_paneTree, it.key())) continue;
        int targetId = -1;
        Qt::Orientation axis = Qt::Horizontal;
        if (chooseSplitTarget(targetId, axis))
            splitLeaf(m_paneTree, targetId, it.key(), axis, false);
    }

    rebuildPaneLayout();
}

void MainWindow::rebuildPaneLayout()
{
    // The primary can be hidden via its ✕ button — a rebuild must not
    // resurrect it. Capture before the detach below (setParent(nullptr)
    // marks every detached widget hidden).
    const bool primaryHidden = m_primaryPanel->isHidden();
    auto showWidget = [this, primaryHidden](QWidget *w){
        if (w == m_primaryPanel && primaryHidden) return;
        w->show();
    };

    if (m_viewById.isEmpty()) seedPaneTree();
    // Closing panes back down to one collapses the root into a lone leaf, and
    // the top-level splitter can only be handed a split's children — without
    // the wrapper the last view is left detached and shows up as its own
    // window the next time something makes it visible.
    if (m_paneTree.isLeaf()) {
        PaneNode root;
        root.axis = paneRowsAxis() ? Qt::Vertical : Qt::Horizontal;
        if (m_paneTree.slot >= 0) root.children.push_back(m_paneTree);
        m_paneTree = root;
    }
    // Whatever the user dragged the handles to is the truth until now.
    captureFractions(m_paneTree, m_panesSplitter);

    // Every live view needs a leaf. One that isn't in the tree still gets
    // detached below and is then never re-added, and a parentless widget
    // shown is a top-level window — that's how clicking a channel used to
    // pop the main view out into one. Take in the strays at the root; the
    // shares are re-evened because a reclaimed view has none of its own.
    for (auto it = m_viewById.constBegin(); it != m_viewById.constEnd(); ++it) {
        if (containsLeaf(m_paneTree, it.key())) continue;
        PaneNode stray;
        stray.slot = it.key();
        m_paneTree.children.push_back(stray);
        m_paneTree.fractions.clear();
    }

    const QList<QWidget*> widgets = paneViewOrder();

    // Detach all pane widgets from wherever they currently live
    for (auto *w : std::as_const(widgets))
        if (w->parentWidget())
            w->setParent(nullptr);

    // Remove and delete any nested splitters left in m_panesSplitter
    while (m_panesSplitter->count() > 0) {
        auto *w = m_panesSplitter->widget(0);
        w->setParent(nullptr);
        if (auto *s = qobject_cast<QSplitter *>(w))
            delete s;
    }

    // The primary header only becomes a drag handle once there's a pane to
    // trade places with — don't advertise the grab before then.
    if (m_primaryHeader)
        m_primaryHeader->setCursor(m_orderedPanes.isEmpty() ? Qt::ArrowCursor
                                                            : Qt::OpenHandCursor);

    // Walk the layout tree, mapping leaf ids to the views they stand for.
    m_panesSplitter->setOrientation(m_paneTree.axis);
    for (const PaneNode &child : m_paneTree.children)
        if (QWidget *w = buildPaneWidgets(child, m_viewById, showWidget))
            m_panesSplitter->addWidget(w);

    applyFractions(m_panesSplitter, m_paneTree);

    // The setParent(nullptr) detach above makes the style engine repolish
    // every pane, which resets programmatic fonts to the app default —
    // re-apply the configured fonts.
    applyFontSizes();
}

// The tree an edge-drop would produce. Empty when the drop can't be honoured.
PaneNode MainWindow::paneTreeAfterDrop(ChannelPane *source, ChannelPane *target,
                                       ChannelPane::DropZone zone) const
{
    if (zone == ChannelPane::DropZone::Center) return {};
    // nullptr is the primary view on either side of the drop.
    const int movingId = viewId(source ? static_cast<QWidget *>(source) : m_primaryPanel);
    const int targetId = viewId(target ? static_cast<QWidget *>(target) : m_primaryPanel);
    if (movingId < 0 || targetId < 0 || movingId == targetId) return {};

    const bool before = (zone == ChannelPane::DropZone::Left ||
                         zone == ChannelPane::DropZone::Top);
    const Qt::Orientation axis = (zone == ChannelPane::DropZone::Left ||
                                  zone == ChannelPane::DropZone::Right) ? Qt::Horizontal
                                                                        : Qt::Vertical;
    PaneNode next = m_paneTree;
    if (!moveLeafBeside(next, movingId, targetId, axis, before)) return {};
    return next;
}

// Whether a drop would actually move anything. Dropping a pane on the side of
// a neighbour it already sits on lands it back where it started, and lighting
// that up promises a rearrangement the release can't deliver.
bool MainWindow::paneDropWouldChange(ChannelPane *source, ChannelPane *target,
                                     ChannelPane::DropZone zone) const
{
    if (zone == ChannelPane::DropZone::Center)
        return source != target; // a swap always moves both views

    const PaneNode after = paneTreeAfterDrop(source, target, zone);
    if (after.children.empty() && after.isLeaf()) return false;
    return after != m_paneTree;
}

// Moves `source` next to `target` on the side the drop chose. A nullptr means
// the primary view in either position. Splits now carry their own axis, so a
// placement no longer has to force the whole layout one way.
void MainWindow::placePaneBeside(ChannelPane *source, ChannelPane *target,
                                 ChannelPane::DropZone zone)
{
    const PaneNode after = paneTreeAfterDrop(source, target, zone);
    if (after != m_paneTree && !(after.isLeaf() && after.children.empty())) {
        m_paneTree = after;
        rebuildPaneLayout();
    }
}

// Puts another buffer in an already-docked pane. The pane keeps its slot, the
// layout is untouched, and the primary view carries on with whatever it had.
void MainWindow::retargetPane(ChannelPane *pane, const ServerId &host, const BufferId &channel)
{
    if (!pane) return;
    const QString oldKey = pane->key();
    const QString newKey = paneKey(host, channel);
    if (oldKey == newKey) return;

    // A buffer lives in exactly one view. If it's already open elsewhere the
    // click can't be honoured, so bounce the highlight back like
    // switchToChannel does for the same case.
    if (m_panes.contains(newKey) || m_paneWindows.contains(newKey)) {
        syncSidebarToActive();
        return;
    }

    const ServerId oldHost = pane->host();
    const BufferId oldChan = pane->channel();
    // The primary is showing the clicked buffer: trade, rather than refuse.
    // The pane gets what was asked for and the primary takes the pane's old
    // channel, so nothing ends up displayed twice and nothing disappears.
    const bool takeFromPrimary =
        host == m_model->activeHost() &&
        channel.str().compare(m_model->activeChannel().str(), Qt::CaseInsensitive) == 0;

    // Stash the unsent text under the buffer it was written for, same as the
    // primary does on a switch — the two share m_inputDrafts.
    const QString oldDraftKey = bufferKey(oldHost, oldChan);
    const QString draft = pane->input()->toPlainText();
    if (draft.isEmpty()) m_inputDrafts.remove(oldDraftKey);
    else                 m_inputDrafts[oldDraftKey] = draft;

    if (m_pendingReplyPane == pane) clearReplyBar(); // it belonged to the old buffer
    // A typing state in flight belongs to the buffer being left, same as the
    // main view does on a switch.
    m_typing->noteBufferLeft(oldHost, oldChan);

    m_panes.remove(oldKey);
    pane->retarget(host, channel);
    m_panes[newKey] = pane;

    if (auto *sess = m_model->session(host))
        pane->setNick(sess->nick);
    if (auto *ch = m_model->channel(host, channel))
        pane->setTopic(ChatRenderer::linkifyTopic(ch->topic));
    pane->setTyping(m_typing->typingText(host, channel));
    m_restoringDraft = true; // a restored draft must not pop the completer
    pane->input()->setPlainText(m_inputDrafts.value(bufferKey(host, channel)));
    m_restoringDraft = false;
    pane->input()->moveCursor(QTextCursor::End);
    refreshPaneChatView(pane);
    refreshPaneNickList(pane);
    m_model->markRead(host, channel);

    if (takeFromPrimary)
        switchToChannel(oldHost, oldChan);
    else
        syncSidebarToActive();
    pane->input()->setFocus(); // stay in the pane; the next click targets it too
}

void MainWindow::refreshPaneNickList(ChannelPane *pane)
{
    pane->clearNickFilter();
    pane->nickModel()->setBuffer(pane->host(), pane->channel());
    auto *ch = m_model->channel(pane->host(), pane->channel());
    pane->setNickCount(ch ? static_cast<int>(ch->nicks.size()) : 0);
}

QString MainWindow::topicAgeStr(quint64 ts)
{
    if (ts == 0) return {};
    const qint64 now  = QDateTime::currentSecsSinceEpoch();
    const qint64 secs = now - static_cast<qint64>(ts);
    if (secs < 60)      return QObject::tr("just now");
    if (secs < 3600)    return QObject::tr("%1m ago").arg(secs / 60);
    if (secs < 86400)   return QObject::tr("%1h ago").arg(secs / 3600);
    if (secs < 604800)  return QObject::tr("%1d ago").arg(secs / 86400);
    return QObject::tr("%1w ago").arg(secs / 604800);
}

void MainWindow::refreshTopicBar(const ServerId &host, const BufferId &channel)
{
    auto *ch = m_model->channel(host, channel);

    QString serverName = host.str();
    for (const auto &sc : std::as_const(m_config.servers))
        if (sc.name == host.str() && !sc.name.isEmpty()) { serverName = sc.name; break; }

    if (channel.str() == "(server)") {
        m_topicLabel->setFullText({});
        m_userInfoLabel->setText(serverName);
        if (m_topicText) m_topicText->clear();
        if (m_topicSetByLabel) m_topicSetByLabel->hide();
    } else {
        const QString modes   = ch ? ch->modes : QString();
        const QString modeStr = modes.isEmpty() ? QString() : " (" + modes + ")";
        m_topicLabel->setFullText(channel.str() + modeStr);
        m_userInfoLabel->clear();

        if (m_topicText) {
            const QString topicHtml = ChatRenderer::linkifyTopic(ch ? ch->topic : QString());
            const double topicPt = m_config.ui.fontSizes.topicText;
            m_topicText->setText(topicHtml.isEmpty()
                ? topicHtml
                : QString("<span style='font-size:%1pt;'>%2</span>").arg(topicPt).arg(topicHtml));
        }

        if (m_topicSetByLabel) {
            const QString setter = ch ? ch->topicSetBy : QString();
            const quint64 ts     = ch ? ch->topicSetAt : 0;
            if (!setter.isEmpty() && ts > 0) {
                m_topicSetByLabel->setFullText("Topic set by " + setter.section('!', 0, 0) + " · " + topicAgeStr(ts));
                m_topicSetByLabel->show();
            } else {
                m_topicSetByLabel->hide();
            }
        }
    }

}

QString MainWindow::msgidAtViewPos(const QPoint & /*viewPos*/) const
{
    // Phase 3: implement via ChatView hit-test
    return {};
}

void MainWindow::openLogSearch()
{
    const ServerId host   = m_model->activeHost();
    const BufferId target = m_model->activeChannel();
    if (host.str().isEmpty() || target.str().isEmpty())
        return;

    auto *dlg = new LogSearchDialog(target.str(),
                                    m_model->logFilePath(host, target),
                                    SessionModel::logsRootPath(),
                                    m_model->messageLoggingEnabled(), this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    connect(dlg, &LogSearchDialog::jumpRequested, this,
            [this](const QString &serverPart, const QString &bufferPart, const QDateTime &ts){
        ServerId jumpHost;
        BufferId jumpChannel;
        if (!m_model->resolveLogBuffer(serverPart, bufferPart, jumpHost, jumpChannel))
            return; // logs of a buffer that isn't open — nowhere to jump
        if (auto *item = m_sidebarCtl->channelItem(jumpHost, jumpChannel)) {
            m_sidebar->setCurrentItem(item);
            onSidebarSelectionChanged();
        }
        startHistoryJump(jumpHost, jumpChannel, ts);
    });
    connect(dlg, &LogSearchDialog::jumpInBufferRequested, this,
            [this, host, target](const QDateTime &ts){
        if (host != m_model->activeHost() || target != m_model->activeChannel()) {
            auto *item = m_sidebarCtl->channelItem(host, target);
            if (!item) return; // buffer closed since the dialog was opened
            m_sidebar->setCurrentItem(item);
            onSidebarSelectionChanged();
        }
        startHistoryJump(host, target, ts);
    });
    dlg->show();
}

void MainWindow::clearReplyBar()
{
    m_pendingReplyMsgid.clear();
    m_pendingReplyHost    = {};
    m_pendingReplyChannel = {};
    if (m_pendingReplyPane) {
        // The pane paints its own input background every frame (seam guard),
        // so nudge the viewport — a placeholder swap alone can sit stale
        // until something else forces a repaint.
        m_pendingReplyPane->input()->setPlaceholderText("Type a message...");
        m_pendingReplyPane->input()->viewport()->update();
        m_pendingReplyPane = nullptr;
    }
    if (m_replyLabel) m_replyLabel->setText({});
    if (m_replyBar)   m_replyBar->hide();
    updateLengthIndicator();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_tray && m_tray->isVisible()) {
        hide();
        event->ignore();
    } else {
        // Quitting: close floating pane windows too, or the app stays alive
        // with only a channel window and no way back to the main UI. Detach
        // our filter first and keep m_paneWindows intact so the aboutToQuit
        // handler still persists them for the next launch.
        for (auto *win : std::as_const(m_paneWindows)) {
            win->removeEventFilter(this);
            win->close();
        }
        event->accept();
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
}

// Accept pane drags over the whole window — sidebar, headers, chrome. Any
// rejecting surface flips the drag cursor to the forbidden shape, and the
// grab hand should hold from pickup to drop (see ChannelPane's dnd
// handlers). A drop outside a real target just ends the drag in place.
void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat(ChannelPane::mimeType().toUtf8()))
        event->acceptProposedAction();
}

void MainWindow::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasFormat(ChannelPane::mimeType().toUtf8()))
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasFormat(ChannelPane::mimeType().toUtf8()))
        event->acceptProposedAction();
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::PaletteChange && m_sendBtn)
        m_sendBtn->setIcon(MenuIcons::send({}, 26));

    if (event->type() == QEvent::ActivationChange && isActiveWindow() && m_tray)
        m_tray->setNotify(false);

    if (event->type() == QEvent::WindowStateChange) {
        const auto *sc = static_cast<QWindowStateChangeEvent *>(event);
        const bool wasMaximized = sc->oldState() & Qt::WindowMaximized;
        const bool isNormal     = !(windowState() & Qt::WindowMaximized);
        if (wasMaximized && isNormal) {
            QTimer::singleShot(0, this, [this]{
                auto *scr = screen() ? screen() : QGuiApplication::primaryScreen();
                if (!scr) return;
                const QRect avail = scr->availableGeometry();
                QRect w = geometry();
                if (w.width() > avail.width() - 80)
                    w.setWidth(qMin(kDefaultWindowW, avail.width() - 100));
                if (w.right()  > avail.right())  w.moveRight(avail.right());
                if (w.left()   < avail.left())   w.moveLeft(avail.left());
                setGeometry(w);
            });
        }
    }

    QMainWindow::changeEvent(event);
}

void MainWindow::saveConfig(bool migratePasswords)
{
    m_configSaving = true;
    Config::save(m_config, Config::defaultPath(), migratePasswords);

    // Some editors replace the file (delete + create) which removes the watch
    const QString path = Config::defaultPath();
    if (m_configWatcher.files().isEmpty())
        m_configWatcher.addPath(path);

    // Keep the guard up long enough for the async watcher notification to arrive
    QTimer::singleShot(1000, this, [this]{ m_configSaving = false; });
}

void MainWindow::onConfigFileChanged()
{
    if (m_configSaving) return;

    // Re-add the watch — editors that do atomic save (write tmp + rename) remove it
    const QString path = Config::defaultPath();
    if (m_configWatcher.files().isEmpty())
        QTimer::singleShot(500, this, [this, path]{ m_configWatcher.addPath(path); });

    // Debounce — some editors fire multiple events per save
    QTimer::singleShot(300, this, [this]{
        const Config fresh = Config::load(Config::defaultPath());

        // Diff server lists — add new, remove deleted
        QSet<QString> freshNames, currentNames;
        for (const auto &s : fresh.servers)    freshNames.insert(s.name);
        for (const auto &s : std::as_const(m_config.servers)) currentNames.insert(s.name);

        for (const auto &s : fresh.servers) {
            if (!currentNames.contains(s.name)) {
                m_config.servers.append(s);
                m_model->addServer(s);
            }
        }

        for (qsizetype i = m_config.servers.size() - 1; i >= 0; --i) {
            if (!freshNames.contains(m_config.servers[i].name)) {
                m_model->removeServer(ServerId{m_config.servers[i].name});
                m_config.servers.removeAt(i);
            }
        }
    });
}

