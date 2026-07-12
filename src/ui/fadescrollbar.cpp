#include "fadescrollbar.h"

#include <QAbstractScrollArea>
#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QEnterEvent>

static constexpr int   kHoldMs   = 3500;
static constexpr int   kFadeMs   = 300;
static constexpr qreal kVisible  = 0.85;
static constexpr int   kOverlayW = 10;

FadeScrollBar::FadeScrollBar(Qt::Orientation orientation, QWidget *parent)
    : QScrollBar(orientation, parent)
{
    m_effect = new QGraphicsOpacityEffect(this);
    m_effect->setOpacity(0.0);
    setGraphicsEffect(m_effect);

    m_hideTimer = new QTimer(this);
    m_hideTimer->setSingleShot(true);
    m_hideTimer->setInterval(kHoldMs);

    m_anim = new QPropertyAnimation(m_effect, "opacity", this);
    m_anim->setDuration(kFadeMs);
    m_anim->setEasingCurve(QEasingCurve::OutCubic);

    connect(m_hideTimer, &QTimer::timeout, this, [this] {
        if (underMouse()) return;
        m_anim->stop();
        m_anim->setStartValue(m_effect->opacity());
        m_anim->setEndValue(0.0);
        m_anim->start();
    });

    connect(this, &QScrollBar::valueChanged,   this, [this](int) { wake(); });
    connect(this, &QScrollBar::sliderPressed,  this, [this]      { wake(); });
    connect(this, &QScrollBar::sliderReleased, this, [this]      { wake(); });
}

void FadeScrollBar::wake()
{
    m_anim->stop();
    m_effect->setOpacity(kVisible);
    scheduleFade();
}

void FadeScrollBar::scheduleFade()
{
    m_hideTimer->start();
}

void FadeScrollBar::enterEvent(QEnterEvent *event)
{
    QScrollBar::enterEvent(event);
    m_anim->stop();
    m_effect->setOpacity(kVisible);
    m_hideTimer->stop();
}

void FadeScrollBar::leaveEvent(QEvent *event)
{
    QScrollBar::leaveEvent(event);
    if (m_effect->opacity() >= kVisible)
        scheduleFade();
}

FadeScrollBar *FadeScrollBar::attachOverlay(QAbstractScrollArea *area)
{
    area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QScrollBar *src = area->verticalScrollBar();   // hidden, still drives the view

    auto *bar = new FadeScrollBar(Qt::Vertical, area);
    bar->m_overlayArea = area;
    bar->setFixedWidth(kOverlayW);

    auto syncFromSrc = [bar, src]{
        bar->setRange(src->minimum(), src->maximum());
        bar->setPageStep(src->pageStep());
        bar->setSingleStep(src->singleStep());
        if (bar->value() != src->value())
            bar->setValue(src->value());
        bar->setVisible(src->maximum() > src->minimum());
    };
    // Deliberately no QSignalBlocker: the wake-on-scroll behavior hangs off
    // valueChanged, and setValue is self-terminating in both directions
    // (valueChanged only fires on an actual change).
    connect(src, &QAbstractSlider::rangeChanged, bar, syncFromSrc);
    connect(src, &QAbstractSlider::valueChanged, bar, syncFromSrc);
    connect(bar, &QAbstractSlider::valueChanged, src, &QAbstractSlider::setValue);

    area->installEventFilter(bar);
    syncFromSrc();
    bar->repositionOverlay();
    return bar;
}

bool FadeScrollBar::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_overlayArea
        && (event->type() == QEvent::Resize || event->type() == QEvent::Show))
        repositionOverlay();
    return QScrollBar::eventFilter(obj, event);
}

void FadeScrollBar::repositionOverlay()
{
    if (!m_overlayArea) return;
    const int fw = m_overlayArea->frameWidth();
    setGeometry(m_overlayArea->width() - width() - fw,
                fw,
                width(),
                m_overlayArea->height() - fw * 2);
    raise();
}
