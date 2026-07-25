#pragma once
#include "config/config.h"
#include "model/ids.h"
#include <QObject>
#include <QPointer>
#include <QThread>

class SessionModel;
class QWidget;

class CommandDispatcher : public QObject {
    Q_OBJECT
public:
    explicit CommandDispatcher(SessionModel *model, Config *config,
                               QWidget *dialogParent, QObject *parent = nullptr);
    ~CommandDispatcher() override;

    // Returns true if a slash command was handled.
    // replyMsgid is the pending reply target (may be empty).
    bool dispatch(const QString &text, const ServerId &host, const BufferId &channel,
                  const QString &replyMsgid);

signals:
    void switchChannel(const ServerId &host, const BufferId &channel);
    void focusInput();
    void clearChat(const ServerId &host, const BufferId &channel);
    void replyBarCleared();
    void openChannelList(const ServerId &host);

private:
    void executeScript(const ScriptBinding &binding, const QString &args,
                       const ServerId &host, const BufferId &channel);

    void trackWorker(QThread *thread);

    SessionModel *m_model;
    Config       *m_config;
    QWidget      *m_dialogParent;
    QString       m_sysinfoCache;
    bool          m_sysinfoLoading{false};
    QList<QPointer<QThread>> m_workers;
};
