#pragma once

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QObject>
#include <QPixmap>
#include <QUrl>

class QHostAddress;
class QNetworkAccessManager;
class QNetworkReply;

class LinkPreview : public QObject
{
    Q_OBJECT
public:
    explicit LinkPreview(QObject *parent = nullptr);
    void fetch(const QUrl &url);
    void fetchHover(const QUrl &url);

signals:
    void titleReady(const QUrl &url, const QString &title);
    void cardReady(const QUrl &pageUrl, const QString &title, const QPixmap &thumbnail);

private:
    struct CachedCard {
        QString    title;
        QByteArray pngData; // compressed PNG thumbnail; empty if no image
    };

    void resolveAndFetch(const QUrl &url);
    void doPageFetch(const QUrl &url, const QHostAddress &addr);
    void fetchImage(const QUrl &pageUrl, const QString &title, const QUrl &imageUrl);
    void doImageFetch(const QUrl &pageUrl, const QString &title, const QUrl &imageUrl,
                      const QHostAddress &addr);
    void resolveAndFetchHover(const QUrl &url);
    void insertCache(const QString &key, CachedCard &&card);

    QString extractTitle(const QByteArray &data) const;
    QUrl    extractImageUrl(const QByteArray &data) const;

    QNetworkAccessManager    *m_nam;

    // Card fetch path
    QNetworkReply            *m_reply{nullptr};
    QUrl                      m_pendingUrl;
    QByteArray                m_buf;
    int                       m_dnsId{0};
    int                       m_imgDnsId{0};
    int                       m_redirectCount{0};
    quint64                   m_fetchGen{0};

    // Hover fetch path (abort-on-new, page only, no image)
    QNetworkReply            *m_hoverReply{nullptr};
    int                       m_hoverDnsId{0};
    quint64                   m_hoverGen{0};

    QHash<QString, CachedCard> m_cache;
    QList<QString>             m_cacheOrder; // insertion order for FIFO eviction
};
