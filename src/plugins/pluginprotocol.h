#pragma once

#include <QByteArray>
#include <QString>

#include "model/message.h"

// JSON-lines protocol between Uplink and plugin processes.
//
// Uplink writes one JSON object per line to the plugin's stdin (events);
// the plugin writes one JSON object per line to stdout (actions).
// Pure Qt Core so the whole codec is unit-testable (tst_pluginprotocol).
namespace PluginProtocol {

// --- Uplink -> plugin -------------------------------------------------------

// {"event":"hello","version":"..."} — first line a plugin sees. Your own
// nick isn't included: it can differ per server; every message event
// carries a "self" flag instead.
QByteArray helloEvent(const QString &version);

// Maps a buffer message to its event line. Returns an empty array for
// message types plugins don't get (Server/Reply/Wallops/Error/Topic) and
// for the synthetic empty-nick Join/Quit lines netsplit detection posts.
QByteArray messageEvent(const QString &server, const QString &buffer,
                        const Message &msg, bool self, bool mentionsYou);

// {"event":"topic","server":...,"buffer":...,"text":<topic>}
QByteArray topicEvent(const QString &server, const QString &buffer,
                      const QString &topic);

// {"event":"connected"|"disconnected","server":...}
QByteArray serverEvent(const QString &event, const QString &server);

// --- plugin -> Uplink -------------------------------------------------------

struct PluginAction {
    enum Kind { Invalid, Say, Me, Print, Command };
    Kind    kind{Invalid};
    QString server;
    QString buffer;
    QString text;   // message text, or the slash-command line for Command
    QString error;  // set when kind == Invalid: reason for the log
};

// Parses one stdout line from a plugin. Never throws; malformed input
// yields Kind::Invalid with error set.
PluginAction parseAction(const QByteArray &line);

} // namespace PluginProtocol
