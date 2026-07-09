#pragma once

#include <QColor>
#include <QHash>
#include <QList>
#include <QRegularExpression>
#include <QSet>
#include <QString>
#include "model/message.h"
#include "ui/chatline.h"

struct Channel;

namespace ChatRenderer {

struct Context {
    bool              coloredNicks{false};
    QString           nickBrackets;
    double            emojiPt{10};
    double            chatPt{10};
    bool              validTheme{false};
    QString           themeText;
    QRegularExpression selfNickRe;   // pre-compiled; invalid = no highlight
    QRegularExpression highlightRe;  // extra keyword highlights; invalid = none
    bool               showTimestamps{true};
    const Channel    *channel{nullptr}; // for reply-reference lookup
};

// ChatLine-based rendering (used by ChatView)
ChatLine formatMessageLine    (const Message &msg, const Context &ctx);
ChatLine formatEventGroupLine (const QList<Message> &msgs, const Context &ctx,
                               const QString &groupId = {}, bool expanded = false);
ChatLine makeStatusLine       (const QString &text, const QString &color = QStringLiteral("#888888"));

// HTML rendering (kept for topic bar QLabel)
QString formatMessage    (const Message &msg, const Context &ctx);
QString formatEventGroup (const QList<Message> &msgs, const Context &ctx,
                          const QString &groupId = {}, bool expanded = false);
QString linkifyTopic     (const QString &text);
QColor  nickColor     (const QString &nick);

// mIRC 16-color palette (shared by the renderer and the input color picker)
QColor  mircColor      (int index);           // index 0-15, else invalid QColor
int     mircColorIndex (const QColor &color);  // exact palette match, else -1

// Lower-level helpers — exposed for unit tests
QString htmlAttr      (const QString &s);
QString ircToHtml     (const QString &raw);
QString linkifyHtml   (const QString &html);
QString wrapEmojiHtml (const QString &html, double ptSize);

inline ChatLine buildReactionLine(const QHash<QString, QSet<QString>> &rx, const QString &msgid)
{
    QTextCharFormat fmt;
    fmt.setForeground(QColor("#888888"));
    QString text;
    QList<ChatSegment> segs;
    for (auto it = rx.constBegin(); it != rx.constEnd(); ++it) {
        ChatSegment seg;
        seg.start  = static_cast<int>(text.size());
        const QString piece = it.key() + "(" + QString::number(it.value().size()) + ") ";
        seg.length = static_cast<int>(piece.size());
        seg.format = fmt;
        text += piece;
        segs.append(seg);
    }
    ChatLine line;
    line.text     = text;
    line.segments = segs;
    line.id       = "rx:" + msgid;
    line.role     = ChatLineRole::Reaction;
    return line;
}

inline bool isCondensable(const Message &msg, const QString &selfNick)
{
    switch (msg.type) {
    case MessageType::Join:
    case MessageType::Part:
    case MessageType::Quit:
    case MessageType::Nick:
    case MessageType::Kick:
        return selfNick.isEmpty()
            || msg.nick.compare(selfNick, Qt::CaseInsensitive) != 0;
    default:
        return false;
    }
}

} // namespace ChatRenderer
