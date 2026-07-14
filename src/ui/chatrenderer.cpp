#include "chatrenderer.h"
#include "model/channel.h"
#include <QDate>
#include <QDateTime>
#include <QRegularExpression>
#include <QFont>
#include <QPixmapCache>

namespace ChatRenderer {

static const QRegularExpression s_urlRe(
    R"((https?://[^ \t\r\n<>"]+))",
    QRegularExpression::CaseInsensitiveOption);

QString htmlAttr(const QString &s)
{
    QString out = s.toHtmlEscaped();
    out.replace(QLatin1Char('\''), QLatin1String("&#39;"));
    return out;
}

QString linkifyTopic(const QString &text)
{
    QString html = ircToHtml(text);
    html.replace(s_urlRe, R"(<a href="\1">\1</a>)");
    return html;
}

QString linkifyHtml(const QString &html)
{
    QString result = html;
    result.replace(s_urlRe, R"(<a href="\1">\1</a>)");
    return result;
}

QColor nickColor(const QString &nick)
{
    static const char *palette[] = {
        "#e06c75", "#98c379", "#e5c07b", "#61afef",
        "#c678dd", "#56b6c2", "#d19a66", "#7ec8a0",
        "#ff7b72", "#79c0ff", "#ffa657", "#85e89d",
        "#f78166", "#58a6ff", "#d4a0f5", "#4db5bd",
    };
    static constexpr int N = int(sizeof(palette) / sizeof(palette[0]));
    return QColor(palette[qHash(nick.toLower()) % N]);
}

static const char* const kIrcPalette[16] = {
    "#FFFFFF","#000000","#00007F","#009300",
    "#FF0000","#7F0000","#9C009C","#FC7F00",
    "#FFFF00","#00FC00","#009393","#00FFFF",
    "#0000FC","#FF00FF","#7F7F7F","#D2D2D2"
};

QColor mircColor(int index)
{
    if (index < 0 || index >= 16) return QColor();
    return QColor(kIrcPalette[index]);
}

int mircColorIndex(const QColor &color)
{
    if (!color.isValid()) return -1;
    for (int i = 0; i < 16; ++i)
        if (QColor(kIrcPalette[i]).rgb() == color.rgb()) return i;
    return -1;
}

struct IrcSpan {
    QString text;
    bool bold{false}, italic{false}, underline{false}, strike{false};
    int fg{-1}, bg{-1};
};

static QList<IrcSpan> parseIrcSpans(const QString &raw)
{
    QList<IrcSpan> spans;
    struct State {
        bool bold{false}, italic{false}, underline{false}, strike{false};
        int fg{-1}, bg{-1};
    } cur;
    QString chunk;
    int i = 0;
    const int len = static_cast<int>(raw.size());

    auto flush = [&]() {
        if (chunk.isEmpty()) return;
        IrcSpan s;
        s.text      = chunk;
        s.bold      = cur.bold;
        s.italic    = cur.italic;
        s.underline = cur.underline;
        s.strike    = cur.strike;
        s.fg        = cur.fg;
        s.bg        = cur.bg;
        spans.append(s);
        chunk.clear();
    };

    while (i < len) {
        const ushort c = raw[i].unicode();
        if      (c == 0x02) { flush(); cur.bold      = !cur.bold;      ++i; }
        else if (c == 0x1D) { flush(); cur.italic    = !cur.italic;    ++i; }
        else if (c == 0x1F) { flush(); cur.underline = !cur.underline; ++i; }
        else if (c == 0x1E) { flush(); cur.strike    = !cur.strike;    ++i; }
        else if (c == 0x16) { flush(); std::swap(cur.fg, cur.bg);      ++i; }
        else if (c == 0x0F || (c == 0x03 && i+1 < len && !raw[i+1].isDigit() && raw[i+1] != ',')) {
            flush(); cur = State{}; ++i;
        } else if (c == 0x03) {
            flush(); ++i;
            if (i < len && raw[i].isDigit()) {
                int fg = raw[i++].digitValue();
                if (i < len && raw[i].isDigit()) fg = fg * 10 + raw[i++].digitValue();
                cur.fg = fg;
                if (i < len && raw[i] == ',' && i+1 < len && raw[i+1].isDigit()) {
                    ++i;
                    int bg = raw[i++].digitValue();
                    if (i < len && raw[i].isDigit()) bg = bg * 10 + raw[i++].digitValue();
                    cur.bg = bg;
                }
            } else {
                cur.fg = -1; cur.bg = -1;
            }
        } else if (c == 0x11) {
            ++i;
        } else {
            chunk += raw[i++];
        }
    }
    flush();
    return spans;
}

QString ircToHtml(const QString &raw)
{
    QString out;
    out.reserve(raw.size() * 2);
    bool inSpan = false;

    auto closeSpan = [&]() {
        if (inSpan) { out += "</span>"; inSpan = false; }
    };

    for (const IrcSpan &s : parseIrcSpans(raw)) {
        QString style;
        if (s.bold)      style += "font-weight:bold;";
        if (s.italic)    style += "font-style:italic;";
        QString td;
        if (s.underline) td += "underline ";
        if (s.strike)    td += "line-through ";
        if (!td.isEmpty()) style += "text-decoration:" + td.trimmed() + ";";
        if (s.fg >= 0 && s.fg < 16) style += QString("color:%1;").arg(kIrcPalette[s.fg]);
        if (s.bg >= 0 && s.bg < 16) style += QString("background-color:%1;").arg(kIrcPalette[s.bg]);
        closeSpan();
        if (!style.isEmpty()) {
            out += "<span style='" + style + "'>";
            inSpan = true;
        }
        for (const QChar &ch : s.text) {
            if      (ch == '<') out += "&lt;";
            else if (ch == '>') out += "&gt;";
            else if (ch == '&') out += "&amp;";
            else if (ch == '"') out += "&quot;";
            else                out += ch;
        }
    }
    closeSpan();
    return out;
}

QString wrapEmojiHtml(const QString &html, double ptSize)
{
    const QString open  = QString("<span style='font-size:%1pt'>").arg(ptSize);
    const QString close = QStringLiteral("</span>");

    QString result;
    result.reserve(html.size() * 2);

    qsizetype i = 0;
    while (i < html.size()) {
        if (html[i] == '<') {
            qsizetype end = html.indexOf('>', i);
            if (end == -1) { result += html.mid(i); break; }
            result += html.mid(i, end - i + 1);
            i = end + 1;
            continue;
        }

        if (html[i].isHighSurrogate() && i + 1 < html.size() && html[i+1].isLowSurrogate()) {
            const uint cp = QChar::surrogateToUcs4(html[i], html[i+1]);
            if (cp >= 0x1F000 && cp <= 0x1FAFF) {
                result += open;
                result += html[i]; result += html[i+1];
                i += 2;
                if (i < html.size() && html[i].unicode() == 0xFE0F)
                    result += html[i++];
                result += close;
            } else {
                result += html[i++]; result += html[i++];
            }
            continue;
        }

        const ushort c = html[i].unicode();
        if (c >= 0x2300 && c <= 0x27BF) {
            result += open + html[i];
            ++i;
            if (i < html.size() && html[i].unicode() == 0xFE0F)
                result += html[i++];
            result += close;
            continue;
        }

        result += html[i++];
    }
    return result;
}

QString formatMessage(const Message &msg, const Context &ctx)
{
    const QDateTime local = msg.timestamp.toLocalTime();
    const bool sameDay = local.date() == QDate::currentDate();
    const QString ts = sameDay
        ? local.toString("hh:mm")
        : local.toString("MM/dd hh:mm");

    const QString dimColor = QStringLiteral("#888888");
    auto wrap = [&](const QString &color, const QString &text) {
        return QString("<span style='color:%1'>%2</span>")
            .arg(msg.isHistory ? dimColor : color, text.toHtmlEscaped());
    };
    const int eventPt = qMax(7, qRound(ctx.chatPt * 0.82));
    auto wrapEvent = [&](const QString &color, const QString &text) {
        return QString("<span style='color:%1; font-size:%2pt'>%3</span>")
            .arg(msg.isHistory ? dimColor : color, QString::number(eventPt), text.toHtmlEscaped());
    };

    QString html;
    const QString tsSpan = msg.msgid.isEmpty()
        ? QString("<span style='color:gray'>%1</span>").arg(ts)
        : QString("<a href='msgid:%1' style='color:gray;text-decoration:none'>%2</a>")
            .arg(htmlAttr(msg.msgid), ts);

    if (msg.redacted) {
        html = tsSpan + " <span style='color:gray;font-style:italic'>[message deleted]</span>";
        return html;
    }

    switch (msg.type) {
    case MessageType::Privmsg: {
        const QString color = msg.isHistory ? dimColor
            : (ctx.coloredNicks ? nickColor(msg.nick).name()
                                : (ctx.validTheme ? ctx.themeText : QStringLiteral("#cccccc")));
        const QString &br = ctx.nickBrackets;
        QString nickOpen, nickClose;
        if (!br.isEmpty()) {
            if (br.length() % 2 == 0) {
                nickOpen  = br.left(br.length() / 2).toHtmlEscaped();
                nickClose = br.mid(br.length() / 2).toHtmlEscaped();
            } else {
                nickOpen  = QString(br.front()).toHtmlEscaped();
                nickClose = QString(br.back()).toHtmlEscaped();
            }
        }
        const QString nickDisplay = nickOpen + msg.nick.toHtmlEscaped() + nickClose;
        const QString titleAttr = msg.account.isEmpty()
            ? QString()
            : " title='account: " + htmlAttr(msg.account) + "'";
        const QString nickAnchor = QString("<a href='nick:%1'%2 style='color:%3; text-decoration:none; font-weight:bold'>%4</a>")
            .arg(htmlAttr(msg.nick), titleAttr, color, nickDisplay);
        QString textHtml = wrapEmojiHtml(linkifyHtml(ircToHtml(msg.text)), ctx.emojiPt);
        textHtml.replace('\n', QLatin1String("<br>"));
        if (ctx.selfNickRe.isValid() && !msg.isHistory)
            textHtml.replace(ctx.selfNickRe, "<span style='color:red;font-weight:bold'>\\1</span>");
        if (msg.isHistory)
            textHtml = "<span style='color:" + dimColor + "'>" + textHtml + "</span>";
        QString replySpan;
        if (!msg.replyTo.isEmpty() && ctx.channel) {
            QString origNick;
            for (const auto &orig : std::as_const(ctx.channel->messages))
                if (orig.msgid == msg.replyTo) { origNick = orig.nick; break; }
            replySpan = origNick.isEmpty()
                ? "<span style='color:#6c7086;font-size:small'>↩</span> "
                : QString("<span style='color:#6c7086;font-size:small'>↩ %1</span> ")
                    .arg(origNick.toHtmlEscaped());
        }
        html = QString("%1 %2%3 %4").arg(tsSpan, replySpan, nickAnchor, textHtml);
        break;
    }
    case MessageType::Action: {
        const QString aTitleAttr = msg.account.isEmpty()
            ? QString()
            : " title='account: " + htmlAttr(msg.account) + "'";
        const QString actionNick = QString("<a href='nick:%1'%2 style='color:inherit; text-decoration:none'>%1</a>")
            .arg(htmlAttr(msg.nick), aTitleAttr);
        html = QString("%1 <span style='color:%2'><i>* %3 %4</i></span>")
            .arg(tsSpan, msg.isHistory ? dimColor : QStringLiteral("inherit"),
                 actionNick, wrapEmojiHtml(linkifyHtml(ircToHtml(msg.text)), ctx.emojiPt));
        break;
    }
    case MessageType::Notice: {
        const QString noticeNick = QString("<a href='nick:%1' style='color:inherit; text-decoration:none'>%1</a>")
            .arg(htmlAttr(msg.nick));
        const QString noticeColor = msg.isHistory ? dimColor : QStringLiteral("#cc8800");
        html = QString("%1 <span style='color:%2'>-%3- %4</span>")
            .arg(tsSpan, noticeColor, noticeNick, wrapEmojiHtml(linkifyHtml(ircToHtml(msg.text)), ctx.emojiPt));
        break;
    }

    case MessageType::Join:
        html = wrapEvent("seagreen",  ts + " → " + msg.text); break;
    case MessageType::Part:
        html = wrapEvent("#e06b6b", ts + " ← " + msg.text); break;
    case MessageType::Quit:
        html = wrapEvent("#e06b6b", ts + " ✕ " + msg.text); break;
    case MessageType::Nick:
        html = wrapEvent("steelblue", ts + " ~ "  + msg.text); break;
    case MessageType::Kick:
        html = wrap("#e06b6b", ts + " ✕ " + msg.text); break;
    case MessageType::Topic:
        html = wrap("steelblue", ts + " ⦁ Topic: " + msg.text); break;
    case MessageType::Error:
        html = wrap("red",       ts + " !! " + msg.text); break;
    case MessageType::Reply:
        html = wrap("#6090c0",   ts + " * "  + msg.text); break;
    case MessageType::Wallops:
        html = wrap("#e09030",   ts + " [W] " + msg.text); break;
    case MessageType::Server:
    default:
        html = wrap("gray",      ts + " * "  + msg.text); break;
    }

    return html;
}

QString formatEventGroup(const QList<Message> &msgs, const Context &ctx,
                         const QString &groupId, bool expanded)
{
    if (msgs.isEmpty()) return {};

    const int eventPt = qMax(7, qRound(ctx.chatPt * 0.82));

    if (expanded) {
        QString collapseAnchor;
        if (!groupId.isEmpty())
            collapseAnchor = QString("<a href='evgrp:%1' style='text-decoration:none;color:gray'>▾ </a>")
                .arg(htmlAttr(groupId));

        QStringList lines;
        bool firstLine = true;
        for (const auto &msg : msgs) {
            const QDateTime mLocal = msg.timestamp.toLocalTime();
            const bool mSameDay = mLocal.date() == QDate::currentDate();
            const QString mTs = mSameDay ? mLocal.toString("hh:mm") : mLocal.toString("MM/dd hh:mm");
            const QString mTsSpan = QString("<span style='color:gray'>%1</span>").arg(mTs);

            QString color, sym;
            switch (msg.type) {
            case MessageType::Join: color = QStringLiteral("seagreen");   sym = QStringLiteral("→"); break;
            case MessageType::Part:
            case MessageType::Quit: color = QStringLiteral("#e06b6b");   sym = QStringLiteral("←"); break;
            case MessageType::Nick: color = QStringLiteral("steelblue"); sym = QStringLiteral("~");      break;
            case MessageType::Kick: color = QStringLiteral("#e06b6b");   sym = QStringLiteral("✕"); break;
            default: continue;
            }

            const QString lineHtml =
                QString("<span style='color:%1;font-size:%2pt'>%3 %4 %5</span>")
                    .arg(color, QString::number(eventPt), mTsSpan, sym, msg.text.toHtmlEscaped());

            lines << (firstLine ? collapseAnchor + lineHtml : lineHtml);
            firstLine = false;
        }
        if (lines.isEmpty()) return {};
        return lines.join(QStringLiteral("<br>"));
    }

    // Collapsed form
    const QDateTime local = msgs.front().timestamp.toLocalTime();
    const bool sameDay = local.date() == QDate::currentDate();
    const QString ts = sameDay ? local.toString("hh:mm") : local.toString("MM/dd hh:mm");
    const QString tsSpan = QString("<span style='color:gray'>%1</span>").arg(ts);

    QStringList joins, parts, kicks;
    QList<QPair<QString,QString>> nickChanges;

    for (const auto &msg : msgs) {
        switch (msg.type) {
        case MessageType::Join:  joins.append(msg.nick);                        break;
        case MessageType::Part:
        case MessageType::Quit:  parts.append(msg.nick);                        break;
        case MessageType::Nick:  nickChanges.append({msg.nick, msg.replyTo});   break;
        case MessageType::Kick:  kicks.append(msg.nick);                        break;
        default: break;
        }
    }

    // Net-change filter: nick that joins and parts in same group → suppress both
    for (qsizetype i = joins.size() - 1; i >= 0; --i) {
        if (parts.contains(joins[i])) {
            parts.removeAll(joins[i]);
            joins.removeAt(i);
        }
    }

    const qsizetype total = joins.size() + parts.size() + nickChanges.size() + kicks.size();
    const int maxNicks = 10;
    int shown = 0;
    QStringList segments;

    auto addSection = [&](const QString &color, const QString &sym, const QStringList &nicks) {
        if (nicks.isEmpty()) return;
        QStringList display;
        for (const QString &n : nicks) {
            if (shown >= maxNicks) break;
            display << QString("<span style='color:%1'>%2</span>").arg(color, n.toHtmlEscaped());
            ++shown;
        }
        if (!display.isEmpty())
            segments << sym + " " + display.join(" ");
    };

    addSection("seagreen",  "→", joins);
    addSection("#e06b6b",   "←", parts);

    if (!nickChanges.isEmpty()) {
        QStringList display;
        for (const auto &p : std::as_const(nickChanges)) {
            if (shown >= maxNicks) break;
            display << QString("<span style='color:steelblue'>%1→%2</span>")
                .arg(p.first.toHtmlEscaped(), p.second.toHtmlEscaped());
            ++shown;
        }
        if (!display.isEmpty())
            segments << "~ " + display.join(" ");
    }

    addSection("#e06b6b", "✕", kicks);

    const qsizetype overflow = total - shown;
    QString body = segments.join("  ");
    if (overflow > 0)
        body += QString("  … %1 more").arg(overflow);

    const QString expandAnchor = groupId.isEmpty()
        ? QString()
        : QString("<a href='evgrp:%1' style='text-decoration:none;color:gray'>▸ </a>")
              .arg(htmlAttr(groupId));

    return QString("<span style='font-size:%1pt'>%2%3  %4</span>")
        .arg(eventPt).arg(expandAnchor, tsSpan, body);
}

// ── ChatLine rendering ────────────────────────────────────────────────────────

namespace {

struct TextBuilder {
    QString            text;
    QList<ChatSegment> segs;

    void append(const QString &s, const QTextCharFormat &fmt, const QString &anchor = {})
    {
        if (s.isEmpty()) return;
        ChatSegment seg;
        seg.start  = static_cast<int>(text.size());
        seg.length = static_cast<int>(s.size());
        seg.format = fmt;
        seg.anchor = anchor;
        text += s;
        segs.append(seg);
    }
};

} // anonymous namespace

static void ircToSegments(const QString &raw, const QTextCharFormat &base, TextBuilder &tb)
{
    for (const IrcSpan &s : parseIrcSpans(raw)) {
        QTextCharFormat fmt = base;
        if (s.bold)      fmt.setFontWeight(QFont::Bold);
        if (s.italic)    fmt.setFontItalic(true);
        if (s.underline) fmt.setFontUnderline(true);
        if (s.strike)    fmt.setFontStrikeOut(true);
        if (s.fg >= 0 && s.fg < 16) fmt.setForeground(QColor(kIrcPalette[s.fg]));
        if (s.bg >= 0 && s.bg < 16) fmt.setBackground(QColor(kIrcPalette[s.bg]));
        tb.append(s.text, fmt);
    }
}

static void linkifySegments(TextBuilder &tb, int textStart)
{
    auto it = s_urlRe.globalMatch(tb.text, textStart);
    while (it.hasNext()) {
        const auto m = it.next();
        ChatSegment seg;
        seg.start  = static_cast<int>(m.capturedStart(1));
        seg.length = static_cast<int>(m.capturedLength(1));
        seg.anchor = "url:" + tb.text.mid(seg.start, seg.length);
        tb.segs.append(seg);
    }
}

static inline bool isEmojiCodepoint(uint cp)
{
    return (cp >= 0x1F000 && cp <= 0x1FAFF)
        || (cp >= 0x2600  && cp <= 0x27BF)
        || (cp >= 0x2300  && cp <= 0x23FF)
        || (cp >= 0xFE00  && cp <= 0xFE0F)
        || cp == 0x200D
        || cp == 0x20E3;
}

static void applyEmojiSize(TextBuilder &tb, int textStart, double emojiPt)
{
    if (emojiPt <= 0) return;
    const QString &t = tb.text;
    const int len = static_cast<int>(t.size());
    int i = textStart;
    while (i < len) {
        int runStart = -1;
        while (i < len) {
            uint cp;
            int advance;
            if (t[i].isHighSurrogate() && i + 1 < len && t[i+1].isLowSurrogate()) {
                cp = QChar::surrogateToUcs4(t[i], t[i+1]);
                advance = 2;
            } else {
                cp = t[i].unicode();
                advance = 1;
            }
            if (isEmojiCodepoint(cp)) {
                if (runStart < 0) runStart = i;
                i += advance;
            } else {
                break;
            }
        }
        if (runStart >= 0) {
            QTextCharFormat fmt;
            fmt.setFontPointSize(emojiPt);
            ChatSegment seg;
            seg.start  = runStart;
            seg.length = i - runStart;
            seg.format = fmt;
            tb.segs.append(seg);
        }
        if (runStart < 0) ++i;
    }
}

static void addSelfNickHighlight(TextBuilder &tb, int textStart, const QRegularExpression &re)
{
    if (!re.isValid()) return;
    QTextCharFormat fmt;
    fmt.setForeground(QColor(Qt::red));
    fmt.setFontWeight(QFont::Bold);
    auto it = re.globalMatch(tb.text, textStart);
    while (it.hasNext()) {
        const auto m = it.next();
        ChatSegment seg;
        seg.start  = static_cast<int>(m.capturedStart(1));
        seg.length = static_cast<int>(m.capturedLength(1));
        seg.format = fmt;
        tb.segs.append(seg);
    }
}

ChatLine formatMessageLine(const Message &msg, const Context &ctx)
{
    const QDateTime local = msg.timestamp.toLocalTime();
    const bool sameDay = local.date() == QDate::currentDate();
    const QString ts = sameDay ? local.toString("hh:mm") : local.toString("MM/dd hh:mm");

    const QColor dimColor("#888888");
    const bool   isHistory = msg.isHistory;
    TextBuilder  tb;
    const QTextCharFormat plainFmt;

    QTextCharFormat tsFmt;
    tsFmt.setForeground(dimColor);
    const QString tsAnchor = msg.msgid.isEmpty() ? QString() : ("msgid:" + msg.msgid);
    if (ctx.showTimestamps)
        tb.append(ts, tsFmt, tsAnchor);

    if (msg.redacted) {
        QTextCharFormat f;
        f.setForeground(dimColor);
        f.setFontItalic(true);
        tb.append(" [message deleted]", f);
        ChatLine line;
        line.text       = tb.text;
        line.segments   = tb.segs;
        line.id         = msg.msgid;
        line.role       = ChatLineRole::Message;
        line.hangIndent = false;
        return line;
    }

    const bool isText = (msg.type == MessageType::Privmsg ||
                         msg.type == MessageType::Action  ||
                         msg.type == MessageType::Notice);

    int prefixEnd = 0; // char offset where body text starts
    switch (msg.type) {
    case MessageType::Privmsg: {
        const QColor nickCol = isHistory ? dimColor
            : (ctx.coloredNicks ? nickColor(msg.nick)
                                : (ctx.validTheme ? QColor(ctx.themeText) : QColor("#cccccc")));

        // Reply indicator
        if (!msg.replyTo.isEmpty() && ctx.channel) {
            QString origNick;
            for (const auto &orig : std::as_const(ctx.channel->messages))
                if (orig.msgid == msg.replyTo) { origNick = orig.nick; break; }
            QTextCharFormat f;
            f.setForeground(QColor("#6c7086"));
            tb.append(" ↩" + (origNick.isEmpty() ? " " : " " + origNick + " "), f);
        } else {
            tb.append(" ", plainFmt);
        }

        const QString &br = ctx.nickBrackets;
        QString nickOpen, nickClose;
        if (!br.isEmpty()) {
            if (br.length() % 2 == 0) {
                nickOpen  = br.left(br.length() / 2);
                nickClose = br.mid(br.length() / 2);
            } else {
                nickOpen  = QString(br.front());
                nickClose = QString(br.back());
            }
        }
        QTextCharFormat nickFmt;
        nickFmt.setFontWeight(QFont::Bold);
        nickFmt.setForeground(nickCol);
        tb.append(nickOpen + msg.nick + nickClose, nickFmt, "nick:" + msg.nick);
        tb.append(" ", plainFmt);

        prefixEnd = static_cast<int>(tb.text.size());
        QTextCharFormat base;
        if (isHistory) base.setForeground(dimColor);
        else           base.setForeground(ctx.validTheme ? QColor(ctx.themeText) : QColor("#cccccc"));
        ircToSegments(msg.text, base, tb);
        if (!isHistory) {
            linkifySegments(tb, prefixEnd);
            if (!ctx.selfNickRe.pattern().isEmpty())
                addSelfNickHighlight(tb, prefixEnd, ctx.selfNickRe);
            if (!ctx.highlightRe.pattern().isEmpty())
                addSelfNickHighlight(tb, prefixEnd, ctx.highlightRe);
        }
        break;
    }
    case MessageType::Action: {
        tb.append(" ", plainFmt);
        QTextCharFormat starFmt;
        starFmt.setFontItalic(true);
        if (isHistory) starFmt.setForeground(dimColor);
        tb.append("* ", starFmt);

        QTextCharFormat nickFmt;
        nickFmt.setFontItalic(true);
        if (isHistory) nickFmt.setForeground(dimColor);
        tb.append(msg.nick + " ", nickFmt, "nick:" + msg.nick);

        prefixEnd = static_cast<int>(tb.text.size());
        QTextCharFormat base;
        base.setFontItalic(true);
        if (isHistory) base.setForeground(dimColor);
        else           base.setForeground(ctx.validTheme ? QColor(ctx.themeText) : QColor("#cccccc"));
        ircToSegments(msg.text, base, tb);
        if (!isHistory) linkifySegments(tb, prefixEnd);
        break;
    }
    case MessageType::Notice: {
        const QColor col = isHistory ? dimColor : QColor("#cc8800");
        tb.append(" ", plainFmt);
        QTextCharFormat f;
        f.setForeground(col);
        tb.append("-" + msg.nick + "- ", f, "nick:" + msg.nick);
        prefixEnd = static_cast<int>(tb.text.size());
        QTextCharFormat base;
        base.setForeground(col);
        ircToSegments(msg.text, base, tb);
        if (!isHistory) linkifySegments(tb, prefixEnd);
        break;
    }
    case MessageType::Join: {
        QTextCharFormat f;
        f.setForeground(isHistory ? dimColor : QColor("seagreen"));
        tb.append(" → " + msg.text, f);
        break;
    }
    case MessageType::Part: {
        QTextCharFormat f;
        f.setForeground(isHistory ? dimColor : QColor("#e06b6b"));
        tb.append(" ← " + msg.text, f);
        break;
    }
    case MessageType::Quit: {
        QTextCharFormat f;
        f.setForeground(isHistory ? dimColor : QColor("#e06b6b"));
        tb.append(" ✕ " + msg.text, f);
        break;
    }
    case MessageType::Nick: {
        QTextCharFormat f;
        f.setForeground(isHistory ? dimColor : QColor("steelblue"));
        tb.append(" ~ " + msg.text, f);
        break;
    }
    case MessageType::Kick: {
        QTextCharFormat f;
        f.setForeground(isHistory ? dimColor : QColor("#e06b6b"));
        tb.append(" ✕ " + msg.text, f);
        break;
    }
    case MessageType::Topic: {
        QTextCharFormat f;
        f.setForeground(isHistory ? dimColor : QColor("steelblue"));
        tb.append(" ⦁ Topic: " + msg.text, f);
        break;
    }
    case MessageType::Error: {
        QTextCharFormat f;
        f.setForeground(isHistory ? dimColor : QColor(Qt::red));
        tb.append(" !! " + msg.text, f);
        break;
    }
    case MessageType::Reply: {
        QTextCharFormat f;
        f.setForeground(isHistory ? dimColor : QColor("#6090c0"));
        tb.append(" * " + msg.text, f);
        break;
    }
    case MessageType::Wallops: {
        QTextCharFormat f;
        f.setForeground(isHistory ? dimColor : QColor("#e09030"));
        tb.append(" [W] " + msg.text, f);
        break;
    }
    case MessageType::Server:
    default: {
        QTextCharFormat f;
        f.setForeground(dimColor);
        tb.append(" * " + msg.text, f);
        break;
    }
    }

    applyEmojiSize(tb, 0, ctx.emojiPt);

    ChatLine line;
    line.text       = tb.text;
    line.segments   = tb.segs;
    line.id         = msg.msgid;
    line.role       = ChatLineRole::Message;
    line.hangIndent      = isText;
    line.hangIndentChars = prefixEnd;
    return line;
}

ChatLine formatEventGroupLine(const QList<Message> &msgs, [[maybe_unused]] const Context &ctx,
                               const QString &groupId, bool expanded)
{
    if (msgs.isEmpty()) return {};

    TextBuilder tb;
    const QTextCharFormat plainFmt;

    if (expanded) {
        if (!groupId.isEmpty()) {
            QTextCharFormat f;
            f.setForeground(QColor(Qt::gray));
            tb.append("▾ ", f, "evgrp:" + groupId);
        }
        bool first = true;
        for (const auto &msg : msgs) {
            if (!first) tb.append("\n  ", plainFmt); // 2-space indent aligns timestamps with first entry (▾ prefix)
            const QDateTime mLocal = msg.timestamp.toLocalTime();
            const QString mTs = mLocal.date() == QDate::currentDate()
                ? mLocal.toString("hh:mm") : mLocal.toString("MM/dd hh:mm");
            QTextCharFormat tsFmt;
            tsFmt.setForeground(QColor(Qt::gray));
            tb.append(mTs + " ", tsFmt);

            QColor col;
            QString sym;
            switch (msg.type) {
            case MessageType::Join: col = QColor("seagreen");   sym = "→"; break;
            case MessageType::Part:
            case MessageType::Quit: col = QColor("#e06b6b");   sym = "←"; break;
            case MessageType::Nick: col = QColor("steelblue"); sym = "~";  break;
            case MessageType::Kick: col = QColor("#e06b6b");   sym = "✕"; break;
            default: continue;
            }
            QTextCharFormat f;
            f.setForeground(col);
            tb.append(sym + " " + msg.text, f);
            first = false;
        }
    } else {
        const QDateTime local = msgs.front().timestamp.toLocalTime();
        const QString ts = local.date() == QDate::currentDate()
            ? local.toString("hh:mm") : local.toString("MM/dd hh:mm");

        if (!groupId.isEmpty()) {
            QTextCharFormat f;
            f.setForeground(QColor(Qt::gray));
            tb.append("▸ ", f, "evgrp:" + groupId);
        }
        QTextCharFormat tsFmt;
        tsFmt.setForeground(QColor(Qt::gray));
        tb.append(ts + "  ", tsFmt);

        QStringList joins, parts, kicks;
        QList<QPair<QString,QString>> nickChanges;
        for (const auto &msg : msgs) {
            switch (msg.type) {
            case MessageType::Join:  joins.append(msg.nick);                          break;
            case MessageType::Part:
            case MessageType::Quit:  parts.append(msg.nick);                          break;
            case MessageType::Nick:  nickChanges.append({msg.nick, msg.replyTo});     break;
            case MessageType::Kick:  kicks.append(msg.nick);                          break;
            default: break;
            }
        }
        for (qsizetype i = joins.size() - 1; i >= 0; --i) {
            if (parts.contains(joins[i])) { parts.removeAll(joins[i]); joins.removeAt(i); }
        }

        const qsizetype total = joins.size() + parts.size() + nickChanges.size() + kicks.size();
        const int maxNicks = 10;
        int shown = 0;
        bool firstSec = true;

        auto addSection = [&](const QColor &col, const QString &sym, const QStringList &nicks) {
            if (nicks.isEmpty()) return;
            if (!firstSec) tb.append("  ", plainFmt);
            QTextCharFormat f;
            f.setForeground(col);
            QString text = sym + " ";
            for (const QString &n : nicks) {
                if (shown >= maxNicks) break;
                if (!text.endsWith(' ')) text += ' ';
                text += n;
                ++shown;
            }
            tb.append(text, f);
            firstSec = false;
        };

        addSection(QColor("seagreen"),  "→", joins);
        addSection(QColor("#e06b6b"),   "←", parts);

        if (!nickChanges.isEmpty()) {
            if (!firstSec) tb.append("  ", plainFmt);
            QTextCharFormat f;
            f.setForeground(QColor("steelblue"));
            QString text = "~ ";
            for (const auto &p : std::as_const(nickChanges)) {
                if (shown >= maxNicks) break;
                if (text.size() > 2) text += ' ';
                text += p.first + "→" + p.second;
                ++shown;
            }
            tb.append(text, f);
            firstSec = false;
        }

        addSection(QColor("#e06b6b"), "✕", kicks);

        const qsizetype overflow = total - shown;
        if (overflow > 0) {
            tb.append("  … " + QString::number(overflow) + " more", plainFmt);
        }
    }

    ChatLine line;
    line.text       = tb.text;
    line.segments   = tb.segs;
    line.id         = "evgrp:" + groupId;
    line.role       = ChatLineRole::EventGroup;
    line.hangIndent = false;
    return line;
}

ChatLine makeStatusLine(const QString &text, const QString &color)
{
    TextBuilder tb;
    QTextCharFormat f;
    f.setForeground(QColor(color));
    tb.append(text, f);
    ChatLine line;
    line.text     = tb.text;
    line.segments = tb.segs;
    line.role     = ChatLineRole::StatusLine;
    return line;
}

ChatLine buildPreviewCardLine(const QString &urlStr, const QString &title,
                              const QString &domain, const QByteArray &pngData)
{
    ChatLine line;
    line.id   = "preview:" + urlStr;
    line.role = ChatLineRole::PreviewCard;
    // Decode the thumbnail once per URL app-wide; every view showing this
    // card shares the pixmap through QPixmapCache instead of holding its
    // own decoded copy.
    const QString cacheKey = QStringLiteral("prevpx:") + urlStr;
    if (!QPixmapCache::find(cacheKey, &line.image) && !pngData.isEmpty()) {
        if (line.image.loadFromData(pngData, "PNG"))
            QPixmapCache::insert(cacheKey, line.image);
    }
    line.text = title + "\n" + domain;
    QTextCharFormat titleFmt;
    titleFmt.setFontWeight(QFont::Bold);
    ChatSegment titleSeg;
    titleSeg.start  = 0;
    titleSeg.length = static_cast<int>(title.size());
    titleSeg.format = titleFmt;
    titleSeg.anchor = "preview:" + urlStr;
    line.segments.append(titleSeg);
    QTextCharFormat domainFmt;
    domainFmt.setForeground(QColor("#888888"));
    ChatSegment domainSeg;
    domainSeg.start  = static_cast<int>(title.size()) + 1;
    domainSeg.length = static_cast<int>(domain.size());
    domainSeg.format = domainFmt;
    line.segments.append(domainSeg);
    return line;
}

} // namespace ChatRenderer
