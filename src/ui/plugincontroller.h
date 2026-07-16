#pragma once

#include <QElapsedTimer>
#include <QList>
#include <QObject>
#include <QProcess>

#include "config/config.h"
#include "model/ids.h"

class SessionModel;
struct Message;

// Runs enabled [[plugin]] entries as long-lived child processes and speaks
// the PluginProtocol JSON-lines contract with them: model events are written
// to each plugin's stdin, action lines read from stdout are applied via
// SessionModel (say/me/print) or forwarded as slash commands.
class PluginController : public QObject
{
    Q_OBJECT
public:
    PluginController(SessionModel *model, QObject *parent = nullptr);
    ~PluginController() override;

    // Start/stop/restart child processes to match the given config.
    void reconcile(const QList<PluginBinding> &plugins);

    // One human-readable line per configured plugin, for /plugins.
    QStringList statusLines() const;

signals:
    // A plugin asked to run a slash command; MainWindow routes it through
    // CommandDispatcher so plugins get every built-in.
    void commandRequested(ServerId host, BufferId buffer, const QString &line);

private:
    struct Proc {
        PluginBinding binding;
        QProcess     *process{nullptr};
        QByteArray    outBuf;          // stdout line assembly
        int           restarts{0};
        bool          failed{false};   // gave up after repeated crashes
        bool          stopping{false}; // deliberate stop — don't restart
        QElapsedTimer floodWindow;
        int           floodCount{0};
        bool          floodWarned{false};
    };

    void  notify(const QString &text);  // log + local line in the active buffer
    void  startProc(Proc &p);
    void  stopProc(Proc &p);
    void  handleFinished(Proc &p, int exitCode, QProcess::ExitStatus status);
    void  handleStdout(Proc &p);
    void  applyAction(Proc &p, const QByteArray &line);
    bool  floodOk(Proc &p);
    void  broadcast(const QByteArray &line);
    Proc *procFor(QObject *processObj);

    void onMessageAdded(ServerId host, BufferId buffer, const Message &msg);
    void onTopicChanged(ServerId host, BufferId buffer, const QString &topic);
    void onServerConnected(ServerId host);
    void onServerDisconnected(ServerId host);

    SessionModel *m_model;
    QList<Proc *> m_procs;
};
