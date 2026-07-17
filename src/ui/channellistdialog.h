#pragma once
#include "model/ids.h"
#include <QDialog>
#include <QString>

class QLineEdit;
class QLabel;
class QPushButton;
class QTableView;
class QStandardItemModel;
class QSortFilterProxyModel;

class ChannelListDialog : public QDialog {
    Q_OBJECT
public:
    explicit ChannelListDialog(const ServerId &host, QWidget *parent = nullptr);

    const ServerId &host() const { return m_host; }
    bool    isFetching() const { return m_fetching; }

    void reset();

public slots:
    void addEntry(const QString &channel, int users, const QString &topic);
    void onListEnd(int total);

signals:
    void joinRequested(const ServerId &host, const BufferId &channel);
    void refreshRequested(const ServerId &host);

private:
    void joinSelected();

    ServerId               m_host;
    bool                   m_fetching{false};
    int                    m_count{0};

    QLineEdit             *m_filter;
    QLabel                *m_status;
    QTableView            *m_view;
    QStandardItemModel    *m_model;
    QSortFilterProxyModel *m_proxy;
    QPushButton           *m_joinBtn;
    QPushButton           *m_refreshBtn;
};
