#pragma once

#include "channel.h"
#include <QHash>
#include <QRegularExpression>
#include <QSet>
#include <QString>

struct NickMeta {
    QString displayName;
    QString avatarUrl;
};

struct ServerSession {
    QString  name;        // display name from config
    QString  host;
    QString  nick;
    bool     connected{false};
    bool     away{false};
    QRegularExpression mentionRe;   // pre-compiled; rebuilt when nick changes
    QRegularExpression highlightRe; // extra keyword highlights; rebuilt from config

    QSet<QString> botNicks;     // lowercased nicks with +B user mode (global)
    static constexpr int kNickMetaCap = 300;
    QHash<QString, NickMeta> nickMeta; // lowercase nick → metadata; capped at kNickMetaCap
    QSet<QString> metaRequested;       // lowercase nicks with a METADATA GET in flight or answered

    void setNickMeta(const QString &lowerNick, const QString &key, const QString &value)
    {
        if (!nickMeta.contains(lowerNick) && nickMeta.size() >= kNickMetaCap)
            nickMeta.erase(nickMeta.begin());
        auto &m = nickMeta[lowerNick];
        if (key == "display-name") m.displayName = value;
        else if (key == "avatar")  m.avatarUrl   = value;
    }

    // key = channel name lowercased
    QHash<QString, Channel> channels;

    Channel &getOrCreate(const QString &chanName)
    {
        const QString key = chanName.toLower();
        if (!channels.contains(key)) {
            Channel ch;
            ch.name = chanName;
            channels.insert(key, ch);
        }
        return channels[key];
    }

    Channel *get(const QString &chanName)
    {
        const QString key = chanName.toLower();
        auto it = channels.find(key);
        return it == channels.end() ? nullptr : &it.value();
    }

    // Server-level messages go into a pseudo-channel called "(server)"
    Channel &serverBuffer() { return getOrCreate("(server)"); }
};
