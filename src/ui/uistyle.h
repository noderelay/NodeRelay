#pragma once
#include <QString>

// Shared QSS snippets for plain chrome buttons (header search/pop-out/close/reveal
// buttons) that don't come from the loaded theme stylesheet.

namespace UiStyle {

inline QString headerButtonStyle()
{
    return QStringLiteral(
        "QToolButton { background: transparent; border: none; }"
        "QToolButton:hover { background: rgba(255,255,255,0.08); border-radius: 4px; }");
}

} // namespace UiStyle
