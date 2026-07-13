#pragma once
#include <QObject>

// Thin wrapper over QNetworkInformation. Emits onlineAgain() when network
// reachability transitions from not-online back to Online, and reports
// whether the current connection is metered. Fully inert (isMetered()
// false, onlineAgain() never fires) when no OS backend loads or when
// built against Qt older than 6.3.
class NetworkMonitor : public QObject
{
    Q_OBJECT
public:
    explicit NetworkMonitor(QObject *parent = nullptr);

    bool isMetered() const;

signals:
    void onlineAgain();

private:
    bool m_wasOnline{true};
};
