#include "pluginprotocol.h"

#include <QJsonDocument>
#include <QJsonObject>

namespace PluginProtocol {

static QByteArray toLine(const QJsonObject &obj)
{
    return QJsonDocument(obj).toJson(QJsonDocument::Compact) + '\n';
}

QByteArray helloEvent(const QString &version)
{
    return toLine({ {"event", "hello"}, {"version", version} });
}

QByteArray messageEvent(const QString &server, const QString &buffer,
                        const Message &msg, bool self, bool mentionsYou)
{
    QString event;
    QString kind;
    switch (msg.type) {
    case MessageType::Privmsg: event = "message"; kind = "message"; break;
    case MessageType::Action:  event = "message"; kind = "action";  break;
    case MessageType::Notice:  event = "message"; kind = "notice";  break;
    case MessageType::Join:    event = "join"; break;
    case MessageType::Part:    event = "part"; break;
    case MessageType::Quit:    event = "quit"; break;
    case MessageType::Kick:    event = "kick"; break;
    case MessageType::Nick:    event = "nick"; break;
    default:                   return {};  // Server/Reply/Wallops/Topic/Error
    }

    // Netsplit/netjoin post Join/Quit lines with no nick — not real events.
    if (msg.nick.isEmpty()) return {};

    QJsonObject o{
        {"event",  event},
        {"server", server},
        {"buffer", buffer},
        {"nick",   msg.nick},
        {"text",   msg.text},
        {"time",   msg.timestamp.toUTC().toString(Qt::ISODate)},
        {"self",   self},
    };
    if (event == "message") {
        o.insert("kind", kind);
        o.insert("mentions_you", mentionsYou);
        if (!msg.msgid.isEmpty())   o.insert("msgid", msg.msgid);
        if (!msg.account.isEmpty()) o.insert("account", msg.account);
    } else if (event == "nick") {
        o.insert("old", msg.nick);
        o.insert("new", msg.replyTo);  // onNickChanged stores the new nick here
    }
    return toLine(o);
}

QByteArray topicEvent(const QString &server, const QString &buffer, const QString &topic)
{
    return toLine({ {"event", "topic"}, {"server", server},
                    {"buffer", buffer}, {"text", topic} });
}

QByteArray serverEvent(const QString &event, const QString &server)
{
    return toLine({ {"event", event}, {"server", server} });
}

PluginAction parseAction(const QByteArray &line)
{
    PluginAction a;
    const QByteArray trimmed = line.trimmed();
    if (trimmed.isEmpty()) { a.error = "empty line"; return a; }

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(trimmed, &err);
    if (doc.isNull() || !doc.isObject()) {
        a.error = "not a JSON object: " + err.errorString();
        return a;
    }
    const QJsonObject o = doc.object();
    const QString action = o.value("action").toString();

    if (action == "say")          a.kind = PluginAction::Say;
    else if (action == "me")      a.kind = PluginAction::Me;
    else if (action == "print")   a.kind = PluginAction::Print;
    else if (action == "command") a.kind = PluginAction::Command;
    else {
        a.error = action.isEmpty() ? QStringLiteral("missing \"action\" field")
                                   : "unknown action \"" + action + "\"";
        return a;
    }

    a.server = o.value("server").toString();
    a.buffer = o.value("buffer").toString();
    a.text   = o.value(a.kind == PluginAction::Command ? "line" : "text").toString();

    if (a.text.isEmpty()) {
        a.kind  = PluginAction::Invalid;
        a.error = "empty text";
        return a;
    }
    if (a.server.isEmpty()) {
        a.kind  = PluginAction::Invalid;
        a.error = "missing \"server\" field";
        return a;
    }
    if (a.buffer.isEmpty() && a.kind != PluginAction::Command) {
        a.kind  = PluginAction::Invalid;
        a.error = "missing \"buffer\" field";
        return a;
    }
    return a;
}

} // namespace PluginProtocol
