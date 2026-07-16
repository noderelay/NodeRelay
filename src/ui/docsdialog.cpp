#include "docsdialog.h"
#include "appicons.h"
#include "menuicons.h"

#include <QTabWidget>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QLineEdit>
#include <QFile>
#include <QTextStream>
#include <QApplication>
#include <QStyle>

DocsDialog::DocsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Uplink Documentation");
    setWindowIcon(AppIcons::appIcon());
    resize(800, 580);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 8);
    layout->setSpacing(4);

    m_tabs = new QTabWidget;
    addTab(m_tabs, "Configuration",  ":/docs/configuration.md");
    addTab(m_tabs, "Commands",       ":/docs/commands.md");
    addTab(m_tabs, "IRCv3",          ":/docs/ircv3.md");
    addTab(m_tabs, "FAQ",            ":/docs/faq.md");
    addTab(m_tabs, "Shortcuts",      ":/docs/keyboard-shortcuts.md");
    layout->addWidget(m_tabs, 1);

    // Search widget in the tab bar corner
    m_search = new QLineEdit;
    m_search->setPlaceholderText("Search...");
    m_search->setClearButtonEnabled(true);
    m_search->setFixedWidth(160);
    m_search->setStyleSheet("QLineEdit { border-radius: 10px; padding: 3px 8px; }");
    m_search->addAction(
        QApplication::style()->standardIcon(QStyle::SP_FileDialogContentsView),
        QLineEdit::LeadingPosition);
    m_tabs->setCornerWidget(m_search, Qt::TopRightCorner);

    connect(m_search, &QLineEdit::textChanged, this, &DocsDialog::searchCurrentTab);
    connect(m_tabs, &QTabWidget::currentChanged, this, [this]{
        searchCurrentTab(m_search->text());
    });

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    buttons->button(QDialogButtonBox::Close)->setIcon(MenuIcons::close());
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::close);
    layout->addWidget(buttons);
    layout->setContentsMargins(8, 0, 8, 8);
}

void DocsDialog::addTab(QTabWidget *tabs, const QString &title, const QString &resource)
{
    QFile f(resource);
    QString content;
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream ts(&f);
        content = ts.readAll();
    } else {
        content = "# " + title + "\n\nDocumentation not available.";
    }

    auto *browser = new QTextBrowser;
    browser->setMarkdown(content);
    browser->setOpenExternalLinks(false);
    browser->setStyleSheet("QTextBrowser { padding: 10px; }");
    tabs->addTab(browser, title);
}

void DocsDialog::showTab(const QString &title)
{
    for (int i = 0; i < m_tabs->count(); ++i)
        if (m_tabs->tabText(i) == title) {
            m_tabs->setCurrentIndex(i);
            return;
        }
}

void DocsDialog::searchCurrentTab(const QString &text)
{
    auto *browser = qobject_cast<QTextBrowser *>(m_tabs->currentWidget());
    if (!browser) return;
    browser->moveCursor(QTextCursor::Start);
    if (!text.isEmpty())
        browser->find(text);
}
