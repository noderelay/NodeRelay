#include "logreader.h"

#include <QFile>

#include <algorithm>

namespace LogReader {

bool parseLine(const QString &line, Message &out)
{
    // "[yyyy-MM-dd hh:mm:ss] rest" — 19 timestamp chars inside the brackets.
    if (line.size() < 23 || line[0] != '[' || line[20] != ']' || line[21] != ' ')
        return false;

    const QDateTime ts = QDateTime::fromString(line.mid(1, 19),
                                               QStringLiteral("yyyy-MM-dd hh:mm:ss"));
    if (!ts.isValid())
        return false;

    const QString rest = line.mid(22);

    if (rest.startsWith(QLatin1String("-- "))) {
        // Event/status lines lose their original type in the log; render
        // them back as plain status text.
        out = Message::make(MessageType::Server, {}, rest.mid(3), ts, true);
        return true;
    }
    if (rest.startsWith('<')) {
        const qsizetype gt = rest.indexOf('>');
        if (gt < 0) return false;
        out = Message::make(MessageType::Privmsg, rest.mid(1, gt - 1),
                            rest.mid(gt + 2), ts, true);
        return true;
    }
    if (rest.startsWith(QLatin1String("* "))) {
        const qsizetype sp = rest.indexOf(' ', 2);
        const QString nick = sp < 0 ? rest.mid(2) : rest.mid(2, sp - 2);
        out = Message::make(MessageType::Action, nick,
                            sp < 0 ? QString{} : rest.mid(sp + 1), ts, true);
        return true;
    }
    if (rest.startsWith('-')) {
        const qsizetype dash = rest.indexOf('-', 1);
        if (dash < 1) return false;
        out = Message::make(MessageType::Notice, rest.mid(1, dash - 1),
                            rest.mid(dash + 2), ts, true);
        return true;
    }
    return false;
}

QList<Message> readBefore(const QString &path, const QDateTime &oldest,
                          int sameSecondCount, int limit, qint64 blockSize)
{
    QFile f(path);
    if (limit <= 0 || !oldest.isValid() || !f.open(QIODevice::ReadOnly))
        return {};

    const qint64 oldestSec = oldest.toSecsSinceEpoch();
    int skipTies = qMax(0, sameSecondCount);

    QList<Message> out; // collected newest-first, reversed before returning
    QByteArray pending; // partial first line of the previously read block
    qint64 pos = f.size();

    while (pos > 0 && out.size() < limit) {
        const qint64 len = qMin(blockSize, pos);
        pos -= len;
        f.seek(pos);
        QByteArray block = f.read(len);
        block.append(pending);
        pending.clear();

        // Bytes before the first '\n' belong to a line starting in an
        // earlier block — defer them (unless this is the start of file).
        qsizetype start = 0;
        if (pos > 0) {
            start = block.indexOf('\n');
            if (start < 0) { pending = block; continue; }
            pending = block.left(start);
            ++start;
        }

        // Walk the complete lines back-to-front.
        qsizetype end = block.size();
        while (end > start && out.size() < limit) {
            qsizetype nl = block.lastIndexOf('\n', end - 1);
            if (nl < start) nl = start - 1;
            qsizetype lineLen = end - nl - 1;
            if (lineLen > 0 && block[nl + lineLen] == '\r')
                --lineLen; // writer opens in Text mode; tolerate CRLF
            const QString line = QString::fromUtf8(block.constData() + nl + 1, lineLen);
            end = nl < start ? start : nl;

            Message msg;
            if (!parseLine(line, msg))
                continue;
            const qint64 sec = msg.timestamp.toSecsSinceEpoch();
            if (sec > oldestSec)
                continue; // newer than the boundary — still in memory
            if (sec == oldestSec) {
                if (skipTies > 0) { --skipTies; continue; }
            }
            out.append(msg);
        }
    }

    std::reverse(out.begin(), out.end());
    return out;
}

} // namespace LogReader
