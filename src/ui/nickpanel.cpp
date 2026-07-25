#include "mainwindow.h"
#include "ui/channelpane.h"
#include "ui/nickfilteredit.h"
#include "ui/nicklistmodel.h"
#include "ui/sidebarcontroller.h"
#include "ui/typingcontroller.h"
#include "model/sessionmodel.h"
#include "net/addresscheck.h"
#include "net/networkmonitor.h"
#include "logging.h"

#include <QBuffer>
#include <QHostInfo>
#include <QImageReader>
#include <QLabel>
#include <QListView>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>
#include <QScrollBar>
#include <QTimer>
#include <QUrl>
#include <memory>
#include "version.h"

// Avatars render at 36px; anything past this is hostile or broken.
static constexpr qsizetype kMaxAvatarBytes = 1 * 1024 * 1024;

// Re-snapshot the model without the view jumping: a model reset drops the
// scroll position, so save and restore it around the refresh.
static void refreshKeepingScroll(NickListModel *model, QListView *view)
{
    const int pos = view->verticalScrollBar()->value();
    model->refresh();
    view->verticalScrollBar()->setValue(pos);
}

QString MainWindow::nickTooltip(const QString &nick, const ServerId &host) const
{
    const ServerSession *sess = const_cast<SessionModel *>(m_model)->session(host);
    const NickMeta *meta = nullptr;
    QString account;
    if (sess) {
        auto it = sess->nickMeta.constFind(nick.toLower());
        if (it != sess->nickMeta.constEnd()) meta = &it.value();
        if (!meta)  // fetched on first hover, not roster-wide; shows on next hover
            const_cast<SessionModel *>(m_model)->requestNickMeta(host, nick);
        // account comes from Channel's nicks list
        for (const auto &ch : std::as_const(sess->channels)) {
            auto ni = std::find_if(ch.nicks.cbegin(), ch.nicks.cend(),
                                   [&](const NickEntry &e){ return e.nick.toLower() == nick.toLower(); });
            if (ni != ch.nicks.cend()) { account = ni->account; break; }
        }
    }
    const bool hasImage = meta && !meta->avatarUrl.isEmpty() && m_avatarCache.contains(meta->avatarUrl);
    if (hasImage) {
        QByteArray bytes;
        QBuffer buf(const_cast<QByteArray *>(&bytes));
        buf.open(QIODevice::WriteOnly);
        m_avatarCache[meta->avatarUrl].save(&buf, "PNG");
        const QString b64 = QString::fromLatin1(bytes.toBase64());
        QStringList lines;
        if (!meta->displayName.isEmpty())
            lines << meta->displayName.toHtmlEscaped();
        if (!account.isEmpty())
            lines << account.toHtmlEscaped();
        return QString("<html><body><table><tr>"
                       "<td><img src='data:image/png;base64,%1' width='32' height='32'></td>"
                       "<td style='padding-left:6px;vertical-align:middle'>%2</td>"
                       "</tr></table></body></html>")
                   .arg(b64, lines.join("<br>"));
    }
    QStringList tips;
    if (meta && !meta->displayName.isEmpty()) tips << meta->displayName;
    if (!account.isEmpty())                   tips << account;
    return tips.join('\n');
}

void MainWindow::updateNickViews(const ServerId &host, const BufferId &channel)
{
    auto *ch = m_model->channel(host, channel);

    const bool isActive = (host == m_model->activeHost() &&
                           channel.str().toLower() == m_model->activeChannel().str().toLower());
    if (isActive) {
        refreshKeepingScroll(m_nickModel, m_nickList);
        if (m_nickCountLabel && ch) {
            const QString countStr = QString::number(ch->nicks.size());
            m_nickCountLabel->setText(countStr);
            m_nickCountLabel->setToolTip(countStr + " users");
        }
    }

    if (auto *pane = m_panes.value(paneKey(host, channel))) {
        refreshKeepingScroll(pane->nickModel(), pane->nickList());
        if (ch) pane->setNickCount(static_cast<int>(ch->nicks.size()));
    }
}

void MainWindow::onNickAdded(const ServerId &host, const BufferId &channel, const QString &)
{
    updateNickViews(host, channel);
}

void MainWindow::onNickRemoved(const ServerId &host, const BufferId &channel, const QString &nick)
{
    updateNickViews(host, channel);

    m_typing->forgetNick(host, channel, nick);
}

void MainWindow::onNickRenamed(const ServerId &host, const BufferId &channel,
                                const QString &, const QString &)
{
    updateNickViews(host, channel);
}

void MainWindow::refreshNickList(const ServerId &host, const BufferId &channel)
{
    if (m_nickFilter) m_nickFilter->clear();
    m_nickModel->setBuffer(host, channel);

    auto *ch = m_model->channel(host, channel);
    if (m_nickCountLabel) {
        const QString countStr = QString::number(ch ? ch->nicks.size() : 0);
        m_nickCountLabel->setText(countStr);
        m_nickCountLabel->setToolTip(countStr + " users");
    }
}

// Anything that changes how nicks are drawn (an avatar arriving, the avatar
// setting, the ignore list) has to reach every user list on screen, not just
// the main view's — a pane would otherwise keep the old rendering until
// something unrelated refreshed it.
void MainWindow::refreshVisibleNickLists()
{
    scheduleNickRefresh(m_model->activeHost(), m_model->activeChannel());
    for (auto *p : std::as_const(m_panes))
        scheduleNickRefresh(p->host(), p->channel());
}

// Channel avatars are explicit user actions — a silently missing icon reads
// as broken, so fetch failures get a line in the affected channel buffer(s).
// User avatars keep failing quietly; hover fetches would spam otherwise.
void MainWindow::failChanAvatar(const QString &url, const QString &reason)
{
    for (const auto &buf : m_pendingChanAvatars.take(url)) {
        // The buffer may have been parted/closed while the fetch was in
        // flight — posting would recreate it as an invisible zombie
        if (!m_model->channel(buf.first, buf.second))
            continue;
        m_model->localMessage(buf.first, buf.second,
            "Channel avatar not loaded — " + reason + " (" + url + ")");
    }
}

void MainWindow::onChannelAvatarChanged(const ServerId &host, const BufferId &channel, const QString &url)
{
    if (!m_config.ui.showAvatars) return;   // URL stays in the model for re-enable
    if (url.isEmpty()) {
        m_sidebarCtl->setChannelAvatar(host, channel, QIcon());
        return;
    }
    if (m_avatarCache.contains(url)) {
        m_sidebarCtl->setChannelAvatar(host, channel, QIcon(m_avatarCache.value(url)));
        return;
    }
    m_pendingChanAvatars[url].append({host, channel});
    fetchAvatar(url);
}

void MainWindow::fetchAvatar(const QString &url)
{
    if (!m_config.ui.showAvatars) return;
    if (url.isEmpty() || m_avatarCache.contains(url) || m_avatarFetching.contains(url))
        return;

    auto cacheAndRefresh = [this, url](QPixmap px) {
        if (px.isNull()) return;
        px = px.scaled(36, 36, Qt::KeepAspectRatio, Qt::FastTransformation);
        static constexpr int kAvatarCacheCap = 80;
        if (!m_avatarCache.contains(url)) {
            if (m_avatarCacheOrder.size() >= kAvatarCacheCap) {
                const QString evicted = m_avatarCacheOrder.takeFirst();
                m_avatarCache.remove(evicted);
            }
            m_avatarCacheOrder.append(url);
        }
        m_avatarCache.insert(url, px);
        refreshVisibleNickLists();
        for (const auto &buf : m_pendingChanAvatars.take(url))
            m_sidebarCtl->setChannelAvatar(buf.first, buf.second, QIcon(px));
    };

    // Local file — honored only for the user's own configured avatar. Avatar
    // URLs also arrive via metadata from other users, who must never be able
    // to point Uplink at the local filesystem.
    const QUrl qurl(url);
    if (qurl.isLocalFile() || url.startsWith('/')) {
        if (url != m_config.profileAvatarUrl) {
            failChanAvatar(url, "local file paths are not accepted from the network");
            return;
        }
        cacheAndRefresh(QPixmap(qurl.isLocalFile() ? qurl.toLocalFile() : url));
        return;
    }

    if (m_model->networkMonitor()->isMetered()) {
        failChanAvatar(url, "skipped on metered connection");
        return;   // spare metered data; local avatars above still load
    }

    // Same SSRF discipline as link previews: scheme/literal gate, DNS
    // pre-check against private ranges, then fetch pinned to the vetted IP.
    if (isBlockedBySchemeOrLiteral(qurl)) {
        qCDebug(lcPreview) << "avatar: blocked" << url;
        failChanAvatar(url, "URL scheme or address not allowed");
        return;
    }

    if (!m_avatarNam)
        m_avatarNam = new QNetworkAccessManager(this);
    m_avatarFetching.insert(url);
    QHostInfo::lookupHost(qurl.host(), this,
        [this, url, qurl, cacheAndRefresh](const QHostInfo &info) {
        const auto addrs = info.addresses();
        bool blocked = info.error() != QHostInfo::NoError || addrs.isEmpty();
        for (const QHostAddress &a : addrs)
            if (isPrivateAddress(a)) blocked = true;
        if (blocked) {
            m_avatarFetching.remove(url);
            qCDebug(lcPreview) << "avatar: blocked private address for" << qurl.host();
            failChanAvatar(url, "host is unresolvable or resolves to a private address");
            return;
        }

        QNetworkRequest req = pinnedRequest(qurl, addrs.first());
        req.setRawHeader("User-Agent", "Uplink/" UPLINK_VERSION);
        // A followed redirect would escape the pinned address, so don't follow.
        req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::ManualRedirectPolicy);
        // Without a timeout a stalled server keeps the URL in m_avatarFetching
        // forever, permanently blocking retries for that avatar.
        req.setTransferTimeout(10000);

        auto *reply = m_avatarNam->get(req);
        auto  buf   = std::make_shared<QByteArray>();
        connect(reply, &QNetworkReply::readyRead, this, [reply, buf] {
            const qsizetype rem = kMaxAvatarBytes - buf->size();
            if (rem <= 0) { reply->abort(); return; }
            buf->append(reply->read(rem));
            if (buf->size() >= kMaxAvatarBytes) reply->abort();
        });
        connect(reply, &QNetworkReply::finished, this,
            [this, reply, url, buf, cacheAndRefresh] {
            reply->deleteLater();
            m_avatarFetching.remove(url);
            if (reply->error() != QNetworkReply::NoError) {
                failChanAvatar(url, buf->size() >= kMaxAvatarBytes
                    ? "image exceeds the 1 MB limit"
                    : reply->errorString());
                return;
            }
            if (reply->attribute(QNetworkRequest::RedirectionTargetAttribute).isValid()) {
                failChanAvatar(url, "server redirected; redirects are not followed");
                return;
            }

            // Decode via QImageReader with a dimension gate and scaled decode —
            // a small compressed file must not balloon into a huge bitmap.
            QBuffer imgBuf(buf.get());
            imgBuf.open(QIODevice::ReadOnly);
            QImageReader reader(&imgBuf);
            const QSize srcSize = reader.size();
            if (!srcSize.isValid() || srcSize.width() > 4096 || srcSize.height() > 4096) {
                failChanAvatar(url, "not a valid image, or larger than 4096px");
                return;
            }
            reader.setScaledSize(srcSize.scaled(36, 36, Qt::KeepAspectRatio));
            const QImage img = reader.read();
            if (!img.isNull())
                cacheAndRefresh(QPixmap::fromImage(img));
            else
                failChanAvatar(url, "could not decode image");
        });
    });
}

// Live toggle. Off drops every image we hold — leaving them on screen would
// suggest the fetches are still happening. On re-fetches the URLs we already
// know about; anything else arrives with the next metadata push or hover.
void MainWindow::applyShowAvatarsSetting(bool on)
{
    if (!on) {
        m_avatarCache.clear();
        m_avatarCacheOrder.clear();
        m_pendingChanAvatars.clear();
        for (const auto &sess : m_model->sessions())
            for (const auto &ch : std::as_const(sess.channels))
                m_sidebarCtl->setChannelAvatar(ServerId{sess.name}, BufferId{ch.name}, QIcon());
        refreshVisibleNickLists();
        return;
    }

    for (const auto &sess : m_model->sessions()) {
        for (const auto &ch : std::as_const(sess.channels))
            if (!ch.avatarUrl.isEmpty())
                onChannelAvatarChanged(ServerId{sess.name}, BufferId{ch.name}, ch.avatarUrl);
        for (const auto &meta : std::as_const(sess.nickMeta))
            fetchAvatar(meta.avatarUrl);
    }
    fetchAvatar(m_config.profileAvatarUrl);
}
