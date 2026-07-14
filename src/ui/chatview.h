#pragma once
#include "ui/chatline.h"
#include <QAbstractScrollArea>
#include <QColor>
#include <QFont>
#include <QTextLayout>
#include <QVector>

class QTimer;

class ChatView : public QAbstractScrollArea {
    Q_OBJECT
public:
    explicit ChatView(QWidget *parent = nullptr);
    QSize minimumSizeHint() const override { return {1, 1}; }

    void appendLine(const ChatLine &line);
    void setMaxLines(int n); // retention cap; panes use a smaller one than the main view
    void prependLines(QList<ChatLine> lines);
    void replaceLine(const QString &id, const ChatLine &line);
    void insertAfter(const QString &id, const ChatLine &line);
    void removeLine(const QString &id);
    void clear();

    void setFont(const QFont &f);
    void setColors(const QColor &text, const QColor &bg,
                   const QColor &link, const QColor &cardBg, const QColor &cardBorder);

    bool isAtBottom() const;
    void scrollToBottom();
    void scrollToLine(int lineIdx);
    int  findLine(const QString &id) const; // -1 if not found

    bool findText(const QString &text, bool backward = false);
    void clearFind();
    QString selectedText() const;

signals:
    void anchorActivated(const QString &anchor, const QPoint &globalPos, Qt::MouseButton btn);
    void anchorHovered(const QString &anchor);
    void loadOlderRequested();
    void scrolledAwayFromBottom(bool away);

protected:
    void paintEvent(QPaintEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void leaveEvent(QEvent *e) override;
    void contextMenuEvent(QContextMenuEvent *e) override;

private:
    struct SelPoint {
        int line{-1};
        int pos{0};
        bool valid() const { return line >= 0; }
        bool operator==(const SelPoint &o) const { return line == o.line && pos == o.pos; }
        bool operator<(const SelPoint &o) const {
            if (line != o.line) return line < o.line;
            return pos < o.pos;
        }
    };

    QList<ChatLine>  m_lines;
    QVector<int>     m_cumH;     // m_cumH[i] = doc-Y of top of line i
    QFont            m_font;
    QColor           m_linkColor{QColor("#61afef")};
    QColor           m_cardBg;
    QColor           m_cardBorder;
    QColor           m_subColor;    // subdued text (domain line in preview cards)
    bool             m_atBottom{true};
    bool             m_userScrolledAway{false};
    QString          m_hoveredAnchor;
    SelPoint         m_selAnchor;
    SelPoint         m_selActive;
    bool             m_selecting{false};
    QString          m_clickAnchor;  // anchor under the press point; fired on release if no drag
    int              m_findLine{-1};
    int              m_findFrom{-1};
    int              m_findTo{-1};
    QTimer          *m_relayoutTimer{nullptr}; // deferred off-screen relayout after width change

    static constexpr int kMaxLines   = 2000;
    int              m_maxLines{kMaxLines};
    static constexpr int kVPad       = 2;
    static constexpr int kHPad       = 6;
    static constexpr int kCardIndent = 20;

    int      wrapWidth()  const;
    int      hangWidth()  const;
    int      lineHangW(const ChatLine &line) const;
    int      totalHeight() const;
    void     layoutLine(ChatLine &line) const;
    void     relayoutForWidth(ChatLine &line) const;
    QTextLayout *ensureLayout(const ChatLine &line) const;
    void     scheduleDeferredRelayout();
    void     invalidateHeights();
    void     rebuildCumH();
    void     updateScrollRange();
    int      lineAt(int docY) const;
    QString  anchorAt(const QPoint &vpPos) const;
    SelPoint hitTest(const QPoint &vpPos) const;
    void     setFind(int lineIdx, int from, int to);
    void     drawLine(QPainter &p, const ChatLine &line, int screenY,
                      int selFrom = -1, int selTo = -1,
                      int findFrom = -1, int findTo = -1) const;
    void     drawPreviewCard(QPainter &p, const ChatLine &line, int screenY) const;
    QList<QTextLayout::FormatRange> buildFormatRanges(const ChatLine &line) const;
};
