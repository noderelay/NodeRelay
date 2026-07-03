#include "ui/previewcontroller.h"
#include "ui/linkpreview.h"
#include "model/sessionmodel.h"

#include <QBuffer>
#include <QPixmap>
#include <QTimer>

PreviewController::PreviewController(SessionModel *model, QObject *parent)
    : QObject(parent), m_model(model)
{
    m_linkPreview = new LinkPreview(this);

    m_previewWatchdog = new QTimer(this);
    m_previewWatchdog->setSingleShot(true);
    connect(m_previewWatchdog, &QTimer::timeout, this, [this]{
        // Release the timed-out fetch's slot so it doesn't consume the
        // m_previewChannels budget forever and can be retried later.
        m_previewChannels.remove(m_inFlightUrl);
        m_inFlightUrl.clear();
        m_previewFetchBusy = false;
        processQueue();
    });

    connect(m_linkPreview, &LinkPreview::cardReady,
            this, &PreviewController::onCardReady);
}

void PreviewController::enqueue(const QUrl &url, ServerId host, BufferId channel, const QString &msgid)
{
    const QString key = url.toString();
    if (key.isEmpty()) return;
    if (m_previewChannels.contains(key)) return;
    if (m_previewQueue.size() >= 10) return;
    if (m_previewChannels.size() >= 100) return;
    m_previewChannels.insert(key, {host, channel, msgid});
    m_previewQueue.enqueue(url);
    processQueue();
}

void PreviewController::processQueue()
{
    if (m_previewFetchBusy || m_previewQueue.isEmpty()) return;
    m_previewFetchBusy = true;
    const QUrl url = m_previewQueue.dequeue();
    m_inFlightUrl = url.toString();
    m_previewWatchdog->start(20000);
    m_linkPreview->fetch(url);
}

void PreviewController::onCardReady(const QUrl &pageUrl, const QString &title, const QPixmap &thumbnail)
{
    const QString urlStr = pageUrl.toString();
    m_previewWatchdog->stop();
    m_inFlightUrl.clear();
    m_previewFetchBusy = false;

    auto it = m_previewChannels.find(urlStr);
    if (it == m_previewChannels.end()) {
        processQueue();
        return;
    }
    const ServerId host    = it->host;
    const BufferId channel = it->channel;
    const QString msgid    = it->msgid;
    m_previewChannels.erase(it);
    processQueue();

    auto *ch = m_model->channel(host, channel);
    if (!ch) return;

    // Thumbnail scaled to max 240 px wide
    QPixmap thumb;
    if (!thumbnail.isNull())
        thumb = thumbnail.scaledToWidth(qMin(thumbnail.width(), 240),
                                        Qt::SmoothTransformation);

    Channel::PreviewCard card;
    card.title   = title.left(120);
    card.domain  = pageUrl.host();
    card.pageUrl = urlStr;
    if (!thumb.isNull()) {
        QBuffer pngBuf(&card.pngData);
        pngBuf.open(QIODevice::WriteOnly);
        thumb.save(&pngBuf, "PNG");
    }
    ch->addPreview(urlStr, card);

    emit cardStored(host, channel, msgid, urlStr, thumb);
}
