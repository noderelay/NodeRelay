#include "ui/nicklistmodel.h"
#include "ui/chatrenderer.h"
#include "ui/menuicons.h"
#include "model/sessionmodel.h"

#include <QBuffer>

NickListModel::NickListModel(SessionModel *model, const NickListStyle *style,
                             QObject *parent)
    : QAbstractListModel(parent), m_model(model), m_style(style)
{}

void NickListModel::setBuffer(const ServerId &host, const BufferId &channel)
{
    m_host    = host;
    m_channel = channel;
    m_filter.clear();
    refresh();
}

void NickListModel::refresh()
{
    beginResetModel();
    snapshot();
    endResetModel();
}

void NickListModel::setFilter(const QString &prefix)
{
    if (m_filter == prefix) return;
    m_filter = prefix;
    refresh();
}

void NickListModel::snapshot()
{
    m_nicks.clear();
    const Channel *ch = m_model->channel(m_host, m_channel);
    if (!ch) return;
    if (m_filter.isEmpty()) {
        m_nicks = ch->nicks; // implicit sharing — no per-entry copy
        return;
    }
    for (const NickEntry &e : ch->nicks)
        if (e.nick.startsWith(m_filter, Qt::CaseInsensitive))
            m_nicks.append(e);
}

int NickListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_nicks.size());
}

QVariant NickListModel::data(const QModelIndex &idx, int role) const
{
    const int row = idx.row();
    if (row < 0 || row >= m_nicks.size()) return {};
    const NickEntry &e = m_nicks[row];

    switch (role) {
    case Qt::DisplayRole:
        return e.display();
    case Qt::UserRole:
        return e.nick;
    case Qt::ForegroundRole:
        if (m_style->coloredNicks)
            return ChatRenderer::nickColor(e.nick);
        return {};
    case Qt::UserRole + 1: // ignored marker, painted by NickDelegate
        if (m_model->isIgnored(e.nick.toLower()))
            return QVariant::fromValue(MenuIcons::eyeOff());
        return {};
    case Qt::DecorationRole: {
        const QString lower = e.nick.toLower();
        const Channel       *ch   = m_model->channel(m_host, m_channel);
        const ServerSession *sess = m_model->session(m_host);
        const bool isBot = (ch && ch->botNicks.contains(lower))
                        || (sess && sess->botNicks.contains(lower));
        if (!isBot) return {};
        return QVariant::fromValue(
            MenuIcons::fromSvg(QStringLiteral(":/icons/mi-smart-toy.svg"), m_style->accent));
    }
    case Qt::ToolTipRole:
        return tooltipFor(e);
    default:
        return {};
    }
}

QVariant NickListModel::tooltipFor(const NickEntry &e) const
{
    const ServerSession *sess = m_model->session(m_host);
    const NickMeta *meta = nullptr;
    if (sess) {
        const auto it = sess->nickMeta.constFind(e.nick.toLower());
        if (it != sess->nickMeta.constEnd())
            meta = &it.value();
    }
    if (!meta)  // fetched on first hover, not roster-wide; shows on next hover
        m_model->requestNickMeta(m_host, e.nick);
    const bool hasAvatarImage = meta && !meta->avatarUrl.isEmpty()
                                && m_style->avatarCache->contains(meta->avatarUrl);
    if (hasAvatarImage) {
        QByteArray pngBytes;
        QBuffer avatarBuf(&pngBytes);
        avatarBuf.open(QIODevice::WriteOnly);
        m_style->avatarCache->value(meta->avatarUrl).save(&avatarBuf, "PNG");
        const QString b64 = QString::fromLatin1(pngBytes.toBase64());
        QStringList lines;
        if (!meta->displayName.isEmpty())
            lines << QLatin1String("Name:") + meta->displayName.toHtmlEscaped();
        if (!e.account.isEmpty())
            lines << QLatin1String("Account: ") + e.account.toHtmlEscaped();
        return QString("<html><body><table><tr>"
                       "<td><img src='data:image/png;base64,%1' width='32' height='32'></td>"
                       "<td style='padding-left:6px;vertical-align:middle'>%2</td>"
                       "</tr></table></body></html>")
                   .arg(b64, lines.join("<br>"));
    }
    QStringList tips;
    if (meta && !meta->displayName.isEmpty())
        tips << QLatin1String("Name:") + meta->displayName;
    if (!e.account.isEmpty())
        tips << QLatin1String("Account: ") + e.account;
    if (meta && !meta->avatarUrl.isEmpty())
        tips << QLatin1String("Avatar: ") + meta->avatarUrl;
    if (tips.isEmpty()) return {};
    return tips.join('\n');
}
