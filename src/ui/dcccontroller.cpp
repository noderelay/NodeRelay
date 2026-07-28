#include "ui/dcccontroller.h"
#include "irc/ircclient.h"
#include "irc/dccsend.h"
#include "irc/dccreceive.h"
#include "model/sessionmodel.h"
#include "net/addresscheck.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QHostAddress>
#include <QMessageBox>
#include <QPointer>
#include <QProgressDialog>
#include <QRegularExpression>
#include <QTimer>

#include <climits>

DccController::DccController(SessionModel *model, QWidget *parentWindow)
    : QObject(parentWindow), m_model(model), m_window(parentWindow)
{
    // Queued: these slots open modal dialogs, which must not run inside the
    // socket's readyRead signal chain (nested event loop re-enters the client).
    connect(m_model, &SessionModel::dccSendReceived,
            this, &DccController::onSendReceived, Qt::QueuedConnection);
    connect(m_model, &SessionModel::dccPassiveOfferReceived,
            this, &DccController::onPassiveOfferReceived, Qt::QueuedConnection);
    connect(m_model, &SessionModel::dccPassiveSendReply,
            this, &DccController::onPassiveSendReply, Qt::QueuedConnection);
}

// The address we put in outgoing DCC offers. Behind NAT the socket's local
// address is a LAN one the peer can't reach. Chain: the public address the
// network showed for us (fresher than any hand-set value on dynamic IPs),
// then the [dcc] external_ip override, then the socket address.
quint32 DccController::advertisedIp(IrcClient *client) const
{
    if (client) {
        if (const quint32 discovered = client->externalIpv4())
            return discovered;
    }
    const DccConfig &dcc = m_model->dccConfig();
    if (!dcc.externalIp.isEmpty()) {
        bool ok = false;
        const quint32 v4 = QHostAddress(dcc.externalIp).toIPv4Address(&ok);
        if (ok) return v4;
    }
    return client ? client->localIpv4() : 0;
}

void DccController::onSendReceived(const ServerId &, const QString &fromNick,
                                   const QString &filename, quint32 ip, quint16 port, qint64 filesize)
{
    const QString sizeStr = filesize >= 1024*1024
        ? QString::number(filesize / (1024*1024)) + " MB"
        : QString::number(filesize / 1024) + " KB";
    const QString ipStr = QHostAddress(ip).toString();

    const int ret = QMessageBox::question(m_window, "Incoming DCC File",
        "Sender: " + fromNick + "\n"
        "File: " + filename + "\n"
        "Size: " + sizeStr + "\n"
        "Address: " + ipStr + ":" + QString::number(port) + "\n\n"
        "DCC connects directly to the sender and may reveal your IP address.\n"
        "Only accept files from people you trust.\n\nAccept?",
        QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) return;

    const QString savePath = QFileDialog::getSaveFileName(
        m_window, "Save File", QFileInfo(filename).fileName());
    if (savePath.isEmpty()) return;

    auto *dcc  = new DccReceive(savePath, ip, port, filesize, this);
    QPointer<DccReceive> dccGuard(dcc);
    auto *prog = new QProgressDialog("Receiving " + filename + " from " + fromNick,
                                      "Cancel", 0, filesize > INT_MAX ? INT_MAX : static_cast<int>(filesize), m_window);
    prog->setWindowModality(Qt::NonModal);
    prog->setAttribute(Qt::WA_DeleteOnClose);

    connect(dcc, &DccReceive::progress, prog, [prog, filesize](qint64 received, qint64){
        prog->setValue(static_cast<int>(filesize > INT_MAX
            ? received * INT_MAX / filesize : received));
    });
    connect(dcc, &DccReceive::finished, this, [this, prog, dccGuard](const QString &path){
        prog->setValue(prog->maximum());
        prog->close(); // setValue(max) only hides; close() lets WA_DeleteOnClose fire
        if (dccGuard) dccGuard->deleteLater();
        QMessageBox::information(m_window, "DCC", "File received:\n" + path);
    });
    connect(dcc, &DccReceive::error, this, [this, prog, dccGuard](const QString &msg){
        prog->close();
        if (dccGuard) dccGuard->deleteLater();
        QMessageBox::warning(m_window, "DCC Error", msg);
    });
    connect(prog, &QProgressDialog::canceled, dcc, [dccGuard]{
        if (dccGuard) { dccGuard->cancel(); dccGuard->deleteLater(); }
    });

    // Show before start(): start() can emit error() synchronously, and its
    // handler close()s the dialog — show() afterwards would resurrect it.
    prog->show();
    dcc->start();
}

void DccController::onPassiveOfferReceived(const ServerId &server, const QString &fromNick,
                                           const QString &filename, quint32 senderIp,
                                           qint64 filesize, const QString &token)
{
    const QString sizeStr = filesize >= 1024*1024
        ? QString::number(filesize / (1024*1024)) + " MB"
        : QString::number(filesize / 1024) + " KB";
    const QString ipStr = QHostAddress(senderIp).toString();

    const int ret = QMessageBox::question(m_window, "Incoming DCC File (Passive)",
        "Sender: " + fromNick + "\n"
        "File: " + filename + "\n"
        "Size: " + sizeStr + "\n"
        "Sender address: " + ipStr + "\n\n"
        "DCC connects directly to the sender and may reveal your IP address.\n"
        "Only accept files from people you trust.\n\nAccept?",
        QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) return;

    const QString savePath = QFileDialog::getSaveFileName(
        m_window, "Save File", QFileInfo(filename).fileName());
    if (savePath.isEmpty()) return;

    const DccConfig &dccCfg = m_model->dccConfig();
    auto *dcc = new DccReceive(savePath, 0, 0, filesize, this);
    if (!dcc->listenPassive(senderIp, dccCfg.portMin, dccCfg.portMax)) { dcc->deleteLater(); return; }
    QPointer<DccReceive> dccGuard(dcc);

    IrcClient *client = m_model->clientFor(server);
    const quint32 ourIp   = advertisedIp(client);
    const quint16 ourPort = dcc->listenPort();
    QString fn = QFileInfo(filename).fileName().replace(' ', '_');
    static const QRegularExpression kCtrlChars(QStringLiteral("[\\x00-\\x1f\\x7f]"));
    fn.remove(kCtrlChars);
    if (fn.isEmpty()) fn = QStringLiteral("file");
    fn = fn.left(180);

    m_model->sendRaw(server,
        "PRIVMSG " + fromNick + " :\x01""DCC SEND "
        + fn + " " + QString::number(ourIp)
        + " " + QString::number(ourPort)
        + " " + QString::number(filesize)
        + " " + token + "\x01");

    auto *prog = new QProgressDialog("Receiving " + filename + " from " + fromNick,
                                      "Cancel", 0, filesize > INT_MAX ? INT_MAX : static_cast<int>(filesize), m_window);
    prog->setWindowModality(Qt::NonModal);
    prog->setAttribute(Qt::WA_DeleteOnClose);

    connect(dcc, &DccReceive::progress, prog, [prog, filesize](qint64 received, qint64){
        prog->setValue(static_cast<int>(filesize > INT_MAX ? received * INT_MAX / filesize : received));
    });
    connect(dcc, &DccReceive::finished, this, [this, prog, dccGuard](const QString &path){
        prog->setValue(prog->maximum());
        prog->close(); // setValue(max) only hides; close() lets WA_DeleteOnClose fire
        if (dccGuard) dccGuard->deleteLater();
        QMessageBox::information(m_window, "DCC", "File received:\n" + path);
    });
    connect(dcc, &DccReceive::error, this, [this, prog, dccGuard](const QString &msg){
        prog->close();
        if (dccGuard) dccGuard->deleteLater();
        QMessageBox::warning(m_window, "DCC Error", msg);
    });
    connect(prog, &QProgressDialog::canceled, dcc, [dccGuard]{
        if (dccGuard) { dccGuard->cancel(); dccGuard->deleteLater(); }
    });
    prog->show();
}

void DccController::onPassiveSendReply(const ServerId &, const QString &, const QString &,
                                       quint32 ip, quint16 port, qint64, const QString &token)
{
    DccSend *dcc = m_pendingPassiveSends.take(token);
    if (dcc) {
        if (isPrivateAddress(QHostAddress(ip))) {
            QMessageBox::warning(m_window, "DCC", "Blocked: remote address is private or reserved.");
            dcc->deleteLater();
        } else {
            dcc->connectOut(ip, port);
        }
    }
}

void DccController::sendFile(const ServerId &host, const QString &nick)
{
    const QString path = QFileDialog::getOpenFileName(m_window, "Send File to " + nick);
    if (path.isEmpty()) return;

    IrcClient *client = m_model->clientFor(host);
    if (!client) return;

    // Bind to the socket's local address; advertise the effective one (they
    // differ behind NAT when [dcc] external_ip is set).
    const quint32 localIp = client->localIpv4();
    const DccConfig &dccCfg = m_model->dccConfig();
    auto *dcc = new DccSend(path, this);
    if (!dcc->listen(localIp ? QHostAddress(localIp) : QHostAddress::Any,
                     std::nullopt, dccCfg.portMin, dccCfg.portMax)) {
        dcc->deleteLater(); return;
    }
    QPointer<DccSend> dccGuard(dcc);

    const quint32 ip   = advertisedIp(client);
    const quint16 port = dcc->port();
    const QString fn   = dcc->filename();
    const qint64  size = dcc->filesize();

    m_model->sendRaw(host,
        "PRIVMSG " + nick + " :\x01""DCC SEND "
        + fn + " " + QString::number(ip)
        + " " + QString::number(port)
        + " " + QString::number(size) + "\x01");

    auto *prog = new QProgressDialog("Sending " + fn + " to " + nick,
                                      "Cancel", 0, size > INT_MAX ? INT_MAX : static_cast<int>(size), m_window);
    prog->setWindowModality(Qt::NonModal);
    prog->setAttribute(Qt::WA_DeleteOnClose);

    connect(dcc, &DccSend::progress, prog, [prog, size](qint64 sent, qint64){
        prog->setValue(static_cast<int>(size > INT_MAX ? sent * INT_MAX / size : sent));
    });
    connect(dcc, &DccSend::finished, prog, [prog, dccGuard]{
        prog->setValue(prog->maximum());
        prog->close(); // setValue(max) only hides; close() lets WA_DeleteOnClose fire
        if (dccGuard) dccGuard->deleteLater();
    });
    connect(dcc, &DccSend::error, this, [this, prog, dccGuard](const QString &msg){
        prog->close();
        if (dccGuard) dccGuard->deleteLater();
        QMessageBox::warning(m_window, "DCC Error", msg);
    });
    connect(prog, &QProgressDialog::canceled, dcc, [dccGuard]{
        if (dccGuard) { dccGuard->cancel(); dccGuard->deleteLater(); }
    });

    prog->show();
}

void DccController::sendFilePassive(const ServerId &host, const QString &nick)
{
    const QString path = QFileDialog::getOpenFileName(m_window, "Send File to " + nick + " (Passive)");
    if (path.isEmpty()) return;

    auto *dcc = new DccSend(path, this);
    const QString token = dcc->initPassive();
    if (token.isEmpty()) { dcc->deleteLater(); return; }

    const QString fn   = dcc->filename();
    const qint64  size = dcc->filesize();

    QPointer<DccSend> dccGuard(dcc);
    m_pendingPassiveSends.insert(token, dcc);
    m_model->sendRaw(host,
        "PRIVMSG " + nick + " :\x01""DCC SEND "
        + fn + " 0 0"
        + " " + QString::number(size)
        + " " + token + "\x01");

    // Fix #4: timeout stale passive sends after 120s
    QTimer::singleShot(120000, this, [this, token, dccGuard]{
        if (DccSend *d = m_pendingPassiveSends.take(token)) {
            d->cancel();
            if (dccGuard) dccGuard->deleteLater();
        }
    });

    auto *prog = new QProgressDialog("Waiting for " + nick + " to accept...",
                                      "Cancel", 0, size > INT_MAX ? INT_MAX : static_cast<int>(size), m_window);
    prog->setWindowModality(Qt::NonModal);
    prog->setAttribute(Qt::WA_DeleteOnClose);

    connect(dcc, &DccSend::progress, prog, [prog, size](qint64 sent, qint64){
        prog->setValue(static_cast<int>(size > INT_MAX ? sent * INT_MAX / size : sent));
    });
    connect(dcc, &DccSend::finished, prog, [prog, dccGuard]{
        prog->setValue(prog->maximum());
        prog->close(); // setValue(max) only hides; close() lets WA_DeleteOnClose fire
        if (dccGuard) dccGuard->deleteLater();
    });
    connect(dcc, &DccSend::error, this, [this, prog, dccGuard, token](const QString &msg){
        prog->close();
        m_pendingPassiveSends.remove(token);
        if (dccGuard) dccGuard->deleteLater();
        QMessageBox::warning(m_window, "DCC Error", msg);
    });
    connect(prog, &QProgressDialog::canceled, dcc, [this, dccGuard, token]{
        m_pendingPassiveSends.remove(token);
        if (dccGuard) { dccGuard->cancel(); dccGuard->deleteLater(); }
    });
    prog->show();
}
