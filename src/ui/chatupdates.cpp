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
        ctx.selfNickRe   = m_selfNickRe;
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
        if (pCh) appendToView(pane->chatView(), pCh);
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
        ctx.selfNickRe   = m_selfNickRe;
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
    ctx.selfNickRe   = m_selfNickRe;
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
                    ChatLine card;
                    card.role   = ChatLineRole::PreviewCard;
                    card.id     = "preview:" + urlStr;
                    if (!p->pngData.isEmpty()) card.image.loadFromData(p->pngData, "PNG");
                    card.text   = p->title + "\n" + p->domain;
                    ChatSegment seg; seg.start = 0; seg.length = static_cast<int>(card.text.size());
                    seg.anchor = "preview:" + urlStr;
                    card.segments.append(seg);
                    pane->chatView()->appendLine(card);
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
                    ChatLine card;
                    card.role   = ChatLineRole::PreviewCard;
                    card.id     = "preview:" + urlStr;
                    if (!p->pngData.isEmpty()) card.image.loadFromData(p->pngData, "PNG");
                    card.text   = p->title + "\n" + p->domain;
                    ChatSegment seg; seg.start = 0; seg.length = static_cast<int>(card.text.size());
                    seg.anchor = "preview:" + urlStr;
                    card.segments.append(seg);
                    m_chatView->appendLine(card);
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

    const bool isText = (msg.type == MessageType::Privmsg ||
                         msg.type == MessageType::Action  ||
                         msg.type == MessageType::Notice);
    if (autoPreview && isText) {
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
                    ChatLine card;
                    card.role   = ChatLineRole::PreviewCard;
                    card.id     = "preview:" + urlStr;
                    if (!p->pngData.isEmpty()) card.image.loadFromData(p->pngData, "PNG");
                    card.text   = p->title + "\n" + p->domain;
                    ChatSegment seg; seg.start = 0; seg.length = static_cast<int>(card.text.size());
                    seg.anchor = "preview:" + urlStr;
                    card.segments.append(seg);
                    m_chatView->appendLine(card);
                    continue;
                }
            }
            m_previews->enqueue(QUrl(urlStr), host, channel, msg.msgid);
        }
    }

    if (m_chatView->isAtBottom()) m_chatView->scrollToBottom();
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
