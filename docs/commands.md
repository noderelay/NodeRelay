# Slash Commands

Type any of these commands in the message input box and press Enter.

---

## Channel commands

| Command | Description |
|---|---|
| `/join #channel` | Join a channel |
| `/join #channel [key]` | Join a key-protected channel |
| `/j #channel` | Short alias for `/join` |
| `/part` | Leave the current channel |
| `/part reason` | Leave the current channel with a part message. Everything after `/part` is the reason |
| `/leave` | Alias for `/part`; leaves the current channel |
| `/close` | Alias for `/part`; leaves the current channel (or closes a PM buffer) |
| `/topic` | Show the current channel topic |
| `/topic <text>` | Set the current channel topic |
| `/topic #channel <text>` | Set the topic on a specific channel |
| `/kick <nick> [reason]` | Kick a user from the current channel (requires op) |
| `/invite <nick>` | Invite a user to the current channel |
| `/invite <nick> #channel` | Invite a user to a specific channel |
| `/mode <target> <flags> [args]` | Set channel or user modes |
| `/op <nick>` | Give op (`+o`) in the current channel |
| `/deop <nick>` | Remove op (`-o`) in the current channel |
| `/voice <nick>` | Give voice (`+v`) in the current channel |
| `/devoice <nick>` | Remove voice (`-v`) in the current channel |
| `/ban <mask>` | Ban a mask (`+b`) in the current channel |
| `/unban <mask>` | Remove a ban (`-b`) in the current channel |
| `/ignore <nick>` | Suppress private messages, notices, and invites from a nick; channel messages are never blocked |
| `/ignore <nick> pm` | Suppress only private messages from that nick |
| `/ignore <nick> pm notice invite` | Suppress specific types: any combination of `pm`, `notice`, `invite` |
| `/unignore <nick>` | Stop ignoring a nick |
| `/ignored` | List all currently ignored nicks and which types are suppressed |
| `/monitor add <nick>` | Add a nick to the online/offline watch list (IRCv3 MONITOR) |
| `/monitor del <nick>` | Remove a nick from the watch list |
| `/monitor remove <nick>` | Alias for `/monitor del` |
| `/monitor list` | Show all watched nicks |
| `/monitor clear` | Clear the entire watch list |
| `/monitor status` | Ask the server for current online/offline status |
| `/clear` | Clear the chat buffer |

### Examples

```
/join #linux
/join #private secretkey
/j #uplinkirc
/part
/part see you later
/leave
/close
/topic new topic here
/topic #uplinkirc Welcome to the server
/kick baduser spamming
/invite alice
/invite alice #linux
/mode #uplinkirc +m
/op alice
/deop bob
/voice alice
/ban *!*@spammer.host
/unban *!*@spammer.host
/ignore spammer              # suppress PMs, notices, and invites (channel messages still visible)
/ignore spammer pm           # suppress only private messages
/ignore spammer pm notice    # suppress PMs and private notices
/ignore spammer invite       # suppress invites only
/unignore spammer            # remove from ignore list entirely
/ignored                     # list: spammer [pm,notice,invite]
/clear
```

---

## Messaging

| Command | Description |
|---|---|
| `/me <text>` | Send an action message. Shown as `* yournick text` in chat |
| `/msg <nick> <text>` | Send a private message; opens a PM buffer in the sidebar for the conversation |
| `/query <nick>` | Open a PM buffer without sending a message |
| `/notice <target> <text>` | Send a NOTICE to a user or channel |

### Examples

```
/me waves hello
/msg alice hey, are you around?
/query alice
/notice alice heads up
```

---

## Services shortcuts

Shortcuts for sending messages to network services. These are equivalent to `/msg <Service> <text>` but shorter to type.

| Command | Equivalent |
|---|---|
| `/ns <text>` | `/msg NickServ <text>` |
| `/cs <text>` | `/msg ChanServ <text>` |
| `/bs <text>` | `/msg BotServ <text>` |
| `/ms <text>` | `/msg MemoServ <text>` |

### Examples

```
/ns identify mypassword
/ns register mypassword email@example.com
/cs op #uplinkirc
/cs invite #uplinkirc
/ms list
```

---

## Nick and identity

| Command | Description |
|---|---|
| `/nick <name>` | Change your nickname |
| `/away [message]` | Set yourself as away. If no message is given, uses the server's configured `away_message`; if that is also unset, sends `"Away"` as the fallback message |
| `/back` | Clear your away status |
| `/whois <nick>` | Look up info about a user; reply appears in the active channel |
| `/whowas <nick>` | Query history for a departed nick; shows last known user@host and realname |
| `/setname <realname>` | Change your realname (GECOS) on the fly without reconnecting (IRCv3 setname) |
| `/displayname <text>` | Set your display name via IRCv3 `draft/metadata`; shown in nick list tooltips. Leave blank to clear. Requires server support. |
| `/avatar <url>` | Set your avatar via IRCv3 `draft/metadata`. Accepts an `https://` URL or a local file path. Leave blank to clear. Requires server support. |
| `/caps` | List all IRCv3 capabilities currently negotiated with the server |

### Examples

```
/nick coolnick
/away grabbing coffee
/away                    # uses configured away_message, or "Away" if none set
/back                    # always clears away regardless of config
/whois alice
/whowas alice
/setname Alice Smith
/displayname Alice Smith
/displayname
/avatar https://example.com/avatar.png
/avatar /home/alice/avatar.png
/avatar
/caps
```

---

## Connection & IRC operator

| Command | Description |
|---|---|
| `/connect [host[:port]]` | Reconnect to the current server, or connect to a new server. If the host is already in your config, uses that entry; otherwise creates an ad-hoc connection with auto-detected SSL (port 6667 = plain, everything else = SSL). Append `+` to the host (e.g. `/connect irc.example.org+:6667`) to force SSL regardless of port. |
| `/server [host[:port]]` | Alias for `/connect` |
| `/disconnect` | Close the current server and all its channels from the sidebar. The server remains in your config and will reconnect on next launch. |
| `/quit [message]` | Disconnect from the current server. If no message is given, uses the server's configured `quit_message` (default: `"Uplink"`) |
| `/motd [server]` | Request the message of the day |
| `/list` | Open the channel browser: a sortable dialog showing all channels with user count and topic; type to filter, double-click or press Join to join |
| `/stats <query>` | Request server statistics: `u`=uptime, `o`=opers, `m`=commands |
| `/time` | Query the server's local time |
| `/oper <user> <pass>` | IRC operator login; sends `OPER user :pass` to the server |

### Examples

```
/connect                 # reconnect to the current server
/connect irc.libera.chat # connect to Libera Chat (SSL, port 6697)
/server irc.example.org:6667  # connect plain-text on port 6667
/disconnect              # close the current server from the sidebar
/quit                    # uses configured quit_message (default: "Uplink")
/quit later everyone     # overrides for this disconnect only
/motd
/list
/stats u
/stats o
/time
/oper myname mysecretpassword
```

---

## CTCP

| Command | Description |
|---|---|
| `/ping <nick>` | Send a CTCP PING to a user; reply shows round-trip time in the active channel |
| `/time <nick>` | Ask a user for their local time via CTCP; reply appears in the active channel |
| `/version [nick]` | Request server version, or CTCP VERSION from a nick |
| `/ctcp <nick> <command> [args]` | Send a raw CTCP request to a user |
| `/sysinfo` | Collect OS, CPU, RAM, GPU, and uptime info and post it to the current channel |

Incoming CTCP VERSION and PING requests are answered automatically.

> **Note:** `/sysinfo` collects info in the background (GPU detection via `vulkaninfo`/`lspci` can take a moment). Uplink posts "Collecting system info…" immediately and replaces it with the result when ready. A 12-second timeout applies; if collection hangs, an error is posted instead.

> **Note:** `/time` and `/ping` only work if the target client supports those CTCP commands. Most IRC clients do, but bots often do not respond. If you see no reply, the other side simply does not support it.

### Examples

```
/ping alice
/time alice
/version
/version alice
/ctcp alice CLIENTINFO
/sysinfo
```

---

## File transfer (DCC)

DCC Send is initiated from the **nick list right-click menu**, not a slash command.

Uplink supports two DCC modes. Choose based on your network situation:

| Mode | When to use |
|---|---|
| **Send File** (active) | You have a reachable port: direct internet connection, or your router has port forwarding set up. The sender listens; the recipient connects in. |
| **Send File (Passive)** | You are behind NAT (home router, VPN) and cannot accept incoming connections. The recipient opens the port instead and you connect out to them. |

**Sending a file (active mode):**

1. Right-click any nick in the user list on the right side of the window.
2. Choose **Send File**.
3. Pick a file in the file dialog.
4. A progress dialog appears. Transfer begins as soon as the recipient accepts.

**Sending a file (passive mode, for use behind NAT):**

1. Right-click the recipient's nick.
2. Choose **Send File (Passive)**.
3. Pick a file. Uplink sends the offer with `port=0`, signalling that the recipient should open the port.
4. The recipient's client opens a port and sends you its address. Uplink then connects out to them automatically.
5. A progress dialog tracks the transfer.

**Receiving a file:**

When someone sends you a file (either mode), a dialog appears:

```
alice wants to send you:
report.pdf  (2.1 MB)

Accept?   [Yes]  [No]
```

Click **Yes**, choose a save location, and a progress dialog tracks the download. Click **Cancel** at any time to abort.

The file is written to a `.part` file while downloading and renamed to the final filename only when the transfer completes successfully. If the transfer is cancelled or fails, the partial file is deleted automatically.

**Receive limits:**

| Limit | Value |
|---|---|
| Maximum file size | 2 GiB |
| Disk space | Checked against available space before starting; transfer is rejected if there is not enough room |

**Stall detection:** if the other side stops responding mid-transfer (no data acknowledged for over a minute), Uplink aborts the transfer with an error instead of hanging forever. Slow transfers are fine; only a genuinely stalled connection is aborted.

> **NAT and firewalls:** Active DCC requires the sender's port to be reachable from the internet. If the sender is behind a home router, use **Send File (Passive)**; this flips who opens the port. If *both* sides are behind NAT, neither mode will work without a relay.

---

## Sidebar right-click menu

Right-clicking a **channel** in the sidebar opens a context menu:

| Action | What it does |
|---|---|
| **Open in Pane** | Opens the channel as a side-by-side pane with its own chat view, nick list, topic bar, and input bar. Appears when the channel is not already open in a pane. Pane layout is saved automatically and restored on the next launch. |
| **Close Pane** | Closes the side-by-side pane for this channel. Appears when a pane is already open. |
| **Rejoin** | Parts the channel and immediately rejoins it. |
| **Leave** | Parts the channel (`PART`). |
| **Close** | Closes the buffer without sending a `PART` (use after a kick, for example). |

Right-clicking a **server header** shows:

| Action | What it does |
|---|---|
| **Disconnect** | Sends `QUIT` and closes the connection. |
| **Reconnect** | Re-connects to the server using the current config. |
| **Close Server** | Disconnects and removes the server and all its channels from the sidebar. The server stays in your config and returns on next launch. |
| **Move Up** | Moves the server up in the sidebar. Order is saved to config. |
| **Move Down** | Moves the server down in the sidebar. Order is saved to config. |

---

## Nick list right-click menu

Right-clicking any nick, in the user list or directly on a nick link in the chat view, opens the same context menu. The title shows the nick in bold.

| Action | What it does |
|---|---|
| **Message** | Opens a private message buffer in the sidebar. Equivalent to `/msg nick`. |
| **Whois** | Sends `WHOIS nick`. The response appears in the active channel. |
| **Copy Nick** | Copies the nickname to the clipboard. |
| **Ignore ▶** | Opens a submenu with three checkboxes: **Private Messages**, **Notices**, **Invites**. Tick or untick each independently. Channel messages are always visible. If any type is active, **Unignore All** appears at the bottom. For keyboard control use `/ignore nick [pm] [notice] [invite]`. Persists in config. |
| **CTCP ▶** | Submenu with CTCP commands (see below). |
| **DCC ▶** | Submenu for file transfer (see below). |
| **Chan Ops ▶** | Submenu for channel operator actions (see below). |

### CTCP ▶

| Action | What it does |
|---|---|
| **Ping** | Sends a CTCP PING. Reply shows RTT in the active buffer: `Ping reply from nick: Xms`. |
| **Version** | Sends a CTCP VERSION request. Reply appears in the active channel. |

### DCC ▶

| Action | What it does |
|---|---|
| **Send File** | Opens a file picker and sends the file via active DCC. Use when you have a reachable port. |
| **Send File (Passive)** | Same as Send File, but the recipient opens the port instead. Use when you are behind NAT. |

### Chan Ops ▶

| Action | What it does |
|---|---|
| **Give Op** | Sets `+o` on the nick. Requires op. |
| **Take Op** | Removes `-o` from the nick. Requires op. |
| **Give Voice** | Sets `+v` on the nick. Requires op or half-op. |
| **Take Voice** | Removes `-v` from the nick. Requires op or half-op. |
| **Invite** | Opens a dialog pre-filled with the current channel. Edit if needed and click OK to send `INVITE nick #channel`. |
| **Kick** | Prompts for an optional reason, then kicks. Requires op. |
| **Ban** | Sets `MODE #channel +b nick!*@*`. Requires op. |
| **Kick & Ban** | Bans first, then kicks (correct order). Prompts for reason. Requires op. |

### Examples

```
# Right-click alice → Chan Ops ▶ → Invite
# Dialog pre-filled: #uplinkirc   → click OK
# Sends: INVITE alice #uplinkirc

# Right-click spammer → Chan Ops ▶ → Kick & Ban
# Reason dialog: "flooding"
# Sends: MODE #uplinkirc +b spammer!*@*
#         KICK #uplinkirc spammer :flooding

# Right-click alice → CTCP ▶ → Ping
# Buffer: Ping reply from alice: 42ms

# Right-click alice → DCC ▶ → Send File
# Pick a file → transfer starts

# Right-click bob → Ignore ▶ → tick Private Messages
# Bob's PMs are hidden; channel messages still visible
```

---

## Reactions

Uplink supports IRCv3 `draft/react`: emoji reactions attached to specific messages.

**Sending a reaction:**
- Right-click any message **timestamp** in the chat view → **React** → the emoji picker opens; search by name (e.g. `thumbs`, `:fire`) and click the emoji, or type a shortcode like `:poop:` and press Enter
- Or use `/react <emoji>` after right-clicking a timestamp and choosing **Reply** (sets the message target)

**Receiving reactions:**
- Reactions appear inline below the original message as `emoji(count)` whenever they are received

**Requirements:** the server must support the `draft/react` capability. If the server does not advertise it, reactions are silently skipped.

### Examples

```
# Right-click a timestamp → React → emoji picker opens → click 👍 or type :thumbsup: Enter

# Via slash command (requires a reply target set first):
/react 🔥
/react 👍
```

---

## Message timestamp right-click

Right-clicking a message **timestamp** (the `hh:mm` at the left of each line) opens a context menu for that specific message:

| Action | What it does |
|---|---|
| **Reply** | Sets this message as the reply target. A `↩ nick` bar appears above the input. Press Enter to send; Escape to cancel. |
| **React** | Opens the emoji picker. Search by name or shortcode, click an emoji or press Enter to send it as an IRCv3 `draft/react` reaction. Requires server support for `draft/react`. |

---

## User Scripts

Uplink lets you link external scripts to custom slash commands. Any executable (bash, python, ruby, whatever) can become a command. Configure them in **Preferences → Scripts**.

Uplink ships with four bundled scripts (`/music`, `/weather`, `/uptime`, `/roll`) that are auto-installed on first launch to `~/.config/uplink/scripts/`. They show up in Preferences → Scripts ready to use.

You can also add your own. Each script row has:
- **Enabled** checkbox
- **Command** name (e.g. `music`, which becomes `/music`)
- **Script path**: full path to the executable
- **Browse** button to pick the file
- **Delete** button to remove the binding

If you delete a bundled script and want it back, click **Restore Defaults**; it only re-adds the missing ones without touching your custom scripts.

When you run a user script command, Uplink:
1. Runs the script in a background thread (UI stays responsive)
2. Passes context as environment variables: `UPLINK_NICK`, `UPLINK_SERVER`, `UPLINK_CHANNEL`, `UPLINK_ARGS`
3. Also passes any arguments after the command as positional arguments
4. Sends each line of stdout to the current channel (max 5 lines, 450 chars each)
5. Shows errors (non-zero exit, stderr, timeout) locally, never sent to the channel

Scripts have a 10-second timeout. User scripts cannot shadow built-in commands. On Windows, `.sh` scripts are automatically launched via `bash.exe` (Git Bash or WSL).

### Included: `/music`

The `scripts/music.sh` script shows the currently playing track. It auto-detects the platform:

| Platform | How it works | Dependency |
|---|---|---|
| **macOS** | Queries macOS Now Playing center | `brew install nowplaying-cli` |
| **Linux** | Queries MPRIS2 via D-Bus; works with any MPRIS-compatible player (Spotify, VLC, mpd, Firefox, Chrome/YouTube Music, etc.) | `playerctl` recommended (`pacman -S playerctl` or `apt install playerctl`), falls back to `dbus-send` if not installed |
| **Windows** | Queries Windows Global System Media Transport Controls (GSMTC) via PowerShell; works with any media player that integrates with Windows media controls (Spotify, Chrome, Edge, VLC, etc.) | Git Bash, Windows 10 1809+, PowerShell 5.1+ (all standard) |

On macOS, if `nowplaying-cli` is not in PATH (common when launching from a GUI app), the script falls back to `/opt/homebrew/bin/nowplaying-cli`. If you installed Homebrew to a non-default location, edit the path in the script.

The `/music` command is auto-installed on first launch. On macOS, install the dependency first: `brew install nowplaying-cli`. On Linux, install `playerctl`, or the script falls back to raw D-Bus queries automatically. On Windows, no extra dependencies are needed beyond Git Bash.

**Usage:**
```
/music
♫ Now playing: Flesh and Bone — Landon Wilks (Flesh and Bone)
```

### Included: `/weather`

Shows current weather from [wttr.in](https://wttr.in). No dependencies beyond `curl`.

```
/weather              - weather for your IP-detected location
/weather Portland     - weather for Portland
🌤 Portland: Sunny +88°F 25% humidity ↘13mph wind
```

### Included: `/uptime`

Shows system uptime. No dependencies; uses built-in commands on macOS, Linux, and Windows.

```
/uptime
⏱ up 4 hours, 58 minutes
```

### Included: `/roll`

Dice roller using tabletop notation (NdS = N dice with S sides). No dependencies; pure bash.

```
/roll           - rolls 1d6 (default)
/roll 2d6       - roll 2 six-sided dice
/roll 1d20      - classic D&D d20
🎲 2d6: 3 + 5 = 8
```

### Writing your own scripts

A script is any executable file that prints to stdout. Example: a simple greeting script:

```bash
#!/bin/bash
echo "Hello from $UPLINK_NICK on $UPLINK_SERVER in $UPLINK_CHANNEL!"
echo "Args: $UPLINK_ARGS"
```

Available environment variables:

| Variable | Value |
|---|---|
| `UPLINK_NICK` | Your current nickname |
| `UPLINK_SERVER` | Server name (e.g. `LiberaChat`) |
| `UPLINK_CHANNEL` | Current channel (e.g. `#uplinkirc`) |
| `UPLINK_ARGS` | Everything typed after the command |

Arguments are also passed as positional parameters (`$1`, `$2`, etc.).

---

## Raw IRC protocol

| Command | Description |
|---|---|
| `/raw <line>` | Send a raw IRC protocol line directly to the server |
| `/quote <line>` | Alias for `/raw` |

Use these when you need to send a command that Uplink does not have a built-in shortcut for. Unrecognized slash commands are NOT forwarded to the server: typing `/REHASH` prints `Unknown command: REHASH (use /raw or /quote to send raw IRC)` and sends nothing. Prefix server-specific commands with `/raw` instead. (The one exception: commands you have bound with a user script run as usual.)

### Examples

```
/raw MODE #uplinkirc +m
/raw INVITE alice #uplinkirc
/quote JOIN #test

/raw REHASH
/raw SAMODE #uplinkirc +o alice
/raw GLOBOPS Hello opers
```

---

## Help

| Command | Description |
|---|---|
| `/help` | Print a list of all available commands in the current chat buffer |

---

---

## Emoji

Uplink has two ways to insert emoji:

### Emoji picker button
Click **⚙** to open **Preferences** and check **Show Emoji Button** (or set `show_emoji_button = true` in config). Click the 😊 button next to the input box to open a searchable grid of ~400 emoji.

### :shortcode: typing
Type a colon followed by a keyword and a completion list appears:

| You type | Result |
|---|---|
| `:smi` | autocomplete list: 😄 smile, 😃 smiley, … |
| `:trident:` | auto-replaces to 🔱 as soon as you type the closing `:` |
| `:fire:` + Enter | sends 🔥 in the message |

- Use **Up/Down** arrows to navigate the list, **Enter** or **Tab** to confirm, **Escape** to dismiss.
- Any `:shortcode:` patterns still in the text when you press Enter are resolved before the message is sent.

---

## Notes

- Commands are case-insensitive: `/JOIN` works the same as `/join`.
- Any input that does not start with `/` is sent as a regular message to the current channel or user.
- If you want to send a message that starts with `/` literally, prefix it with a space.
