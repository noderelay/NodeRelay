#pragma once
#include <QLineEdit>

class NickListModel;

// Live nick-list filter: typing narrows the model to nicks starting with the
// typed text; Escape clears the filter. Shared by MainWindow's primary nick
// panel and ChannelPane (which sets its model after construction).
class NickFilterEdit : public QLineEdit {
    Q_OBJECT
public:
    explicit NickFilterEdit(NickListModel *model = nullptr, QWidget *parent = nullptr);
    void setModel(NickListModel *model) { m_model = model; }
protected:
    void keyPressEvent(QKeyEvent *event) override;
private:
    NickListModel *m_model;
};
