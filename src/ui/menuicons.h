#pragma once
#include <QHash>
#include <QIcon>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>
#include <cmath>
#include <QApplication>
#include <QScreen>
#include <QPalette>
#include <QSvgRenderer>
#include <QStringBuilder>

// Material Symbols Outlined icons, colorized to the current palette at runtime.

namespace MenuIcons {

// logicalSize: the icon size in logical pixels; rendered at screen DPR for crisp HiDPI output.
inline QIcon fromSvg(const QString &path, const QColor &color = {}, int logicalSize = 16)
{
    const QColor col = color.isValid() ? color
                                       : QApplication::palette().color(QPalette::WindowText);
    const qreal dpr = (QApplication::primaryScreen())
                          ? qMax(1.0, QApplication::primaryScreen()->devicePixelRatio())
                          : 1.0;

    // Cache rendered icons — SVG render + QPixmap alloc is expensive per-call.
    // Key: path|aarrggbb|size|dpr*100 — all distinct combinations in the app fit in ~30 entries.
    static QHash<QString, QIcon> s_cache;
    const QString cacheKey = path
        % QLatin1Char('|') % QString::number(col.rgba(), 16)
        % QLatin1Char('|') % QString::number(logicalSize)
        % QLatin1Char('|') % QString::number(qRound(dpr * 100));
    auto it = s_cache.constFind(cacheKey);
    if (it != s_cache.constEnd())
        return *it;

    QSvgRenderer renderer(path);
    const int phys = qRound(logicalSize * dpr);
    QPixmap pix(phys, phys);
    pix.fill(Qt::transparent);
    {
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing);
        renderer.render(&p);
        p.setCompositionMode(QPainter::CompositionMode_SourceIn);
        p.fillRect(pix.rect(), col);
    }
    pix.setDevicePixelRatio(dpr);
    return s_cache.emplace(cacheKey, QIcon(pix)).value();
}

inline QIcon about          (const QColor &c = {}) { return fromSvg(":/icons/mi-info.svg",               c); }
inline QIcon documentation  (const QColor &c = {}) { return fromSvg(":/icons/mi-menu-book.svg",          c); }
inline QIcon servers        (const QColor &c = {}) { return fromSvg(":/icons/mi-dns.svg",                c); }
inline QIcon fontConfig     (const QColor &c = {}) { return fromSvg(":/icons/mi-font-download.svg",      c); }
inline QIcon theme          (const QColor &c = {}) { return fromSvg(":/icons/mi-palette.svg",            c); }
inline QIcon pipExit        (const QColor &c = {}) { return fromSvg(":/icons/mi-pip-exit.svg",          c); }
inline QIcon pipEnter       (const QColor &c = {}) { return fromSvg(":/icons/mi-pip.svg",               c); }
inline QIcon coloredNicks   (const QColor &c = {}) { return fromSvg(":/icons/mi-format-color-text.svg",  c); }
inline QIcon checkForUpdates(const QColor &c = {}) { return fromSvg(":/icons/mi-new-releases.svg",       c); }
inline QIcon exit           (const QColor &c = {}) { return fromSvg(":/icons/mi-logout.svg",             c); }
inline QIcon preferences    (const QColor &c = {}) { return fromSvg(":/icons/mi-tune.svg",               c); }
inline QIcon eye            (const QColor &c = {}) { return fromSvg(":/icons/mi-visibility.svg",          c); }
inline QIcon eyeOff         (const QColor &c = {}) { return fromSvg(":/icons/mi-visibility-off.svg",      c); }
inline QIcon confirm        (const QColor &c = {}) { return fromSvg(":/icons/mi-check.svg",               c); }
inline QIcon close          (const QColor &c = {}) { return fromSvg(":/icons/mi-close.svg",               c); }
inline QIcon mention        (const QColor &c = {}) { return fromSvg(":/icons/mi-bolt.svg",              c); }
inline QIcon unread         (const QColor &c = {}) { return fromSvg(":/icons/mi-forum.svg",               c); }
inline QIcon send           (const QColor &c = {}, int sz = 16) { return fromSvg(":/icons/mi-send.svg", c, sz); }
inline QIcon connectedServer(const QColor &c = {}) { return fromSvg(":/icons/mi-host.svg",                c); }
inline QIcon manageServers  (const QColor &c = {}) { return fromSvg(":/icons/mi-add-link.svg",           c, 24); }
inline QIcon gear           (const QColor &c = {}) { return fromSvg(":/icons/mi-manage-accounts.svg",     c, 24); }

inline QIcon scripts        (const QColor &c = {}) { return fromSvg(":/icons/mi-lightbulb-2.svg",        c); }
inline QIcon plugins        (const QColor &c = {}) { return fromSvg(":/icons/mi-extension.svg",          c); }
inline QIcon deleteIcon     (const QColor &c = {}) { return fromSvg(":/icons/mi-delete.svg",            c); }
inline QIcon cut            (const QColor &c = {}) { return fromSvg(":/icons/mi-content-cut.svg",        c); }
inline QIcon copy           (const QColor &c = {}) { return fromSvg(":/icons/mi-content-copy.svg",       c); }
inline QIcon paste          (const QColor &c = {}) { return fromSvg(":/icons/mi-content-paste.svg",      c); }
inline QIcon keyboard       (const QColor &c = {}) { return fromSvg(":/icons/mi-keyboard.svg",           c); }
inline QIcon openConfig     (const QColor &c = {}) { return fromSvg(":/icons/mi-file-open.svg",          c); }
inline QIcon reloadConfig   (const QColor &c = {}) { return fromSvg(":/icons/mi-restart-alt.svg",        c); }
inline QIcon clearBuffer    (const QColor &c = {}) { return fromSvg(":/icons/mi-clear-all.svg",          c); }

// Speech-bubble topic icon, drawn with painter primitives (no SVG asset).
inline QIcon topicBubble(const QColor &color)
{
    const int sz = 14;
    QPixmap pix(sz, sz);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    QPen pen(color, 1.3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    // Speech bubble body
    p.drawRoundedRect(QRectF(1.0, 1.0, 11.0, 8.5), 2.0, 2.0);
    // Tail pointing bottom-left
    QPolygonF tail;
    tail << QPointF(2.5, 9.5) << QPointF(1.0, 13.0) << QPointF(5.5, 9.5);
    p.drawPolyline(tail);
    // Two text lines inside the bubble
    p.setPen(QPen(color, 1.0, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(QPointF(3.5, 4.0), QPointF(9.5, 4.0));
    p.drawLine(QPointF(3.5, 6.5), QPointF(7.5, 6.5));
    return QIcon(pix);
}

// Closed-hand "grabbing" cursor pixmap for pane drags. Drawn in code: the
// icon set has no hand glyph, and a cursor needs the classic white-fill /
// dark-outline look the single-color tint pass can't produce. DPR is rounded
// up to an integer — Wayland cursor buffers only take integer scales, so a
// fractional-scale pixmap would render blurry.
inline QPixmap grabCursor(int logicalSize = 24)
{
    const qreal dpr = (QApplication::primaryScreen())
                          ? qMax(1.0, std::ceil(QApplication::primaryScreen()->devicePixelRatio()))
                          : 1.0;
    const int phys = qRound(logicalSize * dpr);
    QPixmap pix(phys, phys);
    pix.fill(Qt::transparent);
    {
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing);
        p.scale(phys / 24.0, phys / 24.0);
        // Back of a closed fist: palm with four knuckle bumps.
        QPainterPath hand;
        hand.setFillRule(Qt::WindingFill);
        hand.addRoundedRect(QRectF(4.5, 9.5, 15.0, 9.0), 4.0, 4.0);
        hand.addRoundedRect(QRectF(4.9, 7.0, 3.2, 6.0), 1.6, 1.6);
        hand.addRoundedRect(QRectF(8.5, 6.0, 3.2, 6.0), 1.6, 1.6);
        hand.addRoundedRect(QRectF(12.1, 6.2, 3.2, 6.0), 1.6, 1.6);
        hand.addRoundedRect(QRectF(15.7, 7.2, 3.2, 6.0), 1.6, 1.6);
        p.setPen(QPen(QColor(45, 45, 45), 1.3));
        p.setBrush(Qt::white);
        p.drawPath(hand.simplified());
    }
    pix.setDevicePixelRatio(dpr);
    return pix;
}

// Groups glyph as a QPixmap (for QLabel::setPixmap).
inline QPixmap groups(const QColor &color, int size = 16)
{
    QSvgRenderer renderer(QStringLiteral(":/icons/mi-groups.svg"));
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    renderer.render(&p);
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.fillRect(pix.rect(), color);
    p.end();
    return pix;
}

} // namespace MenuIcons
