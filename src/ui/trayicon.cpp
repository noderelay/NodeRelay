#include "trayicon.h"
#include "mainwindow.h"
#include "model/ids.h"
#include "model/sessionmodel.h"
#include "ui/appicons.h"

#include <QApplication>
#include <QPixmap>
#include <QPainter>

static QIcon withDot(const QIcon &base, const QColor &color)
{
    QPixmap pm = base.pixmap(32, 32);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(color);
    p.setPen(Qt::NoPen);
    p.drawEllipse(20, 0, 11, 11);
    return QIcon(pm);
}

TrayIcon::TrayIcon(SessionModel *model, MainWindow *window)
    : QSystemTrayIcon(window)
    , m_window(window)
    , m_model(model)
{
    m_baseIcon = AppIcons::appIcon();
    setIcon(m_baseIcon);
    setToolTip("Uplink");

    buildMenu();
    // Right-click menu only — left click is handled via activated signal
    setContextMenu(m_menu);

    connect(this, &QSystemTrayIcon::activated, this, &TrayIcon::onActivated);
    connect(m_model, &SessionModel::serverConnected,    this, &TrayIcon::onServerConnected);
    connect(m_model, &SessionModel::serverDisconnected, this, &TrayIcon::onServerDisconnected);
    connect(m_model, &SessionModel::unreadChanged,      this, &TrayIcon::onUnreadChanged);

    show();
}

void TrayIcon::buildMenu()
{
    // Parented to the window: setContextMenu() does not take ownership.
    m_menu = new QMenu(m_window);

    m_showAction = m_menu->addAction("Show", this, [this]{
        m_window->show();
        m_window->raise();
        m_window->activateWindow();
        updateShowAction();
    });

    m_menu->addSeparator();
    m_menu->addAction("Quit", qApp, &QApplication::quit);

    updateShowAction();
}

void TrayIcon::setBaseIcon(const QIcon &icon)
{
    m_baseIcon = icon;
    updateIcon();
}

void TrayIcon::updateShowAction()
{
    m_showAction->setText(m_window->isVisible() ? "Show" : "Show");
    // Always labelled "Show" — clicking it always brings the window up
}

void TrayIcon::onActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger) {
        if (m_window->isVisible()) {
            m_window->hide();
        } else {
            m_window->show();
            m_window->raise();
            m_window->activateWindow();
        }
    }
}

void TrayIcon::onServerConnected(ServerId)
{
    updateTooltip();
}

void TrayIcon::onServerDisconnected(ServerId)
{
    updateTooltip();
}

void TrayIcon::onUnreadChanged(ServerId, BufferId, int)
{
    m_totalUnread = 0;
    for (const auto &sess : m_model->sessions())
        for (const auto &ch : sess.channels)
            m_totalUnread += ch.unread;

    setUnread(m_totalUnread > 0);
}

void TrayIcon::setUnread(bool hasUnread)
{
    Q_UNUSED(hasUnread)
    updateIcon();
}

void TrayIcon::setNotify(bool hasNotify)
{
    m_hasNotify = hasNotify;
    updateIcon();
}

void TrayIcon::setNotificationsEnabled(bool enabled)
{
    m_notificationsEnabled = enabled;
    if (!enabled) m_hasNotify = false;
    updateIcon();
}

void TrayIcon::updateIcon()
{
    if (!m_notificationsEnabled) {
        setIcon(m_baseIcon);
        return;
    }
    if (m_hasNotify)
        setIcon(withDot(m_baseIcon, QColor("#50e050")));
    else if (m_totalUnread > 0)
        setIcon(withDot(m_baseIcon, QColor("#e05050")));
    else
        setIcon(m_baseIcon);
}

void TrayIcon::updateTooltip()
{
    QStringList connected;
    for (const auto &sess : m_model->sessions())
        if (sess.connected) connected << sess.host;

    setToolTip(connected.isEmpty()
               ? "Uplink — not connected"
               : "Uplink — " + connected.join(", "));
}
