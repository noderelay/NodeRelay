#include "serverdialog.h"
#include "menuicons.h"

#include <QCheckBox>
#include <QButtonGroup>
#include <QRadioButton>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QVBoxLayout>

ServerDialog::ServerDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Add Server");
    setMinimumWidth(380);

    m_disabled = new QCheckBox("Disabled — keep in config but do not connect on startup");
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
    auto addEyeToggle = [](QLineEdit *field) {
        auto *act = field->addAction(MenuIcons::eyeOff(), QLineEdit::TrailingPosition);
        QObject::connect(act, &QAction::triggered, field, [field, act] {
            const bool show = field->echoMode() == QLineEdit::Password;
            field->setEchoMode(show ? QLineEdit::Normal : QLineEdit::Password);
            act->setIcon(show ? MenuIcons::eye() : MenuIcons::eyeOff());
        });
    };

    m_password = new QLineEdit;
    m_password->setEchoMode(QLineEdit::Password);
    m_password->setPlaceholderText("optional");
    addEyeToggle(m_password);
    m_saslUser         = new QLineEdit;
    m_saslPassword     = new QLineEdit;
    m_saslPassword->setEchoMode(QLineEdit::Password);
    addEyeToggle(m_saslPassword);
    m_saslExternal     = new QCheckBox("Use SASL EXTERNAL (client certificate)");
    m_clientCert       = new QLineEdit;
    m_clientCert->setPlaceholderText("path/to/client.crt (PEM)");
    m_clientKey        = new QLineEdit;
    m_clientKey->setPlaceholderText("path/to/client.key (PEM)");
    m_nickservPassword = new QLineEdit;
    m_nickservPassword->setEchoMode(QLineEdit::Password);
    m_nickservPassword->setPlaceholderText("optional");
    addEyeToggle(m_nickservPassword);

    auto makeHeader = [](const QString &text) {
        auto *lbl = new QLabel(text);
        lbl->setStyleSheet("font-weight: bold; margin-top: 6px;");
        return lbl;
    };

    auto *form = new QFormLayout;
    form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    form->addRow(makeHeader("Connection"));
    form->addRow("",         m_disabled);
    form->addRow("Name:",    m_name);
    form->addRow("Host:",    m_host);
    form->addRow("Port:",    m_port);
    form->addRow("",         m_ssl);
    form->addRow("",         m_websocket);

    m_quitMessage = new QLineEdit;
    m_quitMessage->setPlaceholderText("Uplink  (default — leave blank to use this)");
    m_awayMessage = new QLineEdit;
    m_awayMessage->setPlaceholderText("e.g. Gone fishing — used by /away with no argument");

    form->addRow(makeHeader("Identity"));
    form->addRow("Nick:",         m_nick);
    form->addRow("Username:",     m_user);
    form->addRow("Real Name:",    m_realname);
    form->addRow("Quit Message:", m_quitMessage);
    form->addRow("Away Message:", m_awayMessage);

    auto makeBrowseRow = [](QLineEdit *edit, const QString &filter, QWidget *pw) {
        auto *row    = new QWidget(pw);
        auto *hbox   = new QHBoxLayout(row);
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
    };

    form->addRow(makeHeader("Authentication"));
    form->addRow("Server Password:", m_password);
    form->addRow("SASL User:",       m_saslUser);
    form->addRow("SASL Password:",   m_saslPassword);
    form->addRow("",                 m_saslExternal);
    form->addRow("Client Cert:",     makeBrowseRow(m_clientCert, "Certificates (*.crt *.pem *.cer);;All (*)", this));
    form->addRow("Client Key:",      makeBrowseRow(m_clientKey,  "Keys (*.key *.pem);;All (*)", this));
    form->addRow("NickServ:",        m_nickservPassword);

    m_autoJoin = new QLineEdit;
    m_autoJoin->setPlaceholderText("#uplink, #linux, #archlinux");

    m_bouncerGroup = new QButtonGroup(this);
    m_bouncerGroup->setExclusive(true);
    m_bouncerNetwork = new QLineEdit;
    m_bouncerNetwork->setPlaceholderText("e.g. libera, oftc");

    auto *channelNote = new QLabel(
        "⚠ To join a password-protected channel, edit <b>config.toml</b> directly:<br/>"
        "<code>&nbsp;&nbsp;[[server.channel]]<br/>"
        "&nbsp;&nbsp;name = \"#private\"<br/>"
        "&nbsp;&nbsp;key &nbsp;= \"secretkey\"</code>");
    channelNote->setWordWrap(true);
    channelNote->setStyleSheet("font-size: 8pt; color: #aaaaaa; margin-top: 2px;");
    channelNote->setTextFormat(Qt::RichText);

    form->addRow(makeHeader("Channels"));
    form->addRow("Auto-join:", m_autoJoin);
    form->addRow("", channelNote);

    m_bouncerNetworkLabel = new QLabel("Network:");

    form->addRow(makeHeader("Bouncer"));
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

    form->addRow(makeHeader("SOCKS5 Proxy"));
    form->addRow("Proxy Host:", m_proxyHost);
    form->addRow("Proxy Port:", m_proxyPort);
    form->addRow("Proxy User:", m_proxyUser);
    form->addRow("Proxy Pass:", m_proxyPass);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Ok)->setIcon(MenuIcons::confirm());
    buttons->button(QDialogButtonBox::Cancel)->setIcon(MenuIcons::close());
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *scrollWidget = new QWidget;
    auto *scrollLayout = new QVBoxLayout(scrollWidget);
    scrollLayout->setContentsMargins(0, 0, 0, 0);
    scrollLayout->addLayout(form);

    auto *scroll = new QScrollArea;
    scroll->setWidget(scrollWidget);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(scroll);
    layout->addWidget(buttons);
}

ServerDialog::ServerDialog(const ServerConfig &existing, QWidget *parent)
    : ServerDialog(parent)
{
    setWindowTitle("Edit Server");
    m_disabled->setChecked(existing.disabled);
    m_name->setText(existing.name);
    m_host->setText(existing.host);
    m_port->setValue(existing.port);
    m_ssl->setChecked(existing.ssl);
    m_websocket->setChecked(existing.websocket);
    m_nick->setText(existing.nick);
    m_user->setText(existing.user);
    m_realname->setText(existing.realname);
    m_quitMessage->setText(existing.quitMessage);
    m_awayMessage->setText(existing.awayMessage);
    static const QString kPlaceholder = QStringLiteral("Stored in keychain - type to change, clear to remove");
    auto setupPw = [&](QLineEdit *field, const QString &value, bool &flag) {
        if (value == kKeychainSentinel) { field->setPlaceholderText(kPlaceholder); flag = true; }
        else                            { field->setText(value); }
    };
    setupPw(m_password,         existing.password,         m_passwordKeychain);
    m_saslUser->setText(existing.saslUser);
    setupPw(m_saslPassword,     existing.saslPassword,     m_saslPasswordKeychain);
    m_saslExternal->setChecked(existing.saslExternal);
    m_clientCert->setText(existing.clientCertFile);
    m_clientKey->setText(existing.clientKeyFile);
    setupPw(m_nickservPassword, existing.nickservPassword, m_nickservPasswordKeychain);
    if (auto *rb = m_bouncerGroup->button(static_cast<int>(existing.bouncerType)))
        rb->setChecked(true);
    m_bouncerNetwork->setText(existing.bouncerNetwork);
    m_proxyHost->setText(existing.proxyHost);
    m_proxyPort->setValue(existing.proxyPort ? existing.proxyPort : 1080);
    m_proxyUser->setText(existing.proxyUser);
    setupPw(m_proxyPass, existing.proxyPass, m_proxyPassKeychain);

    QStringList names;
    for (const auto &ch : existing.channels)
        names << ch.name;
    m_autoJoin->setText(names.join(","));
}

ServerConfig ServerDialog::serverConfig() const
{
    ServerConfig sc;
    sc.disabled         = m_disabled->isChecked();
    sc.name             = m_name->text().trimmed();
    sc.host             = m_host->text().trimmed();
    sc.port             = static_cast<quint16>(m_port->value());
    sc.ssl              = m_ssl->isChecked();
    sc.websocket        = m_websocket->isChecked();
    sc.nick             = m_nick->text().trimmed();
    sc.user             = m_user->text().trimmed();
    sc.realname         = m_realname->text().trimmed();
    sc.quitMessage      = m_quitMessage->text().trimmed();
    sc.awayMessage      = m_awayMessage->text().trimmed();
    auto resolvePw = [&](QLineEdit *field, bool wasKeychain) {
        return (field->text().isEmpty() && wasKeychain) ? kKeychainSentinel : field->text();
    };
    sc.password         = resolvePw(m_password,     m_passwordKeychain);
    sc.saslUser         = m_saslUser->text().trimmed();
    sc.saslPassword     = resolvePw(m_saslPassword, m_saslPasswordKeychain);
    sc.saslExternal     = m_saslExternal->isChecked();
    sc.clientCertFile   = m_clientCert->text().trimmed();
    sc.clientKeyFile    = m_clientKey->text().trimmed();
    sc.nickservPassword = resolvePw(m_nickservPassword, m_nickservPasswordKeychain);
    sc.bouncerType      = static_cast<BouncerType>(m_bouncerGroup->checkedId());
    sc.bouncerNetwork   = m_bouncerNetwork->text().trimmed();
    sc.proxyHost        = m_proxyHost->text().trimmed();
    sc.proxyPort        = static_cast<quint16>(m_proxyPort->value());
    sc.proxyUser        = m_proxyUser->text().trimmed();
    sc.proxyPass        = resolvePw(m_proxyPass, m_proxyPassKeychain);
    for (const QString &ch : m_autoJoin->text().split(',', Qt::SkipEmptyParts)) {
        const QString name = ch.trimmed();
        if (!name.isEmpty())
            sc.channels.append({name, {}});
    }
    return sc;
}
