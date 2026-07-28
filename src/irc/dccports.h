#pragma once

#include <QHostAddress>
#include <QTcpServer>

// Binds a DCC listener inside [portMin, portMax] when a range is configured
// (first free port wins); an unset range (portMin == 0) keeps the ephemeral
// bind. The loop counter is wider than quint16 so portMax == 65535 terminates.
inline bool dccListen(QTcpServer *server, const QHostAddress &bindAddr,
                      quint16 portMin, quint16 portMax)
{
    if (portMin == 0)
        return server->listen(bindAddr, 0);
    for (quint32 port = portMin; port <= portMax; ++port)
        if (server->listen(bindAddr, quint16(port)))
            return true;
    return false;
}
