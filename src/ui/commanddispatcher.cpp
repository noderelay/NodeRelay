#include "commanddispatcher.h"
#include "model/sessionmodel.h"
#include "irc/ircclient.h"
#include "config/config.h"

#include <memory>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QMetaObject>
#include <QProcess>
#include <QSettings>
#include <QStringList>
#include <QSysInfo>
#include <QTextStream>
#include <QThread>
#include <QTimer>

#if defined(Q_OS_WIN)
#  include <windows.h>
#endif

// ---------------------------------------------------------------------------
// Sysinfo helpers
// ---------------------------------------------------------------------------

#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
static QString sysinfoKernel()
{
    QProcess p;
    p.start("uname", {"-r"});
    p.waitForFinished(2000);
    const QString out = QString::fromLocal8Bit(p.readAllStandardOutput()).trimmed();
    return out.isEmpty() ? "Unknown" : out;
}
#endif

static QString sysinfoOS()
{
#if defined(Q_OS_LINUX)
    QString name, buildId, versionId;
    QFile f("/etc/os-release");
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&f);
        const auto strip = [](QString s) {
            if (s.startsWith('"') && s.endsWith('"'))
                s = s.mid(1, s.length() - 2);
            return s;
        };
        while (!in.atEnd()) {
            const QString line = in.readLine();
            if (line.startsWith("NAME=") && name.isEmpty())
                name = strip(line.mid(5));
            else if (line.startsWith("BUILD_ID=") && buildId.isEmpty())
                buildId = strip(line.mid(9));
            else if (line.startsWith("VERSION_ID=") && versionId.isEmpty())
                versionId = strip(line.mid(11));
        }
    }
    if (name.isEmpty()) name = "Linux";
    const QString ver = !buildId.isEmpty() ? buildId : versionId;
    const QString distro = ver.isEmpty() ? name : name + " " + ver;
    return QString("Linux (%1) (%2)").arg(distro, sysinfoKernel());
#elif defined(Q_OS_FREEBSD)
    QProcess p;
    p.start("uname", {"-s"});
    p.waitForFinished(2000);
    return QString("%1 (%2)").arg(QString::fromLocal8Bit(p.readAllStandardOutput()).trimmed(),
                                  sysinfoKernel());
#else
    return QSysInfo::prettyProductName();
#endif
}

static QString sysinfoCPU()
{
#if defined(Q_OS_LINUX)
    QFile f("/proc/cpuinfo");
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&f);
        QString line;
        while (!(line = in.readLine()).isNull()) {
            if (line.startsWith("model name")) {
                const qsizetype colon = line.indexOf(':');
                if (colon != -1)
                    return line.mid(colon + 1).trimmed();
            }
        }
    }
    return QSysInfo::currentCpuArchitecture();
#elif defined(Q_OS_FREEBSD) || defined(Q_OS_DARWIN)
    QProcess p;
    p.start("sysctl", {"-n", "hw.model"});
    p.waitForFinished(2000);
    const QString model = QString::fromLocal8Bit(p.readAllStandardOutput()).trimmed();
    return model.isEmpty() ? "Unknown" : model;
#elif defined(Q_OS_WIN)
    QSettings reg(
        "HKEY_LOCAL_MACHINE\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
        QSettings::NativeFormat);
    const QString name = reg.value("ProcessorNameString").toString().trimmed();
    return name.isEmpty() ? QSysInfo::currentCpuArchitecture() : name;
#else
    return QSysInfo::currentCpuArchitecture();
#endif
}

static QString sysinfoMEM()
{
#if defined(Q_OS_LINUX)
    QFile f("/proc/meminfo");
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&f);
        QString line;
        while (!(line = in.readLine()).isNull()) {
            const QStringList parts = line.split(' ', Qt::SkipEmptyParts);
            if (parts.size() >= 2 && parts[0] == "MemTotal:") {
                const quint64 kb = parts[1].toULongLong();
                return QString("%1 GB").arg((kb + 512 * 1024) / (1024 * 1024));
            }
        }
    }
    return "Unknown";
#elif defined(Q_OS_FREEBSD)
    QProcess p;
    p.start("sysctl", {"-n", "hw.physmem"});
    p.waitForFinished(2000);
    const quint64 total = QString::fromLocal8Bit(p.readAllStandardOutput()).trimmed().toULongLong();
    if (total > 0)
        return QString("%1 GB").arg((total + 512ULL*1024*1024) / (1024ULL*1024*1024));
    return "Unknown";
#elif defined(Q_OS_DARWIN)
    QProcess p;
    p.start("sysctl", {"-n", "hw.memsize"});
    p.waitForFinished(2000);
    const quint64 total = QString::fromLocal8Bit(p.readAllStandardOutput()).trimmed().toULongLong();
    if (total > 0)
        return QString("%1 GB").arg((total + 512ULL*1024*1024) / (1024ULL*1024*1024));
    return "Unknown";
#elif defined(Q_OS_WIN)
    MEMORYSTATUSEX ms{};
    ms.dwLength = sizeof(ms);
    if (GlobalMemoryStatusEx(&ms))
        return QString("%1 GB").arg((ms.ullTotalPhys + 512ULL*1024*1024) / (1024ULL*1024*1024));
    return "Unknown";
#else
    return "Unknown";
#endif
}

static QString sysinfoGPU()
{
#if defined(Q_OS_LINUX)
    QProcess vk;
    vk.start("vulkaninfo", {"--summary"});
    if (vk.waitForFinished(3000) && vk.exitCode() == 0) {
        const QString out = QString::fromLocal8Bit(vk.readAllStandardOutput());
        // vulkaninfo lists every device; prefer a hardware GPU over a
        // software renderer (llvmpipe/lavapipe) but fall back to the first.
        QString deviceName, driverInfo, firstName, firstInfo, curName;
        for (const QString &line : out.split('\n')) {
            const QString t = line.trimmed();
            if (t.startsWith("deviceName")) {
                curName = t.section('=', 1).trimmed();
            } else if (t.startsWith("driverInfo") && !curName.isEmpty()) {
                const QString info = t.section('=', 1).trimmed();
                if (firstName.isEmpty()) { firstName = curName; firstInfo = info; }
                const bool software =
                    curName.contains("llvmpipe", Qt::CaseInsensitive) ||
                    curName.contains("lavapipe", Qt::CaseInsensitive) ||
                    curName.contains("softpipe", Qt::CaseInsensitive) ||
                    curName.contains("swrast",   Qt::CaseInsensitive);
                if (!software && deviceName.isEmpty()) { deviceName = curName; driverInfo = info; }
                curName.clear();
            }
        }
        if (deviceName.isEmpty()) { deviceName = firstName; driverInfo = firstInfo; }
        if (!deviceName.isEmpty()) {
            const qsizetype lp = driverInfo.lastIndexOf('(');
            const qsizetype rp = driverInfo.lastIndexOf(')');
            if (lp != -1 && rp > lp)
                return QString("%1 (%2) (Vulkan)").arg(deviceName, driverInfo.mid(lp + 1, rp - lp - 1));
            return deviceName + " (Vulkan)";
        }
    }
    QProcess lp;
    lp.start("lspci", {});
    if (lp.waitForFinished(2000)) {
        const QString out = QString::fromLocal8Bit(lp.readAllStandardOutput());
        for (const QString &line : out.split('\n')) {
            if (line.contains("VGA", Qt::CaseInsensitive) ||
                line.contains("3D controller", Qt::CaseInsensitive) ||
                line.contains("Display controller", Qt::CaseInsensitive)) {
                const qsizetype c2 = line.indexOf(':', line.indexOf(':') + 1);
                if (c2 != -1)
                    return line.mid(c2 + 1).section('(', 0, 0).trimmed();
            }
        }
    }
    return "Unknown";
#elif defined(Q_OS_FREEBSD)
    // pciconf is in the FreeBSD base system; find the display-class device block.
    QProcess p;
    p.start("pciconf", {"-lv"});
    if (p.waitForFinished(3000)) {
        const QString out = QString::fromLocal8Bit(p.readAllStandardOutput());
        bool inDisplay = false;
        QString vendor, device;
        for (const QString &raw : out.split('\n')) {
            if (!raw.isEmpty() && !raw.at(0).isSpace()) {          // device header line
                if (inDisplay && !device.isEmpty()) break;
                inDisplay = raw.contains("class=0x03");            // 0x03xxxx = display
                vendor.clear();
                device.clear();
            } else if (inDisplay) {
                const QString t = raw.trimmed();
                if      (t.startsWith("vendor")) vendor = t.section('\'', 1, 1);
                else if (t.startsWith("device")) device = t.section('\'', 1, 1);
            }
        }
        if (!device.isEmpty())
            return vendor.isEmpty() ? device : (vendor + " " + device);
    }
    return "Unknown";
#elif defined(Q_OS_DARWIN)
    QProcess p;
    p.start("system_profiler", {"SPDisplaysDataType"});
    if (p.waitForFinished(8000)) {
        const QString out = QString::fromLocal8Bit(p.readAllStandardOutput());
        for (const QString &line : out.split('\n')) {
            const QString t = line.trimmed();
            if (t.startsWith("Chipset Model:"))
                return t.mid(15).trimmed();
        }
    }
    return "Unknown";
#elif defined(Q_OS_WIN)
    QProcess p;
    p.start("powershell", {"-NoProfile", "-Command",
        "(Get-CimInstance Win32_VideoController | Select-Object -First 1).Name"});
    if (p.waitForFinished(5000)) {
        const QString out = QString::fromLocal8Bit(p.readAllStandardOutput()).trimmed();
        if (!out.isEmpty()) return out;
    }
    return "Unknown";
#else
    return "Unknown";
#endif
}

static QString sysinfoUptime()
{
#if defined(Q_OS_LINUX)
    QFile f("/proc/uptime");
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const quint64 s = static_cast<quint64>(
            QString::fromLocal8Bit(f.readLine()).section(' ', 0, 0).toDouble());
        const quint64 days    = s / 86400;
        const quint64 hours   = (s % 86400) / 3600;
        const quint64 minutes = (s % 3600) / 60;
        const quint64 seconds = s % 60;
        QStringList parts;
        if (days > 0)    parts << QString("%1 day%2").arg(days).arg(days != 1 ? "s" : "");
        if (hours > 0)   parts << QString("%1 hour%2").arg(hours).arg(hours != 1 ? "s" : "");
        if (minutes > 0) parts << QString("%1 minute%2").arg(minutes).arg(minutes != 1 ? "s" : "");
        parts << QString("%1 second%2").arg(seconds).arg(seconds != 1 ? "s" : "");
        return parts.join(' ') + " ago";
    }
    return "Unknown";
#elif defined(Q_OS_FREEBSD) || defined(Q_OS_DARWIN)
    QProcess p;
    p.start("sysctl", {"-n", "kern.boottime"});
    p.waitForFinished(2000);
    const QString out = QString::fromLocal8Bit(p.readAllStandardOutput());
    const qsizetype eq = out.indexOf("sec = ");
    const qsizetype cm = out.indexOf(',', eq);
    if (eq != -1 && cm != -1) {
        const quint64 bootSec = out.mid(eq + 6, cm - eq - 6).trimmed().toULongLong();
        const quint64 now = static_cast<quint64>(QDateTime::currentSecsSinceEpoch());
        const quint64 s   = now > bootSec ? now - bootSec : 0;
        const quint64 days    = s / 86400;
        const quint64 hours   = (s % 86400) / 3600;
        const quint64 minutes = (s % 3600) / 60;
        const quint64 seconds = s % 60;
        QStringList parts;
        if (days > 0)    parts << QString("%1 day%2").arg(days).arg(days != 1 ? "s" : "");
        if (hours > 0)   parts << QString("%1 hour%2").arg(hours).arg(hours != 1 ? "s" : "");
        if (minutes > 0) parts << QString("%1 minute%2").arg(minutes).arg(minutes != 1 ? "s" : "");
        parts << QString("%1 second%2").arg(seconds).arg(seconds != 1 ? "s" : "");
        return parts.join(' ') + " ago";
    }
    return "Unknown";
#elif defined(Q_OS_WIN)
    const quint64 s = GetTickCount64() / 1000;
    const quint64 days    = s / 86400;
    const quint64 hours   = (s % 86400) / 3600;
    const quint64 minutes = (s % 3600) / 60;
    const quint64 seconds = s % 60;
    QStringList parts;
    if (days > 0)    parts << QString("%1 day%2").arg(days).arg(days != 1 ? "s" : "");
    if (hours > 0)   parts << QString("%1 hour%2").arg(hours).arg(hours != 1 ? "s" : "");
    if (minutes > 0) parts << QString("%1 minute%2").arg(minutes).arg(minutes != 1 ? "s" : "");
    parts << QString("%1 second%2").arg(seconds).arg(seconds != 1 ? "s" : "");
    return parts.join(' ') + " ago";
#else
    return "Unknown";
#endif
}

// ---------------------------------------------------------------------------

// The metadata commands are dead either because the server never offered the
// cap or because the user switched it off for this server — say which.
static QString metadataOffReason(const IrcClient *cl)
{
    return (cl && !cl->metadataEnabled())
        ? QStringLiteral("Metadata is turned off for this server")
        : QStringLiteral("Server does not support metadata");
}

CommandDispatcher::CommandDispatcher(SessionModel *model, Config *config,
                                     QWidget *dialogParent, QObject *parent)
    : QObject(parent)
    , m_model(model)
    , m_config(config)
    , m_dialogParent(dialogParent)
{}

CommandDispatcher::~CommandDispatcher()
{
    // Join worker threads before this object goes away — their lambdas capture
    // `this` and invokeMethod it. Every worker self-terminates (all its QProcess
    // waits carry timeouts), so these waits cannot hang.
    for (const auto &t : std::as_const(m_workers))
        if (t) t->wait();
}

void CommandDispatcher::trackWorker(QThread *thread)
{
    m_workers.removeAll(nullptr);
    m_workers.append(thread);
}

bool CommandDispatcher::dispatch(const QString &text, const ServerId &host,
                                  const BufferId &channel, const QString &replyMsgid)
{
    if (!text.startsWith('/')) return false;

    const QString trimmed = text.trimmed();
    const QString cmd  = trimmed.section(' ', 0, 0).toLower();
    const QString args = trimmed.section(' ', 1);

    if (cmd == "/join" || cmd == "/j") {
        m_model->sendJoin(host, BufferId{args.section(' ', 0, 0)}, args.section(' ', 1, 1));
    } else if (cmd == "/part" || cmd == "/leave" || cmd == "/close") {
        if (isChannelName(channel.str()))
            m_model->sendPart(host, channel, args);
        else
            m_model->closeBuffer(host, channel);
    } else if (cmd == "/nick") {
        m_model->sendNick(host, args.trimmed());
    } else if (cmd == "/me") {
        m_model->sendAction(host, channel, args);
    } else if (cmd == "/msg") {
        const QString target = args.section(' ', 0, 0);
        const QString body   = args.section(' ', 1);
        m_model->sendMessage(host, BufferId{target}, body);
        if (!isChannelName(target))
            emit switchChannel(host, BufferId{target});
    } else if (cmd == "/query") {
        const QString target = args.trimmed().section(' ', 0, 0);
        if (!target.isEmpty()) {
            m_model->openPM(host, target);
            emit switchChannel(host, BufferId{target});
            emit focusInput();
        }
    } else if (cmd == "/ns") {
        if (!args.isEmpty()) m_model->sendMessage(host, BufferId{"NickServ"}, args);
    } else if (cmd == "/cs") {
        if (!args.isEmpty()) m_model->sendMessage(host, BufferId{"ChanServ"}, args);
    } else if (cmd == "/bs") {
        if (!args.isEmpty()) m_model->sendMessage(host, BufferId{"BotServ"}, args);
    } else if (cmd == "/ms") {
        if (!args.isEmpty()) m_model->sendMessage(host, BufferId{"MemoServ"}, args);
    } else if (cmd == "/oper") {
        const QString user = args.section(' ', 0, 0);
        const QString pass = args.section(' ', 1);
        if (!user.isEmpty() && !pass.isEmpty())
            m_model->sendRaw(host, "OPER " + user + " :" + pass);
    } else if (cmd == "/notice") {
        const QString target = args.section(' ', 0, 0);
        const QString msg    = args.section(' ', 1);
        if (!target.isEmpty() && !msg.isEmpty())
            m_model->sendRaw(host, "NOTICE " + target + " :" + msg);
    } else if (cmd == "/caps") {
        auto *cl = m_model->clientFor(host);
        if (cl) {
            const QStringList caps = cl->ackedCaps();
            m_model->localMessage(host, channel,
                caps.isEmpty() ? "No caps negotiated."
                               : "Active caps: " + caps.join("  "));
        }
    } else if (cmd == "/quote" || cmd == "/raw") {
        m_model->sendRaw(host, args);
    } else if (cmd == "/quit") {
        if (auto *cl = m_model->clientFor(host)) cl->quit(args);
    } else if (cmd == "/disconnect") {
        m_model->closeServer(host);
    } else if (cmd == "/connect" || cmd == "/server") {
        const QString target = args.trimmed();
        if (target.isEmpty()) {
            if (auto *cl = m_model->clientFor(host))
                cl->reconnect();
            else
                m_model->connectServer(host);
        } else if (!m_model->connectServer(ServerId{target})) {
            QString h = target.section(' ', 0, 0);
            quint16 port = 6697;
            bool ssl = true;
            if (h.contains(':')) {
                bool ok = false;
                quint16 p = h.section(':', -1).toUShort(&ok);
                if (ok) {
                    port = p;
                    h = h.section(':', 0, -2);
                }
            }
            if (h.endsWith('+')) {
                h.chop(1);
            } else if (port == 6667) {
                ssl = false;
            }
            QString nick;
            for (const auto &sc : std::as_const(m_config->servers))
                if (!sc.nick.isEmpty()) { nick = sc.nick; break; }
            if (nick.isEmpty())
                nick = qEnvironmentVariable("USER", "Uplink");
            ServerConfig sc;
            sc.host = h;
            sc.port = port;
            sc.ssl  = ssl;
            sc.nick = nick;
            sc.user = nick;
            sc.realname = nick;
            m_model->addServer(sc);
        }
    } else if (cmd == "/away") {
        QString msg = args.trimmed();
        if (msg.isEmpty()) {
            for (const auto &sc : std::as_const(m_config->servers))
                if (sc.name == host.str()) { msg = sc.awayMessage; break; }
        }
        if (msg.isEmpty()) msg = QStringLiteral("Away");
        m_model->sendRaw(host, "AWAY :" + msg);
    } else if (cmd == "/back") {
        m_model->sendRaw(host, "AWAY");
    } else if (cmd == "/setname") {
        if (args.trimmed().isEmpty())
            m_model->localMessage(host, channel, "Usage: /setname <new realname>");
        else
            m_model->sendRaw(host, "SETNAME :" + args.trimmed());
    } else if (cmd == "/displayname") {
        auto *cl = m_model->clientFor(host);
        if (!cl || !cl->hasMetadataCap()) {
            m_model->localMessage(host, channel,
                metadataOffReason(cl) + " — cannot set display name");
        } else {
            const QString val = args.trimmed();
            m_model->sendRaw(host, "METADATA * SET display-name :" + val);
            m_config->profileDisplayName = val;
            Config::save(*m_config, Config::defaultPath());
            m_model->localMessage(host, channel,
                val.isEmpty() ? "Display name cleared." : "Display name set to: " + val);
        }
    } else if (cmd == "/avatar") {
        auto *cl = m_model->clientFor(host);
        if (!cl || !cl->hasMetadataCap()) {
            m_model->localMessage(host, channel,
                metadataOffReason(cl) + " — cannot set avatar");
        } else {
            const QString val = args.trimmed();
            m_model->sendRaw(host, "METADATA * SET avatar :" + val);
            m_config->profileAvatarUrl = val;
            Config::save(*m_config, Config::defaultPath());
            m_model->localMessage(host, channel,
                val.isEmpty() ? "Avatar cleared." : "Avatar URL set to: " + val);
        }
    } else if (cmd == "/status") {
        auto *cl = m_model->clientFor(host);
        if (!cl || !cl->hasMetadataCap()) {
            m_model->localMessage(host, channel,
                metadataOffReason(cl) + " — cannot set status");
        } else {
            const QString val = args.trimmed();
            m_model->sendRaw(host, "METADATA * SET status :" + val);
            m_config->profileStatusText = val;
            Config::save(*m_config, Config::defaultPath());
            m_model->localMessage(host, channel,
                val.isEmpty() ? "Status cleared." : "Status set to: " + val);
        }
    } else if (cmd == "/chanavatar") {
        auto *cl = m_model->clientFor(host);
        if (!cl || !cl->hasMetadataCap()) {
            m_model->localMessage(host, channel,
                metadataOffReason(cl) + " — cannot set channel avatar");
        } else if (!isChannelName(channel.str())) {
            m_model->localMessage(host, channel,
                "/chanavatar only works in a channel buffer");
        } else {
            const QString val = args.trimmed();
            m_model->sendRaw(host, "METADATA " + channel.str() + " SET avatar :" + val);
            m_model->localMessage(host, channel,
                val.isEmpty() ? "Requested avatar clear for " + channel.str()
                              : "Requested avatar for " + channel.str() + ": " + val);
        }
    } else if (cmd == "/persistence") {
        auto *cl = m_model->clientFor(host);
        const QString val = args.trimmed().toUpper();
        if (!cl || !cl->hasCap("draft/persistence")) {
            m_model->localMessage(host, channel,
                "This server does not support persistence (draft/persistence)");
        } else if (val.isEmpty()) {
            m_model->sendRaw(host, "PERSISTENCE GET");
        } else if (val == "ON" || val == "OFF" || val == "DEFAULT") {
            m_model->sendRaw(host, "PERSISTENCE SET " + val);
        } else {
            m_model->localMessage(host, channel, "Usage: /persistence [on|off|default]");
        }
    } else if (cmd == "/list") {
        emit openChannelList(host);
    } else if (cmd == "/motd") {
        m_model->sendRaw(host, args.isEmpty() ? "MOTD" : "MOTD " + args.trimmed());
    } else if (cmd == "/stats") {
        if (args.trimmed().isEmpty())
            m_model->localMessage(host, channel, "Usage: /stats <query>  (e.g. u=uptime, o=opers, m=commands)");
        else
            m_model->sendRaw(host, "STATS " + args.trimmed().left(1));
    } else if (cmd == "/whois") {
        m_model->sendRaw(host, "WHOIS " + args.trimmed());
    } else if (cmd == "/whowas") {
        if (args.trimmed().isEmpty())
            m_model->localMessage(host, channel, "Usage: /whowas <nick>");
        else
            m_model->sendRaw(host, "WHOWAS " + args.trimmed());
    } else if (cmd == "/topic") {
        QString topicTarget = channel.str();
        QString topicText   = args;
        if (isChannelName(args)) {
            topicTarget = args.section(' ', 0, 0);
            topicText   = args.section(' ', 1);
        }
        if (topicText.isEmpty())
            m_model->sendRaw(host, "TOPIC " + topicTarget);
        else
            m_model->sendRaw(host, "TOPIC " + topicTarget + " :" + topicText);
    } else if (cmd == "/kick") {
        const QString target = args.section(' ', 0, 0);
        const QString reason = args.section(' ', 1);
        if (!target.isEmpty())
            m_model->sendRaw(host, "KICK " + channel.str() + " " + target
                             + (reason.isEmpty() ? "" : " :" + reason));
    } else if (cmd == "/version") {
        if (args.isEmpty())
            m_model->sendRaw(host, "VERSION");
        else
            m_model->sendRaw(host, "PRIVMSG " + args.trimmed() + " :\x01VERSION\x01");
    } else if (cmd == "/ctcp") {
        const QString target   = args.section(' ', 0, 0);
        const QString ctcpcmd  = args.section(' ', 1, 1).toUpper();
        const QString ctcpargs = args.section(' ', 2);
        if (!target.isEmpty() && !ctcpcmd.isEmpty()) {
            const QString ctcp = ctcpargs.isEmpty()
                ? "\x01" + ctcpcmd + "\x01"
                : "\x01" + ctcpcmd + " " + ctcpargs + "\x01";
            m_model->sendRaw(host, "PRIVMSG " + target + " :" + ctcp);
        }
    } else if (cmd == "/sysinfo") {
        if (!m_sysinfoCache.isEmpty()) {
            m_model->sendMessage(host, channel,
                m_sysinfoCache + " UP: " + sysinfoUptime());
        } else if (m_sysinfoLoading) {
            m_model->localMessage(host, channel,
                "System info is still being collected, please wait.");
        } else {
            m_sysinfoLoading = true;
            m_model->localMessage(host, channel, "Collecting system info...");

            auto aborted = std::make_shared<bool>(false);
            auto *thread = QThread::create([this, host, channel, aborted]() {
                const QString result =
                    QString("OS: %1 CPU: %2 MEM: %3 GPU: %4")
                        .arg(sysinfoOS(), sysinfoCPU(), sysinfoMEM(), sysinfoGPU());
                QMetaObject::invokeMethod(this, [this, host, channel, result, aborted]() {
                    if (*aborted) return;
                    m_sysinfoCache   = result;
                    m_sysinfoLoading = false;
                    m_model->sendMessage(host, channel,
                        result + " UP: " + sysinfoUptime());
                }, Qt::QueuedConnection);
            });
            connect(thread, &QThread::finished, thread, &QThread::deleteLater);
            trackWorker(thread);

            auto *timer = new QTimer(this);
            timer->setSingleShot(true);
            connect(timer, &QTimer::timeout, this, [this, host, channel, timer, aborted]() {
                timer->deleteLater();
                if (!m_sysinfoLoading) return;
                *aborted = true;
                m_sysinfoLoading = false;
                m_model->localMessage(host, channel,
                    "System info collection timed out.");
            });
            connect(thread, &QThread::finished, timer, [timer]() {
                timer->stop();
                timer->deleteLater();
            });
            timer->start(12000);
            thread->start();
        }
    } else if (cmd == "/ping") {
        const QString nick = args.trimmed().section(' ', 0, 0);
        if (!nick.isEmpty()) {
            const QString ts = QString::number(QDateTime::currentMSecsSinceEpoch());
            m_model->sendRaw(host, "PRIVMSG " + nick + " :\x01PING " + ts + "\x01");
            m_model->localMessage(host, channel, "Pinged " + nick);
        }
    } else if (cmd == "/time") {
        const QString nick = args.trimmed().section(' ', 0, 0);
        if (!nick.isEmpty()) {
            m_model->sendRaw(host, "PRIVMSG " + nick + " :\x01TIME\x01");
            m_model->localMessage(host, channel, "Querying time for " + nick);
        } else {
            m_model->sendRaw(host, "TIME");
        }
    } else if (cmd == "/invite") {
        const QString nick = args.section(' ', 0, 0);
        const QString chan = args.section(' ', 1, 1);
        if (!nick.isEmpty())
            m_model->sendRaw(host, "INVITE " + nick + " " + (chan.isEmpty() ? channel.str() : chan));
    } else if (cmd == "/mode") {
        if (!args.isEmpty())
            m_model->sendRaw(host, "MODE " + args);
    } else if (cmd == "/op") {
        const QString nick = args.trimmed().section(' ', 0, 0);
        if (!nick.isEmpty())
            m_model->sendRaw(host, "MODE " + channel.str() + " +o " + nick);
    } else if (cmd == "/deop") {
        const QString nick = args.trimmed().section(' ', 0, 0);
        if (!nick.isEmpty())
            m_model->sendRaw(host, "MODE " + channel.str() + " -o " + nick);
    } else if (cmd == "/voice") {
        const QString nick = args.trimmed().section(' ', 0, 0);
        if (!nick.isEmpty())
            m_model->sendRaw(host, "MODE " + channel.str() + " +v " + nick);
    } else if (cmd == "/devoice") {
        const QString nick = args.trimmed().section(' ', 0, 0);
        if (!nick.isEmpty())
            m_model->sendRaw(host, "MODE " + channel.str() + " -v " + nick);
    } else if (cmd == "/ban") {
        const QString mask = args.trimmed().section(' ', 0, 0);
        if (!mask.isEmpty())
            m_model->sendRaw(host, "MODE " + channel.str() + " +b " + mask);
    } else if (cmd == "/unban") {
        const QString mask = args.trimmed().section(' ', 0, 0);
        if (!mask.isEmpty())
            m_model->sendRaw(host, "MODE " + channel.str() + " -b " + mask);
    } else if (cmd == "/react") {
        const QString emoji = args.trimmed();
        if (!emoji.isEmpty() && !replyMsgid.isEmpty()) {
            m_model->sendReact(host, channel, replyMsgid, emoji);
            emit replyBarCleared();
        } else if (emoji.isEmpty()) {
            m_model->localMessage(host, channel, "Usage: /react <emoji> (set a reply target first)");
        } else {
            m_model->localMessage(host, channel, "No message selected — right-click a timestamp to reply first");
        }
    } else if (cmd == "/ignore") {
        const QStringList parts = args.split(' ', Qt::SkipEmptyParts);
        const QString nick = parts.value(0).toLower();
        if (!nick.isEmpty()) {
            IgnoreTypes flags;
            for (int i = 1; i < parts.size(); ++i) {
                const QString f = parts[i].toLower();
                if (f == "pm")     flags |= IgnoreType::PM;
                if (f == "notice") flags |= IgnoreType::Notice;
                if (f == "invite") flags |= IgnoreType::Invite;
            }
            if (!flags) flags = kIgnoreAll;
            m_model->setIgnore(nick, flags);
            bool found = false;
            for (auto &entry : m_config->ignoreList) {
                if (entry.nick == nick) { entry.flags = flags; found = true; break; }
            }
            if (!found)
                m_config->ignoreList.append({nick, flags});
            Config::save(*m_config, Config::defaultPath());
            m_model->localMessage(host, channel, "Now ignoring " + nick);
        }
    } else if (cmd == "/unignore") {
        const QString nick = args.trimmed().toLower();
        if (!nick.isEmpty()) {
            m_model->clearIgnore(nick);
            m_config->ignoreList.removeIf([&](const IgnoreEntry &e){ return e.nick == nick; });
            Config::save(*m_config, Config::defaultPath());
            m_model->localMessage(host, channel, "No longer ignoring " + nick);
        }
    } else if (cmd == "/ignored") {
        if (m_config->ignoreList.isEmpty()) {
            m_model->localMessage(host, channel, "Ignore list is empty.");
        } else {
            QStringList items;
            for (const auto &entry : std::as_const(m_config->ignoreList)) {
                QStringList flagNames;
                if (entry.flags & IgnoreType::PM)     flagNames << "pm";
                if (entry.flags & IgnoreType::Notice) flagNames << "notice";
                if (entry.flags & IgnoreType::Invite) flagNames << "invite";
                items << entry.nick + " [" + flagNames.join(",") + "]";
            }
            m_model->localMessage(host, channel, "Ignored nicks: " + items.join(", "));
        }
    } else if (cmd == "/monitor") {
        const QString sub  = args.section(' ', 0, 0).toLower();
        const QString nick = args.section(' ', 1, 1).trimmed();
        if (sub == "add" && !nick.isEmpty()) {
            if (!m_config->monitorList.contains(nick, Qt::CaseInsensitive))
                m_config->monitorList.append(nick);
            Config::save(*m_config, Config::defaultPath());
            m_model->monitorAdd(host, nick);
            m_model->localMessage(host, channel, "Added " + nick + " to monitor list");
        } else if ((sub == "del" || sub == "remove") && !nick.isEmpty()) {
            m_config->monitorList.removeIf([&](const QString &n){
                return n.compare(nick, Qt::CaseInsensitive) == 0;
            });
            Config::save(*m_config, Config::defaultPath());
            m_model->monitorRemove(host, nick);
            m_model->localMessage(host, channel, "Removed " + nick + " from monitor list");
        } else if (sub == "list") {
            m_model->localMessage(host, channel,
                m_config->monitorList.isEmpty()
                    ? "Monitor list is empty"
                    : "Monitor list: " + m_config->monitorList.join(", "));
        } else if (sub == "clear") {
            m_config->monitorList.clear();
            Config::save(*m_config, Config::defaultPath());
            m_model->monitorClear(host);
            m_model->localMessage(host, channel, "Monitor list cleared");
        } else if (sub == "status") {
            m_model->monitorStatus(host);
        } else {
            m_model->localMessage(host, channel,
                "Usage: /monitor add|del|list|clear|status [nick]");
        }
    } else if (cmd == "/clear") {
        emit clearChat();
    } else if (cmd == "/help") {
        const QStringList lines = {
            "Available commands:",
            "  /join <channel> [key]       — join a channel",
            "  /j <channel> [key]          — alias for /join",
            "  /part [message]             — leave the current channel",
            "  /nick <newnick>             — change your nick",
            "  /me <action>                — send an action (/me waves)",
            "  /msg <target> <message>     — send a private message",
            "  /query <nick>               — open a PM buffer without sending",
            "  /ns <text>                  — message NickServ",
            "  /cs <text>                  — message ChanServ",
            "  /bs <text>                  — message BotServ",
            "  /ms <text>                  — message MemoServ",
            "  /oper <user> <pass>         — IRC operator login",
            "  /notice <target> <message>  — send a NOTICE",
            "  /topic [text]               — show or set the channel topic",
            "  /kick <nick> [reason]       — kick a user",
            "  /invite <nick> [#channel]   — invite a user to a channel",
            "  /mode <target> <flags>      — set channel or user modes",
            "  /op <nick>                  — give op (+o)",
            "  /deop <nick>                — remove op (-o)",
            "  /voice <nick>               — give voice (+v)",
            "  /devoice <nick>             — remove voice (-v)",
            "  /ban <mask>                 — ban a mask (+b)",
            "  /unban <mask>               — remove a ban (-b)",
            "  /ping <nick>                — CTCP PING a user",
            "  /time <nick>                — query a user's local time",
            "  /away [message]             — set away status",
            "  /back                       — clear away status",
            "  /whois <nick>               — request WHOIS info",
            "  /stats <query>              — server stats (u=uptime, o=opers, m=commands…)",
            "  /whowas <nick>              — request WHOWAS info for a departed nick",
            "  /setname <realname>         — change your realname (IRCv3 setname)",
            "  /displayname [text]         — set your display name (draft/metadata; leave blank to clear)",
            "  /avatar [url]               — set your avatar URL (draft/metadata; leave blank to clear)",
            "  /chanavatar [url]           — set this channel's sidebar avatar (needs op; blank to clear)",
            "  /status [text]              — set your status text, shown in nick tooltips (draft/metadata; blank to clear)",
            "  /persistence [on|off|default] — stay on the server while disconnected (draft/persistence)",
            "  /list [filter]              — list channels on the server",
            "  /motd [server]              — request the MOTD",
            "  /version [nick]             — request VERSION (nick optional)",
            "  /ctcp <target> <cmd> [args] — send a CTCP request",
            "  /sysinfo                    — post client/system info to channel",
            "  /react <emoji>              — react to the currently selected message",
            "  /ignore <nick> [pm] [notice] [invite]  — suppress PMs/notices/invites (default: all)",
            "  /unignore <nick>            — stop ignoring nick",
            "  /ignored                    — list ignored nicks and their flags",
            "  /monitor add|del|list|clear|status [nick]  — watch list (MONITOR)",
            "  /clear                      — clear the chat buffer",
            "  /quote <raw>  /raw <raw>    — send a raw IRC line",
            "  /connect [host[:port]]      — reconnect, or connect to a server",
            "  /server [host[:port]]       — alias for /connect",
            "  /disconnect                 — close the current server and all its channels",
            "  /quit [message]             — disconnect from server",
        };
        for (const QString &line : lines)
            m_model->localMessage(host, channel, line);
        // Append user-defined script commands
        bool headerShown = false;
        for (const auto &sb : std::as_const(m_config->scripts)) {
            if (!sb.enabled) continue;
            if (!headerShown) {
                m_model->localMessage(host, channel, "");
                m_model->localMessage(host, channel, "User Scripts:");
                headerShown = true;
            }
            m_model->localMessage(host, channel,
                "  /" + sb.command + " — " + sb.path);
        }
    } else {
        const QString cmdName = cmd.mid(1).toLower();
        const ScriptBinding *binding = nullptr;
        for (const auto &sb : std::as_const(m_config->scripts)) {
            if (sb.enabled && sb.command == cmdName) {
                binding = &sb;
                break;
            }
        }
        if (binding) {
            executeScript(*binding, args, host, channel);
        } else {
            m_model->localMessage(host, channel,
                "Unknown command: " + cmd + "  (use /raw or /quote to send raw IRC)");
        }
    }

    return true;
}

void CommandDispatcher::executeScript(const ScriptBinding &binding,
                                       const QString &args,
                                       const ServerId &host, const BufferId &channel)
{
    const QString scriptPath = binding.path;

    QFileInfo fi(scriptPath);
    if (!fi.exists()) {
        m_model->localMessage(host, channel, "Script not found: " + scriptPath);
        return;
    }
    if (!fi.isExecutable()
#ifdef Q_OS_WIN
        && !scriptPath.endsWith(QLatin1String(".sh"))
#endif
    ) {
        m_model->localMessage(host, channel, "Script is not executable: " + scriptPath);
        return;
    }

    const QString nick   = m_model->selfNick(host);
    const QString server = host.str();
    const QString chan   = channel.str();

    m_model->localMessage(host, channel, "Running /" + binding.command + "...");

    auto *thread = QThread::create([this, scriptPath, args, nick, server, chan, host, channel]() {
        QProcess proc;

        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert("UPLINK_NICK",    nick);
        env.insert("UPLINK_SERVER",  server);
        env.insert("UPLINK_CHANNEL", chan);
        env.insert("UPLINK_ARGS",    args);
        proc.setProcessEnvironment(env);

        QStringList argList;
        if (!args.isEmpty())
            argList = args.split(' ', Qt::SkipEmptyParts);

        // Launch .sh scripts through bash resolved from PATH rather than trusting
        // the script's shebang — FreeBSD keeps bash at /usr/local/bin/bash, so a
        // `#!/bin/bash` script would otherwise fail to exec. Other scripts run
        // directly and rely on their own interpreter line.
        if (scriptPath.endsWith(QLatin1String(".sh")))
            proc.start(QStringLiteral("bash"), QStringList{scriptPath} + argList);
        else
            proc.start(scriptPath, argList);

        if (!proc.waitForStarted(5000)) {
            QMetaObject::invokeMethod(this, [this, host, channel]() {
                m_model->localMessage(host, channel,
                    "Could not start script — is its interpreter (e.g. bash) installed and in PATH?");
            }, Qt::QueuedConnection);
            return;
        }

        if (!proc.waitForFinished(10000)) {
            proc.kill();
            proc.waitForFinished(1000);
            QMetaObject::invokeMethod(this, [this, host, channel]() {
                m_model->localMessage(host, channel, "Script timed out (10s limit).");
            }, Qt::QueuedConnection);
            return;
        }

        const int exitCode = proc.exitCode();
        const QString stdoutText = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
        const QString stderrText = QString::fromUtf8(proc.readAllStandardError()).trimmed();

        QMetaObject::invokeMethod(this, [this, host, channel, exitCode,
                                          stdoutText, stderrText]() {
            if (exitCode != 0) {
                m_model->localMessage(host, channel,
                    "Script exited with code " + QString::number(exitCode));
                if (!stderrText.isEmpty())
                    m_model->localMessage(host, channel, "stderr: " + stderrText.left(500));
                return;
            }

            if (stdoutText.isEmpty()) {
                m_model->localMessage(host, channel, "Script produced no output.");
                return;
            }

            const QStringList lines = stdoutText.split('\n', Qt::SkipEmptyParts);
            const int maxLines = 5;
            const int count = static_cast<int>(qMin<qsizetype>(lines.size(), maxLines));
            for (int i = 0; i < count; ++i) {
                const QString line = lines[i].trimmed().left(450);
                if (!line.isEmpty())
                    m_model->sendMessage(host, channel, line);
            }
            if (lines.size() > maxLines)
                m_model->localMessage(host, channel,
                    QString("(%1 more lines suppressed)").arg(lines.size() - maxLines));
        }, Qt::QueuedConnection);
    });

    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    trackWorker(thread);

    auto *timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(thread, &QThread::finished, timer, [timer]() {
        timer->stop();
        timer->deleteLater();
    });
    timer->start(12000);
    thread->start();
}
