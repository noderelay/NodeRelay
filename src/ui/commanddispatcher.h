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
    bool dispatch(const QString &text, ServerId host, BufferId channel,
                  const QString &replyMsgid);

signals:
    void switchChannel(ServerId host, BufferId channel);
    void focusInput();
    void clearChat();
    void replyBarCleared();
    void openChannelList(ServerId host);
    void pluginsStatusRequested(ServerId host, BufferId channel);

private:
    void executeScript(const ScriptBinding &binding, const QString &args,
                       ServerId host, BufferId channel);

    void trackWorker(QThread *thread);

    SessionModel *m_model;
    Config       *m_config;
    QWidget      *m_dialogParent;
    QString       m_sysinfoCache;
    bool          m_sysinfoLoading{false};
    QList<QPointer<QThread>> m_workers;
};
