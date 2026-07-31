#include "preferencesdialog.h"
#include "ui/themeloader.h"
#include "ui/menuicons.h"
#include "ui/pillbutton.h"
#include "ui/solidcombobox.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QStackedWidget>
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
    { "circle-bubble-black",   "Bubble Black"  },
    { "circle-bubble-blue",    "Bubble Blue"   },
    { "circle-bubble-cyan",    "Bubble Cyan"   },
    { "circle-bubble-green",   "Bubble Green"  },
    { "circle-bubble-magenta", "Bubble Magenta"},
    { "circle-bubble-purple",  "Bubble Purple" },
    { "circle-bubble-red",     "Bubble Red"    },
};

const QList<QPair<QString,QString>> PreferencesDialog::s_bracketChoices = {
    { "<>",   "<nick>  (angle)"   },
    { "[]",   "[nick]  (square)"  },
    { "::::", "::nick::  (colon)" },
    { "",     "nick  (none)"      },
};

const QList<QPair<QString,QString>> PreferencesDialog::s_menuStyleChoices = {
    { "menubar", "Menu bar"                  },
    { "hidden",  "Hidden (shortcuts only)"   },
};

const QList<QPair<QString,QString>> PreferencesDialog::s_persistenceChoices = {
    { "default", "Server default" },
    { "on",      "Enabled"        },
    { "off",     "Disabled"       },
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
    addNavItem("File Transfers",MenuIcons::openConfig());
    addNavItem("Profile",       MenuIcons::gear());
    addNavItem("Scripts",       MenuIcons::scripts());

    m_pages = new QStackedWidget;
    m_pages->addWidget(createAppearancePage(cfg, accent));
    m_pages->addWidget(createChatWindowPage(cfg));
    m_pages->addWidget(createInterfacePage(cfg));
    m_pages->addWidget(createNotificationsPage(cfg));
    m_pages->addWidget(createLoggingPage(cfg));
    m_pages->addWidget(createTransfersPage(cfg));
    m_pages->addWidget(createProfilePage(cfg, accent));
    m_pages->addWidget(createScriptsPage(cfg, accent));

    connect(m_navList, &QListWidget::currentRowChanged,
            m_pages, &QStackedWidget::setCurrentIndex);
    m_navList->setCurrentRow(0);

    auto *sep = new QFrame;
    sep->setFrameShape(QFrame::VLine);
    sep->setFrameShadow(QFrame::Sunken);

    auto *body = new QHBoxLayout;
    body->setContentsMargins(0, 0, 0, 0);
    body->setSpacing(8);
    body->addWidget(m_navList);
    body->addWidget(sep);
    body->addWidget(m_pages, 1);

    // Settings apply live, so a single Close button is all that's needed.
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    buttons->button(QDialogButtonBox::Close)->setIcon(MenuIcons::close());
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::accept);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);
    mainLayout->addLayout(body, 1);
    mainLayout->addWidget(buttons);
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
    m_themeBtn = themeBtn;
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

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    // Auto mode: follow the OS light/dark scheme with a day/night theme pair.
    vbox->addSpacing(6);
    m_themeAutoCheck = new QCheckBox("Follow System Light/Dark (Auto)");
    m_themeAutoCheck->setToolTip("Switch between the two themes below whenever the\n"
                                 "desktop flips between light and dark mode.\n"
                                 "Picking a theme from the list above turns this off.");
    m_themeAutoCheck->setChecked(cfg.ui.themeAuto);
    connect(m_themeAutoCheck, &QCheckBox::toggled, this, [this](bool on){ emit themeAutoToggled(on); });
    vbox->addWidget(m_themeAutoCheck);

    auto makePairRow = [&](const QString &label, const QString &current) {
        auto *row = new QHBoxLayout;
        auto *lbl = new QLabel(label);
        lbl->setMinimumWidth(90);
        auto *combo = new SolidComboBox;
        combo->addItems(ThemeLoader::availableThemes());
        combo->setCurrentText(current);
        combo->setEnabled(m_themeAutoCheck->isChecked());
        connect(m_themeAutoCheck, &QCheckBox::toggled, combo, &QWidget::setEnabled);
        row->addWidget(lbl);
        row->addWidget(combo, 1);
        vbox->addLayout(row);
        return combo;
    };
    m_themeLightCombo = makePairRow("Light theme", cfg.ui.themeLight);
    m_themeDarkCombo  = makePairRow("Dark theme",  cfg.ui.themeDark);
    connect(m_themeLightCombo, &QComboBox::textActivated,
            this, [this](const QString &name){ emit themeLightChanged(name); });
    connect(m_themeDarkCombo, &QComboBox::textActivated,
            this, [this](const QString &name){ emit themeDarkChanged(name); });
#endif

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
        grid->setSpacing(4);
        auto *iconGroup = new QButtonGroup(this);
        iconGroup->setExclusive(true);
        const int cols = 5;
        for (int i = 0; i < s_iconChoices.size(); ++i) {
            const QString &key  = s_iconChoices[i].first;
            const QString &name = s_iconChoices[i].second;
            auto *btn = new QToolButton;
            btn->setIcon(QIcon(QStringLiteral(":/icons/%1.png").arg(key)));
            btn->setIconSize(QSize(40, 40));
            btn->setFixedSize(46, 46);
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
        auto *gridWrap = new QHBoxLayout;
        gridWrap->addLayout(grid);
        gridWrap->addStretch();   // keep tiles left-aligned, not stretched
        vbox->addLayout(gridWrap);
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

    m_avatarsCheck = new QCheckBox("Show Avatars");
    m_avatarsCheck->setToolTip("Avatar images are fetched from whatever URL the user or\n"
                               "channel op set, so the host they point at sees your IP.\n"
                               "Uncheck to skip the fetch — names and status text still work.");
    m_avatarsCheck->setChecked(cfg.ui.showAvatars);
    connect(m_avatarsCheck, &QCheckBox::toggled, this, [this](bool on){ emit avatarsToggled(on); });
    vbox->addWidget(m_avatarsCheck);

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

    m_paneSplitAutoCheck = new QCheckBox("Split Panes Automatically");
    m_paneSplitAutoCheck->setToolTip(
        "Panes split along whichever axis fits: columns on a wide window,\n"
        "rows on a tall one. Dropping a pane on another pane's edge picks\n"
        "a side by hand and turns this off; unchecking it here keeps\n"
        "whichever way the panes are split right now.");
    m_paneSplitAutoCheck->setChecked(cfg.ui.paneSplitAxis == "auto");
    connect(m_paneSplitAutoCheck, &QCheckBox::toggled, this,
            [this](bool on){ emit paneSplitAutoToggled(on); });
    vbox->addWidget(m_paneSplitAutoCheck);

    m_panelCardsCheck = new QCheckBox("Panel Cards");
    m_panelCardsCheck->setToolTip("Side panels use their own theme colors with rounded tops.\n"
                                  "Uncheck for the classic flat look (everything on the chat color).");
    m_panelCardsCheck->setChecked(cfg.ui.panelCards);
    connect(m_panelCardsCheck, &QCheckBox::toggled, this, [this](bool on){ emit panelCardsToggled(on); });
    vbox->addWidget(m_panelCardsCheck);

    vbox->addSpacing(6);
    vbox->addWidget(sectionLabel("Menu Style"));
    {
        m_menuStyleGroup = new QButtonGroup(this);
        m_menuStyleGroup->setExclusive(true);
        for (int i = 0; i < s_menuStyleChoices.size(); ++i) {
            auto *rb = new QRadioButton(s_menuStyleChoices[i].second);
            rb->setChecked(s_menuStyleChoices[i].first == cfg.ui.menuStyle);
            m_menuStyleGroup->addButton(rb, i);
            vbox->addWidget(rb);
        }
        connect(m_menuStyleGroup, &QButtonGroup::idClicked, this, [this](int idx){
            if (idx >= 0 && idx < s_menuStyleChoices.size())
                emit menuStyleChanged(s_menuStyleChoices[idx].first);
        });
    }

    vbox->addSpacing(6);
    vbox->addWidget(sectionLabel("Stay Online (keeps your nick online like an IRC bouncer)"));
    {
        m_persistenceGroup = new QButtonGroup(this);
        m_persistenceGroup->setExclusive(true);
        const QString tip =
            "Keep your nick on the server while Uplink is closed, like a\n"
            "bouncer: no quit/join churn, and missed messages are waiting\n"
            "when you reconnect. Needs a server that supports\n"
            "draft/persistence (e.g. Ergo).";
        for (int i = 0; i < s_persistenceChoices.size(); ++i) {
            auto *rb = new QRadioButton(s_persistenceChoices[i].second);
            rb->setToolTip(tip);
            rb->setChecked(s_persistenceChoices[i].first == cfg.ui.persistence);
            m_persistenceGroup->addButton(rb, i);
            vbox->addWidget(rb);
        }
        connect(m_persistenceGroup, &QButtonGroup::idClicked, this, [this](int idx){
            if (idx >= 0 && idx < s_persistenceChoices.size())
                emit persistenceChanged(s_persistenceChoices[idx].first);
        });
    }

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

// ── File Transfers ───────────────────────────────────────────────────────────

QWidget *PreferencesDialog::createTransfersPage(const Config &cfg)
{
    auto *page = new QWidget;
    auto *vbox = new QVBoxLayout(page);
    vbox->setContentsMargins(12, 8, 12, 8);
    vbox->setSpacing(6);

    vbox->addWidget(pageTitle("File Transfers"));

    {
        auto *note = new QLabel(
            "DCC transfers connect directly between you and the other user, "
            "outside the IRC server. These settings match the <b>[dcc]</b> "
            "block in config.toml.");
        note->setWordWrap(true);
        note->setStyleSheet("font-size: 9pt;");
        vbox->addWidget(note);
    }

    vbox->addSpacing(6);
    m_dccAllowLanCheck = new QCheckBox("Allow Transfers with LAN Peers");
    m_dccAllowLanCheck->setChecked(cfg.dcc.allowLan);
    m_dccAllowLanCheck->setToolTip(
        "Permit DCC with private addresses (e.g. 192.168.x.x), such as "
        "your own machines at home. Off, such peers are refused.");
    connect(m_dccAllowLanCheck, &QCheckBox::toggled, this, [this](bool on){
        emit dccAllowLanToggled(on);
    });
    vbox->addWidget(m_dccAllowLanCheck);

    vbox->addWidget(sectionLabel("Behind NAT"));
    {
        auto *note = new QLabel(
            "When your router hides your public address, set it here and "
            "forward a port range to this machine. Leave the address empty "
            "to use what the IRC network reveals about your host.");
        note->setWordWrap(true);
        note->setStyleSheet("font-size: 9pt;");
        vbox->addWidget(note);
    }
    {
        auto *row = new QHBoxLayout;
        row->addWidget(new QLabel("External IP:"));
        m_dccExternalIpEdit = new QLineEdit;
        m_dccExternalIpEdit->setPlaceholderText("e.g. 203.0.113.7  (empty = auto-discover)");
        m_dccExternalIpEdit->setText(cfg.dcc.externalIp);
        connect(m_dccExternalIpEdit, &QLineEdit::editingFinished, this, [this]{
            emit dccExternalIpChanged(m_dccExternalIpEdit->text().trimmed());
        });
        row->addWidget(m_dccExternalIpEdit, 1);
        vbox->addLayout(row);
    }
    {
        auto *row = new QHBoxLayout;
        row->addWidget(new QLabel("Listen Ports:"));
        m_dccPortMinSpin = new QSpinBox;
        m_dccPortMinSpin->setRange(0, 65535);
        m_dccPortMinSpin->setSpecialValueText("auto");
        m_dccPortMinSpin->setValue(cfg.dcc.portMin);
        m_dccPortMaxSpin = new QSpinBox;
        m_dccPortMaxSpin->setRange(0, 65535);
        m_dccPortMaxSpin->setSpecialValueText("auto");
        m_dccPortMaxSpin->setValue(cfg.dcc.portMax);
        auto emitRange = [this]{
            emit dccPortRangeChanged(quint16(m_dccPortMinSpin->value()),
                                     quint16(m_dccPortMaxSpin->value()));
        };
        connect(m_dccPortMinSpin, &QSpinBox::editingFinished, this, emitRange);
        connect(m_dccPortMaxSpin, &QSpinBox::editingFinished, this, emitRange);
        row->addWidget(m_dccPortMinSpin);
        row->addWidget(new QLabel("to"));
        row->addWidget(m_dccPortMaxSpin);
        row->addStretch();
        vbox->addLayout(row);
    }

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
            "Requires <b>draft/metadata</b> support (Ergo and others).");
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
        auto *row = new QHBoxLayout;
        row->addWidget(new QLabel("Status:"));
        m_statusEdit = new QLineEdit;
        m_statusEdit->setPlaceholderText("e.g. afk until monday  (leave blank to clear)");
        m_statusEdit->setText(cfg.profileStatusText);
        row->addWidget(m_statusEdit, 1);
        vbox->addLayout(row);
    }
    {
        auto *applyBtn = new PillButton("Apply to connected servers");
        applyBtn->setAccentColor(accent);
        applyBtn->setAutoDefault(false);
        connect(applyBtn, &QPushButton::clicked, this, [this]{
            emit profileSetRequested(m_displayNameEdit->text().trimmed(),
                                     m_avatarUrlEdit->text().trimmed(),
                                     m_statusEdit->text().trimmed());
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

    auto *scriptsList = new QVBoxLayout;
    scriptsList->setSpacing(8);
    auto *scriptsContainer = new QWidget;
    scriptsContainer->setLayout(scriptsList);

    auto emitScripts = [this, scriptsContainer] {
        QList<ScriptBinding> scripts;
        const auto rows = scriptsContainer->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
        for (auto *row : rows) {
            auto *cmdEdit  = row->findChild<QLineEdit*>("cmdEdit");
            auto *pathEdit = row->findChild<QLineEdit*>("pathEdit");
            auto *chk      = row->findChild<QCheckBox*>("enabledChk");
            if (!cmdEdit || !pathEdit || !chk) continue;
            ScriptBinding sb;
            sb.command = cmdEdit->text().trimmed().toLower().remove(QChar('/'));
            sb.path    = pathEdit->text().trimmed();
            sb.enabled = chk->isChecked();
            if (!sb.command.isEmpty() && !sb.path.isEmpty())
                scripts.append(sb);
        }
        emit scriptsChanged(scripts);
    };

    auto addRow = [this, scriptsList, emitScripts](const ScriptBinding &sb) {
        auto *row = new QWidget;
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 4, 0, 4);
        rowLayout->setSpacing(6);

        auto *chk = new QCheckBox;
        chk->setObjectName("enabledChk");
        chk->setChecked(sb.enabled);
        chk->setToolTip("Enabled");
        rowLayout->addWidget(chk);

        auto *slash = new QLabel("/");
        slash->setFixedWidth(8);
        rowLayout->addWidget(slash);

        auto *cmdEdit = new QLineEdit(sb.command);
        cmdEdit->setObjectName("cmdEdit");
        cmdEdit->setPlaceholderText("command");
        cmdEdit->setFixedWidth(100);
        rowLayout->addWidget(cmdEdit);

        auto *pathEdit = new QLineEdit(sb.path);
        pathEdit->setObjectName("pathEdit");
        pathEdit->setPlaceholderText("Path to script...");
        rowLayout->addWidget(pathEdit, 1);

        auto *browseBtn = new QPushButton("Browse...");
        browseBtn->setAutoDefault(false);
        connect(browseBtn, &QPushButton::clicked, this, [this, pathEdit] {
            const QString path = QFileDialog::getOpenFileName(
                this, "Select Script", QString(), "All Files (*)");
            if (!path.isEmpty())
                pathEdit->setText(path);
        });
        rowLayout->addWidget(browseBtn);

        auto *removeBtn = new QPushButton;
        removeBtn->setIcon(MenuIcons::deleteIcon());
        removeBtn->setIconSize(QSize(18, 18));
        removeBtn->setFixedSize(28, 28);
        removeBtn->setFlat(true);
        removeBtn->setAutoDefault(false);
        removeBtn->setToolTip("Remove");
        connect(removeBtn, &QPushButton::clicked, this, [row, emitScripts] {
            row->deleteLater();
            emitScripts();
        });
        rowLayout->addWidget(removeBtn);

        scriptsList->addWidget(row);

        connect(cmdEdit,  &QLineEdit::editingFinished, this, emitScripts);
        connect(pathEdit, &QLineEdit::editingFinished, this, emitScripts);
        connect(chk,      &QCheckBox::toggled,         this, emitScripts);
    };

    for (const auto &sb : cfg.scripts)
        addRow(sb);

    vbox->addWidget(scriptsContainer);

    {
        auto *btnRow = new QHBoxLayout;

        auto *addBtn = new PillButton("Add Script");
        addBtn->setAccentColor(accent);
        addBtn->setAutoDefault(false);
        connect(addBtn, &QPushButton::clicked, this, [addRow] {
            addRow(ScriptBinding{});
        });
        btnRow->addWidget(addBtn);

        auto *restoreBtn = new PillButton("Restore Defaults");
        restoreBtn->setAccentColor(accent);
        restoreBtn->setAutoDefault(false);
        connect(restoreBtn, &QPushButton::clicked, this, [addRow, emitScripts, scriptsContainer] {
            QList<ScriptBinding> current;
            const auto rows = scriptsContainer->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
            for (auto *row : rows) {
                auto *cmdEdit = row->findChild<QLineEdit*>("cmdEdit");
                if (cmdEdit)
                    current.append({cmdEdit->text().trimmed().toLower().remove(QChar('/')), {}, true});
            }
            Config::installDefaultScripts(current);
            // Add rows for any newly installed scripts
            for (const auto &sb : std::as_const(current)) {
                bool exists = false;
                for (auto *row : rows) {
                    auto *cmdEdit = row->findChild<QLineEdit*>("cmdEdit");
                    if (cmdEdit && cmdEdit->text().trimmed().toLower().remove(QChar('/')) == sb.command) {
                        exists = true;
                        break;
                    }
                }
                if (!exists)
                    addRow(sb);
            }
            emitScripts();
        });
        btnRow->addWidget(restoreBtn);

        btnRow->addStretch();
        vbox->addLayout(btnRow);
    }

    vbox->addStretch();
    return page;
}

// Re-sync toggle controls after a setting is changed from outside the dialog
// (e.g. the View menu). Blocked signals — no re-emit loops.
void PreferencesDialog::syncFromConfig(const Config &cfg)
{
    auto setCheck = [](QCheckBox *cb, bool on){
        if (!cb) return;
        QSignalBlocker block(cb);
        cb->setChecked(on);
    };
    setCheck(m_topicCheck,         cfg.ui.showTopic);
    setCheck(m_timestampsCheck,    cfg.ui.showTimestamps);
    setCheck(m_unreadCountsCheck,  cfg.ui.showUnreadCounts);
    setCheck(m_avatarsCheck,       cfg.ui.showAvatars);
    setCheck(m_panelCardsCheck,    cfg.ui.panelCards);
    // An edge-drop writes columns/rows, so this can change without the dialog
    setCheck(m_paneSplitAutoCheck, cfg.ui.paneSplitAxis == "auto");
    setCheck(m_themeAutoCheck,     cfg.ui.themeAuto);
    setCheck(m_dccAllowLanCheck,   cfg.dcc.allowLan);
    if (m_dccExternalIpEdit) {
        QSignalBlocker block(m_dccExternalIpEdit);
        m_dccExternalIpEdit->setText(cfg.dcc.externalIp);
    }
    if (m_dccPortMinSpin && m_dccPortMaxSpin) {
        QSignalBlocker b1(m_dccPortMinSpin), b2(m_dccPortMaxSpin);
        m_dccPortMinSpin->setValue(cfg.dcc.portMin);
        m_dccPortMaxSpin->setValue(cfg.dcc.portMax);
    }
    auto setCombo = [&cfg](SolidComboBox *combo, const QString &text){
        if (!combo) return;
        QSignalBlocker block(combo);
        combo->setCurrentText(text);
        combo->setEnabled(cfg.ui.themeAuto);
    };
    setCombo(m_themeLightCombo, cfg.ui.themeLight);
    setCombo(m_themeDarkCombo,  cfg.ui.themeDark);
}
