#include "nickfilteredit.h"
#include "nicklistmodel.h"
#include <QKeyEvent>

NickFilterEdit::NickFilterEdit(NickListModel *model, QWidget *parent)
    : QLineEdit(parent), m_model(model)
{
    setObjectName("nickFilter");
    setPlaceholderText(QStringLiteral("filter users…"));
    setClearButtonEnabled(true);
    connect(this, &QLineEdit::textChanged, this, [this](const QString &text){
        if (m_model) m_model->setFilter(text);
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
