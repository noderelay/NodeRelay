#pragma once

#include <QObject>

#include "model/ids.h"

struct Config;
struct Theme;
class SessionModel;
class SidebarDelegate;
class QTreeWidget;
class QTreeWidgetItem;

// Owns the sidebar tree widget and its item bookkeeping: row creation and
// lookup for servers/channels, display labels, unread badges (self-connected
// to the model), connection-state icons, and checked-out markers. Item data
// roles: UserRole = host, UserRole+1 = channel ("(server)" on server rows),
// UserRole+2 = badge icon, UserRole+3 = unread count, UserRole+5 = channel
// avatar icon.
// Selection behavior, navigation, ordering, and drag/drop stay in MainWindow,
// which reaches the widget via tree().
class SidebarController : public QObject
{
    Q_OBJECT
public:
    SidebarController(SessionModel *model, const Config &config, const Theme &theme,
                      QObject *parent = nullptr);

    QTreeWidget     *tree()     const { return m_tree; }
    SidebarDelegate *delegate() const { return m_delegate; }

    QTreeWidgetItem *serverItem (const ServerId &host) const;
    QTreeWidgetItem *channelItem(const ServerId &host, const BufferId &channel) const;

    // Display name for a server row: the config entry's name, falling back
    // to the host with its "irc." prefix and TLD trimmed.
    QString serverLabel(const ServerId &host) const;

    QTreeWidgetItem *addServerItem (const ServerId &host);
    QTreeWidgetItem *addChannelItem(const ServerId &host, const BufferId &channel);
    void removeServerItem(const ServerId &host);

    void markConnected(const ServerId &host);   // shows the connected dot
    void clearConnectionIcon(const ServerId &host);

    // Italic + dimmed row for channels checked out to a floating window.
    void setCheckedOut(const ServerId &host, const BufferId &channel, bool out);

    // Leading avatar icon for channels that publish one via metadata;
    // a null icon restores the plain row.
    void setChannelAvatar(const ServerId &host, const BufferId &channel, const QIcon &icon);

private:
    void updateUnread(const ServerId &host, const BufferId &channel, int count);

    SessionModel *m_model;
    const Config &m_config;
    const Theme  &m_theme;

    QTreeWidget     *m_tree{nullptr};
    SidebarDelegate *m_delegate{nullptr};
};
