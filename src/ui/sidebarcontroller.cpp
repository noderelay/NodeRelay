#include "sidebarcontroller.h"

#include "config/config.h"
#include "model/sessionmodel.h"
#include "ui/fadescrollbar.h"
#include "ui/mainwindowdelegates.h"
#include "ui/menuicons.h"
#include "ui/themeloader.h"

#include <QScroller>
#include <QTreeWidget>

SidebarController::SidebarController(SessionModel *model, const Config &config,
                                     const Theme &theme, QObject *parent)
    : QObject(parent), m_model(model), m_config(config), m_theme(theme)
{
    m_tree = new QTreeWidget;
    m_tree->setVerticalScrollBar(new FadeScrollBar(Qt::Vertical, m_tree));
    m_tree->setHeaderHidden(true);
    m_tree->setRootIsDecorated(false);
    m_tree->setItemsExpandable(false);
    m_tree->setIndentation(8);
    m_tree->setMinimumWidth(112);
    m_tree->setObjectName("sidebar");
    m_delegate = new SidebarDelegate(m_tree);
    m_delegate->setShowCounts(m_config.ui.showUnreadCounts);
    if (m_theme.valid)
        m_delegate->setColors(QColor(m_theme.accent),
                              QColor(m_theme.border),
                              QColor(m_theme.text),
                              QColor(m_theme.sidebarUnread));
    m_tree->setItemDelegate(m_delegate);

    connect(m_model, &SessionModel::unreadChanged,
            this, &SidebarController::updateUnread);
}

QTreeWidgetItem *SidebarController::serverItem(const ServerId &host) const
{
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
        auto *item = m_tree->topLevelItem(i);
        if (item->data(0, Qt::UserRole).toString() == host.str())
            return item;
    }
    return nullptr;
}

QTreeWidgetItem *SidebarController::channelItem(const ServerId &host, const BufferId &channel) const
{
    auto *srv = serverItem(host);
    if (!srv) return nullptr;
    if (channel.str() == "(server)") return srv;
    for (int i = 0; i < srv->childCount(); ++i) {
        auto *item = srv->child(i);
        if (item->data(0, Qt::UserRole + 1).toString().toLower() == channel.str().toLower())
            return item;
    }
    return nullptr;
}

static QString shortNetworkName(const QString &host)
{
    QString h = host;
    if (h.startsWith("irc.", Qt::CaseInsensitive))
        h = h.mid(4);
    const auto dot = h.lastIndexOf('.');
    if (dot > 0)
        h = h.left(dot);
    return h;
}

QString SidebarController::serverLabel(const ServerId &host) const
{
    for (const auto &sc : std::as_const(m_config.servers))
        if (sc.name == host.str() && !sc.name.isEmpty())
            return sc.name;
    return shortNetworkName(host.str());
}

QTreeWidgetItem *SidebarController::addServerItem(const ServerId &host)
{
    auto *item = new QTreeWidgetItem(m_tree);
    item->setText(0, serverLabel(host).toUpper());
    item->setData(0, Qt::UserRole,     host.str());
    item->setData(0, Qt::UserRole + 1, QString("(server)"));
    item->setExpanded(true);
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    QFont f(m_config.ui.fontFamily);
    f.setPointSizeF(m_config.ui.fontSizes.serverHeader);
    f.setBold(true);
    item->setFont(0, f);
    item->setForeground(0, QColor("#6c7086"));
    return item;
}

QTreeWidgetItem *SidebarController::addChannelItem(const ServerId &host, const BufferId &channel)
{
    auto *srv = serverItem(host);
    if (!srv) return nullptr;
    auto *item = new QTreeWidgetItem(srv);
    item->setText(0, channel.str());
    item->setData(0, Qt::UserRole,     host.str());
    item->setData(0, Qt::UserRole + 1, channel.str());
    return item;
}

void SidebarController::removeServerItem(const ServerId &host)
{
    if (auto *srv = serverItem(host))
        delete m_tree->takeTopLevelItem(m_tree->indexOfTopLevelItem(srv));
}

void SidebarController::markConnected(const ServerId &host)
{
    if (auto *item = serverItem(host))
        item->setData(0, Qt::UserRole + 2, QVariant::fromValue(MenuIcons::connectedServer()));
}

void SidebarController::clearConnectionIcon(const ServerId &host)
{
    if (auto *item = serverItem(host))
        item->setData(0, Qt::UserRole + 2, QVariant());
}

void SidebarController::setCheckedOut(const ServerId &host, const BufferId &channel, bool out)
{
    auto *item = channelItem(host, channel);
    if (!item) return;
    QFont f = item->font(0);
    f.setItalic(out);
    item->setFont(0, f);
    if (out)
        item->setForeground(0, QColor(m_theme.valid ? m_theme.placeholder
                                                    : QStringLiteral("#6c7086")));
    else
        item->setData(0, Qt::ForegroundRole, QVariant()); // reset to default
}

void SidebarController::updateUnread(const ServerId &host, const BufferId &channel, int count)
{
    auto *item = channelItem(host, channel);
    if (!item) return;
    if (channel.str() == "(server)") {
        const bool connected = [&]{
            auto *s = m_model->session(host); return s && s->connected;
        }();
        if (connected) {
            const QColor col = count > 0 ? QColor("#e06c75") : QColor();
            item->setData(0, Qt::UserRole + 2, QVariant::fromValue(MenuIcons::connectedServer(col)));
        }
        item->setText(0, serverLabel(host).toUpper());
    } else {
        if (count > 0 && m_model->hasMention(host, channel))
            item->setData(0, Qt::UserRole + 2, QVariant::fromValue(MenuIcons::mention(QColor("#FFD700"))));
        else if (count > 0)
            item->setData(0, Qt::UserRole + 2, QVariant::fromValue(MenuIcons::unread()));
        else
            item->setData(0, Qt::UserRole + 2, QVariant());
        item->setData(0, Qt::UserRole + 3, count > 0 ? count : QVariant());
        item->setText(0, channel.str());
    }
}
