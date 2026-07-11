// One-time UI construction and signal wiring: toolbar, preferences dialog,
// sidebar, nick panel, chat area and model connections. Split out of
// mainwindow.cpp - all methods remain MainWindow members.

#if defined(__linux__) && !defined(__MUSL__)
#include <malloc.h>
#endif
#include "mainwindow.h"
#include "ui/mainwindowdelegates.h"
#include "ui/commanddispatcher.h"
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
#include "ui/uistyle.h"
#include "ui/searchbar.h"
#include "ui/nickfilteredit.h"
#include "ui/linkpreview.h"
#include "ui/previewcontroller.h"
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
    m_hamburger->setStyleSheet(UiStyle::headerButtonStyle());
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
                m_primaryTopicBtn->setIcon(MenuIcons::topicBubble(
                    QColor(on ? m_theme.accent : m_theme.placeholder)));
            }
            for (auto *pane : std::as_const(m_panes)) {
                auto *nd = new NickDelegate(pane->nickList());
                nd->setColors(QColor(m_theme.accent),
                              QColor(m_theme.border),
                              QColor(m_theme.text));
                pane->nickList()->setItemDelegate(nd);
                pane->setTopicIcon(
                    MenuIcons::topicBubble(QColor(m_theme.placeholder)),
                    MenuIcons::topicBubble(QColor(m_theme.accent)));
                const QColor ic(m_theme.text);
                pane->setSearchIcon(MenuIcons::fromSvg(QStringLiteral(":/icons/mi-search.svg"), ic, 20));
                pane->setPopOutIcon(MenuIcons::pipEnter(ic));
            }
        }
        if (m_sidebarCloseBtn)
            m_sidebarCloseBtn->setStyleSheet(UiStyle::headerButtonStyle());
        if (m_sidebarRevealBtn)
            m_sidebarRevealBtn->setStyleSheet(UiStyle::headerButtonStyle());
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
        for (auto *pane : std::as_const(m_panes)) {
            pane->setTyping("");
            pane->setTypingEnabled(on);
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

    connect(m_prefsDialog, &PreferencesDialog::paneStackRowsToggled, this, [this](bool on){
        m_config.ui.paneStackRows = on;
        saveConfig();
        rebuildPaneLayout();
    });

    connect(m_prefsDialog, &PreferencesDialog::timestampsToggled, this, [this](bool on){
        m_config.ui.showTimestamps = on;
        saveConfig();
        refreshChatView(m_model->activeHost(), m_model->activeChannel());
    });

    connect(m_prefsDialog, &PreferencesDialog::highlightWordsChanged, this, [this](const QString &words){
        m_config.ui.highlightWords = words;
        saveConfig();
        m_highlightRe = SessionModel::buildHighlightRe(words);
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
    m_sidebarToggleBtn->setStyleSheet(UiStyle::headerButtonStyle());
    m_sidebarToggleBtn->setObjectName("sidebarToggleBtn");
    m_sidebarToggleBtn->setToolTip(tr("Preferences"));
    m_sidebarToggleBtn->setIcon(MenuIcons::gear(QColor(m_theme.valid ? m_theme.text : "#ffffff")));
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
        m_serversBtn->setStyleSheet(UiStyle::headerButtonStyle());
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
    m_sidebarCloseBtn->setStyleSheet(UiStyle::headerButtonStyle());
    m_sidebarCloseBtn->setToolTip(tr("Hide channel list"));
    m_sidebarCloseBtn->setIcon(MenuIcons::fromSvg(
        QStringLiteral(":/icons/mi-left-panel-close.svg"),
        QColor(m_theme.valid ? m_theme.text : "#e3e3e3"), 20));
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
    m_nickGroupsIconLabel->setPixmap(MenuIcons::groups(QColor(m_theme.valid ? m_theme.text : "#e3e3e3"), 20));
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
    m_nickToggleBtn->setIcon(MenuIcons::fromSvg(
        QStringLiteral(":/icons/mi-right-panel-close.svg"),
        QColor(m_theme.valid ? m_theme.text : "#e3e3e3"), 20));

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

    m_nickFilter = new NickFilterEdit(m_nickList);

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
    m_primaryPanel->setAcceptDrops(true);
    m_primaryPanel->installEventFilter(this);
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
        m_primaryTopicBtn->setIcon(MenuIcons::topicBubble(
            QColor(m_theme.valid ? m_theme.placeholder : "#888888")));
        m_primaryTopicBtn->setIconSize(QSize(14, 14));
        m_primaryTopicBtn->setAutoRaise(false);
        connect(m_primaryTopicBtn, &QToolButton::toggled, this, [this](bool on){
            const int scrollPos = m_nickList ? m_nickList->verticalScrollBar()->value() : 0;
            const int sbScroll  = m_sidebar  ? m_sidebar->verticalScrollBar()->value()  : 0;
            m_topicDisplay->setVisible(on);
            if (m_theme.valid)
                m_primaryTopicBtn->setIcon(MenuIcons::topicBubble(
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
        m_searchBtn->setStyleSheet(UiStyle::headerButtonStyle());
        m_searchBtn->setToolTip(tr("Search (Ctrl+F)"));
        m_searchBtn->setIcon(MenuIcons::fromSvg(
            QStringLiteral(":/icons/mi-search.svg"),
            QColor(m_theme.valid ? m_theme.text : "#e3e3e3"), 20));
        connect(m_searchBtn, &QToolButton::clicked, this, [this]{
            if (m_searchBar->isVisible()) m_searchBar->dismiss();
            else m_searchBar->open();
        });

        m_popOutBtn = new QToolButton;
        m_popOutBtn->setFixedSize(28, 28);
        m_popOutBtn->setIconSize(QSize(24, 24));
        m_popOutBtn->setAutoRaise(true);
        m_popOutBtn->setStyleSheet(UiStyle::headerButtonStyle());
        m_popOutBtn->setToolTip(tr("Open this channel in a window"));
        m_popOutBtn->setIcon(MenuIcons::pipEnter(
            QColor(m_theme.valid ? m_theme.text : "#e3e3e3")));
        connect(m_popOutBtn, &QToolButton::clicked, this, [this]{
            const ServerId host = m_model->activeHost();
            const BufferId ch   = m_model->activeChannel();
            const QString c = ch.str();
            if (isChannelName(c))
                popOutChannel(host, ch);
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
        hbox->addWidget(m_popOutBtn);
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
        if (m_config.ui.linkPreviews) m_previews->linkPreview()->fetchHover(url);
    });
    connect(m_chatView, &ChatView::loadOlderRequested, this, &MainWindow::loadOlderMessages);
    m_chatView->installEventFilter(this);
    m_chatView->viewport()->installEventFilter(this);

    m_previews = new PreviewController(m_model, this);

    connect(m_previews->linkPreview(), &LinkPreview::titleReady, this, [this](const QUrl &url, const QString &title){
        if (url.toString() != m_hoveredUrl) return;
        const QString display = title.length() > 80 ? title.left(79) + QChar(0x2026) : title;
        statusBar()->showMessage(display);
        QToolTip::showText(m_hoverGlobalPos, display, m_chatView->viewport());
    });

    connect(m_previews, &PreviewController::cardStored, this,
            [this](ServerId host, BufferId channel, const QString &msgid,
                   const QString &urlStr, const QPixmap &thumb){
        auto *ch = m_model->channel(host, channel);
        if (!ch) return;
        const auto p = ch->previews.constFind(urlStr);
        if (p == ch->previews.constEnd()) return;

        auto makeCardLine = [&]() -> ChatLine {
            ChatLine line;
            line.id   = "preview:" + urlStr;
            line.role = ChatLineRole::PreviewCard;
            line.image = thumb;
            line.text = p->title + "\n" + p->domain;
            QTextCharFormat titleFmt;
            titleFmt.setFontWeight(QFont::Bold);
            ChatSegment titleSeg;
            titleSeg.start  = 0;
            titleSeg.length = static_cast<int>(p->title.size());
            titleSeg.format = titleFmt;
            titleSeg.anchor = "preview:" + urlStr;
            line.segments.append(titleSeg);
            QTextCharFormat domainFmt;
            domainFmt.setForeground(QColor("#888888"));
            ChatSegment domainSeg;
            domainSeg.start  = static_cast<int>(p->title.size()) + 1;
            domainSeg.length = static_cast<int>(p->domain.size());
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

        const QString key = paneKey(host, channel);
        if (auto *pane = m_panes.value(key)) {
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
    m_nickRevealBtn->setStyleSheet(UiStyle::headerButtonStyle());
    m_nickRevealBtn->setToolTip(tr("Show user list"));
    m_nickRevealBtn->setIcon(MenuIcons::fromSvg(
        QStringLiteral(":/icons/mi-left-panel-close.svg"),
        QColor(m_theme.valid ? m_theme.text : "#e3e3e3"), 20));
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
    m_scrollBottomBtn->setIcon(MenuIcons::fromSvg(
        QStringLiteral(":/icons/mi-keyboard-double-arrow-down.svg"),
        QColor("#e3e3e3"), 20));
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
    m_sidebarRevealBtn->setStyleSheet(UiStyle::headerButtonStyle());
    m_sidebarRevealBtn->setToolTip(tr("Show channel list"));
    m_sidebarRevealBtn->setIcon(MenuIcons::fromSvg(
        QStringLiteral(":/icons/mi-right-panel-open.svg"),
        QColor(m_theme.valid ? m_theme.text : "#e3e3e3"), 20));
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

    // m_primaryPanel is self-contained (just like any other ChannelPane) —
    // no sidebar inside it — so it can safely land in any pane slot the
    // pane splitter builds, including a shared/stacked one.
    primaryVbox->addWidget(chatSection, 1);

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

    // The sidebar lives one level above the pane splitter, as a permanent
    // sibling of it, so it's always present no matter how panes (including
    // the primary pane) get rearranged.
    m_mainSplitter = new QSplitter(Qt::Horizontal);
    m_mainSplitter->setHandleWidth(0);
    m_mainSplitter->addWidget(m_sidebarPanel);
    m_mainSplitter->addWidget(chatWrapper);
    m_mainSplitter->setStretchFactor(0, 0);
    m_mainSplitter->setStretchFactor(1, 1);
    m_mainSplitter->setMinimumSize(1, 1);
    vbox->addWidget(m_mainSplitter, 1);

    setCentralWidget(m_rightContent);
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
                    MenuIcons::fromSvg(QStringLiteral(":/icons/mi-do-not-disturb.svg"),
                                QColor("#e06c75"), 20)));
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

    m_dcc = new DccController(m_model, this);

    connect(m_model, &SessionModel::pingRtt, this, [this](ServerId host, int ms){
        if (m_signalBars && host == m_model->activeHost())
            m_signalBars->setLatency(ms);
    });
    connect(m_model, &SessionModel::serverReconnecting, this, [this](ServerId host){
        if (m_signalBars && host == m_model->activeHost())
            m_signalBars->setState(SignalBars::State::Reconnecting);
    });
}
