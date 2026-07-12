#include "channelpane.h"
#include "ui/chatview.h"
#include "ui/fadescrollbar.h"
#include "ui/uistyle.h"
#include "ui/searchbar.h"
#include "ui/nickfilteredit.h"
#include "ui/chromepanel.h"
#include "ui/dropframe.h"

#include <QListWidget>
#include <QScroller>
#include <QPlainTextEdit>
#include <QKeyEvent>
#include <QLabel>
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

ChannelPane::ChannelPane(ServerId host, BufferId channel, QWidget *parent)
    : QWidget(parent), m_host(std::move(host)), m_channel(std::move(channel))
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

    auto *nameLabel = new QLabel(m_channel.str());
    nameLabel->setObjectName("paneChannelLabel");
    QFont f = nameLabel->font();
    f.setBold(true);
    nameLabel->setFont(f);

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

    m_closeBtn = new QToolButton;
    m_closeBtn->setText(QStringLiteral("✕"));
    m_closeBtn->setFixedSize(16, 16);
    m_closeBtn->setStyleSheet(
        "QToolButton { background: transparent; border: none; padding: 0px; }"
        "QToolButton:hover { color: palette(highlight); }"
    );
    connect(m_closeBtn, &QToolButton::clicked, this, &ChannelPane::closeRequested);

    hbox->addWidget(m_topicToggle);
    hbox->addWidget(nameLabel, 1);
    hbox->addWidget(m_popOutBtn);
    hbox->addWidget(m_searchBtn);
    hbox->addWidget(m_closeBtn);
    vbox->addWidget(m_header);

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
    tbhbox->addWidget(m_topicText);
    m_topicBar->hide(); // added to the chat column below, next to the body splitter

    connect(m_topicToggle, &QToolButton::toggled, this, [this](bool on){
        m_topicBar->setVisible(on);
        if (m_nickRevealBtn && m_nickRevealBtn->isVisible())
            positionNickRevealBtn();
    });

    // Chat view
    m_chatView = new ChatView;

    m_nickList = new QListWidget;
    m_nickList->setVerticalScrollBar(new FadeScrollBar(Qt::Vertical, m_nickList));
    m_nickList->setSpacing(0);
    m_nickList->setUniformItemSizes(true);
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

    m_nickFilter = new NickFilterEdit(m_nickList);

    m_nickWrapper = new ChromePanel;
    m_nickWrapper->setObjectName("nickPanel");
    m_nickWrapper->setMinimumWidth(24);
    auto *nwvbox = new QVBoxLayout(m_nickWrapper);
    nwvbox->setContentsMargins(0, 0, 0, 0);
    nwvbox->setSpacing(0);
    nwvbox->addWidget(nickHeader);
    nwvbox->addWidget(m_nickFilter);
    // No trailing stretch — see the main window's nick panel for why.
    nwvbox->addWidget(m_nickList, 1);

    // Floating reveal button — shown over the chat when the list is hidden
    m_nickRevealBtn = new QToolButton(this);
    m_nickRevealBtn->setFixedSize(28, 28);
    m_nickRevealBtn->setIconSize(QSize(20, 20));
    m_nickRevealBtn->setAutoRaise(true);
    m_nickRevealBtn->setStyleSheet(UiStyle::headerButtonStyle());
    m_nickRevealBtn->setToolTip(QStringLiteral("Show user list"));
    m_nickRevealBtn->hide();
    connect(m_nickToggleBtn, &QToolButton::clicked, this, [this]{
        m_nickWrapper->hide();
        positionNickRevealBtn();
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
    ccVbox->addWidget(m_topicBar);
    ccVbox->addWidget(m_chatView, 1);

    auto *bodySplitter = new QSplitter(Qt::Horizontal);
    // Backdrop behind the nick panel's rounded top corners.
    bodySplitter->setAttribute(Qt::WA_StyledBackground, true);
    bodySplitter->setHandleWidth(0);
    bodySplitter->addWidget(chatCol);
    bodySplitter->addWidget(m_nickWrapper);
    bodySplitter->setStretchFactor(0, 1);
    bodySplitter->setStretchFactor(1, 0);
    bodySplitter->setSizes({999, 120});
    vbox->addWidget(bodySplitter, 1);

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
    ibox->addWidget(m_nickPrefix);
    ibox->addWidget(m_input, 1);
    ccVbox->addWidget(inputBar);
    updateInputHeight();

    connect(m_input, &QPlainTextEdit::textChanged, this, [this]{
        updateInputHeight();
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
void ChannelPane::setNickChrome(const QString &bg)
{
    const QColor c(bg);
    if (m_nickWrapper) static_cast<ChromePanel *>(m_nickWrapper)->setFill(c);
    if (m_nickHeader)  static_cast<ChromePanel *>(m_nickHeader)->setFill(c);
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

void ChannelPane::clearNickFilter()
{
    if (m_nickFilter) m_nickFilter->clear();
}

void ChannelPane::positionNickRevealBtn()
{
    if (!m_nickRevealBtn) return;
    const int topY = m_header->height()
                   + (m_topicBar && m_topicBar->isVisible() ? m_topicBar->height() : 0)
                   + 4;
    m_nickRevealBtn->move(width() - m_nickRevealBtn->width() - 4, topY);
    m_nickRevealBtn->raise();
}

void ChannelPane::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_nickRevealBtn && m_nickRevealBtn->isVisible())
        positionNickRevealBtn();
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
    m_chatView->setFont(f); // ChatView's own setFont (also fonts the viewport)
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

void ChannelPane::setDragHighlight(bool on)
{
    if (on) {
        if (!m_dropFrame) m_dropFrame = new DropFrame(this);
        m_dropFrame->activate();
    } else if (m_dropFrame) {
        m_dropFrame->hide();
    }
}

QString ChannelPane::mimeType()
{
    return QStringLiteral("application/x-uplink-panekey");
}

void ChannelPane::dragEnterEvent(QDragEnterEvent *event)
{
    const QByteArray fmt = mimeType().toUtf8();
    // A popped-out pane isn't part of the docked layout — dropping on it
    // can't rearrange anything, so don't advertise it as a target.
    if (qobject_cast<QMainWindow *>(window())
        && event->mimeData()->hasFormat(fmt)
        && QString::fromUtf8(event->mimeData()->data(fmt)) != key()) {
        event->acceptProposedAction();
        setDragHighlight(true);
    } else {
        event->ignore();
    }
}

void ChannelPane::dragMoveEvent(QDragMoveEvent *event)
{
    const QByteArray fmt = mimeType().toUtf8();
    if (event->mimeData()->hasFormat(fmt)
        && QString::fromUtf8(event->mimeData()->data(fmt)) != key())
        event->acceptProposedAction();
    else
        event->ignore();
}

void ChannelPane::dragLeaveEvent(QDragLeaveEvent *)
{
    setDragHighlight(false);
}

void ChannelPane::dropEvent(QDropEvent *event)
{
    setDragHighlight(false);
    const QByteArray fmt = mimeType().toUtf8();
    const QString sourceKey = QString::fromUtf8(event->mimeData()->data(fmt));
    event->acceptProposedAction();
    emit dropReceived(sourceKey);
}

bool ChannelPane::eventFilter(QObject *obj, QEvent *event)
{
    // A repolish reset a guarded widget's font — put ours back. Compare the
    // attributes we care about (not QFont equality, whose resolve-mask
    // comparison never matches a resolved widget font) and latch against the
    // FontChange our own setFont re-triggers.
    if (event->type() == QEvent::FontChange && !m_fontGuardBusy) {
        auto *w = qobject_cast<QWidget*>(obj);
        const auto it = w ? m_fontGuards.constFind(w) : m_fontGuards.constEnd();
        if (it != m_fontGuards.constEnd()) {
            const QFont cur = w->font();
            if (cur.families() != it->families()
                || !qFuzzyCompare(cur.pointSizeF(), it->pointSizeF())) {
                m_fontGuardBusy = true;
                if (w == m_chatView) m_chatView->setFont(it.value());
                else                 w->setFont(it.value());
                m_fontGuardBusy = false;
                if (w == m_input) updateInputHeight();
            }
        }
        return false;
    }

    if (obj == m_input && event->type() == QEvent::KeyPress) {
        auto *ke = static_cast<QKeyEvent*>(event);
        if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
            if (ke->modifiers() & Qt::ShiftModifier)
                return false;
            const QString raw = m_input->toPlainText();
            if (!raw.trimmed().isEmpty()) {
                m_input->clear();
                emit inputSubmitted(raw);
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

                auto *mime = new QMimeData;
                mime->setData(mimeType(), key().toUtf8());
                auto *drag = new QDrag(this);
                drag->setMimeData(mime);
                drag->exec(Qt::MoveAction); // blocks until drop or cancel

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
        }
    }
    return false;
}
