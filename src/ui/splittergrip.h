#pragma once
#include <QSplitter>
#include <QMouseEvent>
#include <QCoreApplication>
#include <QWidget>

// Extra grab width a SplitterGrip adds beside its handle, in px.
inline constexpr int kGripExtra = 3;

// Transparent strip floating over a splitter handle that widens its
// effective grab area without widening the visible gap (in cards mode the
// handle IS the gap between the cards). Parented to the splitter's parent —
// a direct child of the QSplitter would be adopted as a pane by
// QSplitter::childEvent. Mouse events are forwarded to the real handle so
// drag semantics (minimums, collapsing) stay native.
//
// The extra width is one-sided on purpose, always growing toward the chat
// column: that keeps the hover zones flanking the input box mirrored
// (sidebar gap grows right, user-list gap grows left), and never reaches
// past a card gap into the far neighbor. The sidebar keeps its scrollbar
// flush on its right edge, so its gap must not grow leftward at all.
class SplitterGrip : public QWidget {
public:
    SplitterGrip(QSplitter *splitter, int handleIndex, int extraLeft, int extraRight)
        : QWidget(splitter->parentWidget()), m_splitter(splitter),
          m_index(handleIndex), m_extraLeft(extraLeft), m_extraRight(extraRight)
    {
        setCursor(splitter->orientation() == Qt::Horizontal ? Qt::SplitHCursor
                                                            : Qt::SplitVCursor);
        m_splitter->installEventFilter(this);
        if (QSplitterHandle *h = m_splitter->handle(m_index))
            h->installEventFilter(this);
        connect(m_splitter, &QSplitter::splitterMoved, this, [this]{ track(); });
        track();
    }

protected:
    bool eventFilter(QObject *obj, QEvent *event) override
    {
        Q_UNUSED(obj);
        switch (event->type()) {
        case QEvent::Resize:
        case QEvent::Move:
        case QEvent::Show:
        case QEvent::Hide:
            track();
            break;
        default:
            break;
        }
        return false;
    }
    void mousePressEvent(QMouseEvent *event) override   { forward(event); }
    void mouseMoveEvent(QMouseEvent *event) override    { forward(event); }
    void mouseReleaseEvent(QMouseEvent *event) override { forward(event); }

private:
    // Mirror the handle: its rect grown sideways, mapped into our parent,
    // shown only while the handle is (a collapsed user list hides it).
    void track()
    {
        QSplitterHandle *h = m_splitter->handle(m_index);
        if (!h) { hide(); return; }
        QRect r = h->geometry();
        if (m_splitter->orientation() == Qt::Horizontal)
            r.adjust(-m_extraLeft, 0, m_extraRight, 0);
        else
            r.adjust(0, -m_extraLeft, 0, m_extraRight);
        r.moveTopLeft(m_splitter->mapTo(parentWidget(), r.topLeft()));
        setGeometry(r);
        setVisible(m_splitter->isVisible() && h->isVisible());
        raise();
    }

    void forward(QMouseEvent *event)
    {
        QSplitterHandle *h = m_splitter->handle(m_index);
        if (!h) return;
        QMouseEvent fwd(event->type(),
                        h->mapFromGlobal(event->globalPosition()),
                        event->globalPosition(),
                        event->button(), event->buttons(), event->modifiers(),
                        event->pointingDevice());
        QCoreApplication::sendEvent(h, &fwd);
        event->accept();
    }

    QSplitter *m_splitter;
    int m_index;
    int m_extraLeft;
    int m_extraRight;
};
