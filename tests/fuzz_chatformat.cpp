// libFuzzer harness for the ChatRenderer formatting path.
//
// Feeds hostile message content (mIRC formatting codes, unterminated color
// sequences, RTL overrides, emoji/ZWJ runs, absurd lengths) through both the
// ChatLine and HTML renderers.
//
// Input layout:
//   byte 0 — message type (low nibble), history/redacted/reply/group flags (high)
//   byte 1 — Context variations (colored nicks, brackets, emoji size, theme)
//   rest   — UTF-8; first line is the nick, remainder is the message text
//
// Build modes:
//   Standalone replay (default, any compiler):
//     cmake -B build -DUPLINK_BUILD_TESTS=ON
//     ./build/fuzz_chatformat tests/corpus_chatformat/*
//
//   libFuzzer (requires clang, ASan, libFuzzer):
//     cmake -B build-fuzz -DCMAKE_CXX_COMPILER=clang++ \
//           -DUPLINK_BUILD_TESTS=ON -DUPLINK_BUILD_FUZZ=ON
//     ./build-fuzz/fuzz_chatformat tests/corpus_chatformat/
//
// Crash reproduction:
//   ./build/fuzz_chatformat crash-<id>

#include "ui/chatrenderer.h"
#include "model/message.h"

#include <QCoreApplication>
#include <cstddef>
#include <cstdint>

#ifdef UPLINK_LIBFUZZER
// ChatLine holds a QPixmap, whose constructor aborts without an application
// object. The standalone driver creates one in main(); libFuzzer needs it here.
extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    static QCoreApplication app(*argc, *argv);
    return 0;
}
#endif

// The fuzzer entry point — called by libFuzzer or the standalone driver below.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 2)
        return 0;

    const uint8_t typeByte = data[0];
    const uint8_t ctxByte  = data[1];

    const QString payload = QString::fromUtf8(
        reinterpret_cast<const char *>(data + 2), static_cast<qsizetype>(size - 2));
    const qsizetype nl = payload.indexOf(QLatin1Char('\n'));
    const QString nick = nl < 0 ? QStringLiteral("nick") : payload.left(nl);
    const QString text = nl < 0 ? payload : payload.mid(nl + 1);

    // Fixed timestamp keeps runs deterministic.
    const QDateTime ts = QDateTime::fromSecsSinceEpoch(1700000000);

    Message msg = Message::make(static_cast<MessageType>((typeByte & 0x0F) % 13),
                                nick, text, ts);
    msg.isHistory = (typeByte & 0x10) != 0;
    msg.redacted  = (typeByte & 0x20) != 0;
    if (typeByte & 0x40) {
        msg.msgid   = QStringLiteral("m1");
        msg.replyTo = QStringLiteral("m0");
        msg.account = nick;
    }

    ChatRenderer::Context ctx;
    ctx.coloredNicks   = (ctxByte & 1) != 0;
    ctx.showTimestamps = (ctxByte & 2) != 0;
    ctx.nickBrackets   = (ctxByte & 4) ? QStringLiteral("<>") : QString();
    ctx.emojiPt        = (ctxByte & 8) ? 10.0 : 0.0;
    ctx.validTheme     = (ctxByte & 16) != 0;
    ctx.themeText      = QStringLiteral("#d0d0d0");
    // Mirror MainWindow's construction: selfNickRe has a capture group,
    // highlightRe is a bare \b...\b alternation.
    ctx.selfNickRe  = QRegularExpression(
        "(\\b" + QRegularExpression::escape(nick) + "\\b)",
        QRegularExpression::CaseInsensitiveOption);
    ctx.highlightRe = QRegularExpression(
        "\\b" + QRegularExpression::escape(text.left(8)) + "\\b",
        QRegularExpression::CaseInsensitiveOption);

    (void)ChatRenderer::formatMessageLine(msg, ctx);
    (void)ChatRenderer::formatMessage(msg, ctx);
    (void)ChatRenderer::linkifyTopic(text);
    (void)ChatRenderer::wrapEmojiHtml(ChatRenderer::ircToHtml(text), 12.0);

    if (typeByte & 0x80) {
        QList<Message> group;
        group << Message::make(MessageType::Join, nick, text, ts)
              << Message::make(MessageType::Nick, nick, text, ts, false, {}, nick)
              << Message::make(MessageType::Quit, nick, text, ts);
        const bool expanded = (ctxByte & 32) != 0;
        (void)ChatRenderer::formatEventGroupLine(group, ctx, QStringLiteral("g1"), expanded);
        (void)ChatRenderer::formatEventGroup(group, ctx, QStringLiteral("g1"), expanded);
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Standalone driver — compiled when NOT building as a real libFuzzer binary.
// ---------------------------------------------------------------------------
#ifndef UPLINK_LIBFUZZER

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <cstdio>

static int replayFile(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        QTextStream(stderr) << "fuzz_chatformat: cannot open " << path << "\n";
        return 0;
    }
    const QByteArray data = f.readAll();
    LLVMFuzzerTestOneInput(
        reinterpret_cast<const uint8_t *>(data.constData()),
        static_cast<size_t>(data.size()));
    return 1;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    if (argc < 2) {
        // No files given — read stdin and format it.
        QFile f;
        (void)f.open(stdin, QIODevice::ReadOnly);
        const QByteArray data = f.readAll();
        LLVMFuzzerTestOneInput(
            reinterpret_cast<const uint8_t *>(data.constData()),
            static_cast<size_t>(data.size()));
        return 0;
    }

    int parsed = 0;
    for (int i = 1; i < argc; ++i) {
        const QFileInfo info(QString::fromLocal8Bit(argv[i]));
        if (info.isDir()) {
            // Directory argument = corpus dir, matching libFuzzer semantics.
            const QDir dir(info.filePath());
            for (const QString &name : dir.entryList(QDir::Files, QDir::Name))
                parsed += replayFile(dir.filePath(name));
        } else {
            parsed += replayFile(info.filePath());
        }
    }
    QTextStream(stdout) << "fuzz_chatformat: replayed " << parsed << " input(s) — no crashes\n";
    if (parsed == 0) {
        QTextStream(stderr) << "fuzz_chatformat: no inputs replayed\n";
        return 1;
    }
    return 0;
}

#endif // UPLINK_LIBFUZZER
