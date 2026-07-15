#include "linkpreview.h"
#include "logging.h"
#include "net/addresscheck.h"

#include <QBuffer>
#include <QHostAddress>
#include <QHostInfo>
#include <QImageReader>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <memory>

static constexpr int kMaxBytes     = 32768;   // 32 KB
static constexpr int kMaxImgBytes  = 2097152; // 2 MB
static constexpr int kMaxCache     = 20;
static constexpr int kTimeoutMs    = 6000;
static constexpr int kImgTimeoutMs = 15000;
static constexpr int kMaxRedirects = 3;

// SSRF guards (isBlockedBySchemeOrLiteral, pinnedRequest) live in
// net/addresscheck.h — shared with the avatar fetch in nickpanel.cpp.

// ── misc helpers ──────────────────────────────────────────────────────────────

static bool isImageUrl(const QUrl &url)
{
    const QString p = url.path().toLower();
    return p.endsWith(".png") || p.endsWith(".jpg") || p.endsWith(".jpeg")
        || p.endsWith(".gif") || p.endsWith(".webp");
}

static QString decodeEntities(const QString &s)
{
    QString r = s;
    r.replace(QLatin1String("&amp;"),  QLatin1String("&"));
    r.replace(QLatin1String("&lt;"),   QLatin1String("<"));
    r.replace(QLatin1String("&gt;"),   QLatin1String(">"));
    r.replace(QLatin1String("&quot;"), QLatin1String("\""));
    r.replace(QLatin1String("&#39;"),  QLatin1String("'"));
    r.replace(QLatin1String("&apos;"), QLatin1String("'"));
    r.replace(QLatin1String("&nbsp;"), QLatin1String(" "));
    r.replace(QLatin1String("&ndash;"), QLatin1String("–"));
    r.replace(QLatin1String("&mdash;"), QLatin1String("—"));
    return r;
}

// ── LinkPreview ───────────────────────────────────────────────────────────────

LinkPreview::LinkPreview(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{}

void LinkPreview::fetch(const QUrl &url)
{
    if (m_dnsId) {
        QHostInfo::abortHostLookup(m_dnsId);
        m_dnsId = 0;
    }
    if (m_imgDnsId) {
        QHostInfo::abortHostLookup(m_imgDnsId);
        m_imgDnsId = 0;
    }
    if (m_reply) {
        m_reply->disconnect(this);
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }
    m_buf.clear();
    m_redirectCount = 0;

    resolveAndFetch(url);
}

void LinkPreview::fetchHover(const QUrl &url)
{
    if (m_hoverDnsId) { QHostInfo::abortHostLookup(m_hoverDnsId); m_hoverDnsId = 0; }
    if (m_hoverReply) {
        m_hoverReply->disconnect(this);
        m_hoverReply->abort();
        m_hoverReply->deleteLater();
        m_hoverReply = nullptr;
    }
    ++m_hoverGen;
    resolveAndFetchHover(url);
}

void LinkPreview::resolveAndFetchHover(const QUrl &url)
{
    if (isBlockedBySchemeOrLiteral(url)) return;

    const QString key = url.toString();
    if (m_cache.contains(key)) {
        emit titleReady(url, m_cache[key].title);
        return;
    }

    const quint64 gen = m_hoverGen;
    m_hoverDnsId = QHostInfo::lookupHost(url.host(), this,
        [this, url, gen](const QHostInfo &info) {
            m_hoverDnsId = 0;
            if (gen != m_hoverGen) return;
            if (info.error() != QHostInfo::NoError) return;
            if (info.addresses().isEmpty()) return;
            for (const QHostAddress &a : info.addresses())
                if (isPrivateAddress(a)) {
                    qCDebug(lcPreview) << "blocked private address for" << url.host();
                    return;
                }

            QNetworkRequest req = pinnedRequest(url, info.addresses().first());
            req.setRawHeader("User-Agent", "WhatsApp/2");
            req.setRawHeader("Accept", "text/html,application/xhtml+xml;q=0.9,*/*;q=0.8");
            req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                             QNetworkRequest::ManualRedirectPolicy);
            req.setTransferTimeout(kTimeoutMs);

            m_hoverReply = m_nam->get(req);
            QNetworkReply *reply = m_hoverReply;
            auto buf = std::make_shared<QByteArray>();

            connect(reply, &QNetworkReply::readyRead, this, [reply, buf] {
                const qsizetype rem = kMaxBytes - buf->size();
                if (rem <= 0) { reply->abort(); return; }
                buf->append(reply->read(rem));
                if (buf->size() >= kMaxBytes) reply->abort();
            });

            connect(reply, &QNetworkReply::finished, this, [this, reply, buf, url, gen] {
                if (m_hoverReply == reply) m_hoverReply = nullptr;
                reply->deleteLater();
                if (gen != m_hoverGen) return;
                const QString title = extractTitle(*buf);
                if (!title.isEmpty())
                    emit titleReady(url, title);
            });
        });
}

// Resolves the hostname via DNS, verifies every returned address is non-private,
// then calls doPageFetch. Used for both initial fetches and redirect follow-ups.
void LinkPreview::resolveAndFetch(const QUrl &url)
{
    if (isBlockedBySchemeOrLiteral(url)) return;

    if (m_redirectCount == 0) {
        const QString key = url.toString();
        if (m_cache.contains(key)) {
            const CachedCard &c = m_cache[key];
            emit titleReady(url, c.title);
            QPixmap pm;
            if (!c.pngData.isEmpty())
                pm.loadFromData(c.pngData, "PNG");
            emit cardReady(url, c.title, pm);
            return;
        }
    }

    m_pendingUrl = url;
    ++m_fetchGen;
    const quint64 gen = m_fetchGen;

    m_dnsId = QHostInfo::lookupHost(url.host(), this,
        [this, url, gen](const QHostInfo &info) {
            m_dnsId = 0;
            if (gen != m_fetchGen) return;
            if (info.error() != QHostInfo::NoError) return;
            if (info.addresses().isEmpty()) return;
            for (const QHostAddress &a : info.addresses())
                if (isPrivateAddress(a)) {
                    qCDebug(lcPreview) << "blocked private address for" << url.host();
                    return;
                }
            doPageFetch(url, info.addresses().first());
        });
}

void LinkPreview::doPageFetch(const QUrl &url, const QHostAddress &addr)
{
    if (isImageUrl(url)) {
        const QString filename = url.fileName();
        fetchImage(url, filename.isEmpty() ? url.host() : filename, url);
        return;
    }

    m_buf.clear();
    QNetworkRequest req = pinnedRequest(url, addr);
    req.setRawHeader("User-Agent", "WhatsApp/2");
    req.setRawHeader("Accept", "text/html,application/xhtml+xml;q=0.9,*/*;q=0.8");
    req.setRawHeader("Accept-Language", "en-US,en;q=0.5");
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::ManualRedirectPolicy);
    req.setTransferTimeout(kTimeoutMs);

    m_reply = m_nam->get(req);
    QNetworkReply *reply = m_reply;

    connect(reply, &QNetworkReply::readyRead, this, [this, reply] {
        if (m_reply != reply) return;
        const qsizetype rem = kMaxBytes - m_buf.size();
        if (rem <= 0) { reply->abort(); return; }
        m_buf.append(reply->read(rem));
        if (m_buf.size() >= kMaxBytes) reply->abort();
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply, url] {
        if (m_reply != reply) return;
        m_reply = nullptr;

        // Re-run DNS check on every redirect destination before following it.
        // Resolve against the logical URL — reply->url() carries the pinned IP.
        const QVariant redir =
            reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
        if (redir.isValid()) {
            const QUrl next = url.resolved(redir.toUrl());
            reply->deleteLater();
            m_buf.clear();
            if (m_redirectCount < kMaxRedirects) {
                ++m_redirectCount;
                resolveAndFetch(next);
            }
            return;
        }

        const int httpStatus =
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (httpStatus >= 400) {
            reply->deleteLater();
            m_buf.clear();
            return;
        }

        const QUrl    pageUrl = m_pendingUrl;
        const QString title   = extractTitle(m_buf);
        const QUrl    imgUrl  = extractImageUrl(m_buf);
        reply->deleteLater();
        m_buf.clear();

        if (title.isEmpty()) return;

        emit titleReady(pageUrl, title);

        if (imgUrl.isValid())
            fetchImage(pageUrl, title, imgUrl);
        else {
            insertCache(pageUrl.toString(), {title, {}});
            emit cardReady(pageUrl, title, {});
        }
    });
}

// DNS-checks the image URL before fetching, same policy as page fetches.
void LinkPreview::fetchImage(const QUrl &pageUrl, const QString &title, const QUrl &imageUrl)
{
    if (isBlockedBySchemeOrLiteral(imageUrl)) return;

    if (m_imgDnsId) {
        QHostInfo::abortHostLookup(m_imgDnsId);
        m_imgDnsId = 0;
    }

    m_imgDnsId = QHostInfo::lookupHost(imageUrl.host(), this,
        [this, pageUrl, title, imageUrl](const QHostInfo &info) {
            m_imgDnsId = 0;
            if (info.error() != QHostInfo::NoError) return;
            if (info.addresses().isEmpty()) return;
            for (const QHostAddress &a : info.addresses())
                if (isPrivateAddress(a)) {
                    qCDebug(lcPreview) << "blocked private address for" << imageUrl.host();
                    return;
                }
            doImageFetch(pageUrl, title, imageUrl, info.addresses().first());
        });
}

void LinkPreview::doImageFetch(const QUrl &pageUrl, const QString &title, const QUrl &imageUrl,
                               const QHostAddress &addr)
{
    QNetworkRequest req = pinnedRequest(imageUrl, addr);
    req.setRawHeader("User-Agent",
        "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 "
        "(KHTML, like Gecko) Chrome/124.0 Safari/537.36");
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::ManualRedirectPolicy);
    req.setTransferTimeout(kImgTimeoutMs);

    auto *imgReply = m_nam->get(req);
    auto  imgBuf   = std::make_shared<QByteArray>();

    connect(imgReply, &QNetworkReply::readyRead, this, [imgReply, imgBuf] {
        const qsizetype rem = kMaxImgBytes - imgBuf->size();
        if (rem <= 0) { imgReply->abort(); return; }
        imgBuf->append(imgReply->read(rem));
        if (imgBuf->size() >= kMaxImgBytes) imgReply->abort();
    });

    connect(imgReply, &QNetworkReply::finished, this,
        [this, imgReply, imgBuf, pageUrl, title] {
            const QVariant redir =
                imgReply->attribute(QNetworkRequest::RedirectionTargetAttribute);
            imgReply->deleteLater();
            if (redir.isValid()) {
                insertCache(pageUrl.toString(), {title, {}});
                emit cardReady(pageUrl, title, {});
                return;
            }

            QPixmap pm;
            if (imgReply->error() != QNetworkReply::NoError) {
                insertCache(pageUrl.toString(), {title, {}});
                emit cardReady(pageUrl, title, {});
                return;
            }
            if (!imgBuf->isEmpty()) {
                QBuffer buf(imgBuf.get());
                buf.open(QIODevice::ReadOnly);
                QImageReader reader(&buf);
                const QSize srcSize = reader.size();
                if (srcSize.isValid() && srcSize.width() <= 4096 && srcSize.height() <= 4096) {
                    reader.setScaledSize(srcSize.scaled(360, 220, Qt::KeepAspectRatio));
                    QImage img = reader.read();
                    if (!img.isNull())
                        pm = QPixmap::fromImage(std::move(img));
                }
            }

            QByteArray pngData;
            if (!pm.isNull()) {
                QBuffer pngBuf(&pngData);
                pngBuf.open(QIODevice::WriteOnly);
                pm.save(&pngBuf, "PNG");
            }
            insertCache(pageUrl.toString(), {title, pngData});
            emit cardReady(pageUrl, title, pm);
        });
}

void LinkPreview::insertCache(const QString &key, CachedCard &&card)
{
    if (m_cache.contains(key)) {
        m_cache[key] = std::move(card);
        return;
    }
    if (m_cache.size() >= kMaxCache) {
        m_cache.remove(m_cacheOrder.front());
        m_cacheOrder.removeFirst();
    }
    m_cache.insert(key, std::move(card));
    m_cacheOrder.append(key);
}

QString LinkPreview::extractTitle(const QByteArray &data) const
{
    const QString html = QString::fromUtf8(data);

    static const QRegularExpression ogPropFirst(
        R"(<meta[^>]+property\s*=\s*["']og:title["'][^>]+content\s*=\s*["']([^"']{1,200})["'])",
        QRegularExpression::CaseInsensitiveOption);
    auto m = ogPropFirst.match(html);
    if (m.hasMatch()) return decodeEntities(m.captured(1).trimmed());

    static const QRegularExpression ogContentFirst(
        R"(<meta[^>]+content\s*=\s*["']([^"']{1,200})["'][^>]+property\s*=\s*["']og:title["'])",
        QRegularExpression::CaseInsensitiveOption);
    m = ogContentFirst.match(html);
    if (m.hasMatch()) return decodeEntities(m.captured(1).trimmed());

    static const QRegularExpression titleTag(
        R"(<title[^>]*>([\s\S]{1,300}?)</title>)",
        QRegularExpression::CaseInsensitiveOption);
    m = titleTag.match(html);
    if (m.hasMatch()) {
        QString t = decodeEntities(m.captured(1).simplified());
        if (!t.isEmpty()) return t;
    }

    return {};
}

QUrl LinkPreview::extractImageUrl(const QByteArray &data) const
{
    const QString html = QString::fromUtf8(data);

    static const QRegularExpression ogPropFirst(
        R"(<meta[^>]+property\s*=\s*["']og:image["'][^>]+content\s*=\s*["']([^"']{1,500})["'])",
        QRegularExpression::CaseInsensitiveOption);
    auto m = ogPropFirst.match(html);
    if (m.hasMatch()) {
        const QUrl u(m.captured(1).trimmed());
        if (u.isValid() && !isBlockedBySchemeOrLiteral(u)) return u;
    }

    static const QRegularExpression ogContentFirst(
        R"(<meta[^>]+content\s*=\s*["']([^"']{1,500})["'][^>]+property\s*=\s*["']og:image["'])",
        QRegularExpression::CaseInsensitiveOption);
    m = ogContentFirst.match(html);
    if (m.hasMatch()) {
        const QUrl u(m.captured(1).trimmed());
        if (u.isValid() && !isBlockedBySchemeOrLiteral(u)) return u;
    }

    return {};
}
