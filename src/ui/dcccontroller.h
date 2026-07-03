#pragma once

#include <QMap>
#include <QObject>
#include <QString>

#include "model/ids.h"

class QWidget;
class SessionModel;
class DccSend;

// Owns all DCC file-transfer UI: incoming offer dialogs, progress dialogs,
// send actions from the nick context menu, and the passive-send pending map.
class DccController : public QObject
{
    Q_OBJECT
public:
    DccController(SessionModel *model, QWidget *parentWindow);

    void sendFile(ServerId host, const QString &nick);
    void sendFilePassive(ServerId host, const QString &nick);

private:
    void onSendReceived(ServerId server, const QString &fromNick,
                        const QString &filename, quint32 ip, quint16 port, qint64 filesize);
    void onPassiveOfferReceived(ServerId server, const QString &fromNick,
                                const QString &filename, quint32 senderIp,
                                qint64 filesize, const QString &token);
    void onPassiveSendReply(ServerId server, const QString &fromNick, const QString &filename,
                            quint32 ip, quint16 port, qint64 filesize, const QString &token);

    SessionModel *m_model;
    QWidget      *m_window;
    QMap<QString, DccSend*> m_pendingPassiveSends;
};
