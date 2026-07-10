#include "channelpane.h"
#include "ui/chatview.h"
#include "ui/fadescrollbar.h"

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

ChannelPane::ChannelPane(ServerId host, BufferId channel, QWidget *parent)
    : QWidget(parent), m_host(std::move(host)), m_channel(std::move(channel))
{
    // Paint a themed background (bufferBg) so the compose strip — typing
    // indicator + input bar — sits on the chat colour like the main window.
    setObjectName("channelPane");
    setAttribute(Qt::WA_StyledBackground, true);

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

    const QString headerBtnStyle =
        "QToolButton { background: transparent; border: none; }"
        "QToolButton:hover { background: rgba(255,255,255,0.08); border-radius: 4px; }";

    m_popOutBtn = new QToolButton;
    m_popOutBtn->setFixedSize(20, 20);
    m_popOutBtn->setIconSize(QSize(16, 16));
    m_popOutBtn->setAutoRaise(true);
    m_popOutBtn->setStyleSheet(headerBtnStyle);
    m_popOutBtn->setToolTip(QStringLiteral("Open in a window"));
    connect(m_popOutBtn, &QToolButton::clicked, this, &ChannelPane::popOutRequested);

    m_searchBtn = new QToolButton;
    m_searchBtn->setFixedSize(20, 20);
    m_searchBtn->setIconSize(QSize(16, 16));
    m_searchBtn->setAutoRaise(true);
    m_searchBtn->setStyleSheet(headerBtnStyle);
    m_searchBtn->setToolTip(QStringLiteral("Search (Ctrl+F)"));
    connect(m_searchBtn, &QToolButton::clicked, this, [this]{
        if (m_searchBar->isVisible()) hideSearch();
        else { m_searchBar->show(); m_searchInput->setFocus(); m_searchInput->selectAll(); }
    });

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
    m_topicBar->hide();
    vbox->addWidget(m_topicBar);

    connect(m_topicToggle, &QToolButton::toggled, this, [this](bool on){
        m_topicBar->setVisible(on);
    });

    // Chat view
    m_chatView = new ChatView;

    m_nickList = new QListWidget;
    m_nickList->setVerticalScrollBar(new FadeScrollBar(Qt::Vertical, m_nickList));
    m_nickList->setSpacing(0);
    m_nickList->setUniformItemSizes(true);
    QScroller::grabGesture(m_nickList->viewport(), QScroller::LeftMouseButtonGesture);

    auto *nickWrapper = new QWidget;
    nickWrapper->setObjectName("nickPanel");
    auto *nwvbox = new QVBoxLayout(nickWrapper);
    nwvbox->setContentsMargins(0, 0, 0, 0);
    nwvbox->setSpacing(0);
    nwvbox->addWidget(m_nickList, 100);
    nwvbox->addStretch(1);

    auto *bodySplitter = new QSplitter(Qt::Horizontal);
    bodySplitter->setHandleWidth(0);
    bodySplitter->addWidget(m_chatView);
    bodySplitter->addWidget(nickWrapper);
    bodySplitter->setStretchFactor(0, 1);
    bodySplitter->setStretchFactor(1, 0);
    bodySplitter->setSizes({999, 120});
    vbox->addWidget(bodySplitter, 1);

    // Search bar (hidden until the magnifier is clicked)
    m_searchBar = new QWidget;
    m_searchBar->setObjectName("searchBar");
    {
        auto *shbox = new QHBoxLayout(m_searchBar);
        shbox->setContentsMargins(4, 2, 4, 2);
        shbox->setSpacing(4);
        m_searchInput = new QLineEdit;
        m_searchInput->setPlaceholderText(QStringLiteral("Search in buffer…"));
        m_searchInput->installEventFilter(this);
        connect(m_searchInput, &QLineEdit::textChanged, this, [this](const QString &text){
            if (text.isEmpty()) m_chatView->clearFind();
            else m_chatView->findText(text, false);
        });
        auto *sClose = new QToolButton;
        sClose->setText(QStringLiteral("✕"));
        sClose->setFixedSize(22, 22);
        sClose->setAutoRaise(true);
        connect(sClose, &QToolButton::clicked, this, [this]{ hideSearch(); });
        shbox->addWidget(m_searchInput, 1);
        shbox->addWidget(sClose);
    }
    m_searchBar->hide();
    vbox->addWidget(m_searchBar);

    // Typing indicator (hidden until someone is typing)
    m_typingLabel = new QLabel;
    m_typingLabel->setObjectName("typingLabel");
    m_typingLabel->setContentsMargins(8, 2, 8, 2);
    m_typingLabel->hide();
    vbox->addWidget(m_typingLabel);

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
    m_input->setFixedHeight(m_input->fontMetrics().lineSpacing() + 10);
    m_input->installEventFilter(this);
    ibox->addWidget(m_nickPrefix);
    ibox->addWidget(m_input, 1);
    vbox->addWidget(inputBar);

    connect(m_input, &QPlainTextEdit::textChanged, this, [this]{
        const QString text = m_input->toPlainText();
        const int lineH = m_input->fontMetrics().lineSpacing();
        const int margins = m_input->contentsMargins().top() + m_input->contentsMargins().bottom() + 8;
        const int lines = qMin(4, static_cast<int>(text.count('\n')) + 1);
        m_input->setFixedHeight(lines * lineH + margins);
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

void ChannelPane::hideSearch()
{
    if (!m_searchBar) return;
    m_searchBar->hide();
    m_searchInput->clear();
    m_chatView->clearFind();
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
    if (m_typingLabel) m_typingLabel->setFont(f);
}

void ChannelPane::setInputFont(const QFont &nickFont, const QFont &inputFont)
{
    if (m_nickPrefix) m_nickPrefix->setFont(nickFont);
    if (m_input)      m_input->setFont(inputFont);
}

void ChannelPane::setTopicFont(const QFont &f)
{
    m_topicFontPt = f.pointSize();
    if (m_topicText) {
        m_topicText->setFont(f);
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

void ChannelPane::setDragHighlight(bool on)
{
    m_header->setStyleSheet(on ? "background: palette(highlight);" : "");
}

bool ChannelPane::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_searchInput && event->type() == QEvent::KeyPress) {
        auto *ke = static_cast<QKeyEvent*>(event);
        if (ke->key() == Qt::Key_Escape) { hideSearch(); return true; }
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

    bool isHeaderArea = (obj == m_header);
    if (!isHeaderArea)
        for (auto *w : m_header->findChildren<QWidget*>(Qt::FindDirectChildrenOnly))
            if (obj == w) { isHeaderArea = true; break; }
    if (!isHeaderArea) return false;

    if (event->type() == QEvent::MouseButtonPress) {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::LeftButton) {
            m_dragPending  = true;
            m_dragStartPos = me->globalPosition().toPoint();
        }
    } else if (event->type() == QEvent::MouseMove) {
        if (!m_dragPending && !m_dragging) return false;
        auto *me = static_cast<QMouseEvent *>(event);
        const QPoint gp = me->globalPosition().toPoint();

        if (m_dragPending) {
            if (!(me->buttons() & Qt::LeftButton)) { m_dragPending = false; return false; }
            if ((gp - m_dragStartPos).manhattanLength() < QApplication::startDragDistance())
                return false;
            m_dragPending = false;
            m_dragging    = true;
            m_header->grabMouse(Qt::ClosedHandCursor);
        }

        if (m_dragging) {
            emit dragActive(key(), gp);
            return true;
        }
    } else if (event->type() == QEvent::MouseButtonRelease) {
        if (m_dragging) {
            auto *me = static_cast<QMouseEvent *>(event);
            m_header->releaseMouse();
            m_header->unsetCursor();
            m_dragging = false;
            emit dragDropped(key(), me->globalPosition().toPoint());
            return true;
        }
        m_dragPending = false;
    }
    return false;
}
