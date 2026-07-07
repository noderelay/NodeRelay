#include "logsearchdialog.h"

#include <QCheckBox>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMetaObject>
#include <QRegularExpression>
#include <QTextStream>
#include <QThread>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>

namespace {
constexpr int kMaxResults = 1000;   // newest N matches retained in memory
constexpr int kDebounceMs = 180;    // wait after a keystroke before scanning
}

LogSearchDialog::LogSearchDialog(const QString &bufferLabel, const QString &logPath,
                                 bool loggingEnabled, QWidget *parent)
    : QDialog(parent)
    , m_logPath(logPath)
    , m_hasLog(!logPath.isEmpty() && QFileInfo::exists(logPath))
{
    setWindowTitle(tr("Search history — %1").arg(bufferLabel));
    resize(560, 420);

    auto *root = new QVBoxLayout(this);

    auto *top = new QHBoxLayout;
    m_input = new QLineEdit;
    m_input->setPlaceholderText(tr("Search all logged history…"));
    m_input->setClearButtonEnabled(true);
    m_regex = new QCheckBox(tr("Regex"));
    top->addWidget(m_input, 1);
    top->addWidget(m_regex);
    root->addLayout(top);

    m_results = new QListWidget;
    m_results->setUniformItemSizes(true);
    m_results->setSelectionMode(QAbstractItemView::ExtendedSelection);
    root->addWidget(m_results, 1);

    m_status = new QLabel;
    m_status->setTextInteractionFlags(Qt::TextSelectableByMouse);
    root->addWidget(m_status);

    m_debounce = new QTimer(this);
    m_debounce->setSingleShot(true);
    m_debounce->setInterval(kDebounceMs);
    connect(m_debounce, &QTimer::timeout, this, &LogSearchDialog::startSearch);

    connect(m_input, &QLineEdit::textChanged, this, [this]{ m_debounce->start(); });
    connect(m_regex, &QCheckBox::toggled,     this, [this]{ m_debounce->start(); });

    if (!m_hasLog) {
        m_input->setEnabled(false);
        m_regex->setEnabled(false);
        m_status->setText(loggingEnabled
            ? tr("No history has been logged for this buffer yet.")
            : tr("Message logging is off — enable it in Preferences to record "
                 "and search past messages."));
    } else {
        m_input->setFocus();
    }
}

LogSearchDialog::~LogSearchDialog()
{
    // Stop the in-flight scan and wait for it to finish so the worker never
    // touches this object after destruction. The cancel flag bounds the wait
    // to a single readLine, so this returns effectively immediately.
    if (m_cancel) m_cancel->store(true);
    if (m_thread) m_thread->wait(5000);
}

void LogSearchDialog::startSearch()
{
    // Supersede any previous scan.
    if (m_cancel) m_cancel->store(true);

    const QString query = m_input->text();
    m_results->clear();
    if (!m_hasLog || query.isEmpty()) {
        if (m_hasLog) m_status->clear();
        return;
    }

    const bool useRegex = m_regex->isChecked();
    QRegularExpression re;
    if (useRegex) {
        re = QRegularExpression(query, QRegularExpression::CaseInsensitiveOption);
        if (!re.isValid()) {
            m_status->setText(tr("Invalid regular expression."));
            return;
        }
    }

    m_status->setText(tr("Searching…"));

    auto cancel = std::make_shared<std::atomic_bool>(false);
    m_cancel = cancel;
    const QString path = m_logPath;

    auto *thread = QThread::create([this, path, query, useRegex, re, cancel]() {
        QFile f(path);
        QStringList matches;
        int total = 0;
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&f);
            QString line;
            while (!(line = in.readLine()).isNull()) {
                if (cancel->load()) return;   // superseded or dialog closing
                const bool hit = useRegex
                    ? re.match(line).hasMatch()
                    : line.contains(query, Qt::CaseInsensitive);
                if (!hit) continue;
                matches.append(line);
                ++total;
                if (matches.size() > kMaxResults)
                    matches.removeFirst();     // retain only the newest matches
            }
        }
        std::reverse(matches.begin(), matches.end());   // newest first
        QMetaObject::invokeMethod(this, [this, matches, total, cancel]() {
            if (cancel->load()) return;
            showResults(matches, total);
        }, Qt::QueuedConnection);
    });
    m_thread = thread;
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

void LogSearchDialog::showResults(const QStringList &lines, int total)
{
    m_results->clear();
    m_results->addItems(lines);
    if (total == 0)
        m_status->setText(tr("No matches."));
    else if (total > lines.size())
        m_status->setText(tr("%1 matches — showing newest %2.").arg(total).arg(lines.size()));
    else
        m_status->setText(tr("%n match(es).", "", total));
}
