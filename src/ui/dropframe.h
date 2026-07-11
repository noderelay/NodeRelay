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

    void activate() {
        setGeometry(parentWidget()->rect());
        raise();
        show();
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
