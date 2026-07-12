#pragma once

#include <QDialog>

class QTabWidget;
class QLineEdit;

class DocsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DocsDialog(QWidget *parent = nullptr);
    void showTab(const QString &title);

private:
    void addTab(QTabWidget *tabs, const QString &title, const QString &resource);
    void searchCurrentTab(const QString &text);

    QTabWidget *m_tabs{nullptr};
    QLineEdit  *m_search{nullptr};
};
