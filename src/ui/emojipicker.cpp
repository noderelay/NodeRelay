#include "emojipicker.h"
#include "emojidata.h"

#include <QLineEdit>
#include <QScrollArea>
#include <QScrollBar>
#include <QWidget>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QToolButton>
#include <QApplication>
#include <QScreen>

static constexpr int kCols     = 9;
static constexpr int kBtnSize  = 34;
static constexpr int kGridGap  = 2;
static constexpr int kGridPad  = 2;
static constexpr int kOuterPad = 4;
static constexpr int kPickerH  = 340;

EmojiPicker::EmojiPicker(QWidget *parent)
    : QFrame(parent, Qt::Popup | Qt::FramelessWindowHint)
{
    setObjectName("emojiPicker");
    setFrameShape(QFrame::StyledPanel);

    auto *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(kOuterPad, kOuterPad, kOuterPad, kOuterPad);
    vbox->setSpacing(4);

    m_search = new QLineEdit;
    m_search->setPlaceholderText("Search emoji...");
    m_search->setClearButtonEnabled(true);
    vbox->addWidget(m_search);

    m_scroll = new QScrollArea;
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    vbox->addWidget(m_scroll);

    m_gridWidget = new QWidget;
    m_gridLayout = new QGridLayout(m_gridWidget);
    m_gridLayout->setSpacing(kGridGap);
    m_gridLayout->setContentsMargins(kGridPad, kGridPad, kGridPad, kGridPad);
    for (int c = 0; c < kCols; ++c)
        m_gridLayout->setColumnMinimumWidth(c, kBtnSize + kGridGap);
    m_gridLayout->setColumnStretch(kCols, 1);
    m_scroll->setWidget(m_gridWidget);

    const int gridW = kCols * (kBtnSize + kGridGap)
                    + (kCols - 1) * kGridGap + 2 * kGridPad;
    const int sbW = m_scroll->verticalScrollBar()->sizeHint().width();
    setFixedSize(gridW + sbW + 2 * kOuterPad, kPickerH);

    buildButtons();
    filterGrid({});

    connect(m_search, &QLineEdit::textChanged, this, &EmojiPicker::onSearch);
    connect(m_search, &QLineEdit::returnPressed, this, [this]{
        for (const auto &entry : std::as_const(m_buttons)) {
            if (entry.btn->isVisible()) {
                hide();
                emit emojiSelected(entry.btn->text());
                return;
            }
        }
    });
}

void EmojiPicker::buildButtons()
{
    QFont f = font();
    f.setPointSize(16);

    for (const auto &entry : kEmojiTable) {
        auto *btn = new QToolButton(m_gridWidget);
        btn->setText(entry.ch.toString());
        btn->setFont(f);
        btn->setFixedSize(kBtnSize, kBtnSize);
        btn->setAutoRaise(true);
        btn->setToolTip(entry.shortcode.toString());
        btn->setFocusPolicy(Qt::NoFocus);
        // Override global QToolButton styling (sidebar nav rules don't apply here)
        btn->setStyleSheet(
            "QToolButton { padding: 0; text-align: center;"
            "              background: transparent; border: none; }"
            "QToolButton:hover { background: rgba(128,128,128,0.2); }");

        connect(btn, &QToolButton::clicked, this, [this, ch = entry.ch.toString()]{
            hide();
            emit emojiSelected(ch);
        });

        m_buttons.append({btn, entry.shortcode.toString()});
    }
}

void EmojiPicker::filterGrid(const QString &filter)
{
    m_gridWidget->setUpdatesEnabled(false);

    for (int i = m_gridLayout->count() - 1; i >= 0; --i)
        delete m_gridLayout->takeAt(i);

    int col = 0, row = 0;
    for (auto &entry : m_buttons) {
        const bool match = filter.isEmpty() ||
                           entry.shortcode.contains(filter, Qt::CaseInsensitive);
        if (match) {
            m_gridLayout->addWidget(entry.btn, row, col);
            entry.btn->show();
            if (++col >= kCols) { col = 0; ++row; }
        } else {
            entry.btn->hide();
        }
    }

    m_gridWidget->adjustSize();
    m_gridWidget->setUpdatesEnabled(true);
}

void EmojiPicker::onSearch(const QString &text)
{
    QString f = text.trimmed();
    if (f.startsWith(':')) f = f.mid(1);
    if (f.endsWith(':'))   f.chop(1);
    filterGrid(f);
    m_scroll->verticalScrollBar()->setValue(0);
}

void EmojiPicker::showAt(const QPoint &globalAnchor)
{
    m_search->clear();
    filterGrid({});
    m_scroll->verticalScrollBar()->setValue(0);

    // Position: prefer above-left of anchor; clamp to screen
    QPoint pos(globalAnchor.x() - width(), globalAnchor.y() - height());

    const QRect screen = QApplication::primaryScreen()->availableGeometry();
    pos.rx() = qBound(screen.left(), pos.x(), screen.right()  - width());
    pos.ry() = qBound(screen.top(),  pos.y(), screen.bottom() - height());

    move(pos);
    show();
    raise();
    m_search->setFocus();
}
