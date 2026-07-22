#pragma once

#include "message.h"
#include <QList>
#include <QString>

// Reads messages back out of the per-buffer log files written by
// SessionModel::logMessage(). Used to page older history into the chat
// view when the connection has no chathistory support.
namespace LogReader {

// Parse one log line ("[yyyy-MM-dd hh:mm:ss] …") into a Message with
// isHistory set. Returns false if the line doesn't parse.
bool parseLine(const QString &line, Message &out);

// Read up to `limit` messages older than `oldest` from the log at `path`,
// returned in chronological order (the qualifying messages closest to
// `oldest`). Log timestamps have second resolution, so the caller passes
// `sameSecondCount` — how many in-memory messages share `oldest`'s second;
// that many trailing same-second log lines are skipped as already loaded.
// `blockSize` is exposed for tests only.
QList<Message> readBefore(const QString &path, const QDateTime &oldest,
                          int sameSecondCount, int limit,
                          qint64 blockSize = 64 * 1024);

} // namespace LogReader
