#pragma once
#include "config/config.h"
#include <QDialog>

class QTreeWidget;
class QLineEdit;

// Edit → Ignore List…: view and edit the client-side ignore list.
// Same semantics as the nick right-click Ignore submenu — nicks are
// lowercased, rows with no type checked are dropped (unignored).
class IgnoreListDialog : public QDialog
{
    Q_OBJECT
public:
    explicit IgnoreListDialog(const QList<IgnoreEntry> &entries, QWidget *parent = nullptr);
    QList<IgnoreEntry> entries() const;

private:
    void addRow(const IgnoreEntry &e);

    QTreeWidget *m_list{nullptr};
    QLineEdit   *m_addEdit{nullptr};
};
