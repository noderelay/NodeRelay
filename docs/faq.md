# FAQ & Troubleshooting

Common questions and fixes for Uplink.

---

## Installation & startup

### Where do I get Uplink?

Pre-built binaries are available on the [GitHub Releases page](https://github.com/noderelay/UplinkIRC/releases/latest):

| Platform | Format | How to run |
|---|---|---|
| **Linux x86_64** | AppImage | `chmod +x Uplink-*.AppImage && ./Uplink-*.AppImage` |
| **Arch Linux** | AUR | `yay -S uplink-irc` (or `uplink-irc-bin` prebuilt, `uplink-irc-git` dev) |
| **Linux x86_64** | tar.gz | Extract, then `./Uplink` |
| **Windows x64** | zip | Extract and run `Uplink.exe` |
| **macOS (Apple Silicon)** | DMG | Open and drag to Applications, or `brew tap noderelay/uplink && brew trust noderelay/uplink && brew install --cask uplink` |
| **FreeBSD** | - | Build from source (see below) |

> **macOS note:** The DMG is built for Apple Silicon (arm64) and does not run on Intel Macs (Rosetta only translates in the other direction: Intel apps on Apple Silicon). Intel users: build from source. See [macOS says the app is damaged or can't be opened](#macos-says-the-app-is-damaged-or-cant-be-opened) if Gatekeeper blocks it.

The AppImage is the recommended Linux download. It is self-contained, runs on any modern x86_64 Linux with glibc 2.35+, and supports in-place updates (see [How do I update the AppImage?](#how-do-i-update-the-appimage) below).

### How do I update the AppImage?

The AppImage embeds zsync metadata pointing to the latest release. Install [`appimageupdatetool`](https://github.com/AppImageCommunity/AppImageUpdate) and run:

```bash
appimageupdatetool ./Uplink-*.AppImage
```

This downloads only the changed blocks from the new release, much faster than a full re-download. The tool prints progress and replaces the file in place when done.

To install `appimageupdatetool` on Arch Linux:

```bash
yay -S appimageupdatetool-bin
```

Or download the AppImage directly from the project's releases page.

### A nick dialog appeared on first launch: what do I do?

Uplink detected that your config still has the placeholder nick `yournick`. Type your desired nickname in the dialog and click OK. It will be saved to your config and used for all servers on the next connect.

### themes/ folder missing: no themes load

Uplink creates `~/.config/uplink/themes/` and seeds it with all bundled themes on first launch. If the folder is empty or missing, delete it and restart; it will be recreated and repopulated automatically.

Themes you have deleted will not be restored on restart. To get a specific theme back, copy it from a fresh download or from the `themes/` directory inside the release tarball.

---

## Configuration

### Where is my config file?

| Platform | Path |
|---|---|
| Linux / FreeBSD | `~/.config/uplink/config.toml` |
| macOS | `~/.config/uplink/config.toml` |
| Windows | `%USERPROFILE%\.config\uplink\config.toml` |

Open it in any text editor and restart Uplink to apply changes.

### Is my config file secure?

**Passwords are stored in your OS keychain, not in the file.** Uplink uses the OS keychain (Secret Service on Linux, macOS Keychain, Windows Credential Manager) for `password`, `sasl_password`, and `nickserv_password`. The config file stores the sentinel value `"<keychain>"` in place of the actual secret. Even if someone reads your `config.toml`, they cannot recover your passwords from it.

When you open Edit Server for a server with a stored password, the field shows a placeholder (*Stored in keychain - type to change, clear to remove*) so you know the value is there. Leaving the field empty and saving preserves the entry. All password fields also have a show/hide eye toggle on the right edge.

If Uplink shows a red **"Keychain: no password stored for…"** error in the server buffer on connect, your config has `<keychain>` as a sentinel but no matching entry in the OS keychain (the entry was never written or was cleared). Fix: open Edit Server, type your password in the affected field, and save. Uplink will store it and authenticate normally going forward.

### I renamed a server and now it says "Invalid credentials"

Passwords are stored in the OS keychain under the server's `name` field. If you rename a server entry in `config.toml` (e.g. from `znc_libera` to `soju_libera`), the keychain entry stays under the old name and the renamed server won't be able to authenticate. Fix: open **Manage Servers**, select the renamed entry, re-enter the password, and save.

### KDE keeps asking me to unlock the wallet when I change a preference

This was a bug fixed in v0.24.3. If you see a KWallet (or any OS keychain) unlock prompt when changing the font, theme, or any preference toggle, update to v0.24.3 or later.

The root cause was that every config save, including unrelated UI changes, attempted to migrate plain-text passwords to the keychain. The fix restricts keychain writes to the Manage Servers dialog, which is the only place credentials are intentionally changed.

If you are on an older build and cannot update yet, a workaround is to unlock the wallet once and keep it unlocked for your session. Alternatively, store your password as plain text in `config.toml` (no `<keychain>` sentinel); the prompt will not appear.

Beyond that:

- `config.toml` is written with **owner-only permissions** (mode `0600` on Linux/macOS) so other users on the machine cannot read it at all.
- Saves are **atomic**: Uplink writes to a temporary file and renames it into place. A crash mid-save cannot leave the config corrupt or empty.

If you ever need to verify permissions on Linux:

```bash
ls -l ~/.config/uplink/config.toml
# should show: -rw------- (600)
```

### My config change isn't taking effect

Uplink watches `config.toml` for external changes. If you edit the file in a text editor while Uplink is running, server additions and removals are picked up automatically; new servers appear in the sidebar within a second.

For other changes (theme, UI toggles, font sizes), click **File → Reload Config**; Uplink restarts immediately and picks up everything.

### TOML parse error on startup

All string values in TOML must be wrapped in double quotes. The `#` character starts a comment outside of strings, which trips up channel names.

```toml
# Wrong, causes a parse error
name = LiberaChat

# Correct
name = "LiberaChat"
```

See [Configuration](configuration.md) for the full format reference.

### Where do I set the channel to auto-join?

Add a `channels` key to the `[[server]]` block, comma-separated, all on one line:

```toml
[[server]]
name = "LiberaChat"
host = "irc.libera.chat"
port = 6697
ssl = true
nick = "yournick"
user = "uplink"
realname = "Uplink User"
channels = "#uplinkirc, #linux"
```

You can also set this from **File → Manage Servers → Edit** using the **Auto-join** field.

### How do I temporarily disable a server without removing it?

Set `disabled = true` in the server block:

```toml
[[server]]
disabled = true
name = "Libera"
host = "irc.libera.chat"
port = 6697
ssl = true
nick = "yournick"
channels = "#linux"
```

Uplink will skip the server entirely on startup (no connection, no sidebar entry) but keeps the block in the file and writes it back on every save.

You can also tick **File → Manage Servers → Edit → Disabled** and uncheck it whenever you want to reconnect.

> **Why not just comment out the block?** Commented `[[server]]` entries are not parsed by Uplink. The next time the app writes `config.toml` (on any Preferences change, `/ignore`, theme switch, etc.) it does a full rewrite from memory and the commented block is permanently gone. `disabled = true` is the safe way to pause a server.

### How do I set a custom quit message?

Add `quit_message` to the server block:

```toml
[[server]]
name = "LiberaChat"
host = "irc.libera.chat"
port = 6697
ssl = true
nick = "yournick"
quit_message = "Later!"
```

This message is sent when you disconnect or type `/quit` with no argument. It is shown to other users as `*** yournick has quit (Later!)`.

You can also set it from **File → Manage Servers → Edit → Quit Message**. To override for a single disconnect: `/quit See you tomorrow`.

If `quit_message` is unset or blank, Uplink sends `"Uplink"`.

### How do I set a default away message?

Add `away_message` to the server block:

```toml
[[server]]
name = "LiberaChat"
host = "irc.libera.chat"
port = 6697
ssl = true
nick = "yournick"
away_message = "Away from keyboard, back soon"
```

When you type `/away` with no argument, this message is sent. Users who message you while you are away receive it automatically as a server reply.

- `/away`: uses `away_message` (or `"Away"` if none is configured)
- `/away Back in 10`: overrides for this session only
- `/back`: always clears away regardless of config

Set from the GUI: **File → Manage Servers → Edit → Away Message**.

### When should I use `ssl = false`?

Almost never on a public server. All modern IRC networks support TLS; use `ssl = true` and port `6697`.

Set `ssl = false` (with port `6667`) only in these cases:

| Situation | Why plain is acceptable |
|---|---|
| Local test IRCd on `127.0.0.1` | No network exposure; traffic never leaves the machine |
| Private LAN server with no TLS cert | Traffic stays inside a trusted network |
| Bouncer listening on localhost | The bouncer handles TLS for its upstream; local socket is loopback-only |
| `.onion` IRC server via Tor | Tor provides its own end-to-end encryption through the hidden service |

```toml
# Local test server, plain connection on loopback
[[server]]
name = "Local test"
host = "127.0.0.1"
port = 6667
ssl = false
nick = "yournick"
```

> For any server on the public internet, always use `ssl = true`. Plaintext IRC sends your nick, password, and every message in the clear to anyone on the network path between you and the server.

---

## Connecting & IRC

### Can I connect to multiple servers at once?

Yes. Add multiple `[[server]]` blocks in your config; each gets its own entry in the sidebar and connects independently. Each server must have a unique `name`. You can even have two entries pointing at the same host (e.g. a direct connection and a bouncer connection) as long as the names differ.

```toml
[[server]]
name = "LiberaChat"
host = "irc.libera.chat"
port = 6697
ssl = true
nick = "yournick"
user = "uplink"
realname = "Uplink User"
channels = "#uplinkirc"

[[server]]
name = "Libera"
host = "irc.libera.chat"
port = 6697
ssl = true
nick = "yournick"
user = "uplink"
realname = "Uplink User"
channels = "#linux"
```

You can also connect to a server on the fly without editing config:

```
/server irc.libera.chat              # SSL on port 6697
/server irc.example.org:6667         # plain text on port 6667
/connect irc.oftc.net                # alias for /server
```

### Uplink disconnected: will it reconnect?

Yes. Uplink reconnects automatically after an unexpected disconnect using exponential backoff: it waits 5 seconds, then 10, 20, 40, and caps at 60 seconds per attempt. A countdown message appears in the server buffer each time. Once reconnected, it re-joins all channels you had open: both those in your config and any you joined manually during the session.

If you disconnect deliberately with `/quit`, `/disconnect`, or the **Disconnect** option in the sidebar right-click menu, no reconnect is attempted.

You can also right-click a server in the sidebar and choose **Reconnect** to connect immediately without waiting, or use `/connect` to reconnect the current server.

### Does Uplink enforce TLS automatically?

Yes, when a server supports the IRCv3 **STS (Strict Transport Security)** capability. If Uplink connects to a server over plaintext and the server advertises an STS policy, Uplink immediately disconnects and reconnects over TLS on the port the server specified. The policy is cached to `~/.config/uplink/sts.ini` and re-applied on every future connection to that host, enforcing TLS even if `ssl = false` in your config, until the policy expires.

This prevents downgrade attacks where someone on the network could intercept a plaintext connection before TLS is negotiated.

### How do I route IRC through a SOCKS5 proxy (or Tor)?

Add `proxy_host` and `proxy_port` to the server block in `config.toml`:

```toml
[[server]]
name = "ExampleNet via Tor"
host = "irc.example.org"
port = 6697
ssl = true
nick = "yournick"
proxy_host = "127.0.0.1"
proxy_port = 9050          # Tor's default SOCKS5 port
```

For an authenticated proxy, also add `proxy_user` and `proxy_pass`. You can also configure this from the GUI: **File → Manage Servers → Edit → SOCKS5 Proxy** section at the bottom of the dialog.

The proxy applies to all connection attempts including reconnects. TLS still works; the TLS handshake happens inside the SOCKS5 tunnel. Leave `proxy_host` empty to connect directly.

### How do I close a private message window?

Type `/close` or `/leave` in the PM window, or right-click the entry in the sidebar and choose **Close Query**.

In a **channel**, `/leave` and `/close` send a PART message (leaving the channel). In a **query** (PM window), they just close the local buffer; no message is sent to the server.

### How do I send a raw IRC command?

You have two options:

**Option 1: just type the command directly.** Any slash command that Uplink does not recognize is sent straight to the server as a raw IRC line. No prefix needed:

```
/REHASH
/SAMODE #uplinkirc +o alice
/GLOBOPS Heads up, opers
/OPER myname mypassword
```

**Option 2: use `/raw` or `/quote`.** Both do the same thing and accept a full IRC protocol line:

```
/raw MODE #uplinkirc +m
/raw INVITE alice #uplinkirc
/quote PRIVMSG #test :hello from raw
```

`/raw` and `/quote` are identical aliases. Use whichever you prefer. The direct passthrough (option 1) is usually simpler for oper and server-specific commands.

### My nick was already in use: what happened?

If your preferred nick is taken, Uplink appends `_` and tries again (e.g. `yournick_`). You will see the new nick reflected next to the input box. Use `/nick yournick` once the original nick becomes available.

### How do I join a channel while connected?

Type `/join #channelname` in the input box and press Enter.

### How do I send a private message?

Use `/msg <nick> <text>`. A PM buffer for that user opens in the sidebar where you can continue the conversation. Incoming PMs from others open their own buffers automatically.

You can also right-click a nick in the user list **or directly in the chat view** and choose **Message** to open the PM buffer without typing a command.

### How do I use a bouncer (ZNC or soju)?

Set the `password` field for bouncer authentication, and set `bouncer = "znc"` or `bouncer = "soju"` to enable bouncer-specific IRCv3 capabilities such as chat history replay, self-message echo, and network enumeration.

**ZNC**: password format is `username/network:password`:

```toml
[[server]]
name = "ZNC - Libera"
host = "znc.example.com"
port = 6697
ssl = true
nick = "yournick"
user = "uplink"
realname = "Uplink User"
password = "joe/libera:mysecretpassword"
bouncer = "znc"
channels = "#linux"
```

With `bouncer = "znc"`, Uplink negotiates `znc.in/playback` and replays missed messages on connect, and echoes messages you sent from other clients via `znc.in/self-message`.

**soju**: password format is `username:password`. Use `bouncer_network` if your soju instance manages more than one IRC network:

```toml
[[server]]
name = "soju - Libera"
host = "soju.example.com"
port = 6697
ssl = true
nick = "yournick"
user = "uplink"
realname = "Uplink User"
password = "joe:mysecretpassword"
bouncer = "soju"
bouncer_network = "libera"
channels = "#linux"
```

With `bouncer = "soju"`, Uplink negotiates `soju.im/bouncer-networks` (lists your networks in the server buffer), `soju.im/read` (syncs your read position across clients), and `soju.im/no-implicit-names`.

### Why don't my unread badges sync between my machines?

When two Uplinks share one identity, reading a channel on one clears its unread badge on the other within a couple of seconds. Three things have to be true:

1. **The server offers a read-marker capability** — `draft/read-marker` (Ergo and others) or `soju.im/read` (soju). Check with `/caps` in the server buffer.
2. **Both clients are logged into the same account** via SASL. Unauthenticated connections are strangers to each other.
3. **The server allows multiple connections on one account.** On Ergo that is the `multiclient` setting (`accounts.multiclient.enabled` + `allowed-by-default` in ircd.yaml, or `/msg NickServ SET MULTICLIENT ON` per account). If your second client connects as `yournick_` instead of `yournick`, this is what's missing. (Uplink automatically tries to take the configured nick back once after SASL login succeeds; if the server still refuses, it stays on the fallback and shows the usual in-use error.)

A marker only clears a badge once it covers everything in the buffer — if the other client read half the backlog, your badge stays.

In the other direction, Uplink only reports messages as read while one of its windows actually has focus. A channel sitting open in a minimized Uplink doesn't silently clear the unread state on your phone; the marks are sent when you come back to the window.

Both bouncer types also negotiate the `chathistory` capability, which automatically requests the last 100 messages for each channel on join. Uplink also supports `draft/chathistory` (used by Ergo IRCd), so history works on Ergo servers without any extra configuration. See [IRCv3 support](ircv3.md) for full details.

### What do the dimmed messages at the top of a channel mean?

Those are history messages replayed by your bouncer or IRC server via the `chathistory` (or `draft/chathistory`) capability. They represent messages that were sent while you were offline or disconnected.

History messages are intentionally displayed at reduced opacity and with their original timestamps so they are easy to distinguish from live messages. Previous-day messages show the date as well as the time (`MM/dd hh:mm`). They do not count as unread, so they will not badge the channel in the sidebar.

---

## Interface

### Can I view two channels at the same time?

Yes: **right-click** any `#channel` in the sidebar and choose **Open in Pane**. The chat area splits and that channel appears as a new column with its own chat history, nick list, topic bar, and input bar. You can type and send messages in either pane independently.

There is no fixed set of shapes. Opening a pane splits the roomiest view in half along its longer side, so panes stay near-square, and each split keeps its own direction: a column of rows beside a single view is a layout you can build. Panes stop when the space does, once halving a view would leave both halves too narrow to read, with a hard ceiling of 8 views.

Want three equal rows instead? Untick **Preferences → Interface → Split Panes Automatically** and every view flattens onto one axis.

Click the `✕` in a pane's header to close it. Closing a pane does not leave the channel; it just removes the split view. The main view has a `✕` of its own once a pane is open: closing it hands the whole window to your panes, and sidebar clicks then load into the pane you're working in rather than reopening it. It comes back when you close the last pane.

**Pane layout is saved automatically.** The arrangement, the nesting, and the divider positions are written to your preferences on quit and restored on the next launch. Proportions are stored rather than pixel widths, so the layout comes back right even at a different window size or on a different monitor.

See [Channel panes](howto.html#channel-panes) in the how-to guide for a full walkthrough.

### Does Tab completion work in channel panes?

Yes. Pressing <kbd>Tab</kbd> in any pane input bar completes nicks and slash commands exactly like the primary input. Completion candidates come from that pane's own channel, so you get the right nick list regardless of which pane has focus. The colon-suffix convention (`alice:` at the start of a line) also applies in panes.

### Why doesn't a channel open in a pane show an unread counter?

By design. A channel that's visible in a pane or popped-out window never accumulates an unread badge in the sidebar; its messages are already on screen, so counting them as unread would just leave a badge you could never clear. Once the pane or window is closed, unread counting resumes like any other background channel.

### Where is the Preferences button?

Open **Settings → Preferences** from the menu bar, or press **Ctrl+,** from anywhere; the dialog stays open while you browse themes, toggle options, and try settings.

Everything the old ☰ hamburger held now lives in the menu bar: server management and config actions under **File**, updates and documentation under **Help**. If you run with `menu_style = "hidden"`, **Ctrl+,** still opens Preferences.

### How do I change the theme?

Open **Settings → Preferences** (**Ctrl+,**), select the **Appearance** page, and click the theme button to expand the list. Use arrow keys to browse, then press **Enter** or click a theme to apply it. The list stays open so you can keep trying themes.

To set a theme in config directly:

```toml
[ui]
theme = "nord"
```

### Can Uplink follow my system dark mode?

Yes, on Qt 6.5 or newer. Open **Preferences → Appearance**, tick **Follow System Light/Dark (Auto)**, and pick a theme for each side in the **Light theme** and **Dark theme** dropdowns. When your desktop switches its color scheme (KDE Quick Settings, GNOME dark mode, a sunset schedule), Uplink recolors itself live, mid-session, with no restart.

Picking a theme from the main list turns Auto off, so a manual choice always wins. In config:

```toml
[ui]
theme_auto = true
theme_light = "light"
theme_dark = "dark"
```

### Can I change the colors of join/part/quit lines?

Yes. Event and status lines (joins, parts, quits, nick changes, topics, notices, errors, server replies) take their colors from the active theme automatically: joins use the theme's green, parts and quits its red, nick and topic changes its blue, notices its amber. Switch themes and they recolor to match; nothing to configure.

If you write your own theme and want different colors for specific line types, add an `[events]` section to the theme file; see the [theme file format](configuration.md#theme-file-format) for the seven keys. Leaving the section out entirely gives you the automatic palette-derived colors.

### How do I change the font size?

Use **Ctrl+Plus** / **Ctrl+Minus** to zoom the font for the UI region that currently has focus (chat, nick list, sidebar, channel header, or input box). You can also use **Ctrl+Scroll wheel** to zoom the region under the cursor. Each region has its own independent font size. For fine-grained control, open **Preferences → Appearance → Font Config** to set exact point sizes for each region. Font sizes are saved to your config automatically.

To set font sizes in config directly:

```toml
[ui]
font_chat = 12
font_sidebar = 10
font_input = 10
font_nick_dock = 9
```

### What happens if my configured font isn't installed?

Nothing breaks: Uplink notices the font is missing and falls back to your system's default monospace font, so text stays readable. This matters most on fresh installs where the default **IBM Plex Mono** (or **Consolas** on Windows) may not be present. To use the intended font, install it with your package manager (e.g. `ttf-ibm-plex` on Arch) and restart; your `font_family` setting is kept as-is and picks it up automatically.

### How do I hide or show the server/channel list?

Click the **close button** in the top-left corner of the sidebar panel. The sidebar collapses completely; the chat panel expands to fill the window. A **reveal button** appears at the bottom-left of the chat area; click it to bring the sidebar back.

You can also drag the divider between the sidebar and the chat area to resize it; the width is saved and restored on the next launch.

### How do I hide or show the user list?

Click the **close panel button** (▦) in the **nick panel header** (top-right corner, left of the groups icon). The list collapses and a reveal button appears in the channel header row, right of the search button, so you can bring it back with one click. Drag the splitter between the chat view and the user list to resize the panel; the width is saved and restored on the next launch.

### My user list vanished and dragging won't bring it back

Two different things can look like a missing user list:

- **You're not in a channel.** The user list only exists in channels; the server window and private messages never show one. Join a channel and check again.
- **On v2026.7.7 and earlier**, dragging the chat/user-list divider all the way to the edge could snap the panel to zero width with nothing left to grab, and the stuck state was remembered across restarts. **v2026.7.8** brings a stuck list back automatically on startup, but if your config predates the fix the panel can still snap shut when you drag the divider; the old saved layout carries the bad behavior with it. One-time cleanup makes it permanent: quit Uplink, delete the line starting with `nickSplitter=` from `~/.config/uplink/uplink.conf` (Linux); on macOS run `defaults delete com.uplink.uplink` in Terminal, on Windows delete the `nickSplitter` value under `HKCU\Software\uplink\uplink` in regedit, then relaunch. Since **v2026.8.0** the config entry is retired entirely and old configs are migrated automatically, so this cleanup is only needed on older versions.

### How do I show the channel topic?

The channel header row at the top of the chat area shows `#channel (+modes)`. The connected network name appears inline in the nick panel header (`* NetworkName`). To see the actual channel topic text:

- **Global toggle:** open **Settings → Preferences** (**Ctrl+,**) → check **Show Topic Bar**. Or set in config:
  ```toml
  [ui]
  show_topic = true
  ```
- **Topic button:** every channel view has a small **speech bubble icon** in the channel header. Click it to show or hide the topic text independently of other channels. It is always visible, whether you are in single-window mode or have multiple panes open.
  - The icon is **muted** (grey) when the topic is hidden and turns **accent-colored** when the topic is showing, so you can see the state at a glance without clicking.
  - The topic opens automatically when you open a pane for a channel that has a topic set.

When the server reports who set the topic and when, the channel header shows **Topic set by nick · Xh ago** (elapsed time shown as minutes, hours, days, or weeks).

### How do I mark myself as away?

Type `/away [message]` in any input bar. For example:

```
/away grabbing coffee
/away                  # uses your configured away_message, or "Away" as a fallback
/back                  # clear away status
```

When you are away:
- A **do-not-disturb icon** appears to the right of the server name in the sidebar.
- **Your nick fades** in the user list so others can see you are away at a glance.
- Anyone who sends you a private message while you are away receives your away message as an automatic server reply.
- A status line appears in the server buffer confirming away is set, and again when you return.

Set a default away message per-server in `config.toml` so you do not have to type it each time:

```toml
[[server]]
away_message = "Away from keyboard, back soon"
```

### How do I hide my nick next to the input box?

Open **Settings → Preferences** (**Ctrl+,**) and uncheck **Show Nick in Input**. Or set it in config:

```toml
[ui]
show_nick_prefix = false
```

### How do I authenticate with NickServ automatically?

Add `nickserv_password` to your server block in `config.toml`. Uplink will send `PRIVMSG NickServ :IDENTIFY <password>` immediately after connecting:

```toml
[[server]]
name = "LiberaChat"
host = "irc.libera.chat"
port = 6697
ssl = true
nick = "yournick"
nickserv_password = "yourpassword"
```

For servers that support it, SASL PLAIN is a better choice; it identifies you before you appear on the network. See [SASL setup](configuration.md#sasl-plain-authentication) in the config docs.

### How do I use SASL to log in?

Add `sasl_user` and `sasl_password` to your server block:

```toml
[[server]]
name = "Libera"
host = "irc.libera.chat"
port = 6697
ssl = true
nick = "yournick"
sasl_user = "yournick"
sasl_password = "yourpassword"
```

Uplink negotiates the `sasl` CAP and authenticates during the connection handshake. The server buffer shows `SASL authentication successful` when it works. Use this instead of `nickserv_password` on networks that support it (Libera.Chat, OFTC, etc.).

### How do I use SASL EXTERNAL (certificate login)?

SASL EXTERNAL authenticates you via a TLS client certificate; no password is ever sent over the network.

**1. Generate a certificate** (one time):

```bash
openssl req -x509 -newkey ec -pkeyopt ec_paramgen_curve:P-384 \
    -keyout ~/.irc/client.key -out ~/.irc/client.crt \
    -days 3650 -nodes -subj "/CN=yournick"
chmod 600 ~/.irc/client.key ~/.irc/client.crt
```

**2. Register your fingerprint** (Libera.Chat):

Connect normally first, then run:

```
/msg NickServ CERT ADD
```

This tells services to trust connections presenting that certificate. After adding it, you can remove your `sasl_password` entirely.

**3. Update config**:

```toml
[[server]]
name = "Libera"
host = "irc.libera.chat"
port = 6697
ssl = true
nick = "yournick"
user = "uplink"
realname = "Uplink User"
channels = "#linux"
sasl_external = true
client_cert = "/home/joe/.irc/client.crt"
client_key = "/home/joe/.irc/client.key"
```

You can also set cert and key paths from the GUI: **File → Manage Servers → Edit** → check **Use SASL EXTERNAL** → Browse for cert and key files.

### How do I add, edit, or remove a server from the UI?

Click **File → Manage Servers...** to open the server manager. You do not need to edit `config.toml` by hand for most server settings.

**Adding a server:**

1. Click **File → Manage Servers...**
2. Click **Add**
3. Fill in the form:

| Section | Fields |
|---|---|
| **Connection** | Disabled checkbox (keep in config but skip on startup), Name (display name in sidebar), Host, Port, SSL checkbox |
| **Identity** | Nick, Username, Real Name, Quit Message, Away Message |
| **Authentication** | Server Password (for bouncers or password-protected servers), SASL User + Password (for SASL PLAIN), SASL EXTERNAL checkbox + Client Cert + Client Key (Browse buttons available), NickServ password |
| **Channels** | Auto-join, comma-separated list of channels (e.g. `#uplinkirc, #linux`). For password-protected channels, see the note in the dialog and use `config.toml` directly. |
| **Bouncer** | Type (None / ZNC / Soju) and Network name (soju only) |

4. Click **OK**; the server is added immediately and begins connecting.

> **Keyed channels:** The auto-join field does not support channel passwords. To join a password-protected channel, add it manually to `config.toml` using the `[[server.channel]]` format with a `key` field. The server dialog shows a note explaining the format.

**Editing a server:**

1. Click **File → Manage Servers...**
2. Select the server in the list
3. Click **Edit**
4. Make changes and click **OK**; the server reconnects with the new settings immediately.

**Removing a server:**

1. Click **File → Manage Servers...**
2. Select the server
3. Click **Remove**; the server disconnects and is removed from the sidebar and config.

### How do I ignore someone?

Right-click their nick in the user list or chat view and hover over **Ignore ▶**. A submenu opens with three checkboxes (**Private Messages**, **Notices**, and **Invites**); tick or untick each independently. **Channel messages are always visible**; ignore only affects private communication, so you can silence someone's PMs and invites while still seeing them talk in a channel.

To unignore individual types, untick them in the submenu. To clear everything at once, choose **Unignore All** at the bottom of the submenu.

For per-type control, use slash commands:

```
/ignore spammer              # suppress PMs, notices, and invites (default)
/ignore spammer pm           # suppress private messages only
/ignore spammer invite       # suppress invites only
/ignore spammer pm notice    # suppress PMs and private notices
/unignore spammer            # remove from the ignore list entirely
/ignored                     # list ignored nicks and their flags
```

The ignore list is saved in `config.toml` and persists across sessions:

```toml
[[ignore.entry]]
nick = "spammer"
flags = ["pm", "notice", "invite"]

[[ignore.entry]]
nick = "recruiter"
flags = ["invite"]    # invites only, their messages still visible
```

When any ignore flag is active for a nick, a **visibility-off icon** appears next to their name in the user list as a visual reminder. The icon appears and clears immediately when you toggle ignore from the right-click menu.

### How do I see the full hostmask and quit reason from a join/quit line?

Join, quit, part, nick-change, and kick events are automatically collapsed into a single compact line to reduce noise. Each compact line starts with a **▸** indicator on the left.

**Click ▸** to expand the group in-place:

```
21:03  ▾
21:03  ← CrystalDotGay (~crystal@2600:1700:...) has quit (Ping timeout: 252 seconds)
21:03  ← AJ_Z0 (~AJ@user/aj-z0) has quit (Remote host closed the connection)
21:03  → halloy2109 (~halloy210@208.59.60.21) has joined #uplinkirc
21:03  → sumeetj_ (~sumeet@103.145.17.55) has joined #uplinkirc
```

**Click ▾** to collapse back to the compact view.

The expanded view shows every event individually (full hostmask, quit/part reason, and exact timestamp per entry) regardless of how many nicks are in the group. The expanded state persists when you switch channels and return.

This is useful any time you want to check a specific user's hostmask, read a quit reason, or confirm the exact order of events during a netsplit or reconnect storm.

### What is the "── N new messages ──" divider I see in chat?

When you switch to a channel that has unread messages, Uplink inserts a separator line at the exact point where new messages begin, right before the first message that arrived while you were away. The view scrolls to show that separator at the top automatically, so you always land at the right spot without hunting for where you left off.

The separator clears as soon as you open the channel (the unread count resets to zero). On the next visit, if new messages have arrived, a fresh separator appears at the new boundary.

### Where does Uplink take me when I switch to a channel?

It depends on your state when you left:

- **You were at the bottom** (reading live chat) → opens at the latest message as normal.
- **You were reading back through history** (scrolled up) → Uplink restores your exact scroll position so you can pick up where you left off.
- **New messages arrived while you were away** → Uplink scrolls to the **unread separator** (see above), regardless of where you were scrolled. This always takes priority so you never miss the unread boundary.

### I was halfway through typing a message and switched channels. Where did it go?

Nowhere: it's waiting in the channel you were typing it in. Each channel and PM buffer keeps its own draft of unsent input text. Switch away mid-sentence, reply to something else, switch back; your half-typed message is restored with the cursor at the end.

Drafts are per-session: they are not saved to disk, and quitting Uplink discards them. Closing a channel discards its draft too.

### What's the fastest way to jump to another channel?

Press **Ctrl+K** (Cmd+K on macOS) to open the **quick switcher**: a small popup listing every buffer on every server. Start typing any part of a channel or server name to filter the list, then press **Enter** to jump to the highlighted entry (arrow keys move the highlight, **Escape** closes it). For example, typing `upl` is enough to jump to `#uplinkirc` from anywhere. **Alt+Up/Down** also steps through channels one at a time if you just want the next one.

### How do I filter the nick list to find someone in a big channel?

The nick list panel has a filter box directly above the list (below the header). Click it and start typing; the list immediately narrows to only show nicks that **start with** your input.

**Example:** type `al` to see `alice`, `albert`, `alan`, and no one else.

- The filter is case-insensitive.
- Click the **✕** in the filter box, or press **Escape** while focused on it, to clear and restore the full list.
- The filter clears automatically when you switch to a different channel.

The filter only changes what you see; nicks that don't match are temporarily hidden. They are still in the channel, still visible in tab completion, and come back the moment you clear the filter.

### How do I send bold, italic, underlined, strikethrough, or colored text?

Press the formatting shortcut in the message input box before typing the text you want formatted:

| Shortcut | Format |
|---|---|
| **Ctrl+B** | Bold |
| **Ctrl+I** | Italic |
| **Ctrl+U** | Underline |
| **Ctrl+S** | Strikethrough |
| **Ctrl+Shift+K** | Color (text + background) |
| **Ctrl+O** | Reset all formatting |

Formatting is applied **visually as you type**: bold text looks bold in the input box, italic looks italic. A small indicator (`B I U S`) appears at the bottom-left of the input showing which formats are currently active; a colored `A` joins it when a color is set. The IRC control codes are generated automatically at send time.

Shortcuts **stack**: press Ctrl+B then Ctrl+U to type bold+underlined text. Press a shortcut again to turn that one format off while keeping the others active. Press Ctrl+O to clear everything at once.

**Color** works a little differently: it opens a menu instead of toggling. Press **Ctrl+Shift+K**, or **right-click the input box** and open the **Color** submenu, then pick a text color from the 16 standard mIRC colors and, optionally, a background. Choose **Reset color** to go back to the default.

**Example (bold part of a message):**
```
Look at this [Ctrl+B]really important[Ctrl+B] thing
```
Sends as: Look at this **really important** thing

**Example (bold and underlined together):**
```
[Ctrl+B][Ctrl+U]critical warning[Ctrl+O] everything else normal
```

Recipients using any standard IRC client will see the formatting. On clients that don't render mIRC codes, the text still arrives cleanly; no visible garbage characters.

### Can I send a message that spans multiple lines?

Yes. Press **Shift+Enter** in the input box to insert a line break. The box grows up to 4 lines automatically. Press **Enter** (without Shift) to send the whole thing.

On servers that support IRCv3 `draft/multiline`, the message arrives as a single multi-line unit. On servers without it, each line is sent as a separate PRIVMSG; other users still see all the lines in order.

You can also paste multi-line text directly into the input; line breaks are preserved.

### How do I react to a message?

Right-click **anywhere on a message** (the timestamp, the nick, the message body, or any blank space on that line) and choose **React**. The emoji picker opens: search by name (`thumbs`, `fire`) or shortcode (`:poop:`), then click the emoji or press Enter to send it.

Reactions appear inline below the original message as `emoji(count)` for all clients that support `draft/react`.

You can also use `/react <emoji>` after right-clicking a message and choosing **Reply** (which sets the message target).

### How do I turn message logging on or off?

Open **Settings → Preferences** (**Ctrl+,**) and check or uncheck **Log Messages to Disk**.

When enabled, all messages are appended to log files at:
```
~/.config/uplink/logs/<server>/<channel>.log
```

Format:
```
[2026-05-31 15:04:22] <nick> message text
[2026-05-31 15:04:22] * nick action text
[2026-05-31 15:04:22] -- system message
```

History replay messages are never logged, only live messages.

You can also set it in config:
```toml
[ui]
log_messages = true    # off by default, opt-in
```

### How far back can I scroll in a channel?

As far as your history reaches. Scrolling to the top of a channel keeps loading older messages automatically, from whichever source is available:

- **On networks with server-side history** (`chathistory`: Ergo, soju, modern ZNC), Uplink asks the server for the next 100 older messages each time you hit the top, until the server runs out.
- **Everywhere else**, Uplink uses its own log files on your disk: when you join a channel (or open a PM or the server window), the last 100 logged messages load automatically, and scrolling to the top pages further back. So scrollback works even on plain networks like Libera with no bouncer. This needs **Log Messages to Disk** enabled in Preferences (see the previous answer); history accumulates from the moment you turn it on.

If Uplink hits the true end of what it has and logging is off, a status line at the top of the buffer tells you exactly that and points you at the setting. Messages loaded from history appear dimmed with their original timestamps.

### How do I message NickServ or ChanServ?

Use the service shortcuts:

```
/ns identify mypassword        # → PRIVMSG NickServ :identify mypassword
/ns register pass email@x.com  # → PRIVMSG NickServ :register …
/cs op #uplinkirc                 # → PRIVMSG ChanServ :op #uplinkirc
/bs botlist                    # → PRIVMSG BotServ :botlist
/ms list                       # → PRIVMSG MemoServ :list
```

Or use the full form: `/msg NickServ identify mypassword`

When you send a message to a service, Uplink opens a PM tab for that service (e.g. **NickServ** or **ChanServ**) in the sidebar. Replies from the service, including help output, confirmations, and error messages, arrive in that same PM tab. If no PM tab is open yet, their replies land in the server buffer.

**Passwords are never shown in the chat.** When you type `/ns identify mypassword`, the NickServ tab shows `IDENTIFY <redacted>`; your actual password is replaced before it is displayed. The command still goes to the server correctly. This applies to `IDENTIFY`, `REGISTER`, `GHOST`, `RECOVER`, `RELEASE`, `REGAIN`, and `SETPASS`.

### How do I copy text from the chat?

Select the text with your mouse (click and drag to highlight it), then right-click the selection and choose **Copy**. Standard keyboard shortcuts (`Ctrl+C` after selecting) also work. You can start a selection from anywhere in the message including the timestamp; nick and URL clicks still work as normal on a plain click (no drag).

The right-click menu for selected text also shows a **Reply** option for the message you clicked on, so you can select a quote and reply to the same message without two separate clicks.

### How do I send a file to someone (DCC)?

Right-click the recipient's nick in the user list, then choose either **Send File** or **Send File (Passive)** depending on your network situation.

**Which one should I use?**

- **Send File** (active): Uplink opens a port on your machine and tells the recipient to connect in. This is the default and works when you have a direct internet connection or have port forwarding set up on your router.

- **Send File (Passive)**: Uplink sends the offer with no port number. The recipient's client opens a port instead, and Uplink connects out to them. Use this if you are behind a home router or VPN and cannot accept incoming connections.

A simple rule: **try Send File first. If the transfer never starts, use Send File (Passive) instead.**

**Sending a file:**

1. Right-click the recipient's nick in the user list on the right.
2. Choose **Send File** or **Send File (Passive)**.
3. Pick a file in the dialog.
4. A progress dialog appears. The transfer starts as soon as they accept.

**What the recipient sees:**

```
alice wants to send you:
report.pdf  (2.1 MB)

Accept?   [Yes]  [No]
```

They click **Yes**, choose where to save it, and a progress dialog tracks the download. Either side can click **Cancel** to abort.

The file is written to a `.part` file while downloading and renamed to the final name only when the transfer completes. If the transfer fails or is cancelled, the partial file is deleted automatically. Stalled transfers, where the other side stops responding for over a minute, are aborted with an error rather than hanging forever.

Uplink enforces two receive limits before accepting a transfer:

- **Maximum file size: 2 GiB.** Files advertised as larger than this are rejected immediately.
- **Disk space check.** Available space on the destination drive is checked against the advertised file size. The transfer is rejected with an error if there is not enough room.

> **If both sides are behind NAT:** neither active nor passive DCC will work; there is no reachable port on either machine. In that case, share the file through another channel (Matrix, email, a file hosting service, etc.).

### How do I check for updates?

Open **Help → Check for Updates** from the menu bar. Uplink connects to the GitHub releases API, reads the latest version tag, and compares it to the version you have installed.

- If a newer version is available, a dialog offers to download and install it automatically. Linux AppImage users get an in-place replace and relaunch. Windows downloads the ZIP to your Downloads folder. macOS downloads and opens the DMG in Finder. Source builds see an informational message instead.
- Installed from the AUR (or another system package)? Uplink notices and points you at your package manager instead of self-updating: `yay -Syu uplink-irc` keeps it current like everything else on your system.
- If you're already on the latest version, a dialog confirms that.

The check requires an internet connection. It sends one small HTTPS request to `api.github.com` and nothing else.

### What do Uplink version numbers mean?

Uplink uses calendar versioning: **`year.month.fix`**. For example, `2026.7.0` is the first release of July 2026; if an urgent fix ships later that month, it becomes `2026.7.1`. Higher is always newer. (Versions before `2026.7.0` used an older `0.25.x` scheme; the update checker handles both.)

Official release builds show the plain version. Builds compiled from a git checkout between releases append the commit they were built from, e.g. `2026.7.8+9dbfe64`, the base version plus the short git hash, with `.dev` added if the source tree had uncommitted changes. A clean checkout sitting exactly on a release tag builds with the plain version, same as the release itself. You'll see this in the About dialog. When reporting a bug from a source build, include this full string so it's clear exactly which code you're running.

### How do I get debug logs for a bug report?

Uplink can print protocol-level debug output to the terminal, switched on at runtime with the `QT_LOGGING_RULES` environment variable; no special build needed. Three categories exist:

| Category | Covers |
|---|---|
| `uplink.irc` | connection lifecycle, every raw IRC line sent and received |
| `uplink.dcc` | DCC file transfer setup, peer connections, completion |
| `uplink.preview` | link preview fetches, timeouts, blocked addresses |

Examples:

```bash
# Just IRC traffic
QT_LOGGING_RULES="uplink.irc.debug=true" ./Uplink

# Everything
QT_LOGGING_RULES="uplink.*.debug=true" ./Uplink

# To a file
QT_LOGGING_RULES="uplink.irc.debug=true" ./Uplink 2> uplink-debug.log
```

Passwords and SASL credentials are redacted from the logged traffic, but the output still contains your conversations and the channels you're in; read it before pasting it anywhere public.

### What do the signal bars mean? (lag / latency indicator)

The four stair-step bars at the left edge of the channel header row (just before the topic bubble) show your connection latency to the active server. Hover over them to see the exact millisecond value.

| Bars | Latency | Quality |
|---|---|---|
| ████ (4 green) | < 50 ms | Excellent |
| ███ (3 green) | 50–149 ms | Good |
| ██ (2 green) | 150–299 ms | Fair |
| █ (1 green) | ≥ 300 ms | High lag |
| Blue flashing | - | Connecting / reconnecting |
| Red flashing | - | Disconnected |

Uplink sends a `PING` every 30 seconds and updates the bars automatically from the round-trip time (RTT) of the reply. When you switch to a different server or channel, the bars update immediately to reflect that server's connection.

### The server name in the sidebar turned purple: what does that mean?

The server name (e.g. **LIBERACHAT**) in the sidebar highlights purple when there are unread messages in the server window that you have not seen yet. This includes:

- **CTCP VERSION replies**: when you do `/version <nick>` or right-click a user and choose **Version**, the reply appears in the server window
- **NickServ and server auth notices** received during connection
- Other server-directed notices

Click the server name in the sidebar to open the server window and read the messages. The highlight clears as soon as you switch to it.

### How do I get alerted when someone mentions a specific word, not just my nick?

Use **highlight words**. Open **Settings → Preferences → Notifications → Highlight Words** and type a comma-separated list of words:

```
myproject, deploy, lunch
```

From then on, any message containing one of those words is treated just like a mention of your nick:

- the word is highlighted **red bold** in the chat
- the channel gets the yellow mention indicator in the sidebar
- it counts toward the mention badge if unread counts are enabled

Matching is case-insensitive and whole-word only: `deploy` matches "time to deploy" but not "deployment". Changes apply immediately; no restart needed.

You can also set it in the config file with `highlight_words = "myproject, deploy"` under the `[ui]` block; see the [configuration guide](configuration.md#the-ui-block).

### What can I do from the nick right-click menu?

Right-click any nick, in the user list or directly on a nick link in the chat view, to open the full action menu:

| Action | Description |
|---|---|
| **Message** | Open a PM buffer for that nick |
| **Send File** | Send a file via active DCC (you open the port) |
| **Send File (Passive)** | Send via passive DCC (recipient opens the port; use behind NAT) |
| **Whois** | Look up the user's info (result in server window) |
| **Invite** | Invite the nick to a channel (dialog pre-fills current channel) |
| **Give Op / Take Op** | Grant or remove `+o` (requires op) |
| **Give Voice / Take Voice** | Grant or remove `+v` (requires op or half-op) |
| **Version** | CTCP VERSION request (reply in server window) |
| **Ping** | CTCP PING; shows round-trip time in the active buffer |
| **Copy Nick** | Copy the nickname to clipboard |
| **Ignore ▶** | Opens a submenu with checkboxes for Private Messages, Notices, and Invites. Tick/untick independently. Unignore All clears everything. Persists across sessions. |
| **Kick** | Kick from current channel with optional reason (requires op) |
| **Ban** | Ban `nick!*@*` in current channel (requires op) |
| **Kick & Ban** | Ban then kick in the correct order (requires op) |

### What can I do from the message right-click menu?

Right-clicking **anywhere on a message** (timestamp, nick, body, or blank space on that line) opens a message-level context menu:

| Action | Description |
|---|---|
| **Reply** | Sets this message as the reply target. A `↩ nick` bar appears above the input. Type your reply and press Enter. Escape cancels. |
| **React** | Opens the emoji picker. Search by name or shortcode and click, or type `:shortcode:` and press Enter. Sends an IRCv3 `draft/react` shown inline below the message. |
| **Copy** | Appears when you have text selected; copies the selection to the clipboard. |

You do not have to click the timestamp exactly to get Reply. If you right-click anywhere in the message body while text is selected, the menu shows both **Copy** and **Reply** for the message you are in.

### How do I post my system info to a channel?

Type `/sysinfo` in the input box. Uplink posts your system info to the current channel immediately.

The collection runs in the background (GPU detection via `vulkaninfo` or `lspci` can take a moment). Uplink posts "Collecting system info…" immediately and replaces it with the result once ready. A 12-second timeout applies in case a subprocess hangs.

The output includes: OS name and version, CPU model, RAM, GPU name, and system uptime. Example:

```
OS: Arch Linux x86_64 | CPU: AMD Ryzen AI 9 HX PRO 370 | RAM: 15.5 GB / 23.2 GB | GPU: AMD Radeon 890M | UP: 3d 14h 22m
```

### How do I check another user's IRC client version?

Right-click the user in the nick list and choose **Version**, or type:

```
/version <nick>
```

This sends a `CTCP VERSION` request. The reply appears in the **server window** (click the server name in the sidebar to see it). The server name will turn purple to let you know a reply arrived.

### How do I ping another user to check their latency?

Right-click their nick and choose **Ping**. Uplink sends a CTCP PING with a millisecond timestamp. When the reply arrives, the round-trip time is printed in the active buffer:

```
Ping reply from alice: 42ms
```

You can also use the slash command:

```
/ping alice
```

### How do I kick or ban someone?

Right-click their nick and choose **Kick**, **Ban**, or **Kick & Ban**. You need channel op (`@`) for these to work.

- **Kick**: prompts for an optional reason, then sends `KICK #channel nick :reason`
- **Ban**: sets `MODE #channel +b nick!*@*` (bans the nick from the channel regardless of hostname or ident)
- **Kick & Ban**: applies the ban first, then kicks (correct order so the user cannot rejoin before the ban lands)

You can also use slash commands:

```
/kick baduser flooding
/ban baduser!*@*
/mode #uplinkirc +b baduser!*@*
```

### How do I invite someone to a channel?

Right-click their nick and choose **Invite**. A dialog opens pre-filled with the current channel; click OK to send the invite, or type a different channel name first.

You can also use:

```
/invite alice
/invite alice #linux
```

### How do I open the server window?

Click the server name (e.g. **LIBERACHAT**) at the top of the sidebar. It shows connection messages, server notices, CTCP replies, and other server-level output. If the server name is highlighted purple, there are unread messages waiting.

Messages in the server window are color-coded by type:

| Color | Type | Examples |
|---|---|---|
| Gray | Status | Connected, Disconnected, Joined #channel |
| Steel blue | Command reply | `/whois`, `/stats`, `/time`, `/motd` responses |
| Amber | WALLOPS | Operator broadcasts sent with `WALLOPS` |
| Red | Error | Nick in use, banned, authentication failed |
| Amber `−nick−` | Notice | Server notices and NickServ/ChanServ messages |

### How do I search the in-app documentation?

Click **Help → Documentation** to open the help viewer. A search field sits at the top right of the tab bar, just to the right of the **Shortcuts** tab. Type any word or phrase; the active tab jumps to the first match immediately. Click **×** to clear. Switching tabs with a query active re-runs the search in the new tab.

### How do link previews work?

Link previews are **disabled by default**. To enable them, open **Settings → Preferences** (**Ctrl+,**) and check **Link Previews**, or add this to `config.toml`:

```toml
[privacy]
link_previews = true
```

When enabled, any live message containing an `http://` or `https://` URL causes Uplink to fetch the link in the background and append a preview card below the message.

**Card layout**: the card appears directly below the message that contained the URL, aligned with the message body text. It shows the page title (bold, single line) and domain name below it, with the thumbnail image stacked beneath. The card background matches your current theme.

**Web pages**: Uplink fetches up to 32 KB of HTML using a messaging-app user-agent (the same trick used by Halloy and WhatsApp). This causes sites like YouTube to serve a compact metadata page with `og:title` and `og:image` tags early in the response, rather than a full JavaScript-heavy document.

**Direct image links**: URLs ending in `.png`, `.jpg`, `.jpeg`, `.gif`, or `.webp` are shown as a thumbnail card directly without HTML parsing. The filename is used as the card label.

**Mouse interaction (URL text in chat):**
- **Left-click** → opens in your default browser
- **Right-click** → context menu:
  - **Copy URL**: copies the full URL to clipboard
  - **Open URL**: opens in the default browser
  - **Hide Preview**: hides the preview card for that link
  - **Show Preview**: restores a previously hidden card (replaces Hide Preview when the card is hidden)

**Mouse interaction (preview card below the message):**
- **Left-click** the card → opens the URL in your default browser
- **Right-click** the card → same context menu as right-clicking the URL text (Open URL / Hide Preview)

Hovering over any URL shows the domain in the tooltip, updating to the full page title once the fetch completes.

**Preview queue:** when multiple URLs arrive at once, Uplink fetches their cards sequentially (up to 10 queued at a time) so fetches do not compete with each other. A URL hovered in chat triggers a lightweight title-only fetch that does not interfere with the card queue.

Preview fetches are lightweight and automatic: HTML is capped at 32 KB, images at 8 MB, and results are cached in memory for the session.

**Private addresses are never fetched**: loopback (`127.x`, `::1`), RFC 1918 private ranges (`10.x`, `172.16–31.x`, `192.168.x`), link-local (`169.254.x`), and `.local` hostnames are blocked. A link posted in chat cannot be used to probe services on your local network.

Preview cards survive channel switches; cards are stored per-channel and reinjected when you switch back.

### The emoji button doesn't show

The emoji button is hidden by default. Open **Settings → Preferences** (**Ctrl+,**) and check **Show Emoji Button**, or set it in config:

```toml
[ui]
show_emoji_button = true
```

Once visible, clicking `😊` opens a searchable grid of ~1,900 emoji. You can also type `:shortcode:` directly in the input box; a completion list appears as you type, and pressing Enter, Tab, or clicking an entry inserts the emoji. Typing the full `:trident:` with the closing colon substitutes it instantly without the completion list.

### How do I search the chat buffer?

Press **Ctrl+F** to open the search bar below the chat area. Start typing to jump to the first match. Press **Enter** to find the next match (wraps around), **Shift+Enter** for the previous match, and **Escape** to close the bar and clear the highlight.

The search is case-insensitive and works on the full visible buffer for the current channel. It does not search across channels or across sessions; switch to the channel you want to search, then press Ctrl+F.

To search *beyond* the loaded buffer (the channel's entire logged history), press **Ctrl+Shift+F**. This opens a separate window that scans the channel's on-disk log file and lists matches newest-first, with an optional **Regex** mode. It only covers history that was actually logged, so it requires message logging to be enabled in Preferences.

To search **everything at once**, tick **All buffers** in the same window. Every logged channel and PM across all your servers is scanned, and results are grouped per buffer with a match count. Double-click a result to jump straight to that buffer.

### How do I reply to a specific message?

Right-click any message in the chat area and choose **Reply** from the context menu. A reply bar appears above the input box showing **↩ nick: preview** of the original message. Type your reply and press **Enter** to send it. Press **Escape** or click **✕** to cancel.

Replied messages carry an IRCv3 `draft/reply` tag referencing the original message ID. In Uplink, received replies show a small **↩ origNick** indicator before the sender's name in the chat.

Switching channels automatically cancels any pending reply.

### How do I minimize to the system tray?

Close the window normally; it minimizes to the system tray instead of quitting. Left-click the tray icon to bring the window back. Right-click for a menu with **Show** and **Quit**.

### How do I change the app icon?

Open **Settings → Preferences** (**Ctrl+,**) and find the **App Icon** grid on the Appearance page. It shows all 15 icon variants as clickable tiles: click one and the icon changes immediately in the title bar and system tray without restarting. The preference is saved automatically.

You can also set it directly in `config.toml`:

```toml
[ui]
app_icon = "flat-black"    # the default
```

Valid names are the 15 variants listed in [Configuration](configuration.md): `flat-black`, `black-old-orange`, `black-orange`, `original-black`, `original-flat-shine`, `colorful-blueish`, `colorful-greenblue`, `colorful-hotbluepink`, `colorful-orange`, `colorful-purple`, and the five `gruvbox-*` colors. Old configs with `app_icon = "dark"` or `"light"` still work; they map to `flat-black` and `original-flat-shine` automatically.

### macOS says the app is damaged or can't be opened

macOS Gatekeeper blocks apps that are not signed with an Apple Developer certificate. The Uplink DMG is currently unsigned, so Gatekeeper may refuse to open it.

**Option 1: Right-click open (easiest)**

1. Right-click (or Ctrl+click) the DMG in Finder.
2. Choose **Open**.
3. Click **Open** in the warning dialog.

This bypasses Gatekeeper for that file and works on most macOS versions.

**Option 2: System Settings**

1. Try to open the DMG normally; macOS will block it and show a notification.
2. Open **System Settings → Privacy & Security**.
3. Scroll to the Security section; you will see a message about the blocked app.
4. Click **Open Anyway**.

**Option 3: Strip the quarantine flag (Terminal)**

If the options above do not work, open Terminal and run:

```bash
xattr -dr com.apple.quarantine ~/Downloads/Uplink-*.dmg
```

Then open the DMG normally. The `-d` flag removes the attribute, `-r` applies it recursively (covers the app bundle inside).

> The DMG is built for **Apple Silicon (arm64)** and does not run on Intel Macs: Rosetta 2 translates Intel apps for Apple Silicon, not the reverse. On an Intel Mac, build from source instead.

---

### macOS: "Host unreachable" when connecting to a server on my local network

If Uplink connects to internet networks fine but a server on your own LAN (a home bouncer, or an ircd on a local machine) fails with `Host unreachable`, macOS is blocking it. Since macOS 15, every app needs the **Local Network** privacy permission to reach local addresses like `192.168.x.x`. Internet servers are unaffected, which is why only the LAN connection fails.

Grant it under **System Settings → Privacy & Security → Local Network**: find Uplink in the list and switch it on. If Uplink is not listed yet, launch it and try connecting once, then check the list again. Because Uplink is not certificate-signed, macOS can be slow to attach the permission; if the toggle is already on and the connection still fails, quit Uplink, toggle the entry off and on, and relaunch. Old builds may have left several Uplink entries in the list; the current one is the newest entry.

---

### How do I connect to a server with a self-signed certificate?

Uplink shows a dialog on first connect:

```
Untrusted Certificate
irc.myserver.example is using a self-signed certificate.
SHA-256 fingerprint: AB:CD:EF:...
```

You have three options:

| Option | What it does |
|---|---|
| **Pin Certificate** | Saves the fingerprint to `config.toml`. All future connections verify this fingerprint; if it changes, you get a clear error and the connection is refused. |
| **Accept Once** | Allows this connection only. You will be asked again on the next connect. |
| **Reject** | Disconnects immediately. |

**Pin Certificate** is the right choice for a server you control or trust. Once pinned, `config.toml` gets a line like:

```toml
ssl_fingerprint = "AB:CD:EF:01:23:45:67:89:..."
```

If the server ever renews its self-signed cert, delete that line and reconnect; you will get the pin dialog again with the new fingerprint.

### The pinned certificate no longer matches: what do I do?

Uplink will disconnect with an error in the server buffer:

```
TLS: certificate fingerprint mismatch!
Pinned: AB:CD:...
Got:    FF:EE:...
```

This means the server's certificate changed. To re-pin:

1. Open `~/.config/uplink/config.toml`
2. Find the `ssl_fingerprint` line in the affected server block and delete it
3. Click **File → Reload Config**
4. Reconnect; the pin dialog appears with the new fingerprint

### How do I add a custom theme?

1. Create a `.toml` file in `~/.config/uplink/themes/`; the simplest starting point is to copy an existing theme from the `themes/` directory next to the binary and edit the color values.
2. Restart Uplink (or click **File → Reload Config**).
3. Your theme appears in the **Preferences** theme list.

A theme file is plain TOML: color sections like `[general]`, `[buffer]`, and `[nicklist]`, each holding `"#rrggbb"` color keys. Every key is optional and falls back to a sensible default. See the [theme file format reference](configuration.md#theme-file-format) for every section and key, or open any of the 297 built-in themes as a live example.

---

## Building from source

### What do I need to build Uplink?

- CMake 3.16 or newer
- Qt 6.2 or newer (Widgets + Network + Ssl modules)
- A C++20 compiler (GCC 10+, Clang 12+, or MSVC 2019 16.11+)
- tomlplusplus (header-only, available via most package managers)

See the [Quick Start](https://github.com/noderelay/UplinkIRC#quick-start) and platform-specific install steps in the README.

### How do I run the unit tests?

Tests are built automatically by default. After configuring with CMake:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target tst_ircparser tst_chatformat
ctest --test-dir build
```

To run a single test binary directly (useful for verbose output):

```bash
./build/tests/tst_ircparser -v1
./build/tests/tst_chatformat -v1
```

Pass `-DUPLINK_BUILD_TESTS=OFF` to CMake to skip the tests entirely (e.g. if Qt6 Test is not installed):

```bash
cmake -B build -DUPLINK_BUILD_TESTS=OFF
```

### Build fails: "tomlplusplus not found"

Install the package for your platform:

```bash
# Arch Linux
sudo pacman -S tomlplusplus

# Ubuntu / Debian
sudo apt install libtomlplusplus-dev

# Fedora
sudo dnf install tomlplusplus-devel

# FreeBSD
sudo pkg install tomlplusplus

# macOS (Homebrew)
brew install tomlplusplus
```

### Build fails: Qt6 not found

Make sure Qt6 is installed and CMake can find it. If Qt is in a non-standard location, pass the prefix:

```bash
cmake -DCMAKE_PREFIX_PATH=/path/to/Qt6 ..
```

---

## Getting help

- Join **#uplinkirc** on `irc.libera.chat`, the Uplink development channel
- File bugs and feature requests on the GitHub Issues page
- Browse the full documentation index at [docs/index.md](index.md)

---

## Link previews

### Link previews don't appear for some URLs

Uplink fetches the page title and thumbnail for URLs posted in chat. If a preview isn't appearing:

- **Are link previews enabled?** They are off by default. Open **Settings → Preferences** (**Ctrl+,**) and check **Link Previews**, or add `link_previews = true` under `[privacy]` in `config.toml`.
- **Direct image link**: `.png`, `.jpg`, `.jpeg`, `.gif`, `.webp` URLs are handled automatically and show a thumbnail card.
- **The site redirects** (e.g. `http://` → `https://`, or bare domain → `www.`); redirects are followed automatically.
- **YouTube and heavy sites**: as of v0.12.0, Uplink uses the `WhatsApp/2` user-agent, which causes most major sites to serve a compact OG-metadata page. If a site still doesn't preview, it may not publish `og:title` or `<title>` at all.
- **SSL certificate errors**: self-signed or expired certs block the fetch. There is no per-site cert override.
- **The page has no `<title>` or `og:title`**: no preview will appear; there is nothing to display.

### Why did link previews stop on my phone hotspot?

When the OS reports the connection as metered (most phone hotspots), Uplink automatically pauses background fetches to spare your data: link preview cards, hover title lookups, and remote avatar downloads. Everything resumes on the next unmetered connection. Local-file avatars and the manual update check are unaffected.

Related: when the network comes back after a drop (laptop resume, Wi-Fi roam), Uplink reconnects immediately instead of waiting out the retry backoff timer. Servers you disconnected yourself stay down.

### Previews disappear when I switch channels

Preview cards are stored per-channel and reinjected when you switch back, so they survive channel switches. If a card is missing after switching back, the fetch may still be in progress; wait a moment and it will appear.

---

## Avatars & display names

### Why don't avatars or display names show through my bouncer?

Metadata travels in the IRCv3 `draft/metadata-2`/`-3` capability, and a client can only use the capabilities its bouncer re-offers. Neither soju nor ZNC passes the metadata capability through (ZNC only forwards capabilities its loaded modules declare), so behind a bouncer Uplink never sees the capability and correctly skips all metadata: no avatars in, no profile out. Connect to the network directly and everything works. To see exactly which capabilities survived your bouncer, run `/caps` in that server's buffer.

### My own avatar shows for me but nobody else sees it

A local file path (e.g. `/home/you/avatar.png`) is displayed only in your own client and is deliberately never published: other people's clients can't read files on your disk. Host the image at a public `https://` URL and set that as your avatar instead (Preferences → Profile, or `/avatar https://...`).

### Someone set their avatar but my tooltip doesn't show it

Hover twice: the first hover requests the data in the background, the second renders it. Uplink fetches metadata lazily on hover (a channel-wide fetch on join would flood servers with rate limiting). If a user was offline when you first hovered them, Uplink retries automatically after they rejoin.

Also check **Preferences → Chat Window → Show Avatars** is still on — with it off, names and status text keep working but no image is ever fetched.

### How do I turn avatars off?

Two levels, depending on what you want gone:

- **Just the images:** uncheck **Preferences → Chat Window → Show Avatars** (or set `show_avatars = false` under `[ui]`). Display names and status text keep working. Takes effect immediately: cached images are dropped and the sidebar icons clear.
- **Metadata entirely for a server:** open **File → Manage Servers**, edit the server, and uncheck **Use metadata** (or set `metadata = false` in that `[[server]]` block). Uplink then never requests the capability there — nothing is published about you, and no names, avatars or status text come back. Takes effect on the next connection.

Avatars are on by default. The reason the switch exists is that an avatar URL is set by another user or a channel op, so fetching the image tells whatever host it points at your IP address. Everything else metadata carries arrives in-band over IRC and reaches no third party.

---

## Account tracking & Monitor

### How do I see someone's NickServ account?

Account names appear as tooltips in two places; hover over a nick in either location:

- **Nick list**: hover any nick in the right-side user list panel
- **Chat view**: hover the sender's nick link directly in a message

The tooltip shows `account: <name>` when the server has reported their account. This is populated via four mechanisms:

- **On join**: if the server supports `extended-join`, the account name arrives with the JOIN message.
- **Login/logout**: `account-notify` sends an `ACCOUNT` command when any nick in a shared channel authenticates or logs out.
- **Per-message**: `account-tag` attaches the account name to every message from an authenticated user, keeping the data current even without a separate notification.
- **WHO scan**: Uplink sends a WHOX query (`WHO #channel %cnfa,42`) on join to bulk-populate accounts; the `354` reply includes the account field. On servers that do not advertise `WHOX` in `ISUPPORT` (e.g. Rizon), a plain `WHO #channel` is sent instead; account data is not available in that case, but no error is produced.

If the tooltip is absent, the server may not support any of these capabilities, or the user has not authenticated with services.

### How do I browse channels on a server?

Type `/list` in any input bar. A **Channel Browser** dialog opens and populates live as results arrive from the server; no waiting for the full list before you can start browsing.

- **Filter**: type in the box at the top to narrow by channel name or topic.
- **Sort**: click any column header (Channel, Users, Topic) to sort; click again to reverse. Users sorts numerically.
- **Join**: double-click any row, or select it and click **Join**.
- **Refresh**: click **Refresh** to re-request the list.

The dialog stays open after joining so you can explore and join multiple channels.

### How do I connect via WebSocket instead of raw TCP?

Some servers and bouncers (e.g. The Lounge) are only reachable over WebSocket. Add `websocket = true` to the server block in `config.toml`:

```toml
[[server]]
name = "The Lounge"
host = "lounge.example.com"
port = 9000
ssl = true
websocket = true
nick = "alice"
```

When `ssl = true`, Uplink uses `wss://` (encrypted). When `ssl = false`, it uses `ws://`. All other features (SASL, reconnect, IRCv3 CAP negotiation, SOCKS5 proxy) work identically over WebSocket.

You can also enable it from the GUI: **File → Manage Servers → Edit** → tick **Use WebSocket**.

### How do I watch for a nick coming online?

Use the `/monitor` command. Uplink uses the IRCv3 MONITOR system, more efficient than the older ISON polling approach.

```
/monitor add alice       - start watching alice
/monitor del alice       - stop watching alice
/monitor list            - show everyone you're watching
/monitor clear           - remove all watched nicks
/monitor status          - force the server to report current online/offline status
```

When a watched nick connects or disconnects, a status line appears in the server buffer: `Now online: alice` / `Now offline: alice`.

The watch list is saved to `config.toml` under `[monitor]` and is automatically resent on every reconnect.

**Note:** if you have multiple servers configured, the same watch list is sent to all of them.

### I deleted a message but it still shows on another client

Message deletion uses the IRCv3 `draft/message-redaction` capability. If the other client does not support it, deleted messages will still display there. In Uplink, a deleted message shows as `[message deleted]` in grey italic. You can only delete your own messages; operators on the server may be able to delete others' messages directly via `/raw REDACT`.

### The "Delete" option doesn't appear on my messages

Two conditions must both be true: the message must be one you sent, and the server must have acknowledged the `draft/message-redaction` capability during the CAP handshake. If either condition fails, the option is not shown. Check the server buffer on connect; if you don't see the CAP listed, the server doesn't support it.

### The window opens too wide and I can't resize it

This can happen on screens narrower than the default 1100 px window width, or when saved geometry from a larger display is restored. Fixed in v0.25.1; upgrade and delete the old settings file to start fresh:

```bash
# Remove old misnamed file (leftover from earlier builds)
rm -f ~/.config/LinuxDojo/Uplink.conf

# Remove new file too so geometry starts from scratch
rm -f ~/.config/uplink/uplink.conf
```

Then relaunch. The window will clamp itself to the available screen area.

### Why does a busy channel always have a scrollbar?

Uplink renders only the most recent 150 messages when you switch to a channel. If a channel has more than 150 messages buffered, the chat view will be scrollable even before you type or receive anything; this is intentional, not a bug.

Scroll to the very top to load the previous 50 messages. Keep scrolling up to go further back. Past the in-memory buffer, older messages load from the server (on networks with `chathistory`) or from your local log files (when message logging is enabled), so scrollback keeps going as far as your history reaches. If neither source is available, a status line at the top of the buffer points you at the logging setting.

Quiet channels with fewer than 150 messages show no scrollbar until content fills the view.

---

### Where does Uplink store window geometry and layout?

Window geometry, sidebar width, splitter positions, and pane layout are saved in:

| Platform | Path |
|---|---|
| Linux / FreeBSD / macOS | `~/.config/uplink/uplink.conf` |
| Windows | `%APPDATA%\uplink\uplink\uplink.conf` |

This is separate from `config.toml` (which holds your server/UI preferences). Deleting `uplink.conf` resets the window layout to defaults without affecting any other settings.
