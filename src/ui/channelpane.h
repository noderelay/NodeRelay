#pragma once
#include "model/ids.h"
#include <QMetaObject>
#include <QWidget>
#include <QString>
#include <QPoint>
#include <QIcon>
#include <QFont>
#include <QHash>

class ChatView;
class SearchBar;
class NickFilterEdit;
class DropFrame;
class QShortcut;
class QListWidget;
class QPlainTextEdit;
class QLabel;
class QToolButton;

class ChannelPane : public QWidget {
    Q_OBJECT
public:
    explicit ChannelPane(ServerId host, BufferId channel, QWidget *parent = nullptr);
    const ServerId &host()    const { return m_host; }
    const BufferId &channel() const { return m_channel; }
    QString         key()     const { return paneKey(m_host, m_channel); }
    ChatView     *chatView() const { return m_chatView; }
    QListWidget  *nickList() const { return m_nickList; }
    QPlainTextEdit *input()  const { return m_input; }
    void setNick(const QString &nick);
    void setNickVisible(bool visible);
    void setNickChrome(const QString &bg, bool rounded = true); // nick panel fill, painted by ChromePanel
    void setNickPanelIcons(const QIcon &hide, const QIcon &reveal, const QPixmap &groups);
    void setNickPanelFont(const QFont &f);
    void setNickCount(int count);
    void clearNickFilter();
    void setCloseIcon(const QIcon &icon);
    void setSearchIcon(const QIcon &icon);
    void setPopOutIcon(const QIcon &icon);
    void setPopOutVisible(bool visible);
    void setTyping(const QString &text);
    void setTypingEnabled(bool on);
    void setTypingFont(const QFont &f);
    void setInputFont(const QFont &nickFont, const QFont &inputFont);
    void setChatFont(const QFont &f);
    void setNickListFont(const QFont &f);
    void setTopicFont(const QFont &f);
    void setTopic(const QString &html);
    void setTopicIcon(const QIcon &collapsed, const QIcon &expanded);
    void setDragHighlight(bool on);
    void toggleSearch();
    void enableSearchShortcut(); // for popped-out windows, where the main window's Ctrl+F can't reach
    static QString mimeType();
signals:
    void closeRequested();
    void popOutRequested();
    void inputSubmitted(const QString &text);
    void dropReceived(const QString &sourceKey);
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
private:
    void positionNickRevealBtn();
    void setTopicRevealInset(bool reserve);
    void updateInputHeight();
    void guardFont(QWidget *w, const QFont &f);
private:
    ServerId              m_host;
    BufferId              m_channel;
    QMetaObject::Connection m_topicIconConn;
    QWidget      *m_header{nullptr};
    QPoint        m_dragStartPos;
    bool          m_dragPending{false};
    bool          m_dragging{false};
    ChatView     *m_chatView{nullptr};
    QListWidget  *m_nickList{nullptr};
    QWidget      *m_nickWrapper{nullptr};
    QWidget      *m_nickHeader{nullptr};
    QLabel       *m_nickGroupsIcon{nullptr};
    QLabel       *m_nickCountLabel{nullptr};
    QToolButton  *m_nickToggleBtn{nullptr};
    QToolButton  *m_nickRevealBtn{nullptr};
    NickFilterEdit *m_nickFilter{nullptr};
    QPlainTextEdit *m_input{nullptr};
    QLabel       *m_nickPrefix{nullptr};
    QLabel       *m_typingLabel{nullptr};
    QToolButton  *m_closeBtn{nullptr};
    QToolButton  *m_searchBtn{nullptr};
    QToolButton  *m_popOutBtn{nullptr};
    SearchBar    *m_searchBar{nullptr};
    DropFrame    *m_dropFrame{nullptr};
    QShortcut    *m_findShortcut{nullptr};
    QWidget      *m_topicBar{nullptr};
    QLabel       *m_topicText{nullptr};
    QToolButton  *m_topicToggle{nullptr};
    int           m_topicFontPt{11};
    QString       m_rawTopicHtml;
    QHash<QWidget*, QFont> m_fontGuards;
    bool          m_fontGuardBusy{false};
};
