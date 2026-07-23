# Configuration

Uplink is configured with a single TOML file. On first launch it is created automatically with default settings; you just need to fill in your nickname.

---

## Config file location

| Platform | Path |
|---|---|
| Linux / FreeBSD | `~/.config/uplink/config.toml` |
| macOS | `~/.config/uplink/config.toml` |
| Windows | `%USERPROFILE%\.config\uplink\config.toml` |

You can edit the file directly, or use the in-app tools:

- **File → Open Config**: opens `config.toml` in your system's default text editor
- **File → Reload Config**: restarts Uplink immediately, picking up all config changes (useful after a manual edit)
- **File → Manage Servers**: add, edit, or remove servers; changes take effect immediately without editing config by hand
- **Settings → Preferences (Ctrl+,)**: the GUI for themes, font sizes, and UI toggles; changes are saved automatically

---

## Full example

This is a complete config file showing every available option.

```toml
[ui]
theme = "catppuccin-mocha"  # omit or set "default" for native look on Windows
theme_auto = false          # follow the OS light/dark scheme (uses theme_light/theme_dark)
theme_light = "light"       # theme while the OS scheme is light (only with theme_auto)
theme_dark = "dark"         # theme while the OS scheme is dark (only with theme_auto)
show_nick_prefix = true
show_topic = true
show_timestamps = true
show_emoji_button = true               # shows 😊 button next to input bar
show_send_button = true               # shows paper-plane send button in the input box
colored_nicks = true
typing_indicator = true
hanging_indent = true               # wrap long messages past the timestamp+nick column
log_messages = false            # write all messages to ~/.config/uplink/logs/ (opt-in)
show_unread_counts = true             # show message count badges in the sidebar
notifications = true             # green dot on tray icon for mentions/PMs when unfocused
highlight_words = ""             # comma-separated words highlighted like mentions (e.g. "myproject,alert")
nick_brackets = "<>"               # "<>" [nick] "()" "{}" "::::" or "" for none
pane_stack_rows = false           # false = stack panes in columns (default), true = stack in rows
panel_cards = true                # side panels float as rounded cards on the backdrop; false = classic flat look
menu_style = "menubar"    # "menubar" File/Edit/... bar (joins the KDE global menu), "hidden" shortcuts only
app_icon = "flat-black"
font_family = "IBM Plex Mono"   # Windows default is "Consolas"
font_toolbar = 10
font_sidebar = 10
font_chat = 10
font_nick_list = 10
font_nick_dock = 9     # nick panel header font size (macOS default: 13)
font_topic_bar = 10
font_topic_text = 10   # the topic text itself, independent of the bar chrome
font_input_nick = 10
font_input = 10
font_typing = 9
font_server_header = 9   # server/section header rows in the sidebar
font_emoji = 16   # emoji size in chat messages (independent of font_chat)

[privacy]
link_previews = false              # set to true to enable URL preview cards in chat

[profile]
display_name = "Alice Smith"           # shown in nick list tooltip (draft/metadata)
avatar_url = "https://example.com/avatar.png"  # https:// URL or /local/path

[[server]]
name = "LiberaChat"
host = "irc.libera.chat"
port = 6697
ssl = true
nick = "yournick"
user = "uplink"
realname = "Uplink User"
# sasl_user = "yournick"       # uncomment to enable SASL PLAIN
# sasl_password = "yourpassword"
# nickserv_password = "yourpassword"   # alternative: NickServ IDENTIFY on connect
# bouncer = "soju"           # "znc" or "soju", enables bouncer-specific caps
# bouncer_network = "libera"         # znc or soju: network name (see Bouncer section)
# proxy_host = "127.0.0.1"      # SOCKS5 proxy hostname (omit for direct connection)
# proxy_port = 1080             # SOCKS5 proxy port (default 1080)
# proxy_user = ""               # optional: proxy username
# proxy_pass = ""               # optional: proxy password
# ssl_fingerprint = ""               # pin a self-signed cert SHA-256 fingerprint (set automatically on first connect)
# websocket = false            # connect via WebSocket (ws:// or wss://) instead of raw TCP
# disabled = false            # set to true to keep in config but skip on startup
# quit_message = "Later!"         # shown to others when you disconnect (default: "Uplink")
# away_message = "AFK"            # used by /away with no argument (default: sends "Away")

[[server.channel]]
name = "#uplinkirc"

[[server.channel]]
name = "#linux"

# Per-type ignore, channel messages always visible
[[ignore.entry]]
nick = "spammer"
flags = ["pm", "notice", "invite"]
```

---

## The `[ui]` block

Controls the look and feel of the interface. All keys are optional; missing keys use the default.

| Key | Type | Default | Description |
|---|---|---|---|
| `theme` | string | `"default"` | Theme name; must match a `.toml` file in `themes/` without the extension. On Windows, `"default"` uses the native Windows style with no custom colors. |
| `theme_auto` | bool | `false` | Follow the OS light/dark scheme (needs Qt 6.5+). When on, Uplink applies `theme_light` or `theme_dark` and switches live when the desktop flips. Picking a theme from the list in Preferences turns this off. If the OS does not report a scheme, `theme` is used as before. |
| `theme_light` | string | `"light"` | Theme applied while the OS scheme is light (only with `theme_auto`) |
| `theme_dark` | string | `"dark"` | Theme applied while the OS scheme is dark (only with `theme_auto`). On Windows, `"default"` gives the native look. |
| `show_nick_prefix` | bool | `true` | Show your nickname label next to the message input box |
| `show_topic` | bool | `true` | Show the channel topic bar below the channel header row |
| `show_timestamps` | bool | `true` | Show the `HH:MM` timestamp column in front of each chat message. Timestamps take their color from the active theme. |
| `show_emoji_button` | bool | `false` | Show the 😊 emoji picker button next to the input box. Also works via `:shortcode:` typing. |
| `show_send_button` | bool | `true` | Show the paper-plane send button inside the input box. Equivalent to pressing Enter. Disable if you prefer a cleaner input area. Toggle live from **Preferences → Interface → Show Send Button**. |
| `colored_nicks` | bool | `true` | Give each nickname a unique color in chat and the nick list |
| `typing_indicator` | bool | `true` | Show "nick is typing…" notifications (IRCv3 `draft/typing`) and send your own |
| `hanging_indent` | bool | `true` | Indent wrapped message lines past the timestamp+nick column so they align with the message text. Toggle live from **Preferences → Chat Window → Hanging Indent**. |
| `log_messages` | bool | `false` | Write all messages to `~/.config/uplink/logs/<server>/<channel>.log`. History replay is not logged. Opt-in, off by default. Toggle from **Preferences → Logging → Log Messages to Disk**. Logs also feed infinite scrollback on servers without chathistory. |
| `show_unread_counts` | bool | `true` | Show a small bold count badge next to the unread indicator icon in the sidebar. Counts mentions and activity separately. Turn off from **Preferences → Interface → Show Unread Message Counts**. |
| `notifications` | bool | `true` | Show a green dot on the tray icon when you receive a mention or PM and the window is not focused. Clears automatically when you focus the window. Also toggled from **Preferences → Notifications → Tray Notifications**. |
| `highlight_words` | string | `""` | Extra words that get treated like a mention of your nick: they render bold in the active theme's keyword highlight color and count as mentions for unread indicators. Comma-separated, case-insensitive, whole words only: `highlight_words = "myproject, deploy, lunch"` highlights "deploy" but not "deployment". Leave empty to only highlight your own nick. Change live from **Preferences → Notifications → Highlight Words**. |
| `nick_brackets` | string | `"<>"` | Characters that wrap nick names in chat messages. Can also be changed live from **Preferences → Chat Window → Nick Brackets**. See [Nick bracket style](#nick-bracket-style) below. |
| `pane_stack_rows` | bool | `false` | When you have 2+ [detachable channel panes](howto.html#channel-panes) open, `false` stacks them in columns (side by side), `true` stacks them in horizontal rows. Toggle live from **Preferences → Interface → Stack Panes in Rows**. |
| `panel_cards` | bool | `true` | The server/channel list and user list use their own `[sidebar]`/`[nicklist]` theme backgrounds and float as fully rounded cards, framed by an even `[general] background` gap on all sides. Set `false` for the classic flat look where the whole window sits on the buffer color. Toggle live from **Preferences → Interface → Panel Cards**. |
| `menu_style` | string | `"menubar"` | How the app menu is presented. `"menubar"` shows the **File / Edit / View / Settings / Help / Find** menu bar; on KDE it joins the global menu automatically; elsewhere it renders in-window. `"hidden"` hides it; everything stays reachable via shortcuts and right-click menus (**Ctrl+,** always opens Preferences). Switch live from **Preferences → Interface → Menu Style**. |
| `app_icon` | string | `"flat-black"` | Which app icon variant to use. 15 choices: `"flat-black"` (default), `"black-old-orange"`, `"black-orange"`, `"original-black"`, `"original-flat-shine"`, `"colorful-blueish"`, `"colorful-greenblue"`, `"colorful-hotbluepink"`, `"colorful-orange"`, `"colorful-purple"`, `"gruvbox-blue"`, `"gruvbox-colorful"`, `"gruvbox-orange"`, `"gruvbox-purple"`, `"gruvbox-yellow"`. Change from **Preferences → Appearance** (visual grid picker). Old `"dark"`/`"light"` values are auto-migrated to `"flat-black"`. |
| `font_family` | string | `"IBM Plex Mono"` | Font family applied to all UI zones. If the configured font is not installed, Uplink silently falls back to your system's default monospace font, so a missing font never leaves the UI unreadable. |
| `font_toolbar` | integer | `10` | Legacy top-bar font size, kept for config compatibility |
| `font_sidebar` | integer | `10` | Font size (pt) for the server/channel tree |
| `font_chat` | integer | `10` | Font size (pt) for the message area |
| `font_nick_list` | integer | `10` | Font size (pt) for the user list |
| `font_nick_dock` | integer | `9` (macOS: `13`) | Font size (pt) for the nick panel header (close button, groups icon, and user count) |
| `font_topic_bar` | integer | `10` | Font size (pt) for the channel header and topic bar |
| `font_topic_text` | integer | `10` | Font size (pt) for the topic text itself, independent of the header chrome |
| `font_input_nick` | integer | `10` | Font size (pt) for your nick label next to the input |
| `font_input` | integer | `10` | Font size (pt) for the message input box |
| `font_typing` | integer | `9` | Font size (pt) for the "nick is typing…" indicator |
| `font_server_header` | integer | `9` | Font size (pt) for server/section header rows in the sidebar |
| `font_emoji` | integer | `16` | Font size (pt) for emoji characters in chat messages, independent of `font_chat` so emoji stay readable at small font sizes |

All font sizes and the theme can be changed live from **Preferences → Appearance → Font Config...** and the theme list in **Preferences → Appearance** without editing the file.

---

## Nick bracket style

The `nick_brackets` key controls the characters that wrap nick names in chat messages.

The value is a string split at its midpoint: the first half becomes the opening bracket and the second half becomes the closing bracket. An empty string removes brackets entirely.

| Value | Result | How it looks |
|---|---|---|
| `"<>"` | angle brackets (default) | `<alice> hello` |
| `"[]"` | square brackets | `[alice] hello` |
| `"()"` | parentheses | `(alice) hello` |
| `"{}"` | curly braces | `{alice} hello` |
| `"::::"` | double colons | `::alice:: hello` |
| `""` | no brackets | `alice hello` |

**Examples:**

```toml
# Default IRC style
nick_brackets = "<>"

# Square bracket style
nick_brackets = "[]"

# Double colons (4 chars, split at 2, so :: on each side)
nick_brackets = "::::"

# No brackets at all, just the nick
nick_brackets = ""
```

> **Note:** The string is always split at the midpoint. `"<>"` → open=`<`, close=`>`. `"::::"` → open=`::`, close=`::`. Odd-length strings use the first character as open and the last as close.

### Example

```toml
[ui]
theme = "nord"
colored_nicks = true
typing_indicator = true
notifications = true
app_icon = "flat-black"
font_family = "IBM Plex Mono"
font_chat = 12
font_sidebar = 10
font_input = 11
```

---

## The `[privacy]` block

Controls features that make outgoing network requests triggered by incoming data. All keys are optional and default to `false`.

| Key | Type | Default | Description |
|---|---|---|---|
| `link_previews` | bool | `false` | Fetch page titles and thumbnail images for URLs posted in chat. When enabled, Uplink makes background HTTP requests to linked sites; the linked site receives your IP address and user-agent. Disabled by default. Toggle from **Preferences → Chat Window → Link Previews** or set here. While the OS reports the connection as metered (phone hotspot), previews, hover titles, and avatar fetches pause automatically. |

```toml
[privacy]
link_previews = true    # enable URL preview cards
```

> **Privacy note:** When link previews are on, every URL posted in chat triggers a background HTTP request from your machine. The target site sees your IP address. Uplink blocks requests to private/LAN addresses, but external URLs are always fetched. Disable if you want no outgoing requests from chat content.

---

## The `[[server]]` block

Each server gets its own `[[server]]` block. The double brackets (`[[...]]`) define an array entry, so you can have as many as you like.

| Key | Type | Required | Description |
|---|---|---|---|
| `name` | string | yes | Display name shown in the sidebar. **Must be unique**: Uplink uses this as the identity key, so two servers cannot share the same name (even if they share the same host). **Renaming a server requires re-entering its password**; passwords are stored in the OS keychain under this name, so a rename loses the keychain association. Open Manage Servers, select the renamed entry, re-enter the password, and save. |
| `host` | string | yes | IRC server hostname or IP address |
| `port` | integer | yes | Server port. Standard ports: `6697` for TLS (recommended), `6667` for plain (unencrypted). |
| `ssl` | bool | yes | Use TLS encryption. Set `true` for any public server; all modern networks support it. Set `false` only for plain connections: local test servers, LAN servers, some bouncers on localhost, or `.onion` addresses where the Tor tunnel provides its own encryption. If the server advertises an STS policy, Uplink enforces TLS automatically regardless of this setting. |
| `nick` | string | yes | Your preferred nickname |
| `user` | string | no | Username in your hostmask (defaults to `"uplink"`) |
| `realname` | string | no | Shown in WHOIS (defaults to `"Uplink User"`) |
| `password` | string | no | Sent as `PASS` during connection. Required for most bouncers; also used for password-protected servers. **Stored in OS keychain; see note below.** |
| `sasl_user` | string | no | SASL username for SASL PLAIN authentication. Set together with `sasl_password` |
| `sasl_password` | string | no | SASL password for SASL PLAIN authentication. Set together with `sasl_user`. **Stored in OS keychain; see note below.** |
| `sasl_external` | bool | no | Use SASL EXTERNAL (certificate-based auth). Set to `true` together with `client_cert` and `client_key`. Cannot be combined with `sasl_user`/`sasl_password`. |
| `client_cert` | string | no | Path to the PEM client certificate for SASL EXTERNAL. |
| `client_key` | string | no | Path to the PEM private key for SASL EXTERNAL. RSA and EC (ECDSA) keys are both supported. |
| `nickserv_password` | string | no | Sends `PRIVMSG NickServ :IDENTIFY <password>` after connecting. Use on servers without SASL support. **Stored in OS keychain; see note below.** |
| `bouncer` | string | no | Bouncer type. Set to `"znc"` or `"soju"` to enable bouncer-specific IRCv3 capabilities. Omit or set `"none"` for a direct server connection |
| `bouncer_network` | string | no | The IRC network name inside your bouncer. For **ZNC**: set this alongside `sasl_user` and `sasl_password` and Uplink assembles `user/network:password` automatically, or leave blank and put the full string in `password` manually. For **soju**: the network to attach to, sent via SASL. Leave empty for single-network instances. |
| `proxy_host` | string | no | SOCKS5 proxy hostname or IP (e.g. `"127.0.0.1"`). Leave empty (the default) to connect directly with no proxy. |
| `proxy_port` | integer | no | SOCKS5 proxy port. Defaults to `1080`. |
| `proxy_user` | string | no | SOCKS5 proxy username. Only needed if your proxy requires authentication. |
| `proxy_pass` | string | no | SOCKS5 proxy password. Only needed if your proxy requires authentication. |
| `ssl_fingerprint` | string | no | SHA-256 fingerprint of a pinned self-signed TLS certificate. Set automatically when you choose "Pin Certificate" on first connect. Once set, the connection is rejected if the certificate changes. |
| `websocket` | bool | no | Connect via WebSocket instead of a raw TCP socket. When `ssl = true`, uses `wss://`; when `ssl = false`, uses `ws://`. Useful for servers behind web infrastructure (e.g. The Lounge). Defaults to `false`. |
| `quit_message` | string | no | Message broadcast to the server when you disconnect or type `/quit` with no argument. Defaults to `"Uplink"` when omitted or blank. You can always override it for a single disconnect with `/quit <message>`. |
| `away_message` | string | no | Default away message sent when you type `/away` with no argument. When omitted or blank, `/away` sends `"Away"` as a fallback. You can always override for a single session with `/away <message>`. Use `/back` to clear away status. |
| `disabled` | bool | no | When `true`, the server block is kept in `config.toml` and written back on every save, but Uplink skips it completely on startup: no connection attempt, no sidebar entry. Toggle from **File → Manage Servers** → select the server → **Disabled** checkbox in the Connection section. Defaults to `false`. |

### Minimal server block

```toml
[[server]]
name = "LiberaChat"
host = "irc.libera.chat"
port = 6697
ssl = true
nick = "yournick"
user = "uplink"
realname = "Uplink User"

[[server.channel]]
name = "#uplinkirc"
```

### Plain (unencrypted) connections

Set `ssl = false` and use port `6667` (the standard plaintext IRC port) when the server does not support TLS. This is uncommon on public networks (most have supported TLS for years) but it comes up in a few scenarios:

**Local test server**: an IRCd running on your own machine for development or testing:

```toml
[[server]]
name = "Local test"
host = "127.0.0.1"
port = 6667
ssl = false
nick = "yournick"
```

**LAN server**: a private IRC server on your home or office network that has no TLS certificate:

```toml
[[server]]
name = "Home LAN"
host = "192.168.1.10"
port = 6667
ssl = false
nick = "yournick"
```

**Bouncer on localhost**: some bouncers (ZNC, soju) are configured to listen locally without TLS, relying on the bouncer's own TLS for the upstream connection:

```toml
[[server]]
name = "ZNC local"
host = "127.0.0.1"
port = 6667
ssl = false
nick = "yournick"
password = "username/network:password"
bouncer = "znc"
```

**Tor hidden service**: `.onion` IRC servers route traffic through Tor, which provides its own layer of encryption. Some `.onion` servers offer only a plain port:

```toml
[[server]]
name = "IRC over Tor"
host = "exampleonionaddress.onion"
port = 6667
ssl = false
nick = "yournick"
proxy_host = "127.0.0.1"
proxy_port = 9050
```

> **Note:** For any public server on the regular internet, use `ssl = true`. Sending your nick, password, and messages in plaintext exposes them to anyone on the network path.

---

## Authentication

### Password storage: OS keychain

Uplink stores all passwords (`password`, `sasl_password`, `nickserv_password`) in your **OS keychain**, not as plaintext in `config.toml`. The file stores the sentinel value `"<keychain>"` instead of the actual secret.

| Platform | Storage backend |
|---|---|
| Linux | Secret Service (GNOME Keyring, KWallet, or any compatible daemon) |
| macOS | macOS Keychain |
| Windows | Windows Credential Manager |

**How migration works:** If you already have a plaintext password in your config from an older version, Uplink migrates it automatically the next time you save your settings. You do not need to do anything manually.

**What you'll see after saving:**

```toml
[[server]]
name = "LiberaChat"
...
nickserv_password = "<keychain>"   # the actual value is in the OS keychain
```

**Startup is non-blocking.** Keychain reads happen in the background after the main window appears. Each server's connection is established as soon as its passwords are resolved; you do not need to wait for all servers before the first one connects.

**If the keychain is unavailable** (e.g. no secret service daemon running on a headless Linux server), Uplink falls back gracefully: the password field simply reads as empty. In that case, enter your password in the server dialog and it will be stored once a keychain becomes available.

---

### NickServ auto-identify

If the server uses NickServ and does not support SASL, add `nickserv_password`. Uplink sends `PRIVMSG NickServ :IDENTIFY <password>` immediately after receiving the welcome numeric.

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
nickserv_password = "yourpassword"
```

The server buffer will show `Sent NickServ IDENTIFY` when this fires. For servers that support SASL, prefer `sasl_user`/`sasl_password`; SASL identifies you before you appear on the network.

### SASL EXTERNAL (certificate authentication)

SASL EXTERNAL authenticates you by your TLS client certificate; no password is ever sent. It is the most secure authentication method and is supported by Libera.Chat, OFTC, and most modern IRC servers.

**Step 1: Generate a client certificate**

```bash
# ECDSA P-384 (recommended: smaller, equally secure)
openssl req -x509 -newkey ec -pkeyopt ec_paramgen_curve:P-384 \
    -keyout ~/.irc/client.key -out ~/.irc/client.crt \
    -days 3650 -nodes -subj "/CN=yournick"

# RSA 4096 (also supported)
openssl req -x509 -newkey rsa:4096 \
    -keyout ~/.irc/client.key -out ~/.irc/client.crt \
    -days 3650 -nodes -subj "/CN=yournick"

# Lock down permissions
chmod 600 ~/.irc/client.key ~/.irc/client.crt
```

**Step 2: Register your fingerprint with the server** (Libera.Chat example)

```
/msg NickServ CERT ADD
```

This adds your certificate's SHA-512 fingerprint to your account. From then on, connecting with the certificate logs you in automatically.

**Step 3: Add to config**

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

The server buffer shows `SASL authentication successful` when it works. Uplink presents the certificate during the TLS handshake, negotiates `AUTHENTICATE EXTERNAL`, and sends an empty response; the server derives your identity from the cert's fingerprint.

> **Note:** Do not combine `sasl_external` with `sasl_user`/`sasl_password`. They are mutually exclusive.

---

### SASL PLAIN

If the server supports SASL (Libera.Chat, OFTC, and others), use `sasl_user` and `sasl_password`. Uplink negotiates the `sasl` CAP and authenticates during the handshake, before registration completes.

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
sasl_user = "yournick"
sasl_password = "yourpassword"
```

The server buffer shows `SASL authentication successful` on connect. Authentication failure does not disconnect; the connection continues without services authentication.

---

## Bouncer support (ZNC and soju)

Uplink has first-class bouncer support. Setting `bouncer = "znc"` or `bouncer = "soju"` in the server block activates bouncer-specific IRCv3 capabilities, enabling features like chat history replay, read markers, and network enumeration.

> **Important:** `host` and `port` point at your **bouncer server**, not at the IRC network. The bouncer stays connected to IRC on your behalf; Uplink connects to the bouncer.

### Connecting to ZNC

ZNC identifies clients via the IRC `PASS` command using the format `username/network:password`. Set `bouncer = "znc"` to activate ZNC-specific caps.

When `znc.in/playback` is available, Uplink sends `PRIVMSG *playback :PLAY * 0` after the welcome message to replay all missed messages. Self-messages sent from other clients are echoed correctly via `znc.in/self-message`.

**Recommended (let Uplink assemble the password):** set `sasl_user`, `sasl_password`, and `bouncer_network` separately. Uplink constructs `user/network:password` automatically at connect time:

```toml
# Uplink connects to ZNC, which connects to Libera on your behalf.
# host/port point at your ZNC server, not at the IRC network.
[[server]]
name = "ZNC - Libera"
host = "znc.example.com"   # your ZNC server, not the IRC network
port = 6697
ssl = true
nick = "yournick"
user = "uplink"
realname = "Uplink User"
sasl_user = "joe"               # ZNC username
sasl_password = "mysecretpassword"  # ZNC password
bouncer = "znc"
bouncer_network = "libera"            # network name inside ZNC
channels = "#linux, #archlinux"
```

**Alternative (embed the full string in `password`):** if you prefer the old-style format or your ZNC requires it:

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
channels = "#linux, #archlinux"
```

If your ZNC instance carries multiple networks, add one `[[server]]` block per network; only `bouncer_network` (or the network name in the password) changes:

```toml
[[server]]
name = "ZNC - Libera"
host = "znc.example.com"
port = 6697
ssl = true
nick = "yournick"
user = "uplink"
realname = "Uplink User"
sasl_user = "joe"
sasl_password = "mysecretpassword"
bouncer = "znc"
bouncer_network = "libera"
channels = "#linux"

[[server]]
name = "ZNC - OFTC"
host = "znc.example.com"
port = 6697
ssl = true
nick = "yournick"
user = "uplink"
realname = "Uplink User"
sasl_user = "joe"
sasl_password = "mysecretpassword"
bouncer = "znc"
bouncer_network = "oftc"
channels = "#debian"
```

### Connecting to soju

Set `bouncer = "soju"` to activate soju-specific IRCv3 capabilities. `soju.im/read` is negotiated automatically, keeping your read position in sync across all clients connected to the same soju instance. (No bouncer needed for this on servers that offer the standard `draft/read-marker` capability, such as Ergo — Uplink negotiates that everywhere.)

soju supports two authentication approaches: SASL PLAIN (preferred) or a `username:password` string in `PASS`. The examples below use SASL, which is the recommended method.

#### Single-network soju

If your soju instance only carries one network, no network selection is needed:

```toml
[[server]]
name = "soju"
host = "soju.example.com"
port = 6697
ssl = true
nick = "yournick"
user = "yournick"
realname = "Uplink User"
sasl_user = "yournick"
sasl_password = "yourpassword"
bouncer = "soju"
channels = "#uplinkirc"
```

#### Multi-network soju

When your soju instance carries multiple networks (Libera, OFTC, a self-hosted Ergo, etc.), you need to tell soju which network to attach to. There are two ways to do this: pick one, do not combine them.

**Option A: `bouncer_network`** (Uplink appends the network to the SASL username automatically):

```toml
[[server]]
name = "soju - Libera"
host = "soju.example.com"
port = 6697
ssl = true
nick = "yournick"
user = "yournick"
realname = "Uplink User"
sasl_user = "yournick"
sasl_password = "yourpassword"
bouncer = "soju"
bouncer_network = "libera"
channels = "#linux"
```

Uplink sends `yournick/libera` as the SASL username. Add one `[[server]]` block per network, changing only `bouncer_network` and `channels`.

**Option B: embed the network in `sasl_user` directly** (useful if your SASL username already includes the network, e.g. when soju is configured that way or when migrating from another client):

```toml
[[server]]
name = "soju - Libera"
host = "soju.example.com"
port = 6697
ssl = true
nick = "yournick"
user = "yournick"
realname = "Uplink User"
sasl_user = "yournick/libera"
sasl_password = "yourpassword"
bouncer = "soju"
channels = "#linux"
```

> **Do not combine both options.** If `sasl_user` already contains `username/network`, leave `bouncer_network` unset. Setting both causes Uplink to produce `username/network/network` as the SASL username, which soju will reject.

#### Finding your soju network name

The name in `bouncer_network` (or after the `/` in `sasl_user`) must exactly match the network identifier in soju's own configuration, not necessarily the `name =` label you give the server in Uplink. To find it, message BouncerServ while connected:

```
/msg BouncerServ network status
```

The name in the first column of the response is what to use.

### Chat history replay

When `chathistory` is negotiated (supported by soju, modern ZNC, and some IRC servers), Uplink automatically requests the last 100 messages for each channel after joining. History messages are:

- Displayed at reduced opacity so they are visually distinct from live messages
- Shown with their original timestamp; the date is prepended (`MM/dd hh:mm`) when the message is from a previous day
- Not counted as unread, so they do not badge the channel in the sidebar

No configuration is required; history replay happens automatically whenever the server supports it.

---

## SOCKS5 proxy

Each server can connect through a SOCKS5 proxy independently. This is useful for Tor, corporate proxies, or any scenario where you need to route IRC traffic through an intermediary.

### Basic proxy (no authentication)

```toml
[[server]]
name = "ExampleNet via Tor"
host = "irc.example.org"
port = 6697
ssl = true
nick = "yournick"
user = "uplink"
realname = "Uplink User"
channels = "#uplinkirc"
proxy_host = "127.0.0.1"
proxy_port = 9050
```

Set `proxy_host` to the hostname or IP of your SOCKS5 proxy. `proxy_port` defaults to `1080` if omitted.

### Authenticated proxy

If your proxy requires a username and password:

```toml
[[server]]
name = "Work IRC"
host = "irc.example.com"
port = 6697
ssl = true
nick = "yournick"
user = "uplink"
realname = "Uplink User"
proxy_host = "proxy.corp.example.com"
proxy_port = 1080
proxy_user = "myuser"
proxy_pass = "mypassword"
```

### GUI setup

Go to **File → Manage Servers** and click **Add** (or select an existing server) to fill in the **SOCKS5 Proxy** section in the right panel. Leave the host field blank to connect directly without a proxy.

### Notes

- The proxy is applied to every connection attempt including automatic reconnects and STS TLS upgrades.
- SSL/TLS still works through the proxy; the TLS handshake happens inside the SOCKS5 tunnel.
- To use **Tor**, set `proxy_host = "127.0.0.1"` and `proxy_port = 9050` (the default Tor SOCKS5 port). Make sure the Tor daemon is running.
- Leaving `proxy_host` empty (the default) connects directly; no proxy is used.

---

## WebSocket transport

Set `websocket = true` to connect to a server over WebSocket instead of raw TCP. The `ssl` key controls the scheme: `wss://` when `ssl = true` (recommended), `ws://` when `ssl = false`.

This is useful for IRC servers or bouncers accessible only over HTTP/S (e.g. The Lounge, some hosted soju instances).

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
channels = "#uplinkirc"
```

All features work identically over WebSocket: SASL, IRCv3 CAP negotiation, STS, SOCKS5 proxy, reconnect backoff, and ping watchdog.

---

## Channels

Channels to auto-join on connect. Two formats are supported.

The easiest way to manage this list from inside the app is **File → Manage Servers → Edit**, which has an **Auto-join** field per server.

### Simple format (string)

Set `channels` to a comma-separated list of channel names directly in the `[[server]]` block. This format is good for public channels that require no key.

```toml
channels = "#uplinkirc, #linux, #archlinux"
```

### Table format (with keys)

For password-protected channels, use `[[server.channel]]` sub-tables with a `key` field. This format is also how Uplink saves channels internally after the first config write.

```toml
[[server]]
name = "LiberaChat"
host = "irc.libera.chat"
port = 6697
ssl = true
nick = "yournick"

[[server.channel]]
name = "#uplinkirc"

[[server.channel]]
name = "#private"
key = "secretkey"
```

Both formats load correctly. On the next save (via **Manage Servers** or **Reload Config**), channels are written in the table format with keys preserved.

> **Note:** Uplink will not prompt you for a missing channel key. If a channel requires a key, add it to the config manually using the `[[server.channel]]` format above.

---

## The `[[ignore.entry]]` block

Stores your client-side ignore list. Each entry suppresses specific types of private communication from a nick: private messages, private notices, and/or invites. **Channel messages are never blocked**; you can ignore someone's PMs and invites while still seeing them talk in a channel.

The block is written automatically when you use `/ignore` or the right-click → **Ignore** menu. You do not normally edit it by hand.

### Format

```toml
[[ignore.entry]]
nick = "spammer"
flags = ["pm", "notice", "invite"]

[[ignore.entry]]
nick = "recruiter"
flags = ["invite"]            # block invites only, still see PMs and channel messages
```

Each entry has two keys:

| Key | Type | Description |
|---|---|---|
| `nick` | string | The nickname to suppress (case-insensitive) |
| `flags` | array of strings | Which types to block. Any combination of `"pm"`, `"notice"`, `"invite"`. Omitting the key defaults to all three. |

### Flag reference

| Flag | What it blocks |
|---|---|
| `"pm"` | Private `PRIVMSG` and `/me` actions sent directly to you |
| `"notice"` | Private `NOTICE` messages sent directly to you |
| `"invite"` | `INVITE` requests to channels |

Channel messages and actions from the ignored nick are **always visible**, regardless of flags.

### Managing the ignore list

Use slash commands in the input bar:

```
/ignore spammer              - suppress PMs, notices, and invites (default: all three)
/ignore spammer pm           - suppress private messages only
/ignore spammer invite       - suppress invites only
/ignore spammer pm invite    - suppress PMs and invites, but not notices
/unignore spammer            - remove spammer from the list entirely
/ignored                     - list all ignored nicks and their flags
```

Or right-click any nick in the user list or chat and choose **Ignore**. The context menu applies all three flags by default. For per-type control, use `/ignore` directly.

### Backwards compatibility

Uplink still reads the old single-table format written by earlier versions:

```toml
[ignore]
nicks = ["spammer", "troll123"]
```

Entries in this format are loaded as if all three flags (`pm`, `notice`, `invite`) were set. On the next save they are automatically rewritten in the new `[[ignore.entry]]` format.

---

## The `[monitor]` block

Stores your IRCv3 **Monitor** watch list: nicks to watch for coming online or going offline. When any watched nick connects or disconnects, a status line appears in the server buffer: `Now online: nick` / `Now offline: nick`.

The block is written automatically when you use `/monitor add` or `/monitor del`.

```toml
[monitor]
nicks = ["friend1", "alice", "bob"]
```

The list is sent to all connected servers on every connect and reconnect.

| Command | Description |
|---|---|
| `/monitor add <nick>` | Start watching a nick |
| `/monitor del <nick>` | Stop watching a nick |
| `/monitor list` | Show the current watch list |
| `/monitor clear` | Remove all watched nicks |
| `/monitor status` | Ask the server for current online/offline status of all watched nicks |

---

## The `[profile]` block

Stores your IRCv3 `draft/metadata` display name and avatar. Values are published to every server you connect to that advertises the capability; other users see them in the nick list tooltip.

```toml
[profile]
display_name = "Alice Smith"
avatar_url = "https://example.com/avatar.png"
```

The block is written automatically when you use the **Preferences → Profile** page or the `/displayname` and `/avatar` commands. You can also edit it directly.

| Key | Type | Description |
|---|---|---|
| `display_name` | string | Friendly name shown in the nick list tooltip alongside your IRC nick. Does not replace your nick. |
| `avatar_url` | string | URL to your avatar image, or a local file path (e.g. `/home/you/avatar.png`). A web URL is broadcast to the server via `METADATA SET` and visible to other users. A local path is displayed only to you; it is never sent to the server. Leave blank to publish no avatar. |

**Setting from Preferences:** Open **Preferences** (click **⚙** in the channel header) and select the **Profile** page from the left navigation. Fill in your Display Name and/or Avatar URL, then click **Apply to connected servers**.

**Setting from commands:**

```
/displayname Alice Smith
/avatar https://example.com/avatar.png
/avatar /home/alice/avatar.png
```

Leave the argument blank to clear a value:

```
/displayname
/avatar
```

Both commands save the value to config automatically. On the next connect to a server that supports metadata, the profile is re-published automatically; no need to re-enter anything.

**Example with both keys:**

```toml
[profile]
display_name = "Alice Smith"
avatar_url = "https://example.com/avatar.png"
```

**Example with a local avatar (visible only to you):**

```toml
[profile]
display_name = "Alice"
avatar_url = "/home/alice/Pictures/avatar.png"
```

> **Note:** `draft/metadata-2`/`-3` is supported by [Ergo](https://ergo.chat/) and [soju](https://soju.im/). Most traditional networks (Libera, OFTC) do not support it. Uplink silently skips publishing if the server does not advertise the capability.

---

## The `[[script]]` block

Stores user-defined script bindings. Each entry maps a slash command to an external executable. The block is written automatically when you use the **Preferences → Scripts** page.

```toml
[[script]]
command = "music"
path = "/home/joe/scripts/music.sh"
enabled = true

[[script]]
command = "fortune"
path = "/usr/games/fortune"
enabled = false
```

| Key | Type | Description |
|---|---|---|
| `command` | string | Command name without the leading slash (e.g. `music` becomes `/music`) |
| `path` | string | Absolute path to the executable script |
| `enabled` | boolean | Whether the command is active. Set to `false` to disable without removing. |

User scripts cannot shadow built-in commands (`/join`, `/quit`, etc.); built-in commands always take priority.

See the [Slash Commands → User Scripts](commands.md#user-scripts) section for full details on writing and using scripts.

---

## Disabling a server

Set `disabled = true` in a server block to keep it in your config without connecting to it on startup. The server block is preserved and written back on every save; nothing is lost.

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

This is the safe alternative to commenting out a `[[server]]` block. Commented-out entries are not parsed by Uplink and will be **permanently removed** the next time the app writes the config file. `disabled = true` is the correct way to temporarily skip a server.

**GUI:** Open **File → Manage Servers**, select the server, and tick the **Disabled** checkbox in the Connection section on the right. Uncheck it to re-enable the server; it will connect immediately.

**Re-enabling:** Remove the `disabled = true` line (or set it to `false`) and reload the config with **File → Reload Config**, or use the GUI checkbox.

---

## Quit message and away message

Each server block can have its own `quit_message` and `away_message`. Both are optional and work independently.

### `quit_message`

The message sent to the server when you disconnect, visible to other users in the channel as `*** yournick has quit (your message here)`.

```toml
[[server]]
name = "LiberaChat"
host = "irc.libera.chat"
port = 6697
ssl = true
nick = "yournick"
quit_message = "Later!"
```

**Defaults to `"Uplink"`** when omitted or left blank.

You can also set it per-disconnect using `/quit`:

```
/quit                       # uses the configured quit_message (or "Uplink")
/quit Heading to bed        # uses "Heading to bed" just this once
```

**GUI:** Open **File → Manage Servers**, select the server, and fill in **Quit Message** in the Identity section on the right.

---

### `away_message`

The message sent to the server when you type `/away` with no argument. Other users who send you a message while you are away receive the away reply automatically.

```toml
[[server]]
name = "LiberaChat"
host = "irc.libera.chat"
port = 6697
ssl = true
nick = "yournick"
away_message = "Away from keyboard, back soon"
```

**When omitted or blank:** `/away` with no argument sends `"Away"` as a fallback message.

**When set:** `/away` with no argument sets you away using the configured message.

You can still pass a one-off message to override it:

```
/away                             # uses configured away_message (or "Away" if none set)
/away Back in 30 minutes          # uses "Back in 30 minutes" just this once
/back                             # always clears away status regardless of config
```

**GUI:** Open **File → Manage Servers**, select the server, and fill in **Away Message** in the Identity section on the right.

---

### Full example with both

```toml
[[server]]
name = "LiberaChat"
host = "irc.libera.chat"
port = 6697
ssl = true
nick = "yournick"
user = "uplink"
realname = "Uplink User"
quit_message = "Later!"
away_message = "Away from keyboard, back soon"
channels = "#uplinkirc, #linux"
```

Different servers can have different messages:

```toml
[[server]]
name = "LiberaChat"
host = "irc.libera.chat"
port = 6697
ssl = true
nick = "yournick"
quit_message = "Later!"
away_message = "AFK"

[[server]]
name = "Libera"
host = "irc.libera.chat"
port = 6697
ssl = true
nick = "yournick"
quit_message = "Disconnecting"
away_message = "Away from keyboard"
```

---

## Multiple servers

Add as many `[[server]]` blocks as you need. Each server appears in the sidebar independently and connects on launch.

```toml
[ui]
theme = "catppuccin-mocha"

[[server]]
name = "LiberaChat"
host = "irc.libera.chat"
port = 6697
ssl = true
nick = "yournick"
user = "uplink"
realname = "Uplink User"

[[server.channel]]
name = "#uplinkirc"

[[server]]
name = "Libera"
host = "irc.libera.chat"
port = 6697
ssl = true
nick = "yournick"
user = "uplink"
realname = "Uplink User"
sasl_user = "yournick"
sasl_password = "yourpassword"

[[server.channel]]
name = "#linux"

[[server.channel]]
name = "#archlinux"
```

---

## Themes

Set `theme` in `[ui]` to any theme name from the list below. The name must match the `.toml` filename in the `themes/` folder without the extension.

Uplink ships with 297 built-in themes: 57 hand-picked originals plus 240 themes from the [base16 catalog](https://github.com/tinted-theming/base16-schemes) (named with a `-base16` suffix). At this point you probably don't need to go looking anywhere else.

Popular picks:

| Theme name | Description |
|---|---|
| `catppuccin-mocha` | Dark, muted purple tones |
| `catppuccin-latte` | Light, warm pastel variant |
| `nord` | Cool blue Arctic palette |
| `dracula` | Dark purple and pink |
| `gruvbox-dark` | Warm retro dark theme |
| `gruvbox-dark-hard-base16` | Gruvbox hard contrast, base16 variant |
| `solarized-dark` | Classic Solarized dark |
| `solarized-dark-base16` | Solarized dark, base16 variant |
| `tokyo-night-dark-base16` | Tokyo Night dark, base16 variant |
| `rose-pine-base16` | Rosé Pine, base16 variant |
| `one-dark` | Atom One Dark |
| `mactahoe26` | macOS Tahoe dark, Apple system grays |
| `mactahoe26-light` | macOS Tahoe light variant |
| `default` | Built-in fallback theme |

Themes can be switched live from **Settings → Preferences → Appearance** (or press **Ctrl+,**) without restarting.

### Where theme files live

Themes are loaded from one place: `~/.config/uplink/themes/<name>.toml`. On first launch Uplink copies its bundled themes (shipped next to the binary) into that folder, so after the first run everything, including the built-in themes, is right there and editable.

To add a custom theme, drop a `.toml` file into `~/.config/uplink/themes/`. It appears in the Preferences theme list on the next launch. Editing an existing file there changes that theme for you alone; your copies are never overwritten by updates.

### Theme file format

A theme is a small TOML file of color sections. Every key is optional; anything you leave out falls back to a sensible default, so a minimal theme can be just a `[general]` block. Colors are ordinary `"#rrggbb"` strings.

```toml
[general]
background = "#1e1e2e"   # window backdrop
text       = "#cdd6f4"   # default text
border     = "#313244"   # separators and outlines
accent     = "#89b4fa"   # selection and accent color

[sidebar]                # server/channel tree
background = "#181825"
text       = "#6c7086"
active     = "#cdd6f4"   # the selected buffer
unread     = "#89b4fa"   # buffers with unread messages
mention    = "#f38ba8"   # buffers where you were mentioned
server     = "#585b70"   # server header rows

[buffer]                 # the chat area
background  = "#1e1e2e"
timestamp   = "#6c7086"  # the HH:MM column
server_line = "#585b70"  # server/status text
action      = "#cba6f7"  # /me lines
nick_self   = "#a6e3a1"  # your own nick
separator   = "#313244"  # unread/date separator lines; falls back to [general] border

[highlights]
mention_bg   = "#2a1a2e" # background behind lines that mention you
mention_text = "#f38ba8" # your nick when someone says it
keyword      = "#fab387" # highlight_words matches

[events]                 # join/part/quit and friends; see below
join    = "#a6e3a1"
leave   = "#f38ba8"      # part, quit, and kick lines
nick    = "#89b4fa"      # nick and topic changes
notice  = "#fab387"
error   = "#f38ba8"
reply   = "#89b4fa"      # numeric server replies (WHOIS output etc.)
wallops = "#fab387"

[nicklist]               # the user list
background = "#181825"
text       = "#6c7086"
op         = "#89b4fa"   # @ operators
halfop     = "#cba6f7"   # % half-ops
voice      = "#a6e3a1"   # + voiced users
away       = "#45475a"   # users marked away

[input]                  # the message box
background = "#313244"
text       = "#cdd6f4"
placeholder= "#6c7086"
nick_color = "#a6e3a1"   # your nick label next to the input
```

The `[events]` section is special: if you leave it out entirely, event lines derive their colors from the rest of your palette automatically. Joins use `nick_self` (your theme's green), parts/quits/errors use `mention_text` (its red), nick/topic changes and server replies use `accent` (its blue), and notices/wallops use `keyword` (its amber). Most themes never need to spell these out; add explicit `[events]` keys only when you want to override a specific line type.

---

## Common mistakes

### Forgetting quotes around strings

All string values in TOML must be in double quotes. The `#` character starts a comment if it appears outside a string, which trips up channel names written bare.

```toml
# Wrong, parse error
name = LiberaChat

# Correct
name = "LiberaChat"
```

### Using single brackets for servers

`[server]` defines a single table. `[[server]]` defines an array entry and is what Uplink expects. Always use double brackets.

```toml
# Wrong
[server]
name = "LiberaChat"

# Correct
[[server]]
name = "LiberaChat"
```

### Nick contains spaces or invalid characters

IRC nicks cannot contain spaces. Allowed characters: letters, digits, `_`, `-`, `[`, `]`, `{`, `}`, `|`, `\`, `^`, and `` ` ``. Most servers enforce a max length of 30 characters.

---

## Self-signed certificate pinning

Most public IRC servers use valid TLS certificates. If you connect to a private or self-hosted server with a self-signed certificate, Uplink will show a dialog on first connect:

> **Untrusted Certificate**
> `irc.myserver.example` is using a self-signed certificate.
> SHA-256 fingerprint: `AB:CD:EF:...`
> Trust this certificate?

You have three choices:

| Option | What it does |
|---|---|
| **Pin Certificate** | Saves the fingerprint to `config.toml`. Future connections verify the fingerprint and fail with an error if the certificate changes. |
| **Accept Once** | Allows this connection only. No config change. You will be asked again on next connect. |
| **Reject** | Disconnects immediately. |

### What gets saved

When you choose **Pin Certificate**, Uplink adds this line to your server block:

```toml
[[server]]
name = "MyServer"
host = "irc.myserver.example"
port = 6697
ssl = true
ssl_fingerprint = "AB:CD:EF:01:23:45:67:89:AB:CD:EF:01:23:45:67:89:AB:CD:EF:01:23:45:67:89:AB:CD:EF:01:23:45"
```

### Certificate changed warning

If the pinned certificate no longer matches (e.g. the server renewed its self-signed cert), Uplink disconnects and shows an error in the server buffer:

```
TLS: certificate fingerprint mismatch!
Pinned: AB:CD:...
Got:    FF:EE:...
```

To re-pin, remove the `ssl_fingerprint` line from `config.toml` and reconnect. You will get the pin dialog again.

### Notes

- Fingerprints are SHA-256, formatted as uppercase hex pairs separated by colons.
- Only self-signed certificate errors trigger the dialog. Other TLS errors (expired, wrong hostname, etc.) always disconnect immediately.
- Pinning is only as trustworthy as your first connection. If you are on a network where someone could intercept that first connect, the pin gives no protection. For servers you set up yourself, it is safe.
