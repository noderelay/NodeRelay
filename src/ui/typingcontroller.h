#pragma once

#include <QHash>
#include <QObject>
#include <QSet>
#include <QString>

#include "model/ids.h"

class SessionModel;
class QTimer;

// Owns typing-indicator state in both directions: outbound TAGMSGs for the
// user's own typing (active on first keypress, paused after 5s idle, done on
// send/clear/buffer switch) and the per-buffer set of peers currently typing
// (each nick expires 6s after its last TAGMSG). Emits typersChanged so the
// UI can refresh its "X is typing..." labels.
class TypingController : public QObject
{
    Q_OBJECT
public:
    explicit TypingController(SessionModel *model, QObject *parent = nullptr);

    void setEnabled(bool on);   // disabling clears all state and timers
    bool enabled() const { return m_enabled; }

    // Outbound: the user's own typing in the active buffer.
    void noteInputChanged(bool hasText);                                 // input box edited by the user
    void noteMessageSent(const ServerId &host, const BufferId &channel); // input submitted
    void noteBufferLeft (const ServerId &host, const BufferId &channel); // switched away mid-draft

    // Inbound: peers typing in a buffer.
    QString typingText(const ServerId &host, const BufferId &channel) const;
    void forgetNick(const ServerId &host, const BufferId &channel, const QString &nick);
    void forgetHost(const ServerId &host);

signals:
    void typersChanged();

private:
    void onTypingReceived(const ServerId &host, const BufferId &channel,
                          const QString &nick, const QString &state);
    void endSelfTyping(const ServerId &host, const BufferId &channel);

    SessionModel *m_model;
    bool          m_enabled{true};

    QTimer *m_selfTimer{nullptr};   // 5s inactivity → "paused"
    bool    m_selfTyping{false};

    QHash<QString, QSet<QString>> m_typers;      // paneKey → nicks
    QHash<QString, QTimer*>       m_nickTimers;  // paneKey|nick → 6s expiry
};
