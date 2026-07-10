#pragma once
#include <QLineEdit>

class QListWidget;

// Live nick-list filter: typing hides list items whose nick doesn't start
// with the typed text; Escape clears the filter. Shared by MainWindow's
// primary nick panel and ChannelPane.
class NickFilterEdit : public QLineEdit {
    Q_OBJECT
public:
    explicit NickFilterEdit(QListWidget *nickList, QWidget *parent = nullptr);
protected:
    void keyPressEvent(QKeyEvent *event) override;
private:
    QListWidget *m_nickList;
};
