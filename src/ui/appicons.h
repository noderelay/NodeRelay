#pragma once

#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QIcon>
#include <QPalette>
#include <QPixmap>
#include <QStandardPaths>
#include <QString>

namespace AppIcons {

// Icon-theme name the app is known by everywhere: Wayland app_id, desktop
// entry, hicolor icon. Reverse-DNS on purpose — KIconLoader falls back by
// stripping dash-suffixes ("uplink-irc" → "uplink"), and gamer icon themes
// ship the Introversion game's icon under the bare name, shadowing ours.
// A reverse-DNS name has no dash to strip and no theme will ever ship it.
inline constexpr auto kSystemIconName = "io.github.noderelay.UplinkIRC";

// Write the chosen icon into the user hicolor dir under the app's icon
// name so desktop-entry lookups (task manager, launcher) track the
// in-app choice. Drops stale icon-theme.cache and old-named PNGs:
// rewriting a file doesn't bump the dir mtime, so a cache would pin
// the old pixmap forever.
inline void publishSystemIcon(const QIcon &icon)
{
    const QString hicolor = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
        + QStringLiteral("/icons/hicolor");
    const QString apps = hicolor + QStringLiteral("/256x256/apps");
    QDir().mkpath(apps);
    icon.pixmap(256, 256).save(apps + QStringLiteral("/") + kSystemIconName
                               + QStringLiteral(".png"));
    QFile::remove(hicolor + QStringLiteral("/icon-theme.cache"));
    QFile::remove(apps + QStringLiteral("/uplink-irc.png"));   // pre-2026.7 names
    QFile::remove(apps + QStringLiteral("/uplink.png"));
}

inline bool isDark()
{
    return qApp->palette().window().color().lightness() < 128;
}

inline QString resolveIconKey(const QString &choice)
{
    if (choice == "dark")  return QStringLiteral("flat-black");
    if (choice == "light") return QStringLiteral("original-flat-shine");
    return choice;
}

inline QIcon appIcon(const QString &choice = "flat-black")
{
    const QString key = resolveIconKey(choice);
    const QString path = QStringLiteral(":/icons/%1.png").arg(key);
    QIcon icon(path);
    if (icon.isNull())
        return QIcon(":/icons/flat-black.png");
    return icon;
}
inline QIcon trayIcon()    { return appIcon(); }
inline QPixmap aboutLogo() { return QPixmap(":/icons/about-logo.png"); }
inline QIcon aboutBanner() { return QIcon(":/icons/banner.png"); }

} // namespace AppIcons
