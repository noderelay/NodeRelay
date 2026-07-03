#if defined(__linux__) && !defined(__MUSL__)
#include <malloc.h>
#endif
#include "mainwindow.h"
#include "ui/commanddispatcher.h"
#include "irc/ircclient.h"
#include "irc/dccsend.h"
#include "irc/dccreceive.h"
#include "ui/trayicon.h"
#include "ui/aboutdialog.h"
#include "ui/channellistdialog.h"
#include "ui/docsdialog.h"
#include "ui/fontdialog.h"
#include "ui/preferencesdialog.h"
#include "ui/serverdialog.h"
#include "ui/manageserversdialog.h"
#include "ui/appicons.h"
#include "ui/themeloader.h"
#include "ui/linkpreview.h"
#include "ui/emojipicker.h"
#include "ui/quickswitcher.h"
#include "ui/updatechecker.h"
#include "ui/emojidata.h"
#include "ui/menuicons.h"
#include "ui/signalbars.h"
#include "ui/fadescrollbar.h"
#include "ui/channelpane.h"
#include "ui/chatrenderer.h"
#include "ui/chatview.h"
#include "config/config.h"
#include "net/addresscheck.h"

#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QToolBar>
#include <QToolButton>
#include <QMenu>
#include <QAction>
#include <QTreeWidget>
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

class NickDelegate : public QStyledItemDelegate {
    QColor  m_accent;
    QColor  m_hover;
    QColor  m_activeText;
    QString m_selfNick;
    bool    m_selfAway{false};
public:
    explicit NickDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent) {}

    void setColors(const QColor &accent, const QColor &hover, const QColor &activeText) {
        m_accent     = accent;
        m_hover      = hover;
        m_activeText = activeText;
    }

    void setSelfAway(const QString &nick, bool away) {
        m_selfNick = nick;
        m_selfAway = away;
    }

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &) const override
    {
        return QSize(option.rect.width(), qMax(16, option.fontMetrics.height() + 2));
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        const QIcon icon       = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        const QIcon ignoreIcon = qvariant_cast<QIcon>(index.data(Qt::UserRole + 1));
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);
        opt.icon = QIcon();
        opt.decorationSize = QSize(0, 0);

        const bool selected = opt.state & QStyle::State_Selected;
        const bool hovered  = opt.state & QStyle::State_MouseOver;

        const QWidget *w    = opt.widget;
        const QStyle  *s    = w ? w->style() : QApplication::style();
        const QRect textRect = s->subElementRect(QStyle::SE_ItemViewItemText, &opt, w);
        const QFontMetrics fm(opt.font);
        const int textW      = fm.horizontalAdvance(opt.text);
        const int textMargin = s->pixelMetric(QStyle::PM_FocusFrameHMargin, &opt, w) + 1;
        constexpr int hPad   = 8;
        constexpr int vPad   = 1;
        constexpr int iconSz = 14;
        constexpr int iconGap = 2;
        const int pillX      = textRect.x() + textMargin - hPad;
        const int iconExtra  = (icon.isNull()       ? 0 : (iconGap + iconSz))
                             + (ignoreIcon.isNull()  ? 0 : (iconGap + iconSz));

        const bool isSelfAway = m_selfAway && !m_selfNick.isEmpty()
            && index.data(Qt::UserRole).toString().compare(m_selfNick, Qt::CaseInsensitive) == 0;

        if (isSelfAway) painter->save(), painter->setOpacity(0.35);

        if (selected || hovered) {
            const QColor bg = selected ? m_accent : m_hover;
            if (bg.isValid()) {
                QRect r(pillX,
                        opt.rect.y() + vPad,
                        qMin(textW + hPad * 2 + iconExtra, opt.rect.right() - pillX),
                        opt.rect.height() - vPad * 2);
                painter->save();
                painter->setRenderHint(QPainter::Antialiasing);
                painter->setPen(Qt::NoPen);
                painter->setBrush(bg);
                painter->drawRoundedRect(r, 6.0, 6.0);
                painter->restore();
            }
        }

        opt.state &= ~(QStyle::State_HasFocus | QStyle::State_MouseOver);
        if (selected && m_accent.isValid()) {
            const QColor textCol = m_accent.lightnessF() > 0.5
                ? QColor("#111111") : QColor("#eeeeee");
            opt.palette.setColor(QPalette::All, QPalette::Highlight,       QColor(Qt::transparent));
            opt.palette.setColor(QPalette::All, QPalette::HighlightedText, textCol);
        }
        QStyledItemDelegate::paint(painter, opt, index);

        int afterIconX = textRect.x() + textMargin + textW;
        if (!icon.isNull()) {
            afterIconX += iconGap;
            QRect r(afterIconX, opt.rect.top() + (opt.rect.height() - iconSz) / 2, iconSz, iconSz);
            icon.paint(painter, r);
            afterIconX += iconSz;
        }
        if (!ignoreIcon.isNull()) {
            afterIconX += iconGap;
            QRect r(afterIconX, opt.rect.top() + (opt.rect.height() - iconSz) / 2, iconSz, iconSz);
            ignoreIcon.paint(painter, r);
        }

        if (isSelfAway) painter->restore();
    }
};

class SidebarDelegate : public QStyledItemDelegate {
    QColor m_accent;
    QColor m_hover;
    QColor m_activeText;
    QColor m_unreadColor;
    bool   m_showCounts{true};
public:
    explicit SidebarDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent) {}

    void setColors(const QColor &accent, const QColor &hover, const QColor &activeText,
                   const QColor &unreadColor = {}) {
        m_accent      = accent;
        m_hover       = hover;
        m_activeText  = activeText;
        m_unreadColor = unreadColor;
    }

    void setShowCounts(bool show) { m_showCounts = show; }

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override
    {
        QSize s = QStyledItemDelegate::sizeHint(option, index);
        s.setHeight(qMax(26, option.fontMetrics.height() + 10));
        return s;
    }


    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        const QIcon indicator  = qvariant_cast<QIcon>(index.data(Qt::UserRole + 2));
        const int   unreadCnt  = m_showCounts ? index.data(Qt::UserRole + 3).toInt() : 0;
        const QString countStr = unreadCnt > 0 ? QString::number(unreadCnt) : QString();
        const QIcon awayIcon   = qvariant_cast<QIcon>(index.data(Qt::UserRole + 4));
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);
        opt.icon = QIcon();
        opt.decorationSize = QSize(0, 0);

        const bool selected = opt.state & QStyle::State_Selected;
        const bool hovered  = opt.state & QStyle::State_MouseOver;

        const QWidget *w    = opt.widget;
        const QStyle  *s    = w ? w->style() : QApplication::style();
        const QRect textRect = s->subElementRect(QStyle::SE_ItemViewItemText, &opt, w);
        const QFontMetrics fm(opt.font);
        QFont countFont = opt.font;
        countFont.setPointSizeF(opt.font.pointSizeF() * 0.86);
        countFont.setBold(true);
        const QFontMetrics cfm(countFont);
        const int textW      = fm.horizontalAdvance(opt.text);
        const int textMargin = s->pixelMetric(QStyle::PM_FocusFrameHMargin, &opt, w) + 1;
        constexpr int hPad   = 8;
        constexpr int vPad   = 2;
        constexpr int iconSz = 14;
        constexpr int iconGap = 2;
        constexpr int countGap = 3;
        const int pillX      = textRect.x() + textMargin - hPad;
        const int iconExtra  = indicator.isNull() ? 0 : (iconGap + iconSz);
        const int countExtra = countStr.isEmpty() ? 0 : (countGap + cfm.horizontalAdvance(countStr));
        const int awayExtra  = awayIcon.isNull()  ? 0 : (iconGap + iconSz);

        if (selected || hovered) {
            const QColor bg = selected ? m_accent : m_hover;
            if (bg.isValid()) {
                QRect r(pillX,
                        opt.rect.y() + vPad,
                        qMin(textW + hPad * 2 + iconExtra + countExtra + awayExtra, opt.rect.right() - pillX),
                        opt.rect.height() - vPad * 2);
                painter->save();
                painter->setRenderHint(QPainter::Antialiasing);
                painter->setPen(Qt::NoPen);
                painter->setBrush(bg);
                painter->drawRoundedRect(r, 10.0, 10.0);
                painter->restore();
            }
        }

        opt.state &= ~(QStyle::State_HasFocus | QStyle::State_MouseOver);
        if (selected && m_accent.isValid()) {
            const QColor textCol = m_accent.lightnessF() > 0.5
                ? QColor("#111111") : QColor("#eeeeee");
            opt.palette.setColor(QPalette::All, QPalette::Highlight,       QColor(Qt::transparent));
            opt.palette.setColor(QPalette::All, QPalette::HighlightedText, textCol);
        }
        QStyledItemDelegate::paint(painter, opt, index);

        int afterTextX = textRect.x() + textMargin + textW;
        if (!indicator.isNull()) {
            const int iconX = afterTextX + iconGap;
            QRect r(iconX,
                    opt.rect.top() + (opt.rect.height() - iconSz) / 2,
                    iconSz, iconSz);
            indicator.paint(painter, r);
            afterTextX = iconX + iconSz;
        }
        if (!countStr.isEmpty()) {
            const QRect cr(afterTextX + countGap,
                           opt.rect.top(),
                           cfm.horizontalAdvance(countStr) + 2,
                           opt.rect.height());
            painter->save();
            painter->setFont(countFont);
            const QColor cc = m_unreadColor.isValid() ? m_unreadColor
                                                       : opt.palette.color(QPalette::Text);
            painter->setPen(cc);
            painter->drawText(cr, Qt::AlignLeft | Qt::AlignVCenter, countStr);
            painter->restore();
            afterTextX += countGap + cfm.horizontalAdvance(countStr) + 2;
        }
        if (!awayIcon.isNull()) {
            const int iconX = afterTextX + iconGap;
            QRect r(iconX,
                    opt.rect.top() + (opt.rect.height() - iconSz) / 2,
                    iconSz, iconSz);
            awayIcon.paint(painter, r);
        }
    }
};

// Minimum width of the topic bar left zone — wide enough to always show the
// hamburger (22) + gear (22) + right margin (4) even when the sidebar is closed.
static constexpr int kDefaultWindowW  = 900;
static constexpr int kDefaultWindowH  = 650;
static constexpr int kInputHistoryCap = 100;
static constexpr int kMaxExtraPanes   = 3;
static constexpr int kRenderWindow    = 150; // messages rendered per channel view
static constexpr int kRenderChunk     = 50;  // messages loaded per scroll-to-top


// Clips all child widgets to a rounded rect via a bitmap mask.
class RoundedPane : public QWidget {
public:
    explicit RoundedPane(QWidget *parent = nullptr) : QWidget(parent) {}
protected:
    void resizeEvent(QResizeEvent *e) override {
        QWidget::resizeEvent(e);
        if (size().isEmpty()) return;
        QBitmap bm(size());
        bm.fill(Qt::color0);
        QPainter p(&bm);
        p.setBrush(Qt::color1);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(rect(), 10, 10);
        setMask(bm);
    }
};

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
    if (!cfg.ui.highlightWords.trimmed().isEmpty()) {
        QStringList parts;
        for (const QString &w : cfg.ui.highlightWords.split(',', Qt::SkipEmptyParts)) {
            const QString t = w.trimmed();
            if (!t.isEmpty()) parts << "\\b" + QRegularExpression::escape(t) + "\\b";
        }
        if (!parts.isEmpty())
            m_highlightRe = QRegularExpression(parts.join('|'), QRegularExpression::CaseInsensitiveOption);
    }

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

    ThemeLoader::apply(m_config.ui.theme);
    m_theme = ThemeLoader::load(m_config.ui.theme);
    setDockOptions(QMainWindow::AllowNestedDocks | QMainWindow::AllowTabbedDocks);

    setupToolbar();
    setupSidebar();
    setupNickPanel();
    setupChatArea();
    setupInputBar();
    connectModel();
    applyFontSizes();

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

    auto *ctrlF = new QShortcut(QKeySequence::Find, this);
    connect(ctrlF, &QShortcut::activated, this, &MainWindow::showSearchBar);

    m_quickSwitcher = new QuickSwitcher(model, this);
    connect(m_quickSwitcher, &QuickSwitcher::channelSelected, this, [this](ServerId host, BufferId channel){
        auto *item = findChannelItem(host, channel);
        if (item) {
            m_sidebar->setCurrentItem(item);
            onSidebarSelectionChanged();
        }
    });
    auto *ctrlK = new QShortcut(QKeySequence("Ctrl+K"), this);
    connect(ctrlK, &QShortcut::activated, m_quickSwitcher, &QuickSwitcher::showCentered);

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
        if (m_primaryHeader && m_sidebarHeader)
            m_sidebarHeader->setFixedHeight(m_primaryHeader->height());
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
        QStringList paneList;
        for (auto *p : std::as_const(m_orderedPanes))
            paneList << p->key();
        s.setValue("panes", paneList);
        s.setValue("primarySlot", m_primarySlot);
    });

#if defined(__linux__) && !defined(__MUSL__)
    auto *trimTimer = new QTimer(this);
    connect(trimTimer, &QTimer::timeout, this, []{ malloc_trim(0); });
    trimTimer->start(60000);
#endif

    m_dispatcher = new CommandDispatcher(m_model, &m_config, this, this);
    connect(m_dispatcher, &CommandDispatcher::switchChannel,  this, &MainWindow::switchToChannel);
    connect(m_dispatcher, &CommandDispatcher::focusInput,     this, [this]{ if (m_input) m_input->setFocus(); });
    connect(m_dispatcher, &CommandDispatcher::clearChat,      this, [this]{
        if (m_chatView) m_chatView->clear();
        auto *ch = m_model->channel(m_model->activeHost(), m_model->activeChannel());
        if (ch) ch->messages.clear();
    });
    connect(m_dispatcher, &CommandDispatcher::openChannelList,this, &MainWindow::openChannelList);
    connect(m_dispatcher, &CommandDispatcher::replyBarCleared, this, &MainWindow::clearReplyBar);
}

MainWindow::~MainWindow() = default;

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
    if (m_primaryHeader && m_sidebarHeader)
        m_sidebarHeader->setFixedHeight(m_primaryHeader->height());
}

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------

void MainWindow::setupToolbar()
{
    auto *tb = addToolBar("Main");
    tb->setMovable(false);
    tb->setFloatable(false);
    tb->setContentsMargins(0, 0, 0, 0);
    if (tb->layout())
        tb->layout()->setContentsMargins(0, 0, 0, 0);
    tb->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(tb, &QWidget::customContextMenuRequested, this, [](const QPoint&){});

    m_hamburger = new QToolButton;
    m_hamburger->setFixedSize(28, 28);
    m_hamburger->setIconSize(QSize(24, 24));
    m_hamburger->setAutoRaise(true);
    m_hamburger->setStyleSheet(
        "QToolButton { background: transparent; border: none; }"
        "QToolButton:hover { background: rgba(255,255,255,0.08); border-radius: 4px; }");
    m_hamburger->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_hamburger, &QWidget::customContextMenuRequested, this, [](const QPoint&){});
    m_hamburger->setObjectName("hamburger");
    tb->hide();

    connect(m_hamburger, &QToolButton::clicked, this, [this]{
        auto *menu = new QMenu(m_hamburger);
        menu->setAttribute(Qt::WA_DeleteOnClose);
        const QColor ic(m_theme.valid ? m_theme.text : "#ffffff");

        menu->addAction(MenuIcons::about(ic), "About Uplink", this, [this]{
            if (!m_aboutDialog) m_aboutDialog = new AboutDialog(this);
            m_aboutDialog->showCentered();
        });

        menu->addAction(MenuIcons::checkForUpdates(ic), "Check for Updates", this, [this]{
            if (!m_updateChecker) m_updateChecker = new UpdateChecker(this);
            m_updateChecker->check();
        });

        menu->addAction(MenuIcons::documentation(ic), "Documentation", this, [this]{
            if (!m_docsDialog)
                m_docsDialog = new DocsDialog(this);
            m_docsDialog->show();
            m_docsDialog->raise();
            m_docsDialog->activateWindow();
        });

        menu->addSeparator();

        menu->addAction(MenuIcons::servers(ic), "Open Config", this, []{
            QDesktopServices::openUrl(QUrl::fromLocalFile(Config::defaultPath()));
        });

        menu->addAction(MenuIcons::connStatus(ic), "Reload Config", this, []{
            QProcess::startDetached(QCoreApplication::applicationFilePath(),
                                    QCoreApplication::arguments());
            QCoreApplication::quit();
        });

        menu->addSeparator();
        menu->addAction(MenuIcons::exit(ic), "Quit Uplink", this, []{ QCoreApplication::quit(); });

        QPoint pos = m_hamburger->mapToGlobal(QPoint(0, m_hamburger->height()));
        menu->exec(pos);
    });
}

static QIcon makeTopicIcon(const QColor &color);
static QIcon makeMenuIcon(const QColor &color);
static QString topicAgeStr(quint64 ts);

void MainWindow::connectPreferences()
{
    connect(m_prefsDialog, &PreferencesDialog::themeChanged, this, [this](const QString &name){
        m_config.ui.theme = name;
        ThemeLoader::apply(name);
        m_theme = ThemeLoader::load(name);
        if (m_chatView && m_theme.valid)
            m_chatView->setColors(QColor(m_theme.text), QColor(m_theme.background),
                                  QColor(m_theme.accent),
                                  QColor(m_theme.background),
                                  QColor(m_theme.border));
        if (m_sidebarDelegate && m_theme.valid)
            m_sidebarDelegate->setColors(QColor(m_theme.accent),
                                         QColor(m_theme.border),
                                         QColor(m_theme.text),
                                         QColor(m_theme.sidebarUnread));
        if (m_nickDelegate && m_theme.valid)
            m_nickDelegate->setColors(QColor(m_theme.accent),
                                      QColor(m_theme.border),
                                      QColor(m_theme.text));
        if (m_theme.valid) {
            if (m_primaryTopicBtn) {
                const bool on = m_primaryTopicBtn->isChecked();
                m_primaryTopicBtn->setIcon(makeTopicIcon(
                    QColor(on ? m_theme.accent : m_theme.placeholder)));
            }
            for (auto *pane : std::as_const(m_panes)) {
                auto *nd = new NickDelegate(pane->nickList());
                nd->setColors(QColor(m_theme.accent),
                              QColor(m_theme.border),
                              QColor(m_theme.text));
                pane->nickList()->setItemDelegate(nd);
                pane->setTopicIcon(
                    makeTopicIcon(QColor(m_theme.placeholder)),
                    makeTopicIcon(QColor(m_theme.accent)));
            }
        }
        if (m_sidebarCloseBtn)
            m_sidebarCloseBtn->setStyleSheet(
                "QToolButton { background: transparent; border: none; }"
                "QToolButton:hover { background: rgba(255,255,255,0.08); border-radius: 4px; }"
            );
        if (m_sidebarRevealBtn)
            m_sidebarRevealBtn->setStyleSheet(
                "QToolButton { background: transparent; border: none; }"
                "QToolButton:hover { background: rgba(255,255,255,0.08); border-radius: 4px; }"
            );
        saveConfig();
    });

    connect(m_prefsDialog, &PreferencesDialog::fontConfigRequested, this, [this]{
        FontDialog dlg(m_config.ui.fontFamily, m_config.ui.fontSizes, this);
        if (dlg.exec() == QDialog::Accepted) {
            m_config.ui.fontFamily = dlg.selectedFamily();
            m_config.ui.fontSizes  = dlg.selectedSizes();
            saveConfig();
            QFont appFont(m_config.ui.fontFamily);
            appFont.setStyleHint(QFont::Monospace);
            QApplication::setFont(appFont);
            applyFontSizes();
        }
    });


    connect(m_prefsDialog, &PreferencesDialog::appIconChanged, this, [this](const QString &key){
        m_config.ui.appIcon = key;
        const QIcon icon = AppIcons::appIcon(key);
        QApplication::setWindowIcon(icon);
        setWindowIcon(icon);
        if (m_tray) m_tray->setBaseIcon(icon);
        const QString iconDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
            + QStringLiteral("/icons/hicolor/256x256/apps");
        QDir().mkpath(iconDir);
        icon.pixmap(256, 256).save(iconDir + QStringLiteral("/uplink-irc.png"));
        QProcess::startDetached(QStringLiteral("gtk-update-icon-cache"),
            {QStringLiteral("-f"), QStringLiteral("-t"),
             QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
                + QStringLiteral("/icons/hicolor/")});
        QProcess::startDetached(QStringLiteral("dbus-send"),
            {QStringLiteral("--session"), QStringLiteral("--type=signal"),
             QStringLiteral("/KIconLoader"),
             QStringLiteral("org.kde.KIconLoader.iconChanged"),
             QStringLiteral("int32:0")});
        saveConfig();
    });

    connect(m_prefsDialog, &PreferencesDialog::topicBarToggled, this, [this](bool on){
        const int scrollPos = m_nickList ? m_nickList->verticalScrollBar()->value() : 0;
        const int sbScroll  = m_sidebar  ? m_sidebar->verticalScrollBar()->value()  : 0;
        m_showTopic = on;
        m_config.ui.showTopic = on;
        m_topicDisplay->setVisible(on);
        if (m_primaryTopicBtn) {
            m_primaryTopicBtn->setChecked(on);
            m_primaryTopicBtn->setText(on ? QStringLiteral("▾") : QStringLiteral("▸"));
        }
        QTimer::singleShot(0, this, [this, scrollPos, sbScroll]{
            if (m_nickList) m_nickList->verticalScrollBar()->setValue(scrollPos);
            if (m_sidebar)  m_sidebar->verticalScrollBar()->setValue(sbScroll);
        });
        saveConfig();
    });

    connect(m_prefsDialog, &PreferencesDialog::nickPrefixToggled, this, [this](bool on){
        m_showNickPrefix = on;
        m_config.ui.showNickPrefix = on;
        m_nickPrefix->setVisible(on);
        for (auto *p : std::as_const(m_orderedPanes))
            p->setNickVisible(on);
        saveConfig();
    });

    connect(m_prefsDialog, &PreferencesDialog::emojiBtnToggled, this, [this](bool on){
        m_showEmojiBtn = on;
        m_config.ui.showEmojiButton = on;
        m_emojiBtn->setVisible(on);
        saveConfig();
    });

    connect(m_prefsDialog, &PreferencesDialog::sendBtnToggled, this, [this](bool on){
        m_config.ui.showSendButton = on;
        m_sendBtn->setVisible(on);
        saveConfig();
    });

    connect(m_prefsDialog, &PreferencesDialog::typingIndicatorToggled, this, [this](bool on){
        m_config.ui.typingIndicator = on;
        saveConfig();
        if (!on) {
            m_typingOutTimer->stop();
            m_typingActive = false;
            m_typingNicks.clear();
            for (auto *t : std::as_const(m_typingNickTimers)) { t->stop(); t->deleteLater(); }
            m_typingNickTimers.clear();
            m_typingLabel->setVisible(false);
        } else {
            m_typingLabel->setText("");
            m_typingLabel->setVisible(true);
        }
    });


    connect(m_prefsDialog, &PreferencesDialog::notificationsToggled, this, [this](bool on){
        m_config.ui.notifications = on;
        saveConfig();
        if (m_tray)
            m_tray->setNotificationsEnabled(on);
    });

    connect(m_prefsDialog, &PreferencesDialog::coloredNicksToggled, this, [this](bool on){
        m_config.ui.coloredNicks = on;
        saveConfig();
        if (!m_model->activeHost().isEmpty() && !m_model->activeChannel().isEmpty())
            refreshNickList(m_model->activeHost(), m_model->activeChannel());
    });

    connect(m_prefsDialog, &PreferencesDialog::hangingIndentToggled, this, [this](bool on){
        m_config.ui.hangingIndent = on;
        saveConfig();
    });

    connect(m_prefsDialog, &PreferencesDialog::loggingToggled, this, [this](bool on){
        m_config.ui.logMessages = on;
        saveConfig();
    });

    connect(m_prefsDialog, &PreferencesDialog::linkPreviewsToggled, this, [this](bool on){
        m_config.ui.linkPreviews = on;
        saveConfig();
    });

    connect(m_prefsDialog, &PreferencesDialog::unreadCountsToggled, this, [this](bool on){
        m_config.ui.showUnreadCounts = on;
        saveConfig();
        m_sidebarDelegate->setShowCounts(on);
        if (!on) {
            // Clear all stored counts from sidebar items
            for (int i = 0; i < m_sidebar->topLevelItemCount(); ++i) {
                auto *srv = m_sidebar->topLevelItem(i);
                for (int j = 0; j < srv->childCount(); ++j)
                    srv->child(j)->setData(0, Qt::UserRole + 3, QVariant());
            }
        }
        m_sidebar->viewport()->update();
    });

    connect(m_prefsDialog, &PreferencesDialog::timestampsToggled, this, [this](bool on){
        m_config.ui.showTimestamps = on;
        saveConfig();
        refreshChatView(m_model->activeHost(), m_model->activeChannel());
    });

    connect(m_prefsDialog, &PreferencesDialog::highlightWordsChanged, this, [this](const QString &words){
        m_config.ui.highlightWords = words;
        saveConfig();
        m_highlightRe = words.trimmed().isEmpty() ? QRegularExpression{}
            : [&]() {
                QStringList parts;
                for (const QString &w : words.split(',', Qt::SkipEmptyParts)) {
                    const QString t = w.trimmed();
                    if (!t.isEmpty()) parts << "\\b" + QRegularExpression::escape(t) + "\\b";
                }
                return parts.isEmpty() ? QRegularExpression{}
                    : QRegularExpression(parts.join('|'), QRegularExpression::CaseInsensitiveOption);
            }();
        m_model->setHighlightWords(words);
    });

    connect(m_prefsDialog, &PreferencesDialog::nickBracketsChanged, this, [this](const QString &br){
        m_config.ui.nickBrackets = br;
        saveConfig();
    });

    connect(m_prefsDialog, &PreferencesDialog::manageServersRequested, this, [this]{
        ManageServersDialog dlg(m_config.servers, this);
        if (dlg.exec() != QDialog::Accepted) return;
        const QList<ServerConfig> updated = dlg.servers();
        for (const ServerConfig &old : m_config.servers) {
            const bool stillPresent = std::any_of(updated.begin(), updated.end(),
                [&](const ServerConfig &s){ return s.name == old.name; });
            if (!stillPresent)
                m_model->removeServer(ServerId{old.name});
        }
        for (const ServerConfig &sc : updated) {
            const ServerConfig *existing = nullptr;
            for (const ServerConfig &old : m_config.servers)
                if (old.name == sc.name) { existing = &old; break; }
            if (!existing) {
                m_model->addServer(sc);
            } else if (*existing != sc) {
                m_model->updateServer(ServerId{existing->name}, sc);
            }
        }
        m_config.servers = updated;
        saveConfig(true);
        syncSidebarOrderFromConfig();
    });

    connect(m_prefsDialog, &PreferencesDialog::aboutRequested, this, [this]{
        if (!m_aboutDialog) m_aboutDialog = new AboutDialog(this);
        m_aboutDialog->showCentered();
    });

    connect(m_prefsDialog, &PreferencesDialog::docsRequested, this, [this]{
        if (!m_docsDialog)
            m_docsDialog = new DocsDialog(this);
        m_docsDialog->show();
        m_docsDialog->raise();
        m_docsDialog->activateWindow();
    });

    connect(m_prefsDialog, &PreferencesDialog::profileSetRequested,
            this, [this](const QString &displayName, const QString &avatarUrl) {
        const QString oldAvatarUrl = m_config.profileAvatarUrl;
        m_config.profileDisplayName = displayName;
        m_config.profileAvatarUrl   = avatarUrl;
        saveConfig();
        // Evict stale cached avatar so the new one is fetched and displayed
        if (!oldAvatarUrl.isEmpty() && oldAvatarUrl != avatarUrl)
            m_avatarCache.remove(oldAvatarUrl);
        QStringList sent, skipped;
        for (const auto &sess : m_model->sessions()) {
            if (!sess.connected) continue;
            // Always update local nickMeta for own nick so tooltip reflects new values
            if (!displayName.isEmpty())
                m_model->onUserMetaChanged(ServerId{sess.name}, sess.nick, "display-name", displayName);
            if (!avatarUrl.isEmpty())
                m_model->onUserMetaChanged(ServerId{sess.name}, sess.nick, "avatar", avatarUrl);
            auto *cl = m_model->clientFor(ServerId{sess.name});
            if (!cl || !cl->hasCap("draft/metadata-2")) {
                skipped << sess.name;
                continue;
            }
            m_model->sendRaw(ServerId{sess.name}, "METADATA * SET display-name :" + displayName);
            const bool localPath = avatarUrl.startsWith('/') || QUrl(avatarUrl).isLocalFile();
            if (!localPath)
                m_model->sendRaw(ServerId{sess.name}, "METADATA * SET avatar :" + avatarUrl);
            sent << sess.name;
        }
        if (!avatarUrl.isEmpty()) fetchAvatar(avatarUrl);
        const ServerId activeHost = m_model->activeHost();
        const BufferId activeChan = m_model->activeChannel();
        if (!sent.isEmpty())
            m_model->localMessage(activeHost, activeChan,
                "Profile sent to: " + sent.join(", "));
        if (!skipped.isEmpty())
            m_model->localMessage(activeHost, activeChan,
                "Skipped (no draft/metadata-2 support): " + skipped.join(", "));
        if (sent.isEmpty() && skipped.isEmpty())
            m_model->localMessage(activeHost, activeChan,
                "No connected servers to send profile to.");
    });

    connect(m_prefsDialog, &PreferencesDialog::scriptsChanged,
            this, [this](const QList<ScriptBinding> &scripts) {
        m_config.scripts = scripts;
        saveConfig();
    });
}


static QIcon makeTopicIcon(const QColor &color)
{
    const int sz = 14;
    QPixmap pix(sz, sz);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    QPen pen(color, 1.3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    // Speech bubble body
    p.drawRoundedRect(QRectF(1.0, 1.0, 11.0, 8.5), 2.0, 2.0);
    // Tail pointing bottom-left
    QPolygonF tail;
    tail << QPointF(2.5, 9.5) << QPointF(1.0, 13.0) << QPointF(5.5, 9.5);
    p.drawPolyline(tail);
    // Two text lines inside the bubble
    p.setPen(QPen(color, 1.0, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(QPointF(3.5, 4.0), QPointF(9.5, 4.0));
    p.drawLine(QPointF(3.5, 6.5), QPointF(7.5, 6.5));
    return QIcon(pix);
}

static QIcon makeGearIcon(int, const QColor &color)
{
    return MenuIcons::gear(color);
}

static QPixmap makeGroupsIcon(const QColor &color, int size = 16)
{
    QSvgRenderer renderer(QStringLiteral(":/icons/mi-groups.svg"));
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    renderer.render(&p);
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.fillRect(pix.rect(), color);
    p.end();
    return pix;
}

static QIcon makeSvgIcon(const QString &svgPath, const QColor &color, int size = 20)
{
    QSvgRenderer renderer(svgPath);
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    renderer.render(&p);
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.fillRect(pix.rect(), color);
    p.end();
    return QIcon(pix);
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
        m_searchBtn->setIcon(makeSvgIcon(
            QStringLiteral(":/icons/mi-search.svg"),
            QColor(m_theme.valid ? m_theme.text : "#e3e3e3")));
    if (m_nickToggleBtn)
        m_nickToggleBtn->setIcon(makeSvgIcon(
            QStringLiteral(":/icons/mi-right-panel-close.svg"),
            QColor(m_theme.valid ? m_theme.text : "#e3e3e3")));
    if (m_nickRevealBtn)
        m_nickRevealBtn->setIcon(makeSvgIcon(
            QStringLiteral(":/icons/mi-left-panel-close.svg"),
            QColor(m_theme.valid ? m_theme.text : "#e3e3e3")));
    if (m_sidebarToggleBtn) {
        m_sidebarToggleBtn->setIcon(makeGearIcon(0, QColor(m_theme.valid ? m_theme.text : "#ffffff")));
        m_sidebarToggleBtn->setStyleSheet(
            "QToolButton { background: transparent; border: none; }"
            "QToolButton:hover { background: rgba(255,255,255,0.08); border-radius: 4px; }");
    }
    if (m_sidebarCloseBtn)
        m_sidebarCloseBtn->setIcon(makeSvgIcon(
            QStringLiteral(":/icons/mi-left-panel-close.svg"),
            QColor(m_theme.valid ? m_theme.text : "#e3e3e3")));
    if (m_sidebarRevealBtn)
        m_sidebarRevealBtn->setIcon(makeSvgIcon(
            QStringLiteral(":/icons/mi-right-panel-open.svg"),
            QColor(m_theme.valid ? m_theme.text : "#e3e3e3")));
    if (m_nickGroupsIconLabel)
        m_nickGroupsIconLabel->setPixmap(
            makeGroupsIcon(QColor(m_theme.valid ? m_theme.text : "#e3e3e3"), 20));
    if (m_hamburger) {
        m_hamburger->setIcon(makeMenuIcon(QColor(m_theme.valid ? m_theme.text : "#ffffff")));
        m_hamburger->setStyleSheet(
            "QToolButton { background: transparent; border: none; }"
            "QToolButton:hover { background: rgba(255,255,255,0.08); border-radius: 4px; }");
    }
    if (m_serversBtn)
        m_serversBtn->setIcon(MenuIcons::manageServers(QColor(m_theme.valid ? m_theme.text : "#ffffff")));
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
    for (auto *p : std::as_const(m_orderedPanes)) {
        const QFont chatFont = makeFont(fs.chat);
        p->chatView()->setFont(chatFont);
        p->nickList()->setFont(makeFont(fs.nickList));
        p->setTopicFont(makeFont(fs.topicText));
        p->setInputFont(makeFont(fs.inputNick), makeFont(fs.input));
    }
    if (m_typingLabel) {
        QFont f = makeFont(fs.typing);
        f.setItalic(true);
        m_typingLabel->setFont(f);
    }
}

static QIcon makeMenuIcon(const QColor &color)
{
    return MenuIcons::hamburger(color);
}

static QIcon makeConnectedIcon()
{
    return MenuIcons::connectedServer();
}

void MainWindow::setupSidebar()
{
    m_sidebar = new QTreeWidget;
    m_sidebar->setVerticalScrollBar(new FadeScrollBar(Qt::Vertical, m_sidebar));
    m_sidebar->setHeaderHidden(true);
    m_sidebar->setRootIsDecorated(false);
    m_sidebar->setItemsExpandable(false);
    m_sidebar->setIndentation(8);
    m_sidebar->setMinimumWidth(112);
    m_sidebar->setObjectName("sidebar");
    m_sidebar->viewport()->installEventFilter(this);
    m_sidebarDelegate = new SidebarDelegate(m_sidebar);
    m_sidebarDelegate->setShowCounts(m_config.ui.showUnreadCounts);
    if (m_theme.valid)
        m_sidebarDelegate->setColors(QColor(m_theme.accent),
                                     QColor(m_theme.border),
                                     QColor(m_theme.text),
                                     QColor(m_theme.sidebarUnread));
    m_sidebar->setItemDelegate(m_sidebarDelegate);
    QScroller::grabGesture(m_sidebar->viewport(), QScroller::LeftMouseButtonGesture);

    connect(m_sidebar, &QTreeWidget::itemClicked,
            this, [this](QTreeWidgetItem *, int){ onSidebarSelectionChanged(); });
    m_sidebar->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_sidebar, &QTreeWidget::customContextMenuRequested,
            this, &MainWindow::onSidebarContextMenu);

    m_sidebarToggleBtn = new QToolButton;
    m_sidebarToggleBtn->setFixedSize(28, 28);
    m_sidebarToggleBtn->setIconSize(QSize(20, 20));
    m_sidebarToggleBtn->setAutoRaise(true);
    m_sidebarToggleBtn->setStyleSheet(
        "QToolButton { background: transparent; border: none; }"
        "QToolButton:hover { background: rgba(255,255,255,0.08); border-radius: 4px; }");
    m_sidebarToggleBtn->setObjectName("sidebarToggleBtn");
    m_sidebarToggleBtn->setToolTip(tr("Preferences"));
    m_sidebarToggleBtn->setIcon(makeGearIcon(0, QColor(m_theme.valid ? m_theme.text : "#ffffff")));
    connect(m_sidebarToggleBtn, &QToolButton::clicked, this, [this]{
        if (!m_prefsDialog) {
            m_prefsDialog = new PreferencesDialog(m_config, this);
            connectPreferences();
        }
        m_prefsDialog->show();
        m_prefsDialog->raise();
        m_prefsDialog->activateWindow();
    });

    m_sidebarPanel = new QWidget;
    m_sidebarPanel->setObjectName("sidebarPanel");
    auto *vbox = new QVBoxLayout(m_sidebarPanel);
    vbox->setContentsMargins(0, 0, 0, 0);
    vbox->setSpacing(0);

    m_sidebarHeader = new QWidget;
    m_sidebarHeader->setObjectName("sidebarHeader");
    m_sidebarHeader->setFixedHeight(34); // synced to primaryHeader+topicDisplay after show

    auto positionSidebarRevealBtn = [this]{
        if (!m_sidebarRevealBtn || !m_chatSection) return;
        m_sidebarRevealBtn->move(4, m_chatSection->height() - m_sidebarRevealBtn->height() - 4);
        m_sidebarRevealBtn->raise();
    };

    {
        auto *shBox = new QHBoxLayout(m_sidebarHeader);
        shBox->setContentsMargins(2, 2, 2, 2);
        shBox->setSpacing(2);
        shBox->setAlignment(Qt::AlignTop);
        shBox->addWidget(m_hamburger);
        shBox->addWidget(m_sidebarToggleBtn);

        m_serversBtn = new QToolButton;
        m_serversBtn->setFixedSize(32, 32);
        m_serversBtn->setIconSize(QSize(28, 28));
        m_serversBtn->setAutoRaise(true);
        m_serversBtn->setStyleSheet(
            "QToolButton { background: transparent; border: none; }"
            "QToolButton:hover { background: rgba(255,255,255,0.08); border-radius: 4px; }");
        m_serversBtn->setToolTip(tr("Add / Manage Servers"));
        m_serversBtn->setIcon(MenuIcons::manageServers(QColor(m_theme.valid ? m_theme.text : "#ffffff")));
        connect(m_serversBtn, &QToolButton::clicked, this, [this]{
            ManageServersDialog dlg(m_config.servers, this);
            if (dlg.exec() != QDialog::Accepted) return;
            const QList<ServerConfig> updated = dlg.servers();
            for (const ServerConfig &old : m_config.servers) {
                const bool stillPresent = std::any_of(updated.begin(), updated.end(),
                    [&](const ServerConfig &s){ return s.name == old.name; });
                if (!stillPresent)
                    m_model->removeServer(ServerId{old.name});
            }
            for (const ServerConfig &sc : updated) {
                const ServerConfig *existing = nullptr;
                for (const ServerConfig &old : m_config.servers)
                    if (old.name == sc.name) { existing = &old; break; }
                if (!existing) {
                    m_model->addServer(sc);
                } else if (*existing != sc) {
                    m_model->updateServer(ServerId{existing->name}, sc);
                }
            }
            m_config.servers = updated;
            saveConfig(true);
            syncSidebarOrderFromConfig();
        });
        shBox->addWidget(m_serversBtn);

        shBox->addStretch(1);
        m_signalBars = new SignalBars(m_sidebarHeader);
        shBox->addWidget(m_signalBars, 0, Qt::AlignVCenter);
    }

    vbox->addWidget(m_sidebarHeader);
    vbox->addWidget(m_sidebar, 1);

    // Floating close button pinned to the bottom-left corner of the sidebar panel
    m_sidebarCloseBtn = new QToolButton(m_sidebarPanel);
    m_sidebarCloseBtn->setFixedSize(28, 28);
    m_sidebarCloseBtn->setIconSize(QSize(20, 20));
    m_sidebarCloseBtn->setAutoRaise(true);
    m_sidebarCloseBtn->setStyleSheet(
        "QToolButton { background: transparent; border: none; }"
        "QToolButton:hover { background: rgba(255,255,255,0.08); border-radius: 4px; }"
    );
    m_sidebarCloseBtn->setToolTip(tr("Hide channel list"));
    m_sidebarCloseBtn->setIcon(makeSvgIcon(
        QStringLiteral(":/icons/mi-left-panel-close.svg"),
        QColor(m_theme.valid ? m_theme.text : "#e3e3e3")));
    m_sidebarCloseBtn->move(4, 0);
    m_sidebarCloseBtn->raise();
    connect(m_sidebarCloseBtn, &QToolButton::clicked, this, [this, positionSidebarRevealBtn]{
        m_sidebarExpanded = false;
        m_sidebarPanel->setVisible(false);
        if (m_inputBar)
            if (auto *l = qobject_cast<QHBoxLayout*>(m_inputBar->layout()))
                l->setContentsMargins(40, 3, 4, 8);
        if (m_sidebarRevealBtn) {
            positionSidebarRevealBtn();
            m_sidebarRevealBtn->setVisible(true);
        }
    });
    m_sidebarPanel->installEventFilter(this);
}

void MainWindow::setupNickPanel()
{
    m_nickList = new QListWidget;
    m_nickList->setVerticalScrollBar(new FadeScrollBar(Qt::Vertical, m_nickList));
    m_nickList->viewport()->installEventFilter(this);
    m_nickList->setSpacing(0);
    m_nickList->setIconSize(QSize(16, 16));
    m_nickList->setUniformItemSizes(true);
    QScroller::grabGesture(m_nickList->viewport(), QScroller::LeftMouseButtonGesture);
    m_nickDelegate = new NickDelegate(m_nickList);
    if (m_theme.valid)
        m_nickDelegate->setColors(QColor(m_theme.accent),
                                  QColor(m_theme.border),
                                  QColor(m_theme.text));
    m_nickList->setItemDelegate(m_nickDelegate);
    m_nickList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_nickList, &QListWidget::customContextMenuRequested,
            this, &MainWindow::onNickListContextMenu);

    m_nickGroupsIconLabel = new QLabel;
    m_nickGroupsIconLabel->setObjectName("nickGroupsIcon");
    m_nickGroupsIconLabel->setContentsMargins(4, 0, 2, 0);
    m_nickGroupsIconLabel->setPixmap(makeGroupsIcon(QColor(m_theme.valid ? m_theme.text : "#e3e3e3"), 20));
    m_nickGroupsIconLabel->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);

    m_nickCountLabel = new QLabel(QStringLiteral("0"));
    m_nickCountLabel->setObjectName("nickCountLabel");
    m_nickCountLabel->setContentsMargins(0, 0, 4, 0);
    m_nickCountLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

    m_nickToggleBtn = new QToolButton;
    m_nickToggleBtn->setFixedSize(28, 28);
    m_nickToggleBtn->setIconSize(QSize(20, 20));
    m_nickToggleBtn->setAutoRaise(true);
    m_nickToggleBtn->setToolTip(tr("Hide user list"));
    m_nickToggleBtn->setIcon(makeSvgIcon(
        QStringLiteral(":/icons/mi-right-panel-close.svg"),
        QColor(m_theme.valid ? m_theme.text : "#e3e3e3")));

    auto positionRevealBtn = [this]{
        if (!m_nickRevealBtn || !m_chatSection) return;
        const int topY = m_primaryHeader->height()
                       + (m_topicDisplay && m_topicDisplay->isVisible() ? m_topicDisplay->height() : 0)
                       + 4;
        m_nickRevealBtn->move(m_chatSection->width() - m_nickRevealBtn->width() - 4, topY);
        m_nickRevealBtn->raise();
    };

    auto toggleNickPanel = [this, positionRevealBtn]{
        m_nickExpanded = !m_nickExpanded;
        m_nickPanel->setVisible(m_nickExpanded);
        if (m_nickRevealBtn) {
            if (!m_nickExpanded) positionRevealBtn();
            m_nickRevealBtn->setVisible(!m_nickExpanded);
        }
    };

    connect(m_nickToggleBtn, &QToolButton::clicked, this, toggleNickPanel);

    m_userInfoLabel = new QLabel;
    m_userInfoLabel->setObjectName("userInfoLabel");
    m_userInfoLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

    m_nickPanelHeader = new QWidget;
    auto *header = m_nickPanelHeader;
    header->setObjectName("nickPanelHeader");
    auto *hbox = new QHBoxLayout(header);
    hbox->setContentsMargins(2, 0, 2, 0);
    hbox->setSpacing(2);
    hbox->addWidget(m_nickToggleBtn);
    hbox->addWidget(m_nickGroupsIconLabel);
    hbox->addSpacing(2);
    hbox->addWidget(m_nickCountLabel);
    hbox->addWidget(m_userInfoLabel, 1);

    m_nickFilter = new QLineEdit;
    m_nickFilter->setObjectName("nickFilter");
    m_nickFilter->setPlaceholderText("filter users…");
    m_nickFilter->setClearButtonEnabled(true);
    m_nickFilter->installEventFilter(this);
    connect(m_nickFilter, &QLineEdit::textChanged, this, [this](const QString &text) {
        const QString lower = text.toLower();
        for (int i = 0; i < m_nickList->count(); ++i) {
            auto *item = m_nickList->item(i);
            const QString nick = item->data(Qt::UserRole).toString().toLower();
            item->setHidden(!lower.isEmpty() && !nick.startsWith(lower));
        }
    });

    m_nickPanel = new QWidget;
    m_nickPanel->setObjectName("nickPanel");
    m_nickPanel->setMinimumWidth(24);
    auto *vbox = new QVBoxLayout(m_nickPanel);
    vbox->setContentsMargins(0, 0, 0, 0);
    vbox->setSpacing(0);
    vbox->addWidget(m_nickPanelHeader);
    vbox->addWidget(m_nickFilter);
    vbox->addWidget(m_nickList, 100);
    vbox->addStretch(1);
}

void MainWindow::setupChatArea()
{
    // Right content — holds the panes splitter only
    m_rightContent = new QWidget;
    m_rightContent->setObjectName("rightContent");
    auto *vbox     = new QVBoxLayout(m_rightContent);
    vbox->setContentsMargins(8, 8, 8, 8);
    vbox->setSpacing(0);

    // Primary panel — first column in the panes splitter
    m_primaryPanel = new QWidget;
    auto *primaryVbox = new QVBoxLayout(m_primaryPanel);
    primaryVbox->setContentsMargins(0, 0, 0, 0);
    primaryVbox->setSpacing(0);

    // Primary panel header: hidden until the first extra pane is opened
    m_primaryHeader = new QWidget;
    m_primaryHeader->setObjectName("paneHeader");
    m_primaryHeader->setVisible(true);
    auto *primaryHeader = m_primaryHeader;
    {
        auto *hbox = new QHBoxLayout(primaryHeader);
        hbox->setContentsMargins(6, 3, 4, 3);
        hbox->setSpacing(6);

        m_primaryTopicBtn = new QToolButton;
        m_primaryTopicBtn->setObjectName("topicToggle");
        m_primaryTopicBtn->setCheckable(true);
        m_primaryTopicBtn->setChecked(m_showTopic);
        m_primaryTopicBtn->setText({});
        m_primaryTopicBtn->setIcon(makeTopicIcon(
            QColor(m_theme.valid ? m_theme.placeholder : "#888888")));
        m_primaryTopicBtn->setIconSize(QSize(14, 14));
        m_primaryTopicBtn->setAutoRaise(false);
        connect(m_primaryTopicBtn, &QToolButton::toggled, this, [this](bool on){
            const int scrollPos = m_nickList ? m_nickList->verticalScrollBar()->value() : 0;
            const int sbScroll  = m_sidebar  ? m_sidebar->verticalScrollBar()->value()  : 0;
            m_topicDisplay->setVisible(on);
            if (m_theme.valid)
                m_primaryTopicBtn->setIcon(makeTopicIcon(
                    QColor(on ? m_theme.accent : m_theme.placeholder)));
            QTimer::singleShot(0, this, [this, scrollPos, sbScroll]{
                if (m_nickList) m_nickList->verticalScrollBar()->setValue(scrollPos);
                if (m_sidebar)  m_sidebar->verticalScrollBar()->setValue(sbScroll);
            });
        });

        m_primaryCloseBtn = new QToolButton;
        m_primaryCloseBtn->setText(QStringLiteral("✕"));
        m_primaryCloseBtn->setFixedSize(16, 16);
        m_primaryCloseBtn->setStyleSheet(
            "QToolButton { background: transparent; border: none; padding: 0px; }"
            "QToolButton:hover { color: palette(highlight); }"
        );
        m_primaryCloseBtn->setVisible(false);
        connect(m_primaryCloseBtn, &QToolButton::clicked, this, [this]{
            m_primaryPanel->hide();
        });

        m_searchBtn = new QToolButton;
        m_searchBtn->setFixedSize(28, 28);
        m_searchBtn->setIconSize(QSize(24, 24));
        m_searchBtn->setAutoRaise(true);
        m_searchBtn->setStyleSheet(
            "QToolButton { background: transparent; border: none; }"
            "QToolButton:hover { background: rgba(255,255,255,0.08); border-radius: 4px; }"
        );
        m_searchBtn->setToolTip(tr("Search (Ctrl+F)"));
        m_searchBtn->setIcon(makeSvgIcon(
            QStringLiteral(":/icons/mi-search.svg"),
            QColor(m_theme.valid ? m_theme.text : "#e3e3e3")));
        connect(m_searchBtn, &QToolButton::clicked, this, [this]{
            if (m_searchBar->isVisible()) { m_searchBar->hide(); m_searchInput->clear(); }
            else showSearchBar();
        });

        m_topicLabel = new QLabel;
        m_topicLabel->setObjectName("channelLabel");

        m_topicSetByLabel = new QLabel;
        m_topicSetByLabel->setObjectName("topicSetByLabel");
        m_topicSetByLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
        m_topicSetByLabel->setStyleSheet(
            QString("QLabel { color: %1; }").arg(m_theme.valid ? m_theme.placeholder : "#888888"));
        m_topicSetByLabel->hide();

        hbox->addWidget(m_primaryTopicBtn);
        hbox->addWidget(m_topicLabel);
        hbox->addSpacing(10);
        hbox->addWidget(m_topicSetByLabel);
        hbox->addStretch(1);
        hbox->addWidget(m_searchBtn);
        hbox->addWidget(m_primaryCloseBtn);
    }
    // Topic display — shown below header when Show Topic is on
    m_topicDisplay = new QWidget;
    auto *tdHbox   = new QHBoxLayout(m_topicDisplay);
    tdHbox->setContentsMargins(8, 3, 8, 3);
    m_topicText = new QLabel;
    m_topicText->setObjectName("topicText");
    m_topicText->setWordWrap(true);
    m_topicText->setTextFormat(Qt::RichText);
    m_topicText->setTextInteractionFlags(Qt::TextBrowserInteraction);
    m_topicText->setOpenExternalLinks(false);
    connect(m_topicText, &QLabel::linkActivated, this, [](const QString &link){
        const QUrl u(link);
        const QString s = u.scheme().toLower();
        if (s == "http" || s == "https")
            QDesktopServices::openUrl(u);
    });
    tdHbox->addWidget(m_topicText, 1);
    m_topicDisplay->setObjectName("topicDisplay");
    m_topicDisplay->setVisible(m_showTopic);
    m_topicText->installEventFilter(this);
    m_topicLabel->installEventFilter(this);
    m_topicDisplay->installEventFilter(this);
    m_primaryHeader->installEventFilter(this);

    m_chatSection     = new QWidget;
    auto *chatSection = m_chatSection;
    auto *chatVbox    = new QVBoxLayout(chatSection);
    chatVbox->setContentsMargins(0, 0, 0, 0);
    chatVbox->setSpacing(0);

    // Chat view
    m_chatView = new ChatView;
    if (m_theme.valid)
        m_chatView->setColors(QColor(m_theme.text), QColor(m_theme.background),
                              QColor(m_theme.accent), QColor(m_theme.background),
                              QColor(m_theme.border));
    connect(m_chatView, &ChatView::anchorActivated,
            this, [this](const QString &anchor, const QPoint &gp, Qt::MouseButton btn){
        if (btn == Qt::LeftButton) {
            if (anchor.startsWith(QLatin1String("evgrp:"))) {
                toggleEventGroupInView(m_chatView, anchor.mid(6),
                                       m_model->activeHost(), m_model->activeChannel());
                return;
            }
            if (anchor.startsWith(QLatin1String("nick:"))) {
                const QString nick = anchor.mid(5);
                m_input->setFocus();
                const QString cur = m_input->toPlainText();
                m_input->setPlainText(cur.isEmpty() ? nick + ": " : cur + nick + " ");
                QTextCursor c = m_input->textCursor();
                c.movePosition(QTextCursor::End);
                m_input->setTextCursor(c);
                return;
            }
            QString href = anchor;
            if (href.startsWith("url:"))  href = href.mid(4);
            if (href.startsWith("preview:")) href = href.mid(8);
            const QUrl u(href);
            const QString s = u.scheme().toLower();
            if (s == "http" || s == "https") QDesktopServices::openUrl(u);
        } else if (btn == Qt::RightButton) {
            handleChatViewContextMenu(m_chatView, anchor, gp,
                                      m_model->activeHost(), m_model->activeChannel());
        }
    });
    connect(m_chatView, &ChatView::anchorHovered, this, [this](const QString &anchor){
        if (anchor.isEmpty()) {
            if (!m_hoveredUrl.isEmpty()) {
                m_hoveredUrl.clear();
                QToolTip::hideText();
                statusBar()->clearMessage();
            }
            return;
        }
        if (anchor.startsWith("nick:")) {
            if (m_hoveredUrl == anchor) return;
            m_hoveredUrl = anchor;
            const QString nick = anchor.mid(5);
            const QString tip = nickTooltip(nick, m_model->activeHost());
            if (!tip.isEmpty()) {
                m_hoverGlobalPos = QCursor::pos();
                QToolTip::showText(m_hoverGlobalPos, tip, m_chatView->viewport());
            }
            return;
        }
        QString href = anchor;
        if (href.startsWith("url:"))     href = href.mid(4);
        if (href.startsWith("preview:")) href = href.mid(8);
        if (href == m_hoveredUrl) return;
        m_hoveredUrl = href;
        const QUrl url(href);
        statusBar()->showMessage(url.host());
        m_hoverGlobalPos = QCursor::pos();
        QToolTip::showText(m_hoverGlobalPos, url.host(), m_chatView->viewport());
        if (m_config.ui.linkPreviews) m_linkPreview->fetchHover(url);
    });
    connect(m_chatView, &ChatView::loadOlderRequested, this, &MainWindow::loadOlderMessages);
    m_chatView->installEventFilter(this);
    m_chatView->viewport()->installEventFilter(this);

    m_linkPreview = new LinkPreview(this);

    m_previewWatchdog = new QTimer(this);
    m_previewWatchdog->setSingleShot(true);
    connect(m_previewWatchdog, &QTimer::timeout, this, [this]{
        m_previewFetchBusy = false;
        processPreviewQueue();
    });

    connect(m_linkPreview, &LinkPreview::titleReady, this, [this](const QUrl &url, const QString &title){
        if (url.toString() != m_hoveredUrl) return;
        const QString display = title.length() > 80 ? title.left(79) + QChar(0x2026) : title;
        statusBar()->showMessage(display);
        QToolTip::showText(m_hoverGlobalPos, display, m_chatView->viewport());
    });

    connect(m_linkPreview, &LinkPreview::cardReady, this,
            [this](const QUrl &pageUrl, const QString &title, const QPixmap &thumbnail){
        const QString urlStr = pageUrl.toString();
        m_previewWatchdog->stop();
        m_previewFetchBusy = false;

        auto it = m_previewChannels.find(urlStr);
        if (it == m_previewChannels.end()) {
            processPreviewQueue();
            return;
        }
        const ServerId host    = it->host;
        const BufferId channel = it->channel;
        const QString msgid    = it->msgid;
        m_previewChannels.erase(it);
        processPreviewQueue();

        auto *ch = m_model->channel(host, channel);
        if (!ch) return;

        // Thumbnail scaled to max 100 px wide
        QPixmap thumb;
        if (!thumbnail.isNull())
            thumb = thumbnail.scaledToWidth(qMin(thumbnail.width(), 240),
                                            Qt::SmoothTransformation);

        Channel::PreviewCard card;
        card.title   = title.left(120);
        card.domain  = pageUrl.host();
        card.pageUrl = urlStr;
        if (!thumb.isNull()) {
            QBuffer pngBuf(&card.pngData);
            pngBuf.open(QIODevice::WriteOnly);
            thumb.save(&pngBuf, "PNG");
        }
        ch->addPreview(urlStr, card);

        auto makeCardLine = [&]() -> ChatLine {
            ChatLine line;
            line.id   = "preview:" + urlStr;
            line.role = ChatLineRole::PreviewCard;
            line.image = thumb;
            line.text = card.title + "\n" + card.domain;
            QTextCharFormat titleFmt;
            titleFmt.setFontWeight(QFont::Bold);
            ChatSegment titleSeg;
            titleSeg.start  = 0;
            titleSeg.length = static_cast<int>(card.title.size());
            titleSeg.format = titleFmt;
            titleSeg.anchor = "preview:" + urlStr;
            line.segments.append(titleSeg);
            QTextCharFormat domainFmt;
            domainFmt.setForeground(QColor("#888888"));
            ChatSegment domainSeg;
            domainSeg.start  = static_cast<int>(card.title.size()) + 1;
            domainSeg.length = static_cast<int>(card.domain.size());
            domainSeg.format = domainFmt;
            line.segments.append(domainSeg);
            return line;
        };

        const bool isActive = (host == m_model->activeHost() &&
                               channel.str().toLower() == m_model->activeChannel().str().toLower());
        if (isActive) {
            const bool atBottom = m_chatView->isAtBottom();
            if (!msgid.isEmpty() && m_chatView->findLine(msgid) >= 0)
                m_chatView->insertAfter(msgid, makeCardLine());
            else
                m_chatView->appendLine(makeCardLine());
            if (atBottom) m_chatView->scrollToBottom();
        }

        const QString paneKey = host.str() + "|" + channel.str().toLower();
        if (auto *pane = m_panes.value(paneKey)) {
            const bool atBottom = pane->chatView()->isAtBottom();
            ChatView *cv = pane->chatView();
            if (!msgid.isEmpty() && cv->findLine(msgid) >= 0)
                cv->insertAfter(msgid, makeCardLine());
            else
                cv->appendLine(makeCardLine());
            if (atBottom) pane->chatView()->scrollToBottom();
        }
    });

    // Header row: primaryHeader only — nickPanelHeader lives inside nickPanel now
    auto *headerRow = new QWidget;
    auto *hrBox = new QHBoxLayout(headerRow);
    hrBox->setContentsMargins(0, 0, 0, 0);
    hrBox->setSpacing(0);
    hrBox->addWidget(primaryHeader, 1);
    chatVbox->addWidget(headerRow);

    auto *chatLeft = new QWidget;
    auto *chatLeftVbox = new QVBoxLayout(chatLeft);
    chatLeftVbox->setContentsMargins(0, 0, 0, 0);
    chatLeftVbox->setSpacing(0);
    chatLeftVbox->addWidget(m_topicDisplay);
    chatLeftVbox->addWidget(m_chatView, 1);

    m_chatSplitter = new QSplitter(Qt::Horizontal);
    m_chatSplitter->setHandleWidth(0);
    m_chatSplitter->addWidget(chatLeft);
    m_chatSplitter->addWidget(m_nickPanel);
    m_chatSplitter->setStretchFactor(0, 1);
    m_chatSplitter->setStretchFactor(1, 0);

    chatVbox->addWidget(m_chatSplitter, 1);

    // Floating reveal button — child of chatSection (plain QWidget, not splitter)
    // so Qt doesn't treat it as a splitter pane.
    m_nickRevealBtn = new QToolButton(m_chatSection);
    m_nickRevealBtn->setFixedSize(28, 28);
    m_nickRevealBtn->setIconSize(QSize(20, 20));
    m_nickRevealBtn->setAutoRaise(true);
    m_nickRevealBtn->setStyleSheet(
        "QToolButton { background: transparent; border: none; }"
        "QToolButton:hover { background: rgba(255,255,255,0.08); border-radius: 4px; }"
    );
    m_nickRevealBtn->setToolTip(tr("Show user list"));
    m_nickRevealBtn->setIcon(makeSvgIcon(
        QStringLiteral(":/icons/mi-left-panel-close.svg"),
        QColor(m_theme.valid ? m_theme.text : "#e3e3e3")));
    m_nickRevealBtn->setVisible(false);
    m_nickRevealBtn->raise();
    connect(m_nickRevealBtn, &QToolButton::clicked, this, [this]{
        m_nickExpanded = true;
        m_nickPanel->setVisible(true);
        m_nickRevealBtn->setVisible(false);
    });
    m_scrollBottomBtn = new QToolButton(m_chatView->viewport());
    m_scrollBottomBtn->setFixedSize(32, 32);
    m_scrollBottomBtn->setIconSize(QSize(22, 22));
    m_scrollBottomBtn->setAutoRaise(true);
    m_scrollBottomBtn->setCursor(Qt::PointingHandCursor);
    m_scrollBottomBtn->setStyleSheet(
        "QToolButton { background: rgba(0,0,0,0.5); border: none; border-radius: 16px; }"
        "QToolButton:hover { background: rgba(0,0,0,0.7); }"
    );
    m_scrollBottomBtn->setToolTip(tr("Jump to bottom"));
    m_scrollBottomBtn->setIcon(makeSvgIcon(
        QStringLiteral(":/icons/mi-keyboard-double-arrow-down.svg"),
        QColor("#e3e3e3")));
    m_scrollBottomBtn->setVisible(false);
    m_scrollBottomOpacity = new QGraphicsOpacityEffect(m_scrollBottomBtn);
    m_scrollBottomOpacity->setOpacity(0.0);
    m_scrollBottomBtn->setGraphicsEffect(m_scrollBottomOpacity);
    m_scrollBottomAnim = new QPropertyAnimation(m_scrollBottomOpacity, "opacity", this);
    m_scrollBottomAnim->setDuration(300);
    m_scrollBottomAnim->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_scrollBottomBtn, &QToolButton::clicked, this, [this]{
        m_chatView->scrollToBottom();
    });
    connect(m_chatView, &ChatView::scrolledAwayFromBottom, this, [this](bool away){
        auto *vp = m_chatView->viewport();
        m_scrollBottomBtn->move(
            vp->width() - m_scrollBottomBtn->width() - 12,
            vp->height() - m_scrollBottomBtn->height() - 12);
        m_scrollBottomBtn->raise();
        m_scrollBottomAnim->stop();
        if (away) {
            m_scrollBottomBtn->setVisible(true);
            m_scrollBottomAnim->setStartValue(m_scrollBottomOpacity->opacity());
            m_scrollBottomAnim->setEndValue(0.85);
            m_scrollBottomAnim->start();
        } else {
            m_scrollBottomAnim->setStartValue(m_scrollBottomOpacity->opacity());
            m_scrollBottomAnim->setEndValue(0.0);
            m_scrollBottomAnim->start();
            connect(m_scrollBottomAnim, &QPropertyAnimation::finished, this, [this]{
                if (m_scrollBottomOpacity->opacity() < 0.01)
                    m_scrollBottomBtn->setVisible(false);
            }, Qt::SingleShotConnection);
        }
    });

    m_chatSection->installEventFilter(this);

    m_sidebarRevealBtn = new QToolButton(m_chatSection);
    m_sidebarRevealBtn->setFixedSize(28, 28);
    m_sidebarRevealBtn->setIconSize(QSize(20, 20));
    m_sidebarRevealBtn->setAutoRaise(true);
    m_sidebarRevealBtn->setStyleSheet(
        "QToolButton { background: transparent; border: none; }"
        "QToolButton:hover { background: rgba(255,255,255,0.08); border-radius: 4px; }"
    );
    m_sidebarRevealBtn->setToolTip(tr("Show channel list"));
    m_sidebarRevealBtn->setIcon(makeSvgIcon(
        QStringLiteral(":/icons/mi-right-panel-open.svg"),
        QColor(m_theme.valid ? m_theme.text : "#e3e3e3")));
    m_sidebarRevealBtn->setVisible(false);
    m_sidebarRevealBtn->raise();
    connect(m_sidebarRevealBtn, &QToolButton::clicked, this, [this]{
        m_sidebarExpanded = true;
        m_sidebarPanel->setVisible(true);
        m_sidebarRevealBtn->setVisible(false);
        if (m_inputBar)
            if (auto *l = qobject_cast<QHBoxLayout*>(m_inputBar->layout()))
                l->setContentsMargins(4, 3, 4, 8);
        const int total = m_mainSplitter->width();
        m_mainSplitter->setSizes({m_sidebarExpandedWidth, total - m_sidebarExpandedWidth});
    });

    m_mainSplitter = new QSplitter(Qt::Horizontal);
    m_mainSplitter->setHandleWidth(0);
    m_mainSplitter->addWidget(m_sidebarPanel);
    m_mainSplitter->addWidget(chatSection);
    m_mainSplitter->setStretchFactor(0, 0);
    m_mainSplitter->setStretchFactor(1, 1);
    m_mainSplitter->setMinimumSize(1, 1);
    primaryVbox->addWidget(m_mainSplitter, 1);

    // setupInputBar will append search/reply/typing/input into chatSection

    m_panesSplitter = new QSplitter(Qt::Horizontal);
    m_panesSplitter->setHandleWidth(2);
    m_panesSplitter->addWidget(m_primaryPanel);
    m_panesSplitter->setStretchFactor(0, 1);

    auto *chatWrapper = new RoundedPane;
    auto *cwLayout    = new QVBoxLayout(chatWrapper);
    cwLayout->setContentsMargins(0, 0, 0, 0);
    cwLayout->setSpacing(0);
    cwLayout->addWidget(m_panesSplitter);
    vbox->addWidget(chatWrapper, 1);

    setCentralWidget(m_rightContent);
}

void MainWindow::ensureEmojiPicker()
{
    if (m_emojiPicker) return;
    m_emojiPicker = new EmojiPicker(this);
    connect(m_emojiPicker, &EmojiPicker::emojiSelected, this, [this](const QString &emoji){
        if (!m_pendingReactMsgid.isEmpty()) {
            m_model->sendReact(ServerId{m_pendingReactHost}, BufferId{m_pendingReactChannel},
                               m_pendingReactMsgid, emoji);
            m_pendingReactMsgid.clear();
            m_pendingReactHost.clear();
            m_pendingReactChannel.clear();
            return;
        }
        QTextCursor tc = m_input->textCursor();
        tc.insertText(emoji);
        m_input->setTextCursor(tc);
        m_input->setFocus();
    });
}

void MainWindow::setupInputBar()
{
    auto *bar  = new QWidget;
    bar->setObjectName("inputBar");
    auto *hbox = new QHBoxLayout(bar);
    hbox->setContentsMargins(4, 3, 4, 8);
    hbox->setSpacing(4);

    m_nickPrefix = new QLabel;
    m_nickPrefix->setStyleSheet("font-weight: bold; padding-right: 4px;");
    m_nickPrefix->setVisible(m_showNickPrefix);

    m_input = new QPlainTextEdit;
    m_input->setPlaceholderText("Type a message...");
    m_input->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    m_input->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_input->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_input->document()->setDocumentMargin(2);
    m_input->setFixedHeight(m_input->fontMetrics().lineSpacing() + 10);
    m_input->installEventFilter(this);

    m_emojiBtn = new QPushButton("😊");
    m_emojiBtn->setFixedSize(30, 30);
    m_emojiBtn->setStyleSheet("QPushButton { padding: 0; font-size: 16px; background: transparent; border: none; }");
    m_emojiBtn->setVisible(m_showEmojiBtn);
    m_emojiBtn->setToolTip("Emoji picker");

    // Send button floats inside the right edge of the input widget.
    m_sendBtn = new QToolButton(m_input);
    m_sendBtn->setFixedSize(28, 28);
    m_sendBtn->setAutoRaise(true);
    m_sendBtn->setToolTip("Send");
    m_sendBtn->setIcon(MenuIcons::send({}, 26));
    m_sendBtn->setIconSize(QSize(26, 26));
    m_sendBtn->setEnabled(false);
    m_sendBtn->setVisible(m_config.ui.showSendButton);
    connect(m_sendBtn, &QToolButton::clicked, this, &MainWindow::onInputSubmit);

    // Push text content left so it doesn't flow under the floating button
    {
        QTextFrameFormat fmt = m_input->document()->rootFrame()->frameFormat();
        fmt.setRightMargin(36);
        m_input->document()->rootFrame()->setFrameFormat(fmt);
    }

    // Format indicator: floats at bottom-left of input, shows active IRC format modes.
    m_formatIndicator = new QLabel(m_input);
    m_formatIndicator->setObjectName("formatIndicator");
    m_formatIndicator->setStyleSheet(
        "color: rgba(200,200,200,1.0); font-size: 13px; padding: 1px 5px;"
        "background: rgba(120,120,120,0.25); border-radius: 4px;");
    m_formatIndicator->hide();

    hbox->addWidget(m_nickPrefix);
    hbox->addWidget(m_input, 1);
    hbox->addWidget(m_emojiBtn);

    m_inputBar = bar;

    m_typingLabel = new QLabel;
    m_typingLabel->setObjectName("typingLabel");
    m_typingLabel->setContentsMargins(8, 2, 8, 2);
    m_typingLabel->setVisible(m_config.ui.typingIndicator);

    // Search bar (Ctrl+F)
    m_searchBar = new QWidget;
    m_searchBar->setObjectName("searchBar");
    {
        auto *shbox = new QHBoxLayout(m_searchBar);
        shbox->setContentsMargins(4, 2, 4, 2);
        shbox->setSpacing(4);
        m_searchInput = new QLineEdit;
        m_searchInput->setPlaceholderText("Search in buffer…");
        m_searchInput->installEventFilter(this);
        connect(m_searchInput, &QLineEdit::textChanged, this, [this](const QString &text){
            if (text.isEmpty()) m_chatView->clearFind();
            else m_chatView->findText(text, false);
        });
        auto *closeBtn = new QToolButton;
        closeBtn->setText("✕");
        closeBtn->setFixedSize(22, 22);
        closeBtn->setAutoRaise(true);
        closeBtn->setStyleSheet(
            "QToolButton { background: transparent; border: none; }"
            "QToolButton:hover { background: rgba(255,255,255,0.08); border-radius: 4px; }"
        );
        connect(closeBtn, &QToolButton::clicked, this, [this]{
            m_searchBar->hide();
            m_chatView->clearFind();
            m_input->setFocus();
        });
        shbox->addWidget(m_searchInput, 1);
        shbox->addWidget(closeBtn);
    }
    m_searchBar->hide();

    // Reply indicator bar
    m_replyBar = new QWidget;
    m_replyBar->setObjectName("replyBar");
    {
        auto *rhbox = new QHBoxLayout(m_replyBar);
        rhbox->setContentsMargins(8, 2, 4, 2);
        rhbox->setSpacing(4);
        m_replyLabel = new QLabel;
        m_replyLabel->setObjectName("replyLabel");
        auto *closeBtn = new QToolButton;
        closeBtn->setText("✕");
        closeBtn->setFixedSize(18, 18);
        closeBtn->setAutoRaise(true);
        connect(closeBtn, &QToolButton::clicked, this, &MainWindow::clearReplyBar);
        rhbox->addWidget(m_replyLabel, 1);
        rhbox->addWidget(closeBtn);
    }
    m_replyBar->hide();

    auto *layout = qobject_cast<QVBoxLayout *>(m_chatSection->layout());
    layout->addWidget(m_searchBar);
    layout->addWidget(m_replyBar);
    layout->addWidget(m_typingLabel);
    layout->addWidget(bar);

    connect(m_emojiBtn, &QPushButton::clicked, this, [this]{
        const QPoint anchor = m_emojiBtn->mapToGlobal(
            QPoint(m_emojiBtn->width(), m_emojiBtn->height()));
        ensureEmojiPicker();
        m_emojiPicker->showAt(anchor);
    });

    // Emoji inline autocomplete list (child widget, no focus steal)
    m_emojiCompleter = new QListWidget(this);
    m_emojiCompleter->setObjectName("emojiCompleter");
    m_emojiCompleter->setFocusPolicy(Qt::NoFocus);
    m_emojiCompleter->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_emojiCompleter->setFrameShape(QFrame::StyledPanel);
    m_emojiCompleter->hide();
    connect(m_emojiCompleter, &QListWidget::itemClicked, this, [this](QListWidgetItem *item){
        commitEmojiAutocomplete(m_emojiCompleter->row(item));
    });

    // Inactivity timer: sends typing=paused after 5s with no keypresses
    m_typingOutTimer = new QTimer(this);
    m_typingOutTimer->setSingleShot(true);
    m_typingOutTimer->setInterval(5000);
    connect(m_typingOutTimer, &QTimer::timeout, this, [this]{
        if (!m_config.ui.typingIndicator) return;
        const ServerId host = m_model->activeHost();
        const BufferId ch   = m_model->activeChannel();
        if (ch.isEmpty() || ch.str() == "(server)") return;
        m_typingActive = false;
        m_model->sendTyping(host, ch, "paused");
    });

    connect(m_input, &QPlainTextEdit::cursorPositionChanged,
            this, &MainWindow::updateFormatIndicator);

    connect(m_input, &QPlainTextEdit::textChanged, this, [this]{
        const QString text = m_input->toPlainText();
        m_sendBtn->setEnabled(!text.trimmed().isEmpty());
        checkEmojiAutocomplete(text);
        // Auto-resize: 1 to 4 lines
        const int lineH = m_input->fontMetrics().lineSpacing();
        const int margins = m_input->contentsMargins().top() + m_input->contentsMargins().bottom() + 8;
        const int lines = qMin(4, static_cast<int>(text.count('\n')) + 1);
        m_input->setFixedHeight(lines * lineH + margins);
        if (!m_config.ui.typingIndicator) return;
        const ServerId host = m_model->activeHost();
        const BufferId ch   = m_model->activeChannel();
        if (ch.isEmpty() || ch.str() == "(server)") return;
        if (!text.isEmpty()) {
            if (!m_typingActive) {
                m_typingActive = true;
                m_model->sendTyping(host, ch, "active");
            }
            m_typingOutTimer->start();
        } else {
            m_typingOutTimer->stop();
            if (m_typingActive) {
                m_typingActive = false;
                m_model->sendTyping(host, ch, "done");
            }
        }
    });

    QTimer::singleShot(0, this, [this]{ repositionSendBtn(); });
}

void MainWindow::connectModel()
{
    connect(m_model, &SessionModel::serverAdded,       this, &MainWindow::onServerAdded);
    connect(m_model, &SessionModel::serverConnected,   this, &MainWindow::onServerConnected);
    connect(m_model, &SessionModel::serverDisconnected,this, &MainWindow::onServerDisconnected);
    connect(m_model, &SessionModel::serverClosed,      this, &MainWindow::onServerClosed);
    connect(m_model, &SessionModel::channelAdded,      this, &MainWindow::onChannelAdded);
    connect(m_model, &SessionModel::channelRemoved,    this, &MainWindow::onChannelRemoved);
    connect(m_model, &SessionModel::messageAdded,      this, &MainWindow::onMessageAdded);
    connect(m_model, &SessionModel::topicChanged,      this, &MainWindow::onTopicChanged);
    connect(m_model, &SessionModel::modesChanged, this, [this](ServerId h, BufferId ch){
        if (h == m_model->activeHost() && ch.str().toLower() == m_model->activeChannel().str().toLower())
            refreshTopicBar(h, ch);
    });
    connect(m_model, &SessionModel::topicSetByChanged, this,
            [this](ServerId h, BufferId ch, const QString &setter, quint64 ts){
        if (h == m_model->activeHost() && ch.str().toLower() == m_model->activeChannel().str().toLower())
            if (m_topicSetByLabel) {
                m_topicSetByLabel->setText("Topic set by " + setter.section('!', 0, 0) + " · " + topicAgeStr(ts));
                m_topicSetByLabel->setVisible(!setter.isEmpty() && ts > 0);
            }
    });
    connect(m_model, &SessionModel::awayStatusChanged, this,
            [this](ServerId h, bool away){
        if (auto *srv = findServerItem(h)) {
            if (away)
                srv->setData(0, Qt::UserRole + 4, QVariant::fromValue(
                    makeSvgIcon(QStringLiteral(":/icons/mi-do-not-disturb.svg"),
                                QColor("#e06c75"))));
            else
                srv->setData(0, Qt::UserRole + 4, QVariant());
        }
        if (h == m_model->activeHost() && m_nickDelegate) {
            const QString selfNick = m_model->selfNick(h);
            m_nickDelegate->setSelfAway(selfNick, away);
            if (m_nickList) m_nickList->viewport()->update();
        }
    });
    connect(m_model, &SessionModel::nickListChanged,   this, &MainWindow::onNickListChanged);
    connect(m_model, &SessionModel::nickAdded,         this, &MainWindow::onNickAdded);
    connect(m_model, &SessionModel::nickRemoved,       this, &MainWindow::onNickRemoved);
    connect(m_model, &SessionModel::nickRenamed,       this, &MainWindow::onNickRenamed);
    connect(m_model, &SessionModel::unreadChanged,     this, &MainWindow::onUnreadChanged);
    connect(m_model, &SessionModel::reactionsChanged,  this, &MainWindow::onReactionsChanged);
    connect(m_model, &SessionModel::selfNickChanged,   this, &MainWindow::onSelfNickChanged);
    connect(m_model, &SessionModel::typingReceived,    this, &MainWindow::onTypingReceived);
    connect(m_model, &SessionModel::messageRedacted,   this, &MainWindow::onMessageRedacted);
    connect(m_model, &SessionModel::olderHistoryLoaded, this, &MainWindow::onOlderHistoryLoaded);
    connect(m_model, &SessionModel::userMetaChanged, this,
            [this](ServerId, const QString &, const QString &key, const QString &value) {
        if (key == QLatin1String("avatar")) fetchAvatar(value);
    });

    connect(m_model, &SessionModel::sslFingerprintPrompt, this,
            [this](ServerId host, const QString &fp)
    {
        QMessageBox box(this);
        box.setWindowTitle("Untrusted Certificate");
        box.setText(host.str() + " presented a certificate that could not be verified.");
        box.setInformativeText("SHA-256 fingerprint:\n" + fp
            + "\n\nTrust this certificate fingerprint for this server?");
        auto *pinBtn  = box.addButton("Pin Certificate", QMessageBox::AcceptRole);
        auto *onceBtn = box.addButton("Accept Once",     QMessageBox::AcceptRole);
        box.addButton("Reject",          QMessageBox::RejectRole);
        box.exec();
        if (box.clickedButton() == pinBtn)
            m_model->pinCertificate(host, fp);
        else if (box.clickedButton() == onceBtn)
            m_model->acceptCertificateOnce(host, fp);
        // Reject: connection already aborted in IrcClient::onSslErrors
    });

    connect(m_model, &SessionModel::dccSendReceived, this,
            [this](ServerId, const QString &fromNick,
                   const QString &filename, quint32 ip, quint16 port, qint64 filesize)
    {
        const QString sizeStr = filesize >= 1024*1024
            ? QString::number(filesize / (1024*1024)) + " MB"
            : QString::number(filesize / 1024) + " KB";
        const QString ipStr = QHostAddress(ip).toString();

        const int ret = QMessageBox::question(this, "Incoming DCC File",
            "Sender: " + fromNick + "\n"
            "File: " + filename + "\n"
            "Size: " + sizeStr + "\n"
            "Address: " + ipStr + ":" + QString::number(port) + "\n\n"
            "DCC connects directly to the sender and may reveal your IP address.\n"
            "Only accept files from people you trust.\n\nAccept?",
            QMessageBox::Yes | QMessageBox::No);
        if (ret != QMessageBox::Yes) return;

        const QString savePath = QFileDialog::getSaveFileName(
            this, "Save File", QFileInfo(filename).fileName());
        if (savePath.isEmpty()) return;

        auto *dcc  = new DccReceive(savePath, ip, port, filesize, this);
        QPointer<DccReceive> dccGuard(dcc);
        auto *prog = new QProgressDialog("Receiving " + filename + " from " + fromNick,
                                          "Cancel", 0, filesize > INT_MAX ? INT_MAX : static_cast<int>(filesize), this);
        prog->setWindowModality(Qt::NonModal);
        prog->setAttribute(Qt::WA_DeleteOnClose);

        connect(dcc, &DccReceive::progress, prog, [prog, filesize](qint64 received, qint64){
            prog->setValue(static_cast<int>(filesize > INT_MAX
                ? received * INT_MAX / filesize : received));
        });
        connect(dcc, &DccReceive::finished, this, [this, prog, dccGuard](const QString &path){
            prog->setValue(prog->maximum());
            if (dccGuard) dccGuard->deleteLater();
            QMessageBox::information(this, "DCC", "File received:\n" + path);
        });
        connect(dcc, &DccReceive::error, this, [this, prog, dccGuard](const QString &msg){
            prog->close();
            if (dccGuard) dccGuard->deleteLater();
            QMessageBox::warning(this, "DCC Error", msg);
        });
        connect(prog, &QProgressDialog::canceled, dcc, [dccGuard]{
            if (dccGuard) { dccGuard->cancel(); dccGuard->deleteLater(); }
        });

        dcc->start();
        prog->show();
    });

    connect(m_model, &SessionModel::dccPassiveOfferReceived, this,
            [this](ServerId server, const QString &fromNick,
                   const QString &filename, quint32 senderIp, qint64 filesize, const QString &token)
    {
        const QString sizeStr = filesize >= 1024*1024
            ? QString::number(filesize / (1024*1024)) + " MB"
            : QString::number(filesize / 1024) + " KB";
        const QString ipStr = QHostAddress(senderIp).toString();

        const int ret = QMessageBox::question(this, "Incoming DCC File (Passive)",
            "Sender: " + fromNick + "\n"
            "File: " + filename + "\n"
            "Size: " + sizeStr + "\n"
            "Sender address: " + ipStr + "\n\n"
            "DCC connects directly to the sender and may reveal your IP address.\n"
            "Only accept files from people you trust.\n\nAccept?",
            QMessageBox::Yes | QMessageBox::No);
        if (ret != QMessageBox::Yes) return;

        const QString savePath = QFileDialog::getSaveFileName(
            this, "Save File", QFileInfo(filename).fileName());
        if (savePath.isEmpty()) return;

        auto *dcc = new DccReceive(savePath, 0, 0, filesize, this);
        if (!dcc->listenPassive(senderIp)) { dcc->deleteLater(); return; }
        QPointer<DccReceive> dccGuard(dcc);

        IrcClient *client = m_model->clientFor(server);
        const quint32 ourIp   = client ? client->localIpv4() : 0;
        const quint16 ourPort = dcc->listenPort();
        QString fn = QFileInfo(filename).fileName().replace(' ', '_');
        fn.remove(QRegularExpression("[\\x00-\\x1f\\x7f]"));
        if (fn.isEmpty()) fn = QStringLiteral("file");
        fn = fn.left(180);

        m_model->sendRaw(server,
            "PRIVMSG " + fromNick + " :\x01""DCC SEND "
            + fn + " " + QString::number(ourIp)
            + " " + QString::number(ourPort)
            + " " + QString::number(filesize)
            + " " + token + "\x01");

        auto *prog = new QProgressDialog("Receiving " + filename + " from " + fromNick,
                                          "Cancel", 0, filesize > INT_MAX ? INT_MAX : static_cast<int>(filesize), this);
        prog->setWindowModality(Qt::NonModal);
        prog->setAttribute(Qt::WA_DeleteOnClose);

        connect(dcc, &DccReceive::progress, prog, [prog, filesize](qint64 received, qint64){
            prog->setValue(static_cast<int>(filesize > INT_MAX ? received * INT_MAX / filesize : received));
        });
        connect(dcc, &DccReceive::finished, this, [this, prog, dccGuard](const QString &path){
            prog->setValue(prog->maximum());
            if (dccGuard) dccGuard->deleteLater();
            QMessageBox::information(this, "DCC", "File received:\n" + path);
        });
        connect(dcc, &DccReceive::error, this, [this, prog, dccGuard](const QString &msg){
            prog->close();
            if (dccGuard) dccGuard->deleteLater();
            QMessageBox::warning(this, "DCC Error", msg);
        });
        connect(prog, &QProgressDialog::canceled, dcc, [dccGuard]{
            if (dccGuard) { dccGuard->cancel(); dccGuard->deleteLater(); }
        });
        prog->show();
    });

    connect(m_model, &SessionModel::dccPassiveSendReply, this,
            [this](ServerId, const QString &, const QString &,
                   quint32 ip, quint16 port, qint64, const QString &token)
    {
        DccSend *dcc = m_pendingPassiveSends.take(token);
        if (dcc) {
            if (isPrivateAddress(QHostAddress(ip))) {
                QMessageBox::warning(this, "DCC", "Blocked: remote address is private or reserved.");
                dcc->deleteLater();
            } else {
                dcc->connectOut(ip, port);
            }
        }
    });

    connect(m_model, &SessionModel::pingRtt, this, [this](ServerId host, int ms){
        if (m_signalBars && host == m_model->activeHost())
            m_signalBars->setLatency(ms);
    });
    connect(m_model, &SessionModel::serverReconnecting, this, [this](ServerId host){
        if (m_signalBars && host == m_model->activeHost())
            m_signalBars->setState(SignalBars::State::Reconnecting);
    });
}

// ---------------------------------------------------------------------------
// Event filter — Tab completion + input history
// ---------------------------------------------------------------------------

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
    for (auto *pane : std::as_const(m_orderedPanes)) {
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

    // Ctrl+wheel or Ctrl+Plus/Minus: zoom the focused region's font
    auto applyZoom = [this](QObject *target, double delta, const QPoint &pos = {}) -> bool {
        double *field = fontFieldForWidget(target, pos);
        if (!field) return false;
        *field = qBound(6.0, *field + delta, 32.0);
        applyFontSizes();
        saveConfig();
        return true;
    };

    if (event->type() == QEvent::Wheel) {
        auto *we = static_cast<QWheelEvent *>(event);
        if (we->modifiers() & Qt::ControlModifier) {
            if (applyZoom(obj, we->angleDelta().y() > 0 ? 0.5 : -0.5,
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
            if (delta != 0 && applyZoom(obj, delta))
                return true;
        }
        if (ke->modifiers() & Qt::AltModifier) {
            if (ke->key() == Qt::Key_Up)   { navigateChannel(-1); return true; }
            if (ke->key() == Qt::Key_Down) { navigateChannel(+1); return true; }
            if (ke->key() == Qt::Key_Left) { navigatePane(-1);    return true; }
            if (ke->key() == Qt::Key_Right){ navigatePane(+1);    return true; }
        }
    }

    if (obj == m_searchInput && event->type() == QEvent::KeyPress) {
        auto *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_Escape) {
            m_searchBar->hide();
            m_chatView->clearFind();
            m_input->setFocus();
            return true;
        }
        if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
            doSearch(ke->modifiers() & Qt::ShiftModifier);
            return true;
        }
    }




    if (obj == m_primaryHeader && event->type() == QEvent::Resize && m_sidebarHeader)
        m_sidebarHeader->setFixedHeight(static_cast<QResizeEvent *>(event)->size().height());

    if (obj == m_chatSection && event->type() == QEvent::Resize &&
        m_nickRevealBtn && m_nickRevealBtn->isVisible()) {
        auto *re = static_cast<QResizeEvent *>(event);
        const int topY = m_primaryHeader->height()
                       + (m_topicDisplay && m_topicDisplay->isVisible() ? m_topicDisplay->height() : 0)
                       + 4;
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
        for (auto *pane : std::as_const(m_orderedPanes)) {
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

    // Nick filter: Escape clears it
    if (obj == m_nickFilter && event->type() == QEvent::KeyPress) {
        auto *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_Escape) {
            m_nickFilter->clear();
            return true;
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
        showSearchBar();
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

void MainWindow::repositionSendBtn()
{
    if (!m_sendBtn || !m_input) return;
    const int pad = 3;
    const int x = m_input->width()  - m_sendBtn->width()  - pad;
    const int y = (m_input->height() - m_sendBtn->height()) / 2;
    m_sendBtn->move(x, y);
    m_sendBtn->raise();
    if (m_formatIndicator && m_formatIndicator->isVisible()) {
        const int fy = m_input->height() - m_formatIndicator->height() - 3;
        m_formatIndicator->move(4, fy);
    }
}

void MainWindow::updateFormatIndicator()
{
    if (!m_formatIndicator || !m_input) return;
    const QTextCharFormat cf = m_input->currentCharFormat();
    const bool bold   = cf.fontWeight() >= QFont::Bold;
    const bool italic = cf.fontItalic();
    const bool under  = cf.fontUnderline();
    const bool strike = cf.fontStrikeOut();
    if (!bold && !italic && !under && !strike) {
        m_formatIndicator->hide();
        return;
    }
    QString text;
    if (bold)   text += "<b>B</b>";
    if (italic) { if (!text.isEmpty()) text += " "; text += "<i>I</i>"; }
    if (under)  { if (!text.isEmpty()) text += " "; text += "<u>U</u>"; }
    if (strike) { if (!text.isEmpty()) text += " "; text += "<s>S</s>"; }
    m_formatIndicator->setText(text);
    m_formatIndicator->adjustSize();
    const int fy = m_input->height() - m_formatIndicator->height() - 3;
    m_formatIndicator->move(4, fy);
    m_formatIndicator->raise();
    m_formatIndicator->show();
}

void MainWindow::handleTabComplete(QPlainTextEdit *input, ServerId host, BufferId channel)
{
    const QTextCursor tc = input->textCursor();
    const QString text = tc.block().text();
    const int pos = tc.positionInBlock();

    if (!m_tabActive) {
        // Start a new cycle: derive prefix from text before cursor
        const qsizetype wordStart = text.lastIndexOf(' ', pos - 1) + 1;
        const QString prefix = text.mid(wordStart, pos - wordStart);
        if (prefix.isEmpty()) return;

        m_tabPrefix    = prefix;
        m_tabWordStart = static_cast<int>(wordStart);
        m_tabCandidates.clear();
        m_tabCandidateIndex = 0;
        m_tabActive = true;

        if (prefix.startsWith('/')) {
            static const QStringList commands = {
                "/away", "/back", "/ban", "/caps", "/clear", "/connect", "/ctcp",
                "/deop", "/devoice", "/disconnect", "/invite", "/j", "/join",
                "/close", "/kick", "/leave", "/me", "/mode", "/motd", "/msg",
                "/nick", "/notice", "/op", "/part", "/ping", "/time",
                "/quit", "/quote", "/raw", "/server", "/sysinfo", "/topic",
                "/unban", "/version", "/voice", "/whois",
            };
            for (const QString &cmd : commands)
                if (cmd.startsWith(prefix, Qt::CaseInsensitive))
                    m_tabCandidates << cmd;
        } else {
            auto *ch = m_model->channel(host, channel);
            if (ch) {
                for (const auto &e : std::as_const(ch->nicks))
                    if (e.nick.startsWith(prefix, Qt::CaseInsensitive))
                        m_tabCandidates << e.nick;
                m_tabCandidates.sort(Qt::CaseInsensitive);
            }
        }
    }
    // else: continuing a cycle — use stored m_tabWordStart and m_tabPrefix as-is

    if (m_tabCandidates.isEmpty()) return;

    const QString completed = m_tabCandidates[m_tabCandidateIndex];
    m_tabCandidateIndex = static_cast<int>((m_tabCandidateIndex + 1) % m_tabCandidates.size());

    // Nicks at line start get ": ", everything else at end-of-line gets " "
    QString suffix;
    if (pos == static_cast<int>(text.length()))
        suffix = (m_tabWordStart == 0 && !completed.startsWith('/'))
            ? QStringLiteral(": ") : QStringLiteral(" ");

    const int blockStart = tc.block().position();
    QTextCursor editCursor = input->textCursor();
    editCursor.setPosition(blockStart + m_tabWordStart);
    editCursor.setPosition(blockStart + pos, QTextCursor::KeepAnchor);
    editCursor.insertText(completed + suffix);
    input->setTextCursor(editCursor);
}

void MainWindow::handleHistoryUp()
{
    if (m_inputHistory.isEmpty()) return;
    if (m_historyIndex == -1)
        m_historyDraft = m_input->toPlainText();
    m_historyIndex = qMin(m_historyIndex + 1, static_cast<int>(m_inputHistory.size()) - 1);
    m_input->setPlainText(m_inputHistory[m_historyIndex]);
    m_input->moveCursor(QTextCursor::End);
}

void MainWindow::handleHistoryDown()
{
    if (m_historyIndex == -1) return;
    m_historyIndex--;
    if (m_historyIndex < 0) {
        m_historyIndex = -1;
        m_input->setPlainText(m_historyDraft);
    } else {
        m_input->setPlainText(m_inputHistory[m_historyIndex]);
    }
    m_input->moveCursor(QTextCursor::End);
}

// ---------------------------------------------------------------------------
// Emoji inline autocomplete
// ---------------------------------------------------------------------------

void MainWindow::checkEmojiAutocomplete(const QString &text)
{
    const int cursorPos = m_input->textCursor().position();
    const QString before = text.left(cursorPos);

    // Auto-substitute a completed :shortcode: when the closing colon is just typed
    if (cursorPos > 0 && text[cursorPos - 1] == ':') {
        const QString beforeColon = before.chopped(1);  // drop the closing ':'
        const qsizetype openColon = beforeColon.lastIndexOf(':');
        if (openColon >= 0) {
            const QString code = beforeColon.mid(openColon + 1);
            static const QRegularExpression wordOnly(R"(^\w+$)");
            if (wordOnly.match(code).hasMatch()) {
                const QString emoji = emojiByCode().value(code);
                if (!emoji.isEmpty()) {
                    QTextCursor tc = m_input->textCursor();
                    tc.setPosition(static_cast<int>(openColon));
                    tc.setPosition(cursorPos, QTextCursor::KeepAnchor);
                    tc.insertText(emoji);
                    m_input->setTextCursor(tc);
                    hideEmojiAutocomplete();
                    return;
                }
            }
        }
    }

    // Find a bare :word pattern ending at cursor — minimum 1 char after colon
    const qsizetype colon = before.lastIndexOf(':');
    if (colon < 0) { hideEmojiAutocomplete(); return; }

    const QString word = before.mid(colon + 1);
    // Only trigger if word is all word-chars and at least 1 char
    static const QRegularExpression wordRe(R"(^\w+$)");
    if (word.isEmpty() || !wordRe.match(word).hasMatch()) {
        hideEmojiAutocomplete();
        return;
    }

    const auto matches = emojiMatching(word);
    if (matches.isEmpty()) { hideEmojiAutocomplete(); return; }

    m_emojiTriggerPos = static_cast<int>(colon);

    m_emojiCompleter->clear();
    const int shown = static_cast<int>(qMin(matches.size(), qsizetype(8)));
    for (int i = 0; i < shown; ++i) {
        const auto &e = matches[i];
        auto *item = new QListWidgetItem(e.ch + "  " + e.shortcode);
        item->setData(Qt::UserRole, e.ch);
        m_emojiCompleter->addItem(item);
    }
    m_emojiCompleter->setCurrentRow(0);

    // Size to fit content
    const int itemH  = m_emojiCompleter->sizeHintForRow(0) + 2;
    const int popupH = itemH * shown + 4;
    const int popupW = 220;

    // Position above the input bar in main-window coords
    const QPoint inputTL = m_input->mapTo(this, QPoint(0, 0));

    // Align left edge with colon position using actual text advance
    const QString textUpToColon = before.left(colon);
    const int colonX = m_input->contentsMargins().left()
                       + m_input->fontMetrics().horizontalAdvance(textUpToColon);
    const QPoint colonLocal = m_input->mapTo(this, QPoint(colonX, 0));

    int px = qMax(inputTL.x(), colonLocal.x());
    int py = inputTL.y() - popupH - 2;
    if (py < 0) py = inputTL.y() + m_input->height() + 2;

    // Clamp horizontally
    if (px + popupW > width()) px = width() - popupW;

    m_emojiCompleter->setGeometry(px, py, popupW, popupH);
    m_emojiCompleter->show();
    m_emojiCompleter->raise();
}

void MainWindow::commitEmojiAutocomplete(int row)
{
    if (row < 0 || row >= m_emojiCompleter->count()) return;

    const QString emoji = m_emojiCompleter->item(row)->data(Qt::UserRole).toString();
    hideEmojiAutocomplete();

    // Replace :word (from trigger pos to cursor) with the emoji
    const int end = m_input->textCursor().position();
    QTextCursor tc = m_input->textCursor();
    tc.setPosition(m_emojiTriggerPos);
    tc.setPosition(end, QTextCursor::KeepAnchor);
    tc.insertText(emoji);
    m_input->setTextCursor(tc);
    m_input->setFocus();
}

void MainWindow::hideEmojiAutocomplete()
{
    m_emojiCompleter->hide();
    m_emojiCompleter->clear();
    m_emojiTriggerPos = -1;
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
        item->setData(0, Qt::UserRole + 2, QVariant::fromValue(makeConnectedIcon()));
    if (m_signalBars && host == m_model->activeHost())
        m_signalBars->setState(SignalBars::State::Connected);

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
    }
}

void MainWindow::onServerClosed(ServerId host)
{
    auto *srv = findServerItem(host);
    if (!srv) return;

    // Close any open panes for channels on this server
    for (int i = srv->childCount() - 1; i >= 0; --i) {
        const BufferId ch{srv->child(i)->data(0, Qt::UserRole + 1).toString()};
        closeChannelPane(host, ch);
    }

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


// Collect the tail run of consecutive condensable messages from ch.messages
// that share the same calendar day as the last message.
static QList<Message> collectEventGroup(const Channel *ch, const QString &selfNick)
{
    QList<Message> group;
    if (ch->messages.isEmpty()) return group;
    const QDate day = ch->messages.last().timestamp.toLocalTime().date();
    for (qsizetype i = ch->messages.size() - 1; i >= 0; --i) {
        const auto &m = ch->messages[i];
        if (!ChatRenderer::isCondensable(m, selfNick)) break;
        if (m.timestamp.toLocalTime().date() != day) break;
        group.prepend(m);
    }
    return group;
}

void MainWindow::onMessageAdded(ServerId host, BufferId channel, const Message &msg)
{
    const QString selfNick = m_model->selfNick(host);

    auto makeCtx = [&](Channel *ch) {
        ChatRenderer::Context ctx;
        ctx.coloredNicks = m_config.ui.coloredNicks;
        ctx.nickBrackets = m_config.ui.nickBrackets;
        ctx.emojiPt      = m_config.ui.fontSizes.emoji;
        ctx.chatPt       = m_config.ui.fontSizes.chat;
        ctx.validTheme   = m_theme.valid;
        ctx.themeText    = m_theme.text;
        ctx.selfNickRe   = m_selfNickRe;
    ctx.highlightRe  = m_highlightRe;
    ctx.showTimestamps = m_config.ui.showTimestamps;
        ctx.channel      = ch;
        return ctx;
    };

    auto appendToView = [&](ChatView *view, Channel *ch) {
        if (ChatRenderer::isCondensable(msg, selfNick)) {
            const QList<Message> group = collectEventGroup(ch, selfNick);
            if (group.isEmpty()) return;
            const QString groupId = QString::number(group.first().timestamp.toMSecsSinceEpoch());
            const bool grpExpanded = m_expandedEventGroups.contains(groupId);
            const QString blockId  = "evgrp:" + groupId;
            const ChatLine line    = ChatRenderer::formatEventGroupLine(group, makeCtx(ch), groupId, grpExpanded);
            if (view->findLine(blockId) >= 0)
                view->replaceLine(blockId, line);
            else
                view->appendLine(line);
        } else {
            view->appendLine(ChatRenderer::formatMessageLine(msg, makeCtx(ch)));
        }
        if (view->isAtBottom()) view->scrollToBottom();
    };

    if (host == m_model->activeHost() &&
        channel.str().toLower() == m_model->activeChannel().str().toLower())
    {
        auto *ch = m_model->channel(host, channel);
        if (ch) {
            if (ChatRenderer::isCondensable(msg, selfNick)) {
                appendToView(m_chatView, ch);
            } else {
                appendMessage(msg, m_config.ui.linkPreviews);
            }
        }
    }

    const QString paneKey = host.str() + "|" + channel.str().toLower();
    if (auto *pane = m_panes.value(paneKey)) {
        auto *pCh = m_model->channel(host, channel);
        if (pCh) appendToView(pane->chatView(), pCh);
    }

    if (m_config.ui.notifications && m_tray && !isActiveWindow()
        && (msg.type == MessageType::Privmsg || msg.type == MessageType::Action))
    {
        const QString myNick = m_model->selfNick(host);
        const bool isPM = !channel.str().startsWith('#') && !channel.str().startsWith('&');
        const bool isMention = !isPM && !myNick.isEmpty()
                               && msg.text.contains(myNick, Qt::CaseInsensitive);
        if (isPM || isMention)
            m_tray->setNotify(true);
    }
}

void MainWindow::onTopicChanged(ServerId host, BufferId channel, const QString &topic)
{
    if (host == m_model->activeHost() &&
        channel.str().toLower() == m_model->activeChannel().str().toLower())
        refreshTopicBar(host, channel);

    const QString paneKey = host.str() + "|" + channel.str().toLower();
    if (auto *pane = m_panes.value(paneKey))
        pane->setTopic(ChatRenderer::linkifyTopic(topic));
}

void MainWindow::onNickListChanged(ServerId host, BufferId channel)
{
    scheduleNickRefresh(host, channel);
}

void MainWindow::scheduleNickRefresh(ServerId host, BufferId channel)
{
    const QString key = host.str() + "|" + channel.str().toLower();
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

void MainWindow::onReactionsChanged(ServerId host, BufferId channel, const QString &msgid)
{
    auto updateView = [&](ChatView *view, Channel *ch) {
        if (view->findLine(msgid) < 0) {
            qWarning() << "onReactionsChanged: findLine miss for msgid=" << msgid;
            return;
        }
        const auto rxIt = ch->reactions.constFind(msgid);
        const bool hasReactions = rxIt != ch->reactions.constEnd() && !rxIt->isEmpty();
        const QString rxId = "rx:" + msgid;
        if (!hasReactions) {
            view->removeLine(rxId);
            return;
        }
        const ChatLine rxLine = ChatRenderer::buildReactionLine(*rxIt, msgid);
        if (view->findLine(rxId) >= 0)
            view->replaceLine(rxId, rxLine);
        else
            view->insertAfter(msgid, rxLine);
    };

    if (host == m_model->activeHost() &&
        channel.str().toLower() == m_model->activeChannel().str().toLower()) {
        auto *ch = m_model->channel(host, channel);
        if (ch) updateView(m_chatView, ch);
    }

    for (auto *pane : std::as_const(m_panes)) {
        if (pane->host() != host || pane->channel().str().toLower() != channel.str().toLower()) continue;
        auto *pCh = m_model->channel(host, channel);
        if (pCh) updateView(pane->chatView(), pCh);
    }
}

void MainWindow::onSelfNickChanged(ServerId host, const QString &nick)
{
    if (host == m_model->activeHost()) {
        m_nickPrefix->setText(nick);
        m_selfNickRe = nick.isEmpty() ? QRegularExpression{}
            : QRegularExpression("(\\b" + QRegularExpression::escape(nick) + "\\b)",
                                 QRegularExpression::CaseInsensitiveOption);
    }

    for (auto *pane : std::as_const(m_panes))
        if (pane->host() == host)
            pane->setNick(nick);
}

void MainWindow::onMessageRedacted(ServerId host, BufferId channel, const QString &msgid)
{
    auto makeCtx = [&](Channel *ch) {
        ChatRenderer::Context ctx;
        ctx.coloredNicks = m_config.ui.coloredNicks;
        ctx.nickBrackets = m_config.ui.nickBrackets;
        ctx.emojiPt      = m_config.ui.fontSizes.emoji;
        ctx.chatPt       = m_config.ui.fontSizes.chat;
        ctx.validTheme   = m_theme.valid;
        ctx.themeText    = m_theme.text;
        ctx.selfNickRe   = m_selfNickRe;
    ctx.highlightRe  = m_highlightRe;
    ctx.showTimestamps = m_config.ui.showTimestamps;
        ctx.channel      = ch;
        return ctx;
    };

    auto updateView = [&](ChatView *view, Channel *ch) {
        if (view->findLine(msgid) < 0) return;
        for (const auto &msg : std::as_const(ch->messages)) {
            if (msg.msgid == msgid) {
                view->replaceLine(msgid, ChatRenderer::formatMessageLine(msg, makeCtx(ch)));
                break;
            }
        }
    };

    if (host == m_model->activeHost() &&
        channel.str().toLower() == m_model->activeChannel().str().toLower()) {
        auto *ch = m_model->channel(host, channel);
        if (ch) updateView(m_chatView, ch);
    }

    for (auto *pane : std::as_const(m_panes)) {
        if (pane->host() != host || pane->channel().str().toLower() != channel.str().toLower()) continue;
        auto *pCh = m_model->channel(host, channel);
        if (pCh) updateView(pane->chatView(), pCh);
    }
}

void MainWindow::onTypingReceived(ServerId host, BufferId channel,
                                   const QString &nick, const QString &state)
{
    if (!m_config.ui.typingIndicator) return;

    const QString key      = host.str() + "|" + channel.str();
    const QString timerKey = host.str() + "|" + channel.str() + "|" + nick;

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


void MainWindow::updateTypingLabel()
{
    const QString key = m_model->activeHost().str() + "|" + m_model->activeChannel().str();
    const QSet<QString> &typers = m_typingNicks.value(key);

    if (!m_config.ui.typingIndicator) {
        m_typingLabel->setVisible(false);
        return;
    }

    if (typers.isEmpty()) {
        m_typingLabel->setText("");
        return;
    }

    QStringList names(typers.begin(), typers.end());
    QString text;
    if (names.size() == 1)
        text = names[0] + " is typing...";
    else if (names.size() == 2)
        text = names[0] + " and " + names[1] + " are typing...";
    else
        text = QString::number(names.size()) + " people are typing...";

    m_typingLabel->setText(text);
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
        for (int c = 0; c < srv->childCount(); ++c)
            channels.append(srv->child(c));
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

void MainWindow::navigatePane(int direction)
{
    if (m_orderedPanes.isEmpty()) return;

    int cur = -1;
    for (int i = 0; i < m_orderedPanes.size(); ++i) {
        auto *pane = m_orderedPanes[i];
        if (pane->host() == m_model->activeHost() &&
            pane->channel() == m_model->activeChannel()) {
            cur = i;
            break;
        }
    }

    const int count = static_cast<int>(m_orderedPanes.size());
    int next = (cur < 0) ? 0 : cur + direction;
    if (next < 0) next = count - 1;
    if (next >= count) next = 0;

    auto *pane = m_orderedPanes[next];
    auto *item = findChannelItem(pane->host(), pane->channel());
    if (item) {
        m_sidebar->setCurrentItem(item);
        onSidebarSelectionChanged();
    }
}

// ---------------------------------------------------------------------------
// Input dispatch
// ---------------------------------------------------------------------------

// Convert the input widget's rich-formatted document to an IRC-encoded string.
// IRC control chars (bold \x02, italic \x1D, underline \x1F, strikethrough \x1E)
// are emitted at format-change boundaries; the visible text has no box glyphs.
static QString inputToIrcText(QPlainTextEdit *edit)
{
    const QTextDocument *doc = edit->document();
    QString result;
    bool curBold = false, curItalic = false, curUnder = false, curStrike = false;
    bool firstBlock = true;
    for (QTextBlock block = doc->begin(); block != doc->end(); block = block.next()) {
        if (!firstBlock) result += '\n';
        firstBlock = false;
        for (auto it = block.begin(); !it.atEnd(); ++it) {
            const QTextFragment frag = it.fragment();
            if (!frag.isValid()) continue;
            const QTextCharFormat fmt = frag.charFormat();
            const bool wB = (fmt.fontWeight() == QFont::Bold);
            const bool wI = fmt.fontItalic();
            const bool wU = fmt.fontUnderline();
            const bool wS = fmt.fontStrikeOut();
            if (wB != curBold)   { result += QChar(0x02); curBold  = wB; }
            if (wI != curItalic) { result += QChar(0x1D); curItalic = wI; }
            if (wU != curUnder)  { result += QChar(0x1F); curUnder = wU; }
            if (wS != curStrike) { result += QChar(0x1E); curStrike = wS; }
            result += frag.text();
        }
    }
    if (curBold || curItalic || curUnder || curStrike)
        result += QChar(0x0F);
    return result;
}

void MainWindow::onInputSubmit()
{
    const QString raw  = inputToIrcText(m_input);
    const QString text = raw.trimmed();
    if (text.isEmpty()) return;

    // Push to history (newest first, skip consecutive duplicates)
    if (m_inputHistory.isEmpty() || m_inputHistory.first() != text) {
        m_inputHistory.prepend(text);
        if (m_inputHistory.size() > kInputHistoryCap)
            m_inputHistory.removeLast();
    }
    m_historyIndex = -1;
    m_tabActive = false;
    m_tabCandidates.clear();
    hideEmojiAutocomplete();

    m_input->clear();
    if (m_formatIndicator) m_formatIndicator->hide();

    const ServerId host    = m_model->activeHost();
    const BufferId channel = m_model->activeChannel();
    if (host.isEmpty() || channel.isEmpty()) return;

    // Stop typing notification on send
    m_typingOutTimer->stop();
    if (m_typingActive && m_config.ui.typingIndicator && channel.str() != "(server)") {
        m_typingActive = false;
        m_model->sendTyping(host, channel, "done");
    }

    const QStringList lines = raw.split('\n', Qt::SkipEmptyParts);
    if (lines.size() > 1) {
        if (lines.size() >= 4) {
            const auto ans = QMessageBox::question(this, "Send multiple lines?",
                QString("Send %1 lines to %2?").arg(lines.size()).arg(channel.str()),
                QMessageBox::Yes | QMessageBox::Cancel);
            if (ans != QMessageBox::Yes) return;
        }
        for (const QString &line : lines)
            dispatchInput(line, host, channel);
        return;
    }

    dispatchInput(raw, host, channel);
}

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
        const QString emoji = emojiByCode().value(m.captured(1));
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

    const bool isChannel = channel.str().startsWith('#') || channel.str().startsWith('&')
                           || channel.str().startsWith('+') || channel.str().startsWith('!');

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

    if (auto *sess = m_model->session(host)) {
        m_nickPrefix->setText(sess->nick);
        m_selfNickRe = sess->nick.isEmpty() ? QRegularExpression{}
            : QRegularExpression("(\\b" + QRegularExpression::escape(sess->nick) + "\\b)",
                                 QRegularExpression::CaseInsensitiveOption);
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

void MainWindow::openChannelPane(ServerId host, BufferId channel)
{
    const QString key = host.str() + "|" + channel.str().toLower();
    if (m_panes.contains(key)) return;
    if (m_orderedPanes.size() >= kMaxExtraPanes) return;

    auto *pane = new ChannelPane(host, channel, this);
    if (m_theme.valid)
        pane->chatView()->setColors(QColor(m_theme.text), QColor(m_theme.background),
                                    QColor(m_theme.accent), QColor(m_theme.background),
                                    QColor(m_theme.border));

    pane->input()->installEventFilter(this);
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
        auto makeFont = [&](double pt){ QFont f(fam); f.setPointSizeF(pt); f.setStyleHint(QFont::Monospace); return f; };
        const QFont chatFont = makeFont(fs.chat);
        pane->chatView()->setFont(chatFont);
        pane->nickList()->setFont(makeFont(fs.nickList));
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
    }

    if (auto *ch = m_model->channel(host, channel))
        pane->setTopic(ChatRenderer::linkifyTopic(ch->topic));

    connect(pane, &ChannelPane::closeRequested, this, [this, host, channel]{
        closeChannelPane(host, channel);
    });
    connect(pane, &ChannelPane::inputSubmitted, this, [this, host, channel](const QString &text){
        dispatchInput(text, host, channel);
    });
    connect(pane, &ChannelPane::dragActive, this, [this](const QString &sourceKey, const QPoint &gp){
        ChannelPane *target = paneAt(gp);
        if (target == m_panes.value(sourceKey)) target = nullptr;
        if (m_dragHighlighted && m_dragHighlighted != target) {
            m_dragHighlighted->setDragHighlight(false);
            m_dragHighlighted = nullptr;
        }
        if (target && !m_dragHighlighted) {
            target->setDragHighlight(true);
            m_dragHighlighted = target;
        }
    });
    connect(pane, &ChannelPane::dragDropped, this, [this](const QString &sourceKey, const QPoint &gp){
        if (m_dragHighlighted) {
            m_dragHighlighted->setDragHighlight(false);
            m_dragHighlighted = nullptr;
        }
        ChannelPane *source = m_panes.value(sourceKey);
        if (!source) return;
        ChannelPane *target = paneAt(gp);
        if (target && target != source) {
            // pane ↔ pane swap
            const qsizetype fromIdx = m_orderedPanes.indexOf(source);
            const qsizetype toIdx   = m_orderedPanes.indexOf(target);
            if (fromIdx >= 0 && toIdx >= 0) {
                m_orderedPanes.swapItemsAt(fromIdx, toIdx);
                rebuildPaneLayout();
            }
        } else if (!target && isOverPrimary(gp)) {
            // pane ↔ primary swap: move primary to source's old slot
            const qsizetype srcIdx  = m_orderedPanes.indexOf(source);
            if (srcIdx >= 0) {
                // combined slot of source (accounts for primary's current position)
                const int paneSlot = static_cast<int>(srcIdx < m_primarySlot ? srcIdx : srcIdx + 1);
                m_primarySlot = paneSlot;
                rebuildPaneLayout();
            }
        }
    });

    m_panes[key] = pane;
    m_orderedPanes.append(pane);
    m_primaryHeader->setVisible(true);
    m_primaryCloseBtn->setVisible(true);

    if (m_theme.valid) {
        pane->setTopicIcon(
            makeTopicIcon(QColor(m_theme.placeholder)),
            makeTopicIcon(QColor(m_theme.accent)));
    }

    rebuildPaneLayout();
    refreshPaneChatView(pane);
    refreshPaneNickList(pane);
}

ChannelPane *MainWindow::paneAt(const QPoint &globalPos) const
{
    QWidget *w = QApplication::widgetAt(globalPos);
    while (w) {
        if (auto *p = qobject_cast<ChannelPane *>(w)) return p;
        w = w->parentWidget();
    }
    return nullptr;
}

bool MainWindow::isOverPrimary(const QPoint &globalPos) const
{
    QWidget *w = QApplication::widgetAt(globalPos);
    while (w) {
        if (w == m_primaryPanel) return true;
        w = w->parentWidget();
    }
    return false;
}

void MainWindow::closeChannelPane(ServerId host, BufferId channel)
{
    const QString key = host.str() + "|" + channel.str().toLower();
    auto *pane = m_panes.take(key);
    if (!pane) return;

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

    auto makeVert = []() -> QSplitter * {
        auto *s = new QSplitter(Qt::Vertical);
        s->setHandleWidth(2);
        return s;
    };

    const qsizetype n = widgets.size();
    if (n <= 2) {
        // 1 or 2 panes: flat horizontal
        for (auto *w : std::as_const(widgets)) {
            m_panesSplitter->addWidget(w);
            w->show();
        }
    } else if (n == 3) {
        // primary full-height left, two panes stacked right
        m_panesSplitter->addWidget(widgets[0]);
        widgets[0]->show();
        auto *right = makeVert();
        right->addWidget(widgets[1]);
        right->addWidget(widgets[2]);
        widgets[1]->show();
        widgets[2]->show();
        m_panesSplitter->addWidget(right);
    } else { // n == 4  (2×2 grid)
        auto *left = makeVert();
        left->addWidget(widgets[0]);
        left->addWidget(widgets[1]);
        widgets[0]->show();
        widgets[1]->show();
        auto *right = makeVert();
        right->addWidget(widgets[2]);
        right->addWidget(widgets[3]);
        widgets[2]->show();
        widgets[3]->show();
        m_panesSplitter->addWidget(left);
        m_panesSplitter->addWidget(right);
    }

    // Equalize top-level columns
    const int total = m_panesSplitter->width();
    if (total > 0 && m_panesSplitter->count() > 0) {
        const int each = total / m_panesSplitter->count();
        QList<int> sizes(m_panesSplitter->count(), each);
        m_panesSplitter->setSizes(sizes);
    }
}

void MainWindow::refreshPaneChatView(ChannelPane *pane)
{
    pane->chatView()->clear();
    auto *ch = m_model->channel(pane->host(), pane->channel());
    if (!ch) return;

    ChatRenderer::Context ctx;
    ctx.coloredNicks = m_config.ui.coloredNicks;
    ctx.nickBrackets = m_config.ui.nickBrackets;
    ctx.emojiPt      = m_config.ui.fontSizes.emoji;
    ctx.chatPt       = m_config.ui.fontSizes.chat;
    ctx.validTheme   = m_theme.valid;
    ctx.themeText    = m_theme.text;
    ctx.selfNickRe   = m_selfNickRe;
    ctx.highlightRe  = m_highlightRe;
    ctx.showTimestamps = m_config.ui.showTimestamps;
    ctx.channel      = ch;

    const QString selfNick = m_model->selfNick(pane->host());

    static const QRegularExpression urlRe(
        R"(https?://[^\s<>"]+)",
        QRegularExpression::CaseInsensitiveOption);

    for (int i = 0; i < ch->messages.size(); ) {
        const auto &msg = ch->messages[i];
        if (ChatRenderer::isCondensable(msg, selfNick)) {
            const QDate day = msg.timestamp.toLocalTime().date();
            int j = i + 1;
            while (j < ch->messages.size()
                   && ChatRenderer::isCondensable(ch->messages[j], selfNick)
                   && ch->messages[j].timestamp.toLocalTime().date() == day)
                ++j;
            QList<Message> group(ch->messages.cbegin() + i, ch->messages.cbegin() + j);
            const QString groupId = QString::number(group.first().timestamp.toMSecsSinceEpoch());
            const bool grpExpanded = m_expandedEventGroups.contains(groupId);
            pane->chatView()->appendLine(
                ChatRenderer::formatEventGroupLine(group, ctx, groupId, grpExpanded));
            i = j;
        } else {
            pane->chatView()->appendLine(ChatRenderer::formatMessageLine(msg, ctx));
            if (!msg.msgid.isEmpty()) {
                auto rxIt = ch->reactions.constFind(msg.msgid);
                if (rxIt != ch->reactions.constEnd() && !rxIt->isEmpty())
                    pane->chatView()->appendLine(ChatRenderer::buildReactionLine(*rxIt, msg.msgid));
            }
            const bool isText = (msg.type == MessageType::Privmsg ||
                                 msg.type == MessageType::Action  ||
                                 msg.type == MessageType::Notice);
            if (isText && !ch->previews.isEmpty()) {
                auto uit = urlRe.globalMatch(msg.text);
                while (uit.hasNext()) {
                    const QString urlStr = QUrl(uit.next().captured(0)).toString();
                    const auto p = ch->previews.constFind(urlStr);
                    if (p == ch->previews.constEnd() || ch->hiddenPreviews.contains(urlStr))
                        continue;
                    ChatLine card;
                    card.role   = ChatLineRole::PreviewCard;
                    card.id     = "preview:" + urlStr;
                    if (!p->pngData.isEmpty()) card.image.loadFromData(p->pngData, "PNG");
                    card.text   = p->title + "\n" + p->domain;
                    ChatSegment seg; seg.start = 0; seg.length = static_cast<int>(card.text.size());
                    seg.anchor = "preview:" + urlStr;
                    card.segments.append(seg);
                    pane->chatView()->appendLine(card);
                }
            }
            ++i;
        }
    }

    pane->chatView()->scrollToBottom();
}

void MainWindow::refreshPaneNickList(ChannelPane *pane)
{
    pane->nickList()->clear();
    auto *ch   = m_model->channel(pane->host(), pane->channel());
    if (!ch) return;
    auto *sess = m_model->session(pane->host());

    for (const auto &e : std::as_const(ch->nicks))
        pane->nickList()->addItem(makeNickItem(e, ch, sess));
}

void MainWindow::enqueuePreview(const QUrl &url, ServerId host, BufferId channel, const QString &msgid)
{
    const QString key = url.toString();
    if (key.isEmpty()) return;
    if (m_previewChannels.contains(key)) return;
    if (m_previewQueue.size() >= 10) return;
    if (m_previewChannels.size() >= 100) return;
    m_previewChannels.insert(key, {host, channel, msgid});
    m_previewQueue.enqueue(url);
    processPreviewQueue();
}

void MainWindow::processPreviewQueue()
{
    if (m_previewFetchBusy || m_previewQueue.isEmpty()) return;
    m_previewFetchBusy = true;
    m_previewWatchdog->start(20000);
    m_linkPreview->fetch(m_previewQueue.dequeue());
}

void MainWindow::refreshChatView(ServerId host, BufferId channel, bool resetToLatest)
{
    m_chatView->clear();
    auto *ch = m_model->channel(host, channel);
    if (!ch) return;

    const QString   key   = host.str() + '\t' + channel.str();
    const qsizetype total = ch->messages.size();

    if (resetToLatest || !m_renderStart.contains(key))
        m_renderStart[key] = static_cast<int>(qMax(qsizetype(0), total - kRenderWindow));
    const int startIdx = m_renderStart.value(key, 0);

    static const QRegularExpression urlRe(
        R"(https?://[^\s<>"]+)",
        QRegularExpression::CaseInsensitiveOption);

    ChatRenderer::Context ctx;
    ctx.coloredNicks = m_config.ui.coloredNicks;
    ctx.nickBrackets = m_config.ui.nickBrackets;
    ctx.emojiPt      = m_config.ui.fontSizes.emoji;
    ctx.chatPt       = m_config.ui.fontSizes.chat;
    ctx.validTheme   = m_theme.valid;
    ctx.themeText    = m_theme.text;
    ctx.selfNickRe   = m_selfNickRe;
    ctx.highlightRe  = m_highlightRe;
    ctx.showTimestamps = m_config.ui.showTimestamps;
    ctx.channel      = ch;

    if (startIdx > 0) {
        ChatLine status = ChatRenderer::makeStatusLine(
            QString("── %1 older messages ──").arg(startIdx), m_theme.separator);
        status.id = "status:older";
        m_chatView->appendLine(status);
    }

    const QString selfNick   = m_model->selfNick(host);
    const int     firstUnread = ch->firstUnreadIdx;
    bool          sepInserted = false;

    for (int i = startIdx; i < ch->messages.size(); ) {
        // Insert "── N new messages ──" separator before first unread message
        if (!sepInserted && firstUnread >= startIdx && i == firstUnread) {
            const int n = ch->unread;
            ChatLine sep = ChatRenderer::makeStatusLine(
                QString("── %1 new message%2 ──").arg(n).arg(n == 1 ? "" : "s"), m_theme.separator);
            sep.id = QStringLiteral("sep:unread");
            m_chatView->appendLine(sep);
            sepInserted = true;
        }

        const auto &msg = ch->messages[i];
        if (ChatRenderer::isCondensable(msg, selfNick)) {
            const QDate day = msg.timestamp.toLocalTime().date();
            int j = i + 1;
            while (j < ch->messages.size()
                   && ChatRenderer::isCondensable(ch->messages[j], selfNick)
                   && ch->messages[j].timestamp.toLocalTime().date() == day)
                ++j;
            QList<Message> group(ch->messages.cbegin() + i, ch->messages.cbegin() + j);
            const QString groupId = QString::number(group.first().timestamp.toMSecsSinceEpoch());
            const bool grpExpanded = m_expandedEventGroups.contains(groupId);
            m_chatView->appendLine(ChatRenderer::formatEventGroupLine(group, ctx, groupId, grpExpanded));
            i = j;
        } else {
            m_chatView->appendLine(ChatRenderer::formatMessageLine(msg, ctx));
            if (!msg.msgid.isEmpty()) {
                auto rxIt = ch->reactions.constFind(msg.msgid);
                if (rxIt != ch->reactions.constEnd() && !rxIt->isEmpty())
                    m_chatView->appendLine(ChatRenderer::buildReactionLine(*rxIt, msg.msgid));
            }
            const bool isText = (msg.type == MessageType::Privmsg ||
                                 msg.type == MessageType::Action  ||
                                 msg.type == MessageType::Notice);
            if (isText && !ch->previews.isEmpty()) {
                auto it = urlRe.globalMatch(msg.text);
                while (it.hasNext()) {
                    const QString urlStr = QUrl(it.next().captured(0)).toString();
                    const auto p = ch->previews.constFind(urlStr);
                    if (p == ch->previews.constEnd() || ch->hiddenPreviews.contains(urlStr))
                        continue;
                    ChatLine card;
                    card.role   = ChatLineRole::PreviewCard;
                    card.id     = "preview:" + urlStr;
                    if (!p->pngData.isEmpty()) card.image.loadFromData(p->pngData, "PNG");
                    card.text   = p->title + "\n" + p->domain;
                    ChatSegment seg; seg.start = 0; seg.length = static_cast<int>(card.text.size());
                    seg.anchor = "preview:" + urlStr;
                    card.segments.append(seg);
                    m_chatView->appendLine(card);
                }
            }
            ++i;
        }
    }

    if (resetToLatest) {
        QTimer::singleShot(0, this, [this, key, firstUnread, startIdx] {
            // Scroll to unread separator when present in the render window
            if (firstUnread >= startIdx) {
                const int li = m_chatView->findLine(QStringLiteral("sep:unread"));
                if (li >= 0) { m_chatView->scrollToLine(li); return; }
            }
            // Restore saved scroll position if user was reading history and nothing new arrived
            const int saved = m_scrollPositions.take(key);
            if (saved > 0)
                m_chatView->verticalScrollBar()->setValue(saved);
            else
                m_chatView->scrollToBottom();
        });
    }

#if defined(__linux__) && !defined(__MUSL__)
    malloc_trim(0);
#endif
}

void MainWindow::loadOlderMessages()
{
    if (m_loadingOlder) return;
    const ServerId host    = m_model->activeHost();
    const BufferId chName  = m_model->activeChannel();
    if (host.isEmpty() || chName.isEmpty()) return;

    const QString key = host.str() + '\t' + chName.str();
    if (!m_renderStart.contains(key) || m_renderStart[key] == 0) {
        if (m_historyExhausted.contains(key)) return;
        m_loadingOlder = true;
        m_model->requestOlderHistory(host, chName);
        return;
    }

    auto *ch = m_model->channel(host, chName);
    if (!ch) return;

    m_loadingOlder = true;

    const int prevStart = m_renderStart[key];
    m_renderStart[key]  = qMax(0, prevStart - kRenderChunk);
    const int newStart  = m_renderStart[key];

    ChatRenderer::Context ctx;
    ctx.coloredNicks = m_config.ui.coloredNicks;
    ctx.nickBrackets = m_config.ui.nickBrackets;
    ctx.emojiPt      = m_config.ui.fontSizes.emoji;
    ctx.chatPt       = m_config.ui.fontSizes.chat;
    ctx.validTheme   = m_theme.valid;
    ctx.themeText    = m_theme.text;
    ctx.selfNickRe   = m_selfNickRe;
    ctx.highlightRe  = m_highlightRe;
    ctx.showTimestamps = m_config.ui.showTimestamps;
    ctx.channel      = ch;

    const QString selfNick = m_model->selfNick(host);
    QList<ChatLine> older;

    if (newStart > 0) {
        ChatLine status = ChatRenderer::makeStatusLine(
            QString("── %1 older messages ──").arg(newStart), m_theme.separator);
        status.id = "status:older";
        older.append(status);
    }

    for (int i = newStart; i < prevStart; ) {
        const auto &msg = ch->messages[i];
        if (ChatRenderer::isCondensable(msg, selfNick)) {
            const QDate day = msg.timestamp.toLocalTime().date();
            int j = i + 1;
            while (j < prevStart
                   && ChatRenderer::isCondensable(ch->messages[j], selfNick)
                   && ch->messages[j].timestamp.toLocalTime().date() == day)
                ++j;
            QList<Message> group(ch->messages.cbegin() + i, ch->messages.cbegin() + j);
            const QString groupId    = QString::number(group.first().timestamp.toMSecsSinceEpoch());
            const bool    grpExpanded = m_expandedEventGroups.contains(groupId);
            older.append(ChatRenderer::formatEventGroupLine(group, ctx, groupId, grpExpanded));
            i = j;
        } else {
            older.append(ChatRenderer::formatMessageLine(msg, ctx));
            if (!msg.msgid.isEmpty()) {
                auto rxIt = ch->reactions.constFind(msg.msgid);
                if (rxIt != ch->reactions.constEnd() && !rxIt->isEmpty())
                    older.append(ChatRenderer::buildReactionLine(*rxIt, msg.msgid));
            }
            ++i;
        }
    }

    // Remove the existing "older messages" sentinel before prepending new batch
    m_chatView->removeLine("status:older");
    m_chatView->prependLines(std::move(older));

    QTimer::singleShot(0, this, [this]{ m_loadingOlder = false; });
}

void MainWindow::onOlderHistoryLoaded(ServerId host, BufferId channel, int count)
{
    m_loadingOlder = false;

    if (host != m_model->activeHost() || channel != m_model->activeChannel())
        return;

    const QString key = host.str() + '\t' + channel.str();
    if (count <= 0) {
        m_historyExhausted.insert(key);
        return;
    }

    auto *ch = m_model->channel(host, channel);
    if (!ch) return;

    m_renderStart[key] = 0;

    ChatRenderer::Context ctx;
    ctx.coloredNicks   = m_config.ui.coloredNicks;
    ctx.nickBrackets   = m_config.ui.nickBrackets;
    ctx.emojiPt        = m_config.ui.fontSizes.emoji;
    ctx.chatPt         = m_config.ui.fontSizes.chat;
    ctx.validTheme     = m_theme.valid;
    ctx.themeText      = m_theme.text;
    ctx.selfNickRe     = m_selfNickRe;
    ctx.highlightRe    = m_highlightRe;
    ctx.showTimestamps = m_config.ui.showTimestamps;
    ctx.channel        = ch;

    const QString selfNick = m_model->selfNick(host);
    QList<ChatLine> older;
    const int end = qMin(count, static_cast<int>(ch->messages.size()));

    for (int i = 0; i < end; ) {
        const auto &msg = ch->messages[i];
        if (ChatRenderer::isCondensable(msg, selfNick)) {
            const QDate day = msg.timestamp.toLocalTime().date();
            int j = i + 1;
            while (j < end
                   && ChatRenderer::isCondensable(ch->messages[j], selfNick)
                   && ch->messages[j].timestamp.toLocalTime().date() == day)
                ++j;
            QList<Message> group(ch->messages.cbegin() + i, ch->messages.cbegin() + j);
            const QString groupId    = QString::number(group.first().timestamp.toMSecsSinceEpoch());
            const bool    grpExpanded = m_expandedEventGroups.contains(groupId);
            older.append(ChatRenderer::formatEventGroupLine(group, ctx, groupId, grpExpanded));
            i = j;
        } else {
            older.append(ChatRenderer::formatMessageLine(msg, ctx));
            if (!msg.msgid.isEmpty()) {
                auto rxIt = ch->reactions.constFind(msg.msgid);
                if (rxIt != ch->reactions.constEnd() && !rxIt->isEmpty())
                    older.append(ChatRenderer::buildReactionLine(*rxIt, msg.msgid));
            }
            ++i;
        }
    }

    m_chatView->removeLine("status:older");
    m_chatView->prependLines(std::move(older));
}

static QString topicAgeStr(quint64 ts)
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

void MainWindow::doSearch(bool backward)
{
    const QString text = m_searchInput ? m_searchInput->text() : QString{};
    if (text.isEmpty()) { m_chatView->clearFind(); return; }
    m_chatView->findText(text, backward);
}

void MainWindow::showSearchBar()
{
    m_searchBar->show();
    m_searchInput->setFocus();
    m_searchInput->selectAll();
}

void MainWindow::clearReplyBar()
{
    m_pendingReplyMsgid.clear();
    if (m_replyLabel) m_replyLabel->setText({});
    if (m_replyBar)   m_replyBar->hide();
}

void MainWindow::appendMessage(const Message &msg, bool autoPreview)
{
    const ServerId host    = m_model->activeHost();
    const BufferId channel = m_model->activeChannel();
    auto *ch = m_model->channel(m_model->activeHost(), m_model->activeChannel());

    ChatRenderer::Context ctx;
    ctx.coloredNicks = m_config.ui.coloredNicks;
    ctx.nickBrackets = m_config.ui.nickBrackets;
    ctx.emojiPt      = m_config.ui.fontSizes.emoji;
    ctx.chatPt       = m_config.ui.fontSizes.chat;
    ctx.validTheme   = m_theme.valid;
    ctx.themeText    = m_theme.text;
    ctx.selfNickRe   = m_selfNickRe;
    ctx.highlightRe  = m_highlightRe;
    ctx.showTimestamps = m_config.ui.showTimestamps;
    ctx.channel      = ch;

    m_chatView->appendLine(ChatRenderer::formatMessageLine(msg, ctx));

    const bool isText = (msg.type == MessageType::Privmsg ||
                         msg.type == MessageType::Action  ||
                         msg.type == MessageType::Notice);
    if (autoPreview && isText) {
        static const QRegularExpression urlRe(
            R"(https?://[^ \t\r\n<>"]+)",
            QRegularExpression::CaseInsensitiveOption);
        auto it = urlRe.globalMatch(msg.text);
        while (it.hasNext()) {
            const QString urlStr = QUrl(it.next().captured(0)).toString();
            if (urlStr.isEmpty()) continue;
            if (ch) {
                const auto p = ch->previews.constFind(urlStr);
                if (p != ch->previews.constEnd() && !ch->hiddenPreviews.contains(urlStr)) {
                    ChatLine card;
                    card.role   = ChatLineRole::PreviewCard;
                    card.id     = "preview:" + urlStr;
                    if (!p->pngData.isEmpty()) card.image.loadFromData(p->pngData, "PNG");
                    card.text   = p->title + "\n" + p->domain;
                    ChatSegment seg; seg.start = 0; seg.length = static_cast<int>(card.text.size());
                    seg.anchor = "preview:" + urlStr;
                    card.segments.append(seg);
                    m_chatView->appendLine(card);
                    continue;
                }
            }
            if (!m_previewChannels.contains(urlStr))
                enqueuePreview(QUrl(urlStr), host, channel, msg.msgid);
        }
    }

    if (m_chatView->isAtBottom()) m_chatView->scrollToBottom();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_tray && m_tray->isVisible()) {
        hide();
        event->ignore();
    } else {
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

QString MainWindow::formatMessage(const Message &msg) const
{
    ChatRenderer::Context ctx;
    ctx.coloredNicks = m_config.ui.coloredNicks;
    ctx.nickBrackets = m_config.ui.nickBrackets;
    ctx.emojiPt      = m_config.ui.fontSizes.emoji;
    ctx.chatPt       = m_config.ui.fontSizes.chat;
    ctx.validTheme   = m_theme.valid;
    ctx.themeText    = m_theme.text;
    ctx.selfNickRe   = m_selfNickRe;
    ctx.highlightRe  = m_highlightRe;
    ctx.showTimestamps = m_config.ui.showTimestamps;
    ctx.channel      = m_model->channel(m_model->activeHost(), m_model->activeChannel());
    return ChatRenderer::formatMessage(msg, ctx);
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

