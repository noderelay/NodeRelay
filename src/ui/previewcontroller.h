#pragma once

#include <QHash>
#include <QObject>
#include <QQueue>
#include <QString>
#include <QUrl>

#include "model/ids.h"

class LinkPreview;
class SessionModel;
class QTimer;

// Owns the link-preview fetch queue: throttled fetching via LinkPreview,
// watchdog recovery, and storing finished cards on the Channel. Emits
// cardStored so the UI can insert the card into visible chat views.
class PreviewController : public QObject
{
    Q_OBJECT
public:
    PreviewController(SessionModel *model, QObject *parent = nullptr);

    LinkPreview *linkPreview() const { return m_linkPreview; }

    void enqueue(const QUrl &url, ServerId host, BufferId channel, const QString &msgid);

signals:
    void cardStored(ServerId host, BufferId channel, const QString &msgid, const QString &url);

private:
    void processQueue();
    void onCardReady(const QUrl &pageUrl, const QString &title, const QPixmap &thumbnail);

    struct PreviewCtx { ServerId host; BufferId channel; QString msgid; };

    SessionModel *m_model;
    LinkPreview  *m_linkPreview;
    QHash<QString, PreviewCtx> m_previewChannels; // url → {host, channel, msgid}
    QQueue<QUrl>  m_previewQueue;
    bool          m_previewFetchBusy{false};
    QTimer       *m_previewWatchdog{nullptr};
};
