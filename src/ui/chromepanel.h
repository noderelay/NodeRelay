#pragma once
#include <QColor>
#include <QPainter>
#include <QPainterPath>
#include <QWidget>

// Gap between floating panel cards and the backdrop (bottom + inner edges) in
// panel-cards mode — matches the input strip's bottom margin and the
// window-edge margins on rightContent.
inline constexpr int kPanelGap = 8;

// Panel chrome surface that paints its own fill with rounded corners.
// Stylesheet background painting for plain QWidgets (app-wide QSS and even
// local per-widget sheets) is silently dropped in some Wayland/KDE sessions,
// so these surfaces own their paintEvent — QPainter fills cannot be lost by
// any styling machinery. Header strips keep square bottoms (the rounding of
// the path is pushed below the widget rect and clipped away); full-height
// cards round the bottom too via roundedBottom.
class ChromePanel : public QWidget {
public:
    explicit ChromePanel(QWidget *parent = nullptr) : QWidget(parent) {}

    void setFill(const QColor &c, bool rounded = true, bool roundedBottom = false)
    {
        m_fill          = c;
        m_rounded       = rounded;
        m_roundedBottom = roundedBottom;
        update();
    }

    // Stop the fill this many px inside the widget's top/bottom edges,
    // exposing the parent's backdrop. Pair with equal margins on the panel's
    // layout so children stop with the fill.
    void setTopInset(int px)
    {
        m_topInset = px;
        update();
    }

    void setBottomInset(int px)
    {
        m_bottomInset = px;
        update();
    }

    void setRightInset(int px)
    {
        m_rightInset = px;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        if (!m_fill.isValid()) return;
        QPainter p(this);
        p.setPen(Qt::NoPen);
        p.setBrush(m_fill);
        const QRectF r = QRectF(rect()).adjusted(0, m_topInset, -m_rightInset, -m_bottomInset);
        if (!m_rounded) {
            p.drawRect(r);
            return;
        }
        p.setRenderHint(QPainter::Antialiasing);
        QPainterPath path;
        path.addRoundedRect(r.adjusted(0, 0, 0, m_roundedBottom ? 0 : kRadius),
                            kRadius, kRadius);
        p.drawPath(path);
    }

private:
    QColor m_fill;
    bool   m_rounded{true};
    bool   m_roundedBottom{false};
    int    m_topInset{0};
    int    m_bottomInset{0};
    int    m_rightInset{0};
    static constexpr int kRadius = 10;
};
