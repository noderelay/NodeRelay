#pragma once

#include <QObject>
#include <QString>

class QWidget;

// Checks GitHub releases for a newer version and downloads/installs it.
// Platform behavior: AppImage replaces in-place and relaunches; Windows
// saves the ZIP to Downloads; macOS opens the DMG; source builds and
// FreeBSD get an informational message.
class UpdateChecker : public QObject
{
    Q_OBJECT
public:
    explicit UpdateChecker(QWidget *parentWindow);

    void check();

private:
    void downloadAndApplyUpdate(const QString &url, const QString &assetName);
    void applyUpdate(const QString &tempPath, const QString &assetName);

    QWidget *m_window;
};
