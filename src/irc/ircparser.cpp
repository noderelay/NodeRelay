#include "ircparser.h"

static QHash<QString,QString> parseTags(const QString &raw)
{
    QHash<QString,QString> out;
    for (const QString &part : raw.split(';', Qt::SkipEmptyParts)) {
        const qsizetype eq = part.indexOf('=');
        if (eq == -1) { out.insert(part, {}); continue; }
        QString val = part.mid(eq + 1);
        // IRCv3 tag value unescaping
        QString unescaped;
        unescaped.reserve(val.size());
        for (int i = 0; i < val.size(); ++i) {
            if (val[i] == '\\' && i + 1 < val.size()) {
                const QChar next = val[++i];
                if      (next == ':')  unescaped += ';';
                else if (next == 's')  unescaped += ' ';
                else if (next == '\\') unescaped += '\\';
                else if (next == 'r')  unescaped += '\r';
                else if (next == 'n')  unescaped += '\n';
                else                   { unescaped += '\\'; unescaped += next; }
            } else {
                unescaped += val[i];
            }
        }
        out.insert(part.left(eq), unescaped);
    }
    return out;
}

IrcMessage IrcParser::parse(const QString &raw)
{
    IrcMessage msg;
    QString line = raw;
    line.remove('\r');
    line.remove('\n');
    if (line.isEmpty()) return msg;

    qsizetype pos = 0;

    // IRCv3 tags
    if (line.startsWith('@')) {
        qsizetype space = line.indexOf(' ');
        if (space == -1) return msg;
        msg.tags = parseTags(line.mid(1, space - 1));
        pos = space + 1;

        const QString t = msg.tags.value("time");
        if (!t.isEmpty())
            msg.serverTime = QDateTime::fromString(t, Qt::ISODateWithMs);
    }

    // prefix
    if (pos < line.size() && line[pos] == ':') {
        qsizetype space = line.indexOf(' ', pos);
        if (space == -1) return msg;
        msg.prefix = line.mid(pos + 1, space - pos - 1);
        pos = space + 1;

        // parse nick!user@host
        qsizetype bang = msg.prefix.indexOf('!');
        qsizetype at   = msg.prefix.indexOf('@');
        if (bang != -1 && at != -1) {
            msg.nick = msg.prefix.left(bang);
            msg.user = msg.prefix.mid(bang + 1, at - bang - 1);
            msg.host = msg.prefix.mid(at + 1);
        } else {
            msg.nick = msg.prefix;
        }
    }

    // command
    {
        qsizetype space = line.indexOf(' ', pos);
        if (space == -1) {
            msg.command = line.mid(pos).toUpper();
            return msg;
        }
        msg.command = line.mid(pos, space - pos).toUpper();
        pos = space + 1;
    }

    // params + trailing
    while (pos < line.size()) {
        if (line[pos] == ':') {
            msg.trailing = line.mid(pos + 1);
            msg.params.append(msg.trailing);
            break;
        }
        qsizetype space = line.indexOf(' ', pos);
        if (space == -1) {
            msg.params.append(line.mid(pos));
            break;
        }
        msg.params.append(line.mid(pos, space - pos));
        pos = space + 1;
    }

    return msg;
}

QString IrcParser::decodeLine(const QByteArray &raw)
{
    // UTF-8 when valid; otherwise Latin-1, which cannot fail (every byte maps
    // to a code point) — the classic pre-UTF-8 IRC convention.
    return raw.isValidUtf8() ? QString::fromUtf8(raw) : QString::fromLatin1(raw);
}
