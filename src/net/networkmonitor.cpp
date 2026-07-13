#include "networkmonitor.h"

#if QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
#include <QNetworkInformation>

NetworkMonitor::NetworkMonitor(QObject *parent)
    : QObject(parent)
{
    // Prefer a backend that also reports metered state; fall back to
    // reachability-only so instant reconnect still works without it.
    using Feature = QNetworkInformation::Feature;
    if (!QNetworkInformation::loadBackendByFeatures(Feature::Reachability | Feature::Metered)
        && !QNetworkInformation::loadBackendByFeatures(Feature::Reachability))
        return;   // no backend on this platform; stay inert

    auto *ni = QNetworkInformation::instance();
    if (!ni)
        return;

    m_wasOnline = ni->reachability() == QNetworkInformation::Reachability::Online;
    connect(ni, &QNetworkInformation::reachabilityChanged,
            this, [this](QNetworkInformation::Reachability r){
        // Site/Local/Unknown all count as not-online; fire once per recovery.
        const bool online = r == QNetworkInformation::Reachability::Online;
        if (online && !m_wasOnline)
            emit onlineAgain();
        m_wasOnline = online;
    });
}

bool NetworkMonitor::isMetered() const
{
    auto *ni = QNetworkInformation::instance();
    return ni && ni->supports(QNetworkInformation::Feature::Metered) && ni->isMetered();
}

#else  // Qt < 6.3: inert stub

NetworkMonitor::NetworkMonitor(QObject *parent)
    : QObject(parent)
{
}

bool NetworkMonitor::isMetered() const
{
    return false;
}

#endif
