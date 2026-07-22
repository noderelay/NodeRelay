#pragma once

#include <QString>
#include <QStringList>

struct Theme {
    // [general]
    QString background, text, border, accent;
    // [sidebar]
    QString sidebarBg, sidebarText, sidebarActive, sidebarUnread, sidebarMention, sidebarServer;
    // [buffer]
    QString bufferBg, timestamp, serverLine, action, nickSelf, separator;
    // [highlights]
    QString mentionBg, mentionText, keyword;
    // [events] — coloured status lines (join/leave/nick/topic/notice/…)
    QString evtJoin, evtLeave, evtNick, evtNotice, evtError, evtReply, evtWallops;
    // [nicklist]
    QString nicklistBg, nicklistText, op, halfop, voice, away;
    // [input]
    QString inputBg, inputText, placeholder, inputNick;

    bool valid{false};
};

class ThemeLoader
{
public:
    static Theme    load(const QString &name);
    // panelCards: side panels get their own [sidebar]/[nicklist] backgrounds
    // and rounded-top card styling; false = classic flat look (everything on
    // the buffer color, as before v2026.7).
    static QString  toStyleSheet(const Theme &t, bool panelCards = true);
    static void     apply(const QString &name, bool panelCards = true); // load + set on QApplication
    static QStringList availableThemes();                // names without .toml
    static QString  themesDir();                         // ~/.config/uplink/themes
    static void     ensureUserThemesDir();               // create + seed on first run
};
