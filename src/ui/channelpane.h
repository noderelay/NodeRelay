#pragma once
#include "model/ids.h"
#include <QMetaObject>
#include <QWidget>
#include <QString>
#include <QPoint>
#include <QIcon>

class ChatView;
class QListWidget;
class QPlainTextEdit;
class QLabel;
class QToolButton;
class QLineEdit;

class ChannelPane : public QWidget {
    Q_OBJECT
public:
    explicit ChannelPane(ServerId host, BufferId channel, QWidget *parent = nullptr);
    const ServerId &host()    const { return m_host; }
    const BufferId &channel() const { return m_channel; }
    QString         key()     const { return m_host.str() + "|" + m_channel.str().toLower(); }
    ChatView     *chatView() const { return m_chatView; }
    QListWidget  *nickList() const { return m_nickList; }
    QPlainTextEdit *input()  const { return m_input; }
    void setNick(const QString &nick);
    void setNickVisible(bool visible);
    void setCloseIcon(const QIcon &icon);
    void setSearchIcon(const QIcon &icon);
    void setPopOutIcon(const QIcon &icon);
    void setPopOutVisible(bool visible);
    void setTyping(const QString &text);
    void setTypingEnabled(bool on);
    void setTypingFont(const QFont &f);
    void setInputFont(const QFont &nickFont, const QFont &inputFont);
    void setTopicFont(const QFont &f);
    void setTopic(const QString &html);
    void setTopicIcon(const QIcon &collapsed, const QIcon &expanded);
    void setDragHighlight(bool on);
signals:
    void closeRequested();
    void popOutRequested();
    void inputSubmitted(const QString &text);
    void dragActive (const QString &sourceKey, const QPoint &globalPos);
    void dragDropped(const QString &sourceKey, const QPoint &globalPos);
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
private:
    void hideSearch();
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
    QPlainTextEdit *m_input{nullptr};
    QLabel       *m_nickPrefix{nullptr};
    QLabel       *m_typingLabel{nullptr};
    QToolButton  *m_closeBtn{nullptr};
    QToolButton  *m_searchBtn{nullptr};
    QToolButton  *m_popOutBtn{nullptr};
    QWidget      *m_searchBar{nullptr};
    QLineEdit    *m_searchInput{nullptr};
    QWidget      *m_topicBar{nullptr};
    QLabel       *m_topicText{nullptr};
    QToolButton  *m_topicToggle{nullptr};
    int           m_topicFontPt{11};
    QString       m_rawTopicHtml;
};
