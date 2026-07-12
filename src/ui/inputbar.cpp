// Input bar: setup, submit, tab completion, input history, emoji
// autocomplete, send button, and format indicator. Split out of
// mainwindow.cpp — all methods remain MainWindow members.

#include "mainwindow.h"
#include "ui/chatview.h"
#include "ui/emojipicker.h"
#include "ui/emojidata.h"
#include "ui/menuicons.h"
#include "ui/chatrenderer.h"
#include "ui/searchbar.h"
#include "model/sessionmodel.h"
#include "config/config.h"

#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMenu>
#include <QMessageBox>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextFrame>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

static constexpr int kInputHistoryCap = 100;

void MainWindow::ensureEmojiPicker()
{
    if (m_emojiPicker) return;
    m_emojiPicker = new EmojiPicker(this);
    connect(m_emojiPicker, &EmojiPicker::emojiSelected, this, [this](const QString &emoji){
        if (!m_pendingReactMsgid.isEmpty()) {
            m_model->sendReact(ServerId{m_pendingReactHost}, BufferId{m_pendingReactChannel},
                               m_pendingReactMsgid, emoji);
            m_pendingReactMsgid.clear();
            m_pendingReactHost.clear();
            m_pendingReactChannel.clear();
            return;
        }
        QTextCursor tc = m_input->textCursor();
        tc.insertText(emoji);
        m_input->setTextCursor(tc);
        m_input->setFocus();
    });
}

void MainWindow::setupInputBar()
{
    auto *bar  = new QWidget;
    bar->setObjectName("inputBar");
    auto *hbox = new QHBoxLayout(bar);
    hbox->setContentsMargins(4, 3, 4, 8);
    hbox->setSpacing(4);

    m_nickPrefix = new QLabel;
    m_nickPrefix->setStyleSheet("font-weight: bold; padding-right: 4px;");
    m_nickPrefix->setVisible(m_showNickPrefix);

    m_input = new QPlainTextEdit;
    m_input->setPlaceholderText("Type a message...");
    m_input->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    m_input->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_input->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_input->document()->setDocumentMargin(2);
    m_input->setFixedHeight(m_input->fontMetrics().lineSpacing() + 10);
    m_input->installEventFilter(this);

    // Right-click: standard edit menu plus a Color submenu.
    m_input->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_input, &QPlainTextEdit::customContextMenuRequested,
            this, [this](const QPoint &pos) {
        QMenu *menu = m_input->createStandardContextMenu();
        menu->addSeparator();
        menu->addMenu(makeColorMenu(menu));
        menu->exec(m_input->mapToGlobal(pos));
        delete menu;
    });

    m_emojiBtn = new QPushButton("😊");
    m_emojiBtn->setFixedSize(30, 30);
    m_emojiBtn->setStyleSheet("QPushButton { padding: 0; font-size: 16px; background: transparent; border: none; }");
    m_emojiBtn->setVisible(m_showEmojiBtn);
    m_emojiBtn->setToolTip("Emoji picker");

    // Send button floats inside the right edge of the input widget.
    m_sendBtn = new QToolButton(m_input);
    m_sendBtn->setFixedSize(28, 28);
    m_sendBtn->setAutoRaise(true);
    m_sendBtn->setToolTip("Send");
    m_sendBtn->setIcon(MenuIcons::send({}, 26));
    m_sendBtn->setIconSize(QSize(26, 26));
    m_sendBtn->setEnabled(false);
    m_sendBtn->setVisible(m_config.ui.showSendButton);
    connect(m_sendBtn, &QToolButton::clicked, this, &MainWindow::onInputSubmit);

    // Push text content left so it doesn't flow under the floating button
    {
        QTextFrameFormat fmt = m_input->document()->rootFrame()->frameFormat();
        fmt.setRightMargin(36);
        m_input->document()->rootFrame()->setFrameFormat(fmt);
    }

    // Format indicator: floats at bottom-left of input, shows active IRC format modes.
    m_formatIndicator = new QLabel(m_input);
    m_formatIndicator->setObjectName("formatIndicator");
    m_formatIndicator->setStyleSheet(
        "color: rgba(200,200,200,1.0); font-size: 13px; padding: 1px 5px;"
        "background: rgba(120,120,120,0.25); border-radius: 4px;");
    m_formatIndicator->hide();

    hbox->addWidget(m_nickPrefix);
    hbox->addWidget(m_input, 1);
    hbox->addWidget(m_emojiBtn);

    m_inputBar = bar;

    m_typingLabel = new QLabel;
    m_typingLabel->setObjectName("typingLabel");
    m_typingLabel->setContentsMargins(8, 2, 8, 2);
    m_typingLabel->setVisible(m_config.ui.typingIndicator);

    // Search bar (Ctrl+F)
    m_searchBar = new SearchBar(m_chatView);
    connect(m_searchBar, &SearchBar::dismissed, this, [this]{ m_input->setFocus(); });

    // Reply indicator bar
    m_replyBar = new QWidget;
    m_replyBar->setObjectName("replyBar");
    {
        auto *rhbox = new QHBoxLayout(m_replyBar);
        rhbox->setContentsMargins(8, 2, 4, 2);
        rhbox->setSpacing(4);
        m_replyLabel = new QLabel;
        m_replyLabel->setObjectName("replyLabel");
        auto *closeBtn = new QToolButton;
        closeBtn->setText("✕");
        closeBtn->setFixedSize(18, 18);
        closeBtn->setAutoRaise(true);
        connect(closeBtn, &QToolButton::clicked, this, &MainWindow::clearReplyBar);
        rhbox->addWidget(m_replyLabel, 1);
        rhbox->addWidget(closeBtn);
    }
    m_replyBar->hide();

    // Into the chat column left of the user list — the list runs full height
    // and the compose strip ends at its edge.
    auto *layout = m_chatLeftVbox;
    layout->addWidget(m_searchBar);
    layout->addWidget(m_replyBar);
    layout->addWidget(m_typingLabel);
    layout->addWidget(bar);

    connect(m_emojiBtn, &QPushButton::clicked, this, [this]{
        const QPoint anchor = m_emojiBtn->mapToGlobal(
            QPoint(m_emojiBtn->width(), m_emojiBtn->height()));
        ensureEmojiPicker();
        m_emojiPicker->showAt(anchor);
    });

    // Emoji inline autocomplete list (child widget, no focus steal)
    m_emojiCompleter = new QListWidget(this);
    m_emojiCompleter->setObjectName("emojiCompleter");
    m_emojiCompleter->setFocusPolicy(Qt::NoFocus);
    m_emojiCompleter->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_emojiCompleter->setFrameShape(QFrame::StyledPanel);
    m_emojiCompleter->hide();
    connect(m_emojiCompleter, &QListWidget::itemClicked, this, [this](QListWidgetItem *item){
        commitEmojiAutocomplete(m_emojiCompleter->row(item));
    });

    // Inactivity timer: sends typing=paused after 5s with no keypresses
    m_typingOutTimer = new QTimer(this);
    m_typingOutTimer->setSingleShot(true);
    m_typingOutTimer->setInterval(5000);
    connect(m_typingOutTimer, &QTimer::timeout, this, [this]{
        if (!m_config.ui.typingIndicator) return;
        const ServerId host = m_model->activeHost();
        const BufferId ch   = m_model->activeChannel();
        if (ch.isEmpty() || ch.str() == "(server)") return;
        m_typingActive = false;
        m_model->sendTyping(host, ch, "paused");
    });

    connect(m_input, &QPlainTextEdit::cursorPositionChanged,
            this, &MainWindow::updateFormatIndicator);

    connect(m_input, &QPlainTextEdit::textChanged, this, [this]{
        const QString text = m_input->toPlainText();
        m_sendBtn->setEnabled(!text.trimmed().isEmpty());
        checkEmojiAutocomplete(text);
        // Auto-resize: 1 to 4 lines
        const int lineH = m_input->fontMetrics().lineSpacing();
        const int margins = m_input->contentsMargins().top() + m_input->contentsMargins().bottom() + 8;
        const int lines = qMin(4, static_cast<int>(text.count('\n')) + 1);
        m_input->setFixedHeight(lines * lineH + margins);
        if (!m_config.ui.typingIndicator) return;
        const ServerId host = m_model->activeHost();
        const BufferId ch   = m_model->activeChannel();
        if (ch.isEmpty() || ch.str() == "(server)") return;
        if (!text.isEmpty()) {
            if (!m_typingActive) {
                m_typingActive = true;
                m_model->sendTyping(host, ch, "active");
            }
            m_typingOutTimer->start();
        } else {
            m_typingOutTimer->stop();
            if (m_typingActive) {
                m_typingActive = false;
                m_model->sendTyping(host, ch, "done");
            }
        }
    });

    QTimer::singleShot(0, this, [this]{ repositionSendBtn(); });
}

void MainWindow::repositionSendBtn()
{
    if (!m_sendBtn || !m_input) return;
    const int pad = 3;
    const int x = m_input->width()  - m_sendBtn->width()  - pad;
    const int y = (m_input->height() - m_sendBtn->height()) / 2;
    m_sendBtn->move(x, y);
    m_sendBtn->raise();
    if (m_formatIndicator && m_formatIndicator->isVisible()) {
        const int fy = m_input->height() - m_formatIndicator->height() - 3;
        m_formatIndicator->move(4, fy);
    }
}

void MainWindow::updateFormatIndicator()
{
    if (!m_formatIndicator || !m_input) return;
    const QTextCharFormat cf = m_input->currentCharFormat();
    const bool bold   = cf.fontWeight() >= QFont::Bold;
    const bool italic = cf.fontItalic();
    const bool under  = cf.fontUnderline();
    const bool strike = cf.fontStrikeOut();
    const bool hasFg  = cf.hasProperty(QTextFormat::ForegroundBrush)
        && ChatRenderer::mircColorIndex(cf.foreground().color()) >= 0;
    const bool hasBg  = cf.hasProperty(QTextFormat::BackgroundBrush)
        && ChatRenderer::mircColorIndex(cf.background().color()) >= 0;
    if (!bold && !italic && !under && !strike && !hasFg && !hasBg) {
        m_formatIndicator->hide();
        return;
    }
    QString text;
    if (bold)   text += "<b>B</b>";
    if (italic) { if (!text.isEmpty()) text += " "; text += "<i>I</i>"; }
    if (under)  { if (!text.isEmpty()) text += " "; text += "<u>U</u>"; }
    if (strike) { if (!text.isEmpty()) text += " "; text += "<s>S</s>"; }
    if (hasFg || hasBg) {
        if (!text.isEmpty()) text += " ";
        const QString fg = hasFg ? cf.foreground().color().name() : QStringLiteral("inherit");
        const QString bg = hasBg ? cf.background().color().name() : QStringLiteral("transparent");
        text += QString("<span style='color:%1;background-color:%2'>A</span>").arg(fg, bg);
    }
    m_formatIndicator->setText(text);
    m_formatIndicator->adjustSize();
    const int fy = m_input->height() - m_formatIndicator->height() - 3;
    m_formatIndicator->move(4, fy);
    m_formatIndicator->raise();
    m_formatIndicator->show();
}

// Build the mIRC color menu: "Text color" + "Background" swatch submenus and a
// reset. Reused by the Ctrl+Shift+K popup and the input's right-click menu.
// Colors are stored as QTextCharFormat brushes (like bold/italic) on the input
// and encoded at send time.
QMenu *MainWindow::makeColorMenu(QWidget *parent)
{
    static const char *const kNames[16] = {
        "White", "Black", "Blue", "Green", "Red", "Brown", "Purple", "Orange",
        "Yellow", "Light Green", "Cyan", "Light Cyan", "Light Blue", "Pink",
        "Grey", "Light Grey"
    };
    auto swatch = [](int i) {
        QPixmap pm(16, 16);
        pm.fill(ChatRenderer::mircColor(i));
        return QIcon(pm);
    };

    QMenu *menu = new QMenu(tr("Color"), parent);
    QMenu *fgMenu = menu->addMenu(tr("Text color"));
    QMenu *bgMenu = menu->addMenu(tr("Background"));
    for (int i = 0; i < 16; ++i) {
        const QString label = QString("%1  %2").arg(i, 2).arg(kNames[i]);
        connect(fgMenu->addAction(swatch(i), label), &QAction::triggered,
                this, [this, i] { applyInputColor(i, -2); });
        connect(bgMenu->addAction(swatch(i), label), &QAction::triggered,
                this, [this, i] { applyInputColor(-2, i); });
    }
    bgMenu->addSeparator();
    connect(bgMenu->addAction(tr("None")), &QAction::triggered,
            this, [this] { applyInputColor(-2, -1); });
    menu->addSeparator();
    connect(menu->addAction(tr("Reset color")), &QAction::triggered,
            this, [this] { applyInputColor(-1, -1); });
    return menu;
}

// Ctrl+Shift+K: pop the color menu open at the input's caret.
void MainWindow::showColorPicker()
{
    QMenu *menu = makeColorMenu(this);
    const QRect cr = m_input->cursorRect();
    menu->exec(m_input->viewport()->mapToGlobal(cr.bottomLeft()));
    delete menu;
    m_input->setFocus();
}

// fg/bg: 0-15 sets that mIRC color, -1 clears it, -2 leaves it unchanged.
void MainWindow::applyInputColor(int fg, int bg)
{
    QTextCharFormat cf = m_input->currentCharFormat();
    if (fg == -1)      cf.clearProperty(QTextFormat::ForegroundBrush);
    else if (fg >= 0)  cf.setForeground(ChatRenderer::mircColor(fg));
    if (bg == -1)      cf.clearProperty(QTextFormat::BackgroundBrush);
    else if (bg >= 0)  cf.setBackground(ChatRenderer::mircColor(bg));
    m_input->textCursor().setCharFormat(cf);
    m_input->setCurrentCharFormat(cf);
    updateFormatIndicator();
}

void MainWindow::handleTabComplete(QPlainTextEdit *input, ServerId host, BufferId channel)
{
    const QTextCursor tc = input->textCursor();
    const QString text = tc.block().text();
    const int pos = tc.positionInBlock();

    if (!m_tabActive) {
        // Start a new cycle: derive prefix from text before cursor.
        // pos == 0 must not reach lastIndexOf: a from-index of -1 means
        // "search from the end" in Qt and would grab the last word.
        if (pos == 0) return;
        const qsizetype wordStart = text.lastIndexOf(' ', pos - 1) + 1;
        const QString prefix = text.mid(wordStart, pos - wordStart);
        if (prefix.isEmpty()) return;

        m_tabPrefix    = prefix;
        m_tabWordStart = static_cast<int>(wordStart);
        m_tabCandidates.clear();
        m_tabCandidateIndex = 0;
        m_tabActive = true;

        if (prefix.startsWith('/')) {
            static const QStringList commands = {
                "/away", "/back", "/ban", "/caps", "/clear", "/connect", "/ctcp",
                "/deop", "/devoice", "/disconnect", "/invite", "/j", "/join",
                "/close", "/kick", "/leave", "/me", "/mode", "/motd", "/msg",
                "/nick", "/notice", "/op", "/part", "/ping", "/time",
                "/quit", "/quote", "/raw", "/server", "/sysinfo", "/topic",
                "/unban", "/version", "/voice", "/whois",
            };
            for (const QString &cmd : commands)
                if (cmd.startsWith(prefix, Qt::CaseInsensitive))
                    m_tabCandidates << cmd;
        } else {
            auto *ch = m_model->channel(host, channel);
            if (ch) {
                for (const auto &e : std::as_const(ch->nicks))
                    if (e.nick.startsWith(prefix, Qt::CaseInsensitive))
                        m_tabCandidates << e.nick;
                m_tabCandidates.sort(Qt::CaseInsensitive);
            }
        }
    }
    // else: continuing a cycle — use stored m_tabWordStart and m_tabPrefix as-is

    if (m_tabCandidates.isEmpty()) return;

    const QString completed = m_tabCandidates[m_tabCandidateIndex];
    m_tabCandidateIndex = static_cast<int>((m_tabCandidateIndex + 1) % m_tabCandidates.size());

    // Nicks at line start get ": ", everything else at end-of-line gets " "
    QString suffix;
    if (pos == static_cast<int>(text.length()))
        suffix = (m_tabWordStart == 0 && !completed.startsWith('/'))
            ? QStringLiteral(": ") : QStringLiteral(" ");

    const int blockStart = tc.block().position();
    QTextCursor editCursor = input->textCursor();
    editCursor.setPosition(blockStart + m_tabWordStart);
    editCursor.setPosition(blockStart + pos, QTextCursor::KeepAnchor);
    editCursor.insertText(completed + suffix);
    input->setTextCursor(editCursor);
}

void MainWindow::handleHistoryUp()
{
    if (m_inputHistory.isEmpty()) return;
    if (m_historyIndex == -1)
        m_historyDraft = m_input->toPlainText();
    m_historyIndex = qMin(m_historyIndex + 1, static_cast<int>(m_inputHistory.size()) - 1);
    m_input->setPlainText(m_inputHistory[m_historyIndex]);
    m_input->moveCursor(QTextCursor::End);
}

void MainWindow::handleHistoryDown()
{
    if (m_historyIndex == -1) return;
    m_historyIndex--;
    if (m_historyIndex < 0) {
        m_historyIndex = -1;
        m_input->setPlainText(m_historyDraft);
    } else {
        m_input->setPlainText(m_inputHistory[m_historyIndex]);
    }
    m_input->moveCursor(QTextCursor::End);
}

// ---------------------------------------------------------------------------
// Emoji inline autocomplete
// ---------------------------------------------------------------------------

void MainWindow::checkEmojiAutocomplete(const QString &text)
{
    const int cursorPos = m_input->textCursor().position();
    const QString before = text.left(cursorPos);

    // Auto-substitute a completed :shortcode: when the closing colon is just typed
    if (cursorPos > 0 && text[cursorPos - 1] == ':') {
        const QString beforeColon = before.chopped(1);  // drop the closing ':'
        const qsizetype openColon = beforeColon.lastIndexOf(':');
        if (openColon >= 0) {
            const QString code = beforeColon.mid(openColon + 1);
            static const QRegularExpression wordOnly(R"(^\w+$)");
            if (wordOnly.match(code).hasMatch()) {
                const QString emoji = emojiForCode(code);
                if (!emoji.isEmpty()) {
                    QTextCursor tc = m_input->textCursor();
                    tc.setPosition(static_cast<int>(openColon));
                    tc.setPosition(cursorPos, QTextCursor::KeepAnchor);
                    tc.insertText(emoji);
                    m_input->setTextCursor(tc);
                    hideEmojiAutocomplete();
                    return;
                }
            }
        }
    }

    // Find a bare :word pattern ending at cursor — minimum 1 char after colon
    const qsizetype colon = before.lastIndexOf(':');
    if (colon < 0) { hideEmojiAutocomplete(); return; }

    const QString word = before.mid(colon + 1);
    // Only trigger if word is all word-chars and at least 1 char
    static const QRegularExpression wordRe(R"(^\w+$)");
    if (word.isEmpty() || !wordRe.match(word).hasMatch()) {
        hideEmojiAutocomplete();
        return;
    }

    const auto matches = emojiMatching(word);
    if (matches.isEmpty()) { hideEmojiAutocomplete(); return; }

    m_emojiTriggerPos = static_cast<int>(colon);

    m_emojiCompleter->clear();
    const int shown = static_cast<int>(qMin(matches.size(), qsizetype(8)));
    for (int i = 0; i < shown; ++i) {
        const auto &e = matches[i];
        auto *item = new QListWidgetItem(e.ch.toString() + "  " + e.shortcode.toString());
        item->setData(Qt::UserRole, e.ch.toString());
        m_emojiCompleter->addItem(item);
    }
    m_emojiCompleter->setCurrentRow(0);

    // Size to fit content
    const int itemH  = m_emojiCompleter->sizeHintForRow(0) + 2;
    const int popupH = itemH * shown + 4;
    const int popupW = 220;

    // Position above the input bar in main-window coords
    const QPoint inputTL = m_input->mapTo(this, QPoint(0, 0));

    // Align left edge with colon position using actual text advance
    const QString textUpToColon = before.left(colon);
    const int colonX = m_input->contentsMargins().left()
                       + m_input->fontMetrics().horizontalAdvance(textUpToColon);
    const QPoint colonLocal = m_input->mapTo(this, QPoint(colonX, 0));

    int px = qMax(inputTL.x(), colonLocal.x());
    int py = inputTL.y() - popupH - 2;
    if (py < 0) py = inputTL.y() + m_input->height() + 2;

    // Clamp horizontally
    if (px + popupW > width()) px = width() - popupW;

    m_emojiCompleter->setGeometry(px, py, popupW, popupH);
    m_emojiCompleter->show();
    m_emojiCompleter->raise();
}

void MainWindow::commitEmojiAutocomplete(int row)
{
    if (row < 0 || row >= m_emojiCompleter->count()) return;

    const QString emoji = m_emojiCompleter->item(row)->data(Qt::UserRole).toString();
    hideEmojiAutocomplete();

    // Replace :word (from trigger pos to cursor) with the emoji
    const int end = m_input->textCursor().position();
    QTextCursor tc = m_input->textCursor();
    tc.setPosition(m_emojiTriggerPos);
    tc.setPosition(end, QTextCursor::KeepAnchor);
    tc.insertText(emoji);
    m_input->setTextCursor(tc);
    m_input->setFocus();
}

void MainWindow::hideEmojiAutocomplete()
{
    m_emojiCompleter->hide();
    m_emojiCompleter->clear();
    m_emojiTriggerPos = -1;
}

// Convert the input widget's rich-formatted document to an IRC-encoded string.
// IRC control chars (bold \x02, italic \x1D, underline \x1F, strikethrough \x1E,
// color \x03fg[,bg]) are emitted at format-change boundaries; the visible text
// has no control glyphs.
static QString inputToIrcText(QPlainTextEdit *edit)
{
    const QTextDocument *doc = edit->document();
    QString result;
    bool curBold = false, curItalic = false, curUnder = false, curStrike = false;
    int  curFg = -1, curBg = -1;   // active mIRC color indices, -1 = default
    bool firstBlock = true;
    for (QTextBlock block = doc->begin(); block != doc->end(); block = block.next()) {
        if (!firstBlock) result += '\n';
        firstBlock = false;
        for (auto it = block.begin(); !it.atEnd(); ++it) {
            const QTextFragment frag = it.fragment();
            if (!frag.isValid()) continue;
            const QTextCharFormat fmt = frag.charFormat();
            const int wFg = fmt.hasProperty(QTextFormat::ForegroundBrush)
                ? ChatRenderer::mircColorIndex(fmt.foreground().color()) : -1;
            const int wBg = fmt.hasProperty(QTextFormat::BackgroundBrush)
                ? ChatRenderer::mircColorIndex(fmt.background().color()) : -1;

            // Color transition. Clearing color needs \x0F, which also drops the
            // other attributes, so reset their state and let the toggles below
            // re-emit whatever is still active.
            if (wFg != curFg || wBg != curBg) {
                if (wFg < 0 && wBg < 0) {
                    result += QChar(0x0F);
                    curBold = curItalic = curUnder = curStrike = false;
                } else {
                    result += QChar(0x03);
                    result += QString("%1").arg(wFg < 0 ? 99 : wFg, 2, 10, QChar('0'));
                    if (wBg >= 0)
                        result += ',' + QString("%1").arg(wBg, 2, 10, QChar('0'));
                }
                curFg = wFg;
                curBg = wBg;
            }

            const bool wB = (fmt.fontWeight() == QFont::Bold);
            const bool wI = fmt.fontItalic();
            const bool wU = fmt.fontUnderline();
            const bool wS = fmt.fontStrikeOut();
            if (wB != curBold)   { result += QChar(0x02); curBold  = wB; }
            if (wI != curItalic) { result += QChar(0x1D); curItalic = wI; }
            if (wU != curUnder)  { result += QChar(0x1F); curUnder = wU; }
            if (wS != curStrike) { result += QChar(0x1E); curStrike = wS; }
            result += frag.text();
        }
    }
    if (curBold || curItalic || curUnder || curStrike || curFg >= 0 || curBg >= 0)
        result += QChar(0x0F);
    return result;
}

void MainWindow::onInputSubmit()
{
    const QString raw  = inputToIrcText(m_input);
    const QString text = raw.trimmed();
    if (text.isEmpty()) return;

    // Push to history (newest first, skip consecutive duplicates)
    if (m_inputHistory.isEmpty() || m_inputHistory.first() != text) {
        m_inputHistory.prepend(text);
        if (m_inputHistory.size() > kInputHistoryCap)
            m_inputHistory.removeLast();
    }
    m_historyIndex = -1;
    m_tabActive = false;
    m_tabCandidates.clear();
    hideEmojiAutocomplete();

    m_input->clear();
    if (m_formatIndicator) m_formatIndicator->hide();

    const ServerId host    = m_model->activeHost();
    const BufferId channel = m_model->activeChannel();
    if (host.isEmpty() || channel.isEmpty()) return;

    // Stop typing notification on send
    m_typingOutTimer->stop();
    if (m_typingActive && m_config.ui.typingIndicator && channel.str() != "(server)") {
        m_typingActive = false;
        m_model->sendTyping(host, channel, "done");
    }

    const QStringList lines = raw.split('\n', Qt::SkipEmptyParts);
    if (lines.size() > 1) {
        if (lines.size() >= 4) {
            const auto ans = QMessageBox::question(this, "Send multiple lines?",
                QString("Send %1 lines to %2?").arg(lines.size()).arg(channel.str()),
                QMessageBox::Yes | QMessageBox::Cancel);
            if (ans != QMessageBox::Yes) return;
        }
        for (const QString &line : lines)
            dispatchInput(line, host, channel);
        return;
    }

    dispatchInput(raw, host, channel);
}
