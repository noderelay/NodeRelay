#pragma once

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

// Full-history search over a single buffer's on-disk log file. The scan runs
// on a worker thread with bounded memory (only the newest N matches are kept)
// and is cancellable, so typing stays responsive even on very large logs.
class LogSearchDialog : public QDialog
{
    Q_OBJECT

public:
    LogSearchDialog(const QString &bufferLabel, const QString &logPath,
                    bool loggingEnabled, QWidget *parent = nullptr);
    ~LogSearchDialog() override;

private:
    void startSearch();
    void showResults(const QStringList &lines, int total);

    const QString m_logPath;
    const bool    m_hasLog;

    QLineEdit   *m_input{nullptr};
    QCheckBox   *m_regex{nullptr};
    QListWidget *m_results{nullptr};
    QLabel      *m_status{nullptr};
    QTimer      *m_debounce{nullptr};

    QPointer<QThread>                 m_thread;   // latest in-flight scan
    std::shared_ptr<std::atomic_bool> m_cancel;   // cancels the in-flight scan
};
