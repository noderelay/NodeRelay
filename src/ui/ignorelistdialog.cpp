#include "ignorelistdialog.h"

#include <QTreeWidget>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHBoxLayout>

IgnoreListDialog::IgnoreListDialog(const QList<IgnoreEntry> &entries, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Ignore List"));
    resize(420, 340);

    auto *vbox = new QVBoxLayout(this);

    m_list = new QTreeWidget;
    m_list->setColumnCount(4);
    m_list->setHeaderLabels({tr("Nick"), tr("PMs"), tr("Notices"), tr("Invites")});
    m_list->setRootIsDecorated(false);
    m_list->header()->setStretchLastSection(false);
    m_list->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    for (const IgnoreEntry &e : entries)
        addRow(e);
    vbox->addWidget(m_list, 1);

    auto *addBox = new QHBoxLayout;
    m_addEdit = new QLineEdit;
    m_addEdit->setPlaceholderText(tr("nick"));
    auto *addBtn    = new QPushButton(tr("Add"));
    auto *removeBtn = new QPushButton(tr("Remove"));
    addBox->addWidget(m_addEdit, 1);
    addBox->addWidget(addBtn);
    addBox->addWidget(removeBtn);
    vbox->addLayout(addBox);

    auto doAdd = [this]{
        const QString nick = m_addEdit->text().trimmed().toLower();
        if (nick.isEmpty()) return;
        for (int i = 0; i < m_list->topLevelItemCount(); ++i)
            if (m_list->topLevelItem(i)->text(0) == nick) { m_addEdit->clear(); return; }
        addRow({nick, kIgnoreAll});
        m_addEdit->clear();
    };
    connect(addBtn,    &QPushButton::clicked,    this, doAdd);
    connect(m_addEdit, &QLineEdit::returnPressed, this, doAdd);
    connect(removeBtn, &QPushButton::clicked, this, [this]{
        delete m_list->currentItem();
    });

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    vbox->addWidget(buttons);
}

void IgnoreListDialog::addRow(const IgnoreEntry &e)
{
    auto *item = new QTreeWidgetItem(m_list);
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setText(0, e.nick.toLower());
    item->setCheckState(1, (e.flags & IgnoreType::PM)     ? Qt::Checked : Qt::Unchecked);
    item->setCheckState(2, (e.flags & IgnoreType::Notice) ? Qt::Checked : Qt::Unchecked);
    item->setCheckState(3, (e.flags & IgnoreType::Invite) ? Qt::Checked : Qt::Unchecked);
}

QList<IgnoreEntry> IgnoreListDialog::entries() const
{
    QList<IgnoreEntry> out;
    for (int i = 0; i < m_list->topLevelItemCount(); ++i) {
        const auto *item = m_list->topLevelItem(i);
        IgnoreTypes flags;
        if (item->checkState(1) == Qt::Checked) flags |= IgnoreType::PM;
        if (item->checkState(2) == Qt::Checked) flags |= IgnoreType::Notice;
        if (item->checkState(3) == Qt::Checked) flags |= IgnoreType::Invite;
        if (!flags) continue;   // nothing checked = unignored
        out.append({item->text(0), flags});
    }
    return out;
}
