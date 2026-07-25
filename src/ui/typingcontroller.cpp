#include "typingcontroller.h"
#include "model/sessionmodel.h"

#include <QTimer>
#include <utility>

TypingController::TypingController(SessionModel *model, QObject *parent)
    : QObject(parent), m_model(model)
{
    // Inactivity timer: sends typing=paused after 5s with no keypresses
    m_selfTimer = new QTimer(this);
    m_selfTimer->setSingleShot(true);
    m_selfTimer->setInterval(5000);
    connect(m_selfTimer, &QTimer::timeout, this, [this]{
        if (!m_enabled || !m_selfTyping) return;
        // The buffer the typing was happening in, not whatever is active by
        // the time the pause fires — panes type without becoming active.
        m_selfTyping = false;
        m_model->sendTyping(m_selfHost, m_selfChannel, "paused");
    });

    connect(m_model, &SessionModel::typingReceived,
            this, &TypingController::onTypingReceived);
}

void TypingController::setEnabled(bool on)
{
    m_enabled = on;
    if (on) return;
    m_selfTimer->stop();
    m_selfTyping = false;
    m_typers.clear();
    for (auto *t : std::as_const(m_nickTimers)) { t->stop(); t->deleteLater(); }
    m_nickTimers.clear();
}

void TypingController::noteInputChanged(const ServerId &host, const BufferId &ch, bool hasText)
{
    if (!m_enabled) return;
    if (ch.isEmpty() || ch.str() == "(server)") return;
    // Typing moved to a different input (main view vs pane) — close out the
    // old buffer's state first so it doesn't dangle as "active" over there.
    if (m_selfTyping && (host != m_selfHost || ch.str() != m_selfChannel.str()))
        endSelfTyping(m_selfHost, m_selfChannel);
    if (hasText) {
        if (!m_selfTyping) {
            m_selfTyping = true;
            m_selfHost    = host;
            m_selfChannel = ch;
            m_model->sendTyping(host, ch, "active");
        }
        m_selfTimer->start();
    } else {
        m_selfTimer->stop();
        if (m_selfTyping) {
            m_selfTyping = false;
            m_model->sendTyping(host, ch, "done");
        }
    }
}

void TypingController::noteMessageSent(const ServerId &host, const BufferId &channel)
{
    endSelfTyping(host, channel);
}

void TypingController::noteBufferLeft(const ServerId &host, const BufferId &channel)
{
    endSelfTyping(host, channel);
}

// Stop the inactivity timer and close out an in-flight typing state with a
// final "done". The buffer must be passed in by the caller: on a switch the
// TAGMSG has to go to the buffer being left, not whatever is active by the
// time the timer would have fired.
void TypingController::endSelfTyping(const ServerId &host, const BufferId &channel)
{
    // Only an in-flight state for THIS buffer — leaving or closing one view
    // must not close out typing that belongs to another.
    if (!m_selfTyping || host != m_selfHost
        || channel.str().compare(m_selfChannel.str(), Qt::CaseInsensitive) != 0)
        return;
    m_selfTimer->stop();
    if (m_enabled && channel.str() != "(server)")
        m_model->sendTyping(host, channel, "done");
    m_selfTyping = false;
}

// Remove a nick from a buffer's typer set without operator[]'s
// default-insert, and drop the set once it empties — the map otherwise
// keeps one dead key per buffer that ever saw a typing TAGMSG.
void TypingController::removeTyper(const QString &key, const QString &nick)
{
    auto it = m_typers.find(key);
    if (it == m_typers.end()) return;
    it->remove(nick);
    if (it->isEmpty()) m_typers.erase(it);
}

void TypingController::onTypingReceived(const ServerId &host, const BufferId &channel,
                                        const QString &nick, const QString &state)
{
    if (!m_enabled) return;

    // Lowercase the channel so lookups match regardless of source case
    // (restored panes carry lowercased names; the server sends its own case).
    const QString key      = paneKey(host, channel);
    const QString timerKey = key + "|" + nick;

    if (state == "active" || state == "paused") {
        m_typers[key].insert(nick);

        if (m_nickTimers.contains(timerKey)) {
            m_nickTimers[timerKey]->start(6000);
        } else {
            auto *t = new QTimer(this);
            t->setSingleShot(true);
            connect(t, &QTimer::timeout, this, [this, key, timerKey, nick]{
                removeTyper(key, nick);
                if (auto *timer = m_nickTimers.value(timerKey)) {
                    m_nickTimers.remove(timerKey);
                    timer->deleteLater();
                }
                emit typersChanged();
            });
            m_nickTimers.insert(timerKey, t);
            t->start(6000);
        }
    } else {
        removeTyper(key, nick);
        if (auto *t = m_nickTimers.value(timerKey)) {
            t->stop();
            t->deleteLater();
            m_nickTimers.remove(timerKey);
        }
    }

    emit typersChanged();
}

// Builds the "X is typing..." string for a buffer, or empty if nobody is.
QString TypingController::typingText(const ServerId &host, const BufferId &channel) const
{
    if (!m_enabled) return {};
    const QString key = paneKey(host, channel);
    const QSet<QString> &typers = m_typers.value(key);
    if (typers.isEmpty()) return {};

    QStringList names(typers.begin(), typers.end());
    if (names.size() == 1) return names[0] + " is typing...";
    if (names.size() == 2) return names[0] + " and " + names[1] + " are typing...";
    return QString::number(names.size()) + " people are typing...";
}

void TypingController::forgetNick(const ServerId &host, const BufferId &channel, const QString &nick)
{
    const QString key      = paneKey(host, channel);
    const QString timerKey = key + "|" + nick;
    if (auto *t = m_nickTimers.value(timerKey)) {
        t->stop();
        t->deleteLater();
        m_nickTimers.remove(timerKey);
        removeTyper(key, nick);
        emit typersChanged();
    }
}

void TypingController::forgetHost(const ServerId &host)
{
    const QString prefix = host.str() + "|";
    for (auto it = m_nickTimers.begin(); it != m_nickTimers.end(); ) {
        if (it.key().startsWith(prefix)) {
            it.value()->stop();
            it.value()->deleteLater();
            it = m_nickTimers.erase(it);
        } else {
            ++it;
        }
    }
    bool removed = false;
    for (auto it = m_typers.begin(); it != m_typers.end(); ) {
        if (it.key().startsWith(prefix)) {
            removed = !it.value().isEmpty() || removed;
            it = m_typers.erase(it);
        } else {
            ++it;
        }
    }
    if (removed)
        emit typersChanged();
}
