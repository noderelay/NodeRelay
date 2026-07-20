#include "quickswitcher.h"
#include "model/sessionmodel.h"

#include <QLineEdit>
#include <QListWidget>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QGraphicsDropShadowEffect>

QuickSwitcher::QuickSwitcher(SessionModel *model, QWidget *parent)
    : QFrame(parent), m_model(model)
{
    setFixedSize(380, 320);
    setFrameStyle(QFrame::Box | QFrame::Raised);
    setLineWidth(1);
    setAutoFillBackground(true);
    hide();

    auto *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(24);
    shadow->setOffset(0, 6);
    shadow->setColor(QColor(0, 0, 0, 140));
    setGraphicsEffect(shadow);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(4);

    m_input = new QLineEdit;
    m_input->setPlaceholderText("Switch to channel…");
    m_input->setClearButtonEnabled(true);
    m_input->setStyleSheet("QLineEdit { border-radius: 6px; padding: 6px 10px; font-size: 14px; }");
    layout->addWidget(m_input);

    m_list = new QListWidget;
    m_list->setFrameShape(QFrame::NoFrame);
    m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_list->setStyleSheet(
        "QListWidget { border: none; }"
        "QListWidget::item { padding: 4px 8px; }"
        "QListWidget::item:selected { background: palette(highlight); color: palette(highlighted-text); border-radius: 4px; }");
    layout->addWidget(m_list, 1);

    connect(m_input, &QLineEdit::textChanged, this, &QuickSwitcher::filter);
    connect(m_list, &QListWidget::itemActivated, this, [this]{ accept(); });
}

void QuickSwitcher::showCentered()
{
    populate();
    m_input->clear();
    filter({});

    if (QWidget *p = parentWidget()) {
        move((p->width() - width()) / 2,
             (p->height() - height()) / 3);
    }
    show();
    raise();
    m_input->setFocus();
}

void QuickSwitcher::populate()
{
    m_entries.clear();
    for (const auto &sess : m_model->sessions()) {
        for (auto it = sess.channels.cbegin(); it != sess.channels.cend(); ++it) {
            if (it.value().name == "(server)") continue;
            m_entries.append({ServerId{sess.name}, BufferId{it.value().name}, sess.name});
        }
    }
    std::sort(m_entries.begin(), m_entries.end(), [](const Entry &a, const Entry &b){
        return a.channel.str().compare(b.channel.str(), Qt::CaseInsensitive) < 0;
    });
}

void QuickSwitcher::filter(const QString &text)
{
    m_list->clear();
    const QString query = text.trimmed().toLower();

    for (const auto &e : std::as_const(m_entries)) {
        const QString label = e.channel.str() + "  —  " + e.serverName;
        if (query.isEmpty() || label.toLower().contains(query)) {
            auto *item = new QListWidgetItem(label);
            item->setData(Qt::UserRole, e.host.str());
            item->setData(Qt::UserRole + 1, e.channel.str());
            m_list->addItem(item);
        }
    }
    if (m_list->count() > 0)
        m_list->setCurrentRow(0);
}

void QuickSwitcher::accept()
{
    auto *item = m_list->currentItem();
    if (!item) return;
    hide();
    emit channelSelected(
        ServerId{item->data(Qt::UserRole).toString()},
        BufferId{item->data(Qt::UserRole + 1).toString()});
}

void QuickSwitcher::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        hide();
        return;
    }
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        accept();
        return;
    }
    if (event->key() == Qt::Key_Up) {
        int row = m_list->currentRow();
        if (row > 0) m_list->setCurrentRow(row - 1);
        return;
    }
    if (event->key() == Qt::Key_Down) {
        int row = m_list->currentRow();
        if (row < m_list->count() - 1) m_list->setCurrentRow(row + 1);
        return;
    }
    QFrame::keyPressEvent(event);
}
