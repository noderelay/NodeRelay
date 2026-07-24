#pragma once

#include <QMainWindow>
#include <QFileSystemWatcher>
#include <QRegularExpression>
#include <QStringList>
#include <QColor>
#include <QHash>
#include <QMap>
#include <QPixmap>
#include <QSet>
#include <QPair>
#include "model/sessionmodel.h"
#include "config/config.h"
#include "ui/nicklistmodel.h"
#include "ui/themeloader.h"

namespace ChatRenderer { struct Context; }

class CommandDispatcher;
class SidebarDelegate;
class NickDelegate;
class TrayIcon;
class SearchBar;
class NickFilterEdit;
class SignalBars;
class AboutDialog;
class ChannelListDialog;
class DocsDialog;
class PreferencesDialog;
class EmojiPicker;
class DccController;
class PreviewController;
class SidebarController;
class TypingController;
class ChannelPane;
class DropFrame;
class QuickSwitcher;
class UpdateChecker;
class QNetworkAccessManager;
class QGraphicsOpacityEffect;
class QPropertyAnimation;

class ChatView;
class QTimer;
class QTreeWidget;
class QTreeWidgetItem;
class QPlainTextEdit;
class QLabel;
class QMenu;
class QPushButton;
class QListWidget;
class QListWidgetItem;
class QListView;
class QToolButton;
class QSplitter;
class QVBoxLayout;
class QAction;
class QMenuBar;
class ElidedLabel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(SessionModel *model, const Config &cfg, QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    // Model → UI
    void onServerAdded      (const ServerId &host);
    void onServerConnected  (const ServerId &host);
    void onServerDisconnected(const ServerId &host);
    void onServerClosed     (const ServerId &host);
    void onChannelAdded     (const ServerId &host, const BufferId &channel);
    void onChannelRemoved   (const ServerId &host, const BufferId &channel);
    void onMessageAdded     (const ServerId &host, const BufferId &channel, const Message &msg);
    void onTopicChanged     (const ServerId &host, const BufferId &channel, const QString &topic);
    void onNickListChanged     (const ServerId &host, const BufferId &channel);
    void onNickAdded           (const ServerId &host, const BufferId &channel, const QString &nick);
    void onNickRemoved         (const ServerId &host, const BufferId &channel, const QString &nick);
    void onNickRenamed         (const ServerId &host, const BufferId &channel,
                                const QString &oldNick, const QString &newNick);
    void onNickListContextMenu  (const QPoint &pos);
    void onSidebarContextMenu   (const QPoint &pos);
    void onReactionsChanged (const ServerId &host, const BufferId &channel, const QString &msgid);
    void onSelfNickChanged  (const ServerId &host, const QString &nick);
    void onMessageRedacted   (const ServerId &host, const BufferId &channel, const QString &msgid);

    // UI → Model
    void onSidebarSelectionChanged();
    void onInputSubmit();
    void dispatchInput(const QString &text, const ServerId &host, const BufferId &channel);

private:
    void setupSidebar();
    void setupChatArea();
    void setupNickPanel();
    void setupInputBar();
    void correctStartupGeometry();
    void ensureEmojiPicker();
    void connectModel();
    void connectPreferences();

    void switchToChannel(const ServerId &host, const BufferId &channel);
    void openChannelList(const ServerId &host);
    void refreshChatView(const ServerId &host, const BufferId &channel, bool resetToLatest = true);
    void loadOlderMessages();
    void onOlderHistoryLoaded(const ServerId &host, const BufferId &channel, int count);
    // Jump-to-search-result: paginate server history until the timestamp is
    // in memory, then scroll to and flash the closest message.
    void startHistoryJump(const ServerId &host, const BufferId &channel, const QDateTime &ts);
    void continueHistoryJump();
    void refreshNickList(const ServerId &host, const BufferId &channel);
    void updateNickViews(const ServerId &host, const BufferId &channel);
    void scheduleNickRefresh(const ServerId &host, const BufferId &channel);
    void refreshTopicBar(const ServerId &host, const BufferId &channel);
    void appendMessage  (const Message &msg, bool autoPreview = false);
    void appendPreviewCards(ChatView *view, const Message &msg,
                            const ServerId &host, const BufferId &channel);
    QRegularExpression selfNickReFor(const ServerId &host) const;
    void applyFontSizes();
    void applyPanelChrome();
    void applyThemeByName(const QString &name);  // visuals only; no config write
    QString effectiveThemeName() const;          // auto pair when enabled, else ui.theme
    void updateTypingLabel();
    void openChannelPane (const ServerId &host, const BufferId &channel);
    void popOutChannel   (const ServerId &host, const BufferId &channel);
    void floatPane       (ChannelPane *pane);
    void closeChannelPane(const ServerId &host, const BufferId &channel);
    void closePanesForHost(const ServerId &host);
    ChannelPane *createPane(const ServerId &host, const BufferId &channel);
    void switchAwayFromChannel(const ServerId &host, const BufferId &channel);
    void refreshPaneChatView(ChannelPane *pane);
    void refreshPaneNickList(ChannelPane *pane);
    void rebuildPaneLayout();

    // Menu bar (menu_style) — mainwindow_menubar.cpp
    void setupMenuActions();
    void buildMenuBar();
    void applyMenuStyle();
    void openPreferences();
    void openManageServers();
    void openIgnoreList();
    void openFontConfig();

    // Shared setting appliers (Preferences signals + menu actions)
    void applyTopicBarSetting(bool on);
    void applyTimestampsSetting(bool on);
    void applyUnreadCountsSetting(bool on);
    void applyPanelCardsSetting(bool on);
    void applyPaneStackRowsSetting(bool on);
    void setSidebarVisible(bool on);
    void setNickPanelVisible(bool on);
    void clearActiveBuffer();

    static QString   topicAgeStr (quint64 ts);

    QString    formatMessage(const Message &msg) const;
    void       applyThemeColors(ChatRenderer::Context &ctx) const;
    void       toggleEventGroupInView(ChatView *view, const QString &groupId,
                                      const ServerId &host, const BufferId &channel);
    void       handleChatViewContextMenu(ChatView *view, const QString &anchor,
                                         const QPoint &globalPos,
                                         const ServerId &host, const BufferId &channel);
    void       showNickContextMenu(const QString &nick, const QPoint &globalPos);
    QString    msgidAtViewPos(const QPoint &viewPos) const;
    void       openLogSearch();
    void       clearReplyBar();

    // Channel / pane navigation (Alt+arrows)
    void navigateChannel(int direction);
    void navigatePane(int direction);

    // Font zoom (Ctrl+wheel / Ctrl+±)
    double *fontFieldForWidget(QObject *obj, const QPoint &pos = {});
    bool zoomFont(QObject *target, double delta, const QPoint &pos = {});

    // Tab completion
    void handleTabComplete(QPlainTextEdit *input, const ServerId &host, const BufferId &channel,
                           bool backward = false);
    void repositionSendBtn();
    void updateInputViewportFill();
    void updateFormatIndicator();
    void updateLengthIndicator();
    void moveLengthIndicator();
    void showColorPicker();
    QMenu *makeColorMenu(QWidget *parent);   // reused by Ctrl+Shift+K and right-click
    void applyInputColor(int fg, int bg);    // -1 clears, -2 leaves unchanged
    QStringList m_tabCandidates;
    int         m_tabCandidateIndex{0};
    int         m_tabWordStart{0};
    QString     m_tabPrefix;
    bool        m_tabActive{false};

    // Emoji inline autocomplete
    void checkEmojiAutocomplete(const QString &text);
    void commitEmojiAutocomplete(int row);
    void hideEmojiAutocomplete();
    QListWidget *m_emojiCompleter{nullptr};
    int          m_emojiTriggerPos{-1};

    // Input history
    void handleHistoryUp();
    void handleHistoryDown();
    QStringList m_inputHistory;
    int         m_historyIndex{-1};
    QString     m_historyDraft;

    void syncSidebarOrderToConfig();
    void syncSidebarOrderFromConfig();
    void syncChannelOrderToConfig(const ServerId &host);

    // Widgets
    QTreeWidget      *m_sidebar;
    SidebarController *m_sidebarCtl{nullptr};
    SidebarDelegate  *m_sidebarDelegate{nullptr};
    NickDelegate     *m_nickDelegate{nullptr};
    ChatView *m_chatView;
    QPlainTextEdit *m_input{nullptr};
    QLabel       *m_nickPrefix{nullptr};
    QPushButton  *m_emojiBtn{nullptr};
    QToolButton  *m_sendBtn{nullptr};
    QLabel       *m_formatIndicator{nullptr};
    QLabel       *m_lengthIndicator{nullptr};
    QWidget      *m_sidebarPanel{nullptr};
    bool          m_sidebarExpanded{true};
    int           m_sidebarExpandedWidth{180};
    QSplitter    *m_mainSplitter{nullptr};
    QWidget      *m_rightContent{nullptr};
    QWidget      *m_primaryPanel{nullptr};
    DropFrame    *m_primaryDropFrame{nullptr};
    QWidget      *m_primaryHeader{nullptr};
    QToolButton  *m_primaryTopicBtn{nullptr};
    QToolButton  *m_searchBtn{nullptr};
    QToolButton  *m_popOutBtn{nullptr};
    QToolButton  *m_primaryCloseBtn{nullptr};
    QListView    *m_nickList{nullptr};
    NickListModel *m_nickModel{nullptr};
    QWidget      *m_nickPanel{nullptr};
    QWidget      *m_nickPanelHeader{nullptr};
    NickFilterEdit *m_nickFilter{nullptr};
    QLabel       *m_nickGroupsIconLabel{nullptr};
    QLabel       *m_nickCountLabel{nullptr};
    QToolButton  *m_nickToggleBtn{nullptr};
    QToolButton  *m_nickRevealBtn{nullptr};
    QToolButton              *m_scrollBottomBtn{nullptr};
    QGraphicsOpacityEffect   *m_scrollBottomOpacity{nullptr};
    QPropertyAnimation       *m_scrollBottomAnim{nullptr};
    QToolButton  *m_sidebarRevealBtn{nullptr};
    QToolButton  *m_sidebarCloseBtn{nullptr};
    QWidget      *m_chatSection{nullptr};
    QSplitter    *m_chatSplitter{nullptr};
    QVBoxLayout  *m_chatLeftVbox{nullptr}; // chat column: header/topic/chat/search/reply/typing/input
    QSplitter    *m_panesSplitter{nullptr};
    QHash<QString, ChannelPane*> m_panes;        // key: "host|channel_lower"
    QList<ChannelPane*>          m_orderedPanes; // insertion order for layout (docked panes only)
    QHash<QString, QWidget*>     m_paneWindows;  // key -> top-level window for popped-out panes
    QSet<QString>                m_nickRefreshPending;    // channels with a debounced refresh queued
    QSet<QString>                m_expandedEventGroups;  // groupIds (first-msg timestamp ms) of expanded event batches
    int                          m_primarySlot{0}; // position of primary panel in layout order
    bool          m_primaryDragPending{false}; // primary header press seen, waiting for threshold
    bool          m_primaryDragging{false};    // primary pane drag in flight
    QPoint        m_primaryDragStart;          // global press position of the pending drag
    bool          m_nickExpanded{true};
    int           m_nickExpandedWidth{180};
    ElidedLabel  *m_topicLabel{nullptr};    // #channel (modes)
    QLabel       *m_userInfoLabel{nullptr}; // * network (in nick panel header)
    QWidget      *m_topicDisplay{nullptr};  // topic text — shown when showTopic
    QLabel       *m_topicText{nullptr};
    ElidedLabel  *m_topicSetByLabel{nullptr};
    QLabel       *m_appLabel{nullptr};
    QLabel       *m_typingLabel{nullptr};
    QWidget      *m_inputBar{nullptr};
    SearchBar    *m_searchBar{nullptr};
    QWidget      *m_replyBar{nullptr};
    QLabel       *m_replyLabel{nullptr};
    QString       m_pendingReplyMsgid;
    QString       m_pendingReactMsgid;
    ServerId      m_pendingReactHost;
    BufferId      m_pendingReactChannel;
    AboutDialog        *m_aboutDialog{nullptr};
    ChannelListDialog  *m_channelListDialog{nullptr};
    DocsDialog         *m_docsDialog{nullptr};
    PreferencesDialog  *m_prefsDialog{nullptr};
    EmojiPicker       *m_emojiPicker{nullptr};
    QuickSwitcher     *m_quickSwitcher{nullptr};

    // Menu bar (menu_style = "menubar"); actions are parented to this window
    // and addAction()ed so their shortcuts fire in all three menu styles.
    QMenuBar *m_menuBarWidget{nullptr};
    QAction  *m_actFind{nullptr};
    QAction  *m_actHistorySearch{nullptr};
    QAction  *m_actQuickSwitch{nullptr};
    QAction  *m_actInsertColor{nullptr};
    QAction  *m_actQuit{nullptr};
    QAction  *m_actPreferences{nullptr};
    QAction  *m_actViewSidebar{nullptr};
    QAction  *m_actViewUserList{nullptr};
    QAction  *m_actViewTopic{nullptr};
    QAction  *m_actViewTimestamps{nullptr};
    QAction  *m_actViewUnread{nullptr};
    QAction  *m_actViewCards{nullptr};

    // Typing indicator state
    TypingController            *m_typing{nullptr};
    bool                         m_restoringDraft{false};   // suppress typing TAGMSG on draft restore
    NickListStyle                 m_nickStyle;         // shared by main + pane nick models
    QHash<QString, int>           m_renderStart;        // "host\tchannel" → first rendered msg index
    QHash<QString, int>           m_scrollPositions;   // "host\tchannel" → saved scroll px (non-bottom)
    QHash<QString, QString>       m_inputDrafts;       // "host\tchannel" → unsent input text
    bool                          m_loadingOlder{false};
    QSet<QString>                 m_historyExhausted;  // channels with no more server history

    // In-flight jump to a search result (m_jumpTs invalid = no jump active)
    ServerId                      m_jumpHost;
    BufferId                      m_jumpChannel;
    QDateTime                     m_jumpTs;
    int                           m_jumpRounds{0};

    // Avatar image cache
    QNetworkAccessManager        *m_avatarNam{nullptr};
    QHash<QString, QPixmap>       m_avatarCache;       // URL → scaled pixmap
    QList<QString>                m_avatarCacheOrder;  // FIFO eviction order
    QSet<QString>                 m_avatarFetching;    // in-flight URLs
    QHash<QString, QList<QPair<ServerId, BufferId>>> m_pendingChanAvatars; // URL → buffers awaiting icon
    void fetchAvatar(const QString &url);
    void onChannelAvatarChanged(const ServerId &host, const BufferId &channel, const QString &url);
    QString nickTooltip(const QString &nick, const ServerId &host) const;

    QRegularExpression m_selfNickRe;  // pre-compiled highlight regex for active host's nick
    QRegularExpression m_highlightRe; // extra keyword highlights from config

    SessionModel *m_model;
    TrayIcon     *m_tray{nullptr};
    SignalBars   *m_signalBars{nullptr};
    PreviewController *m_previews{nullptr};
    QString       m_hoveredUrl;
    QPoint        m_hoverGlobalPos;
    DccController *m_dcc{nullptr};
    Config        m_config;
    Theme         m_theme;
    QString       m_appliedThemeName;   // last theme actually applied to the UI

    // Config file watcher — hot-reloads servers added via text editor
    QFileSystemWatcher m_configWatcher;
    bool               m_configSaving{false};
    void saveConfig(bool migratePasswords = false);
    void onConfigFileChanged();

    UpdateChecker *m_updateChecker{nullptr};

    bool    m_showNickPrefix{true};
    bool    m_showEmojiBtn{false};
    bool    m_showTopic{true};

    CommandDispatcher *m_dispatcher{nullptr};

};
