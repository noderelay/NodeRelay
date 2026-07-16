#include "plugincontroller.h"

#include <QFileInfo>
#include <QTimer>

#include "logging.h"
#include "model/message.h"
#include "model/sessionmodel.h"
#include "plugins/pluginprotocol.h"
#include "version.h"

using namespace PluginProtocol;

// Flood guard: a plugin may emit at most kFloodMax actions per kFloodWindowMs.
static constexpr int kFloodWindowMs = 5000;
static constexpr int kFloodMax      = 10;
static constexpr int kMaxRestarts   = 3;

PluginController::PluginController(SessionModel *model, QObject *parent)
    : QObject(parent), m_model(model)
{
    connect(m_model, &SessionModel::messageAdded,       this, &PluginController::onMessageAdded);
    connect(m_model, &SessionModel::topicChanged,       this, &PluginController::onTopicChanged);
    connect(m_model, &SessionModel::serverConnected,    this, &PluginController::onServerConnected);
    connect(m_model, &SessionModel::serverDisconnected, this, &PluginController::onServerDisconnected);
}

PluginController::~PluginController()
{
    for (Proc *p : std::as_const(m_procs)) {
        stopProc(*p);
        delete p;
    }
}

void PluginController::reconcile(const QList<PluginBinding> &plugins)
{
    // Stop and drop processes whose entry is gone, disabled, or repathed.
    for (auto it = m_procs.begin(); it != m_procs.end();) {
        Proc *p = *it;
        bool keep = false;
        for (const auto &pb : plugins)
            if (pb.name == p->binding.name && pb.path == p->binding.path && pb.enabled)
                { keep = true; break; }
        if (keep) { ++it; continue; }
        stopProc(*p);
        delete p;
        it = m_procs.erase(it);
    }

    // Track every configured plugin (running or not) so /plugins can list it;
    // start the enabled ones that aren't running yet.
    for (const auto &pb : plugins) {
        Proc *existing = nullptr;
        for (Proc *p : std::as_const(m_procs))
            if (p->binding.name == pb.name) { existing = p; break; }

        if (existing) {
            existing->binding = pb;
            continue;  // running procs were already filtered to match above
        }
        Proc *p = new Proc;
        p->binding = pb;
        m_procs.append(p);
        if (pb.enabled)
            startProc(*p);
    }
}

QStringList PluginController::statusLines() const
{
    QStringList lines;
    for (const Proc *p : m_procs) {
        QString state;
        if (p->process && p->process->state() == QProcess::Running)
            state = QString("running (pid %1)").arg(p->process->processId());
        else if (p->failed)
            state = "failed — fix the script, then re-enable it in Preferences";
        else if (!p->binding.enabled)
            state = "disabled";
        else
            state = "stopped";
        lines << QString("%1 — %2  [%3]").arg(p->binding.name, state, p->binding.path);
    }
    if (lines.isEmpty())
        lines << "No plugins configured. See Preferences → Plugins (Ctrl+,).";
    return lines;
}

void PluginController::notify(const QString &text)
{
    qCInfo(lcPlugin) << text;
    const ServerId host = m_model->activeHost();
    const BufferId chan = m_model->activeChannel();
    if (!host.isEmpty() && !chan.isEmpty())
        m_model->localMessage(host, chan, text);
}

void PluginController::startProc(Proc &p)
{
    const QFileInfo fi(p.binding.path);
    if (!fi.exists() || !fi.isFile()) {
        p.failed = true;
        notify(QString("Plugin \"%1\": file not found: %2")
                              .arg(p.binding.name, p.binding.path));
        return;
    }

    auto *proc = new QProcess(this);
    p.process  = proc;
    p.stopping = false;
    p.outBuf.clear();

    // .sh via bash from PATH — mirrors executeScript(); FreeBSD has no /bin/bash.
    if (p.binding.path.endsWith(".sh")) {
        proc->setProgram("bash");
        proc->setArguments({ p.binding.path });
    } else {
        proc->setProgram(p.binding.path);
    }
    proc->setWorkingDirectory(fi.absolutePath());

    connect(proc, &QProcess::readyReadStandardOutput, this, [this, proc] {
        if (Proc *pp = procFor(proc)) handleStdout(*pp);
    });
    connect(proc, &QProcess::readyReadStandardError, this, [this, proc] {
        if (Proc *pp = procFor(proc)) {
            const QByteArray err = proc->readAllStandardError();
            for (const QByteArray &line : err.split('\n'))
                if (!line.trimmed().isEmpty())
                    qCDebug(lcPlugin) << pp->binding.name << "stderr:" << line.trimmed();
        }
    });
    connect(proc, &QProcess::finished, this,
            [this, proc](int code, QProcess::ExitStatus status) {
        if (Proc *pp = procFor(proc)) handleFinished(*pp, code, status);
    });
    connect(proc, &QProcess::errorOccurred, this, [this, proc](QProcess::ProcessError e) {
        if (e != QProcess::FailedToStart) return;
        if (Proc *pp = procFor(proc)) {
            pp->failed  = true;
            pp->process = nullptr;
            proc->deleteLater();
            qCWarning(lcPlugin) << pp->binding.name << "failed to start:" << proc->errorString();
            notify(QString("Plugin \"%1\" failed to start: %2")
                                  .arg(pp->binding.name, proc->errorString()));
        }
    });

    qCInfo(lcPlugin) << "starting" << p.binding.name << p.binding.path;
    proc->start();
    proc->write(helloEvent(QStringLiteral(UPLINK_VERSION)));
}

void PluginController::stopProc(Proc &p)
{
    QProcess *proc = p.process;
    if (!proc) return;
    p.stopping = true;
    p.process  = nullptr;
    proc->disconnect(this);
    proc->closeWriteChannel();
    proc->terminate();
    if (!proc->waitForFinished(1500))
        proc->kill();
    proc->deleteLater();
}

void PluginController::handleFinished(Proc &p, int exitCode, QProcess::ExitStatus status)
{
    QProcess *proc = p.process;
    p.process = nullptr;
    if (proc) proc->deleteLater();
    if (p.stopping) return;

    qCInfo(lcPlugin) << p.binding.name << "exited" << exitCode << status;

    if (p.restarts >= kMaxRestarts) {
        p.failed = true;
        notify(QString("Plugin \"%1\" crashed %2 times and was stopped. "
                                  "Fix the script, then re-enable it in Preferences → Plugins.")
                              .arg(p.binding.name).arg(p.restarts + 1));
        return;
    }

    ++p.restarts;
    const int delayMs = 1000 * p.restarts * p.restarts;  // 1s, 4s, 9s
    notify(QString("Plugin \"%1\" exited — restarting in %2s.")
                          .arg(p.binding.name).arg(delayMs / 1000));
    QTimer::singleShot(delayMs, this, [this, name = p.binding.name] {
        for (Proc *pp : std::as_const(m_procs))
            if (pp->binding.name == name && pp->binding.enabled && !pp->process && !pp->failed)
                startProc(*pp);
    });
}

void PluginController::handleStdout(Proc &p)
{
    p.outBuf += p.process->readAllStandardOutput();

    // Runaway-output guard: no sane action line is this long.
    if (p.outBuf.size() > 1024 * 1024) {
        qCWarning(lcPlugin) << p.binding.name << "output line exceeded 1 MB, discarding buffer";
        p.outBuf.clear();
        return;
    }

    qsizetype nl;
    while ((nl = p.outBuf.indexOf('\n')) >= 0) {
        const QByteArray line = p.outBuf.left(nl);
        p.outBuf.remove(0, nl + 1);
        if (!line.trimmed().isEmpty())
            applyAction(p, line);
    }
}

void PluginController::applyAction(Proc &p, const QByteArray &line)
{
    const PluginAction a = parseAction(line);
    if (a.kind == PluginAction::Invalid) {
        qCWarning(lcPlugin) << p.binding.name << "bad action:" << a.error << line.left(200);
        return;
    }
    if (!floodOk(p)) return;

    const ServerId host{a.server};
    const BufferId buffer{a.buffer};
    switch (a.kind) {
    case PluginAction::Say:     m_model->sendMessage(host, buffer, a.text); break;
    case PluginAction::Me:      m_model->sendAction(host, buffer, a.text);  break;
    case PluginAction::Print:   m_model->localMessage(host, buffer, "[" + p.binding.name + "] " + a.text); break;
    case PluginAction::Command: emit commandRequested(host, buffer, a.text); break;
    case PluginAction::Invalid: break;
    }
}

bool PluginController::floodOk(Proc &p)
{
    if (!p.floodWindow.isValid() || p.floodWindow.elapsed() > kFloodWindowMs) {
        p.floodWindow.start();
        p.floodCount  = 0;
        p.floodWarned = false;
    }
    if (++p.floodCount <= kFloodMax) return true;
    if (!p.floodWarned) {
        p.floodWarned = true;
        qCWarning(lcPlugin) << p.binding.name << "flooding — dropping actions";
        notify(QString("Plugin \"%1\" is sending too fast — extra actions dropped.")
                              .arg(p.binding.name));
    }
    return false;
}

void PluginController::broadcast(const QByteArray &line)
{
    if (line.isEmpty()) return;
    for (Proc *p : std::as_const(m_procs))
        if (p->process && p->process->state() == QProcess::Running)
            p->process->write(line);
}

PluginController::Proc *PluginController::procFor(QObject *processObj)
{
    for (Proc *p : std::as_const(m_procs))
        if (p->process == processObj) return p;
    return nullptr;
}

void PluginController::onMessageAdded(ServerId host, BufferId buffer, const Message &msg)
{
    // History backfill and redacted lines must never reach plugins — a bot
    // would happily answer a week-old "!ping" replayed by chathistory.
    if (msg.isHistory || msg.redacted) return;

    const QString me   = m_model->selfNick(host);
    const bool    self = !me.isEmpty() && msg.nick.compare(me, Qt::CaseInsensitive) == 0;

    bool mentions = false;
    if (!self && !me.isEmpty()) {
        const auto re = SessionModel::buildHighlightRe(me);
        mentions = re.isValid() && re.match(msg.text).hasMatch();
    }

    broadcast(messageEvent(host.str(), buffer.str(), msg, self, mentions));
}

void PluginController::onTopicChanged(ServerId host, BufferId buffer, const QString &topic)
{
    broadcast(topicEvent(host.str(), buffer.str(), topic));
}

void PluginController::onServerConnected(ServerId host)
{
    broadcast(serverEvent(QStringLiteral("connected"), host.str()));
}

void PluginController::onServerDisconnected(ServerId host)
{
    broadcast(serverEvent(QStringLiteral("disconnected"), host.str()));
}
