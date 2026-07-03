#include "chatview.h"
#include "ui/fadescrollbar.h"
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QScroller>
#include <QScrollerProperties>
#include <QWheelEvent>
#include <QPainter>
#include <QScrollBar>
#include <QTextLayout>
#include <QTimer>
#include <QTextLine>
#include <algorithm>
#include <cmath>

ChatView::ChatView(QWidget *parent)
    : QAbstractScrollArea(parent)
{
    setVerticalScrollBar(new FadeScrollBar(Qt::Vertical, this));
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setFrameShape(QFrame::NoFrame);
    viewport()->setMouseTracking(true);
    viewport()->setAutoFillBackground(false);

    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int val) {
        const bool atMax = (val >= verticalScrollBar()->maximum() - 4);
        if (m_userScrolledAway) {
            if (atMax) {
                m_userScrolledAway = false;
                m_atBottom = true;
                emit scrolledAwayFromBottom(false);
            }
        } else {
            const bool wasBottom = m_atBottom;
            m_atBottom = atMax;
            if (wasBottom != m_atBottom)
                emit scrolledAwayFromBottom(!m_atBottom);
        }
        viewport()->update();
        if (val == 0 && !m_lines.isEmpty())
            emit loadOlderRequested();
    });

    connect(verticalScrollBar(), &QScrollBar::rangeChanged, this,
        [this](int, int max) {
            if (m_atBottom && !m_userScrolledAway)
                verticalScrollBar()->setValue(max);
        });

    QScroller::grabGesture(viewport(), QScroller::LeftMouseButtonGesture);
    auto *scroller = QScroller::scroller(viewport());
    QScrollerProperties props = scroller->scrollerProperties();
    props.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy,
                          QVariant::fromValue(QScrollerProperties::OvershootAlwaysOff));
    props.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy,
                          QVariant::fromValue(QScrollerProperties::OvershootAlwaysOff));
    scroller->setScrollerProperties(props);
    connect(scroller, &QScroller::stateChanged, this, [this](QScroller::State state) {
        if (state == QScroller::Dragging && !m_userScrolledAway) {
            m_userScrolledAway = true;
            m_atBottom = false;
            emit scrolledAwayFromBottom(true);
        }
    });
}

// ── Public API ───────────────────────────────────────────────────────────────

void ChatView::appendLine(const ChatLine &line)
{
    const int prevTotal = totalHeight();
    m_lines.append(line);
    if (m_lines.last().role == ChatLineRole::PreviewCard && m_lines.size() >= 2) {
        const auto &prev = m_lines[m_lines.size() - 2];
        if (prev.cachedHangPx > 0)
            m_lines.last().cachedHangPx = prev.cachedHangPx;
    }
    layoutLine(m_lines.last());
    m_cumH.append(prevTotal);

    if (m_lines.size() > kMaxLines) {
        m_lines.removeFirst();
        m_cumH.removeFirst();
        rebuildCumH();
    }

    updateScrollRange();
    viewport()->update();
}

void ChatView::prependLines(QList<ChatLine> lines)
{
    if (lines.isEmpty()) return;

    int addedHeight = 0;
    for (auto &l : lines) {
        layoutLine(l);
        addedHeight += l.cachedH;
    }

    m_lines = lines + m_lines;
    rebuildCumH();
    updateScrollRange();

    verticalScrollBar()->setValue(verticalScrollBar()->value() + addedHeight);
    viewport()->update();
}

void ChatView::replaceLine(const QString &id, const ChatLine &line)
{
    const int idx = findLine(id);
    if (idx < 0) return;
    m_lines[idx] = line;
    layoutLine(m_lines[idx]);
    // Rebuild cumH from idx onward (heights before idx are unchanged)
    for (int i = idx + 1; i < m_lines.size(); ++i)
        m_cumH[i] = m_cumH[i-1] + m_lines[i-1].cachedH;
    updateScrollRange();
    viewport()->update();
}

void ChatView::insertAfter(const QString &id, const ChatLine &line)
{
    ChatLine newLine = line;
    const int idx = findLine(id);
    if (idx >= 0 && newLine.role == ChatLineRole::PreviewCard && m_lines[idx].cachedHangPx > 0)
        newLine.cachedHangPx = m_lines[idx].cachedHangPx;
    layoutLine(newLine);
    const int insertIdx = (idx >= 0) ? idx + 1 : static_cast<int>(m_lines.size());
    m_lines.insert(insertIdx, newLine);
    m_cumH.insert(insertIdx, 0);
    rebuildCumH();
    updateScrollRange();
    viewport()->update();
}

void ChatView::removeLine(const QString &id)
{
    const int idx = findLine(id);
    if (idx < 0) return;
    m_lines.removeAt(idx);
    m_cumH.removeAt(idx);
    // Rebuild cumH from idx onward
    for (int i = idx; i < m_lines.size(); ++i)
        m_cumH[i] = (i == 0) ? 0 : m_cumH[i-1] + m_lines[i-1].cachedH;
    updateScrollRange();
    viewport()->update();
}

void ChatView::clear()
{
    m_lines.clear();
    m_cumH.clear();
    m_paintFirst = -1;
    m_paintLast  = -1;
    m_atBottom = true;
    m_userScrolledAway = false;
    verticalScrollBar()->setRange(0, 0);
    emit scrolledAwayFromBottom(false);
    viewport()->update();
}

void ChatView::setFont(const QFont &f)
{
    m_font = f;
    QWidget::setFont(f);
    viewport()->setFont(f);
    invalidateHeights();
    for (auto &l : m_lines) layoutLine(l);
    rebuildCumH();
    updateScrollRange();
    viewport()->update();
}

void ChatView::setColors(const QColor &text, const QColor &bg,
                          const QColor &link, const QColor &cardBg, const QColor &cardBorder)
{
    QPalette pal = viewport()->palette();
    if (text.isValid())  pal.setColor(QPalette::Text, text);
    if (bg.isValid())    pal.setColor(QPalette::Base, bg);
    viewport()->setPalette(pal);
    if (link.isValid())        m_linkColor  = link;
    if (cardBg.isValid())      m_cardBg     = cardBg;
    if (cardBorder.isValid())  m_cardBorder = cardBorder;
    // Subdued text = 50% blend of text and bg for domain line in preview cards
    if (text.isValid() && bg.isValid())
        m_subColor = QColor::fromRgb((text.red()   + bg.red())   / 2,
                                     (text.green() + bg.green()) / 2,
                                     (text.blue()  + bg.blue())  / 2);
    viewport()->update();
}

bool ChatView::isAtBottom() const
{
    if (m_userScrolledAway) return false;
    const auto *sb = verticalScrollBar();
    return sb->value() >= sb->maximum() - 4;
}

void ChatView::scrollToBottom()
{
    const bool wasAway = m_userScrolledAway;
    m_userScrolledAway = false;
    m_atBottom = true;
    verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    if (wasAway)
        emit scrolledAwayFromBottom(false);
}

void ChatView::scrollToLine(int lineIdx)
{
    if (lineIdx < 0 || lineIdx >= m_lines.size()) return;
    if (m_cumH.size() != m_lines.size()) rebuildCumH();
    verticalScrollBar()->setValue(m_cumH.value(lineIdx, 0));
    m_atBottom = false;
}

int ChatView::findLine(const QString &id) const
{
    if (id.isEmpty()) return -1;
    for (int i = static_cast<int>(m_lines.size()) - 1; i >= 0; --i)
        if (m_lines[i].id == id) return i;
    return -1;
}

// ── Protected events ─────────────────────────────────────────────────────────

void ChatView::paintEvent(QPaintEvent *)
{
    QPainter p(viewport());
    p.setFont(m_font);
    p.fillRect(viewport()->rect(), viewport()->palette().color(QPalette::Base));

    if (m_lines.isEmpty()) return;

    const int scrollY = verticalScrollBar()->value();
    const int vpH     = viewport()->height();

    // Compute ordered selection range
    SelPoint selLo = m_selAnchor, selHi = m_selActive;
    if (selHi < selLo) std::swap(selLo, selHi);

    int i = lineAt(scrollY);
    if (i < 0) i = 0;
    const int first = i;

    for (; i < m_lines.size(); ++i) {
        const int screenY = m_cumH[i] - scrollY;
        if (screenY >= vpH) break;

        int selFrom = -1, selTo = -1;
        if (selLo.valid() && i >= selLo.line && i <= selHi.line) {
            selFrom = (i == selLo.line) ? selLo.pos : 0;
            selTo   = (i == selHi.line) ? selHi.pos : static_cast<int>(m_lines[i].text.size());
            if (selFrom >= selTo) { selFrom = -1; selTo = -1; }
        }
        int findFrom = -1, findTo = -1;
        if (i == m_findLine) { findFrom = m_findFrom; findTo = m_findTo; }
        drawLine(p, m_lines[i], screenY, selFrom, selTo, findFrom, findTo);
    }

    // Evict cached layouts for lines that left the painted window so memory
    // stays bounded to roughly one screenful of layouts.
    const int last = i;
    if (m_paintFirst >= 0) {
        for (int k = m_paintFirst; k < m_paintLast && k < m_lines.size(); ++k)
            if (k < first || k >= last)
                m_lines[k].cachedLayout.reset();
    }
    m_paintFirst = first;
    m_paintLast  = last;
}

void ChatView::resizeEvent(QResizeEvent *e)
{
    QAbstractScrollArea::resizeEvent(e);
    if (e->oldSize().width() == e->size().width()) {
        updateScrollRange();
        return;
    }

    // Width changed: relayout only the visible window now, defer the rest
    // to idle chunks so drag-resizing stays smooth on a full backlog.
    const int scrollY = verticalScrollBar()->value();
    const int vpH     = viewport()->height();
    int i = lineAt(scrollY);
    if (i < 0) i = 0;
    for (; i < m_lines.size(); ++i) {
        if (m_cumH.value(i, 0) - scrollY >= vpH) break;
        relayoutForWidth(m_lines[i]);
    }
    rebuildCumH();
    updateScrollRange();
    scheduleDeferredRelayout();
}

void ChatView::wheelEvent(QWheelEvent *e)
{
    const int lineH = QFontMetrics(m_font).height() + kVPad * 2;
    const int step  = lineH * 3;
    const int delta = e->angleDelta().y();
    if (delta == 0) { e->ignore(); return; }

    if (delta > 0 && !m_userScrolledAway) {
        m_userScrolledAway = true;
        m_atBottom = false;
        emit scrolledAwayFromBottom(true);
    }

    const int pixels = -delta * step / 120;
    auto *sb = verticalScrollBar();
    sb->setValue(sb->value() + pixels);
    e->accept();
}

void ChatView::keyPressEvent(QKeyEvent *e)
{
    if (e->matches(QKeySequence::Copy)) {
        const QString text = selectedText();
        if (!text.isEmpty())
            QApplication::clipboard()->setText(text);
    } else {
        QAbstractScrollArea::keyPressEvent(e);
    }
}

void ChatView::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        // Always start a selection; fire the anchor on release if the mouse didn't move
        m_clickAnchor = anchorAt(e->pos());
        m_selAnchor   = hitTest(e->pos());
        m_selActive   = m_selAnchor;
        m_selecting   = true;
        viewport()->update();
    }
}

void ChatView::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton && m_selecting) {
        m_selecting = false;
        m_selActive = hitTest(e->pos());
        if (selectedText().isEmpty()) {
            // No drag: treat as a plain click — fire anchor if one was under the press
            m_selAnchor = {};
            m_selActive = {};
            if (!m_clickAnchor.isEmpty())
                emit anchorActivated(m_clickAnchor, e->globalPosition().toPoint(), Qt::LeftButton);
        }
        m_clickAnchor.clear();
        viewport()->update();
    }
}

void ChatView::mouseMoveEvent(QMouseEvent *e)
{
    if (m_selecting && (e->buttons() & Qt::LeftButton)) {
        m_selActive = hitTest(e->pos());
        viewport()->update();
    }

    const QString anc = m_selecting ? QString{} : anchorAt(e->pos());
    if (anc != m_hoveredAnchor) {
        m_hoveredAnchor = anc;
        emit anchorHovered(anc);
    }
    viewport()->setCursor(m_selecting || anc.isEmpty() ? Qt::IBeamCursor : Qt::PointingHandCursor);
}

void ChatView::leaveEvent(QEvent *)
{
    if (!m_hoveredAnchor.isEmpty()) {
        m_hoveredAnchor.clear();
        emit anchorHovered({});
    }
    viewport()->setCursor(Qt::ArrowCursor);
}

void ChatView::contextMenuEvent(QContextMenuEvent *e)
{
    QString anc = anchorAt(e->pos());

    // Fall back to the line's msgid so right-clicking anywhere on a message
    // (not just the timestamp) triggers React/Reply.
    if (anc.isEmpty()) {
        const int idx = lineAt(e->pos().y() + verticalScrollBar()->value());
        if (idx >= 0 && idx < m_lines.size()) {
            const ChatLine &ln = m_lines[idx];
            if (ln.role == ChatLineRole::Message && !ln.id.isEmpty())
                anc = "msgid:" + ln.id;
        }
    }

    const QString sel = selectedText();
    if (!sel.isEmpty() && anc.isEmpty()) {
        // Text selected but not on a message line — just Copy.
        QMenu menu(viewport());
        connect(menu.addAction("Copy"), &QAction::triggered,
                this, [sel]{ QApplication::clipboard()->setText(sel); });
        menu.exec(e->globalPos());
    } else {
        // Delegate to the anchor handler; it will add Copy if sel is non-empty.
        emit anchorActivated(anc, e->globalPos(), Qt::RightButton);
    }
    e->accept();
}

// ── Private helpers ──────────────────────────────────────────────────────────

int ChatView::wrapWidth() const
{
    const int sbW = verticalScrollBar()->sizeHint().width();
    const int reserved = viewport()->width() - sbW - kHPad * 2;
    return qMax(1, reserved);
}

int ChatView::hangWidth() const
{
    return QFontMetrics(m_font).horizontalAdvance("00:00  ");
}

int ChatView::lineHangW(const ChatLine &line) const
{
    if (!line.hangIndent) return 0;
    if (line.cachedHangPx > 0) return line.cachedHangPx;
    // Fallback: global timestamp width (used before layoutLine runs)
    return hangWidth();
}

int ChatView::totalHeight() const
{
    if (m_lines.isEmpty()) return 0;
    return m_cumH.last() + m_lines.last().cachedH;
}

void ChatView::invalidateHeights()
{
    for (auto &l : m_lines) {
        l.cachedH    = 0;
        l.cachedHangPx = 0;
        l.layoutWidth = 0;
        l.cachedNaturalW = 0;
        l.visLines.clear();
        l.cachedLayout.reset();
    }
}

void ChatView::rebuildCumH()
{
    m_cumH.resize(m_lines.size());
    if (m_lines.isEmpty()) return;
    m_cumH[0] = 0;
    for (int i = 1; i < m_lines.size(); ++i)
        m_cumH[i] = m_cumH[i-1] + m_lines[i-1].cachedH;
}

void ChatView::updateScrollRange()
{
    const int vh    = viewport()->height();
    const int total = totalHeight();
    verticalScrollBar()->setRange(0, qMax(0, total - vh));
    verticalScrollBar()->setPageStep(vh);
    const int lineH = QFontMetrics(m_font).height() + kVPad * 2;
    verticalScrollBar()->setSingleStep(lineH * 3);
}

int ChatView::lineAt(int docY) const
{
    if (m_cumH.isEmpty()) return -1;
    // upper_bound finds first element > docY; step back one
    auto it = std::upper_bound(m_cumH.constBegin(), m_cumH.constEnd(), docY);
    if (it == m_cumH.constBegin()) return 0;
    --it;
    return static_cast<int>(it - m_cumH.constBegin());
}

QString ChatView::anchorAt(const QPoint &vpPos) const
{
    const int scrollY = verticalScrollBar()->value();
    const int docY    = vpPos.y() + scrollY;
    const int idx     = lineAt(docY);
    if (idx < 0 || idx >= m_lines.size()) return {};

    const ChatLine &line = m_lines[idx];
    if (line.text.isEmpty() || line.visLines.isEmpty()) return {};

    const qreal relY = docY - m_cumH[idx] - kVPad;

    QTextLayout *layout = ensureLayout(line);

    // Find which visual sub-line was clicked
    int visIdx = layout->lineCount() - 1;
    for (int v = 0; v < layout->lineCount(); ++v) {
        const QTextLine vl = layout->lineAt(v);
        if (relY < vl.y() + vl.height()) { visIdx = v; break; }
    }

    const QTextLine tl = layout->lineAt(visIdx);
    if (!tl.isValid()) return {};

    const int charPos = tl.xToCursor(static_cast<qreal>(vpPos.x()), QTextLine::CursorBetweenCharacters);

    for (const auto &seg : line.segments) {
        if (!seg.anchor.isEmpty()
            && charPos >= seg.start
            && charPos < seg.start + seg.length)
            return seg.anchor;
    }
    return {};
}

ChatView::SelPoint ChatView::hitTest(const QPoint &vpPos) const
{
    const int scrollY = verticalScrollBar()->value();
    const int docY    = vpPos.y() + scrollY;
    const int idx     = lineAt(docY);
    if (idx < 0 || idx >= m_lines.size()) return {};

    const ChatLine &line = m_lines[idx];
    if (line.text.isEmpty() || line.visLines.isEmpty())
        return {idx, 0};

    const qreal relY = docY - m_cumH[idx] - kVPad;

    QTextLayout *layout = ensureLayout(line);

    int visIdx = layout->lineCount() - 1;
    for (int v = 0; v < layout->lineCount(); ++v) {
        const QTextLine vl = layout->lineAt(v);
        if (relY < vl.y() + vl.height()) { visIdx = v; break; }
    }

    const QTextLine tl = layout->lineAt(visIdx);
    if (!tl.isValid()) return {idx, 0};

    const int charPos = tl.xToCursor(static_cast<qreal>(vpPos.x()), QTextLine::CursorBetweenCharacters);
    return {idx, charPos};
}

QString ChatView::selectedText() const
{
    SelPoint lo = m_selAnchor, hi = m_selActive;
    if (!lo.valid()) return {};
    if (hi < lo) std::swap(lo, hi);
    QString result;
    for (int i = lo.line; i <= hi.line && i < m_lines.size(); ++i) {
        const QString &text = m_lines[i].text;
        const int from = (i == lo.line) ? lo.pos : 0;
        const int to   = (i == hi.line) ? hi.pos : static_cast<int>(text.size());
        if (to <= from) continue;
        if (!result.isEmpty()) result += '\n';
        result += text.mid(from, to - from);
    }
    return result;
}

bool ChatView::findText(const QString &text, bool backward)
{
    if (text.isEmpty() || m_lines.isEmpty()) { clearFind(); return false; }

    const int n         = static_cast<int>(m_lines.size());
    const int startLine = (m_findLine >= 0 && m_findLine < n)
        ? m_findLine
        : qBound(0, lineAt(verticalScrollBar()->value()), n - 1);

    if (backward) {
        for (int k = 0; k < n; ++k) {
            const int i    = ((startLine - k) % n + n) % n;
            const int upTo = (k == 0 && m_findFrom > 0) ? m_findFrom - 1 : static_cast<int>(m_lines[i].text.size());
            const int pos  = static_cast<int>(m_lines[i].text.lastIndexOf(text, upTo, Qt::CaseInsensitive));
            if (pos >= 0) { setFind(i, pos, pos + static_cast<int>(text.size())); return true; }
        }
    } else {
        for (int k = 0; k < n; ++k) {
            const int i    = (startLine + k) % n;
            const int from = (k == 0 && m_findTo > 0) ? m_findTo : 0;
            const int pos  = static_cast<int>(m_lines[i].text.indexOf(text, from, Qt::CaseInsensitive));
            if (pos >= 0) { setFind(i, pos, pos + static_cast<int>(text.size())); return true; }
        }
    }

    clearFind();
    return false;
}

void ChatView::clearFind()
{
    m_findLine = -1;
    m_findFrom = -1;
    m_findTo   = -1;
    viewport()->update();
}

void ChatView::setFind(int lineIdx, int from, int to)
{
    m_findLine = lineIdx;
    m_findFrom = from;
    m_findTo   = to;

    if (lineIdx >= 0 && lineIdx < m_cumH.size()) {
        const int lineTop    = m_cumH[lineIdx];
        const int lineBottom = lineTop + m_lines[lineIdx].cachedH;
        const int vpH        = viewport()->height();
        const int scrollY    = verticalScrollBar()->value();
        if (lineTop < scrollY || lineBottom > scrollY + vpH)
            verticalScrollBar()->setValue(qMax(0, lineTop - vpH / 3));
    }
    viewport()->update();
}

void ChatView::layoutLine(ChatLine &line) const
{
    if (line.cachedH > 0) return;

    const int fh = QFontMetrics(m_font).height();

    if (line.role == ChatLineRole::PreviewCard) {
        const int imgH  = line.image.isNull() ? 0 : (line.image.height() + 4);
        const int textH = fh * 2 + 4;
        const int cardX = kHPad + (line.cachedHangPx > 0 ? line.cachedHangPx : hangWidth());
        line.cachedH = kVPad + textH + imgH + kVPad;
        line.visLines = { {0, static_cast<int>(line.text.size()), static_cast<qreal>(cardX),
                            0, static_cast<qreal>(wrapWidth() - cardX + kHPad), static_cast<qreal>(line.cachedH)} };
        line.layoutWidth = wrapWidth();
        return;
    }

    if (line.text.isEmpty()) {
        line.cachedH = fh + kVPad * 2;
        line.visLines = { {0, 0, static_cast<qreal>(kHPad), static_cast<qreal>(kVPad), static_cast<qreal>(wrapWidth()), static_cast<qreal>(fh)} };
        line.layoutWidth = wrapWidth();
        return;
    }

    QTextLayout layout;
    layout.setFont(m_font);
    layout.setText(line.text);
    layout.setFormats(buildFormatRanges(line));
    QTextOption opt;
    opt.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    layout.setTextOption(opt);

    // Compute per-message hang indent (respects bold nick width)
    if (line.hangIndent && line.hangIndentChars > 0 && line.cachedHangPx == 0) {
        QTextLayout pl;
        pl.setFont(m_font);
        pl.setText(line.text.left(line.hangIndentChars));
        QList<QTextLayout::FormatRange> pfmts;
        for (const auto &seg : line.segments) {
            if (seg.start >= line.hangIndentChars) break;
            QTextLayout::FormatRange fr;
            fr.start  = seg.start;
            fr.length = qMin(seg.length, line.hangIndentChars - seg.start);
            fr.format = seg.format;
            pfmts.append(fr);
        }
        pl.setFormats(pfmts);
        pl.beginLayout();
        QTextLine ptl = pl.createLine();
        if (ptl.isValid()) {
            ptl.setNumColumns(line.hangIndentChars);
            line.cachedHangPx = static_cast<int>(std::ceil(ptl.naturalTextWidth()));
        }
        pl.endLayout();
    }

    const int wrap  = wrapWidth();
    const int hangW = lineHangW(line);

    line.visLines.clear();
    layout.beginLayout();
    qreal y    = kVPad;
    bool  first = true;
    qreal naturalW = 0;
    while (true) {
        QTextLine tl = layout.createLine();
        if (!tl.isValid()) break;
        const int indent = (!first && line.hangIndent) ? hangW : 0;
        tl.setLineWidth(qMax(1, wrap - indent));
        tl.setPosition(QPointF(kHPad + indent, y));
        if (first) naturalW = tl.naturalTextWidth();

        ChatLine::VisLine vl;
        vl.charStart = tl.textStart();
        vl.charEnd   = tl.textStart() + tl.textLength();
        vl.x = kHPad + indent;
        vl.y = y;
        vl.w = tl.width();
        vl.h = tl.height();
        line.visLines.append(vl);

        y    += tl.height();
        first = false;
    }
    layout.endLayout();

    line.cachedH = qMax(1, static_cast<int>(std::ceil(y))) + kVPad;
    line.layoutWidth    = wrap;
    // Natural width only meaningful for the single-visual-line case; it lets
    // relayoutForWidth() skip lines that still fit after a width change.
    line.cachedNaturalW = (line.visLines.size() == 1) ? naturalW : 0;
}

// Relayout a line for the current wrap width, skipping the QTextLayout pass
// when the height cannot have changed: preview cards and empty lines have
// width-independent heights, and single-visual-line messages that still fit
// the new width keep their height.
void ChatView::relayoutForWidth(ChatLine &line) const
{
    const int wrap = wrapWidth();
    if (line.cachedH > 0 && line.layoutWidth == wrap) return;
    line.cachedLayout.reset();

    if (line.cachedH > 0) {
        if (line.role == ChatLineRole::PreviewCard || line.text.isEmpty()) {
            line.layoutWidth = wrap;
            return;
        }
        if (line.visLines.size() == 1 && line.cachedNaturalW > 0
            && line.cachedNaturalW <= wrap) {
            line.layoutWidth = wrap;
            return;
        }
    }

    line.cachedH = 0;
    line.visLines.clear();
    layoutLine(line);
}

// Process remaining stale lines in chunks on a zero-interval timer, keeping
// the top visible line anchored while cumulative heights are rebuilt.
void ChatView::scheduleDeferredRelayout()
{
    if (!m_relayoutTimer) {
        m_relayoutTimer = new QTimer(this);
        m_relayoutTimer->setSingleShot(true);
        m_relayoutTimer->setInterval(0);
        connect(m_relayoutTimer, &QTimer::timeout, this, [this]{
            const int wrap = wrapWidth();
            constexpr int kChunk = 256;

            const int scrollY   = verticalScrollBar()->value();
            const int anchorIdx = lineAt(scrollY);
            const int anchorOff = (anchorIdx >= 0 && anchorIdx < m_cumH.size())
                                ? scrollY - m_cumH[anchorIdx] : 0;

            int  processed = 0;
            bool more      = false;
            for (auto &l : m_lines) {
                if (l.cachedH > 0 && l.layoutWidth == wrap) continue;
                relayoutForWidth(l);
                if (++processed >= kChunk) { more = true; break; }
            }
            if (processed > 0) {
                rebuildCumH();
                updateScrollRange();
                if (!m_atBottom && anchorIdx >= 0 && anchorIdx < m_cumH.size())
                    verticalScrollBar()->setValue(m_cumH[anchorIdx] + anchorOff);
                viewport()->update();
            }
            if (more) m_relayoutTimer->start();
        });
    }
    m_relayoutTimer->start();
}

QList<QTextLayout::FormatRange> ChatView::buildFormatRanges(const ChatLine &line) const
{
    QList<QTextLayout::FormatRange> result;
    result.reserve(line.segments.size());
    for (const auto &seg : line.segments) {
        if (seg.length <= 0) continue;
        QTextLayout::FormatRange fr;
        fr.start  = seg.start;
        fr.length = seg.length;
        fr.format = seg.format;
        // URLs and preview links get link color + underline if not already set
        if (!seg.anchor.isEmpty()
            && (seg.anchor.startsWith("url:")     ||
                seg.anchor.startsWith("preview:"))) {
            if (!fr.format.hasProperty(QTextFormat::ForegroundBrush))
                fr.format.setForeground(m_linkColor);
            fr.format.setFontUnderline(true);
        } else if (!seg.anchor.isEmpty()
                   && (seg.anchor.startsWith("nick:")  ||
                       seg.anchor.startsWith("evgrp:"))) {
            // Nicks and event group toggles get link color if no explicit color set
            if (!fr.format.hasProperty(QTextFormat::ForegroundBrush))
                fr.format.setForeground(m_linkColor);
        }
        result.append(fr);
    }
    return result;
}

// Build (or reuse) the QTextLayout for a line at the current wrap width.
// The layout is cached on the line so repaints during scrolling and selection
// drags don't re-run text itemization and line breaking; paintEvent evicts
// caches for lines that scroll out of view.
QTextLayout *ChatView::ensureLayout(const ChatLine &line) const
{
    const int wrap = wrapWidth();
    if (line.cachedLayout && line.layoutWidth == wrap)
        return line.cachedLayout.get();

    auto layout = std::make_shared<QTextLayout>();
    layout->setFont(m_font);
    layout->setText(line.text);
    layout->setFormats(buildFormatRanges(line));
    QTextOption opt;
    opt.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    if (line.role == ChatLineRole::StatusLine)
        opt.setAlignment(Qt::AlignCenter);
    layout->setTextOption(opt);

    const int hangW = lineHangW(line);

    layout->beginLayout();
    qreal y    = kVPad;
    bool  first = true;
    while (true) {
        QTextLine tl = layout->createLine();
        if (!tl.isValid()) break;
        const int indent = (!first && line.hangIndent) ? hangW : 0;
        tl.setLineWidth(qMax(1, wrap - indent));
        tl.setPosition(QPointF(kHPad + indent, y));
        y    += tl.height();
        first = false;
    }
    layout->endLayout();

    line.cachedLayout = std::move(layout);
    line.layoutWidth  = wrap;
    return line.cachedLayout.get();
}

void ChatView::drawLine(QPainter &p, const ChatLine &line, int screenY,
                         int selFrom, int selTo, int findFrom, int findTo) const
{
    if (line.role == ChatLineRole::PreviewCard) {
        drawPreviewCard(p, line, screenY);
        return;
    }

    if (line.text.isEmpty()) return;

    QTextLayout *layout = ensureLayout(line);

    QList<QTextLayout::FormatRange> selections;
    if (selFrom >= 0 && selTo > selFrom) {
        QTextLayout::FormatRange sr;
        sr.start  = selFrom;
        sr.length = selTo - selFrom;
        sr.format.setBackground(viewport()->palette().color(QPalette::Highlight));
        sr.format.setForeground(viewport()->palette().color(QPalette::HighlightedText));
        selections.append(sr);
    }
    if (findFrom >= 0 && findTo > findFrom) {
        QTextLayout::FormatRange fr;
        fr.start  = findFrom;
        fr.length = findTo - findFrom;
        fr.format.setBackground(QColor("#ffdd57"));
        fr.format.setForeground(QColor("#000000"));
        selections.append(fr);
    }

    p.setPen(viewport()->palette().color(QPalette::Text));
    layout->draw(&p, QPointF(0, screenY), selections);
}

void ChatView::drawPreviewCard(QPainter &p, const ChatLine &line, int screenY) const
{
    const int x    = kHPad + (line.cachedHangPx > 0 ? line.cachedHangPx : hangWidth());
    const int maxW = viewport()->width() - kHPad - x;
    if (line.text.isEmpty() && line.image.isNull()) return;

    QFont boldFont = m_font;
    boldFont.setBold(true);
    const QFontMetrics fmB(boldFont);
    const QFontMetrics fm(m_font);

    const qsizetype newline = line.text.indexOf('\n');
    const QString titleStr  = newline >= 0 ? line.text.left(newline) : line.text;
    const QString domainStr = newline >= 0 ? line.text.mid(newline + 1) : QString();

    int y = screenY + kVPad;

    // Title: bold, text color, one line, elided
    p.setFont(boldFont);
    p.setPen(viewport()->palette().color(QPalette::Text));
    p.drawText(x, y + fmB.ascent(), fmB.elidedText(titleStr, Qt::ElideRight, maxW));
    y += fmB.height() + 2;

    // Domain: subdued, one line, elided
    if (!domainStr.isEmpty()) {
        p.setFont(m_font);
        p.setPen(m_subColor.isValid() ? m_subColor : viewport()->palette().color(QPalette::Text));
        p.drawText(x, y + fm.ascent(), fm.elidedText(domainStr, Qt::ElideRight, maxW));
        y += fm.height() + 4;
    }

    // Thumbnail: stacked below text, left-aligned
    if (!line.image.isNull())
        p.drawPixmap(x, y, line.image);

    p.setFont(m_font);
}
