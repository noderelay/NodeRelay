#pragma once

// MainWindow's private item delegates and container widgets, shared between
// mainwindow.cpp and mainwindow_setup.cpp.

#include <QApplication>
#include <QColor>
#include <QFontMetrics>
#include <QIcon>
#include <QPainter>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QWidget>

class NickDelegate : public QStyledItemDelegate {
    QColor  m_accent;
    QColor  m_hover;
    QColor  m_activeText;
    QString m_selfNick;
    bool    m_selfAway{false};
public:
    explicit NickDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent) {}

    void setColors(const QColor &accent, const QColor &hover, const QColor &activeText) {
        m_accent     = accent;
        m_hover      = hover;
        m_activeText = activeText;
    }

    void setSelfAway(const QString &nick, bool away) {
        m_selfNick = nick;
        m_selfAway = away;
    }

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &) const override
    {
        return QSize(option.rect.width(), qMax(16, option.fontMetrics.height() + 2));
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        const QIcon icon       = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        const QIcon ignoreIcon = qvariant_cast<QIcon>(index.data(Qt::UserRole + 1));
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);
        opt.icon = QIcon();
        opt.decorationSize = QSize(0, 0);

        const bool selected = opt.state & QStyle::State_Selected;
        const bool hovered  = opt.state & QStyle::State_MouseOver;

        const QWidget *w    = opt.widget;
        const QStyle  *s    = w ? w->style() : QApplication::style();
        const QRect textRect = s->subElementRect(QStyle::SE_ItemViewItemText, &opt, w);
        const QFontMetrics fm(opt.font);
        const int textW      = fm.horizontalAdvance(opt.text);
        const int textMargin = s->pixelMetric(QStyle::PM_FocusFrameHMargin, &opt, w) + 1;
        constexpr int hPad   = 8;
        constexpr int vPad   = 1;
        constexpr int iconSz = 14;
        constexpr int iconGap = 2;
        const int pillX      = textRect.x() + textMargin - hPad;
        const int iconExtra  = (icon.isNull()       ? 0 : (iconGap + iconSz))
                             + (ignoreIcon.isNull()  ? 0 : (iconGap + iconSz));

        const bool isSelfAway = m_selfAway && !m_selfNick.isEmpty()
            && index.data(Qt::UserRole).toString().compare(m_selfNick, Qt::CaseInsensitive) == 0;

        if (isSelfAway) painter->save(), painter->setOpacity(0.35);

        if (selected || hovered) {
            const QColor bg = selected ? m_accent : m_hover;
            if (bg.isValid()) {
                QRect r(pillX,
                        opt.rect.y() + vPad,
                        qMin(textW + hPad * 2 + iconExtra, opt.rect.right() - pillX),
                        opt.rect.height() - vPad * 2);
                painter->save();
                painter->setRenderHint(QPainter::Antialiasing);
                painter->setPen(Qt::NoPen);
                painter->setBrush(bg);
                painter->drawRoundedRect(r, 6.0, 6.0);
                painter->restore();
            }
        }

        opt.state &= ~(QStyle::State_HasFocus | QStyle::State_MouseOver);
        if (selected && m_accent.isValid()) {
            const QColor textCol = m_accent.lightnessF() > 0.5
                ? QColor("#111111") : QColor("#eeeeee");
            opt.palette.setColor(QPalette::All, QPalette::Highlight,       QColor(Qt::transparent));
            opt.palette.setColor(QPalette::All, QPalette::HighlightedText, textCol);
        }
        QStyledItemDelegate::paint(painter, opt, index);

        int afterIconX = textRect.x() + textMargin + textW;
        if (!icon.isNull()) {
            afterIconX += iconGap;
            QRect r(afterIconX, opt.rect.top() + (opt.rect.height() - iconSz) / 2, iconSz, iconSz);
            icon.paint(painter, r);
            afterIconX += iconSz;
        }
        if (!ignoreIcon.isNull()) {
            afterIconX += iconGap;
            QRect r(afterIconX, opt.rect.top() + (opt.rect.height() - iconSz) / 2, iconSz, iconSz);
            ignoreIcon.paint(painter, r);
        }

        if (isSelfAway) painter->restore();
    }
};

class SidebarDelegate : public QStyledItemDelegate {
    QColor m_accent;
    QColor m_hover;
    QColor m_activeText;
    QColor m_unreadColor;
    bool   m_showCounts{true};
public:
    explicit SidebarDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent) {}

    void setColors(const QColor &accent, const QColor &hover, const QColor &activeText,
                   const QColor &unreadColor = {}) {
        m_accent      = accent;
        m_hover       = hover;
        m_activeText  = activeText;
        m_unreadColor = unreadColor;
    }

    void setShowCounts(bool show) { m_showCounts = show; }

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override
    {
        QSize s = QStyledItemDelegate::sizeHint(option, index);
        s.setHeight(qMax(26, option.fontMetrics.height() + 10));
        return s;
    }


    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        const QIcon indicator  = qvariant_cast<QIcon>(index.data(Qt::UserRole + 2));
        const int   unreadCnt  = m_showCounts ? index.data(Qt::UserRole + 3).toInt() : 0;
        const QString countStr = unreadCnt > 0 ? QString::number(unreadCnt) : QString();
        const QIcon awayIcon   = qvariant_cast<QIcon>(index.data(Qt::UserRole + 4));
        const QIcon avatar     = qvariant_cast<QIcon>(index.data(Qt::UserRole + 5));
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);
        opt.icon = QIcon();
        opt.decorationSize = QSize(0, 0);
        // Channel avatar is painted manually at a position derived only from
        // the row rect — never via the style's decoration layout, which
        // shifts on selection under fractional scaling. Text keeps the plain
        // no-decoration layout, inset by a constant.
        constexpr int avatarSz  = 16;
        constexpr int avatarGap = 4;
        QRect avatarRect;
        if (!avatar.isNull()) {
            avatarRect = QRect(opt.rect.x() + avatarGap,
                               opt.rect.y() + (opt.rect.height() - avatarSz) / 2,
                               avatarSz, avatarSz);
            opt.rect.setLeft(opt.rect.left() + avatarGap + avatarSz + avatarGap);
        }

        const bool selected = opt.state & QStyle::State_Selected;
        const bool hovered  = opt.state & QStyle::State_MouseOver;

        const QWidget *w    = opt.widget;
        const QStyle  *s    = w ? w->style() : QApplication::style();
        const QRect textRect = s->subElementRect(QStyle::SE_ItemViewItemText, &opt, w);
        const QFontMetrics fm(opt.font);
        QFont countFont = opt.font;
        countFont.setPointSizeF(opt.font.pointSizeF() * 0.86);
        countFont.setBold(true);
        const QFontMetrics cfm(countFont);
        const int textW      = fm.horizontalAdvance(opt.text);
        const int textMargin = s->pixelMetric(QStyle::PM_FocusFrameHMargin, &opt, w) + 1;
        constexpr int hPad   = 8;
        constexpr int vPad   = 2;
        constexpr int iconSz = 14;
        constexpr int iconGap = 2;
        constexpr int countGap = 3;
        const int pillX      = textRect.x() + textMargin - hPad;
        const int iconExtra  = indicator.isNull() ? 0 : (iconGap + iconSz);
        const int countExtra = countStr.isEmpty() ? 0 : (countGap + cfm.horizontalAdvance(countStr));
        const int awayExtra  = awayIcon.isNull()  ? 0 : (iconGap + iconSz);

        if (selected || hovered) {
            const QColor bg = selected ? m_accent : m_hover;
            if (bg.isValid()) {
                QRect r(pillX,
                        opt.rect.y() + vPad,
                        qMin(textW + hPad * 2 + iconExtra + countExtra + awayExtra, opt.rect.right() - pillX),
                        opt.rect.height() - vPad * 2);
                painter->save();
                painter->setRenderHint(QPainter::Antialiasing);
                painter->setPen(Qt::NoPen);
                painter->setBrush(bg);
                painter->drawRoundedRect(r, 10.0, 10.0);
                painter->restore();
            }
        }

        opt.state &= ~(QStyle::State_HasFocus | QStyle::State_MouseOver);
        if (selected && m_accent.isValid()) {
            const QColor textCol = m_accent.lightnessF() > 0.5
                ? QColor("#111111") : QColor("#eeeeee");
            opt.palette.setColor(QPalette::All, QPalette::Highlight,       QColor(Qt::transparent));
            opt.palette.setColor(QPalette::All, QPalette::HighlightedText, textCol);
        }
        QStyledItemDelegate::paint(painter, opt, index);

        // Always QIcon::Normal — the style's Selected-mode pixmap generation
        // is another thing that behaves differently under fractional scaling
        if (!avatar.isNull())
            avatar.paint(painter, avatarRect, Qt::AlignCenter, QIcon::Normal, QIcon::Off);

        int afterTextX = textRect.x() + textMargin + textW;
        if (!indicator.isNull()) {
            const int iconX = afterTextX + iconGap;
            QRect r(iconX,
                    opt.rect.top() + (opt.rect.height() - iconSz) / 2,
                    iconSz, iconSz);
            indicator.paint(painter, r);
            afterTextX = iconX + iconSz;
        }
        if (!countStr.isEmpty()) {
            const QRect cr(afterTextX + countGap,
                           opt.rect.top(),
                           cfm.horizontalAdvance(countStr) + 2,
                           opt.rect.height());
            painter->save();
            painter->setFont(countFont);
            const QColor cc = m_unreadColor.isValid() ? m_unreadColor
                                                       : opt.palette.color(QPalette::Text);
            painter->setPen(cc);
            painter->drawText(cr, Qt::AlignLeft | Qt::AlignVCenter, countStr);
            painter->restore();
            afterTextX += countGap + cfm.horizontalAdvance(countStr) + 2;
        }
        if (!awayIcon.isNull()) {
            const int iconX = afterTextX + iconGap;
            QRect r(iconX,
                    opt.rect.top() + (opt.rect.height() - iconSz) / 2,
                    iconSz, iconSz);
            awayIcon.paint(painter, r);
        }
    }
};

// Container for the chat area. Historically clipped its children to a
// rounded rect with a widget mask — do NOT reintroduce that: widget masks
// mis-scale under fractional display scaling on Wayland (KDE 145% clips a
// corner-radius-sized band off the bottom of the content; a 1x-DPR QBitmap
// mask, a logical-coords QRegion mask, and a mask extended past the bottom
// edge ALL exhibited it on a live session). If rounded corners come back,
// paint them as overlay nibs, not as a mask.
class RoundedPane : public QWidget {
public:
    explicit RoundedPane(QWidget *parent = nullptr) : QWidget(parent) {}
};
