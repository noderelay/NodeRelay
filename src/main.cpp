#if defined(__linux__) && !defined(__MUSL__)
#include <malloc.h>
#endif
#include <QApplication>
#include <QFont>
#include <QInputDialog>
#include <QPixmapCache>
#include <QScopedPointer>
#include "config/config.h"
#include "model/sessionmodel.h"
#include "ui/mainwindow.h"
#include "ui/themeloader.h"
#include "gitversion.h"

int main(int argc, char *argv[])
{
#if defined(__linux__) && !defined(__MUSL__)
    mallopt(M_ARENA_MAX, 2);
    mallopt(M_TRIM_THRESHOLD, 64 * 1024);
#endif
    QApplication app(argc, argv);
    QPixmapCache::setCacheLimit(2048); // 2 MB
    app.setApplicationName("Uplink");
    app.setApplicationVersion(UPLINK_VERSION_FULL);

#if defined(Q_OS_WIN)
    // Use native Windows rendering as the base style.
    // Custom themes layer QSS on top; the default theme leaves it pure native.
    if (!QApplication::setStyle("windows11"))
        if (!QApplication::setStyle("windowsvista"))
            QApplication::setStyle("windows");
#endif

    const QString cfgPath = Config::defaultPath();
    Config cfg = Config::load(cfgPath);
    ThemeLoader::ensureUserThemesDir();

    QFont font(cfg.ui.fontFamily);
    font.setStyleHint(QFont::Monospace);
    app.setFont(font);

    // First-run nick setup
    if (cfg.needsNickSetup()) {
        bool ok = false;
        const QString nick = QInputDialog::getText(
            nullptr, "Uplink — Set Your Nick",
            "Choose a nickname:", QLineEdit::Normal, "", &ok);
        if (ok && !nick.trimmed().isEmpty()) {
            for (auto &s : cfg.servers)
                if (s.nick == "yournick" || s.nick.isEmpty())
                    s.nick = nick.trimmed();
            Config::save(cfg, cfgPath);
        }
    }

    QScopedPointer<SessionModel> model(new SessionModel);

    MainWindow window(model.get(), cfg);
    window.show();

    // Load config after MainWindow has connected its signals
    model->loadConfig(cfg);

    return app.exec();
}
