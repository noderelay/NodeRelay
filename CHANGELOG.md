# Changelog

<!--
Session 2026-07-02:
- AppImage catalog PR #3778 MERGED by probonopd; UplinkIRC accepted, listing pending next site rebuild (not yet visible on appimage.github.io/apps).
- MainWindow refactor complete: UpdateChecker, DccController, PreviewController extracted as classes;
  input bar and chat view update logic split to inputbar.cpp / chatupdates.cpp. mainwindow.cpp 4669 → 3184 lines.
- ChatView perf: per-line QTextLayout cache reused across repaints (evicted off-screen);
  incremental width relayout skips lines whose height can't change; deferred chunked relayout on resize.
- 8-angle code review of the day's diff found and fixed: layoutWidth staleness poisoning (deferred pass
  could permanently skip stale lines), resize visible-window computed from stale heights, eviction index
  drift, preview watchdog leaking the 100-entry budget (pre-existing), Tab-at-column-0 clobbering
  (pre-existing), scroll-anchor overshoot, PNG re-decode on live previews.
- CI matrix + CodeQL green; ASan/UBSan build and tests clean. User smoke-tested on macOS.
No release tagged. Known deferred cleanups: preview-card ChatLine built 4 ways (live cards styled,
refreshed cards plain — pre-existing); ChatRenderer::Context copy-pasted 9 times.
Next: /calc /8ball /shrug /tableflip bundled scripts; accessibility; long-press context menus.
-->

<!--
Session 2026-06-28:
- AppImage catalog PR #3778 still open; reviewer has not responded since noderelay replied pointing to v0.25.55.
- Cleaned up GitHub release history: deleted 70 of 83 releases, kept 13 meaningful milestones.
  Reason: suspicious release cadence (40+ in June with 2 stars) flagged in external code review.
- Rewrote CHANGELOG.md from 6900+ lines to 183 lines covering only the 13 kept releases;
  features from dropped releases folded into the nearest kept version.
- Fixed docs/index.html version (was stuck on v0.25.54); ran sync-site.sh to update README.md.
- Fix: topic set-by label now strips the nick!user@host mask down to just the nick.
  mainwindow.cpp — both setText call sites use setter.section('!', 0, 0).
No regressions. No known issues.
Next: wait on AppImage PR; consider v1.0 milestone renumber on next release.
-->

## v0.25.55 — 2026-06-27

- AppImage now builds on Ubuntu 22.04 (glibc 2.35) for broader distro compatibility
- Auto-update: "Check for Updates" now downloads and installs the latest release; AppImage gets an in-place replace and relaunch, Windows downloads the ZIP, macOS opens the DMG in Finder
- User script bindings: bind any executable to a slash command via Preferences → Scripts; scripts get context via env vars (UPLINK_NICK, UPLINK_CHANNEL, UPLINK_ARGS); 10s timeout, 5-line output cap
- Bundled scripts ship out of the box and auto-install on first launch: /music (MPRIS2 on Linux, nowplaying-cli on macOS, GSMTC on Windows), /weather (wttr.in), /uptime, /roll (e.g. /roll 2d6)
- Keyboard navigation: Alt+Up/Down cycles channels, Alt+Left/Right cycles panes
- Ctrl+K quick channel switcher: floating popup with live filtering
- Scroll to top of a channel buffer fetches older history via CHATHISTORY BEFORE
- Jump-to-bottom button fades in when scrolled up; click to return to live chat
- Touch/tablet flick scrolling with momentum on chat view, sidebar, nick list, and panes
- Emoji set expanded from 622 to 1,906 (Unicode 16.0 with GitHub gemoji shortcodes)
- AppImage bundles Noto Color Emoji as a fallback font
- /sysinfo posts directly without a confirmation dialog
- Link preview no longer shows error page titles; HTTP 4xx/5xx responses are dropped
- Emoji autocomplete popup now tracks cursor position correctly


## v0.25.45 — 2026-06-18

- Preferences dialog redesigned: flat navigation list on the left (Appearance, Chat Window, Interface, Notifications, Logging, Profile), content pages on the right
- Manage Servers dialog redesigned: server list on the left, settings form on the right inline; no separate Edit dialog
- 15 app icon variants with a grid picker in Preferences → Appearance; icon updates window, tray, and KDE taskbar live
- CodeQL semantic analysis added to CI (every push, PR, and weekly)
- Config hot-reload: Uplink watches config.toml for external edits; server changes take effect without restarting
- Font zoom: Ctrl+mousewheel or Ctrl+Plus/Minus zooms the font for the region under the cursor; persists to config
- Fade scrollbars on chat view, sidebar, nick list, and panes; fade out at rest, visible on scroll or hover
- AppImage self-integrates on first run: writes a .desktop entry and icon to ~/.local/share/ so it appears in launchers
- /connect and /server connect to an arbitrary host[:port] with auto-detected SSL; /disconnect closes the current server
- Server reordering via Manage Servers or right-click context menu; persists to config
- Server identity based on config name instead of hostname; fixes collisions when two entries point to the same host
- Duplicate server names rejected in Manage Servers
- Sidebar order syncs live after Manage Servers closes
- Log directories renamed from hostname-based to name-based on startup; existing history preserved
- Keychain credential migration delimiter fixed; all four fields now migrate correctly
- DCC use-after-free on cancel/error/finish race fixed; all DCC lambdas use QPointer
- Stale passive DCC entries cleaned up after 120 seconds
- DCC SEND with quoted filenames (e.g. "my file.txt") now parses correctly
- TLS prompt says "could not be verified" for hostname mismatches instead of "self-signed"
- Height-only window resizes skip the full chat relayout
- /nick now correctly updates the UI display and self-highlight regex
- Single-word IRC message bodies (no colon prefix) now display and trigger mention detection correctly


## v0.25.33 — 2026-06-15

- New messages separator: switching to a channel with unread messages shows a divider before the first unread and scrolls to it; clears on focus
- Non-bottom scroll positions saved per-channel and restored on return
- mIRC formatting input: Ctrl+B bold, Ctrl+I italic, Ctrl+U underline, Ctrl+O reset; inserts IRC control codes at cursor
- Nick list filter field above the nick list; clears on channel switch, Escape to clear
- Virtual scrolling: chat view renders the most recent 150 messages; scroll to top loads 50 more at a time with position preserved
- Per-server quit and away messages configurable in config or Edit Server dialog
- Per-server disabled flag: disabled = true keeps a server in config without connecting on startup
- Per-type ignore: /ignore nick [pm] [notice] [invite] suppresses each type independently; /ignored shows active flags per nick
- ZNC bouncer_network field: set it separately and Uplink assembles user/network:password at connect time
- Nick right-click menu reorganized into submenus: CTCP (Ping, Version), DCC (Send File, Send File Passive), Chan Ops (Op, Voice, Invite, Kick, Ban, Kick & Ban)
- Send button disabled when the input field is empty
- WHO falls back to plain WHO on servers that don't advertise WHOX in ISUPPORT; fixes "Unknown command" on networks like Rizon
- Mention detection no longer fires on every unread message when no highlight pattern is set
- Typing indicator works on servers using the finalized typing CAP, not just the draft form
- HiDPI icons render at actual screen DPI instead of a hardcoded 2x
- Windows maximize button no longer greyed out on first launch


## v0.25.0 — 2026-06-07

- WHOX bot detection fixed for Ergo IRCd: returns 5-field WHO replies without a token; size check and param indices corrected so bot nicks and account names populate
- echo-message self-echo fixed: typing TAGMSG and outgoing DCC CTCP echoed back from the server no longer trigger indicators or dialogs on the sender
- KDE wallet prompts fixed: Config::save() only writes to the OS keychain when saving from Manage Servers; unrelated config changes no longer trigger wallet unlock
- /whois, /version, and /ping responses now appear in the active channel or PM buffer instead of the server buffer
- Nicks with +B mode get a robot or alien icon in the nick list rendered via QIcon; stable per session


## v0.24.0 — 2026-06-05

- Multi-line messages (draft/multiline): Shift+Enter inserts a line break, Enter sends; input grows to 4 lines; delivered as an IRCv3 batch on supporting servers, separate PRIVMSGs otherwise
- DCC passive mode (Send File Passive): receiver opens the port, works through sender-side NAT
- In-app update check: Hamburger → Check for Updates fetches the GitHub releases API and reports whether a newer version is available
- Link previews are now opt-in; disabled by default; toggle in Preferences → Interface or set link_previews = true in [privacy]
- TLS credentials (PASS, SASL, NickServ) no longer sent before the user accepts an unknown self-signed certificate; connection aborts on unrecognized cert and only reconnects after user confirms
- Proxy passwords and channel keys stored in the OS keychain; plaintext fallback only when keychain is unavailable
- IRC reaction values HTML-escaped before insertion into the chat view
- sendRaw strips embedded CR/LF from every outgoing line
- Log files and directories created with owner-only permissions (600/700)
- FetchContent dependencies pinned to immutable commit hashes; AppImage packaging tools SHA256-verified
- IRC receive buffer uses QByteArray; UTF-16 conversion deferred to per-line processing
- Channel::removeNick does an in-place index patch instead of a full O(n^2) rebuild
- Chat view scrolls to bottom on channel switch
- Mention detection false positives from empty nick regex fixed


## v0.20.0 — 2026-06-03

- Detachable channel panes: right-click any channel in the sidebar → Open in Pane; up to 4 panes total; each has its own chat view, nick list, topic bar, and input bar
- Layout auto-adjusts: 1 pane = full width, 2 = side by side, 3 = primary left + two stacked right, 4 = 2x2 grid
- Drag any pane header to swap it with another pane
- TLS fingerprint pinning for self-signed servers: first connect shows Accept Once / Pin Certificate / Reject; pinning saves the fingerprint and verifies on every reconnect
- Hamburger, settings gear, and all menu icons replaced with Material Symbols SVGs; colorized to active theme text color at runtime
- MD3-inspired UI pass across all themes: pill buttons, rounder inputs, menus, tooltips, and scrollbar
- All 55 bundled themes copied to ~/.config/uplink/themes/ on first launch; themes deleted by the user stay deleted
- Server messages (MOTD, welcome banner, ISUPPORT) always route to the server buffer
- Right-clicking a channel in the sidebar no longer changes the active primary channel


## v0.16.0 — 2026-05-31

- draft/message-redaction: right-click your own message timestamp → Delete; server sends REDACT; message shows as [message deleted]. Only available when server advertises the CAP
- draft/react: incoming reactions stored per-msgid and rendered inline as emoji + count; right-click a timestamp → React, or /react <emoji>
- account-notify + extended-join: NickServ account name tracked per nick in real time; shown as tooltip in nick list
- WHOX: WHO requests use field mask to pull account names on channel join without separate WHOIS queries
- Monitor: /monitor add/del/list/clear/status; watch list persists to config and is resent on every reconnect
- invite-notify: channel-broadcast invites post a status line in the relevant buffer
- setname: real name changes via SETNAME show a quiet status line
- userhost-in-names: NAMES replies with nick!user@host parsed correctly
- chghost: host changes show a single status line instead of fake QUIT+JOIN noise
- Ignore list: /ignore, /unignore, /ignored; right-click → Ignore/Unignore; persists to config
- Per-channel logging to ~/.config/uplinkirc/logs/<server>/<channel>.log; toggle in Preferences
- /query opens a PM buffer without sending; /ns, /cs, /bs, /ms shorthand for network services
- Right-click Copy now works in the chat view


## v0.13.0 — 2026-05-31

- All outgoing IRC send paths strip \r/\n and validate tokens before writing to the socket; IRCv3 reply tags escaped; CTCP PING payloads sanitised
- openUrl only opens http/https links; other schemes dropped silently
- og:image and direct image fetches now run the same private-address check as the initial page request; LAN/loopback image URLs blocked
- DCC receive: incoming data capped to the advertised file size; sender cannot write past it
- DCC send: files over 4 GiB rejected at send time; ACK comparison uses qint64 to prevent 32-bit wrap
- DCC listen socket binds to the IRC connection's local interface instead of all interfaces
- DCC cancel cleans up sockets and removes partial files
- DCC offers with zero port or non-positive filesize rejected before the accept dialog appears
- Socket aborted before reconnect to avoid stale Qt socket state
- Channel keys persist across config save/load using [[server.channel]] tables; old comma-string format still loads correctly


## v0.11.0 — 2026-05-30

- Nick right-click menu: Take Op, Take Voice, Kick (optional reason), Ban (sets +b nick!*@*), Kick & Ban, Ping (CTCP reply shown in buffer), Copy Nick, Invite
- Right-click a URL with a hidden preview card to show it again; state tracked per-URL without re-fetching


## v0.9.0 — 2026-05-30

- DCC Send File: right-click a nick → Send File; both sides get a live progress dialog with cancel; 60-second connection timeout; standard 4-byte big-endian ACK protocol
- SASL EXTERNAL: authenticate via TLS client certificate; set sasl_external = true, client_cert, and client_key in the server block; RSA and EC keys supported
- AppImage packaging: packaging/build-appimage.sh produces a self-contained AppImage with zsync metadata for in-place updates via appimageupdatetool
- Pushing a v* tag triggers CI builds for Linux (AppImage + tar.gz), Windows (zip), and macOS (DMG); all artifacts uploaded to the GitHub release automatically
- Link preview titles with HTML entities decoded correctly before display


## v0.8.0 — 2026-05-29

- TLS certificate verification enforced; onSslErrors disconnects instead of silently ignoring errors
- PASS, AUTHENTICATE, and NickServ IDENTIFY commands redacted before appearing in logs or any panel
- config.toml written with mode 0600
- Config saves via QSaveFile so a crash during write cannot corrupt the file
- Passwords with quotes, backslashes, or newlines no longer corrupt the TOML config
- Link preview blocks loopback, RFC 1918, link-local, and .local addresses
- CTCP VERSION and PING replies rate-limited to once per nick per 5 seconds; reflected PING payloads capped at 32 bytes
- Inbound buffer capped at 64 KB; lines over 8 KB dropped
- Open batch cap: 8 batches max, 1,000 messages per batch
- Link preview follows http→https redirects and uses a standard browser User-Agent; fetch buffer raised to 32 KB; timeout raised to 6s
- Link preview cards saved per-channel and restored when switching back


## v0.6.0 — 2026-05-29

- Emoji picker: click the emoji button in the input bar to open a searchable grid of ~400 emoji; click any to insert at cursor
- :shortcode: autocomplete: type :smi for a live match list above the input; navigate with Up/Down, confirm with Enter/Tab/click, dismiss with Escape; typing the closing colon substitutes inline; remaining shortcodes resolved on send
- Bot nick icons: nicks with +B mode get a robot or alien icon, assigned by hash and stable per session
- Manage Servers dialog: add, edit, and remove servers without touching the config file; edit pre-fills the form; changes take effect immediately
- Link preview card: URLs in chat fetch og:title and og:image and render a small card below the message
- Hamburger menu icons drawn via QPainter; palette-aware for dark/light themes
- Windows: native windows11 Qt style by default; Consolas as the default font


## v0.3.0 — 2026-05-29

- Clickable URLs: http and https links in chat messages open in the system browser
- Auto-reconnect with exponential backoff on unexpected disconnect: 5s, 10s, 20s, 40s, 60s cap; /quit disables it
- Sidebar right-click menus: right-click a server to Disconnect or Reconnect; right-click a channel to Rejoin or Leave
