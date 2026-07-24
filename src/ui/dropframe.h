#pragma once
#include <QWidget>
#include <QPainter>
#include <QPen>

// Transparent overlay that draws a highlight frame around its parent while a
// pane drag hovers over it, marking the whole widget as the drop target.
// Painted as an overlay (not a stylesheet border) so showing it never
// repolishes or reflows the widget underneath.
class DropFrame : public QWidget {
public:
    explicit DropFrame(QWidget *parent) : QWidget(parent) {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        // The app stylesheet has a generic `QWidget { background: ... }` rule
        // that would paint this overlay opaque, blanking the pane under it.
        setAttribute(Qt::WA_NoSystemBackground);
        setStyleSheet(QStringLiteral("background: transparent;"));
        hide();
    }

    void activate() { activate(parentWidget()->rect()); }

    // Cover only part of the parent — an edge band, when the drop would place
    // the dragged pane on that side rather than swap with it.
    void activate(const QRect &area) {
        setGeometry(area);
        raise();
        show();
        update(); // geometry may be unchanged while the highlighted side isn't
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setPen(QPen(palette().color(QPalette::Highlight), kFrameWidth));
        p.setBrush(Qt::NoBrush);
        const qreal half = kFrameWidth / 2.0;
        p.drawRect(QRectF(rect()).adjusted(half, half, -half, -half));
    }

private:
    static constexpr int kFrameWidth = 3;
};

// Overlay that stands in for a pane while it is being dragged: the pane's
// old area is filled with the theme's alternate colour (sidebarBg via
// QPalette::AlternateBase) plus a dashed accent frame, so the slot reads as
// vacated until the drag is dropped or cancelled.
class DragPlaceholder : public QWidget {
public:
    explicit DragPlaceholder(QWidget *parent) : QWidget(parent) {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
        setStyleSheet(QStringLiteral("background: transparent;"));
        hide();
    }

    void activate() {
        setGeometry(parentWidget()->rect());
        raise();
        show();
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.fillRect(rect(), palette().color(QPalette::AlternateBase));
        QColor frame = palette().color(QPalette::Highlight);
        frame.setAlphaF(0.45f);
        QPen pen(frame, kFrameWidth);
        pen.setStyle(Qt::DashLine);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        const qreal half = kFrameWidth / 2.0;
        p.drawRect(QRectF(rect()).adjusted(half, half, -half, -half));
    }

private:
    static constexpr int kFrameWidth = 2;
};
