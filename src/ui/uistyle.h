#pragma once
#include <QFontDatabase>
#include <QFontInfo>
#include <QString>
#include <QtGlobal>

// Shared QSS snippets for plain chrome buttons (header search/pop-out/close/reveal
// buttons) that don't come from the loaded theme stylesheet.

namespace UiStyle {

inline QString headerButtonStyle()
{
    return QStringLiteral(
        "QToolButton { background: transparent; border: none; }"
        "QToolButton:hover { background: rgba(255,255,255,0.08); border-radius: 4px; }");
}

// Configured family if it's actually installed, otherwise the user's system
// monospace. Fonts are built as {family, emoji fonts...} — if the configured
// family is missing, Qt promotes the next installed candidate, and typing
// through Noto Color Emoji is not an experience anyone chose. The config
// value itself is never rewritten.
inline QString effectiveFontFamily(const QString &configured)
{
    if (QFontDatabase::hasFamily(configured))
        return configured;
    // systemFont() can return a fontconfig alias ("monospace"), which never
    // matches inside a setFamilies() list — resolve it to the concrete
    // family it stands for.
    const QString fallback =
        QFontInfo(QFontDatabase::systemFont(QFontDatabase::FixedFont)).family();
    static QString warned;
    if (warned != configured) {
        warned = configured;
        qInfo("uistyle: configured font \"%s\" is not installed, using \"%s\"",
              qPrintable(configured), qPrintable(fallback));
    }
    return fallback;
}

} // namespace UiStyle
