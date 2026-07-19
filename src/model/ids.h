#pragma once
#include <QString>
#include <QHashFunctions>

// Strong identifier types — prevent accidentally swapping a server host
// for a channel name at call sites. Explicit construction is intentional:
// passing a plain QString where ServerId/BufferId is expected is a compile error.

class ServerId {
    QString m_v;
public:
    ServerId() = default;
    explicit ServerId(const QString &v) : m_v(v) {}
    explicit ServerId(QString &&v)       : m_v(std::move(v)) {}

    const QString &str() const & { return m_v; }
    QString        str() &&      { return std::move(m_v); }

    bool isEmpty()                    const { return m_v.isEmpty(); }
    bool operator==(const ServerId &o) const { return m_v == o.m_v; }
    bool operator!=(const ServerId &o) const { return m_v != o.m_v; }
    bool operator< (const ServerId &o) const { return m_v <  o.m_v; }
};

class BufferId {
    QString m_v;
public:
    BufferId() = default;
    explicit BufferId(const QString &v) : m_v(v) {}
    explicit BufferId(QString &&v)       : m_v(std::move(v)) {}

    const QString &str() const & { return m_v; }
    QString        str() &&      { return std::move(m_v); }

    bool isEmpty()                    const { return m_v.isEmpty(); }
    bool operator==(const BufferId &o) const { return m_v == o.m_v; }
    bool operator!=(const BufferId &o) const { return m_v != o.m_v; }
    bool operator< (const BufferId &o) const { return m_v <  o.m_v; }
};

inline size_t qHash(const ServerId &id, size_t seed = 0) { return qHash(id.str(), seed); }
inline size_t qHash(const BufferId &id, size_t seed = 0) { return qHash(id.str(), seed); }

// Composite key used to track which host|channel a pane/window belongs to
// (docked ChannelPane, popped-out window, timers, etc). Channel is
// lower-cased for case-insensitive lookups.
inline QString paneKey(const QString &host, const QString &channel) {
    return host + "|" + channel.toLower();
}
inline QString paneKey(const ServerId &host, const BufferId &channel) {
    return paneKey(host.str(), channel.str());
}

// Composite key for per-buffer UI caches (scroll positions, drafts, render
// starts). Case-preserving — distinct from paneKey's lower-cased channel.
inline QString bufferKey(const ServerId &host, const BufferId &channel) {
    return host.str() + '\t' + channel.str();
}

// RFC 2811 channel name prefixes: '#' (standard), '&' (local-only),
// '+' (modeless), '!' (safe). Networks that advertise a narrower
// ISUPPORT CHANTYPES simply never issue the others, so checking all
// four here is always safe.
inline bool isChannelName(const QString &name) {
    return name.startsWith('#') || name.startsWith('&')
        || name.startsWith('+') || name.startsWith('!');
}
