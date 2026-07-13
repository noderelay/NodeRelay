#if defined(__linux__) && !defined(__MUSL__)
#include <malloc.h>
#endif
#include "mainwindow.h"
#include "ui/mainwindowdelegates.h"
#include "ui/commanddispatcher.h"
#include "ui/uistyle.h"
#include "ui/searchbar.h"
#include "ui/nickfilteredit.h"
#include "irc/ircclient.h"
#include "ui/dcccontroller.h"
#include "ui/trayicon.h"
#include "ui/aboutdialog.h"
#include "ui/channellistdialog.h"
#include "ui/docsdialog.h"
#include "ui/logsearchdialog.h"
#include "ui/fontdialog.h"
#include "ui/preferencesdialog.h"
#include "ui/serverdialog.h"
#include "ui/manageserversdialog.h"
#include "ui/appicons.h"
#include "ui/themeloader.h"
#include "ui/linkpreview.h"
#include "ui/previewcontroller.h"
#include "ui/emojipicker.h"
#include "ui/quickswitcher.h"
#include "ui/updatechecker.h"
#include "ui/emojidata.h"
#include "ui/chromepanel.h"
#include "ui/menuicons.h"
#include "ui/signalbars.h"
#include "ui/fadescrollbar.h"
#include "ui/channelpane.h"
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
static constexpr int kMaxExtraPanes   = 3;
static constexpr int kMaxPaneWindows  = 4;

// Returns the slot index sharing a nested cross-splitter ("stack") with
// `slot`, or -1 if `slot` is a full/lone top-level column with no stack-
// mate. Mirrors the slot shapes rebuildPaneLayout() actually builds for
// n<=2 / n==3 / n==4:
//   n<=2  — every slot is a lone top-level column, no stacking at all.
//   n==3  — slot 0 is lone; slots 1 and 2 share one stack.
//   n==4  — slots (0,1) share one stack, slots (2,3) share the other.
static int siblingSlot(qsizetype totalSlots, int slot)
{
    if (totalSlots <= 2) return -1;
    if (totalSlots == 3) return slot == 0 ? -1 : 3 - slot;
    return slot ^ 1; // n == 4
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
    {
        const QString iconDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
            + QStringLiteral("/icons/hicolor/256x256/apps");
        QDir().mkpath(iconDir);
        appIcon.pixmap(256, 256).save(iconDir + QStringLiteral("/uplink-irc.png"));
    }
    resize(kDefaultWindowW, kDefaultWindowH);

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
    connect(m_quickSwitcher, &QuickSwitcher::channelSelected, this, [this](ServerId host, BufferId channel){
        auto *item = findChannelItem(host, channel);
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

    if (settings.contains("nickSplitter"))
        m_chatSplitter->restoreState(settings.value("nickSplitter").toByteArray());
    if (settings.contains("sidebarWidth"))
        m_sidebarExpandedWidth = settings.value("sidebarWidth").toInt();

    const QStringList savedPanes       = settings.value("panes").toStringList();
    const int         savedPrimarySlot = settings.value("primarySlot", 0).toInt();

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
        QTimer::singleShot(0, this, [this, savedPanes, savedPrimarySlot]{
            for (const QString &k : savedPanes) {
                const qsizetype sep = k.indexOf('|');
                if (sep < 0) continue;
                openChannelPane(ServerId{k.left(sep)}, BufferId{k.mid(sep + 1)});
            }
            m_primarySlot = qBound(0, savedPrimarySlot, static_cast<int>(m_orderedPanes.size()));
            rebuildPaneLayout();
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

    connect(qApp, &QApplication::aboutToQuit, this, [this]{
        QSettings s("uplink", "uplink");
        s.setValue("geometry", saveGeometry());
        s.setValue("windowState", saveState());
        s.setValue("nickSplitter", m_chatSplitter->saveState());
        s.setValue("sidebarWidth", m_sidebarExpandedWidth);
        // Persist original-case host|channel (not the lowercased pane key):
        // the restore path rebuilds BufferIds from these strings, and a
        // lowercased channel breaks case-sensitive lookups (typing state)
        // and window titles.
        QStringList paneList;
        for (auto *p : std::as_const(m_orderedPanes))
            paneList << p->host().str() + "|" + p->channel().str();
        s.setValue("panes", paneList);
        s.setValue("primarySlot", m_primarySlot);
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

    m_dispatcher = new CommandDispatcher(m_model, &m_config, this, this);
    connect(m_dispatcher, &CommandDispatcher::switchChannel,  this, &MainWindow::switchToChannel);
    connect(m_dispatcher, &CommandDispatcher::focusInput,     this, [this]{ if (m_input) m_input->setFocus(); });
    connect(m_dispatcher, &CommandDispatcher::clearChat,
            this, &MainWindow::clearActiveBuffer);
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

// Right inset on the topic bar so its text wraps a little before the
// floating show-user-list button instead of running underneath it.
void MainWindow::setTopicRevealInset(bool reserve)
{
    if (auto *l = m_topicDisplay ? m_topicDisplay->layout() : nullptr)
        l->setContentsMargins(8, 3, reserve ? 44 : 8, 3);
}

void MainWindow::applyFontSizes()
{
    const QString &fam = m_config.ui.fontFamily;
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
    if (m_chatView)       m_chatView->setFont(makeFont(fs.chat));
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
    if (m_topicLabel)    m_topicLabel->setFont(makeFont(fs.topicBar));
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

void MainWindow::clearActiveBuffer()
{
    if (m_chatView) m_chatView->clear();
    auto *ch = m_model->channel(m_model->activeHost(), m_model->activeChannel());
    if (ch) ch->messages.clear();
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

    // Pane header dropped onto the primary view: pane <-> primary swap.
    if (obj == m_primaryPanel) {
        if (event->type() == QEvent::DragEnter || event->type() == QEvent::DragMove) {
            auto *de = static_cast<QDragMoveEvent *>(event);
            if (de->mimeData()->hasFormat(ChannelPane::mimeType().toUtf8())) {
                de->acceptProposedAction();
                if (!m_primaryDropFrame)
                    m_primaryDropFrame = new DropFrame(m_primaryPanel);
                m_primaryDropFrame->activate();
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

            // pane <-> primary: the dragged pane stays exactly where it is;
            // primary takes over whichever slot the dragged pane's stack-
            // mate currently holds (or the dragged pane's own slot directly,
            // if it has no stack-mate). E.g. dragging one of two stacked
            // panes onto a lone primary column: primary joins the stack
            // alongside the dragged pane, and the pane displaced from the
            // stack is promoted to the now-vacant lone column. Reduces to a
            // plain swap when the dragged pane has no stack-mate — or when
            // its stack-mate IS the primary (swapping primary with its own
            // slot would be a silent no-op).
            ChannelPane *source = m_panes.value(sourceKey);
            const qsizetype srcIdx = source ? m_orderedPanes.indexOf(source) : -1;
            if (srcIdx >= 0) {
                const qsizetype nSlots = 1 + m_orderedPanes.size();
                const int srcSlot = static_cast<int>(srcIdx < m_primarySlot ? srcIdx : srcIdx + 1);
                const int sib = siblingSlot(nSlots, srcSlot);
                const int swapSlot = (sib >= 0 && sib != m_primarySlot) ? sib : srcSlot;

                QList<ChannelPane*> combined; // nullptr marks the primary slot
                int pi = 0;
                for (int i = 0; i < nSlots; i++)
                    combined.append(i == m_primarySlot ? nullptr : m_orderedPanes[pi++]);

                ChannelPane *tmp = combined[m_primarySlot];
                combined[m_primarySlot] = combined[swapSlot];
                combined[swapSlot] = tmp;

                m_orderedPanes.clear();
                for (int i = 0; i < nSlots; i++) {
                    if (!combined[i]) m_primarySlot = i;
                    else m_orderedPanes.append(combined[i]);
                }
                rebuildPaneLayout();
            }
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
        m_nickRevealBtn && m_nickRevealBtn->isVisible()) {
        auto *re = static_cast<QResizeEvent *>(event);
        // Same line the collapse button occupied — keep in sync with
        // positionRevealBtn in setupNickPanel.
        const int topY = m_primaryHeader->height();
        m_nickRevealBtn->move(re->size().width() - m_nickRevealBtn->width() - 4, topY);
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
                if (ke->key() == Qt::Key_Tab) {
                    handleTabComplete(pane->input(), pane->host(), pane->channel());
                    return true;
                }
                // Non-Tab resets the completion cycle
                m_tabActive = false;
                m_tabCandidates.clear();
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
    if (m_emojiCompleter->isVisible()) {
        if (ke->key() == Qt::Key_Escape) {
            hideEmojiAutocomplete();
            return true;
        }
        if (ke->key() == Qt::Key_Up) {
            const int cur = m_emojiCompleter->currentRow();
            m_emojiCompleter->setCurrentRow(qMax(0, cur - 1));
            return true;
        }
        if (ke->key() == Qt::Key_Down) {
            const int cur = m_emojiCompleter->currentRow();
            m_emojiCompleter->setCurrentRow(
                qMin(m_emojiCompleter->count() - 1, cur + 1));
            return true;
        }
        if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter ||
            ke->key() == Qt::Key_Tab) {
            const int row = m_emojiCompleter->currentRow();
            if (row >= 0) {
                commitEmojiAutocomplete(row);
                return true;
            }
        }
    }

    if (ke->key() == Qt::Key_F && (ke->modifiers() & Qt::ControlModifier)) {
        m_searchBar->open();
        return true;
    }

    if (ke->key() == Qt::Key_Escape && !m_pendingReplyMsgid.isEmpty()) {
        clearReplyBar();
        return true;
    }

    if (ke->key() == Qt::Key_Tab) {
        handleTabComplete(m_input, m_model->activeHost(), m_model->activeChannel());
        return true;
    }

    // Any non-Tab key resets nick completion cycle
    m_tabActive = false;
    m_tabCandidates.clear();

    if (ke->key() == Qt::Key_Up && !m_emojiCompleter->isVisible()) {
        if (m_input->textCursor().blockNumber() == 0) {
            handleHistoryUp();
            return true;
        }
    }
    if (ke->key() == Qt::Key_Down && !m_emojiCompleter->isVisible()) {
        if (m_input->textCursor().blockNumber() == m_input->document()->blockCount() - 1) {
            handleHistoryDown();
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

    // mIRC formatting: toggle visual QTextCharFormat only — no control chars in the widget.
    // IRC codes are generated from the document formatting at send time (inputToIrcText).
    if (ke->modifiers() == Qt::ControlModifier) {
        switch (ke->key()) {
        case Qt::Key_B: {
            QTextCharFormat cf = m_input->currentCharFormat();
            cf.setFontWeight(cf.fontWeight() >= QFont::Bold ? QFont::Normal : QFont::Bold);
            m_input->textCursor().setCharFormat(cf);
            m_input->setCurrentCharFormat(cf);
            updateFormatIndicator();
            return true;
        }
        case Qt::Key_I: {
            QTextCharFormat cf = m_input->currentCharFormat();
            cf.setFontItalic(!cf.fontItalic());
            m_input->textCursor().setCharFormat(cf);
            m_input->setCurrentCharFormat(cf);
            updateFormatIndicator();
            return true;
        }
        case Qt::Key_U: {
            QTextCharFormat cf = m_input->currentCharFormat();
            cf.setFontUnderline(!cf.fontUnderline());
            m_input->textCursor().setCharFormat(cf);
            m_input->setCurrentCharFormat(cf);
            updateFormatIndicator();
            return true;
        }
        case Qt::Key_S: {
            QTextCharFormat cf = m_input->currentCharFormat();
            cf.setFontStrikeOut(!cf.fontStrikeOut());
            m_input->textCursor().setCharFormat(cf);
            m_input->setCurrentCharFormat(cf);
            updateFormatIndicator();
            return true;
        }
        case Qt::Key_O:
            m_input->setCurrentCharFormat(QTextCharFormat{});
            updateFormatIndicator();
            return true;
        default: break;
        }
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
    auto *srvItem = findServerItem(host);
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

QTreeWidgetItem *MainWindow::findServerItem(const ServerId &host) const
{
    for (int i = 0; i < m_sidebar->topLevelItemCount(); ++i) {
        auto *item = m_sidebar->topLevelItem(i);
        if (item->data(0, Qt::UserRole).toString() == host.str())
            return item;
    }
    return nullptr;
}

QTreeWidgetItem *MainWindow::findChannelItem(const ServerId &host, const BufferId &channel) const
{
    auto *srv = findServerItem(host);
    if (!srv) return nullptr;
    if (channel.str() == "(server)") return srv;
    for (int i = 0; i < srv->childCount(); ++i) {
        auto *item = srv->child(i);
        if (item->data(0, Qt::UserRole + 1).toString().toLower() == channel.str().toLower())
            return item;
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// Model → UI slots
// ---------------------------------------------------------------------------

static QString shortNetworkName(const QString &host)
{
    QString h = host;
    if (h.startsWith("irc.", Qt::CaseInsensitive))
        h = h.mid(4);
    const auto dot = h.lastIndexOf('.');
    if (dot > 0)
        h = h.left(dot);
    return h;
}

void MainWindow::onServerAdded(ServerId host)
{
    if (findServerItem(host)) return;
    QString label;
    for (const auto &sc : std::as_const(m_config.servers))
        if (sc.name == host.str() && !sc.name.isEmpty()) { label = sc.name; break; }
    if (label.isEmpty())
        label = shortNetworkName(host.str());
    auto *item = new QTreeWidgetItem(m_sidebar);
    item->setText(0, label.toUpper());
    item->setData(0, Qt::UserRole,     host.str());
    item->setData(0, Qt::UserRole + 1, QString("(server)"));
    item->setExpanded(true);
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    QFont f(m_config.ui.fontFamily);
    f.setPointSizeF(m_config.ui.fontSizes.serverHeader);
    f.setBold(true);
    item->setFont(0, f);
    item->setForeground(0, QColor("#6c7086"));

    if (m_signalBars && (m_model->activeHost().isEmpty() || host == m_model->activeHost()))
        m_signalBars->setState(SignalBars::State::Connecting);
}

void MainWindow::onServerConnected(ServerId host)
{
    auto *item = findServerItem(host);
    if (item)
        item->setData(0, Qt::UserRole + 2, QVariant::fromValue(MenuIcons::connectedServer()));
    if (m_signalBars && host == m_model->activeHost())
        m_signalBars->setState(SignalBars::State::Connected);
    updateBookmarksMenu();   // server submenu enable state

    if (!m_config.profileDisplayName.isEmpty() || !m_config.profileAvatarUrl.isEmpty()) {
        auto *cl = m_model->clientFor(host);
        if (cl && cl->hasCap("draft/metadata-2")) {
            m_model->sendRaw(host, "METADATA * SET display-name :" + m_config.profileDisplayName);
            const bool localPath = m_config.profileAvatarUrl.startsWith('/')
                                   || QUrl(m_config.profileAvatarUrl).isLocalFile();
            if (!localPath)
                m_model->sendRaw(host, "METADATA * SET avatar :" + m_config.profileAvatarUrl);
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

void MainWindow::onServerDisconnected(ServerId host)
{
    auto *item = findServerItem(host);
    if (item)
        item->setData(0, Qt::UserRole + 2, QVariant());
    if (m_signalBars && host == m_model->activeHost())
        m_signalBars->setState(SignalBars::State::Disconnected);
    updateBookmarksMenu();   // server submenu enable state

    // Prune typing state for all channels on this host
    const QString prefix = host.str() + "|";
    for (auto it = m_typingNickTimers.begin(); it != m_typingNickTimers.end(); ) {
        if (it.key().startsWith(prefix)) {
            it.value()->stop();
            it.value()->deleteLater();
            it = m_typingNickTimers.erase(it);
        } else {
            ++it;
        }
    }
    for (auto it = m_typingNicks.begin(); it != m_typingNicks.end(); )
        it = it.key().startsWith(prefix) ? m_typingNicks.erase(it) : ++it;

    if (auto *sess = m_model->session(host)) {
        for (const auto &ch : std::as_const(sess->channels))
            for (const QString &bn : ch.botNicks)
                m_botIconIdx.remove(bn);
        for (const QString &bn : sess->botNicks)
            m_botIconIdx.remove(bn);
    } else {
        // No session left → the server was removed (Manage Servers), not just
        // dropped. Tear down its panes and floating windows or they linger
        // as zombies pointing at the removed session.
        closePanesForHost(host);
    }
}

// Closes every docked pane and floating window belonging to a server.
void MainWindow::closePanesForHost(ServerId host)
{
    const QString prefix = host.str() + "|";
    const QStringList keys = m_panes.keys();
    for (const QString &k : keys) {
        if (!k.startsWith(prefix)) continue;
        if (auto *pane = m_panes.value(k))
            closeChannelPane(pane->host(), pane->channel());
    }
}

void MainWindow::onServerClosed(ServerId host)
{
    auto *srv = findServerItem(host);
    if (!srv) return;

    // Close any open panes/windows for channels on this server
    closePanesForHost(host);

    // Prune per-channel caches for this server
    const QString prefix = host.str() + '\t';
    for (auto it = m_scrollPositions.begin(); it != m_scrollPositions.end(); )
        it = it.key().startsWith(prefix) ? m_scrollPositions.erase(it) : ++it;
    for (auto it = m_renderStart.begin(); it != m_renderStart.end(); )
        it = it.key().startsWith(prefix) ? m_renderStart.erase(it) : ++it;

    const int idx = m_sidebar->indexOfTopLevelItem(srv);
    delete m_sidebar->takeTopLevelItem(idx);

    if (m_signalBars && host == m_model->activeHost())
        m_signalBars->setState(SignalBars::State::Disconnected);
    updateBookmarksMenu();   // server may have been removed from config

    onSidebarSelectionChanged();
}

void MainWindow::onChannelAdded(ServerId host, BufferId channel)
{
    if (findChannelItem(host, channel)) return;
    auto *srv = findServerItem(host);
    if (!srv) return;
    auto *item = new QTreeWidgetItem(srv);
    item->setText(0, channel.str());
    item->setData(0, Qt::UserRole,     host.str());
    item->setData(0, Qt::UserRole + 1, channel.str());

    // Checked out to a floating window: re-mark, but don't select or raise —
    // (re)joins would otherwise steal focus and misplace the sidebar highlight.
    if (m_paneWindows.contains(paneKey(host, channel))) {
        setChannelCheckedOut(host, channel, true);
        return;
    }

    m_sidebar->setCurrentItem(item);
    switchToChannel(host, channel);
}

void MainWindow::onChannelRemoved(ServerId host, BufferId channel)
{
    auto *item = findChannelItem(host, channel);
    if (item) delete item;
    closeChannelPane(host, channel);

    const QString key = host.str() + '\t' + channel.str();
    m_scrollPositions.remove(key);
    m_renderStart.remove(key);

    onSidebarSelectionChanged();
}


void MainWindow::onTopicChanged(ServerId host, BufferId channel, const QString &topic)
{
    if (host == m_model->activeHost() &&
        channel.str().toLower() == m_model->activeChannel().str().toLower())
        refreshTopicBar(host, channel);

    const QString key = paneKey(host, channel);
    if (auto *pane = m_panes.value(key))
        pane->setTopic(ChatRenderer::linkifyTopic(topic));
}

void MainWindow::onNickListChanged(ServerId host, BufferId channel)
{
    scheduleNickRefresh(host, channel);
}

void MainWindow::scheduleNickRefresh(ServerId host, BufferId channel)
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

void MainWindow::onUnreadChanged(ServerId host, BufferId channel, int count)
{
    auto *item = findChannelItem(host, channel);
    if (!item) return;
    QString label = channel.str();
    if (channel.str() == "(server)") {
        label = QString();
        for (const auto &sc : std::as_const(m_config.servers))
            if (sc.name == host.str() && !sc.name.isEmpty()) { label = sc.name; break; }
        if (label.isEmpty())
            label = shortNetworkName(host.str());
    }
    if (channel.str() == "(server)") {
        const bool connected = [&]{
            auto *s = m_model->session(host); return s && s->connected;
        }();
        if (connected) {
            const QColor col = count > 0 ? QColor("#e06c75") : QColor();
            item->setData(0, Qt::UserRole + 2, QVariant::fromValue(MenuIcons::connectedServer(col)));
        }
        item->setText(0, label.toUpper());
    } else {
        if (count > 0 && m_model->hasMention(host, channel))
            item->setData(0, Qt::UserRole + 2, QVariant::fromValue(MenuIcons::mention(QColor("#FFD700"))));
        else if (count > 0)
            item->setData(0, Qt::UserRole + 2, QVariant::fromValue(MenuIcons::unread()));
        else
            item->setData(0, Qt::UserRole + 2, QVariant());
        item->setData(0, Qt::UserRole + 3, count > 0 ? count : QVariant());
        item->setText(0, label);
    }
}

void MainWindow::onSelfNickChanged(ServerId host, const QString &nick)
{
    if (host == m_model->activeHost()) {
        m_nickPrefix->setText(nick);
        m_selfNickRe = SessionModel::buildHighlightRe(nick);
    }

    for (auto *pane : std::as_const(m_panes))
        if (pane->host() == host)
            pane->setNick(nick);
}

void MainWindow::onTypingReceived(ServerId host, BufferId channel,
                                   const QString &nick, const QString &state)
{
    if (!m_config.ui.typingIndicator) return;

    // Lowercase the channel so lookups match regardless of source case
    // (restored panes carry lowercased names; the server sends its own case).
    const QString key      = paneKey(host, channel);
    const QString timerKey = key + "|" + nick;

    if (state == "active" || state == "paused") {
        m_typingNicks[key].insert(nick);

        if (m_typingNickTimers.contains(timerKey)) {
            m_typingNickTimers[timerKey]->start(6000);
        } else {
            auto *t = new QTimer(this);
            t->setSingleShot(true);
            connect(t, &QTimer::timeout, this, [this, key, timerKey, nick]{
                m_typingNicks[key].remove(nick);
                if (auto *timer = m_typingNickTimers.value(timerKey)) {
                    m_typingNickTimers.remove(timerKey);
                    timer->deleteLater();
                }
                updateTypingLabel();
            });
            m_typingNickTimers.insert(timerKey, t);
            t->start(6000);
        }
    } else {
        m_typingNicks[key].remove(nick);
        if (auto *t = m_typingNickTimers.value(timerKey)) {
            t->stop();
            t->deleteLater();
            m_typingNickTimers.remove(timerKey);
        }
    }

    updateTypingLabel();
}


// Builds the "X is typing..." string for a buffer, or empty if nobody is.
QString MainWindow::typingText(ServerId host, BufferId channel) const
{
    if (!m_config.ui.typingIndicator) return {};
    const QString key = paneKey(host, channel);
    const QSet<QString> &typers = m_typingNicks.value(key);
    if (typers.isEmpty()) return {};

    QStringList names(typers.begin(), typers.end());
    if (names.size() == 1) return names[0] + " is typing...";
    if (names.size() == 2) return names[0] + " and " + names[1] + " are typing...";
    return QString::number(names.size()) + " people are typing...";
}

void MainWindow::updateTypingLabel()
{
    if (!m_config.ui.typingIndicator)
        m_typingLabel->setVisible(false);
    else
        m_typingLabel->setText(typingText(m_model->activeHost(), m_model->activeChannel()));

    // Panes (docked and popped-out) carry their own typing indicator.
    for (auto *pane : std::as_const(m_panes))
        pane->setTyping(typingText(pane->host(), pane->channel()));
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

    // Focus targets in slot order, with the primary input at m_primarySlot.
    QList<QWidget*> targets;
    int pi = 0;
    const qsizetype nSlots = 1 + m_orderedPanes.size();
    for (int i = 0; i < nSlots; i++)
        targets.append(i == m_primarySlot ? static_cast<QWidget*>(m_input)
                                          : m_orderedPanes[pi++]->input());

    // Current slot: whichever pane holds the focus; anything else is primary.
    int cur = m_primarySlot;
    if (QWidget *fw = QApplication::focusWidget()) {
        for (qsizetype i = 0; i < m_orderedPanes.size(); ++i) {
            if (m_orderedPanes[i]->isAncestorOf(fw)) {
                cur = static_cast<int>(i < m_primarySlot ? i : i + 1);
                break;
            }
        }
    }

    const int count = static_cast<int>(targets.size());
    int next = cur + direction;
    if (next < 0) next = count - 1;
    if (next >= count) next = 0;
    targets[next]->setFocus();
}

// ---------------------------------------------------------------------------
// Input dispatch
// ---------------------------------------------------------------------------

void MainWindow::dispatchInput(const QString &text, ServerId host, BufferId channel)
{
    if (text.startsWith('/')) {
        m_dispatcher->dispatch(text, host, channel, m_pendingReplyMsgid);
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

    const QString replyMsgid = m_pendingReplyMsgid;
    clearReplyBar();
    m_model->sendMessage(host, channel, outText, replyMsgid);
}

// ---------------------------------------------------------------------------
// View helpers
// ---------------------------------------------------------------------------

void MainWindow::switchToChannel(ServerId host, BufferId channel)
{
    // Checked out to a floating window — raise it instead of loading in main,
    // and keep the sidebar highlight on what the main view is actually showing.
    const QString key = paneKey(host, channel);
    if (auto *win = m_paneWindows.value(key)) {
        win->show();
        win->raise();
        win->activateWindow();
        if (auto *active = findChannelItem(m_model->activeHost(), m_model->activeChannel()))
            m_sidebar->setCurrentItem(active);
        else
            m_sidebar->clearSelection(); // don't leave the checked-out row highlighted
        return;
    }

    // Already docked in a visible pane — don't also load it into the primary
    // view. Same reasoning as the floating-window case above.
    if (m_panes.contains(key)) {
        if (auto *active = findChannelItem(m_model->activeHost(), m_model->activeChannel()))
            m_sidebar->setCurrentItem(active);
        else
            m_sidebar->clearSelection();
        return;
    }

    // Save scroll position for the channel we're leaving (only if not at bottom)
    if (m_chatView) {
        const QString prevKey = m_model->activeHost().str() + '\t' + m_model->activeChannel().str();
        if (!prevKey.startsWith('\t')) {
            if (!m_chatView->isAtBottom())
                m_scrollPositions[prevKey] = m_chatView->verticalScrollBar()->value();
            else
                m_scrollPositions.remove(prevKey);
        }
    }

    m_primaryPanel->setVisible(true);

    const bool isChannel = isChannelName(channel.str());

    // Nick panel: only meaningful in channels
    if (m_nickPanel) {
        const bool show = isChannel && m_nickExpanded;
        m_nickPanel->setVisible(show);
        if (m_nickRevealBtn)
            m_nickRevealBtn->setVisible(isChannel && !m_nickExpanded);
        setTopicRevealInset(isChannel && !m_nickExpanded);
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
    updateBookmarksMenu();   // "Bookmark This Channel" label tracks the active buffer
}

void MainWindow::openChannelList(ServerId host)
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
            m_channelListDialog, [this, host](ServerId h, BufferId ch, int u, const QString &t) {
        if (h == host)
            m_channelListDialog->addEntry(ch.str(), u, t);
    });
    connect(m_model, &SessionModel::channelListEnd,
            m_channelListDialog, [this, host](ServerId h, int total) {
        if (h == host)
            m_channelListDialog->onListEnd(total);
    });
    connect(m_channelListDialog, &ChannelListDialog::joinRequested,
            this, [this](ServerId h, BufferId channel) {
        m_model->sendRaw(h, "JOIN " + channel.str());
    });
    connect(m_channelListDialog, &ChannelListDialog::refreshRequested,
            this, [this](ServerId h) {
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
ChannelPane *MainWindow::createPane(ServerId host, BufferId channel)
{
    const QString key = paneKey(host, channel);
    if (m_panes.contains(key)) return nullptr;

    auto *pane = new ChannelPane(host, channel, this);
    pane->setNickModel(new NickListModel(m_model, &m_nickStyle));
    if (m_theme.valid)
        pane->chatView()->setColors(QColor(m_theme.text), QColor(m_theme.background),
                                    QColor(m_theme.accent), QColor(m_theme.background),
                                    QColor(m_theme.border));

    pane->input()->installEventFilter(this);
    pane->setTypingEnabled(m_config.ui.typingIndicator);
    pane->setTyping(typingText(pane->host(), pane->channel())); // seed current typers
    {
        const QColor ic(m_theme.valid ? m_theme.text : QStringLiteral("#e3e3e3"));
        pane->setSearchIcon(MenuIcons::fromSvg(QStringLiteral(":/icons/mi-search.svg"), ic, 20));
        pane->setPopOutIcon(MenuIcons::pipEnter(ic));
        pane->setNickPanelIcons(
            MenuIcons::fromSvg(QStringLiteral(":/icons/mi-right-panel-close.svg"), ic, 20),
            MenuIcons::fromSvg(QStringLiteral(":/icons/mi-left-panel-close.svg"), ic, 20),
            MenuIcons::groups(ic, 20));
        if (m_theme.valid)
            pane->setNickChrome(m_config.ui.panelCards ? m_theme.nicklistBg : m_theme.bufferBg,
                                m_config.ui.panelCards);
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
        const QString   &fam = m_config.ui.fontFamily;
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

    connect(pane, &ChannelPane::closeRequested, this, [this, host, channel]{
        closeChannelPane(host, channel);
    });
    connect(pane, &ChannelPane::inputSubmitted, this, [this, host, channel](const QString &text){
        dispatchInput(text, host, channel);
    });
    connect(pane, &ChannelPane::dropReceived, this, [this, pane](const QString &sourceKey){
        ChannelPane *source = m_panes.value(sourceKey);
        ChannelPane *target = pane;
        if (!source || source == target) return;

        const qsizetype fromIdx = m_orderedPanes.indexOf(source);
        const qsizetype toIdx   = m_orderedPanes.indexOf(target);
        if (fromIdx >= 0 && toIdx >= 0) {
            // pane ↔ pane swap
            m_orderedPanes.swapItemsAt(fromIdx, toIdx);
            rebuildPaneLayout();
        }
    });

    m_panes[key] = pane;

    if (m_theme.valid) {
        pane->setTopicIcon(
            MenuIcons::topicBubble(QColor(m_theme.placeholder)),
            MenuIcons::topicBubble(QColor(m_theme.accent)));
    }
    return pane;
}

// Docks a channel as a tiled pane in the main window.
void MainWindow::openChannelPane(ServerId host, BufferId channel)
{
    if (m_orderedPanes.size() >= kMaxExtraPanes) return;
    auto *pane = createPane(std::move(host), std::move(channel));
    if (!pane) return;

    m_orderedPanes.append(pane);
    m_primaryHeader->setVisible(true);
    m_primaryCloseBtn->setVisible(true);

    rebuildPaneLayout();
    refreshPaneChatView(pane);
    refreshPaneNickList(pane);
    m_model->markRead(pane->host(), pane->channel());
}

// Opens a channel in its own floating top-level window. Closing the window
// (or the pane's ✕) drops it back to the server list without leaving the buffer.
void MainWindow::popOutChannel(ServerId host, BufferId channel)
{
    const QString key = paneKey(host, channel);
    // Already a window → just raise it. Already a docked pane → float that one.
    if (auto *win = m_paneWindows.value(key)) {
        win->show(); win->raise(); win->activateWindow();
        return;
    }
    if (m_paneWindows.size() >= kMaxPaneWindows) return;
    if (auto *existing = m_panes.value(key)) { floatPane(existing); return; }

    if (auto *pane = createPane(std::move(host), std::move(channel)))
        floatPane(pane);
}

// Moves a pane (freshly created or currently docked) into its own floating
// top-level window and checks the channel out of the main view.
void MainWindow::floatPane(ChannelPane *pane)
{
    // Cap check must precede the m_orderedPanes removal so a refused
    // docked pane stays docked untouched.
    if (!pane || m_paneWindows.size() >= kMaxPaneWindows) return;
    const QString  key     = pane->key();
    const ServerId host    = pane->host();
    const BufferId channel = pane->channel();

    const bool wasDocked = m_orderedPanes.removeOne(pane);

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
        m_primarySlot = qMin(m_primarySlot, static_cast<int>(m_orderedPanes.size()));
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
    setChannelCheckedOut(host, channel, true);
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
void MainWindow::setChannelCheckedOut(ServerId host, BufferId channel, bool out)
{
    auto *item = findChannelItem(host, channel);
    if (!item) return;
    QFont f = item->font(0);
    f.setItalic(out);
    item->setFont(0, f);
    if (out)
        item->setForeground(0, QColor(m_theme.valid ? m_theme.placeholder
                                                    : QStringLiteral("#6c7086")));
    else
        item->setData(0, Qt::ForegroundRole, QVariant()); // reset to default
}

// Moves the main view off host/channel to another available (non-popped) buffer,
// falling back to the server buffer if that channel was the only one.
void MainWindow::switchAwayFromChannel(ServerId host, BufferId channel)
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
    if (auto *srv = findServerItem(host)) {
        m_sidebar->setCurrentItem(srv);
        onSidebarSelectionChanged();
    }
}

void MainWindow::closeChannelPane(ServerId host, BufferId channel)
{
    const QString key = paneKey(host, channel);
    auto *pane = m_panes.take(key);
    if (!pane) return;

    // Floating pane: tear down its window and return to the server list.
    if (auto *win = m_paneWindows.take(key)) {
        QSettings settings("uplink", "uplink");
        settings.setValue("paneWinGeom/" + key, win->saveGeometry());
        win->removeEventFilter(this);
        win->deleteLater(); // deletes the pane it owns
        setChannelCheckedOut(host, channel, false);
        // Skip the reselect while the server itself is being torn down —
        // no point churning the main view through dying buffers.
        if (m_model->session(host)) {
            if (auto *item = findChannelItem(host, channel)) {
                m_sidebar->setCurrentItem(item);
                switchToChannel(host, channel); // available again → show in main
            }
        }
        return;
    }

    m_orderedPanes.removeOne(pane);
    m_primarySlot = qMin(m_primarySlot, static_cast<int>(m_orderedPanes.size()));
    pane->setParent(nullptr); // detach before rebuild
    pane->deleteLater();

    if (m_orderedPanes.isEmpty()) {
        m_primaryCloseBtn->setVisible(false);
        m_primaryPanel->setVisible(true);
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

    // Collect widgets in display order, inserting primary at m_primarySlot.
    QList<QWidget*> widgets;
    int pi = 0;
    const qsizetype nSlots = 1 + m_orderedPanes.size();
    for (int i = 0; i < nSlots; i++) {
        if (i == m_primarySlot)
            widgets.append(m_primaryPanel);
        else
            widgets.append(m_orderedPanes[pi++]);
    }

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

    // Columns mode (default): primary top-level splitter is horizontal, any
    // stacking within a slot uses a nested vertical splitter. Rows mode is
    // the same shapes transposed 90°.
    const Qt::Orientation mainAxis  = m_config.ui.paneStackRows ? Qt::Vertical : Qt::Horizontal;
    const Qt::Orientation crossAxis = m_config.ui.paneStackRows ? Qt::Horizontal : Qt::Vertical;
    m_panesSplitter->setOrientation(mainAxis);

    auto makeCross = [crossAxis]() -> QSplitter * {
        auto *s = new QSplitter(crossAxis);
        s->setHandleWidth(2);
        return s;
    };

    const qsizetype n = widgets.size();
    if (n <= 2) {
        // 1 or 2 panes: flat along the main axis
        for (auto *w : std::as_const(widgets)) {
            m_panesSplitter->addWidget(w);
            showWidget(w);
        }
    } else if (n == 3) {
        // primary full-length in the first slot, two panes stacked in the second
        m_panesSplitter->addWidget(widgets[0]);
        showWidget(widgets[0]);
        auto *second = makeCross();
        second->addWidget(widgets[1]);
        second->addWidget(widgets[2]);
        showWidget(widgets[1]);
        showWidget(widgets[2]);
        m_panesSplitter->addWidget(second);
    } else { // n == 4  (2×2 grid)
        auto *first = makeCross();
        first->addWidget(widgets[0]);
        first->addWidget(widgets[1]);
        showWidget(widgets[0]);
        showWidget(widgets[1]);
        auto *second = makeCross();
        second->addWidget(widgets[2]);
        second->addWidget(widgets[3]);
        showWidget(widgets[2]);
        showWidget(widgets[3]);
        m_panesSplitter->addWidget(first);
        m_panesSplitter->addWidget(second);
    }

    // Equalize top-level slices along whichever axis is now the main one
    const int total = (mainAxis == Qt::Horizontal) ? m_panesSplitter->width()
                                                    : m_panesSplitter->height();
    if (total > 0 && m_panesSplitter->count() > 0) {
        const int each = total / m_panesSplitter->count();
        QList<int> sizes(m_panesSplitter->count(), each);
        m_panesSplitter->setSizes(sizes);
    }

    // The setParent(nullptr) detach above makes the style engine repolish
    // every pane, which resets programmatic fonts to the app default —
    // re-apply the configured fonts.
    applyFontSizes();
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

void MainWindow::refreshTopicBar(ServerId host, BufferId channel)
{
    auto *ch = m_model->channel(host, channel);

    QString serverName = host.str();
    for (const auto &sc : std::as_const(m_config.servers))
        if (sc.name == host.str() && !sc.name.isEmpty()) { serverName = sc.name; break; }

    if (channel.str() == "(server)") {
        m_topicLabel->clear();
        m_userInfoLabel->setText(serverName);
        if (m_topicText) m_topicText->clear();
        if (m_topicSetByLabel) m_topicSetByLabel->hide();
    } else {
        const QString modes   = ch ? ch->modes : QString();
        const QString modeStr = modes.isEmpty() ? QString() : " (" + modes + ")";
        m_topicLabel->setText(channel.str() + modeStr);
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
                m_topicSetByLabel->setText("Topic set by " + setter.section('!', 0, 0) + " · " + topicAgeStr(ts));
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
            [this](const QString &serverPart, const QString &bufferPart){
        ServerId jumpHost;
        BufferId jumpChannel;
        if (!m_model->resolveLogBuffer(serverPart, bufferPart, jumpHost, jumpChannel))
            return; // logs of a buffer that isn't open — nowhere to jump
        if (auto *item = findChannelItem(jumpHost, jumpChannel)) {
            m_sidebar->setCurrentItem(item);
            onSidebarSelectionChanged();
        }
    });
    dlg->show();
}

void MainWindow::clearReplyBar()
{
    m_pendingReplyMsgid.clear();
    if (m_replyLabel) m_replyLabel->setText({});
    if (m_replyBar)   m_replyBar->hide();
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
        for (const auto &s : m_config.servers) currentNames.insert(s.name);

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

