// Chat view rendering glue: message -> ChatLine appends, reaction and
// redaction updates, event condensation, view refresh and scrollback
// pagination. Split out of mainwindow.cpp - all methods remain
// MainWindow members.

#if defined(__linux__) && !defined(__MUSL__)
#include <malloc.h>
#endif
#include "mainwindow.h"
#include "ui/channelpane.h"
#include "ui/chatrenderer.h"
#include "ui/chatview.h"
#include "ui/previewcontroller.h"
#include "ui/trayicon.h"
#include "model/sessionmodel.h"
#include "config/config.h"

#include <QApplication>
#include <QLabel>
#include <QListWidget>
#include <QRegularExpression>
#include <QScrollBar>
#include <QTimer>
#include <QUrl>

static constexpr int kRenderWindow = 150; // messages rendered per channel view
static constexpr int kRenderChunk  = 50;  // messages loaded per scroll-to-top

// Collect the tail run of consecutive condensable messages from ch.messages
// that share the same calendar day as the last message.
static QList<Message> collectEventGroup(const Channel *ch, const QString &selfNick)
{
    QList<Message> group;
    if (ch->messages.isEmpty()) return group;
    const QDate day = ch->messages.last().timestamp.toLocalTime().date();
    for (qsizetype i = ch->messages.size() - 1; i >= 0; --i) {
        const auto &m = ch->messages[i];
        if (!ChatRenderer::isCondensable(m, selfNick)) break;
        if (m.timestamp.toLocalTime().date() != day) break;
        group.prepend(m);
    }
    return group;
}

// Highlight regex for a host's own nick. m_selfNickRe tracks the active
// host, but panes can belong to a different server where the user may be
// under a different nick.
QRegularExpression MainWindow::selfNickReFor(const ServerId &host) const
{
    if (host == m_model->activeHost()) return m_selfNickRe;
    return SessionModel::buildHighlightRe(m_model->selfNick(host));
}

void MainWindow::onMessageAdded(ServerId host, BufferId channel, const Message &msg)
{
    const QString selfNick = m_model->selfNick(host);

    auto makeCtx = [&](Channel *ch) {
        ChatRenderer::Context ctx;
        ctx.coloredNicks = m_config.ui.coloredNicks;
        ctx.nickBrackets = m_config.ui.nickBrackets;
        ctx.emojiPt      = m_config.ui.fontSizes.emoji;
        ctx.chatPt       = m_config.ui.fontSizes.chat;
        ctx.validTheme   = m_theme.valid;
        ctx.themeText    = m_theme.text;
        ctx.selfNickRe   = selfNickReFor(host);
    ctx.highlightRe  = m_highlightRe;
    ctx.showTimestamps = m_config.ui.showTimestamps;
        ctx.channel      = ch;
        return ctx;
    };

    auto appendToView = [&](ChatView *view, Channel *ch) {
        if (ChatRenderer::isCondensable(msg, selfNick)) {
            const QList<Message> group = collectEventGroup(ch, selfNick);
            if (group.isEmpty()) return;
            const QString groupId = QString::number(group.first().timestamp.toMSecsSinceEpoch());
            const bool grpExpanded = m_expandedEventGroups.contains(groupId);
            const QString blockId  = "evgrp:" + groupId;
            const ChatLine line    = ChatRenderer::formatEventGroupLine(group, makeCtx(ch), groupId, grpExpanded);
            if (view->findLine(blockId) >= 0)
                view->replaceLine(blockId, line);
            else
                view->appendLine(line);
        } else {
            view->appendLine(ChatRenderer::formatMessageLine(msg, makeCtx(ch)));
        }
        if (view->isAtBottom()) view->scrollToBottom();
    };

    if (host == m_model->activeHost() &&
        channel.str().toLower() == m_model->activeChannel().str().toLower())
    {
        auto *ch = m_model->channel(host, channel);
        if (ch) {
            if (ChatRenderer::isCondensable(msg, selfNick)) {
                appendToView(m_chatView, ch);
            } else {
                appendMessage(msg, m_config.ui.linkPreviews);
            }
        }
    }

    const QString key = paneKey(host, channel);
    if (auto *pane = m_panes.value(key)) {
        auto *pCh = m_model->channel(host, channel);
        if (pCh) {
            appendToView(pane->chatView(), pCh);
            if (m_config.ui.linkPreviews) {
                appendPreviewCards(pane->chatView(), msg, host, channel);
                if (pane->chatView()->isAtBottom()) pane->chatView()->scrollToBottom();
            }
        }
        // Visible in a pane/window — never let it count as unread.
        m_model->markRead(host, channel);
    }

    // Suppress when either the main window or this channel's own popped-out
    // window is focused — the user is already looking at the message.
    auto *paneWin = m_paneWindows.value(key);
    const bool channelWindowActive = paneWin && paneWin->isActiveWindow();
    if (m_config.ui.notifications && m_tray && !isActiveWindow() && !channelWindowActive
        && (msg.type == MessageType::Privmsg || msg.type == MessageType::Action))
    {
        const QString myNick = m_model->selfNick(host);
        const bool isPM = !isChannelName(channel.str());
        const bool isMention = !isPM && !myNick.isEmpty()
                               && msg.text.contains(myNick, Qt::CaseInsensitive);
        if (isPM || isMention)
            m_tray->setNotify(true);
    }
}

void MainWindow::onReactionsChanged(ServerId host, BufferId channel, const QString &msgid)
{
    auto updateView = [&](ChatView *view, Channel *ch) {
        if (view->findLine(msgid) < 0) {
            qWarning() << "onReactionsChanged: findLine miss for msgid=" << msgid;
            return;
        }
        const auto rxIt = ch->reactions.constFind(msgid);
        const bool hasReactions = rxIt != ch->reactions.constEnd() && !rxIt->isEmpty();
        const QString rxId = "rx:" + msgid;
        if (!hasReactions) {
            view->removeLine(rxId);
            return;
        }
        const ChatLine rxLine = ChatRenderer::buildReactionLine(*rxIt, msgid);
        if (view->findLine(rxId) >= 0)
            view->replaceLine(rxId, rxLine);
        else
            view->insertAfter(msgid, rxLine);
    };

    if (host == m_model->activeHost() &&
        channel.str().toLower() == m_model->activeChannel().str().toLower()) {
        auto *ch = m_model->channel(host, channel);
        if (ch) updateView(m_chatView, ch);
    }

    for (auto *pane : std::as_const(m_panes)) {
        if (pane->key() != paneKey(host, channel)) continue;
        auto *pCh = m_model->channel(host, channel);
        if (pCh) updateView(pane->chatView(), pCh);
    }
}

void MainWindow::onMessageRedacted(ServerId host, BufferId channel, const QString &msgid)
{
    auto makeCtx = [&](Channel *ch) {
        ChatRenderer::Context ctx;
        ctx.coloredNicks = m_config.ui.coloredNicks;
        ctx.nickBrackets = m_config.ui.nickBrackets;
        ctx.emojiPt      = m_config.ui.fontSizes.emoji;
        ctx.chatPt       = m_config.ui.fontSizes.chat;
        ctx.validTheme   = m_theme.valid;
        ctx.themeText    = m_theme.text;
        ctx.selfNickRe   = selfNickReFor(host);
    ctx.highlightRe  = m_highlightRe;
    ctx.showTimestamps = m_config.ui.showTimestamps;
        ctx.channel      = ch;
        return ctx;
    };

    auto updateView = [&](ChatView *view, Channel *ch) {
        if (view->findLine(msgid) < 0) return;
        for (const auto &msg : std::as_const(ch->messages)) {
            if (msg.msgid == msgid) {
                view->replaceLine(msgid, ChatRenderer::formatMessageLine(msg, makeCtx(ch)));
                break;
            }
        }
    };

    if (host == m_model->activeHost() &&
        channel.str().toLower() == m_model->activeChannel().str().toLower()) {
        auto *ch = m_model->channel(host, channel);
        if (ch) updateView(m_chatView, ch);
    }

    for (auto *pane : std::as_const(m_panes)) {
        if (pane->key() != paneKey(host, channel)) continue;
        auto *pCh = m_model->channel(host, channel);
        if (pCh) updateView(pane->chatView(), pCh);
    }
}

void MainWindow::refreshPaneChatView(ChannelPane *pane)
{
    pane->chatView()->clear();
    auto *ch = m_model->channel(pane->host(), pane->channel());
    if (!ch) return;

    ChatRenderer::Context ctx;
    ctx.coloredNicks = m_config.ui.coloredNicks;
    ctx.nickBrackets = m_config.ui.nickBrackets;
    ctx.emojiPt      = m_config.ui.fontSizes.emoji;
    ctx.chatPt       = m_config.ui.fontSizes.chat;
    ctx.validTheme   = m_theme.valid;
    ctx.themeText    = m_theme.text;
    ctx.selfNickRe   = selfNickReFor(pane->host());
    ctx.highlightRe  = m_highlightRe;
    ctx.showTimestamps = m_config.ui.showTimestamps;
    ctx.channel      = ch;

    const QString selfNick = m_model->selfNick(pane->host());

    static const QRegularExpression urlRe(
        R"(https?://[^\s<>"]+)",
        QRegularExpression::CaseInsensitiveOption);

    for (int i = 0; i < ch->messages.size(); ) {
        const auto &msg = ch->messages[i];
        if (ChatRenderer::isCondensable(msg, selfNick)) {
            const QDate day = msg.timestamp.toLocalTime().date();
            int j = i + 1;
            while (j < ch->messages.size()
                   && ChatRenderer::isCondensable(ch->messages[j], selfNick)
                   && ch->messages[j].timestamp.toLocalTime().date() == day)
                ++j;
            QList<Message> group(ch->messages.cbegin() + i, ch->messages.cbegin() + j);
            const QString groupId = QString::number(group.first().timestamp.toMSecsSinceEpoch());
            const bool grpExpanded = m_expandedEventGroups.contains(groupId);
            pane->chatView()->appendLine(
                ChatRenderer::formatEventGroupLine(group, ctx, groupId, grpExpanded));
            i = j;
        } else {
            pane->chatView()->appendLine(ChatRenderer::formatMessageLine(msg, ctx));
            if (!msg.msgid.isEmpty()) {
                auto rxIt = ch->reactions.constFind(msg.msgid);
                if (rxIt != ch->reactions.constEnd() && !rxIt->isEmpty())
                    pane->chatView()->appendLine(ChatRenderer::buildReactionLine(*rxIt, msg.msgid));
            }
            const bool isText = (msg.type == MessageType::Privmsg ||
                                 msg.type == MessageType::Action  ||
                                 msg.type == MessageType::Notice);
            if (isText && !ch->previews.isEmpty()) {
                auto uit = urlRe.globalMatch(msg.text);
                while (uit.hasNext()) {
                    const QString urlStr = QUrl(uit.next().captured(0)).toString();
                    const auto p = ch->previews.constFind(urlStr);
                    if (p == ch->previews.constEnd() || ch->hiddenPreviews.contains(urlStr))
                        continue;
                    pane->chatView()->appendLine(ChatRenderer::buildPreviewCardLine(
                        urlStr, p->title, p->domain, p->pngData));
                }
            }
            ++i;
        }
    }

    pane->chatView()->scrollToBottom();
}

void MainWindow::refreshChatView(ServerId host, BufferId channel, bool resetToLatest)
{
    m_chatView->clear();
    auto *ch = m_model->channel(host, channel);
    if (!ch) return;

    const QString   key   = host.str() + '\t' + channel.str();
    const qsizetype total = ch->messages.size();

    if (resetToLatest || !m_renderStart.contains(key))
        m_renderStart[key] = static_cast<int>(qMax(qsizetype(0), total - kRenderWindow));
    const int startIdx = m_renderStart.value(key, 0);

    static const QRegularExpression urlRe(
        R"(https?://[^\s<>"]+)",
        QRegularExpression::CaseInsensitiveOption);

    ChatRenderer::Context ctx;
    ctx.coloredNicks = m_config.ui.coloredNicks;
    ctx.nickBrackets = m_config.ui.nickBrackets;
    ctx.emojiPt      = m_config.ui.fontSizes.emoji;
    ctx.chatPt       = m_config.ui.fontSizes.chat;
    ctx.validTheme   = m_theme.valid;
    ctx.themeText    = m_theme.text;
    ctx.selfNickRe   = m_selfNickRe;
    ctx.highlightRe  = m_highlightRe;
    ctx.showTimestamps = m_config.ui.showTimestamps;
    ctx.channel      = ch;

    if (startIdx > 0) {
        ChatLine status = ChatRenderer::makeStatusLine(
            QString("── %1 older messages ──").arg(startIdx), m_theme.separator);
        status.id = "status:older";
        m_chatView->appendLine(status);
    }

    const QString selfNick   = m_model->selfNick(host);
    const int     firstUnread = ch->firstUnreadIdx;
    bool          sepInserted = false;

    for (int i = startIdx; i < ch->messages.size(); ) {
        // Insert "── N new messages ──" separator before first unread message
        if (!sepInserted && firstUnread >= startIdx && i == firstUnread) {
            const int n = ch->unread;
            ChatLine sep = ChatRenderer::makeStatusLine(
                QString("── %1 new message%2 ──").arg(n).arg(n == 1 ? "" : "s"), m_theme.separator);
            sep.id = QStringLiteral("sep:unread");
            m_chatView->appendLine(sep);
            sepInserted = true;
        }

        const auto &msg = ch->messages[i];
        if (ChatRenderer::isCondensable(msg, selfNick)) {
            const QDate day = msg.timestamp.toLocalTime().date();
            int j = i + 1;
            while (j < ch->messages.size()
                   && ChatRenderer::isCondensable(ch->messages[j], selfNick)
                   && ch->messages[j].timestamp.toLocalTime().date() == day)
                ++j;
            QList<Message> group(ch->messages.cbegin() + i, ch->messages.cbegin() + j);
            const QString groupId = QString::number(group.first().timestamp.toMSecsSinceEpoch());
            const bool grpExpanded = m_expandedEventGroups.contains(groupId);
            m_chatView->appendLine(ChatRenderer::formatEventGroupLine(group, ctx, groupId, grpExpanded));
            i = j;
        } else {
            m_chatView->appendLine(ChatRenderer::formatMessageLine(msg, ctx));
            if (!msg.msgid.isEmpty()) {
                auto rxIt = ch->reactions.constFind(msg.msgid);
                if (rxIt != ch->reactions.constEnd() && !rxIt->isEmpty())
                    m_chatView->appendLine(ChatRenderer::buildReactionLine(*rxIt, msg.msgid));
            }
            const bool isText = (msg.type == MessageType::Privmsg ||
                                 msg.type == MessageType::Action  ||
                                 msg.type == MessageType::Notice);
            if (isText && !ch->previews.isEmpty()) {
                auto it = urlRe.globalMatch(msg.text);
                while (it.hasNext()) {
                    const QString urlStr = QUrl(it.next().captured(0)).toString();
                    const auto p = ch->previews.constFind(urlStr);
                    if (p == ch->previews.constEnd() || ch->hiddenPreviews.contains(urlStr))
                        continue;
                    m_chatView->appendLine(ChatRenderer::buildPreviewCardLine(
                        urlStr, p->title, p->domain, p->pngData));
                }
            }
            ++i;
        }
    }

    if (resetToLatest) {
        QTimer::singleShot(0, this, [this, key, firstUnread, startIdx] {
            // Scroll to unread separator when present in the render window
            if (firstUnread >= startIdx) {
                const int li = m_chatView->findLine(QStringLiteral("sep:unread"));
                if (li >= 0) { m_chatView->scrollToLine(li); return; }
            }
            // Restore saved scroll position if user was reading history and nothing new arrived
            const int saved = m_scrollPositions.take(key);
            if (saved > 0)
                m_chatView->verticalScrollBar()->setValue(saved);
            else
                m_chatView->scrollToBottom();
        });
    }

#if defined(__linux__) && !defined(__MUSL__)
    malloc_trim(0);
#endif
}

void MainWindow::loadOlderMessages()
{
    if (m_loadingOlder) return;
    const ServerId host    = m_model->activeHost();
    const BufferId chName  = m_model->activeChannel();
    if (host.isEmpty() || chName.isEmpty()) return;

    const QString key = host.str() + '\t' + chName.str();
    if (!m_renderStart.contains(key) || m_renderStart[key] == 0) {
        if (m_historyExhausted.contains(key)) return;
        m_loadingOlder = true;
        m_model->requestOlderHistory(host, chName);
        return;
    }

    auto *ch = m_model->channel(host, chName);
    if (!ch) return;

    m_loadingOlder = true;

    const int prevStart = m_renderStart[key];
    m_renderStart[key]  = qMax(0, prevStart - kRenderChunk);
    const int newStart  = m_renderStart[key];

    ChatRenderer::Context ctx;
    ctx.coloredNicks = m_config.ui.coloredNicks;
    ctx.nickBrackets = m_config.ui.nickBrackets;
    ctx.emojiPt      = m_config.ui.fontSizes.emoji;
    ctx.chatPt       = m_config.ui.fontSizes.chat;
    ctx.validTheme   = m_theme.valid;
    ctx.themeText    = m_theme.text;
    ctx.selfNickRe   = m_selfNickRe;
    ctx.highlightRe  = m_highlightRe;
    ctx.showTimestamps = m_config.ui.showTimestamps;
    ctx.channel      = ch;

    const QString selfNick = m_model->selfNick(host);
    QList<ChatLine> older;

    if (newStart > 0) {
        ChatLine status = ChatRenderer::makeStatusLine(
            QString("── %1 older messages ──").arg(newStart), m_theme.separator);
        status.id = "status:older";
        older.append(status);
    }

    for (int i = newStart; i < prevStart; ) {
        const auto &msg = ch->messages[i];
        if (ChatRenderer::isCondensable(msg, selfNick)) {
            const QDate day = msg.timestamp.toLocalTime().date();
            int j = i + 1;
            while (j < prevStart
                   && ChatRenderer::isCondensable(ch->messages[j], selfNick)
                   && ch->messages[j].timestamp.toLocalTime().date() == day)
                ++j;
            QList<Message> group(ch->messages.cbegin() + i, ch->messages.cbegin() + j);
            const QString groupId    = QString::number(group.first().timestamp.toMSecsSinceEpoch());
            const bool    grpExpanded = m_expandedEventGroups.contains(groupId);
            older.append(ChatRenderer::formatEventGroupLine(group, ctx, groupId, grpExpanded));
            i = j;
        } else {
            older.append(ChatRenderer::formatMessageLine(msg, ctx));
            if (!msg.msgid.isEmpty()) {
                auto rxIt = ch->reactions.constFind(msg.msgid);
                if (rxIt != ch->reactions.constEnd() && !rxIt->isEmpty())
                    older.append(ChatRenderer::buildReactionLine(*rxIt, msg.msgid));
            }
            ++i;
        }
    }

    // Remove the existing "older messages" sentinel before prepending new batch
    m_chatView->removeLine("status:older");
    m_chatView->prependLines(std::move(older));

    QTimer::singleShot(0, this, [this]{ m_loadingOlder = false; });
}

void MainWindow::onOlderHistoryLoaded(ServerId host, BufferId channel, int count)
{
    m_loadingOlder = false;

    // Resume an in-flight search-result jump once this batch is rendered.
    // Deferred so the prepend below runs first; continueHistoryJump()
    // cancels itself if the buffer is no longer active.
    if (m_jumpTs.isValid() && host == m_jumpHost && channel == m_jumpChannel)
        QTimer::singleShot(0, this, &MainWindow::continueHistoryJump);

    if (host != m_model->activeHost() || channel != m_model->activeChannel())
        return;

    const QString key = host.str() + '\t' + channel.str();
    if (count <= 0) {
        m_historyExhausted.insert(key);
        return;
    }

    auto *ch = m_model->channel(host, channel);
    if (!ch) return;

    m_renderStart[key] = 0;

    ChatRenderer::Context ctx;
    ctx.coloredNicks   = m_config.ui.coloredNicks;
    ctx.nickBrackets   = m_config.ui.nickBrackets;
    ctx.emojiPt        = m_config.ui.fontSizes.emoji;
    ctx.chatPt         = m_config.ui.fontSizes.chat;
    ctx.validTheme     = m_theme.valid;
    ctx.themeText      = m_theme.text;
    ctx.selfNickRe     = m_selfNickRe;
    ctx.highlightRe    = m_highlightRe;
    ctx.showTimestamps = m_config.ui.showTimestamps;
    ctx.channel        = ch;

    const QString selfNick = m_model->selfNick(host);
    QList<ChatLine> older;
    const int end = qMin(count, static_cast<int>(ch->messages.size()));

    for (int i = 0; i < end; ) {
        const auto &msg = ch->messages[i];
        if (ChatRenderer::isCondensable(msg, selfNick)) {
            const QDate day = msg.timestamp.toLocalTime().date();
            int j = i + 1;
            while (j < end
                   && ChatRenderer::isCondensable(ch->messages[j], selfNick)
                   && ch->messages[j].timestamp.toLocalTime().date() == day)
                ++j;
            QList<Message> group(ch->messages.cbegin() + i, ch->messages.cbegin() + j);
            const QString groupId    = QString::number(group.first().timestamp.toMSecsSinceEpoch());
            const bool    grpExpanded = m_expandedEventGroups.contains(groupId);
            older.append(ChatRenderer::formatEventGroupLine(group, ctx, groupId, grpExpanded));
            i = j;
        } else {
            older.append(ChatRenderer::formatMessageLine(msg, ctx));
            if (!msg.msgid.isEmpty()) {
                auto rxIt = ch->reactions.constFind(msg.msgid);
                if (rxIt != ch->reactions.constEnd() && !rxIt->isEmpty())
                    older.append(ChatRenderer::buildReactionLine(*rxIt, msg.msgid));
            }
            ++i;
        }
    }

    m_chatView->removeLine("status:older");
    m_chatView->prependLines(std::move(older));
}

void MainWindow::startHistoryJump(ServerId host, BufferId channel, const QDateTime &ts)
{
    if (!ts.isValid()) return;
    m_jumpHost    = host;
    m_jumpChannel = channel;
    m_jumpTs      = ts;
    m_jumpRounds  = 0;
    continueHistoryJump();
}

void MainWindow::continueHistoryJump()
{
    if (!m_jumpTs.isValid()) return;
    const ServerId  host    = m_jumpHost;
    const BufferId  channel = m_jumpChannel;
    // The user moved on to another buffer — drop the jump silently.
    if (host != m_model->activeHost() || channel != m_model->activeChannel()) {
        m_jumpTs = {};
        return;
    }
    auto *ch = m_model->channel(host, channel);
    if (!ch || ch->messages.isEmpty()) {
        m_jumpTs = {};
        return;
    }

    const QString key = host.str() + '\t' + channel.str();

    // Target older than everything in memory → pull another server batch.
    // onOlderHistoryLoaded() calls back here after each one. Bounded so a
    // very old result can't fetch unbounded history; without chathistory
    // the first request marks the buffer exhausted and we land at the
    // oldest message we have.
    static constexpr int kMaxJumpRounds = 10;
    if (ch->messages.first().timestamp.toLocalTime() > m_jumpTs
        && !m_historyExhausted.contains(key)
        && m_jumpRounds < kMaxJumpRounds) {
        ++m_jumpRounds;
        m_model->requestOlderHistory(host, channel);
        return;
    }

    const QDateTime ts = m_jumpTs;
    m_jumpTs = {};

    // First message at/after the log line's timestamp (list is time-ordered).
    const auto &msgs = ch->messages;
    int idx = 0;
    while (idx < msgs.size() - 1 && msgs[idx].timestamp.toLocalTime() < ts)
        ++idx;

    // Render still-lazy in-memory chunks until the target line exists.
    int guard = 0;
    while (m_renderStart.value(key, 0) > idx && ++guard < 1000) {
        m_loadingOlder = false;
        loadOlderMessages();
    }
    m_loadingOlder = false;

    // The exact message may have no view line of its own (grouped events,
    // missing msgid) — walk outward until a nearby message has one.
    int lineIdx = -1;
    for (int off = 0; lineIdx < 0 && off < 50; ++off) {
        for (int sign = 0; sign < 2 && lineIdx < 0; ++sign) {
            const qsizetype i = sign ? idx - off : idx + off;
            if (i < 0 || i >= msgs.size()) continue;
            lineIdx = m_chatView->findLine(msgs[i].msgid);
            if (lineIdx < 0)  // group lines are keyed on their first member
                lineIdx = m_chatView->findLine(
                    "evgrp:" + QString::number(msgs[i].timestamp.toMSecsSinceEpoch()));
        }
    }
    if (lineIdx < 0)
        lineIdx = 0;  // best effort: top of the loaded history

    m_chatView->highlightLine(lineIdx);
    QTimer::singleShot(2500, m_chatView, &ChatView::clearFind);
}

void MainWindow::appendMessage(const Message &msg, bool autoPreview)
{
    const ServerId host    = m_model->activeHost();
    const BufferId channel = m_model->activeChannel();
    auto *ch = m_model->channel(m_model->activeHost(), m_model->activeChannel());

    ChatRenderer::Context ctx;
    ctx.coloredNicks = m_config.ui.coloredNicks;
    ctx.nickBrackets = m_config.ui.nickBrackets;
    ctx.emojiPt      = m_config.ui.fontSizes.emoji;
    ctx.chatPt       = m_config.ui.fontSizes.chat;
    ctx.validTheme   = m_theme.valid;
    ctx.themeText    = m_theme.text;
    ctx.selfNickRe   = m_selfNickRe;
    ctx.highlightRe  = m_highlightRe;
    ctx.showTimestamps = m_config.ui.showTimestamps;
    ctx.channel      = ch;

    m_chatView->appendLine(ChatRenderer::formatMessageLine(msg, ctx));

    if (autoPreview)
        appendPreviewCards(m_chatView, msg, host, channel);

    if (m_chatView->isAtBottom()) m_chatView->scrollToBottom();
}

// Appends cached preview cards for URLs in msg to the given view, or queues
// a fetch for new ones. Shared by the primary view and pane appends; the
// PreviewController dedupes by URL, so both enqueueing for the same message
// is safe when a channel is visible in two places.
void MainWindow::appendPreviewCards(ChatView *view, const Message &msg,
                                    const ServerId &host, const BufferId &channel)
{
    const bool isText = (msg.type == MessageType::Privmsg ||
                         msg.type == MessageType::Action  ||
                         msg.type == MessageType::Notice);
    if (!isText) return;
    auto *ch = m_model->channel(host, channel);

    static const QRegularExpression urlRe(
        R"(https?://[^ \t\r\n<>"]+)",
        QRegularExpression::CaseInsensitiveOption);
    auto it = urlRe.globalMatch(msg.text);
    while (it.hasNext()) {
        const QString urlStr = QUrl(it.next().captured(0)).toString();
        if (urlStr.isEmpty()) continue;
        if (ch) {
            const auto p = ch->previews.constFind(urlStr);
            if (p != ch->previews.constEnd() && !ch->hiddenPreviews.contains(urlStr)) {
                view->appendLine(ChatRenderer::buildPreviewCardLine(
                    urlStr, p->title, p->domain, p->pngData));
                continue;
            }
        }
        m_previews->enqueue(QUrl(urlStr), host, channel, msg.msgid);
    }
}

QString MainWindow::formatMessage(const Message &msg) const
{
    ChatRenderer::Context ctx;
    ctx.coloredNicks = m_config.ui.coloredNicks;
    ctx.nickBrackets = m_config.ui.nickBrackets;
    ctx.emojiPt      = m_config.ui.fontSizes.emoji;
    ctx.chatPt       = m_config.ui.fontSizes.chat;
    ctx.validTheme   = m_theme.valid;
    ctx.themeText    = m_theme.text;
    ctx.selfNickRe   = m_selfNickRe;
    ctx.highlightRe  = m_highlightRe;
    ctx.showTimestamps = m_config.ui.showTimestamps;
    ctx.channel      = m_model->channel(m_model->activeHost(), m_model->activeChannel());
    return ChatRenderer::formatMessage(msg, ctx);
}
