#pragma once

#include "model/channel.h"
#include "model/ids.h"
#include <QAbstractListModel>
#include <QColor>
#include <QHash>
#include <QPixmap>

class SessionModel;

// Shared look/context for nick-list models — owned by MainWindow, referenced
// by the main view's model and every pane's model.
struct NickListStyle {
    QColor accent{QStringLiteral("#5588ff")};
    bool   coloredNicks{true};
    QHash<QString, int>     *botIconIdx{nullptr};  // lowercased nick → 0 (robot) or 1 (alien)
    QHash<QString, QPixmap> *avatarCache{nullptr}; // avatar URL → scaled pixmap
};

// Virtualized replacement for the per-nick QListWidgetItem lists: the view
// only asks for rows it paints, so a 10k-nick channel costs one snapshot of
// implicitly-shared entries instead of 10k heap-allocated items rebuilt on
// every refresh. Tooltips (which embed base64 avatar PNGs) are built lazily
// on hover via ToolTipRole instead of eagerly per item.
class NickListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit NickListModel(SessionModel *model, const NickListStyle *style,
                           QObject *parent = nullptr);

    void setBuffer(ServerId host, BufferId channel); // retarget + snapshot
    void refresh();                                  // re-snapshot from the model
    void setFilter(const QString &prefix);           // case-insensitive startsWith

    int      rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &idx, int role) const override;

private:
    void     snapshot();
    QVariant tooltipFor(const NickEntry &e) const;

    SessionModel        *m_model;
    const NickListStyle *m_style;
    ServerId             m_host;
    BufferId             m_channel;
    QString              m_filter;
    QList<NickEntry>     m_nicks; // filter-applied snapshot in model (sorted) order
};
