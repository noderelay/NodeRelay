#include "channelpane.h"
#include "ui/chatview.h"
#include "ui/fadescrollbar.h"
#include "ui/uistyle.h"
#include "ui/searchbar.h"
#include "ui/nickfilteredit.h"
#include "ui/nicklistmodel.h"
#include "ui/chromepanel.h"
#include "ui/dropframe.h"
#include "ui/elidedlabel.h"
#include "ui/menuicons.h"
#include "ui/splittergrip.h"

#include <QListView>
#include <QScroller>
#include <QPlainTextEdit>
#include <QKeyEvent>
#include <QLabel>
#include <QPainter>
#include <QLineEdit>
#include <QToolButton>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QFont>
#include <QSizePolicy>
#include <QApplication>
#include <QMouseEvent>
#include <QDesktopServices>
#include <QUrl>
#include <QDrag>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QShortcut>
#include <QMainWindow>
#include <QAbstractButton>

// Scale of the pane snapshot that follows the cursor during a header drag.
static constexpr qreal kDragGhostScale = 0.5;

ChannelPane::ChannelPane(const ServerId &host, const BufferId &channel, QWidget *parent)
    : QWidget(parent), m_host(host), m_channel(channel)
{
    // Paint a themed background (bufferBg) so the compose strip — typing
    // indicator + input bar — sits on the chat colour like the main window.
    setObjectName("channelPane");
    setAttribute(Qt::WA_StyledBackground, true);
    setAcceptDrops(true);

    auto *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(0, 0, 0, 0);
    vbox->setSpacing(0);

    // Header
    m_header = new QWidget;
    m_header->setObjectName("paneHeader");
    auto *hbox = new QHBoxLayout(m_header);
    hbox->setContentsMargins(6, 3, 4, 3);
    hbox->setSpacing(6);

    m_topicToggle = new QToolButton;
    m_topicToggle->setObjectName("topicToggle");
    m_topicToggle->setText({});
    m_topicToggle->setIconSize(QSize(14, 14));
    m_topicToggle->setAutoRaise(false);
    m_topicToggle->setCheckable(true);

    m_nameLabel = new ElidedLabel(m_channel.str());
    m_nameLabel->setObjectName("paneChannelLabel");
    QFont f = m_nameLabel->font();
    f.setBold(true);
    m_nameLabel->setFont(f);

    m_popOutBtn = new QToolButton;
    m_popOutBtn->setFixedSize(28, 28);
    m_popOutBtn->setIconSize(QSize(24, 24));
    m_popOutBtn->setAutoRaise(true);
    m_popOutBtn->setStyleSheet(UiStyle::headerButtonStyle());
    m_popOutBtn->setToolTip(QStringLiteral("Open in a window"));
    connect(m_popOutBtn, &QToolButton::clicked, this, &ChannelPane::popOutRequested);

    m_searchBtn = new QToolButton;
    m_searchBtn->setFixedSize(28, 28);
    m_searchBtn->setIconSize(QSize(24, 24));
    m_searchBtn->setAutoRaise(true);
    m_searchBtn->setStyleSheet(UiStyle::headerButtonStyle());
    m_searchBtn->setToolTip(QStringLiteral("Search (Ctrl+F)"));
    connect(m_searchBtn, &QToolButton::clicked, this, &ChannelPane::toggleSearch);

    // Reveal button — header row, right of the search glass; shown only
    // while the user list is collapsed.
    m_nickRevealBtn = new QToolButton;
    m_nickRevealBtn->setFixedSize(28, 28);
    m_nickRevealBtn->setIconSize(QSize(20, 20));
    m_nickRevealBtn->setAutoRaise(true);
    m_nickRevealBtn->setStyleSheet(UiStyle::headerButtonStyle());
    m_nickRevealBtn->setToolTip(QStringLiteral("Show user list"));
    m_nickRevealBtn->hide();

    m_closeBtn = new QToolButton;
    m_closeBtn->setText(QStringLiteral("✕"));
    m_closeBtn->setFixedSize(16, 16);
    m_closeBtn->setStyleSheet(
        "QToolButton { background: transparent; border: none; padding: 0px; }"
        "QToolButton:hover { color: palette(highlight); }"
    );
    connect(m_closeBtn, &QToolButton::clicked, this, &ChannelPane::closeRequested);

    hbox->addWidget(m_topicToggle);
    hbox->addWidget(m_nameLabel, 1);
    hbox->addWidget(m_popOutBtn);
    hbox->addWidget(m_searchBtn);
    hbox->addWidget(m_nickRevealBtn);
    hbox->addWidget(m_closeBtn);
    // Added to the chat column further down, not here: the header has to stop
    // at the user list's edge like the main window's does (mainwindow_setup:
    // chatLeftVbox), or its buttons sit above the user list instead.

    // The header is the drag handle for the whole pane — show that on hover.
    // The buttons sitting in it are click targets, so they keep their own
    // cursor rather than inheriting the grab hand.
    m_header->setCursor(Qt::OpenHandCursor);
    for (QToolButton *b : {m_topicToggle, m_popOutBtn, m_searchBtn, m_nickRevealBtn, m_closeBtn})
        b->setCursor(Qt::PointingHandCursor);

    m_header->installEventFilter(this);
    for (auto *w : m_header->findChildren<QWidget*>(Qt::FindDirectChildrenOnly))
        w->installEventFilter(this);

    // Topic bar
    m_topicBar = new QWidget;
    m_topicBar->setObjectName("topicDisplay");
    m_topicBar->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    auto *tbhbox = new QHBoxLayout(m_topicBar);
    tbhbox->setContentsMargins(8, 4, 8, 4);
    m_topicText = new QLabel;
    m_topicText->setObjectName("topicText");
    m_topicText->setWordWrap(true);
    m_topicText->setTextFormat(Qt::RichText);
    m_topicText->setTextInteractionFlags(Qt::TextBrowserInteraction);
    m_topicText->setOpenExternalLinks(false);
    connect(m_topicText, &QLabel::linkActivated, this, [](const QString &link){
        const QUrl u(link);
        const QString s = u.scheme().toLower();
        if (s == "http" || s == "https")
            QDesktopServices::openUrl(u);
    });
    m_topicText->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    // Explicit minimum so an unbreakable topic word (a long URL) can't pin
    // the pane splitter; the label just clips when squeezed that far.
    m_topicText->setMinimumWidth(1);
    tbhbox->addWidget(m_topicText);
    m_topicBar->hide(); // added to the chat column below, next to the body splitter

    connect(m_topicToggle, &QToolButton::toggled, this, [this](bool on){
        m_topicBar->setVisible(on);
    });

    // Chat view
    m_chatView = new ChatView;

    m_nickList = new QListView;
    FadeScrollBar::attachOverlay(m_nickList);   // floats — no reserved gutter
    m_nickList->setSpacing(0);
    m_nickList->setUniformItemSizes(true);
    m_nickList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    QScroller::grabGesture(m_nickList->viewport(), QScroller::LeftMouseButtonGesture);

    // Nick panel header — same widgets as the main window's user list
    m_nickToggleBtn = new QToolButton;
    m_nickToggleBtn->setFixedSize(28, 28);
    m_nickToggleBtn->setIconSize(QSize(20, 20));
    m_nickToggleBtn->setAutoRaise(true);
    m_nickToggleBtn->setToolTip(QStringLiteral("Hide user list"));

    m_nickGroupsIcon = new QLabel;
    m_nickGroupsIcon->setObjectName("nickGroupsIcon");
    m_nickGroupsIcon->setContentsMargins(4, 0, 2, 0);
    m_nickGroupsIcon->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);

    m_nickCountLabel = new QLabel(QStringLiteral("0"));
    m_nickCountLabel->setObjectName("nickCountLabel");
    m_nickCountLabel->setContentsMargins(0, 0, 4, 0);
    m_nickCountLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

    auto *nickHeader = new ChromePanel;
    m_nickHeader = nickHeader;
    nickHeader->setObjectName("nickPanelHeader");
    auto *nhbox = new QHBoxLayout(nickHeader);
    nhbox->setContentsMargins(2, 0, 2, 0);
    nhbox->setSpacing(2);
    nhbox->addWidget(m_nickToggleBtn);
    nhbox->addWidget(m_nickGroupsIcon);
    nhbox->addSpacing(2);
    nhbox->addWidget(m_nickCountLabel);
    nhbox->addStretch(1);

    m_nickFilter = new NickFilterEdit; // model attached via setNickModel()

    m_nickWrapper = new ChromePanel;
    m_nickWrapper->setObjectName("nickPanel");
    // Same drag floor as the main window's nick panel (see mainwindow_setup).
    m_nickWrapper->setMinimumWidth(112);
    auto *nwvbox = new QVBoxLayout(m_nickWrapper);
    nwvbox->setContentsMargins(0, 0, 0, 0);
    nwvbox->setSpacing(0);
    nwvbox->addWidget(nickHeader);
    nwvbox->addWidget(m_nickFilter);
    // No trailing stretch — see the main window's nick panel for why.
    nwvbox->addWidget(m_nickList, 1);

    connect(m_nickToggleBtn, &QToolButton::clicked, this, [this]{
        m_nickWrapper->hide();
        m_nickRevealBtn->show();
    });
    connect(m_nickRevealBtn, &QToolButton::clicked, this, [this]{
        m_nickRevealBtn->hide();
        m_nickWrapper->show();
    });

    // Chat column: everything except the user list, so the list runs the
    // full pane height and the compose strip ends at its edge.
    auto *chatCol  = new QWidget;
    auto *ccVbox   = new QVBoxLayout(chatCol);
    ccVbox->setContentsMargins(0, 0, 0, 0);
    ccVbox->setSpacing(0);
    ccVbox->addWidget(m_header);
    ccVbox->addWidget(m_topicBar);
    ccVbox->addWidget(m_chatView, 1);

    auto *bodySplitter = new QSplitter(Qt::Horizontal);
    m_bodySplitter = bodySplitter;
    bodySplitter->setObjectName("paneBodySplitter");
    // Backdrop behind the nick panel's rounded top corners.
    bodySplitter->setAttribute(Qt::WA_StyledBackground, true);
    bodySplitter->setHandleWidth(0);
    // Collapsing would snap past the nick panel's minimum to 0 — same trap
    // #134 closed on the main window's splitters. Hide/show is the collapse
    // toggle's job.
    bodySplitter->setChildrenCollapsible(false);
    bodySplitter->addWidget(chatCol);
    bodySplitter->addWidget(m_nickWrapper);
    bodySplitter->setStretchFactor(0, 1);
    bodySplitter->setStretchFactor(1, 0);
    bodySplitter->setSizes({999, 120});
    vbox->addWidget(bodySplitter, 1);
    // Widen the resize grab area past the visible gap; grows toward the
    // chat column, matching the main window's grips (see SplitterGrip).
    new SplitterGrip(bodySplitter, 1, kGripExtra, 0);

    // Search bar (hidden until the magnifier is clicked)
    m_searchBar = new SearchBar(m_chatView);
    ccVbox->addWidget(m_searchBar);

    // Typing indicator (hidden until someone is typing)
    m_typingLabel = new QLabel;
    m_typingLabel->setObjectName("typingLabel");
    m_typingLabel->setContentsMargins(8, 2, 8, 2);
    m_typingLabel->hide();
    ccVbox->addWidget(m_typingLabel);

    // Input bar
    auto *inputBar = new QWidget;
    inputBar->setObjectName("inputBar");
    auto *ibox = new QHBoxLayout(inputBar);
    ibox->setContentsMargins(4, 3, 4, 8);
    ibox->setSpacing(4);
    m_nickPrefix = new QLabel;
    m_nickPrefix->setStyleSheet("font-weight: bold; padding-right: 4px;");
    m_input = new QPlainTextEdit;
    m_input->setPlaceholderText("Type a message...");
    m_input->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    m_input->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_input->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_input->document()->setDocumentMargin(2);
    m_input->installEventFilter(this);
    m_input->viewport()->installEventFilter(this); // seam-guard re-assert hook
    // Active IRC format modes, floating bottom-left inside the input — the
    // same badge the main view carries, so Ctrl+B in a pane shows something.
    m_formatIndicator = new QLabel(m_input);
    m_formatIndicator->setObjectName("formatIndicator");
    m_formatIndicator->setStyleSheet(
        "color: rgba(200,200,200,1.0); font-size: 13px; padding: 1px 5px;"
        "background: rgba(120,120,120,0.25); border-radius: 4px;");
    m_formatIndicator->hide();

    ibox->addWidget(m_nickPrefix);
    ibox->addWidget(m_input, 1);
    ccVbox->addWidget(inputBar);
    updateInputHeight();

    connect(m_input, &QPlainTextEdit::textChanged, this, [this]{
        updateInputHeight();
        // Pin the view to the freshest line; see the matching block in
        // MainWindow's input (inputbar.cpp) for why this is explicit.
        if (m_input->textCursor().atEnd()) {
            auto *vb = m_input->verticalScrollBar();
            vb->setValue(vb->maximum());
        }
    });
    // The scroll range updates lazily, after textChanged — a pin that ran
    // against the stale range was clamped to 0 and never retried, leaving
    // the previous wrapped line on show. Re-pin when the range lands.
    connect(m_input->verticalScrollBar(), &QAbstractSlider::rangeChanged,
            this, [this](int, int max){
        if (m_input->textCursor().atEnd())
            m_input->verticalScrollBar()->setValue(max);
    });
}

void ChannelPane::setNick(const QString &nick)
{
    if (m_nickPrefix) m_nickPrefix->setText(nick);
}

void ChannelPane::setNickVisible(bool visible)
{
    if (m_nickPrefix) m_nickPrefix->setVisible(visible);
}

// ChromePanel fills — see MainWindow::applyPanelChrome for why these don't
// rely on stylesheet background painting.
void ChannelPane::setNickChrome(const QString &bg, bool rounded)
{
    const QColor c(bg);
    const int gap = rounded ? kPanelGap : 0;
    if (m_nickWrapper) {
        auto *wrapper = static_cast<ChromePanel *>(m_nickWrapper);
        wrapper->setFill(c, rounded, /*roundedBottom=*/rounded);
        wrapper->setTopInset(gap);
        wrapper->setBottomInset(gap);
        wrapper->setRightInset(gap); // window-facing edge of the user list
        if (m_nickWrapper->layout())
            m_nickWrapper->layout()->setContentsMargins(0, gap, gap, gap);
    }
    if (m_nickHeader)  static_cast<ChromePanel *>(m_nickHeader)->setFill(c, rounded);
    if (m_bodySplitter) m_bodySplitter->setHandleWidth(gap);
}

void ChannelPane::setNickPanelIcons(const QIcon &hide, const QIcon &reveal, const QPixmap &groups)
{
    if (m_nickToggleBtn)  m_nickToggleBtn->setIcon(hide);
    if (m_nickRevealBtn)  m_nickRevealBtn->setIcon(reveal);
    if (m_nickGroupsIcon) m_nickGroupsIcon->setPixmap(groups);
}

void ChannelPane::setNickPanelFont(const QFont &f)
{
    guardFont(m_nickWrapper, f);
    guardFont(m_nickCountLabel, f);
}

void ChannelPane::setNickCount(int count)
{
    if (!m_nickCountLabel) return;
    const QString countStr = QString::number(count);
    m_nickCountLabel->setText(countStr);
    m_nickCountLabel->setToolTip(countStr + " users");
}

void ChannelPane::setNickModel(NickListModel *model)
{
    m_nickModel = model;
    model->setParent(this);
    m_nickList->setModel(model);
    m_nickFilter->setModel(model);
}

void ChannelPane::setFormatBadge(const QString &html)
{
    if (!m_formatIndicator) return;
    if (html.isEmpty()) { m_formatIndicator->hide(); return; }
    m_formatIndicator->setText(html);
    m_formatIndicator->adjustSize();
    m_formatIndicator->move(4, m_input->height() - m_formatIndicator->height() - 3);
    m_formatIndicator->raise();
    m_formatIndicator->show();
}

void ChannelPane::clearNickFilter()
{
    if (m_nickFilter) m_nickFilter->clear();
}

void ChannelPane::setCloseIcon(const QIcon &icon)
{
    if (!m_closeBtn) return;
    m_closeBtn->setText({});
    m_closeBtn->setIcon(icon);
    m_closeBtn->setIconSize(QSize(14, 14));
    m_closeBtn->setToolTip(QStringLiteral("Close window"));
}

void ChannelPane::setSearchIcon(const QIcon &icon)
{
    if (m_searchBtn) m_searchBtn->setIcon(icon);
}

void ChannelPane::setPopOutIcon(const QIcon &icon)
{
    if (m_popOutBtn) m_popOutBtn->setIcon(icon);
}

void ChannelPane::setPopOutVisible(bool visible)
{
    if (m_popOutBtn) m_popOutBtn->setVisible(visible);
}

void ChannelPane::setTyping(const QString &text)
{
    // Text only — the label keeps its (reserved) space so the input bar
    // doesn't jump. Overall visibility is governed by setTypingEnabled().
    if (m_typingLabel) m_typingLabel->setText(text);
}

void ChannelPane::setTypingEnabled(bool on)
{
    if (m_typingLabel) m_typingLabel->setVisible(on);
}

void ChannelPane::setTypingFont(const QFont &f)
{
    guardFont(m_typingLabel, f);
}

// Repolishes (reparenting under the app stylesheet — QTBUG-45332) reset
// programmatic fonts to the app default, and on some platforms that happens
// asynchronously after the pane is laid out. Remember every font we assign
// and re-assert it whenever a FontChange deviates from it.
void ChannelPane::guardFont(QWidget *w, const QFont &f)
{
    if (!w) return;
    m_fontGuards[w] = f;
    w->setFont(f);
    w->installEventFilter(this);
}

// Fractional-scale hairline guard — see MainWindow::updateInputViewportFill.
void ChannelPane::setInputBase(const QColor &bg)
{
    if (!m_input || !bg.isValid()) return;
    m_inputBase = bg;
    QPalette vp = m_input->viewport()->palette();
    vp.setColor(QPalette::Base, bg);
    m_input->viewport()->setPalette(vp);
    m_input->viewport()->setAutoFillBackground(true);
}

void ChannelPane::setInputFont(const QFont &nickFont, const QFont &inputFont)
{
    guardFont(m_nickPrefix, nickFont);
    if (m_input) {
        guardFont(m_input, inputFont);
        updateInputHeight();
    }
}

void ChannelPane::setChatFont(const QFont &f)
{
    if (!m_chatView) return;
    m_fontGuards[m_chatView] = f;
    m_chatView->setChatFont(f); // also fonts the viewport
    m_chatView->installEventFilter(this);
}

void ChannelPane::setNickListFont(const QFont &f)
{
    guardFont(m_nickList, f);
}

// Auto-resize: 1 to 4 lines. Polish first — the stylesheet's input padding
// only lands in contentsMargins() once the widget is polished, and measuring
// before that undersizes the box.
void ChannelPane::updateInputHeight()
{
    if (!m_input) return;
    m_input->ensurePolished();
    const int lineH   = m_input->fontMetrics().lineSpacing();
    const int margins = m_input->contentsMargins().top() + m_input->contentsMargins().bottom() + 8;
    const int lines   = qMin(4, static_cast<int>(m_input->toPlainText().count('\n')) + 1);
    m_input->setFixedHeight(lines * lineH + margins);
}

void ChannelPane::setTopicFont(const QFont &f)
{
    m_topicFontPt = f.pointSize();
    if (m_topicText) {
        guardFont(m_topicText, f);
        if (!m_rawTopicHtml.isEmpty())
            m_topicText->setText(
                QString("<span style='font-size:%1pt;'>%2</span>").arg(m_topicFontPt).arg(m_rawTopicHtml));
    }
}

void ChannelPane::setTopic(const QString &html)
{
    m_rawTopicHtml = html;
    if (m_topicText) {
        const QString sized = html.isEmpty()
            ? html
            : QString("<span style='font-size:%1pt;'>%2</span>").arg(m_topicFontPt).arg(html);
        m_topicText->setText(sized);
    }
    if (m_topicBar && m_topicToggle) {
        const bool hasTopic = !html.isEmpty();
        m_topicToggle->setChecked(hasTopic);
        m_topicBar->setVisible(hasTopic);
    }
}

void ChannelPane::setTopicIcon(const QIcon &collapsed, const QIcon &expanded)
{
    if (!m_topicToggle) return;
    m_topicToggle->setIcon(m_topicToggle->isChecked() ? expanded : collapsed);
    disconnect(m_topicIconConn);
    m_topicIconConn = connect(m_topicToggle, &QToolButton::toggled, this,
                              [this, collapsed, expanded](bool on){
        m_topicToggle->setIcon(on ? expanded : collapsed);
    });
}

// Swap the pane onto another buffer. Only the pane's own identity and header
// are touched here — the chat view, nick list, topic and typing line are
// refilled by MainWindow, which owns the model lookups.
void ChannelPane::retarget(const ServerId &host, const BufferId &channel)
{
    m_host    = host;
    m_channel = channel;
    if (m_nameLabel) m_nameLabel->setText(channel.str());
    setTopic({});
    setTyping({});
    // Matches and the query belong to the buffer we just left.
    if (m_searchBar && m_searchBar->isVisible())
        m_searchBar->dismiss();
}

void ChannelPane::toggleSearch()
{
    if (m_searchBar->isVisible()) m_searchBar->dismiss();
    else m_searchBar->open();
}

// Only for popped-out panes: a window-scoped Ctrl+F. Docked panes must NOT
// have one — it would collide with the main window's Ctrl+F shortcut and
// Qt would treat every press as ambiguous. The main window routes Ctrl+F
// to docked panes by focus instead.
void ChannelPane::enableSearchShortcut()
{
    if (m_findShortcut) return;
    m_findShortcut = new QShortcut(QKeySequence::Find, this);
    m_findShortcut->setContext(Qt::WindowShortcut);
    connect(m_findShortcut, &QShortcut::activated, this, &ChannelPane::toggleSearch);
}

// Edge bands are a quarter of the pane, floored so a narrow pane still has a
// grabbable edge and capped so a wide one keeps a large middle for the swap.
static QSize dropBands(const QSize &size)
{
    return QSize(qBound(24, size.width()  / 4, 120),
                 qBound(24, size.height() / 4, 120));
}

ChannelPane::DropZone ChannelPane::zoneFor(const QSize &size, const QPoint &pos)
{
    const QSize band = dropBands(size);
    // Left/right win the corners: the horizontal split is the common one.
    if (pos.x() < band.width())                    return DropZone::Left;
    if (pos.x() > size.width()  - band.width())    return DropZone::Right;
    if (pos.y() < band.height())                   return DropZone::Top;
    if (pos.y() > size.height() - band.height())   return DropZone::Bottom;
    return DropZone::Center;
}

QRect ChannelPane::zoneRect(const QSize &size, DropZone zone)
{
    const QSize band = dropBands(size);
    const QRect full(QPoint(0, 0), size);
    switch (zone) {
    case DropZone::Left:   return QRect(0, 0, band.width(), size.height());
    case DropZone::Right:  return QRect(size.width() - band.width(), 0,
                                        band.width(), size.height());
    case DropZone::Top:    return QRect(0, 0, size.width(), band.height());
    case DropZone::Bottom: return QRect(0, size.height() - band.height(),
                                        size.width(), band.height());
    case DropZone::Center: break;
    }
    return full;
}

void ChannelPane::setDragHighlight(DropZone zone)
{
    if (!m_dropFrame) m_dropFrame = new DropFrame(this);
    m_dropFrame->activate(zoneRect(size(), zone));
}

void ChannelPane::clearDragHighlight()
{
    if (m_dropFrame) m_dropFrame->hide();
}

QString ChannelPane::mimeType()
{
    return QStringLiteral("application/x-uplink-panekey");
}

// True when a drop here would actually rearrange: this pane is docked in
// the main window and isn't the pane being dragged. A popped-out pane
// isn't part of the docked layout, so it can't be a target.
bool ChannelPane::isPaneDropTarget(const QMimeData *mime) const
{
    return qobject_cast<QMainWindow *>(window())
        && QString::fromUtf8(mime->data(mimeType().toUtf8())) != key();
}

void ChannelPane::dragEnterEvent(QDragEnterEvent *event)
{
    if (!event->mimeData()->hasFormat(mimeType().toUtf8())) {
        event->ignore();
        return;
    }
    // Accept the pane mime even when this pane can't take the drop (itself,
    // or popped out). On Wayland the compositor picks the drag cursor from
    // the accept state — any rejecting surface flips it to the forbidden
    // shape, and the grab hand should hold from pickup to drop. Only real
    // targets get the drop frame, and only they act on the drop.
    event->acceptProposedAction();
    updateDropHighlight(event->mimeData(), event->position().toPoint());
}

void ChannelPane::dragMoveEvent(QDragMoveEvent *event)
{
    if (!event->mimeData()->hasFormat(mimeType().toUtf8())) {
        event->ignore();
        return;
    }
    event->acceptProposedAction();
    // Track the cursor: the highlight has to say which side is about to be
    // taken, not just that this pane is the target.
    updateDropHighlight(event->mimeData(), event->position().toPoint());
}

void ChannelPane::dragLeaveEvent(QDragLeaveEvent *)
{
    clearDragHighlight();
}

void ChannelPane::dropEvent(QDropEvent *event)
{
    clearDragHighlight();
    event->acceptProposedAction();
    const QByteArray fmt = mimeType().toUtf8();
    if (!isPaneDropTarget(event->mimeData())) return;
    const QString sourceKey = QString::fromUtf8(event->mimeData()->data(fmt));
    const DropZone zone = zoneFor(size(), event->position().toPoint());
    // Nothing was highlighted, so nothing was promised.
    if (m_zoneFilter && !m_zoneFilter(sourceKey, zone)) return;
    emit dropReceived(sourceKey, zone);
}

// Light up the zone under the cursor, but only when releasing there would
// rearrange something — see the filter MainWindow installs.
void ChannelPane::updateDropHighlight(const QMimeData *mime, const QPoint &pos)
{
    if (!isPaneDropTarget(mime)) return;
    const QString sourceKey = QString::fromUtf8(mime->data(mimeType().toUtf8()));
    const DropZone zone = zoneFor(size(), pos);
    if (m_zoneFilter && !m_zoneFilter(sourceKey, zone)) {
        clearDragHighlight();
        return;
    }
    setDragHighlight(zone);
}

bool ChannelPane::eventFilter(QObject *obj, QEvent *event)
{
    // Fractional-scale seam guard — paint the input background in code every
    // frame; see the matching block in MainWindow::eventFilter for why.
    if (m_input && m_inputBase.isValid() && event->type() == QEvent::Paint) {
        if (obj == m_input->viewport()) {
            QPainter p(m_input->viewport());
            p.fillRect(m_input->viewport()->rect(), m_inputBase);
        } else if (obj == m_input) {
            QPainter p(m_input);
            p.setRenderHint(QPainter::Antialiasing);
            p.setPen(Qt::NoPen);
            p.setBrush(m_inputBase);
            p.drawRoundedRect(m_input->rect(), 8, 8);
        }
        return false;
    }

    // A repolish reset a guarded widget's font — put ours back. Compare the
    // attributes we care about (not QFont equality, whose resolve-mask
    // comparison never matches a resolved widget font) and latch against the
    // FontChange our own setFont re-triggers.
    if (event->type() == QEvent::FontChange && !m_fontGuardBusy) {
        auto *w = qobject_cast<QWidget*>(obj);
        const auto it = w ? m_fontGuards.constFind(w) : m_fontGuards.constEnd();
        if (w && it != m_fontGuards.constEnd()) {
            const QFont cur = w->font();
            // cppcheck-suppress-begin derefInvalidIterator ; guarded by the constEnd() check above
            if (cur.families() != it->families()
                || !qFuzzyCompare(cur.pointSizeF(), it->pointSizeF())) {
            // cppcheck-suppress-end derefInvalidIterator
                m_fontGuardBusy = true;
                if (w == m_chatView) m_chatView->setChatFont(it.value());
                else                 w->setFont(it.value());
                m_fontGuardBusy = false;
                if (w == m_input) updateInputHeight();
            }
        }
        return false;
    }

    // Escape cancels whatever the pane has pending (a reply, today) no matter
    // which of its widgets holds focus — the input and the chat view trade it
    // back and forth as menus open and close. Not consumed: the search bar
    // owns Escape while it's open.
    if (event->type() == QEvent::KeyPress
        && static_cast<QKeyEvent*>(event)->key() == Qt::Key_Escape
        && !(m_searchBar && m_searchBar->isVisible())) {
        emit escapePressed();
    }

    if (obj == m_input && event->type() == QEvent::KeyPress) {
        auto *ke = static_cast<QKeyEvent*>(event);
        if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
            if (ke->modifiers() & Qt::ShiftModifier)
                return false;
            const QString raw = m_input->toPlainText();
            if (!raw.trimmed().isEmpty()) {
                // Emit before clearing: the receiver reads the IRC codes off
                // the document (bold/colour live in the char formats, not the
                // plain text) and an already-cleared input has none left.
                emit inputSubmitted(raw);
                m_input->clear();
            }
            return true;
        }
    }

    // While a drag is pending/active for this pane, track it from ANY
    // mouse-move or release in the application, not just events landing on
    // m_header's own children. Under this environment's Wayland/KDE
    // integration there's no reliable implicit mouse capture on press, so
    // once the cursor drifts off the (thin) header strip onto a sibling
    // widget — e.g. the topic bar — further move/release events are
    // delivered there instead, starving a header-scoped check and leaving
    // the gesture stuck. Watching qApp catches them regardless of target
    // widget; this is pure Qt-internal event dispatch, unaffected by the
    // platform grab limitation. Installed/removed just-in-time (below) so
    // the app-wide filter only exists while actually needed.
    if (m_dragPending || m_dragging) {
        if (event->type() == QEvent::MouseButtonRelease) {
            if (m_dragPending) {
                m_dragPending = false;
                qApp->removeEventFilter(this);
            }
            return false;
        }
        if (event->type() == QEvent::MouseMove) {
            auto *me = static_cast<QMouseEvent *>(event);
            const QPoint gp = me->globalPosition().toPoint();

            if (m_dragPending) {
                if (!(me->buttons() & Qt::LeftButton)) {
                    m_dragPending = false;
                    qApp->removeEventFilter(this);
                    return false;
                }
                if ((gp - m_dragStartPos).manhattanLength() < QApplication::startDragDistance())
                    return false;
                m_dragPending = false;
                m_dragging    = true;

                execPaneDrag(this, key(), m_dragStartPos); // blocks until drop or cancel

                m_dragging = false;
                qApp->removeEventFilter(this);
            }
            return false; // never swallow moves/releases meant for other widgets
        }
    }

    bool isHeaderArea = (obj == m_header);
    if (!isHeaderArea)
        for (auto *w : m_header->findChildren<QWidget*>(Qt::FindDirectChildrenOnly))
            if (obj == w) { isHeaderArea = true; break; }
    if (!isHeaderArea) return false;

    if (event->type() == QEvent::MouseButtonPress) {
        auto *me = static_cast<QMouseEvent *>(event);
        // Guards against installing the filter twice: a single click can
        // legitimately deliver MouseButtonPress here more than once (e.g.
        // once for a header child label that ignores it, then again for
        // m_header itself, via Qt's ignore-then-propagate-to-parent
        // behavior) — Qt calls an event filter once per installation, so
        // installing twice for one gesture would run this filter twice per
        // event for the rest of the drag.
        // Popped-out panes live in their own window — there's nothing a
        // header drag from there can rearrange, so don't start one.
        if (me->button() == Qt::LeftButton && !m_dragPending && !m_dragging
            && qobject_cast<QMainWindow *>(window())) {
            m_dragPending  = true;
            m_dragStartPos = me->globalPosition().toPoint();
            qApp->installEventFilter(this);
            // Consume presses on the passive header surfaces (background and
            // labels). Left to propagate, they bubble up to the QMainWindow,
            // where KDE's Breeze style arms its "drag windows from empty
            // areas" whole-window move — a top-level QMainWindow is always
            // eligible — and that compositor grab races and usually steals
            // the pane drag. Buttons accept their own presses and stop the
            // propagation naturally, so they keep native behavior.
            if (!qobject_cast<QAbstractButton *>(obj))
                return true;
        }
    }
    return false;
}

// While a pane drag is in flight, accept-and-swallow the dnd events aimed
// at text-entry widgets (inputs, filter and search fields). They accept
// drops natively for text, so they — not their pane — become the dnd target,
// then reject the pane mime, and every rejection flips the drag cursor to
// the forbidden shape. Dropping on them is a deliberate no-op.
class PaneDragEditGuard : public QObject {
protected:
    bool eventFilter(QObject *obj, QEvent *event) override
    {
        const QEvent::Type t = event->type();
        if (t != QEvent::DragEnter && t != QEvent::DragMove && t != QEvent::Drop)
            return false;
        // A QPlainTextEdit receives dnd on its VIEWPORT (a plain QWidget
        // child), so match the parent too — matching only the edit itself
        // leaves the guard dead for the message inputs.
        if (!qobject_cast<QLineEdit *>(obj) && !qobject_cast<QPlainTextEdit *>(obj)
            && !qobject_cast<QPlainTextEdit *>(obj->parent()))
            return false;
        auto *de = static_cast<QDropEvent *>(event);
        if (!de->mimeData()->hasFormat(ChannelPane::mimeType().toUtf8()))
            return false;
        de->acceptProposedAction();
        return true;
    }
};

// Runs the shared pane-drag gesture: snapshots the pane before the
// placeholder covers it, lifts the scaled ghost under the cursor, and marks
// the vacated slot with a DragPlaceholder until the drop lands or the drag
// is cancelled. Blocks in QDrag::exec. Used by docked ChannelPanes and by
// the main window for the primary panel.
void ChannelPane::execPaneDrag(QWidget *pane, const QString &key, const QPoint &grabbedAtGlobal)
{
    const QPixmap snap = pane->grab();
    const qreal   dpr  = snap.devicePixelRatio();
    QPixmap ghost = snap.scaled(snap.size() * kDragGhostScale,
                                Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ghost.setDevicePixelRatio(dpr);

    auto *placeholder = new DragPlaceholder(pane);
    placeholder->activate();

    auto *mime = new QMimeData;
    mime->setData(mimeType(), key.toUtf8());
    auto *drag = new QDrag(pane);
    drag->setMimeData(mime);
    drag->setPixmap(ghost);
    drag->setHotSpot(pane->mapFromGlobal(grabbedAtGlobal) * kDragGhostScale);
    // Grab hand for platforms where the client draws the drag cursor (X11,
    // macOS) — there it holds even outside our windows. On Wayland the
    // compositor derives the cursor from the accept state instead; that
    // path is covered by every widget accepting the pane mime (see
    // dragEnterEvent and PaneDragEditGuard).
    const QPixmap hand = MenuIcons::grabCursor();
    drag->setDragCursor(hand, Qt::MoveAction);
    drag->setDragCursor(hand, Qt::IgnoreAction);

    PaneDragEditGuard editGuard;
    qApp->installEventFilter(&editGuard);
    drag->exec(Qt::MoveAction); // blocks until drop or cancel
    qApp->removeEventFilter(&editGuard);

    placeholder->deleteLater();
}
