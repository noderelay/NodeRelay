#pragma once

#include <QLabel>
#include <QResizeEvent>

// QLabel that elides when squeezed instead of enforcing its full text width
// as a layout minimum — a long header label otherwise pins the minimum width
// of its pane and the splitter handles stop moving. Text is swapped via
// QLabel::setText so the normal QSS-styled paint path still applies.
// Plain text only; set text through setFullText(), not setText().
class ElidedLabel : public QLabel {
public:
    explicit ElidedLabel(QWidget *parent = nullptr) : QLabel(parent) {}
    explicit ElidedLabel(const QString &text, QWidget *parent = nullptr)
        : QLabel(parent) { setFullText(text); }

    // updateGeometry() is load-bearing: elide() may leave the *displayed*
    // text unchanged (e.g. still elided-to-nothing at the current width),
    // and QLabel::setText only invalidates the layout when the shown text's
    // size changes — without it the layout keeps serving a stale cached
    // width hint (an empty-constructed label stays 0 px wide forever).
    void setFullText(const QString &text) { m_fullText = text; elide(); updateGeometry(); }
    QString fullText() const { return m_fullText; }

    QSize minimumSizeHint() const override
    {
        return {0, QLabel::minimumSizeHint().height()};
    }

    QSize sizeHint() const override
    {
        const QMargins m = contentsMargins();
        return {fontMetrics().horizontalAdvance(m_fullText) + m.left() + m.right(),
                QLabel::sizeHint().height()};
    }

protected:
    void resizeEvent(QResizeEvent *ev) override
    {
        QLabel::resizeEvent(ev);
        elide();
    }

    void changeEvent(QEvent *ev) override
    {
        QLabel::changeEvent(ev);
        if (ev->type() == QEvent::FontChange)
            elide();
    }

private:
    void elide()
    {
        // Explicit fits-check first: elidedText() can elide at exactly the
        // fitting width (its text engine may measure a hair wider than
        // horizontalAdvance), which clips the tail when the layout grants
        // precisely the size hint.
        const int w = contentsRect().width();
        const QString shown = fontMetrics().horizontalAdvance(m_fullText) <= w
            ? m_fullText
            : fontMetrics().elidedText(m_fullText, Qt::ElideRight, w);
        QLabel::setText(shown);
        setToolTip(shown == m_fullText ? QString() : m_fullText);
    }

    QString m_fullText;
};
