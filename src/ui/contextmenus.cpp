#include "mainwindow.h"
#include "ui/chatrenderer.h"
#include "ui/chatview.h"
#include "ui/emojipicker.h"
#include "ui/menuicons.h"
#include "ui/channelpane.h"
#include "irc/ircclient.h"
#include "ui/dcccontroller.h"
#include "model/sessionmodel.h"
#include "config/config.h"

#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDesktopServices>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QTimer>
#include <QTreeWidget>

void MainWindow::toggleEventGroupInView(ChatView *view, const QString &groupId,
                                         ServerId host, BufferId channel)
{
    auto *ch = m_model->channel(host, channel);
    if (!ch) return;

    const qint64 targetMs = groupId.toLongLong();
    const QString selfNick = m_model->selfNick(host);
    int start = -1;
    for (int i = 0; i < ch->messages.size(); ++i) {
        if (ch->messages[i].timestamp.toMSecsSinceEpoch() == targetMs
            && ChatRenderer::isCondensable(ch->messages[i], selfNick)) {
            start = i;
            break;
        }
    }
    if (start < 0) return;

    const QDate day = ch->messages[start].timestamp.toLocalTime().date();
    int j = start + 1;
    while (j < ch->messages.size()
           && ChatRenderer::isCondensable(ch->messages[j], selfNick)
           && ch->messages[j].timestamp.toLocalTime().date() == day)
        ++j;

    QList<Message> group(ch->messages.cbegin() + start, ch->messages.cbegin() + j);

    const bool expand = !m_expandedEventGroups.contains(groupId);
    if (expand) {
        if (m_expandedEventGroups.size() >= 200)
            m_expandedEventGroups.clear();
        m_expandedEventGroups.insert(groupId);
    } else {
        m_expandedEventGroups.remove(groupId);
    }

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

    const bool atBottom = view->isAtBottom();
    view->replaceLine("evgrp:" + groupId,
                      ChatRenderer::formatEventGroupLine(group, ctx, groupId, expand));
    if (atBottom) view->scrollToBottom();
}

void MainWindow::handleChatViewContextMenu(ChatView *view, const QString &anchor,
                                            const QPoint &globalPos,
                                            ServerId host, BufferId channel)
{
    if (anchor.startsWith("nick:")) {
        showNickContextMenu(anchor.mid(5), globalPos);
        return;
    }

    if (anchor.startsWith("msgid:")) {
        const QString msgid = anchor.mid(6);
        QMenu menu(view->viewport());
        auto *cl = m_model->clientFor(host);
        const QString sel = view->selectedText();
        if (!sel.isEmpty()) {
            connect(menu.addAction("Copy"), &QAction::triggered, this,
                    [sel]{ QApplication::clipboard()->setText(sel); });
            menu.addSeparator();
        }
        connect(menu.addAction("Reply"), &QAction::triggered, this,
                [this, msgid, host, channel]{
            auto *ch = m_model->channel(host, channel);
            QString origNick;
            if (ch) for (const auto &m : std::as_const(ch->messages))
                if (m.msgid == msgid) { origNick = m.nick; break; }
            m_pendingReplyMsgid = msgid;
            if (m_replyLabel) m_replyLabel->setText("↩ " + (origNick.isEmpty() ? msgid : origNick));
            if (m_replyBar) m_replyBar->show();
            if (m_input)    m_input->setFocus();
            updateLengthIndicator();
        });
        if (cl && cl->hasCap("message-tags")) {
            connect(menu.addAction("React"), &QAction::triggered, this,
                    [this, msgid, host, channel, globalPos]{
                m_pendingReactMsgid    = msgid;
                m_pendingReactHost     = host.str();
                m_pendingReactChannel  = channel.str();
                ensureEmojiPicker();
                m_emojiPicker->showAt(globalPos);
            });
        }
        {
            auto *ch = m_model->channel(host, channel);
            if (cl && cl->hasCap("draft/message-redaction") && ch) {
                QString msgNick;
                for (const auto &m : std::as_const(ch->messages))
                    if (m.msgid == msgid) { msgNick = m.nick; break; }
                if (msgNick == m_model->selfNick(host)) {
                    connect(menu.addAction("Delete"), &QAction::triggered, this,
                            [this, msgid, host, channel]{
                        m_model->sendRedact(host, channel, msgid);
                    });
                }
            }
        }
        menu.exec(globalPos);
        return;
    }

    if (anchor.startsWith("preview:")) {
        const QString url = anchor.mid(8);
        QMenu menu(view->viewport());
        connect(menu.addAction("Open URL"), &QAction::triggered, this, [url]{
            const QUrl u(url);
            if (u.scheme().toLower() == "http" || u.scheme().toLower() == "https")
                QDesktopServices::openUrl(u);
        });
        auto *ch = m_model->channel(host, channel);
        if (ch && ch->previews.contains(url)) {
            connect(menu.addAction("Hide Preview"), &QAction::triggered, this,
                    [this, url, host, channel]{
                auto *inner = m_model->channel(host, channel);
                if (inner) inner->hiddenPreviews.insert(url);
                refreshChatView(host, channel);
            });
        }
        menu.exec(globalPos);
        return;
    }

    if (!anchor.isEmpty()) {
        // URL or other anchor
        QString href = anchor;
        if (href.startsWith("url:")) href = href.mid(4);
        QMenu menu(view->viewport());
        connect(menu.addAction("Copy URL"), &QAction::triggered,
                this, [href]{ QApplication::clipboard()->setText(href); });
        connect(menu.addAction("Open URL"), &QAction::triggered, this, [href]{
            const QUrl u(href);
            if (u.scheme().toLower() == "http" || u.scheme().toLower() == "https")
                QDesktopServices::openUrl(u);
        });
        auto *ch = m_model->channel(host, channel);
        const bool isHidden   = ch && ch->hiddenPreviews.contains(href);
        const bool hasPreview = ch && ch->previews.contains(href);
        if (isHidden) {
            connect(menu.addAction("Show Preview"), &QAction::triggered, this,
                    [this, href, host, channel]{
                auto *inner = m_model->channel(host, channel);
                if (inner) inner->hiddenPreviews.remove(href);
                refreshChatView(host, channel);
            });
        } else {
            auto *hideAction = menu.addAction("Hide Preview");
            hideAction->setEnabled(hasPreview);
            connect(hideAction, &QAction::triggered, this, [this, href, host, channel]{
                auto *inner = m_model->channel(host, channel);
                if (inner) inner->hiddenPreviews.insert(href);
                refreshChatView(host, channel);
            });
        }
        menu.exec(globalPos);
    }
}

void MainWindow::onSidebarContextMenu(const QPoint &pos)
{
    auto *item = m_sidebar->itemAt(pos);
    if (!item) return;

    const ServerId host{item->data(0, Qt::UserRole).toString()};
    const BufferId channel{item->data(0, Qt::UserRole + 1).toString()};

    // Heap-allocate and use popup() instead of exec() to avoid a Qt/Wayland
    // issue where the pending button-release from the triggering click is
    // delivered into exec()'s event loop, immediately selecting the first item.
    auto *menu = new QMenu(this);
    connect(menu, &QMenu::aboutToHide, menu, &QObject::deleteLater);

    if (channel.str() == "(server)") {
        auto *sess = m_model->session(host);
        if (sess && sess->connected) {
            menu->addAction("Disconnect", this, [this, host]{
                if (auto *cl = m_model->clientFor(host))
                    cl->quit();
            });
        } else {
            menu->addAction("Reconnect", this, [this, host]{
                if (auto *cl = m_model->clientFor(host))
                    cl->reconnect();
            });
        }
        menu->addAction("Close Server", this, [this, host]{
            m_model->closeServer(host);
        });
        menu->addSeparator();
        const int idx = m_sidebar->indexOfTopLevelItem(item);
        if (idx > 0) {
            menu->addAction("Move Up", this, [this, idx]{
                auto *moved = m_sidebar->takeTopLevelItem(idx);
                m_sidebar->insertTopLevelItem(idx - 1, moved);
                moved->setExpanded(true);
                m_sidebar->setCurrentItem(moved);
                syncSidebarOrderToConfig();
            });
        }
        if (idx < m_sidebar->topLevelItemCount() - 1) {
            menu->addAction("Move Down", this, [this, idx]{
                auto *moved = m_sidebar->takeTopLevelItem(idx);
                m_sidebar->insertTopLevelItem(idx + 1, moved);
                moved->setExpanded(true);
                m_sidebar->setCurrentItem(moved);
                syncSidebarOrderToConfig();
            });
        }
    } else if (isChannelName(channel.str())) {
        const QString key = paneKey(host, channel);
        if (!m_panes.contains(key)) {
            menu->addAction("Open in Pane", this, [this, host, channel]{
                openChannelPane(host, channel);
            });
            menu->addAction("Open in Window", this, [this, host, channel]{
                popOutChannel(host, channel);
            });
        } else if (m_paneWindows.contains(key)) {
            menu->addAction("Close Window", this, [this, host, channel]{
                closeChannelPane(host, channel);
            });
        } else {
            menu->addAction("Close Pane", this, [this, host, channel]{
                closeChannelPane(host, channel);
            });
        }
        menu->addSeparator();
        menu->addAction("Rejoin", this, [this, host, channel]{
            m_model->sendPart(host, channel);
            QTimer::singleShot(500, this, [this, host, channel]{
                m_model->sendJoin(host, channel);
            });
        });
        menu->addAction("Leave", this, [this, host, channel]{
            m_model->sendPart(host, channel);
        });
        menu->addAction("Close", this, [this, host, channel]{
            m_model->closeBuffer(host, channel);
        });
        menu->addSeparator();
        addNotificationsMenu(menu, host, channel);
        auto *srvItem = item->parent();
        if (srvItem) {
            const int cidx = srvItem->indexOfChild(item);
            menu->addSeparator();
            if (cidx > 0) {
                menu->addAction("Move Up", this, [this, host, srvItem, item, cidx]{
                    srvItem->takeChild(cidx);
                    srvItem->insertChild(cidx - 1, item);
                    m_sidebar->setCurrentItem(item);
                    syncChannelOrderToConfig(host);
                });
            }
            if (cidx < srvItem->childCount() - 1) {
                menu->addAction("Move Down", this, [this, host, srvItem, item, cidx]{
                    srvItem->takeChild(cidx);
                    srvItem->insertChild(cidx + 1, item);
                    m_sidebar->setCurrentItem(item);
                    syncChannelOrderToConfig(host);
                });
            }
        }
    } else if (!channel.isEmpty() && channel.str() != "(server)") {
        // PM / user query
        addNotificationsMenu(menu, host, channel);
        menu->addAction("Close Query", this, [this, host, channel]{
            m_model->closeBuffer(host, channel);
        });
    }

    if (!menu->actions().isEmpty())
        menu->popup(m_sidebar->viewport()->mapToGlobal(pos));
    else
        menu->deleteLater();
}

void MainWindow::addNotificationsMenu(QMenu *menu, ServerId host, BufferId channel)
{
    QMenu *sub = menu->addMenu("Notifications");
    auto *group = new QActionGroup(sub);
    group->setExclusive(true);
    const NotifyLevel current = m_model->notifyLevel(host, channel);

    const auto addLevel = [&](const QString &label, NotifyLevel level,
                              const QString &feedback){
        QAction *a = sub->addAction(label);
        a->setCheckable(true);
        a->setChecked(current == level);
        group->addAction(a);
        connect(a, &QAction::triggered, this, [this, host, channel, level, feedback]{
            m_model->setNotifyLevel(host, channel, level);
            m_config.notifyLevels.removeIf([&](const NotifyOverride &no){
                return no.server == host.str()
                    && no.buffer.compare(channel.str(), Qt::CaseInsensitive) == 0;
            });
            if (level != NotifyLevel::Mentions)
                m_config.notifyLevels.append({host.str(), channel.str(), level});
            saveConfig();
            m_model->localMessage(host, channel, feedback);
        });
    };

    const QString name = channel.str();
    addLevel("Everything",    NotifyLevel::All,
             "Notifications for " + name + ": everything");
    addLevel("Mentions Only", NotifyLevel::Mentions,
             "Notifications for " + name + ": mentions only (default)");
    addLevel("Mute",          NotifyLevel::Mute,
             "Notifications for " + name + ": muted");
}

void MainWindow::onNickListContextMenu(const QPoint &pos)
{
    const QModelIndex idx = m_nickList->indexAt(pos);
    if (!idx.isValid()) return;
    const QString nick = idx.data(Qt::UserRole).toString();
    if (nick.isEmpty()) return;
    showNickContextMenu(nick, m_nickList->mapToGlobal(pos));
}

void MainWindow::showNickContextMenu(const QString &nick, const QPoint &globalPos)
{
    const ServerId host    = m_model->activeHost();
    const BufferId channel = m_model->activeChannel();
    if (nick.isEmpty() || host.isEmpty()) return;

    QMenu menu(this);

    QAction *title = menu.addAction(nick);
    title->setEnabled(false);
    QFont tf = title->font();
    tf.setBold(true);
    title->setFont(tf);
    menu.addSeparator();

    // ── Common actions ────────────────────────────────────────────────────────
    connect(menu.addAction("Message"), &QAction::triggered, this, [this, host, nick]{
        m_model->openPM(host, nick);
        switchToChannel(host, BufferId{nick});
        if (m_input) m_input->setFocus();
    });

    connect(menu.addAction("Whois"), &QAction::triggered, this, [this, host, nick]{
        m_model->sendRaw(host, "WHOIS " + nick);
    });

    connect(menu.addAction("Copy Nick"), &QAction::triggered, this, [nick]{
        qApp->clipboard()->setText(nick);
    });

    menu.addSeparator();

    // ── Ignore ▶ ─────────────────────────────────────────────────────────────
    {
        const QString key = nick.toLower();
        const IgnoreTypes curFlags = m_model->ignoreFlags(key);

        auto *ignoreSub = new QMenu("Ignore", &menu);
        ignoreSub->setIcon(MenuIcons::eyeOff());

        auto makeTypeAction = [&](const QString &label, IgnoreType type) {
            auto *act = ignoreSub->addAction(label);
            act->setCheckable(true);
            act->setChecked(bool(curFlags & type));
            connect(act, &QAction::triggered, this, [this, host, key, type](bool checked) {
                IgnoreTypes flags = m_model->ignoreFlags(key);
                if (checked) flags |= type;
                else         flags &= ~IgnoreTypes(type);
                m_config.ignoreList.removeIf([&](const IgnoreEntry &e){ return e.nick == key; });
                if (flags) {
                    m_model->setIgnore(key, flags);
                    m_config.ignoreList.append({key, flags});
                } else {
                    m_model->clearIgnore(key);
                    m_model->localMessage(host, m_model->activeChannel(),
                                          "No longer ignoring " + key);
                }
                saveConfig();
                scheduleNickRefresh(host, m_model->activeChannel());
            });
        };

        makeTypeAction("Private Messages", IgnoreType::PM);
        makeTypeAction("Notices",          IgnoreType::Notice);
        makeTypeAction("Invites",          IgnoreType::Invite);

        if (curFlags) {
            ignoreSub->addSeparator();
            connect(ignoreSub->addAction(MenuIcons::close(), "Unignore All"),
                    &QAction::triggered, this, [this, host, key] {
                m_model->clearIgnore(key);
                m_config.ignoreList.removeIf([&](const IgnoreEntry &e){ return e.nick == key; });
                saveConfig();
                m_model->localMessage(host, m_model->activeChannel(),
                                      "No longer ignoring " + key);
                scheduleNickRefresh(host, m_model->activeChannel());
            });
        }

        menu.addMenu(ignoreSub);
    }

    menu.addSeparator();

    // ── CTCP ▶ ───────────────────────────────────────────────────────────────
    {
        auto *ctcpSub = new QMenu("CTCP", &menu);

        connect(ctcpSub->addAction("Ping"), &QAction::triggered, this, [this, host, nick]{
            const qint64 ts = QDateTime::currentMSecsSinceEpoch();
            m_model->sendRaw(host, "PRIVMSG " + nick + " :\x01PING " + QString::number(ts) + "\x01");
        });

        connect(ctcpSub->addAction("Time"), &QAction::triggered, this, [this, host, nick]{
            m_model->sendRaw(host, "PRIVMSG " + nick + " :\x01TIME\x01");
        });

        connect(ctcpSub->addAction("Version"), &QAction::triggered, this, [this, host, nick]{
            m_model->sendRaw(host, "PRIVMSG " + nick + " :\x01VERSION\x01");
        });

        menu.addMenu(ctcpSub);
    }

    // ── DCC ▶ ────────────────────────────────────────────────────────────────
    {
        auto *dccSub = new QMenu("DCC", &menu);

        connect(dccSub->addAction("Send File"), &QAction::triggered, this, [this, host, nick]{
            m_dcc->sendFile(host, nick);
        });

        connect(dccSub->addAction("Send File (Passive)"), &QAction::triggered, this, [this, host, nick]{
            m_dcc->sendFilePassive(host, nick);
        });

        menu.addMenu(dccSub);
    }

    menu.addSeparator();

    // ── Chan Ops ▶ ───────────────────────────────────────────────────────────
    {
        auto *opSub = new QMenu("Chan Ops", &menu);

        connect(opSub->addAction("Give Op"), &QAction::triggered, this, [this, host, channel, nick]{
            if (!channel.isEmpty() && channel.str() != "(server)")
                m_model->sendRaw(host, "MODE " + channel.str() + " +o " + nick);
        });
        connect(opSub->addAction("Take Op"), &QAction::triggered, this, [this, host, channel, nick]{
            if (!channel.isEmpty() && channel.str() != "(server)")
                m_model->sendRaw(host, "MODE " + channel.str() + " -o " + nick);
        });
        connect(opSub->addAction("Give Voice"), &QAction::triggered, this, [this, host, channel, nick]{
            if (!channel.isEmpty() && channel.str() != "(server)")
                m_model->sendRaw(host, "MODE " + channel.str() + " +v " + nick);
        });
        connect(opSub->addAction("Take Voice"), &QAction::triggered, this, [this, host, channel, nick]{
            if (!channel.isEmpty() && channel.str() != "(server)")
                m_model->sendRaw(host, "MODE " + channel.str() + " -v " + nick);
        });

        opSub->addSeparator();

        connect(opSub->addAction("Invite"), &QAction::triggered, this, [this, host, channel, nick]{
            bool ok;
            const QString target = QInputDialog::getText(
                this, "Invite " + nick, "Channel:", QLineEdit::Normal,
                (channel.isEmpty() || channel.str() == "(server)") ? QString() : channel.str(), &ok);
            if (!ok || target.isEmpty()) return;
            m_model->sendRaw(host, "INVITE " + nick + " " + target);
        });
        connect(opSub->addAction("Kick"), &QAction::triggered, this, [this, host, channel, nick]{
            if (channel.isEmpty() || channel.str() == "(server)") return;
            bool ok;
            QString reason = QInputDialog::getText(this, "Kick " + nick, "Reason:", QLineEdit::Normal, {}, &ok);
            if (!ok) return;
            m_model->sendRaw(host, "KICK " + channel.str() + " " + nick + (reason.isEmpty() ? QString() : " :" + reason));
        });
        connect(opSub->addAction("Ban"), &QAction::triggered, this, [this, host, channel, nick]{
            if (!channel.isEmpty() && channel.str() != "(server)")
                m_model->sendRaw(host, "MODE " + channel.str() + " +b " + nick + "!*@*");
        });
        connect(opSub->addAction("Kick && Ban"), &QAction::triggered, this, [this, host, channel, nick]{
            if (channel.isEmpty() || channel.str() == "(server)") return;
            bool ok;
            QString reason = QInputDialog::getText(this, "Kick & Ban " + nick, "Reason:", QLineEdit::Normal, {}, &ok);
            if (!ok) return;
            m_model->sendRaw(host, "MODE " + channel.str() + " +b " + nick + "!*@*");
            m_model->sendRaw(host, "KICK " + channel.str() + " " + nick + (reason.isEmpty() ? QString() : " :" + reason));
        });

        menu.addMenu(opSub);
    }

    menu.exec(globalPos);
}
