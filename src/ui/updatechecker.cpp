#include "ui/updatechecker.h"
#include "version.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QProgressDialog>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QSysInfo>
#include <QUrl>

UpdateChecker::UpdateChecker(QWidget *parentWindow)
    : QObject(parentWindow), m_window(parentWindow)
{
}

void UpdateChecker::check()
{
    auto *nam = new QNetworkAccessManager(this);
    QNetworkRequest req(QUrl("https://api.github.com/repos/noderelay/UplinkIRC/releases/latest"));
    req.setRawHeader("User-Agent", "Uplink/" UPLINK_VERSION);
    req.setTransferTimeout(15000);
    auto *reply = nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, nam]{
        reply->deleteLater();
        nam->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            QMessageBox::warning(m_window, "Update Check",
                "Could not check for updates:\n" + reply->errorString());
            return;
        }
        const QByteArray body = reply->readAll();
        const QJsonDocument doc = QJsonDocument::fromJson(body);
        const QString tag = doc.object().value("tag_name").toString();
        const QRegularExpression re(R"(^v?(\d+)\.(\d+)\.(\d+)$)");
        const auto m = re.match(tag);
        if (!m.hasMatch()) {
            QMessageBox::warning(m_window, "Update Check", "Could not parse release info.");
            return;
        }
        const int maj = m.captured(1).toInt();
        const int min = m.captured(2).toInt();
        const int pat = m.captured(3).toInt();
        const bool newer = (maj != UPLINK_VERSION_MAJOR) ? maj > UPLINK_VERSION_MAJOR
                         : (min != UPLINK_VERSION_MINOR) ? min > UPLINK_VERSION_MINOR
                         : pat > UPLINK_VERSION_PATCH;
        if (!newer) {
            QMessageBox::information(m_window, "Up to Date",
                "You are running the latest version (v" UPLINK_VERSION ").");
            return;
        }

        const QString newVer = QString("v%1.%2.%3").arg(maj).arg(min).arg(pat);
        const QJsonArray assets = doc.object().value("assets").toArray();

        // Find the right release asset for this platform/runtime
        QString downloadUrl, assetName;
#if defined(Q_OS_LINUX)
        if (qEnvironmentVariableIsSet("APPIMAGE")) {
            const QString arch = QSysInfo::currentCpuArchitecture() == "arm64"
                               ? "arm64" : "x86_64";
            for (const QJsonValue &v : assets) {
                const QString name = v.toObject().value("name").toString();
                if (name.endsWith(arch + ".AppImage")) {
                    downloadUrl = v.toObject().value("browser_download_url").toString();
                    assetName   = name;
                    break;
                }
            }
        }
#elif defined(Q_OS_WIN)
        for (const QJsonValue &v : assets) {
            const QString name = v.toObject().value("name").toString();
            if (name.endsWith("windows-x64.zip")) {
                downloadUrl = v.toObject().value("browser_download_url").toString();
                assetName   = name;
                break;
            }
        }
#elif defined(Q_OS_MACOS)
        {
            const QString arch = QSysInfo::currentCpuArchitecture() == "arm64"
                               ? "arm64" : "x86_64";
            for (const QJsonValue &v : assets) {
                const QString name = v.toObject().value("name").toString();
                if (name.contains("macos") && name.contains(arch) && name.endsWith(".dmg")) {
                    downloadUrl = v.toObject().value("browser_download_url").toString();
                    assetName   = name;
                    break;
                }
            }
        }
#endif

        // Non-downloadable platforms: just inform
#if defined(Q_OS_LINUX)
        if (!qEnvironmentVariableIsSet("APPIMAGE")) {
            // A binary under /usr (but not /usr/local) was put there by the
            // system package manager (e.g. the AUR packages) — self-updating
            // would fight it. /usr/local and everywhere else = manual builds.
            const QString exe = QCoreApplication::applicationFilePath();
            const bool pkgInstall = exe.startsWith("/usr/") && !exe.startsWith("/usr/local/");
            if (pkgInstall) {
                QMessageBox::information(m_window, "Update Available",
                    QString("Uplink %1 is available (you are on v" UPLINK_VERSION ").\n\n"
                            "This copy was installed by your package manager, so update it there.\n"
                            "On Arch: yay -Syu uplink-irc (or uplink-irc-bin).")
                        .arg(newVer));
            } else {
                QMessageBox::information(m_window, "Update Available",
                    QString("Uplink %1 is available (you are on v" UPLINK_VERSION ").\n\n"
                            "You appear to be running from source or a tarball.\n"
                            "Rebuild from source or download the AppImage from the releases page.")
                        .arg(newVer));
            }
            return;
        }
#elif defined(Q_OS_FREEBSD)
        QMessageBox::information(m_window, "Update Available",
            QString("Uplink %1 is available (you are on v" UPLINK_VERSION ").\n\n"
                    "Update via your ports tree or pkg.").arg(newVer));
        return;
#endif

        if (downloadUrl.isEmpty()) {
            QMessageBox::information(m_window, "Update Available",
                QString("Uplink %1 is available (you are on v" UPLINK_VERSION ").\n\n"
                        "No download found for this platform — visit the releases page.")
                    .arg(newVer));
            return;
        }

#if defined(Q_OS_WIN)
        const QString actionLabel = "Download the update ZIP to your Downloads folder";
#elif defined(Q_OS_MACOS)
        const QString actionLabel = "Download and open the DMG";
#else
        const QString actionLabel = "Download and install now";
#endif
        const int ret = QMessageBox::question(m_window, "Update Available",
            QString("Uplink %1 is available (you are on v" UPLINK_VERSION ").\n\n%2?")
                .arg(newVer, actionLabel),
            QMessageBox::Yes | QMessageBox::No);
        if (ret != QMessageBox::Yes) return;

        downloadAndApplyUpdate(downloadUrl, assetName);
    });
}

void UpdateChecker::downloadAndApplyUpdate(const QString &url, const QString &assetName)
{
    const QString tempPath = QDir::tempPath() + "/" + assetName;

    auto *f = new QFile(tempPath, this);
    if (!f->open(QIODevice::WriteOnly)) {
        QMessageBox::warning(m_window, "Update Failed", "Could not create temporary file.");
        f->deleteLater();
        return;
    }

    auto *progress = new QProgressDialog("Downloading " + assetName + "...", "Cancel", 0, 100, m_window);
    progress->setWindowModality(Qt::WindowModal);
    progress->setMinimumDuration(0);
    progress->setValue(0);

    auto *nam = new QNetworkAccessManager(this);
    QNetworkRequest req{QUrl(url)};
    req.setRawHeader("User-Agent", "Uplink/" UPLINK_VERSION);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    // Inactivity timeout — resets while bytes flow, so slow-but-alive
    // downloads survive; only a genuine stall aborts.
    req.setTransferTimeout(30000);
    auto *reply = nam->get(req);

    connect(reply, &QNetworkReply::readyRead, f, [reply, f]{ f->write(reply->readAll()); });

    connect(reply, &QNetworkReply::downloadProgress, progress,
        [progress](qint64 received, qint64 total){
            if (total > 0)
                progress->setValue(static_cast<int>(received * 100 / total));
        });

    connect(progress, &QProgressDialog::canceled, reply, &QNetworkReply::abort);

    connect(reply, &QNetworkReply::finished, this,
        [this, reply, nam, progress, f, tempPath, assetName]{
            f->close();
            f->deleteLater();
            progress->deleteLater();
            reply->deleteLater();
            nam->deleteLater();

            if (reply->error() == QNetworkReply::OperationCanceledError) {
                QFile::remove(tempPath);
                return;
            }
            if (reply->error() != QNetworkReply::NoError) {
                QFile::remove(tempPath);
                QMessageBox::warning(m_window, "Update Failed",
                    "Download failed:\n" + reply->errorString());
                return;
            }

            applyUpdate(tempPath, assetName);
        });
}

void UpdateChecker::applyUpdate(const QString &tempPath, const QString &assetName)
{
#if defined(Q_OS_LINUX)
    Q_UNUSED(assetName);
    // Linux allows replacing a running file — the kernel holds the old inode open
    const QString appImagePath = qEnvironmentVariable("APPIMAGE");
    QFile::remove(appImagePath);
    if (!QFile::copy(tempPath, appImagePath)) {
        QMessageBox::warning(m_window, "Update Failed",
            "Could not replace the AppImage.\n"
            "You can update manually:\n" + tempPath + "\n→ " + appImagePath);
        return;
    }
    QFile::remove(tempPath);
    QFile(appImagePath).setPermissions(
        QFileDevice::ReadOwner  | QFileDevice::WriteOwner | QFileDevice::ExeOwner |
        QFileDevice::ReadGroup  | QFileDevice::ExeGroup   |
        QFileDevice::ReadOther  | QFileDevice::ExeOther);
    QMessageBox::information(m_window, "Update Ready", "Uplink has been updated. It will now restart.");
    QProcess::startDetached(appImagePath, QCoreApplication::arguments());
    QCoreApplication::quit();

#elif defined(Q_OS_WIN)
    const QString dest = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)
                       + "/" + assetName;
    QFile::remove(dest);
    if (!QFile::copy(tempPath, dest)) {
        QMessageBox::warning(m_window, "Update Failed",
            "Could not save to Downloads.\nThe file is at:\n" + tempPath);
        return;
    }
    QFile::remove(tempPath);
    QMessageBox::information(m_window, "Update Downloaded",
        "Saved to:\n" + dest + "\n\n"
        "Quit Uplink, extract the ZIP, and replace your current installation.");
    QDesktopServices::openUrl(QUrl::fromLocalFile(
        QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)));

#elif defined(Q_OS_MACOS)
    Q_UNUSED(assetName);
    QDesktopServices::openUrl(QUrl::fromLocalFile(tempPath));
    QMessageBox::information(m_window, "Update Ready",
        "The DMG has been opened in Finder.\n"
        "Drag Uplink to Applications to complete the update, then relaunch.");

#else
    Q_UNUSED(tempPath);
    Q_UNUSED(assetName);
#endif
}
