#include "channellistdialog.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QTableView>
#include <QVBoxLayout>

ChannelListDialog::ChannelListDialog(const ServerId &host, QWidget *parent)
    : QDialog(parent)
    , m_host(host)
{
    setWindowTitle(tr("Channel List — %1").arg(m_host.str()));
    resize(700, 480);
    setAttribute(Qt::WA_DeleteOnClose, false);

    m_filter = new QLineEdit(this);
    m_filter->setPlaceholderText(tr("Filter channels…"));
    m_filter->setClearButtonEnabled(true);

    m_status = new QLabel(tr("Fetching…"), this);
    m_status->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_model = new QStandardItemModel(0, 3, this);
    m_model->setHorizontalHeaderLabels({tr("Channel"), tr("Users"), tr("Topic")});

    m_proxy = new QSortFilterProxyModel(this);
    m_proxy->setSourceModel(m_model);
    m_proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxy->setFilterKeyColumn(-1); // search all columns

    m_view = new QTableView(this);
    m_view->setModel(m_proxy);
    m_view->setSortingEnabled(true);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_view->setSelectionMode(QAbstractItemView::SingleSelection);
    m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_view->setAlternatingRowColors(true);
    m_view->verticalHeader()->hide();
    m_view->horizontalHeader()->setSortIndicatorShown(true);
    m_view->horizontalHeader()->setStretchLastSection(true);
    m_view->setColumnWidth(0, 160);
    m_view->setColumnWidth(1, 60);
    m_view->sortByColumn(1, Qt::DescendingOrder);

    m_joinBtn    = new QPushButton(tr("Join"), this);
    m_refreshBtn = new QPushButton(tr("Refresh"), this);
    auto *closeBtn = new QPushButton(tr("Close"), this);

    m_joinBtn->setEnabled(false);

    auto *filterRow = new QHBoxLayout;
    filterRow->addWidget(m_filter);
    filterRow->addWidget(m_status);

    auto *btnRow = new QHBoxLayout;
    btnRow->addStretch();
    btnRow->addWidget(m_joinBtn);
    btnRow->addWidget(m_refreshBtn);
    btnRow->addWidget(closeBtn);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(filterRow);
    layout->addWidget(m_view);
    layout->addLayout(btnRow);

    connect(m_filter, &QLineEdit::textChanged,
            m_proxy, &QSortFilterProxyModel::setFilterFixedString);

    connect(m_view->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this](const QItemSelection &sel) {
        m_joinBtn->setEnabled(!sel.isEmpty());
    });

    connect(m_view, &QTableView::doubleClicked, this, [this](const QModelIndex &) {
        joinSelected();
    });

    connect(m_joinBtn,    &QPushButton::clicked, this, &ChannelListDialog::joinSelected);
    connect(m_refreshBtn, &QPushButton::clicked, this, [this] {
        if (!m_fetching)
            emit refreshRequested(m_host);
    });
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::hide);

    m_fetching = true;
}

void ChannelListDialog::reset()
{
    m_model->removeRows(0, m_model->rowCount());
    m_count   = 0;
    m_fetching = true;
    m_joinBtn->setEnabled(false);
    m_status->setText(tr("Fetching…"));
}

void ChannelListDialog::addEntry(const QString &channel, int users, const QString &topic)
{
    auto *nameItem  = new QStandardItem(channel);
    auto *usersItem = new QStandardItem;
    usersItem->setData(users, Qt::DisplayRole);
    auto *topicItem = new QStandardItem(topic);

    nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
    usersItem->setFlags(usersItem->flags() & ~Qt::ItemIsEditable);
    topicItem->setFlags(topicItem->flags() & ~Qt::ItemIsEditable);

    m_model->appendRow({nameItem, usersItem, topicItem});
    ++m_count;
    m_status->setText(tr("%1 channels…").arg(m_count));
}

void ChannelListDialog::onListEnd(int total)
{
    m_fetching = false;
    m_status->setText(tr("%1 channels").arg(total));
}

void ChannelListDialog::joinSelected()
{
    const QModelIndex idx = m_view->currentIndex();
    if (!idx.isValid())
        return;
    const QModelIndex nameIdx = m_proxy->index(idx.row(), 0);
    const QString channel = m_proxy->data(nameIdx).toString();
    if (!channel.isEmpty())
        emit joinRequested(m_host, BufferId{channel});
}
