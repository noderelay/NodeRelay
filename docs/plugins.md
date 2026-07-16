# Plugins

Plugins are scripts that keep running while Uplink is open and react to
what happens on IRC. Where a [user script](commands.md#user-scripts) runs
once when you type its slash command, a plugin watches everything and can
answer on its own: auto-responders, link loggers, notify rules, bots.

You don't need to be a programmer. If you can copy a file and edit a line
of text, you can run a plugin. If you can write ten lines of Python, you
can build your own.

## How it works (30 seconds)

A plugin is just a program. Uplink starts it, keeps it running, and talks
to it over two pipes:

- Uplink writes **events** to the plugin's stdin — one JSON object per line
  ("alice said hi in #linux", "bob joined #chat", ...)
- The plugin writes **actions** to stdout — one JSON object per line
  ("say pong in #linux", ...)

That's the whole interface. Anything that can read stdin and print JSON is
an Uplink plugin: Python, bash, node, a compiled Go binary, whatever you
like.

## Your first plugin in 5 minutes

Uplink ships three example plugins. They're installed to
`~/.config/uplink/plugins/` on first launch and appear in
**Preferences → Plugins**, disabled, waiting for you:

| Example | What it does | Good first look at |
|---|---|---|
| `pingpong.py` | answers "pong" when someone says `!ping` | responding to messages |
| `linklog.py` | saves every link you see to `~/uplink-links.txt` | watching without sending |
| `greeter.sh` | welcomes people joining your channel | join events, bash plugins |

Let's turn on pingpong:

1. Open **Settings → Preferences** (Ctrl+,) and pick the **Plugins** page.
2. Tick the checkbox next to **pingpong**. That's it — it's running now.
3. Type `/plugins` in any channel. You should see
   `pingpong — running (pid 12345)`.
4. Ask a friend (or your other client) to say `!ping` in a channel you're
   in. The plugin answers `pong` as you.

To stop it, untick the box. Plugins you enable start automatically with
Uplink from then on.

> **Note:** the Python examples need `python3` installed, which Linux,
> macOS and FreeBSD all have. `greeter.sh` also needs `jq`.

## Writing your own

Copy an example and start from there — `pingpong.py` is commented
line-by-line for exactly this purpose. The core of every plugin looks the
same:

```python
#!/usr/bin/env python3
import json, sys

for line in sys.stdin:                      # each line is one event
    event = json.loads(line)

    if event.get("event") != "message":     # only chat messages
        continue
    if event.get("self"):                   # NEVER react to yourself (see below)
        continue

    if "good bot" in event["text"].lower():
        print(json.dumps({
            "action": "say",
            "server": event["server"],
            "buffer": event["buffer"],
            "text":   "thank you!",
        }), flush=True)                     # flush=True matters!
```

Save it, `chmod +x myplugin.py`, then **Preferences → Plugins → Add
Plugin**, give it a name, browse to the file, tick the box. Done.

Two rules every plugin must follow:

- **Skip `self` events.** Your plugin's own messages come back as events
  with `"self": true`. React to them and your plugin talks to itself
  forever. Every example starts with this check — keep it.
- **Flush stdout.** Languages buffer output by default. In Python use
  `print(..., flush=True)`; in other languages flush after each line.
  A plugin that "does nothing" is almost always a plugin that isn't
  flushing.

## Test without Uplink

Because the interface is just stdin/stdout, you can test a plugin in a
terminal — no IRC needed:

```bash
echo '{"event":"message","server":"s","buffer":"#c","nick":"alice","text":"!ping","kind":"message","self":false}' | ./pingpong.py
```

You should see the action it would send:

```
{"action": "say", "server": "s", "buffer": "#c", "text": "pong"}
```

If it works in the terminal, it works in Uplink.

## Events reference

Every event line is one JSON object with an `"event"` field. Fields your
plugin doesn't care about are safe to ignore, and new fields may be added
over time — read what you need, skip the rest.

### `hello`
The first line every plugin receives:
```json
{"event":"hello","version":"2026.7.6"}
```

### `message` — someone said something
```json
{"event":"message","server":"LiberaChat","buffer":"#uplinkirc",
 "nick":"alice","text":"hi joe","kind":"message",
 "time":"2026-07-16T19:00:00Z","msgid":"abc123",
 "self":false,"mentions_you":true,"account":"alice"}
```
| Field | Meaning |
|---|---|
| `server` | which network (the name from your config sidebar) |
| `buffer` | channel (`#linux`) or nick for private messages |
| `nick` | who said it |
| `text` | what they said |
| `kind` | `message`, `action` (a `/me`), or `notice` |
| `time` | UTC timestamp, ISO 8601 |
| `self` | `true` when *you* (or your plugin) said it — always check this |
| `mentions_you` | `true` when your nick appears in the text |
| `msgid` | server message id, when the network provides one |
| `account` | sender's services account, when known — useful for "only obey my friends" checks |

### `join`, `part`, `quit`, `kick` — people coming and going
```json
{"event":"join","server":"LiberaChat","buffer":"#uplinkirc",
 "nick":"bob","text":"bob has joined the channel",
 "time":"2026-07-16T19:00:00Z","self":false}
```
`text` is the human-readable line as shown in the channel (it includes
part/quit reasons and who did the kicking).

### `nick` — someone renamed
```json
{"event":"nick","server":"LiberaChat","buffer":"#uplinkirc",
 "nick":"oldname","old":"oldname","new":"newname",
 "text":"oldname is now known as newname","self":false}
```

### `topic` — channel topic changed
```json
{"event":"topic","server":"LiberaChat","buffer":"#uplinkirc","text":"the new topic"}
```

### `connected` / `disconnected` — server link state
```json
{"event":"connected","server":"LiberaChat"}
```

Things plugins deliberately never receive: replayed history (your bot
must not answer a week-old `!ping` when chat history loads), redacted
messages, and server/numeric noise.

## Actions reference

Write one JSON object per line to stdout. `server` and `buffer` say where;
they'll usually just echo back what the triggering event carried.

### `say` — send a message
```json
{"action":"say","server":"LiberaChat","buffer":"#uplinkirc","text":"pong"}
```

### `me` — send a /me action
```json
{"action":"me","server":"LiberaChat","buffer":"#uplinkirc","text":"waves"}
```

### `print` — show a line only you can see
```json
{"action":"print","server":"LiberaChat","buffer":"#uplinkirc","text":"saved 3 links"}
```
Shows `[pluginname] saved 3 links` locally in that buffer. Nothing is sent
to the network — perfect for status output.

### `command` — run any slash command
```json
{"action":"command","server":"LiberaChat","buffer":"#uplinkirc","line":"/join #backup"}
```
Anything you could type works: `/join`, `/mode`, `/topic`, `/raw`, ...
The `buffer` provides the "current channel" context commands act on.

## Lifecycle, limits, and debugging

- **Start/stop**: enabled plugins start with Uplink and stop when it quits.
  Toggling the Preferences checkbox starts/stops them immediately.
- **Crashes**: a crashed plugin is restarted with increasing delays. After
  3 restarts Uplink gives up, tells you, and marks it `failed` — fix the
  script, then toggle it off and on.
- **Flood guard**: a plugin may send at most 10 actions per 5 seconds.
  Beyond that, actions are dropped and you're warned once. Bots should be
  polite anyway — add cooldowns like `pingpong.py` does.
- **`/plugins`** lists every configured plugin and whether it's running.
- **stderr is your debug channel.** Anything the plugin writes to stderr
  lands in Uplink's debug log. To watch it live, start Uplink from a
  terminal like this:

```bash
QT_LOGGING_RULES="uplink.plugin.debug=true" ./Uplink
```

## Troubleshooting

| Symptom | Likely cause |
|---|---|
| Plugin shows `stopped` right after enabling | Script crashed on startup — run it in a terminal and look at the error |
| Plugin shows `failed` | It crashed repeatedly. Test standalone (see above), fix, re-toggle |
| Plugin runs but never answers | Missing `flush` on stdout (the #1 cause), or your event filter never matches — test standalone |
| `file not found` notice | The path in Preferences → Plugins is wrong, or the file moved |
| Python plugin does nothing on Windows | Make sure `.py` files are associated with Python, or point the plugin entry at a `.bat` wrapper |
| Plugin replies to itself in a loop | You removed the `self` check. Put it back |

## Cookbook

Ideas to copy and adapt (all trivial edits of `pingpong.py`):

- **Notify rule**: on `mentions_you`, `print` to a special buffer — or use
  `command` with `/raw` tricks for anything fancier.
- **Seen tracker**: remember the last `time` you saw each `nick`; answer
  `!seen somenick`.
- **Auto-away answerer**: reply to private messages (`buffer` == the
  sender's nick, no `#`) with "afk, back soon" — with a long cooldown per
  person so you don't spam anyone.
- **Karma bot**: count `word++` occurrences into a JSON file; answer
  `!karma word`.

Share what you build in [#uplinkirc on Libera.Chat](https://web.libera.chat/#uplinkirc).
