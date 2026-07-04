#pragma once

#include "message.h"
#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <QHash>
#include <QList>
#include <QSet>
#include <QStringList>

static constexpr int kMessageBufferCap = 500;

// Nick prefix rank: ~ & @ % + (owner → admin → op → halfop → voice)
inline int prefixRank(QChar p)
{
    switch (p.unicode()) {
    case '~': return 5;
    case '&': return 4;
    case '@': return 3;
    case '%': return 2;
    case '+': return 1;
    default:  return 0;
    }
}

struct NickEntry {
    QString     nick;
    QString     lowerNick; // pre-computed nick.toLower() for fast comparisons
    QString     account;   // NickServ account, empty if unknown or logged out
    QChar       prefix{' '};
    QSet<QChar> prefixes;

    void recomputePrefix()
    {
        prefix = ' ';
        for (QChar p : std::as_const(prefixes))
            if (prefixRank(p) > prefixRank(prefix))
                prefix = p;
    }

    QString display() const
    {
        return (prefix != ' ' && prefix != '\0') ? QString(prefix) + nick : nick;
    }

    bool operator<(const NickEntry &o) const
    {
        int ra = prefixRank(prefix), rb = prefixRank(o.prefix);
        if (ra != rb) return ra > rb;
        return lowerNick < o.lowerNick;
    }
};

struct Channel {
    QString          name;
    QString          topic;
    QString          topicSetBy;
    quint64          topicSetAt{0};
    QString          modes;
    QList<NickEntry>          nicks;
    QHash<QString, qsizetype> nickIndex; // lowercase nick → index in nicks
    QList<Message>   messages;
    QSet<QString>    botNicks;  // lowercased nicks with +B channel user mode
    struct PreviewCard {
        QString    title;
        QString    domain;
        QString    pageUrl;
        QByteArray pngData; // compressed PNG bytes; reconstruct QPixmap in the view layer
    };
    QHash<QString, PreviewCard> previews;      // url → card data
    QSet<QString>               hiddenPreviews; // urls the user manually hid
    // msgid → emoji → set of nicks who reacted (QSet deduplicates per nick)
    QHash<QString, QHash<QString, QSet<QString>>> reactions;
    int              unread{0};
    int              mentions{0};
    int              firstUnreadIdx{-1}; // index of first unread msg; -1 when none
    bool             joined{false};
    QDateTime        lastRead;  // soju.im/read marker

    void addMessage(const Message &msg)
    {
        messages.append(msg);
        while (messages.size() > kMessageBufferCap) {
            const QString oldId = messages.front().msgid;
            messages.removeFirst();
            if (!oldId.isEmpty())
                reactions.remove(oldId);
        }
    }

    void prependMessages(const QList<Message> &msgs)
    {
        if (msgs.isEmpty()) return;
        // Prepended history sits at the front; trimming drops the newest tail.
        // Copy only the messages that survive the cap instead of concatenating
        // the whole buffer and discarding the overflow afterwards.
        const qsizetype keepOld = qMax<qsizetype>(0, kMessageBufferCap - msgs.size());
        for (qsizetype i = keepOld; i < messages.size(); ++i) {
            const QString &oldId = messages[i].msgid;
            if (!oldId.isEmpty())
                reactions.remove(oldId);
        }
        QList<Message> combined;
        combined.reserve(qMin<qsizetype>(kMessageBufferCap, msgs.size() + messages.size()));
        combined.append(msgs);
        for (qsizetype i = 0; i < keepOld && i < messages.size(); ++i)
            combined.append(messages[i]);
        messages = std::move(combined);
        // If the backfill batch alone exceeds the cap, trim its tail.
        while (messages.size() > kMessageBufferCap) {
            const QString oldId = messages.back().msgid;
            messages.removeLast();
            if (!oldId.isEmpty())
                reactions.remove(oldId);
        }
    }

    void rebuildNickIndex()
    {
        nickIndex.clear();
        nickIndex.reserve(nicks.size());
        for (qsizetype i = 0; i < nicks.size(); ++i)
            nickIndex[nicks[i].lowerNick] = i;
    }

    void setNicks(const QStringList &raw)
    {
        nicks.clear();
        for (const QString &n : raw) {
            if (n.isEmpty()) continue;
            NickEntry e;
            int i = 0;
            while (i < n.size() && prefixRank(n[i]) > 0) {
                e.prefixes.insert(n[i]);
                ++i;
            }
            e.recomputePrefix();
            e.nick = n.mid(i);
            // userhost-in-names: strip !user@host suffix if present
            const qsizetype bang = e.nick.indexOf('!');
            if (bang != -1)
                e.nick = e.nick.left(bang);
            e.lowerNick = e.nick.toLower();
            nicks.append(e);
        }
        std::sort(nicks.begin(), nicks.end());
        rebuildNickIndex();
    }

    void setNickAccount(const QString &nick, const QString &account)
    {
        const auto it = nickIndex.constFind(nick.toLower());
        if (it == nickIndex.constEnd()) return;
        auto &e = nicks[it.value()];
        e.account = (account == "*") ? QString() : account;
    }

    // Parse a raw NAMES-style token (optional prefix chars + nick) into an entry.
    static NickEntry parseNick(const QString &raw)
    {
        NickEntry e;
        int i = 0;
        while (i < raw.size() && prefixRank(raw[i]) > 0) {
            e.prefixes.insert(raw[i]);
            ++i;
        }
        e.recomputePrefix();
        e.nick = raw.mid(i);
        e.lowerNick = e.nick.toLower();
        return e;
    }

    void addNick(const QString &raw)
    {
        NickEntry e = parseNick(raw);
        if (nickIndex.contains(e.lowerNick)) return;
        const qsizetype idx = std::lower_bound(nicks.cbegin(), nicks.cend(), e) - nicks.cbegin();
        nicks.insert(idx, e);
        rebuildNickIndex();
    }

    // Batched insert for JOIN bursts (netjoin). Appends all new nicks, then
    // sorts and rebuilds the index once — O(m + n log n) instead of O(m·n).
    void addNicks(const QStringList &raw)
    {
        bool changed = false;
        for (const QString &r : raw) {
            NickEntry e = parseNick(r);
            if (e.nick.isEmpty() || nickIndex.contains(e.lowerNick)) continue;
            // Guard against duplicates within the batch itself.
            nickIndex.insert(e.lowerNick, -1);
            nicks.append(e);
            changed = true;
        }
        if (!changed) return;
        std::sort(nicks.begin(), nicks.end());
        rebuildNickIndex();
    }

    void removeNick(const QString &nick)
    {
        const auto it = nickIndex.find(nick.toLower());
        if (it == nickIndex.end()) return;
        const qsizetype idx = it.value();
        nicks.removeAt(idx);
        rebuildNickIndex();
    }

    void renameNick(const QString &oldNick, const QString &newNick)
    {
        const auto it = nickIndex.constFind(oldNick.toLower());
        if (it == nickIndex.constEnd()) return;
        nicks[it.value()].nick = newNick;
        nicks[it.value()].lowerNick = newNick.toLower();
        std::sort(nicks.begin(), nicks.end());
        rebuildNickIndex();
    }

    void removeNicks(const QSet<QString> &lowerNicks)
    {
        nicks.removeIf([&](const NickEntry &e){
            return lowerNicks.contains(e.lowerNick);
        });
        rebuildNickIndex();
    }

    static constexpr int kPreviewCap = 4;

    void addPreview(const QString &url, const PreviewCard &card)
    {
        if (previews.size() >= kPreviewCap) {
            const QString evicted = previews.begin().key();
            hiddenPreviews.remove(evicted);
            previews.erase(previews.begin());
        }
        previews.insert(url, card);
    }
};
