#include "preferencesdialog.h"
#include "ui/themeloader.h"
#include "ui/menuicons.h"
#include "ui/pillbutton.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QRadioButton>
#include <QStackedWidget>
#include <QTableWidget>
#include <QToolButton>
#include <QVBoxLayout>

const QList<QPair<QString,QString>> PreferencesDialog::s_iconChoices = {
    { "flat-black",          "Flat Black"    },
    { "black-old-orange",    "Old Orange"    },
    { "black-orange",        "Black Orange"  },
    { "original-black",      "Original"      },
    { "original-flat-shine", "Flat Shine"    },
    { "colorful-blueish",    "Blueish"       },
    { "colorful-greenblue",  "Green Blue"    },
    { "colorful-hotbluepink","Hot Pink"      },
    { "colorful-orange",     "Orange"        },
    { "colorful-purple",     "Purple"        },
    { "gruvbox-blue",        "Gruvbox Blue"  },
    { "gruvbox-colorful",    "Gruvbox Color" },
    { "gruvbox-orange",      "Gruvbox Orange"},
    { "gruvbox-purple",      "Gruvbox Purple"},
    { "gruvbox-yellow",      "Gruvbox Yellow"},
};

const QList<QPair<QString,QString>> PreferencesDialog::s_bracketChoices = {
    { "<>",   "<nick>  (angle)"   },
    { "[]",   "[nick]  (square)"  },
    { "::::", "::nick::  (colon)" },
    { "",     "nick  (none)"      },
};

static QLabel *pageTitle(const QString &text)
{
    auto *l = new QLabel(text);
    l->setStyleSheet("font-size: 16pt; font-weight: bold;");
    l->setContentsMargins(0, 0, 0, 8);
    return l;
}

static QLabel *sectionLabel(const QString &text)
{
    auto *l = new QLabel("<b>" + text + "</b>");
    l->setContentsMargins(0, 6, 0, 2);
    return l;
}

PreferencesDialog::PreferencesDialog(const Config &cfg, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Preferences");
    setMinimumSize(680, 520);
    resize(760, 580);

    const Theme t = ThemeLoader::load(cfg.ui.theme);
    const QColor accent(t.accent);

    m_navList = new QListWidget;
    m_navList->setFixedWidth(160);
    m_navList->setIconSize(QSize(20, 20));
    m_navList->setSpacing(2);
    m_navList->setFrameShape(QFrame::NoFrame);

    auto addNavItem = [this](const QString &label, const QIcon &icon) {
        auto *item = new QListWidgetItem(icon, label);
        item->setSizeHint(QSize(0, 36));
        m_navList->addItem(item);
    };
    addNavItem("Appearance",    MenuIcons::theme());
    addNavItem("Chat Window",   MenuIcons::unread());
    addNavItem("Interface",     MenuIcons::preferences());
    addNavItem("Notifications", MenuIcons::mention());
    addNavItem("Logging",       MenuIcons::documentation());
    addNavItem("Profile",       MenuIcons::gear());
    addNavItem("Scripts",       MenuIcons::scripts());

    m_pages = new QStackedWidget;
    m_pages->addWidget(createAppearancePage(cfg, accent));
    m_pages->addWidget(createChatWindowPage(cfg));
    m_pages->addWidget(createInterfacePage(cfg));
    m_pages->addWidget(createNotificationsPage(cfg));
    m_pages->addWidget(createLoggingPage(cfg));
    m_pages->addWidget(createProfilePage(cfg, accent));
    m_pages->addWidget(createScriptsPage(cfg, accent));

    connect(m_navList, &QListWidget::currentRowChanged,
            m_pages, &QStackedWidget::setCurrentIndex);
    m_navList->setCurrentRow(0);

    auto *sep = new QFrame;
    sep->setFrameShape(QFrame::VLine);
    sep->setFrameShadow(QFrame::Sunken);

    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);
    mainLayout->addWidget(m_navList);
    mainLayout->addWidget(sep);
    mainLayout->addWidget(m_pages, 1);
}

// ── Appearance ───────────────────────────────────────────────────────────────

QWidget *PreferencesDialog::createAppearancePage(const Config &cfg, const QColor &accent)
{
    auto *page = new QWidget;
    auto *vbox = new QVBoxLayout(page);
    vbox->setContentsMargins(12, 8, 12, 8);
    vbox->setSpacing(6);

    vbox->addWidget(pageTitle("Appearance"));

    vbox->addWidget(sectionLabel("Theme"));

    auto *themeBtn = new PillButton(cfg.ui.theme);
    themeBtn->setCheckable(true);
    themeBtn->setAutoDefault(false);
    themeBtn->setAccentColor(accent);
    themeBtn->setLeftAlign(true);
    vbox->addWidget(themeBtn);

    auto *themeList = new QListWidget;
    themeList->setFrameShape(QFrame::StyledPanel);
    themeList->setFixedHeight(150);
    themeList->setVisible(false);
    for (const QString &name : ThemeLoader::availableThemes())
        themeList->addItem(name);
    {
        const auto matches = themeList->findItems(cfg.ui.theme, Qt::MatchExactly);
        if (!matches.isEmpty()) {
            themeList->setCurrentItem(matches.first());
            themeList->scrollToItem(matches.first());
        }
    }
    vbox->addWidget(themeList);

    connect(themeBtn, &QPushButton::toggled, themeList, &QWidget::setVisible);

    auto applyTheme = [this, themeBtn](QListWidgetItem *item){
        if (!item) return;
        themeBtn->setText(item->text());
        emit themeChanged(item->text());
    };
    connect(themeList, &QListWidget::itemClicked,        this, applyTheme);
    connect(themeList, &QListWidget::itemActivated,      this, applyTheme);
    connect(themeList, &QListWidget::currentItemChanged,  this, applyTheme);

    vbox->addSpacing(6);
    auto *fontBtn = new PillButton("Font Config...");
    fontBtn->setIcon(MenuIcons::fontConfig());
    fontBtn->setAccentColor(accent);
    fontBtn->setAutoDefault(false);
    connect(fontBtn, &QPushButton::clicked, this, [this]{ emit fontConfigRequested(); });
    vbox->addWidget(fontBtn);

    vbox->addSpacing(6);
    vbox->addWidget(sectionLabel("App Icon"));
    {
        auto *grid = new QGridLayout;
        grid->setSpacing(6);
        auto *iconGroup = new QButtonGroup(this);
        iconGroup->setExclusive(true);
        const int cols = 5;
        for (int i = 0; i < s_iconChoices.size(); ++i) {
            const QString &key  = s_iconChoices[i].first;
            const QString &name = s_iconChoices[i].second;
            auto *btn = new QToolButton;
            btn->setIcon(QIcon(QStringLiteral(":/icons/%1.png").arg(key)));
            btn->setIconSize(QSize(80, 80));
            btn->setFixedSize(88, 88);
            btn->setCheckable(true);
            btn->setAutoRaise(true);
            btn->setToolTip(name);
            btn->setChecked(key == cfg.ui.appIcon);
            btn->setStyleSheet(
                "QToolButton { background: transparent; border: none; }"
                "QToolButton:checked { border: 2px solid palette(highlight); border-radius: 6px; }");
            iconGroup->addButton(btn, i);
            grid->addWidget(btn, i / cols, i % cols);
        }
        connect(iconGroup, &QButtonGroup::idClicked, this, [this](int id){
            if (id >= 0 && id < s_iconChoices.size())
                emit appIconChanged(s_iconChoices[id].first);
        });
        vbox->addLayout(grid);
    }

    vbox->addStretch();
    return page;
}

// ── Chat Window ──────────────────────────────────────────────────────────────

QWidget *PreferencesDialog::createChatWindowPage(const Config &cfg)
{
    auto *page = new QWidget;
    auto *vbox = new QVBoxLayout(page);
    vbox->setContentsMargins(12, 8, 12, 8);
    vbox->setSpacing(6);

    vbox->addWidget(pageTitle("Chat Window"));

    m_topicCheck = new QCheckBox("Show Topic Bar");
    m_topicCheck->setChecked(cfg.ui.showTopic);
    connect(m_topicCheck, &QCheckBox::toggled, this, [this](bool on){ emit topicBarToggled(on); });
    vbox->addWidget(m_topicCheck);

    m_timestampsCheck = new QCheckBox("Show Timestamps");
    m_timestampsCheck->setChecked(cfg.ui.showTimestamps);
    connect(m_timestampsCheck, &QCheckBox::toggled, this, [this](bool on){ emit timestampsToggled(on); });
    vbox->addWidget(m_timestampsCheck);

    m_coloredNicksCheck = new QCheckBox("Colored Nicks");
    m_coloredNicksCheck->setChecked(cfg.ui.coloredNicks);
    connect(m_coloredNicksCheck, &QCheckBox::toggled, this, [this](bool on){ emit coloredNicksToggled(on); });
    vbox->addWidget(m_coloredNicksCheck);

    m_hangingIndentCheck = new QCheckBox("Hanging Indent (wrap under message)");
    m_hangingIndentCheck->setChecked(cfg.ui.hangingIndent);
    connect(m_hangingIndentCheck, &QCheckBox::toggled, this, [this](bool on){ emit hangingIndentToggled(on); });
    vbox->addWidget(m_hangingIndentCheck);

    m_linkPreviewsCheck = new QCheckBox("Link Previews");
    m_linkPreviewsCheck->setChecked(cfg.ui.linkPreviews);
    connect(m_linkPreviewsCheck, &QCheckBox::toggled, this, [this](bool on){ emit linkPreviewsToggled(on); });
    vbox->addWidget(m_linkPreviewsCheck);

    vbox->addSpacing(6);
    vbox->addWidget(sectionLabel("Nick Brackets"));
    {
        m_bracketsGroup = new QButtonGroup(this);
        m_bracketsGroup->setExclusive(true);
        for (int i = 0; i < s_bracketChoices.size(); ++i) {
            auto *rb = new QRadioButton(s_bracketChoices[i].second);
            rb->setChecked(s_bracketChoices[i].first == cfg.ui.nickBrackets);
            m_bracketsGroup->addButton(rb, i);
            vbox->addWidget(rb);
        }
        connect(m_bracketsGroup, &QButtonGroup::idClicked, this, [this](int idx){
            if (idx >= 0 && idx < s_bracketChoices.size())
                emit nickBracketsChanged(s_bracketChoices[idx].first);
        });
    }

    vbox->addStretch();
    return page;
}

// ── Interface ────────────────────────────────────────────────────────────────

QWidget *PreferencesDialog::createInterfacePage(const Config &cfg)
{
    auto *page = new QWidget;
    auto *vbox = new QVBoxLayout(page);
    vbox->setContentsMargins(12, 8, 12, 8);
    vbox->setSpacing(6);

    vbox->addWidget(pageTitle("Interface"));

    m_nickPrefixCheck = new QCheckBox("Show Nick in Input");
    m_nickPrefixCheck->setChecked(cfg.ui.showNickPrefix);
    connect(m_nickPrefixCheck, &QCheckBox::toggled, this, [this](bool on){ emit nickPrefixToggled(on); });
    vbox->addWidget(m_nickPrefixCheck);

    m_emojiCheck = new QCheckBox("Show Emoji Button");
    m_emojiCheck->setChecked(cfg.ui.showEmojiButton);
    connect(m_emojiCheck, &QCheckBox::toggled, this, [this](bool on){ emit emojiBtnToggled(on); });
    vbox->addWidget(m_emojiCheck);

    m_sendBtnCheck = new QCheckBox("Show Send Button");
    m_sendBtnCheck->setChecked(cfg.ui.showSendButton);
    connect(m_sendBtnCheck, &QCheckBox::toggled, this, [this](bool on){ emit sendBtnToggled(on); });
    vbox->addWidget(m_sendBtnCheck);

    m_typingCheck = new QCheckBox("Typing Indicator");
    m_typingCheck->setChecked(cfg.ui.typingIndicator);
    connect(m_typingCheck, &QCheckBox::toggled, this, [this](bool on){ emit typingIndicatorToggled(on); });
    vbox->addWidget(m_typingCheck);

    m_unreadCountsCheck = new QCheckBox("Show Unread Message Counts");
    m_unreadCountsCheck->setChecked(cfg.ui.showUnreadCounts);
    connect(m_unreadCountsCheck, &QCheckBox::toggled, this, [this](bool on){ emit unreadCountsToggled(on); });
    vbox->addWidget(m_unreadCountsCheck);

    vbox->addStretch();
    return page;
}

// ── Notifications ────────────────────────────────────────────────────────────

QWidget *PreferencesDialog::createNotificationsPage(const Config &cfg)
{
    auto *page = new QWidget;
    auto *vbox = new QVBoxLayout(page);
    vbox->setContentsMargins(12, 8, 12, 8);
    vbox->setSpacing(6);

    vbox->addWidget(pageTitle("Notifications"));

    m_notificationsCheck = new QCheckBox("Tray Notifications");
    m_notificationsCheck->setChecked(cfg.ui.notifications);
    connect(m_notificationsCheck, &QCheckBox::toggled, this, [this](bool on){ emit notificationsToggled(on); });
    vbox->addWidget(m_notificationsCheck);

    vbox->addSpacing(6);
    {
        auto *row = new QHBoxLayout;
        row->addWidget(new QLabel("Highlight Words:"));
        m_highlightWordsEdit = new QLineEdit;
        m_highlightWordsEdit->setPlaceholderText("e.g. myproject, alert, todo");
        m_highlightWordsEdit->setText(cfg.ui.highlightWords);
        connect(m_highlightWordsEdit, &QLineEdit::editingFinished, this, [this]{
            emit highlightWordsChanged(m_highlightWordsEdit->text().trimmed());
        });
        row->addWidget(m_highlightWordsEdit, 1);
        vbox->addLayout(row);
    }

    vbox->addStretch();
    return page;
}

// ── Logging ──────────────────────────────────────────────────────────────────

QWidget *PreferencesDialog::createLoggingPage(const Config &cfg)
{
    auto *page = new QWidget;
    auto *vbox = new QVBoxLayout(page);
    vbox->setContentsMargins(12, 8, 12, 8);
    vbox->setSpacing(6);

    vbox->addWidget(pageTitle("Logging"));

    m_loggingCheck = new QCheckBox("Log Messages to Disk");
    m_loggingCheck->setChecked(cfg.ui.logMessages);
    connect(m_loggingCheck, &QCheckBox::toggled, this, [this](bool on){ emit loggingToggled(on); });
    vbox->addWidget(m_loggingCheck);

    vbox->addStretch();
    return page;
}

// ── Profile ──────────────────────────────────────────────────────────────────

QWidget *PreferencesDialog::createProfilePage(const Config &cfg, const QColor &accent)
{
    auto *page = new QWidget;
    auto *vbox = new QVBoxLayout(page);
    vbox->setContentsMargins(12, 8, 12, 8);
    vbox->setSpacing(6);

    vbox->addWidget(pageTitle("Profile"));

    {
        auto *note = new QLabel(
            "Your display name and avatar URL are published to the server when you connect. "
            "Other users can see them in the nick list tooltip. "
            "Requires <b>draft/metadata-2</b> support (Ergo, soju, and others).");
        note->setWordWrap(true);
        note->setStyleSheet("font-size: 9pt;");
        vbox->addWidget(note);
    }

    vbox->addSpacing(6);
    {
        auto *row = new QHBoxLayout;
        row->addWidget(new QLabel("Display Name:"));
        m_displayNameEdit = new QLineEdit;
        m_displayNameEdit->setPlaceholderText("e.g. Alice Smith  (leave blank to clear)");
        m_displayNameEdit->setText(cfg.profileDisplayName);
        row->addWidget(m_displayNameEdit, 1);
        vbox->addLayout(row);
    }
    {
        auto *row = new QHBoxLayout;
        row->addWidget(new QLabel("Avatar URL:"));
        m_avatarUrlEdit = new QLineEdit;
        m_avatarUrlEdit->setPlaceholderText("https://example.com/avatar.png  or  /path/to/local.png");
        m_avatarUrlEdit->setText(cfg.profileAvatarUrl);
        row->addWidget(m_avatarUrlEdit, 1);
        auto *browseBtn = new QPushButton("Browse...");
        browseBtn->setAutoDefault(false);
        connect(browseBtn, &QPushButton::clicked, this, [this] {
            const QString path = QFileDialog::getOpenFileName(
                this, "Select Avatar Image", QString(),
                "Images (*.png *.jpg *.jpeg *.gif *.ico *.webp)");
            if (!path.isEmpty())
                m_avatarUrlEdit->setText(path);
        });
        row->addWidget(browseBtn);
        vbox->addLayout(row);
    }
    {
        auto *applyBtn = new PillButton("Apply to connected servers");
        applyBtn->setAccentColor(accent);
        applyBtn->setAutoDefault(false);
        connect(applyBtn, &QPushButton::clicked, this, [this]{
            emit profileSetRequested(m_displayNameEdit->text().trimmed(),
                                     m_avatarUrlEdit->text().trimmed());
        });
        vbox->addWidget(applyBtn);
    }

    vbox->addStretch();
    return page;
}

// ── Scripts ─────────────────────────────────────────────────────────────────

QWidget *PreferencesDialog::createScriptsPage(const Config &cfg, const QColor &accent)
{
    auto *page = new QWidget;
    auto *vbox = new QVBoxLayout(page);
    vbox->setContentsMargins(12, 8, 12, 8);
    vbox->setSpacing(6);

    vbox->addWidget(pageTitle("Scripts"));

    {
        auto *note = new QLabel(
            "Link external scripts to custom slash commands. "
            "Script stdout is sent as a message to the current channel. "
            "Scripts receive context via environment variables: "
            "<b>UPLINK_NICK</b>, <b>UPLINK_SERVER</b>, <b>UPLINK_CHANNEL</b>, <b>UPLINK_ARGS</b>.");
        note->setWordWrap(true);
        note->setStyleSheet("font-size: 9pt;");
        vbox->addWidget(note);
    }

    vbox->addSpacing(4);

    m_scriptsTable = new QTableWidget(0, 3);
    m_scriptsTable->setHorizontalHeaderLabels({"Command", "Script Path", "Enabled"});
    m_scriptsTable->horizontalHeader()->setStretchLastSection(false);
    m_scriptsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    m_scriptsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_scriptsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_scriptsTable->setColumnWidth(0, 120);
    m_scriptsTable->verticalHeader()->setVisible(false);
    m_scriptsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_scriptsTable->setSelectionMode(QAbstractItemView::SingleSelection);

    auto emitScripts = [this] {
        QList<ScriptBinding> scripts;
        for (int r = 0; r < m_scriptsTable->rowCount(); ++r) {
            ScriptBinding sb;
            if (auto *cmdItem = m_scriptsTable->item(r, 0))
                sb.command = cmdItem->text().trimmed().toLower().remove(QChar('/'));
            auto *pathWidget = m_scriptsTable->cellWidget(r, 1);
            if (auto *edit = pathWidget ? pathWidget->findChild<QLineEdit*>() : nullptr)
                sb.path = edit->text().trimmed();
            auto *chkWidget = m_scriptsTable->cellWidget(r, 2);
            if (auto *chk = chkWidget ? chkWidget->findChild<QCheckBox*>() : nullptr)
                sb.enabled = chk->isChecked();
            if (!sb.command.isEmpty() && !sb.path.isEmpty())
                scripts.append(sb);
        }
        emit scriptsChanged(scripts);
    };

    auto addRow = [this, emitScripts](const ScriptBinding &sb) {
        const int row = m_scriptsTable->rowCount();
        m_scriptsTable->insertRow(row);

        auto *cmdItem = new QTableWidgetItem(sb.command);
        m_scriptsTable->setItem(row, 0, cmdItem);

        auto *pathContainer = new QWidget;
        auto *pathLayout = new QHBoxLayout(pathContainer);
        pathLayout->setContentsMargins(2, 0, 2, 0);
        pathLayout->setSpacing(2);
        auto *pathEdit = new QLineEdit(sb.path);
        pathEdit->setPlaceholderText("/path/to/script");
        pathLayout->addWidget(pathEdit, 1);
        auto *browseBtn = new QPushButton("...");
        browseBtn->setFixedWidth(30);
        browseBtn->setAutoDefault(false);
        connect(browseBtn, &QPushButton::clicked, this, [this, pathEdit] {
            const QString path = QFileDialog::getOpenFileName(
                this, "Select Script", QString(), "All Files (*)");
            if (!path.isEmpty())
                pathEdit->setText(path);
        });
        pathLayout->addWidget(browseBtn);
        m_scriptsTable->setCellWidget(row, 1, pathContainer);

        auto *chkContainer = new QWidget;
        auto *chkLayout = new QHBoxLayout(chkContainer);
        chkLayout->setContentsMargins(0, 0, 0, 0);
        chkLayout->setAlignment(Qt::AlignCenter);
        auto *chk = new QCheckBox;
        chk->setChecked(sb.enabled);
        chkLayout->addWidget(chk);
        m_scriptsTable->setCellWidget(row, 2, chkContainer);

        connect(pathEdit, &QLineEdit::editingFinished, this, emitScripts);
        connect(chk, &QCheckBox::toggled, this, emitScripts);
    };

    for (const auto &sb : cfg.scripts)
        addRow(sb);

    connect(m_scriptsTable, &QTableWidget::cellChanged, this, emitScripts);

    vbox->addWidget(m_scriptsTable, 1);

    {
        auto *btnRow = new QHBoxLayout;
        auto *addBtn = new PillButton("Add");
        addBtn->setAccentColor(accent);
        addBtn->setAutoDefault(false);
        connect(addBtn, &QPushButton::clicked, this, [addRow, emitScripts] {
            addRow(ScriptBinding{});
            emitScripts();
        });
        btnRow->addWidget(addBtn);

        auto *removeBtn = new PillButton("Remove");
        removeBtn->setAccentColor(accent);
        removeBtn->setAutoDefault(false);
        connect(removeBtn, &QPushButton::clicked, this, [this, emitScripts] {
            const int row = m_scriptsTable->currentRow();
            if (row >= 0) {
                m_scriptsTable->removeRow(row);
                emitScripts();
            }
        });
        btnRow->addWidget(removeBtn);
        btnRow->addStretch();
        vbox->addLayout(btnRow);
    }

    vbox->addStretch();
    return page;
}
