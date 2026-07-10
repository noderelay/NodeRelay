#include "searchbar.h"
#include "ui/chatview.h"
#include "ui/uistyle.h"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QToolButton>
#include <QKeyEvent>

SearchBar::SearchBar(ChatView *view, QWidget *parent)
    : QWidget(parent), m_view(view)
{
    setObjectName("searchBar");
    auto *hbox = new QHBoxLayout(this);
    hbox->setContentsMargins(4, 2, 4, 2);
    hbox->setSpacing(4);

    m_input = new QLineEdit;
    m_input->setPlaceholderText(QStringLiteral("Search in buffer…"));
    m_input->installEventFilter(this);
    connect(m_input, &QLineEdit::textChanged, this, [this](const QString &text){
        if (text.isEmpty()) m_view->clearFind();
        else m_view->findText(text, false);
    });

    auto *closeBtn = new QToolButton;
    closeBtn->setText(QStringLiteral("✕"));
    closeBtn->setFixedSize(22, 22);
    closeBtn->setAutoRaise(true);
    closeBtn->setStyleSheet(UiStyle::headerButtonStyle());
    connect(closeBtn, &QToolButton::clicked, this, &SearchBar::dismiss);

    hbox->addWidget(m_input, 1);
    hbox->addWidget(closeBtn);
    hide();
}

void SearchBar::open()
{
    show();
    m_input->setFocus();
    m_input->selectAll();
}

void SearchBar::dismiss()
{
    hide();
    m_input->clear();
    m_view->clearFind();
    emit dismissed();
}

bool SearchBar::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_input && event->type() == QEvent::KeyPress) {
        auto *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_Escape) {
            dismiss();
            return true;
        }
        if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
            const QString text = m_input->text();
            if (!text.isEmpty())
                m_view->findText(text, ke->modifiers() & Qt::ShiftModifier);
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}
