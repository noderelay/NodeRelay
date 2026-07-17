#include "manageserversdialog.h"
#include "menuicons.h"
#include "ui/pillbutton.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QVBoxLayout>

static QLabel *sectionHeader(const QString &text)
{
    auto *lbl = new QLabel(text);
    lbl->setStyleSheet("font-weight: bold; margin-top: 6px;");
    return lbl;
}

static void addEyeToggle(QLineEdit *field)
{
    auto *act = field->addAction(MenuIcons::eyeOff(), QLineEdit::TrailingPosition);
    QObject::connect(act, &QAction::triggered, field, [field, act] {
        const bool show = field->echoMode() == QLineEdit::Password;
        field->setEchoMode(show ? QLineEdit::Normal : QLineEdit::Password);
        act->setIcon(show ? MenuIcons::eye() : MenuIcons::eyeOff());
    });
}

static QWidget *makeBrowseRow(QLineEdit *edit, const QString &filter, QWidget *pw)
{
    auto *row  = new QWidget(pw);
    auto *hbox = new QHBoxLayout(row);
    hbox->setContentsMargins(0, 0, 0, 0);
    hbox->addWidget(edit);
    auto *btn = new QPushButton("Browse…", row);
    btn->setFixedWidth(90);
    hbox->addWidget(btn);
    QObject::connect(btn, &QPushButton::clicked, edit, [edit, filter, pw]{
        const QString p = QFileDialog::getOpenFileName(pw, "Select File", {}, filter);
        if (!p.isEmpty()) edit->setText(p);
    });
    return row;
}

ManageServersDialog::ManageServersDialog(const QList<ServerConfig> &servers, QWidget *parent)
    : QDialog(parent)
    , m_servers(servers)
{
    setWindowTitle("Manage Servers");
    setMinimumSize(740, 520);
    resize(820, 580);

    // ── Left panel: server list + management buttons ─────────────────────────

    m_serverList = new QListWidget;
    m_serverList->setFrameShape(QFrame::NoFrame);

    auto *btnAdd    = new PillButton("Add");
    auto *btnRemove = new PillButton("Remove");
    auto *btnUp     = new PillButton("▲");
    auto *btnDown   = new PillButton("▼");
    // Compact arrows so the four buttons fit inside the fixed-width left column.
    btnUp->setFixedWidth(34);
    btnDown->setFixedWidth(34);

    connect(btnAdd,    &QPushButton::clicked, this, &ManageServersDialog::addServer);
    connect(btnRemove, &QPushButton::clicked, this, &ManageServersDialog::removeServer);
    connect(btnUp,     &QPushButton::clicked, this, &ManageServersDialog::moveUp);
    connect(btnDown,   &QPushButton::clicked, this, &ManageServersDialog::moveDown);

    auto *btnBar = new QHBoxLayout;
    btnBar->setContentsMargins(0, 0, 0, 0);
    btnBar->setSpacing(4);
    btnBar->addWidget(btnAdd);
    btnBar->addWidget(btnRemove);
    btnBar->addStretch();
    btnBar->addWidget(btnUp);
    btnBar->addWidget(btnDown);

    // Fixed-width left column: without this the button bar (wider than the list)
    // stretches the whole column and shoves the form far to the right.
    auto *leftContainer = new QWidget;
    leftContainer->setFixedWidth(230);
    auto *leftPanel = new QVBoxLayout(leftContainer);
    leftPanel->setContentsMargins(0, 0, 0, 0);
    leftPanel->addWidget(m_serverList, 1);
    leftPanel->addLayout(btnBar);

    // ── Right panel: server form ─────────────────────────────────────────────

    m_disabled = new QCheckBox("Disabled");
    m_disabled->setToolTip("Keep this server in the config but do not connect on startup");
    m_name     = new QLineEdit;
    m_host     = new QLineEdit;
    m_host->setPlaceholderText("irc.example.org");
    m_port     = new QSpinBox;
    m_port->setRange(1, 65535);
    m_port->setValue(6697);
    m_ssl       = new QCheckBox("Use SSL/TLS");
    m_ssl->setChecked(true);
    m_websocket = new QCheckBox("Use WebSocket (ws:// / wss://)");

    m_nick     = new QLineEdit;
    m_user     = new QLineEdit;
    m_realname = new QLineEdit;
    m_quitMessage = new QLineEdit;
    m_quitMessage->setPlaceholderText("Uplink  (default — leave blank to use this)");
    m_awayMessage = new QLineEdit;
    m_awayMessage->setPlaceholderText("e.g. Gone fishing — used by /away with no argument");

    m_password = new QLineEdit;
    m_password->setEchoMode(QLineEdit::Password);
    m_password->setPlaceholderText("optional");
    addEyeToggle(m_password);
    m_saslUser     = new QLineEdit;
    m_saslPassword = new QLineEdit;
    m_saslPassword->setEchoMode(QLineEdit::Password);
    addEyeToggle(m_saslPassword);
    m_saslExternal = new QCheckBox("Use SASL EXTERNAL (client certificate)");
    m_clientCert   = new QLineEdit;
    m_clientCert->setPlaceholderText("path/to/client.crt (PEM)");
    m_clientKey    = new QLineEdit;
    m_clientKey->setPlaceholderText("path/to/client.key (PEM)");
    m_nickservPassword = new QLineEdit;
    m_nickservPassword->setEchoMode(QLineEdit::Password);
    m_nickservPassword->setPlaceholderText("optional");
    addEyeToggle(m_nickservPassword);

    m_autoJoin = new QLineEdit;
    m_autoJoin->setPlaceholderText("#uplinkirc, #linux, #archlinux");

    auto *channelNote = new QLabel(
        "⚠ To join a password-protected channel, edit <b>config.toml</b> directly:<br/>"
        "<code>&nbsp;&nbsp;[[server.channel]]<br/>"
        "&nbsp;&nbsp;name = \"#private\"<br/>"
        "&nbsp;&nbsp;key &nbsp;= \"secretkey\"</code>");
    channelNote->setWordWrap(true);
    channelNote->setStyleSheet("font-size: 8pt; color: #aaaaaa; margin-top: 2px;");
    channelNote->setTextFormat(Qt::RichText);

    m_bouncerGroup   = new QButtonGroup(this);
    m_bouncerGroup->setExclusive(true);
    m_bouncerNetwork = new QLineEdit;
    m_bouncerNetwork->setPlaceholderText("e.g. libera, oftc");
    m_bouncerNetworkLabel = new QLabel("Network:");

    m_proxyHost = new QLineEdit;
    m_proxyHost->setPlaceholderText("e.g. 127.0.0.1 (leave blank for no proxy)");
    m_proxyPort = new QSpinBox;
    m_proxyPort->setRange(1, 65535);
    m_proxyPort->setValue(1080);
    m_proxyUser = new QLineEdit;
    m_proxyUser->setPlaceholderText("optional");
    m_proxyPass = new QLineEdit;
    m_proxyPass->setPlaceholderText("optional");
    m_proxyPass->setEchoMode(QLineEdit::Password);
    addEyeToggle(m_proxyPass);

    auto *form = new QFormLayout;
    form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    form->setLabelAlignment(Qt::AlignLeft);
    // Hug the left: without this the (width-capped) form centers itself in the
    // panel, leaving a big empty gap to the left of the labels.
    form->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);

    form->addRow(sectionHeader("Connection"));
    form->addRow("",         m_disabled);
    form->addRow("Name:",    m_name);
    form->addRow("Host:",    m_host);
    form->addRow("Port:",    m_port);
    form->addRow("",         m_ssl);
    form->addRow("",         m_websocket);

    form->addRow(sectionHeader("Identity"));
    form->addRow("Nick:",         m_nick);
    form->addRow("Username:",     m_user);
    form->addRow("Real Name:",    m_realname);
    form->addRow("Quit Message:", m_quitMessage);
    form->addRow("Away Message:", m_awayMessage);

    form->addRow(sectionHeader("Authentication"));
    form->addRow("Server Password:", m_password);
    form->addRow("SASL User:",       m_saslUser);
    form->addRow("SASL Password:",   m_saslPassword);
    form->addRow("",                 m_saslExternal);
    form->addRow("Client Cert:",     makeBrowseRow(m_clientCert, "Certificates (*.crt *.pem *.cer);;All (*)", this));
    form->addRow("Client Key:",      makeBrowseRow(m_clientKey,  "Keys (*.key *.pem);;All (*)", this));
    form->addRow("NickServ:",        m_nickservPassword);

    form->addRow(sectionHeader("Channels"));
    form->addRow("Auto-join:", m_autoJoin);
    form->addRow("", channelNote);

    form->addRow(sectionHeader("Bouncer"));
    {
        struct { const char *label; BouncerType type; } opts[] = {
            { "None", BouncerType::None },
            { "ZNC",  BouncerType::ZNC  },
            { "Soju", BouncerType::Soju },
        };
        auto *bouncerRow = new QHBoxLayout;
        for (auto &o : opts) {
            auto *rb = new QRadioButton(o.label);
            rb->setChecked(o.type == BouncerType::None);
            m_bouncerGroup->addButton(rb, static_cast<int>(o.type));
            bouncerRow->addWidget(rb);
        }
        bouncerRow->addStretch();
        form->addRow("Type:", bouncerRow);
    }
    form->addRow(m_bouncerNetworkLabel, m_bouncerNetwork);

    auto updateNetworkRow = [this]{
        const int t = m_bouncerGroup->checkedId();
        const bool show = t == static_cast<int>(BouncerType::Soju)
                       || t == static_cast<int>(BouncerType::ZNC);
        m_bouncerNetworkLabel->setVisible(show);
        m_bouncerNetwork->setVisible(show);
    };
    connect(m_bouncerGroup, &QButtonGroup::idClicked, this, updateNetworkRow);
    updateNetworkRow();

    form->addRow(sectionHeader("SOCKS5 Proxy"));
    form->addRow("Proxy Host:", m_proxyHost);
    form->addRow("Proxy Port:", m_proxyPort);
    form->addRow("Proxy User:", m_proxyUser);
    form->addRow("Proxy Pass:", m_proxyPass);

    m_formPanel = new QWidget;
    auto *formVbox = new QVBoxLayout(m_formPanel);
    // Small left gap (~4 spaces) between the network-list border and the labels;
    // right margin keeps the pill fields clear of the vertical scrollbar.
    formVbox->setContentsMargins(24, 0, 16, 0);
    formVbox->addLayout(form);
    formVbox->addStretch();

    auto *scroll = new QScrollArea;
    scroll->setWidget(m_formPanel);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // ── Bottom buttons ───────────────────────────────────────────────────────

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Ok)->setIcon(MenuIcons::confirm());
    buttons->button(QDialogButtonBox::Cancel)->setIcon(MenuIcons::close());
    connect(buttons, &QDialogButtonBox::accepted, this, [this]{ saveCurrentToModel(); accept(); });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // ── Assemble layout ──────────────────────────────────────────────────────

    auto *sep = new QFrame;
    sep->setFrameShape(QFrame::VLine);
    sep->setFrameShadow(QFrame::Sunken);

    auto *body = new QHBoxLayout;
    body->setContentsMargins(0, 0, 0, 0);
    body->setSpacing(8);
    body->addWidget(leftContainer);
    body->addWidget(sep);
    body->addWidget(scroll, 1);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);
    mainLayout->addLayout(body, 1);
    mainLayout->addWidget(buttons);

    // ── Initialize ───────────────────────────────────────────────────────────

    for (int i = 0; i < m_servers.size(); ++i)
        m_keychainStates.append(KeychainState{});

    connect(m_serverList, &QListWidget::currentRowChanged, this, &ManageServersDialog::onServerSelected);
    refreshList();

    if (m_servers.isEmpty())
        m_formPanel->setEnabled(false);
}

// ── List management ──────────────────────────────────────────────────────────

void ManageServersDialog::refreshList()
{
    const int prev = m_serverList->currentRow();
    m_serverList->clear();
    for (const ServerConfig &sc : std::as_const(m_servers)) {
        const QString label = sc.name.isEmpty() ? sc.host : sc.name;
        auto *item = new QListWidgetItem(MenuIcons::connectedServer(), label);
        item->setSizeHint(QSize(0, 32));
        m_serverList->addItem(item);
    }
    if (prev >= 0 && prev < m_serverList->count())
        m_serverList->setCurrentRow(prev);
    else if (m_serverList->count() > 0)
        m_serverList->setCurrentRow(0);
}

bool ManageServersDialog::nameExists(const QString &name, int excludeRow) const
{
    for (int i = 0; i < m_servers.size(); ++i) {
        if (i == excludeRow) continue;
        if (m_servers[i].name.compare(name, Qt::CaseInsensitive) == 0)
            return true;
    }
    return false;
}

void ManageServersDialog::addServer()
{
    ServerConfig sc;
    sc.port = 6697;
    sc.ssl  = true;
    m_servers.append(sc);
    m_keychainStates.append(KeychainState{});
    refreshList();
    m_serverList->setCurrentRow(static_cast<int>(m_servers.size()) - 1);
    m_formPanel->setEnabled(true);
    m_host->setFocus();
}

void ManageServersDialog::removeServer()
{
    const int row = m_serverList->currentRow();
    if (row < 0 || row >= m_servers.size()) return;
    m_currentRow = -1;
    m_servers.removeAt(row);
    m_keychainStates.removeAt(row);
    refreshList();
    if (m_servers.isEmpty())
        m_formPanel->setEnabled(false);
}

void ManageServersDialog::moveUp()
{
    const int row = m_serverList->currentRow();
    if (row <= 0 || row >= m_servers.size()) return;
    saveCurrentToModel();
    m_currentRow = -1;
    m_servers.swapItemsAt(row, row - 1);
    m_keychainStates.swapItemsAt(row, row - 1);
    refreshList();
    m_serverList->setCurrentRow(row - 1);
}

void ManageServersDialog::moveDown()
{
    const int row = m_serverList->currentRow();
    if (row < 0 || row >= m_servers.size() - 1) return;
    saveCurrentToModel();
    m_currentRow = -1;
    m_servers.swapItemsAt(row, row + 1);
    m_keychainStates.swapItemsAt(row, row + 1);
    refreshList();
    m_serverList->setCurrentRow(row + 1);
}

// ── Form ↔ model ─────────────────────────────────────────────────────────────

void ManageServersDialog::onServerSelected(int row)
{
    if (row == m_currentRow) return;
    saveCurrentToModel();
    loadServerToForm(row);
    m_currentRow = row;
}

void ManageServersDialog::saveCurrentToModel()
{
    if (m_currentRow < 0 || m_currentRow >= m_servers.size()) return;

    auto resolvePw = [&](QLineEdit *field, bool wasKeychain) {
        return (field->text().isEmpty() && wasKeychain) ? kKeychainSentinel : field->text();
    };

    ServerConfig &sc = m_servers[m_currentRow];
    KeychainState &ks = m_keychainStates[m_currentRow];

    sc.disabled       = m_disabled->isChecked();
    sc.name           = m_name->text().trimmed();
    sc.host           = m_host->text().trimmed();
    sc.port           = static_cast<quint16>(m_port->value());
    sc.ssl            = m_ssl->isChecked();
    sc.websocket      = m_websocket->isChecked();
    sc.nick           = m_nick->text().trimmed();
    sc.user           = m_user->text().trimmed();
    sc.realname       = m_realname->text().trimmed();
    sc.quitMessage    = m_quitMessage->text().trimmed();
    sc.awayMessage    = m_awayMessage->text().trimmed();
    sc.password       = resolvePw(m_password, ks.password);
    sc.saslUser       = m_saslUser->text().trimmed();
    sc.saslPassword   = resolvePw(m_saslPassword, ks.saslPassword);
    sc.saslExternal   = m_saslExternal->isChecked();
    sc.clientCertFile = m_clientCert->text().trimmed();
    sc.clientKeyFile  = m_clientKey->text().trimmed();
    sc.nickservPassword = resolvePw(m_nickservPassword, ks.nickservPassword);
    sc.bouncerType    = static_cast<BouncerType>(m_bouncerGroup->checkedId());
    sc.bouncerNetwork = m_bouncerNetwork->text().trimmed();
    sc.proxyHost      = m_proxyHost->text().trimmed();
    sc.proxyPort      = static_cast<quint16>(m_proxyPort->value());
    sc.proxyUser      = m_proxyUser->text().trimmed();
    sc.proxyPass      = resolvePw(m_proxyPass, ks.proxyPass);

    sc.channels.clear();
    for (const QString &ch : m_autoJoin->text().split(',', Qt::SkipEmptyParts)) {
        const QString name = ch.trimmed();
        if (!name.isEmpty())
            sc.channels.append({name, {}});
    }

    // Update list label
    const QString label = sc.name.isEmpty() ? sc.host : sc.name;
    if (auto *item = m_serverList->item(m_currentRow))
        item->setText(label);
}

void ManageServersDialog::loadServerToForm(int row)
{
    if (row < 0 || row >= m_servers.size()) return;

    static const QString kPlaceholder = QStringLiteral("Stored in keychain - type to change, clear to remove");
    static const QString kOptional    = QStringLiteral("optional");

    const ServerConfig &sc = m_servers[row];
    KeychainState &ks = m_keychainStates[row];

    auto loadPw = [&](QLineEdit *field, const QString &value, bool &flag) {
        field->clear();
        if (value == kKeychainSentinel) {
            field->setPlaceholderText(kPlaceholder);
            flag = true;
        } else {
            field->setPlaceholderText(kOptional);
            field->setText(value);
            flag = false;
        }
    };

    m_disabled->setChecked(sc.disabled);
    m_name->setText(sc.name);
    m_host->setText(sc.host);
    m_port->setValue(sc.port ? sc.port : 6697);
    m_ssl->setChecked(sc.ssl);
    m_websocket->setChecked(sc.websocket);
    m_nick->setText(sc.nick);
    m_user->setText(sc.user);
    m_realname->setText(sc.realname);
    m_quitMessage->setText(sc.quitMessage);
    m_awayMessage->setText(sc.awayMessage);

    loadPw(m_password,         sc.password,         ks.password);
    m_saslUser->setText(sc.saslUser);
    loadPw(m_saslPassword,     sc.saslPassword,     ks.saslPassword);
    m_saslExternal->setChecked(sc.saslExternal);
    m_clientCert->setText(sc.clientCertFile);
    m_clientKey->setText(sc.clientKeyFile);
    loadPw(m_nickservPassword, sc.nickservPassword, ks.nickservPassword);

    if (auto *rb = m_bouncerGroup->button(static_cast<int>(sc.bouncerType)))
        rb->setChecked(true);
    m_bouncerNetwork->setText(sc.bouncerNetwork);
    {
        const int t = static_cast<int>(sc.bouncerType);
        const bool show = t == static_cast<int>(BouncerType::Soju)
                       || t == static_cast<int>(BouncerType::ZNC);
        m_bouncerNetworkLabel->setVisible(show);
        m_bouncerNetwork->setVisible(show);
    }

    QStringList names;
    for (const auto &ch : sc.channels)
        names << ch.name;
    m_autoJoin->setText(names.join(","));

    m_proxyHost->setText(sc.proxyHost);
    m_proxyPort->setValue(sc.proxyPort ? sc.proxyPort : 1080);
    m_proxyUser->setText(sc.proxyUser);
    loadPw(m_proxyPass, sc.proxyPass, ks.proxyPass);
}

QList<ServerConfig> ManageServersDialog::servers()
{
    saveCurrentToModel();
    return m_servers;
}
