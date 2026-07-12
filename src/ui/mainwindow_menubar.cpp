// Menu bar (menu_style = "menubar"/"hidden"): persistent window-level
// actions, QMenuBar construction, and the shared setting appliers used by
// both the Preferences dialog and the menus. Split out of mainwindow.cpp -
// all methods remain MainWindow members.

#include "mainwindow.h"
#include "ui/mainwindowdelegates.h"
#include "ui/commanddispatcher.h"
#include "irc/ircclient.h"
#include "ui/aboutdialog.h"
#include "ui/docsdialog.h"
#include "ui/preferencesdialog.h"
#include "ui/serverdialog.h"
#include "ui/manageserversdialog.h"
#include "ui/ignorelistdialog.h"
#include "ui/fontdialog.h"
#include "ui/themeloader.h"
#include "ui/menuicons.h"
#include "ui/signalbars.h"
#include "ui/searchbar.h"
#include "ui/quickswitcher.h"
#include "ui/updatechecker.h"
#include "ui/channelpane.h"
#include "ui/chatview.h"
#include "config/config.h"
#include "model/ids.h"

#include <QApplication>
#include <QAction>
#include <QFont>
#include <QMenu>
#include <QMenuBar>
#include <QToolButton>
#include <QHBoxLayout>
#include <QSplitter>
#include <QScrollBar>
#include <QTreeWidget>
#include <QListWidget>
#include <QStyle>
#include <QTimer>
#include <QDesktopServices>
#include <QProcess>
#include <QUrl>

// ---------------------------------------------------------------------------
// Persistent actions — created once, parented to the window and addAction()ed
// so their shortcuts fire in all three menu styles. One owner per key: these
// QActions are the only registration for their shortcuts (the old QShortcuts
// were removed) — adding a QShortcut for the same key would make it ambiguous.
// ---------------------------------------------------------------------------

void MainWindow::setupMenuActions()
{
    m_actFind = new QAction(tr("Find in Buffer"), this);
    m_actFind->setShortcut(QKeySequence::Find);
    connect(m_actFind, &QAction::triggered, this, [this]{
        // Focus inside a docked pane → toggle that pane's own search bar.
        if (QWidget *fw = QApplication::focusWidget())
            for (auto *pane : std::as_const(m_panes))
                if (pane->window() == this && pane->isAncestorOf(fw)) {
                    pane->toggleSearch();
                    return;
                }
        m_searchBar->open();
    });
    addAction(m_actFind);

    m_actHistorySearch = new QAction(tr("Search All History"), this);
    m_actHistorySearch->setShortcut(QKeySequence("Ctrl+Shift+F"));
    connect(m_actHistorySearch, &QAction::triggered, this, &MainWindow::openLogSearch);
    addAction(m_actHistorySearch);

    m_actQuickSwitch = new QAction(tr("Quick Switcher"), this);
    m_actQuickSwitch->setShortcut(QKeySequence("Ctrl+K"));
    connect(m_actQuickSwitch, &QAction::triggered, this, [this]{
        if (m_quickSwitcher) m_quickSwitcher->showCentered();
    });
    addAction(m_actQuickSwitch);

    // The input event filter keeps its own Ctrl+Shift+K branch for popped-out
    // pane windows, where this window-scoped action doesn't apply.
    m_actInsertColor = new QAction(tr("Insert Color…"), this);
    m_actInsertColor->setShortcut(QKeySequence("Ctrl+Shift+K"));
    connect(m_actInsertColor, &QAction::triggered, this, &MainWindow::showColorPicker);
    addAction(m_actInsertColor);

    m_actQuit = new QAction(tr("Quit"), this);
    m_actQuit->setShortcut(QKeySequence("Ctrl+Q"));
    connect(m_actQuit, &QAction::triggered, qApp, &QCoreApplication::quit);
    addAction(m_actQuit);

    // Escape hatch: in hidden mode (and in menubar mode without a visible
    // menu host) this is the only way back into Preferences.
    m_actPreferences = new QAction(tr("Preferences…"), this);
    m_actPreferences->setShortcut(QKeySequence("Ctrl+,"));
    connect(m_actPreferences, &QAction::triggered, this, &MainWindow::openPreferences);
    addAction(m_actPreferences);

    // Checkable view toggles — shared by the View/Window menus; check state
    // is refreshed both from the appliers and on menu aboutToShow.
    m_actViewSidebar = new QAction(tr("Server/Channel List"), this);
    m_actViewSidebar->setCheckable(true);
    connect(m_actViewSidebar, &QAction::triggered, this, &MainWindow::setSidebarVisible);

    m_actViewUserList = new QAction(tr("User List"), this);
    m_actViewUserList->setCheckable(true);
    connect(m_actViewUserList, &QAction::triggered, this, &MainWindow::setNickPanelVisible);

    m_actViewTopic = new QAction(tr("Topic Bar"), this);
    m_actViewTopic->setCheckable(true);
    connect(m_actViewTopic, &QAction::triggered, this, &MainWindow::applyTopicBarSetting);

    m_actViewTimestamps = new QAction(tr("Timestamps"), this);
    m_actViewTimestamps->setCheckable(true);
    connect(m_actViewTimestamps, &QAction::triggered, this, &MainWindow::applyTimestampsSetting);

    m_actViewUnread = new QAction(tr("Unread Count Badges"), this);
    m_actViewUnread->setCheckable(true);
    connect(m_actViewUnread, &QAction::triggered, this, &MainWindow::applyUnreadCountsSetting);

    m_actViewCards = new QAction(tr("Panel Cards"), this);
    m_actViewCards->setCheckable(true);
    connect(m_actViewCards, &QAction::triggered, this, &MainWindow::applyPanelCardsSetting);

    m_actStackRows = new QAction(tr("Stack Panes in Rows"), this);
    m_actStackRows->setCheckable(true);
    connect(m_actStackRows, &QAction::triggered, this, &MainWindow::applyPaneStackRowsSetting);
}

// ---------------------------------------------------------------------------
// Menu bar — built on entering menubar mode, destroyed on leaving it (a
// hidden QMenuBar can stay exported to the KDE global menu, so no hide()).
// ---------------------------------------------------------------------------

void MainWindow::buildMenuBar()
{
    if (m_menuBarWidget) return;
    m_menuBarWidget = new QMenuBar(this);
    const QColor ic(m_theme.valid ? m_theme.text : "#ffffff");

    // ── File ──────────────────────────────────────────────────────────────
    auto *fileMenu = m_menuBarWidget->addMenu(tr("&File"));
    fileMenu->addAction(MenuIcons::servers(ic), tr("Add Server…"), this, [this]{
        ServerDialog dlg(this);
        if (dlg.exec() != QDialog::Accepted) return;
        const ServerConfig sc = dlg.serverConfig();
        if (sc.host.isEmpty()) return;
        m_config.servers.append(sc);
        m_model->addServer(sc);
        saveConfig(true);
        syncSidebarOrderFromConfig();
    });
    fileMenu->addAction(MenuIcons::manageServers(ic), tr("Manage Servers…"),
                        this, &MainWindow::openManageServers);
    fileMenu->addSeparator();
    QAction *connAct = fileMenu->addAction(MenuIcons::connectedServer(ic), tr("Disconnect"));
    connect(connAct, &QAction::triggered, this, [this]{
        auto *sess = m_model->session(m_model->activeHost());
        auto *cl   = m_model->clientFor(m_model->activeHost());
        if (!sess || !cl) return;
        if (sess->connected) cl->quit();
        else                 cl->reconnect();
    });
    fileMenu->addSeparator();
    fileMenu->addAction(tr("Open Config"), this, []{
        QDesktopServices::openUrl(QUrl::fromLocalFile(Config::defaultPath()));
    });
    fileMenu->addAction(tr("Reload Config"), this, []{
        // arguments() includes argv[0]; passing it verbatim would add a
        // duplicate program path to the child's argv on every reload.
        QProcess::startDetached(QCoreApplication::applicationFilePath(),
                                QCoreApplication::arguments().mid(1));
        QCoreApplication::quit();
    });
    fileMenu->addSeparator();
    m_actQuit->setIcon(MenuIcons::exit(ic));
    fileMenu->addAction(m_actQuit);
    connect(fileMenu, &QMenu::aboutToShow, this, [this, connAct]{
        auto *sess = m_model->session(m_model->activeHost());
        connAct->setEnabled(sess != nullptr);
        connAct->setText(sess && sess->connected ? tr("Disconnect") : tr("Connect"));
    });

    // ── Edit ──────────────────────────────────────────────────────────────
    auto *editMenu = m_menuBarWidget->addMenu(tr("&Edit"));
    // WidgetShortcut: the key sequence renders in the menu but never fires
    // globally — widgets keep their native Ctrl+X/C/V handling (ChatView
    // copies from its own keyPressEvent).
    auto addFocusEdit = [&](const QString &text, QKeySequence::StandardKey key,
                            const char *member){
        QAction *a = editMenu->addAction(text);
        a->setShortcut(key);
        a->setShortcutContext(Qt::WidgetShortcut);
        connect(a, &QAction::triggered, this, [member]{
            if (QWidget *fw = QApplication::focusWidget())
                QMetaObject::invokeMethod(fw, member);
        });
    };
    addFocusEdit(tr("Cut"),   QKeySequence::Cut,   "cut");
    addFocusEdit(tr("Copy"),  QKeySequence::Copy,  "copy");
    addFocusEdit(tr("Paste"), QKeySequence::Paste, "paste");
    editMenu->addSeparator();
    m_actInsertColor->setIcon(MenuIcons::coloredNicks(ic));
    editMenu->addAction(m_actInsertColor);
    editMenu->addSeparator();
    editMenu->addAction(tr("Clear Buffer"), this, &MainWindow::clearActiveBuffer);
    editMenu->addAction(MenuIcons::eyeOff(ic), tr("Ignore List…"),
                        this, &MainWindow::openIgnoreList);

    // ── View ──────────────────────────────────────────────────────────────
    auto *viewMenu = m_menuBarWidget->addMenu(tr("&View"));
    viewMenu->addAction(m_actViewSidebar);
    viewMenu->addAction(m_actViewUserList);
    viewMenu->addAction(m_actViewTopic);
    viewMenu->addSeparator();
    viewMenu->addAction(m_actViewTimestamps);
    viewMenu->addAction(m_actViewUnread);
    viewMenu->addAction(m_actViewCards);
    viewMenu->addSeparator();
    viewMenu->addAction(tr("Increase Font Size"), this, [this]{ zoomFont(m_chatView, +0.5); });
    viewMenu->addAction(tr("Decrease Font Size"), this, [this]{ zoomFont(m_chatView, -0.5); });
    connect(viewMenu, &QMenu::aboutToShow, this, [this]{
        m_actViewSidebar->setChecked(m_sidebarExpanded);
        m_actViewUserList->setChecked(m_nickExpanded);
        m_actViewTopic->setChecked(m_topicDisplay && m_topicDisplay->isVisible());
        m_actViewTimestamps->setChecked(m_config.ui.showTimestamps);
        m_actViewUnread->setChecked(m_config.ui.showUnreadCounts);
        m_actViewCards->setChecked(m_config.ui.panelCards);
    });

    // ── Window ────────────────────────────────────────────────────────────
    auto *winMenu = m_menuBarWidget->addMenu(tr("&Window"));
    QAction *paneAct = winMenu->addAction(tr("Open Channel in Pane"), this, [this]{
        openChannelPane(m_model->activeHost(), m_model->activeChannel());
    });
    QAction *popAct = winMenu->addAction(MenuIcons::pipEnter(ic), tr("Pop Out Channel"), this, [this]{
        popOutChannel(m_model->activeHost(), m_model->activeChannel());
    });
    winMenu->addSeparator();
    winMenu->addAction(m_actStackRows);
    connect(winMenu, &QMenu::aboutToShow, this, [this, paneAct, popAct]{
        const bool isChan = isChannelName(m_model->activeChannel().str());
        paneAct->setEnabled(isChan);
        popAct->setEnabled(isChan);
        m_actStackRows->setChecked(m_config.ui.paneStackRows);
    });

    // ── Bookmarks ─────────────────────────────────────────────────────────
    // Rebuilt on every open (same pattern as Plugins): "Bookmark This
    // Channel" toggles the active channel's presence in its server's
    // auto-join list; below it, saved channels are listed per network.
    auto *bmMenu = m_menuBarWidget->addMenu(tr("&Bookmarks"));
    connect(bmMenu, &QMenu::aboutToShow, this, [this, bmMenu, ic]{
        bmMenu->clear();
        const ServerId host    = m_model->activeHost();
        const BufferId channel = m_model->activeChannel();
        const QString  chan    = channel.str();
        // Space/comma can't appear in valid channel names and would corrupt
        // the compact channels = "#a, #b" config form.
        const bool bookmarkable = isChannelName(chan)
                                  && !chan.contains(' ') && !chan.contains(',');
        const bool saved = bookmarkable && isBookmarked(host, channel);
        QAction *bm = bmMenu->addAction(MenuIcons::bookmark(ic),
            saved ? tr("Remove Bookmark") : tr("Bookmark This Channel"));
        bm->setEnabled(bookmarkable);
        connect(bm, &QAction::triggered, this, [this, host, channel, saved]{
            toggleBookmark(host, channel, !saved);
        });
        bmMenu->addSeparator();

        bool any = false;
        for (const auto &sc : std::as_const(m_config.servers)) {
            if (sc.channels.isEmpty()) continue;
            any = true;
            auto *sub = bmMenu->addMenu(sc.name);
            auto *sess = m_model->session(ServerId{sc.name});
            sub->setEnabled(sess && sess->connected);   // JOIN is a silent no-op offline
            for (const auto &cc : sc.channels) {
                connect(sub->addAction(cc.name), &QAction::triggered, this,
                        [this, srv = sc.name, name = cc.name]{
                    joinBookmark(ServerId{srv}, name);
                });
            }
        }
        if (!any)
            bmMenu->addAction(tr("No bookmarked channels"))->setEnabled(false);
    });

    // ── Plugins ───────────────────────────────────────────────────────────
    auto *plugMenu = m_menuBarWidget->addMenu(tr("&Plugins"));
    connect(plugMenu, &QMenu::aboutToShow, this, [this, plugMenu, ic]{
        plugMenu->clear();
        plugMenu->addAction(MenuIcons::scripts(ic), tr("Manage Scripts…"), this, [this]{
            openPreferences();
            if (m_prefsDialog) m_prefsDialog->showScriptsPage();
        });
        plugMenu->addSeparator();
        const bool haveBuffer = !m_model->activeHost().str().isEmpty();
        for (const auto &sb : std::as_const(m_config.scripts)) {
            if (!sb.enabled) continue;
            QAction *a = plugMenu->addAction(QStringLiteral("/") + sb.command);
            a->setEnabled(haveBuffer);
            connect(a, &QAction::triggered, this, [this, cmd = sb.command]{
                m_dispatcher->dispatch(QStringLiteral("/") + cmd,
                                       m_model->activeHost(), m_model->activeChannel(),
                                       QString());
            });
        }
    });

    // ── Settings ──────────────────────────────────────────────────────────
    auto *setMenu = m_menuBarWidget->addMenu(tr("&Settings"));
    m_actPreferences->setIcon(MenuIcons::preferences(ic));
    setMenu->addAction(m_actPreferences);
    setMenu->addSeparator();
    setMenu->addAction(MenuIcons::theme(ic), tr("Themes…"), this, [this]{
        openPreferences();
        m_prefsDialog->showPage(QStringLiteral("Appearance"));
        m_prefsDialog->setThemeListExpanded(true);
    });
    setMenu->addAction(MenuIcons::appIcon(ic), tr("App Icon…"), this, [this]{
        openPreferences();
        m_prefsDialog->showPage(QStringLiteral("Appearance"));
    });
    setMenu->addAction(MenuIcons::fontConfig(ic), tr("Fonts…"),
                       this, &MainWindow::openFontConfig);
    setMenu->addAction(MenuIcons::gear(ic), tr("Profile…"), this, [this]{
        openPreferences();
        m_prefsDialog->showPage(QStringLiteral("Profile"));
    });

    // ── Help ──────────────────────────────────────────────────────────────
    auto *helpMenu = m_menuBarWidget->addMenu(tr("&Help"));
    helpMenu->addAction(MenuIcons::documentation(ic), tr("Documentation"), this, [this]{
        if (!m_docsDialog) m_docsDialog = new DocsDialog(this);
        m_docsDialog->show();
        m_docsDialog->raise();
        m_docsDialog->activateWindow();
    });
    helpMenu->addAction(tr("Keyboard Shortcuts"), this, [this]{
        if (!m_docsDialog) m_docsDialog = new DocsDialog(this);
        m_docsDialog->showTab(QStringLiteral("Shortcuts"));
        m_docsDialog->show();
        m_docsDialog->raise();
        m_docsDialog->activateWindow();
    });
    helpMenu->addAction(MenuIcons::checkForUpdates(ic), tr("Check for Updates"), this, [this]{
        if (!m_updateChecker) m_updateChecker = new UpdateChecker(this);
        m_updateChecker->check();
    });
    helpMenu->addSeparator();
    helpMenu->addAction(MenuIcons::about(ic), tr("About Uplink"), this, [this]{
        if (!m_aboutDialog) m_aboutDialog = new AboutDialog(this);
        m_aboutDialog->showCentered();
    });

    // ── Search ────────────────────────────────────────────────────────────
    auto *searchMenu = m_menuBarWidget->addMenu(tr("Sea&rch"));
    searchMenu->addAction(m_actFind);
    searchMenu->addAction(m_actHistorySearch);
    searchMenu->addAction(m_actQuickSwitch);
    searchMenu->addSeparator();
    searchMenu->addAction(tr("Channel List"), this, [this]{
        openChannelList(m_model->activeHost());
    });

    setMenuBar(m_menuBarWidget);
}

// ---------------------------------------------------------------------------
// Mode applier — idempotent; called at startup and on live setting changes.
// ---------------------------------------------------------------------------

void MainWindow::applyMenuStyle()
{
    if (m_config.ui.menuStyle == QLatin1String("hidden")) {
        if (m_menuBarWidget) {
            setMenuBar(nullptr);  // deletes the old bar; actions survive (parented to this)
            m_menuBarWidget = nullptr;
        }
    } else {
        buildMenuBar();
    }
}

// ---------------------------------------------------------------------------
// Dialog launchers
// ---------------------------------------------------------------------------

void MainWindow::openPreferences()
{
    if (!m_prefsDialog) {
        m_prefsDialog = new PreferencesDialog(m_config, this);
        connectPreferences();
    }
    m_prefsDialog->show();
    m_prefsDialog->raise();
    m_prefsDialog->activateWindow();
}

void MainWindow::openManageServers()
{
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
}

void MainWindow::openFontConfig()
{
    FontDialog dlg(m_config.ui.fontFamily, m_config.ui.fontSizes, this);
    if (dlg.exec() != QDialog::Accepted) return;
    m_config.ui.fontFamily = dlg.selectedFamily();
    m_config.ui.fontSizes  = dlg.selectedSizes();
    saveConfig();
    QFont appFont(m_config.ui.fontFamily);
    appFont.setStyleHint(QFont::Monospace);
    QApplication::setFont(appFont);
    applyFontSizes();
}

// ---------------------------------------------------------------------------
// Bookmarks — the per-server auto-join list surfaced as a menu. Config edits
// are in-place + saveConfig() only; SessionModel::updateServer() would tear
// down and reconnect the live session, so it is never used here.
// ---------------------------------------------------------------------------

bool MainWindow::isBookmarked(ServerId host, BufferId channel) const
{
    const QString name = channel.str().toLower();
    for (const auto &sc : m_config.servers) {
        if (sc.name != host.str()) continue;
        for (const auto &cc : sc.channels)
            if (cc.name.toLower() == name) return true;
        return false;
    }
    return false;
}

void MainWindow::toggleBookmark(ServerId host, BufferId channel, bool on)
{
    for (auto &sc : m_config.servers) {
        if (sc.name != host.str()) continue;
        const QString lower = channel.str().toLower();
        sc.channels.removeIf([&](const ChannelConfig &cc){
            return cc.name.toLower() == lower;
        });
        if (on)
            sc.channels.append(ChannelConfig{channel.str(), QString()});
        saveConfig();
        // Visible confirmation — the config write itself changes nothing on
        // screen, so say what happened in the channel that was bookmarked.
        m_model->localMessage(host, channel,
            on ? channel.str() + " added to auto-join for " + sc.name
                 + " — it will open automatically at startup"
               : channel.str() + " removed from auto-join for " + sc.name);
        return;
    }
}

void MainWindow::joinBookmark(ServerId host, const QString &channelName)
{
    // Already joined → just switch; otherwise JOIN with the stored key
    // (the switch follows automatically via channelAdded when the server
    // confirms the join).
    const BufferId channel{channelName};
    if (m_model->channel(host, channel)) {
        switchToChannel(host, channel);
        return;
    }
    auto *sess = m_model->session(host);
    if (!sess || !sess->connected) return;
    QString key;
    for (const auto &sc : m_config.servers) {
        if (sc.name != host.str()) continue;
        for (const auto &cc : sc.channels)
            if (cc.name.toLower() == channelName.toLower()) { key = cc.password; break; }
        break;
    }
    m_model->sendJoin(host, channel, key);
}

void MainWindow::openIgnoreList()
{
    IgnoreListDialog dlg(m_config.ignoreList, this);
    if (dlg.exec() != QDialog::Accepted) return;
    const QList<IgnoreEntry> updated = dlg.entries();
    for (const IgnoreEntry &old : std::as_const(m_config.ignoreList))
        m_model->clearIgnore(old.nick);
    for (const IgnoreEntry &e : updated)
        m_model->setIgnore(e.nick, e.flags);
    m_config.ignoreList = updated;
    saveConfig();
    scheduleNickRefresh(m_model->activeHost(), m_model->activeChannel());
}

// ---------------------------------------------------------------------------
// Shared setting appliers — each updates config, applies live, persists, and
// re-syncs the menu action + Preferences checkbox (aboutToShow alone isn't
// reliable over dbusmenu on the KDE global menu).
// ---------------------------------------------------------------------------

void MainWindow::applyTopicBarSetting(bool on)
{
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
    if (m_actViewTopic) m_actViewTopic->setChecked(on);
    if (m_prefsDialog)  m_prefsDialog->syncFromConfig(m_config);
}

void MainWindow::applyTimestampsSetting(bool on)
{
    m_config.ui.showTimestamps = on;
    saveConfig();
    refreshChatView(m_model->activeHost(), m_model->activeChannel());
    if (m_actViewTimestamps) m_actViewTimestamps->setChecked(on);
    if (m_prefsDialog)       m_prefsDialog->syncFromConfig(m_config);
}

void MainWindow::applyUnreadCountsSetting(bool on)
{
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
    if (m_actViewUnread) m_actViewUnread->setChecked(on);
    if (m_prefsDialog)   m_prefsDialog->syncFromConfig(m_config);
}

void MainWindow::applyPanelCardsSetting(bool on)
{
    m_config.ui.panelCards = on;
    saveConfig();
    ThemeLoader::apply(m_config.ui.theme, on);
    applyPanelChrome();
    if (m_actViewCards) m_actViewCards->setChecked(on);
    if (m_prefsDialog)  m_prefsDialog->syncFromConfig(m_config);
}

void MainWindow::applyPaneStackRowsSetting(bool on)
{
    m_config.ui.paneStackRows = on;
    saveConfig();
    rebuildPaneLayout();
    if (m_actStackRows) m_actStackRows->setChecked(on);
    if (m_prefsDialog)  m_prefsDialog->syncFromConfig(m_config);
}

// ---------------------------------------------------------------------------
// Panel visibility — shared by the header buttons and the View menu.
// ---------------------------------------------------------------------------

void MainWindow::setSidebarVisible(bool on)
{
    m_sidebarExpanded = on;
    m_sidebarPanel->setVisible(on);
    if (m_inputBar)
        if (auto *l = qobject_cast<QHBoxLayout*>(m_inputBar->layout()))
            l->setContentsMargins(on ? 4 : 40, 3, 4, 8);
    if (m_sidebarRevealBtn) {
        if (!on && m_chatSection) {
            m_sidebarRevealBtn->move(4, m_chatSection->height() - m_sidebarRevealBtn->height() - 4);
            m_sidebarRevealBtn->raise();
        }
        m_sidebarRevealBtn->setVisible(!on);
    }
    if (on && m_mainSplitter) {
        const int total = m_mainSplitter->width();
        m_mainSplitter->setSizes({m_sidebarExpandedWidth, total - m_sidebarExpandedWidth});
    }
    if (m_actViewSidebar) m_actViewSidebar->setChecked(on);
}

void MainWindow::setNickPanelVisible(bool on)
{
    m_nickExpanded = on;
    m_nickPanel->setVisible(on);
    if (m_nickRevealBtn) {
        if (!on && m_chatSection) {
            // Same line the collapse button sat on (top of the nick panel,
            // right below the header row) — not below the topic bar, which
            // made the button visually "jump down" on collapse.
            m_nickRevealBtn->move(m_chatSection->width() - m_nickRevealBtn->width() - 4,
                                  m_primaryHeader->height());
            m_nickRevealBtn->raise();
        }
        m_nickRevealBtn->setVisible(!on);
    }
    setTopicRevealInset(!on);
    if (m_actViewUserList) m_actViewUserList->setChecked(on);
}
