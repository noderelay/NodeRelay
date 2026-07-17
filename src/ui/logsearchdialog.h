#pragma once

#include <QDateTime>
#include <QDialog>
#include <QPointer>
#include <atomic>
#include <memory>

class QCheckBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QThread;
class QTimer;

// Full-history search over on-disk log files. Searches the current buffer's
// log by default; the "All buffers" toggle scans every log under the logs
// root, grouping results per buffer. Activating a result jumps to that
// message in its buffer (via the log line's timestamp). Scans run on a
// worker thread with bounded memory (only the newest N matches are kept
// per buffer) and are cancellable, so typing stays responsive even on
// very large logs.
class LogSearchDialog : public QDialog
{
    Q_OBJECT

public:
    LogSearchDialog(const QString &bufferLabel, const QString &logPath,
                    const QString &logsRoot, bool loggingEnabled,
                    QWidget *parent = nullptr);
    ~LogSearchDialog() override;

signals:
    // Sanitized "<server>/<buffer>" log-path components of the activated
    // result; the receiver maps them back to a live buffer. ts is the
    // result line's timestamp (invalid on buffer-header rows).
    void jumpRequested(const QString &serverPart, const QString &bufferPart,
                       const QDateTime &ts);
    // Activated result in single-buffer mode: jump within the buffer the
    // dialog was opened for.
    void jumpInBufferRequested(const QDateTime &ts);

private:
    struct BufferHits {
        QString     serverPart; // sanitized log dir name
        QString     bufferPart; // sanitized log file base name
        QStringList lines;      // newest first
        int         total{0};
    };

    void startSearch();
    void showResults(const QStringList &lines, int total);
    void showAllBufferResults(const QList<BufferHits> &hits);

    const QString m_logPath;
    const QString m_logsRoot;
    const bool    m_hasLog;
    const bool    m_loggingEnabled;

    QLineEdit   *m_input{nullptr};
    QCheckBox   *m_allBuffers{nullptr};
    QCheckBox   *m_regex{nullptr};
    QListWidget *m_results{nullptr};
    QLabel      *m_status{nullptr};
    QTimer      *m_debounce{nullptr};

    QPointer<QThread>                 m_thread;   // latest in-flight scan
    std::shared_ptr<std::atomic_bool> m_cancel;   // cancels the in-flight scan
};
