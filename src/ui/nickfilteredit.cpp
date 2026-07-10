#include "nickfilteredit.h"
#include <QListWidget>
#include <QKeyEvent>

NickFilterEdit::NickFilterEdit(QListWidget *nickList, QWidget *parent)
    : QLineEdit(parent), m_nickList(nickList)
{
    setObjectName("nickFilter");
    setPlaceholderText(QStringLiteral("filter users…"));
    setClearButtonEnabled(true);
    connect(this, &QLineEdit::textChanged, this, [this](const QString &text){
        const QString lower = text.toLower();
        for (int i = 0; i < m_nickList->count(); ++i) {
            auto *item = m_nickList->item(i);
            const QString nick = item->data(Qt::UserRole).toString().toLower();
            item->setHidden(!lower.isEmpty() && !nick.startsWith(lower));
        }
    });
}

void NickFilterEdit::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        clear();
        return;
    }
    QLineEdit::keyPressEvent(event);
}
