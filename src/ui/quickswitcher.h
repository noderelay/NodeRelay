#pragma once

#include "model/ids.h"
#include <QFrame>

class QLineEdit;
class QListWidget;
class QKeyEvent;
class SessionModel;

class QuickSwitcher : public QFrame
{
    Q_OBJECT
public:
    explicit QuickSwitcher(SessionModel *model, QWidget *parent = nullptr);
    void showCentered();

signals:
    void channelSelected(const ServerId &host, const BufferId &channel);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    struct Entry {
        ServerId host;
        BufferId channel;
        QString  serverName;
    };

    void populate();
    void filter(const QString &text);
    void accept();

    SessionModel *m_model;
    QLineEdit    *m_input;
    QListWidget  *m_list;
    QList<Entry>  m_entries;
};
