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

    void setFullText(const QString &text) { m_fullText = text; elide(); }
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
        const QString shown = fontMetrics().elidedText(
            m_fullText, Qt::ElideRight, contentsRect().width());
        QLabel::setText(shown);
        setToolTip(shown == m_fullText ? QString() : m_fullText);
    }

    QString m_fullText;
};
