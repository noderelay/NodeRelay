#pragma once
#include <QList>
#include <QPixmap>
#include <QString>
#include <QTextCharFormat>
#include <memory>

class QTextLayout;

enum class ChatLineRole { Message, EventGroup, PreviewCard, Reaction, StatusLine };

struct ChatSegment {
    int             start{0};
    int             length{0};
    QTextCharFormat format;
    QString         anchor; // "" | "nick:N" | "msgid:M" | "url:U" | "evgrp:G" | "preview:U"
};

struct ChatLine {
    QString             text;
    QList<ChatSegment>  segments;
    QString             id;        // msgid | "evgrp:G" | "rx:M" | ""
    ChatLineRole        role{ChatLineRole::Message};
    bool                hangIndent{false};
    int                 hangIndentChars{0}; // char count of prefix before body text (0 = global ts width)
    QPixmap             image;     // non-null for PreviewCard lines

    // Layout cache — invalidated on resize/font change
    mutable int     cachedH{0};
    mutable int     cachedHangPx{0}; // pixel width of prefix, computed from hangIndentChars
    mutable int     layoutWidth{0};  // wrap width cachedH/visLines were computed for
    mutable qreal   cachedNaturalW{0}; // natural text width when single visual line (0 = multi/unknown)
    struct VisLine {
        int    charStart;
        int    charEnd;
        qreal  x, y, w, h;
    };
    mutable QList<VisLine> visLines;

    // Built QTextLayout, reused across repaints; held only for lines near the
    // viewport (paintEvent evicts on scroll-away) so memory stays bounded.
    // cachedLayoutWidth tracks the wrap width the layout was built at — kept
    // separate from layoutWidth, which is the height-cache staleness marker.
    mutable std::shared_ptr<QTextLayout> cachedLayout;
    mutable int cachedLayoutWidth{0};
};
