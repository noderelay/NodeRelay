#include "mainwindow.h"
#include "ui/chatrenderer.h"
#include "ui/channelpane.h"
#include "ui/menuicons.h"
#include "ui/nickfilteredit.h"
#include "model/sessionmodel.h"

#include <QBuffer>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>
#include <QRandomGenerator>
#include <QTimer>
#include <QUrl>
#include "version.h"

QListWidgetItem *MainWindow::makeNickItem(const NickEntry &e, const Channel *ch,
                                           const ServerSession *sess)
{
    const bool isBot = ch->botNicks.contains(e.nick.toLower())
                    || (sess && sess->botNicks.contains(e.nick.toLower()));
    auto *item = new QListWidgetItem(e.display());
    if (isBot) {
        const QString key = e.nick.toLower();
        if (!m_botIconIdx.contains(key)) {
            if (m_botIconIdx.size() >= 500)
                m_botIconIdx.erase(m_botIconIdx.begin());
            m_botIconIdx[key] = QRandomGenerator::global()->bounded(2);
        }
        const QString svgPath = m_botIconIdx[key] == 0
            ? QStringLiteral(":/icons/mi-smart-toy.svg")
            : QStringLiteral(":/icons/mi-alien.svg");
        item->setIcon(MenuIcons::fromSvg(svgPath,
                                         QColor(m_theme.valid ? m_theme.accent : "#5588ff")));
    }
    item->setData(Qt::UserRole, e.nick);
    if (m_model->isIgnored(e.nick.toLower()))
        item->setData(Qt::UserRole + 1, QVariant::fromValue(MenuIcons::eyeOff()));
    {
        const NickMeta *meta = nullptr;
        if (sess) {
            auto it = sess->nickMeta.constFind(e.nick.toLower());
            if (it != sess->nickMeta.constEnd())
                meta = &it.value();
        }
        const bool hasAvatarImage = meta && !meta->avatarUrl.isEmpty()
                                    && m_avatarCache.contains(meta->avatarUrl);
        if (hasAvatarImage) {
            QByteArray pngBytes;
            QBuffer avatarBuf(&pngBytes);
            avatarBuf.open(QIODevice::WriteOnly);
            m_avatarCache[meta->avatarUrl].save(&avatarBuf, "PNG");
            const QString b64 = QString::fromLatin1(pngBytes.toBase64());
            QStringList lines;
            if (!meta->displayName.isEmpty())
                lines << QLatin1String("Name:") + meta->displayName.toHtmlEscaped();
            if (!e.account.isEmpty())
                lines << QLatin1String("Account: ") + e.account.toHtmlEscaped();
            item->setToolTip(
                QString("<html><body><table><tr>"
                        "<td><img src='data:image/png;base64,%1' width='32' height='32'></td>"
                        "<td style='padding-left:6px;vertical-align:middle'>%2</td>"
                        "</tr></table></body></html>")
                    .arg(b64, lines.join("<br>")));
        } else {
            QStringList tips;
            if (meta && !meta->displayName.isEmpty())
                tips << QLatin1String("Name:") + meta->displayName;
            if (!e.account.isEmpty())
                tips << QLatin1String("Account: ") + e.account;
            if (meta && !meta->avatarUrl.isEmpty())
                tips << QLatin1String("Avatar: ") + meta->avatarUrl;
            if (!tips.isEmpty())
                item->setToolTip(tips.join('\n'));
        }
    }
    if (m_config.ui.coloredNicks)
        item->setForeground(ChatRenderer::nickColor(e.nick));
    return item;
}

QString MainWindow::nickTooltip(const QString &nick, const ServerId &host) const
{
    const ServerSession *sess = const_cast<SessionModel *>(m_model)->session(host);
    const NickMeta *meta = nullptr;
    QString account;
    if (sess) {
        auto it = sess->nickMeta.constFind(nick.toLower());
        if (it != sess->nickMeta.constEnd()) meta = &it.value();
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

int MainWindow::findNickRow(QListWidget *list, const QString &nick)
{
    const QString lower = nick.toLower();
    for (int i = 0; i < list->count(); ++i)
        if (list->item(i)->data(Qt::UserRole).toString().toLower() == lower)
            return i;
    return -1;
}

void MainWindow::onNickAdded(ServerId host, BufferId channel, const QString &nick)
{
    auto *ch   = m_model->channel(host, channel);
    auto *sess = m_model->session(host);
    if (!ch) return;
    const qsizetype row = ch->nickIndex.value(nick.toLower(), -1);
    if (row < 0) return;
    const NickEntry &e = ch->nicks[row];

    const bool isActive = (host == m_model->activeHost() &&
                           channel.str().toLower() == m_model->activeChannel().str().toLower());
    if (isActive) {
        m_nickList->insertItem(static_cast<int>(row), makeNickItem(e, ch, sess));
        if (m_nickCountLabel) {
            const QString countStr = QString::number(ch->nicks.size());
            m_nickCountLabel->setText(countStr);
            m_nickCountLabel->setToolTip(countStr + " users");
        }
    }

    const QString key = paneKey(host, channel);
    if (auto *pane = m_panes.value(key)) {
        pane->nickList()->insertItem(static_cast<int>(row), makeNickItem(e, ch, sess));
        pane->setNickCount(static_cast<int>(ch->nicks.size()));
    }
}

void MainWindow::onNickRemoved(ServerId host, BufferId channel, const QString &nick)
{
    auto *ch = m_model->channel(host, channel);

    const bool isActive = (host == m_model->activeHost() &&
                           channel.str().toLower() == m_model->activeChannel().str().toLower());
    if (isActive) {
        const int row = findNickRow(m_nickList, nick);
        if (row >= 0) delete m_nickList->takeItem(row);
        if (m_nickCountLabel && ch) {
            const QString countStr = QString::number(ch->nicks.size());
            m_nickCountLabel->setText(countStr);
            m_nickCountLabel->setToolTip(countStr + " users");
        }
    }

    const QString key = paneKey(host, channel);
    if (auto *pane = m_panes.value(key)) {
        const int row = findNickRow(pane->nickList(), nick);
        if (row >= 0) delete pane->nickList()->takeItem(row);
        if (ch) pane->setNickCount(static_cast<int>(ch->nicks.size()));
    }

    const QString timerKey = paneKey(host, channel) + "|" + nick;
    if (auto *t = m_typingNickTimers.value(timerKey)) {
        t->stop();
        t->deleteLater();
        m_typingNickTimers.remove(timerKey);
        m_typingNicks[key].remove(nick);
        updateTypingLabel();
    }
}

void MainWindow::onNickRenamed(ServerId host, BufferId channel,
                                const QString &oldNick, const QString &newNick)
{
    auto *ch   = m_model->channel(host, channel);
    auto *sess = m_model->session(host);
    if (!ch) return;
    const qsizetype newRow = ch->nickIndex.value(newNick.toLower(), -1);
    if (newRow < 0) return;
    const NickEntry &e = ch->nicks[newRow];

    auto apply = [&](QListWidget *list) {
        const int oldRow = findNickRow(list, oldNick);
        if (oldRow < 0) return;
        delete list->takeItem(oldRow);
        list->insertItem(static_cast<int>(newRow), makeNickItem(e, ch, sess));
    };

    const bool isActive = (host == m_model->activeHost() &&
                           channel.str().toLower() == m_model->activeChannel().str().toLower());
    if (isActive) apply(m_nickList);

    const QString key = paneKey(host, channel);
    if (auto *pane = m_panes.value(key)) apply(pane->nickList());
}

void MainWindow::refreshNickList(ServerId host, BufferId channel)
{
    if (m_nickFilter) m_nickFilter->clear();
    m_nickList->clear();
    auto *ch   = m_model->channel(host, channel);
    if (!ch) return;
    auto *sess = m_model->session(host);

    for (const auto &e : std::as_const(ch->nicks))
        m_nickList->addItem(makeNickItem(e, ch, sess));

    if (m_nickCountLabel) {
        const QString countStr = QString::number(ch->nicks.size());
        m_nickCountLabel->setText(countStr);
        m_nickCountLabel->setToolTip(countStr + " users");
    }
}

void MainWindow::fetchAvatar(const QString &url)
{
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
        scheduleNickRefresh(m_model->activeHost(), m_model->activeChannel());
    };

    // Local file — load directly without network
    const QUrl qurl(url);
    if (qurl.isLocalFile()) {
        cacheAndRefresh(QPixmap(qurl.toLocalFile()));
        return;
    }
    if (url.startsWith('/')) {
        cacheAndRefresh(QPixmap(url));
        return;
    }

    if (!m_avatarNam)
        m_avatarNam = new QNetworkAccessManager(this);
    m_avatarFetching.insert(url);
    QNetworkRequest req{qurl};
    req.setRawHeader("User-Agent", "Uplink/" UPLINK_VERSION);
    auto *reply = m_avatarNam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, url, cacheAndRefresh] {
        reply->deleteLater();
        m_avatarFetching.remove(url);
        if (reply->error() != QNetworkReply::NoError)
            return;
        QPixmap px;
        if (px.loadFromData(reply->readAll()))
            cacheAndRefresh(px);
    });
}
