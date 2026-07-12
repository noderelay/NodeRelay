#pragma once
#include <QColor>
#include <QPainter>
#include <QPainterPath>
#include <QWidget>

// Panel chrome surface that paints its own fill with a rounded top.
// Stylesheet background painting for plain QWidgets (app-wide QSS and even
// local per-widget sheets) is silently dropped in some Wayland/KDE sessions,
// so these surfaces own their paintEvent — QPainter fills cannot be lost by
// any styling machinery. Bottom corners stay square: the rounding of the
// path is pushed below the widget rect and clipped away.
class ChromePanel : public QWidget {
public:
    explicit ChromePanel(QWidget *parent = nullptr) : QWidget(parent) {}

    void setFill(const QColor &c, bool rounded = true)
    {
        m_fill    = c;
        m_rounded = rounded;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        if (!m_fill.isValid()) return;
        QPainter p(this);
        p.setPen(Qt::NoPen);
        p.setBrush(m_fill);
        if (!m_rounded) {
            p.drawRect(rect());
            return;
        }
        p.setRenderHint(QPainter::Antialiasing);
        QPainterPath path;
        path.addRoundedRect(QRectF(rect()).adjusted(0, 0, 0, kRadius), kRadius, kRadius);
        p.drawPath(path);
    }

private:
    QColor m_fill;
    bool   m_rounded{true};
    static constexpr int kRadius = 10;
};
