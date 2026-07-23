# IRCv3 Support

Uplink aims to be a first-class IRCv3 client. Below is the full status of every negotiated capability, including bouncer-specific extensions.

---

## How capability negotiation works

Uplink uses **CAP LS 302**: it requests the server's full capability list before deciding what to ask for, and only requests capabilities the server actually advertises. This avoids failures from batching unsupported caps together. Multi-line CAP LS responses (marked with `*`) are fully buffered before any `CAP REQ` is sent, and capabilities advertised in `name=value` form (e.g. `sasl=PLAIN,EXTERNAL`) are matched correctly. After the server acknowledges the requested caps (CAP ACK), Uplink completes registration or starts SASL if needed.

---

## Active capabilities

These capabilities are fully negotiated and actively used.

### `multi-prefix`

Returns all mode prefixes for a nick in NAMES replies: for example, `@+alice` instead of just `@alice`. The nick list correctly displays users who hold more than one status simultaneously (e.g. both op and voice).

### `away-notify`

When a user in a shared channel sets or clears away status, the server sends a real-time AWAY message. Uplink receives and tracks these notifications.

### `server-time`

When the server attaches a `time` tag to a message, Uplink uses that timestamp instead of the local clock. This is essential for bouncer history replay: messages show the time they were originally sent, not the time the client received them. Works across all message types (PRIVMSG, NOTICE, JOIN, etc.).

### `message-tags`

The full IRCv3 message tag extension is negotiated. Tags are parsed as a key-value map with proper IRCv3 escape unescaping. This is the transport layer for `server-time`, `typing`, `batch`, and all other tag-based features.

### `batch`

`BATCH` commands are fully handled. When a batch starts (`BATCH +ref type param`), Uplink buffers all messages tagged with `batch=ref` instead of processing them immediately. When the batch ends (`BATCH -ref`), the buffered messages are delivered as a unit with appropriate handling based on the batch type:

- `chathistory` / `draft/chathistory`: messages are delivered as history (dimmed, original timestamps, no unread count)
- `znc.in/batch/playback`: same treatment as chathistory
- `netsplit`: all QUITs in the batch are collapsed into one summary line per affected channel (see [netsplit/netjoin](#netsplit--netjoin-batch-types))
- `netjoin`: all JOINs in the batch are collapsed into one summary line per affected channel (see [netsplit/netjoin](#netsplit--netjoin-batch-types))

Safety limits apply: at most **8 batches** may be open simultaneously, and each batch may contain at most **1 000 messages**. A misbehaving or malicious server that opens a batch and never closes it cannot grow client RAM without bound.

### `chathistory`

When `chathistory` (or `draft/chathistory`) is negotiated, Uplink sends `CHATHISTORY LATEST <channel> * 100` after joining each channel, requesting up to 100 recent messages. The server (or bouncer) responds with a `BATCH` of the matching type.

Uplink requests both the ratified `chathistory` cap name and the draft name `draft/chathistory` used by Ergo IRCd, so history auto-loads on Ergo servers without any extra configuration.

History messages are visually distinct from live messages:
- Displayed at reduced opacity
- Timestamped with their original send time; previous-day messages show `MM/dd hh:mm` so you always know when they were sent
- Not counted as unread, so they do not badge the channel in the sidebar

When the user scrolls to the top of a channel buffer and all loaded messages have been displayed, Uplink sends `CHATHISTORY BEFORE <channel> timestamp=<oldest> 100` to fetch the next batch of older messages. These are prepended to the buffer without jumping the scroll position. Uplink stops requesting when the server returns an empty batch. The bound is a timestamp rather than a msgid because some implementations (soju) reject msgid bounds; timestamps are accepted everywhere.

This works on any server or bouncer that supports either cap name, including Ergo, soju, and modern ZNC.

On connections without chathistory support, Uplink's own log files stand in (when message logging is enabled in Preferences): joining a channel seeds the buffer with the last 100 logged messages, mirroring `CHATHISTORY LATEST`, and scrolling to the top pages further back, so scrollback still reaches as far as your local logs go. Seeding is skipped when the server or a ZNC playback module will replay the backlog itself, so the two sources never duplicate.

### `sasl` (PLAIN and EXTERNAL)

Authenticates your account during the CAP handshake, before you appear on the network. Uplink holds `CAP END` until the server confirms success (`903`) or failure (`904`/`906`). On failure the connection continues without authentication.

**PLAIN**: Add `sasl_user` and `sasl_password` to a server block. Uplink sends `AUTHENTICATE PLAIN` followed by the base64-encoded `\0user\0password` payload.

**EXTERNAL**: Add `sasl_external = true`, `client_cert`, and `client_key` to a server block. Uplink loads the PEM certificate and key, presents the certificate during the TLS handshake, then sends `AUTHENTICATE EXTERNAL` followed by an empty `+` response. The server derives your identity from the certificate's fingerprint; no password is transmitted. RSA and EC (ECDSA) PEM keys are both supported.

```
Client → AUTHENTICATE EXTERNAL
Server → AUTHENTICATE +
Client → AUTHENTICATE +
Server → 903 :SASL authentication successful
```

### `typing` / `draft/typing`

Uplink requests both the ratified `typing` cap name and the older `draft/typing` used by most servers today, so typing indicators work wherever either name is offered.

When you start typing in the input box, Uplink immediately sends `TAGMSG` with `+typing=active`. It restarts a 5-second inactivity timer on each keypress; if you stop typing for 5 seconds, it sends `+typing=paused`. When you send or clear the input (or switch to another buffer with text still in the box), it sends `+typing=done` to the channel you were typing in.

Incoming typing notifications from other users appear as "nick is typing…" above the input bar and time out automatically. The feature can be toggled from the **Preferences** dialog (**Settings → Preferences**, or **Ctrl+,**).

### `labeled-response`

CAP is negotiated. Labels tie server responses to outgoing commands.

### `echo-message`

With this cap the server echoes your own `PRIVMSG` and `TAGMSG` back to you. Uplink renders your outgoing messages from that echo instead of optimistically: your line appears in the buffer with the server's timestamp and, most importantly, its `msgid`, which is what makes reactions, replies, and redaction work on your own messages. Without the cap, Uplink falls back to echoing locally the moment you hit Enter. Self-echoes are recognized and never re-trigger typing indicators or DCC handling.

### `msgid`

Assigns a globally unique ID to every message. This is a message tag that arrives via `message-tags` rather than a capability Uplink requests by name. Parsed from the `msgid` tag on all incoming `PRIVMSG`, `NOTICE`, and `ACTION` messages, stored on the `Message` struct, and carried through batch delivery. Used as the anchor for `draft/reply`, `draft/react`, and future redaction (`draft/message-redaction`).

### `draft/reply`

A client-only tag (`+draft/reply=<msgid>`) that marks a message as a reply to a specific message by its ID. Uplink supports both sending and receiving:

- **Sending:** right-click any message timestamp in the chat, choose **Reply**, then type and press Enter. The outgoing `PRIVMSG` carries `@+draft/reply=<original-msgid>`.
- **Receiving:** incoming messages with a `+draft/reply` tag display a small **↩ origNick** indicator before the sender nick, showing whose message was being replied to.

Requires `message-tags` (already negotiated). Compatible with any modern IRC server or bouncer; clients that do not support the tag see the message as normal.

### `chghost`

When a user's vhost or ident changes, the server sends a `CHGHOST newuser newhost` command rather than a fake QUIT+JOIN pair. Uplink negotiates this capability and shows a single quiet status line, `nick changed host (newuser@newhost)`, in each channel the user shares, instead of the noisy disconnect/reconnect flood.

### `draft/react`

Emoji reactions attached to specific messages via `TAGMSG` with a `+draft/react` tag and a `+draft/reply` tag identifying the target message ID.

- **Sending:** right-click any message **timestamp** → **React** → emoji picker opens; search by name (`:thumbs`, `fire`, etc.) and click to send. Or use `/react <emoji>` after setting a reply target.
- **Receiving:** reactions are stored per-message and rendered inline below the original message as `emoji(count)` in the chat view.
- Requires `message-tags` and server support for `draft/react`. On servers that do not advertise the capability, reactions are not sent.

### `draft/message-redaction`

Removes a sent message from history and signals other clients to hide it. Requires `msgid` to identify the target message.

- **Sending:** right-click a **timestamp** on your own message → **Delete**. Only available when the server has acknowledged the `draft/message-redaction` CAP.
- **Receiving:** redacted messages are replaced with `[message deleted]` in grey italic. The message ID anchor is preserved so reply threading is not broken.

**Server requirements (Ergo):** three settings must be present in `ircd.yaml`:

```yaml
datastore:
  sqlite:
    enabled: true
    database-path: /var/db/ergo/history.db
    busy-timeout: 5s
    max-conns: 1

history:
  enabled: true
  persistent:
    enabled: true
    registered-channels: opt-out
    direct-messages: opt-out
  retention:
    allow-individual-delete: true  # must be under retention:, not directly under history:
```

The default Ergo package binary may not include SQLite support. If not, rebuild from source: `git clone https://github.com/ergochat/ergo && cd ergo && make build_full`.

### `account-notify`

Notifies the client in real time when a user logs in or out of services (NickServ). When a nick's account changes, Uplink updates the account field stored for that nick across all shared channels. The account name is shown as a tooltip when you hover over a nick in the nick list.

### `extended-join`

When this capability is active, `JOIN` messages include the joining user's account name and real name as additional parameters. Uplink uses this to populate the account field for the nick immediately on join, without needing a WHOIS or a subsequent `account-notify`.

### `invite-notify`

When you are a member of a channel, the server notifies you any time someone is invited to that channel (not just when you personally are invited). Invite-from-outside messages post a status line to the server buffer; broadcast invites from inside a shared channel post to the channel buffer.

### `setname`

Allows users to update their real name (GECOS) field after connecting. When another user in a shared channel changes their real name, Uplink posts a status line in every channel you share with them: `nick changed their realname to "…"`.

You can change your own realname with `/setname <new realname>`; no reconnect needed.

### `userhost-in-names`

When active, `NAMES` replies include the full `user@host` for each nick (e.g. `@alice!alice@example.com`). Uplink strips the `!user@host` suffix before displaying nicks, so the nick list looks the same, but the underlying data is richer and ready for future ban/ignore improvements.

### `Monitor`

A standardized server command for watching when specific nicks come online or go offline. Uplink sends `MONITOR + nick1,nick2,…` after the welcome message whenever a monitor list is configured. Status changes post to the server buffer as **Now online: nick** / **Now offline: nick**.

Manage the watch list with `/monitor`:

| Subcommand | Description |
|---|---|
| `/monitor add <nick>` | Add nick to the watch list (persisted to config) |
| `/monitor del <nick>` | Remove nick from the watch list |
| `/monitor list` | Show the current watch list |
| `/monitor clear` | Clear the entire watch list |
| `/monitor status` | Ask the server for current online/offline status of all watched nicks |

The list is stored in `config.toml` under `[monitor] nicks = [...]`.

### `WHOX`

An extended version of the `WHO` command. Uplink checks the server's `ISUPPORT` tokens on connect: if `WHOX` is advertised, it sends `WHO <channel> %cnfa,42` after joining each channel, fetching channel, nick, flags, and account in a single query. The server reply (`354 RPL_WHOSPCRPL`) is handled by the same bot-detection logic as the regular `WHO` reply, and any account name returned fires an `accountChanged` update to populate nick list tooltips. On servers that do not advertise `WHOX` (e.g. Rizon), Uplink falls back to a plain `WHO <channel>` so no "Unknown command" error is produced.

### `sts` (Strict Transport Security)

When a server advertises an STS policy in its `CAP LS` response, Uplink upgrades the connection automatically and caches the policy to disk (`~/.config/uplink/sts.ini`). On future connections to that host, TLS is enforced even if `ssl = false` is set in the config; the stored policy takes precedence and the TLS port from the policy is used.

- **Plain connection:** Uplink receives `sts=port=6697,duration=2678400`, immediately disconnects, and reconnects over TLS on the specified port. The policy is saved with an expiry timestamp based on the `duration` value (in seconds).
- **TLS connection:** Uplink receives `sts=duration=2678400` (no port, already on TLS), refreshes the stored policy expiry.
- **Policy removal:** If the server sends `sts=duration=0` on a TLS connection, the cached policy is deleted.

Policies survive restarts and are applied before each connection attempt. This prevents downgrade attacks where an attacker could intercept a plaintext connection before TLS is established, equivalent to HSTS in web browsers.

### `account-tag`

When negotiated, the server attaches an `account` tag to every message sent by an authenticated user. Uplink uses this to keep per-nick account information current on a per-message basis, complementing `account-notify` (login/logout notifications) and `extended-join` (account on join).

- The account name is stored with each message and shown as a **tooltip when you hover over the sender's nick in the chat view**; the same tooltip as in the nick list.
- The nick list account data is also kept current, so both tooltips always show the same value.
- If a user is not authenticated with services, no `account` tag is sent and no tooltip appears.

### `netsplit` / `netjoin` batch types

When a netsplit occurs, the server wraps the resulting flood of QUITs in a `BATCH` of type `netsplit`. When the servers reconnect, the resulting flood of JOINs arrives in a `BATCH` of type `netjoin`.

Uplink collapses both into a single status line per affected channel instead of flooding the buffer. Nick lists remain accurate: nicks are removed and re-added correctly.

**Example output:**

```
-- Netsplit: 47 users lost (irc1.example.com irc2.example.com)
-- Netjoin: 47 users returned (irc1.example.com irc2.example.com)
```

Without this, a busy network split would print one quit line per user, often hundreds, making the channel unreadable.

### `standard-replies`

A structured format for servers to send machine-readable diagnostics. Uplink requests the cap, and also parses the three commands from servers that send them without advertising it:

| Command | Meaning | Uplink display |
|---|---|---|
| `FAIL` | A command the client sent failed | Red error line in the relevant buffer |
| `WARN` | A non-fatal warning | Server info line |
| `NOTE` | Informational server message | Server info line |

Format: `FAIL <triggered-by-command> <code> [context...] :<description>`

Uplink routes each reply to the most relevant buffer:
- If a channel name appears in the context parameters, the message goes to that channel's buffer.
- Otherwise it goes to the active channel, or the server buffer if no channel is active.

**Example output:**

```
[FAIL] JOIN CHANNEL_BANNED: You are banned from that channel
[WARN] PRIVMSG RATE_LIMITED: You are sending messages too quickly
[NOTE] * SESSION_EXPIRY: Your session will expire in 5 minutes
```

The `[FAIL]` / `[WARN]` / `[NOTE]` prefix lets you tell them apart at a glance.

### `UTF8ONLY`

An ISUPPORT token (not a CAP) that signals the server only accepts UTF-8 encoded traffic. Uplink detects it in the `005 RPL_ISUPPORT` parameters, sets an internal flag, and posts a status line to the server buffer: `UTF8ONLY: server enforces UTF-8 encoding`. Non-UTF-8 input is not blocked client-side, but the server will reject it.

### `draft/multiline`

Allows composing and sending messages that span multiple lines as a single logical unit, delivered using a `BATCH` of type `draft/multiline`.

**Composing:** Press **Shift+Enter** in the message input to insert a line break. The input box grows up to 4 lines automatically. Press **Enter** to send.

**Protocol flow:** When the server has acknowledged `draft/multiline`, Uplink opens a batch with `BATCH +ref draft/multiline <target>`, sends each line as `@batch=ref PRIVMSG <target> :<line>` (with the `draft/multiline-concat` tag on lines that should be joined without a newline), then closes it with `BATCH -ref`.

**Receiving:** Incoming `draft/multiline` batches are assembled into a single message with embedded newlines and rendered with `<br>` breaks in the chat view.

**Fallback:** On servers that do not advertise `draft/multiline`, each line is sent as a separate `PRIVMSG`. Recipients see the same lines in sequence; no content is lost.

---

## Bouncer capabilities

These capabilities are negotiated only when `bouncer = "znc"` or `bouncer = "soju"` is set in the server block. They are not requested against regular IRC servers.

### `znc.in/playback` (ZNC only)

When available, Uplink sends `PRIVMSG *playback :PLAY * 0` immediately after the welcome message. This tells ZNC's playback module to replay all buffered messages since last seen. Replayed messages arrive with `server-time` tags and are rendered with their original timestamps.

### `znc.in/self-message` (ZNC only)

When you send a message from another client connected to the same ZNC, this capability causes ZNC to echo that message to Uplink. Echoed messages are displayed correctly as your own outgoing messages rather than appearing as incoming messages from yourself.

### `znc.in/batch` (ZNC only)

Enables ZNC's batch extension for grouping related messages. Used in conjunction with `znc.in/playback` to deliver replayed history as a batch.

### `soju.im/bouncer-networks` (soju only)

After CAP negotiation, Uplink sends `BOUNCER LISTNETWORKS`. soju responds with a `BOUNCER NETWORK` line for each attached network, showing the network ID, name, and connection state. Each network entry is listed in the server buffer so you can see which networks your soju instance is managing.

### `soju.im/bouncer-networks-notify` (soju only)

Negotiated to receive real-time notifications when a network's connection state changes (e.g. a network goes offline or reconnects). Used to keep the network list current without polling.

### `soju.im/read` (soju only)

Synchronizes your read position across all clients connected to the same soju instance. When you read messages in Uplink, it sends `MARKREAD <target> timestamp=<iso8601>` to record your position. When another client advances the read marker, soju forwards the updated marker to Uplink.

### `soju.im/no-implicit-names` (soju only)

Tells soju not to send a NAMES list automatically on JOIN. This prevents duplicate nick list entries when soju is already managing channel state.

### `draft/metadata-2`

Associates key-value metadata with users: display names and avatar URLs stored server-side and synced to clients in real time.

Uplink subscribes to `display-name` and `avatar` changes at registration (`METADATA * SUB`), and fetches a user's keys on demand the first time you hover their nick; a channel-wide fetch on join would flood rate-limited servers. When a subscribed user changes a key, the server pushes a `METADATA` notification and Uplink updates immediately. Data is stored per-nick, the avatar image is fetched in the background, and everything shows in the **nick list tooltip**:

```
[avatar image]  Name: Alice Smith
                Account: alice
```

Avatar images are fetched asynchronously when metadata arrives and cached for the session. Images are displayed at native size, capped at 32×32 with no upscaling, so small icons like favicons remain crisp.

#### Setting your own profile

**Via Preferences:** Open **Preferences** (click the **⚙ gear icon** in the channel header) and scroll to the **Profile** section. Enter your Display Name and Avatar URL (or click **Browse...** to pick a local image file), then click **Apply to connected servers**. The values are saved to your config and re-sent automatically every time you connect.

**Via commands:** Type in any channel or PM buffer:

```
/displayname Alice Smith
/avatar https://example.com/avatar.png
/avatar /home/alice/avatar.png
```

Leave the argument blank to clear the value:

```
/displayname
/avatar
```

Both commands print a confirmation line in the current buffer and tell you if the server does not support the capability.

#### Local file avatars

The `avatar_url` config key and the Preferences Avatar URL field accept local file paths (e.g. `/home/alice/avatar.png`). Local images are loaded from disk and displayed in your own tooltips. They are **not** sent to the server; only `http://`/`https://` URLs are broadcast via `METADATA SET`, so other users will only see your avatar if you use a web-accessible URL.

#### Config

```toml
[profile]
display_name = "Alice Smith"
avatar_url = "https://example.com/avatar.png"
```

No additional configuration is required; metadata is received, fetched, and displayed automatically whenever the server supports `draft/metadata-2`.

#### Fetch semantics and bouncers

The on-demand fetch is retried sensibly: a lookup that fails because the user just went offline no longer blocks that nick for the rest of the session, and a user rejoining clears their cached metadata so the next hover fetches fresh values (the server only pushes changes while both sides are online). The first hover on a nick requests the data; the tooltip fills in on the next hover.

**Bouncers:** neither soju nor ZNC passes `draft/metadata-2` through to clients, so metadata works on direct connections only; behind a bouncer Uplink never sees the capability and skips metadata entirely. Use `/caps` in a server buffer to check what your bouncer forwards.

### WebSocket transport

IRC lines carried over a WebSocket connection (`ws://` or `wss://`) instead of a raw TCP socket. Useful for connecting to servers hosted behind web infrastructure, or in environments where raw TCP is blocked.

Enable per server with `websocket = true` in the server block (see [configuration](configuration.md)). The `ssl` key controls whether the connection uses `wss://` (TLS, recommended) or `ws://` (plain). All standard features (SASL, IRCv3 CAP negotiation, STS, SOCKS5 proxy, reconnect backoff, and ping watchdog) work identically over WebSocket.

```toml
[[server]]
name = "The Lounge"
host = "lounge.example.com"
port = 9000
ssl = true
websocket = true
nick = "yournick"
user = "uplink"
realname = "Uplink User"
```

### `cap-notify`

Keeps Uplink in sync if the server's capability set changes while you're already connected. Without this, Uplink only knows what the server can do at login time and never learns about changes.

When `cap-notify` is active, the server sends:
- `CAP NEW <caps>`: a capability was added (e.g. a server module was loaded). Uplink checks whether any of the new caps are on its wanted list and sends a `CAP REQ` for them.
- `CAP DEL <caps>`: a capability was removed. Uplink drops it from its active set so it stops trying to use it.

No configuration required. Negotiated automatically when the server supports it.

---

## Reference

Full IRCv3 specification and capability registry: [https://ircv3.net/](https://ircv3.net/)
