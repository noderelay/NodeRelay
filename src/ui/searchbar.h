#pragma once
#include <QWidget>

class ChatView;
class QLineEdit;

// Inline chat-search bar: a line edit wired to ChatView's incremental find,
// with Enter/Shift+Enter for next/prev match and Escape/close-button to
// dismiss. Shared by MainWindow's primary view and ChannelPane.
class SearchBar : public QWidget {
    Q_OBJECT
public:
    explicit SearchBar(ChatView *view, QWidget *parent = nullptr);
    void open();    // show, focus, select all
    void dismiss(); // hide, clear text, clear the view's highlight
signals:
    void dismissed(); // emitted after dismiss() so the owner can refocus its own input
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
private:
    ChatView  *m_view;
    QLineEdit *m_input{nullptr};
};
