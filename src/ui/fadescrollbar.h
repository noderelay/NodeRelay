#pragma once
#include <QScrollBar>

class QAbstractScrollArea;
class QTimer;
class QPropertyAnimation;
class QGraphicsOpacityEffect;

class FadeScrollBar : public QScrollBar
{
    Q_OBJECT
public:
    explicit FadeScrollBar(Qt::Orientation orientation, QWidget *parent = nullptr);

    // Floats a FadeScrollBar over the right edge of `area` instead of
    // letting it reserve a layout column (the empty gutter reads as a gap
    // once the bar has faded out). The area's own scrollbar is hidden and
    // kept in sync bidirectionally; wheel/kinetic scrolling is unaffected.
    static FadeScrollBar *attachOverlay(QAbstractScrollArea *area);

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void wake();
    void scheduleFade();
    void repositionOverlay();

    QGraphicsOpacityEffect *m_effect;
    QTimer                 *m_hideTimer;
    QPropertyAnimation     *m_anim;
    QAbstractScrollArea    *m_overlayArea{nullptr};
};
