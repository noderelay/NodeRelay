#include "logsearchdialog.h"

#include <QCheckBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMetaObject>
#include <QRegularExpression>
#include <QTextStream>
#include <QThread>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>

namespace {
constexpr int kMaxResults   = 1000;  // newest N matches retained (single buffer)
constexpr int kMaxPerBuffer = 200;   // newest N matches per buffer (all buffers)
constexpr int kDebounceMs   = 180;   // wait after a keystroke before scanning
}

LogSearchDialog::LogSearchDialog(const QString &bufferLabel, const QString &logPath,
                                 const QString &logsRoot, bool loggingEnabled,
                                 QWidget *parent)
    : QDialog(parent)
    , m_logPath(logPath)
    , m_logsRoot(logsRoot)
    , m_hasLog(!logPath.isEmpty() && QFileInfo::exists(logPath))
    , m_loggingEnabled(loggingEnabled)
{
    setWindowTitle(tr("Search history — %1").arg(bufferLabel));
    resize(560, 420);

    auto *root = new QVBoxLayout(this);

    auto *top = new QHBoxLayout;
    m_input = new QLineEdit;
    m_input->setPlaceholderText(tr("Search all logged history…"));
    m_input->setClearButtonEnabled(true);
    m_allBuffers = new QCheckBox(tr("All buffers"));
    m_regex = new QCheckBox(tr("Regex"));
    top->addWidget(m_input, 1);
    top->addWidget(m_allBuffers);
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

    connect(m_input,      &QLineEdit::textChanged, this, [this]{ m_debounce->start(); });
    connect(m_regex,      &QCheckBox::toggled,     this, [this]{ m_debounce->start(); });
    connect(m_allBuffers, &QCheckBox::toggled,     this, [this]{ m_debounce->start(); });

    // Double-click or Enter on a result opens that buffer (all-buffers mode —
    // single-buffer results are for the buffer the user is already in).
    connect(m_results, &QListWidget::itemActivated, this, [this](QListWidgetItem *it){
        const QStringList parts = it->data(Qt::UserRole).toStringList();
        if (parts.size() == 2) emit jumpRequested(parts[0], parts[1]);
    });

    if (!m_hasLog) {
        // The current buffer has no log yet, but "All buffers" can still
        // search other logs — keep the input enabled.
        m_status->setText(m_loggingEnabled
            ? tr("No history has been logged for this buffer yet.")
            : tr("Message logging is off — enable it in Preferences to record "
                 "and search past messages."));
    }
    m_input->setFocus();
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

    const bool allBuffers = m_allBuffers->isChecked();
    const QString query = m_input->text();
    m_results->clear();
    if (query.isEmpty()) {
        if (m_hasLog || allBuffers) m_status->clear();
        return;
    }
    if (!allBuffers && !m_hasLog) {
        m_status->setText(m_loggingEnabled
            ? tr("No history has been logged for this buffer yet.")
            : tr("Message logging is off — enable it in Preferences to record "
                 "and search past messages."));
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

    QThread *thread = nullptr;
    if (allBuffers) {
        const QString logsRoot = m_logsRoot;
        thread = QThread::create([this, logsRoot, query, useRegex, re, cancel]() {
            QList<BufferHits> hits;
            const QDir rootDir(logsRoot);
            const QStringList serverDirs =
                rootDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
            for (const QString &sd : serverDirs) {
                const QDir dir(rootDir.filePath(sd));
                const QStringList files =
                    dir.entryList({QStringLiteral("*.log")}, QDir::Files, QDir::Name);
                for (const QString &fn : files) {
                    if (cancel->load()) return;
                    QFile f(dir.filePath(fn));
                    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
                    BufferHits bh;
                    bh.serverPart = sd;
                    bh.bufferPart = fn.chopped(4); // drop ".log"
                    QTextStream in(&f);
                    QString line;
                    while (!(line = in.readLine()).isNull()) {
                        if (cancel->load()) return;
                        const bool hit = useRegex
                            ? re.match(line).hasMatch()
                            : line.contains(query, Qt::CaseInsensitive);
                        if (!hit) continue;
                        bh.lines.append(line);
                        ++bh.total;
                        if (bh.lines.size() > kMaxPerBuffer)
                            bh.lines.removeFirst(); // retain only the newest
                    }
                    if (bh.total > 0) {
                        std::reverse(bh.lines.begin(), bh.lines.end()); // newest first
                        hits.append(bh);
                    }
                }
            }
            QMetaObject::invokeMethod(this, [this, hits, cancel]() {
                if (cancel->load()) return;
                showAllBufferResults(hits);
            }, Qt::QueuedConnection);
        });
    } else {
        const QString path = m_logPath;
        thread = QThread::create([this, path, query, useRegex, re, cancel]() {
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
    }
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

void LogSearchDialog::showAllBufferResults(const QList<BufferHits> &hits)
{
    m_results->clear();
    if (hits.isEmpty()) {
        m_status->setText(tr("No matches."));
        return;
    }

    QFont headerFont = m_results->font();
    headerFont.setBold(true);

    int total = 0;
    bool truncated = false;
    for (const BufferHits &bh : hits) {
        total += bh.total;
        if (bh.total > bh.lines.size()) truncated = true;
        const QStringList jump{bh.serverPart, bh.bufferPart};

        auto *header = new QListWidgetItem(
            bh.bufferPart + QStringLiteral(" — ") + bh.serverPart
            + QStringLiteral("  (") + QString::number(bh.total) + QStringLiteral(")"));
        header->setFont(headerFont);
        header->setData(Qt::UserRole, jump);
        m_results->addItem(header);

        for (const QString &l : bh.lines) {
            auto *item = new QListWidgetItem(QStringLiteral("    ") + l);
            item->setData(Qt::UserRole, jump);
            m_results->addItem(item);
        }
    }

    QString status = tr("%1 matches across %2 buffers — double-click a result to open that buffer.")
                         .arg(total).arg(hits.size());
    if (truncated)
        status += tr(" Newest %1 shown per buffer.").arg(kMaxPerBuffer);
    m_status->setText(status);
}
