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
#include "ui/themeloader.h"

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
class QToolButton;
class QSplitter;
class QAction;

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

private slots:
    // Model → UI
    void onServerAdded      (ServerId host);
    void onServerConnected  (ServerId host);
    void onServerDisconnected(ServerId host);
    void onServerClosed     (ServerId host);
    void onChannelAdded     (ServerId host, BufferId channel);
    void onChannelRemoved   (ServerId host, BufferId channel);
    void onMessageAdded     (ServerId host, BufferId channel, const Message &msg);
    void onTopicChanged     (ServerId host, BufferId channel, const QString &topic);
    void onNickListChanged     (ServerId host, BufferId channel);
    void onNickAdded           (ServerId host, BufferId channel, const QString &nick);
    void onNickRemoved         (ServerId host, BufferId channel, const QString &nick);
    void onNickRenamed         (ServerId host, BufferId channel,
                                const QString &oldNick, const QString &newNick);
    void onNickListContextMenu  (const QPoint &pos);
    void onSidebarContextMenu   (const QPoint &pos);
    void onUnreadChanged    (ServerId host, BufferId channel, int count);
    void onReactionsChanged (ServerId host, BufferId channel, const QString &msgid);
    void onSelfNickChanged  (ServerId host, const QString &nick);
    void onMessageRedacted   (ServerId host, BufferId channel, const QString &msgid);

    // UI → Model
    void onSidebarSelectionChanged();
    void onInputSubmit();
    void dispatchInput(const QString &text, ServerId host, BufferId channel);

    // Typing
    void onTypingReceived(ServerId host, BufferId channel,
                          const QString &nick, const QString &state);

private:
    void setupToolbar();
    void setupSidebar();
    void setupChatArea();
    void setupNickPanel();
    void setupInputBar();
    void correctStartupGeometry();
    void ensureEmojiPicker();
    void connectModel();
    void connectPreferences();

    void switchToChannel(ServerId host, BufferId channel);
    void openChannelList(ServerId host);
    void refreshChatView(ServerId host, BufferId channel, bool resetToLatest = true);
    void loadOlderMessages();
    void onOlderHistoryLoaded(ServerId host, BufferId channel, int count);
    void refreshNickList(ServerId host, BufferId channel);
    void scheduleNickRefresh(ServerId host, BufferId channel);
    void refreshTopicBar(ServerId host, BufferId channel);
    void appendMessage  (const Message &msg, bool autoPreview = false);
    void appendPreviewCards(ChatView *view, const Message &msg,
                            const ServerId &host, const BufferId &channel);
    QRegularExpression selfNickReFor(const ServerId &host) const;
    void applyFontSizes();
    void updateTypingLabel();
    QString typingText(ServerId host, BufferId channel) const;
    void openChannelPane (ServerId host, BufferId channel);
    void popOutChannel   (ServerId host, BufferId channel);
    void floatPane       (ChannelPane *pane);
    void closeChannelPane(ServerId host, BufferId channel);
    void closePanesForHost(ServerId host);
    ChannelPane *createPane(ServerId host, BufferId channel);
    void setChannelCheckedOut(ServerId host, BufferId channel, bool out);
    void switchAwayFromChannel(ServerId host, BufferId channel);
    void refreshPaneChatView(ChannelPane *pane);
    void refreshPaneNickList(ChannelPane *pane);
    void rebuildPaneLayout();

    QListWidgetItem *makeNickItem(const NickEntry &e, const Channel *ch, const ServerSession *sess);
    static int       findNickRow (QListWidget *list, const QString &nick);
    static QString   topicAgeStr (quint64 ts);

    QString    formatMessage(const Message &msg) const;
    void       toggleEventGroupInView(ChatView *view, const QString &groupId,
                                      ServerId host, BufferId channel);
    void       handleChatViewContextMenu(ChatView *view, const QString &anchor,
                                         const QPoint &globalPos,
                                         ServerId host, BufferId channel);
    void       showNickContextMenu(const QString &nick, const QPoint &globalPos);
    QString    msgidAtViewPos(const QPoint &viewPos) const;
    void       openLogSearch();
    void       clearReplyBar();

    // Channel / pane navigation (Alt+arrows)
    void navigateChannel(int direction);
    void navigatePane(int direction);

    // Font zoom (Ctrl+wheel / Ctrl+±)
    double *fontFieldForWidget(QObject *obj, const QPoint &pos = {});

    // Tab completion
    void handleTabComplete(QPlainTextEdit *input, ServerId host, BufferId channel);
    void repositionSendBtn();
    void updateFormatIndicator();
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
    QTreeWidgetItem *findServerItem (const ServerId &host) const;
    QTreeWidgetItem *findChannelItem(const ServerId &host, const BufferId &channel) const;

    // Widgets
    QTreeWidget      *m_sidebar;
    SidebarDelegate  *m_sidebarDelegate{nullptr};
    NickDelegate     *m_nickDelegate{nullptr};
    ChatView *m_chatView;
    QPlainTextEdit *m_input;
    QLabel       *m_nickPrefix;
    QPushButton  *m_emojiBtn;
    QToolButton  *m_sendBtn{nullptr};
    QLabel       *m_formatIndicator{nullptr};
    QWidget      *m_sidebarPanel{nullptr};
    QWidget      *m_sidebarHeader{nullptr};
    QToolButton  *m_sidebarToggleBtn{nullptr};
    QToolButton  *m_serversBtn{nullptr};
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
    QListWidget  *m_nickList;
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
    QSplitter    *m_panesSplitter{nullptr};
    QHash<QString, ChannelPane*> m_panes;        // key: "host|channel_lower"
    QList<ChannelPane*>          m_orderedPanes; // insertion order for layout (docked panes only)
    QHash<QString, QWidget*>     m_paneWindows;  // key -> top-level window for popped-out panes
    QSet<QString>                m_nickRefreshPending;    // channels with a debounced refresh queued
    QSet<QString>                m_expandedEventGroups;  // groupIds (first-msg timestamp ms) of expanded event batches
    int                          m_primarySlot{0}; // position of primary panel in layout order
    bool          m_nickExpanded{true};
    QLabel       *m_topicLabel{nullptr};    // #channel (modes)
    QLabel       *m_userInfoLabel{nullptr}; // * network (in nick panel header)
    QWidget      *m_topicDisplay{nullptr};  // topic text — shown when showTopic
    QLabel       *m_topicText{nullptr};
    QLabel       *m_topicSetByLabel{nullptr};
    QToolButton  *m_hamburger;
    QLabel       *m_appLabel{nullptr};
    QLabel       *m_typingLabel{nullptr};
    QWidget      *m_inputBar{nullptr};
    SearchBar    *m_searchBar{nullptr};
    QWidget      *m_replyBar{nullptr};
    QLabel       *m_replyLabel{nullptr};
    QString       m_pendingReplyMsgid;
    QString       m_pendingReactMsgid;
    QString       m_pendingReactHost;
    QString       m_pendingReactChannel;
    AboutDialog        *m_aboutDialog{nullptr};
    ChannelListDialog  *m_channelListDialog{nullptr};
    DocsDialog         *m_docsDialog{nullptr};
    PreferencesDialog  *m_prefsDialog{nullptr};
    EmojiPicker       *m_emojiPicker{nullptr};
    QuickSwitcher     *m_quickSwitcher{nullptr};

    // Typing indicator state
    QTimer                      *m_typingOutTimer{nullptr};
    bool                         m_typingActive{false};
    QHash<QString, QSet<QString>> m_typingNicks;       // "host|channel" → nicks
    QHash<QString, QTimer*>       m_typingNickTimers;  // "host|channel|nick" → timeout
    QHash<QString, int>           m_botIconIdx;        // lowercased nick → 0 (robot) or 1 (alien)
    QHash<QString, int>           m_renderStart;        // "host\tchannel" → first rendered msg index
    QHash<QString, int>           m_scrollPositions;   // "host\tchannel" → saved scroll px (non-bottom)
    bool                          m_loadingOlder{false};
    QSet<QString>                 m_historyExhausted;  // channels with no more server history

    // Avatar image cache
    QNetworkAccessManager        *m_avatarNam{nullptr};
    QHash<QString, QPixmap>       m_avatarCache;       // URL → scaled pixmap
    QList<QString>                m_avatarCacheOrder;  // FIFO eviction order
    QSet<QString>                 m_avatarFetching;    // in-flight URLs
    void fetchAvatar(const QString &url);
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
