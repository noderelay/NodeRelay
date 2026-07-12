# Uplink Roadmap

A fast, secure, IRCv3-featured IRC client built with Qt6 and C++.  
Default network: **irc.linuxdojo.org:6697** — channel **#uplink**

---

## Completed

- [x] Project scaffold — Qt6/C++17, CMake, directory structure
- [x] Config loader — TOML format via toml++, auto-creates `~/.config/uplink/config.toml` on first run
- [x] First-run nick dialog — prompts for nick if config has placeholder `yournick`
- [x] IRC connection — SSL/TLS via QSslSocket, CAP LS 302 negotiation
- [x] IRCv3 message tag parser — full prefix + tag parsing
- [x] CAP negotiation — requests multi-prefix, away-notify, server-time, message-tags, batch, labeled-response
- [x] Full IRC numerics — 001 welcome, 332/353/366 topic+names, MOTD, ISUPPORT, nick-in-use fallback
- [x] IRC commands — JOIN, PART, QUIT, NICK, KICK, MODE, TOPIC, PRIVMSG, NOTICE, CTCP ACTION
- [x] Data model — Message, Channel, ServerSession, SessionModel
- [x] Message buffer cap — 500 messages per channel (RAM-flat on long sessions)
- [x] Nick list — sorted by prefix rank (~&@%+), live updates on JOIN/PART/QUIT/NICK
- [x] Main window UI — toolbar, sidebar (server/channel tree), chat view, nick list panel, topic bar, input bar
- [x] Nick list panel — embedded on the right side of the chat view in a QSplitter
- [x] Topic bar — shows topic text + channel modes, toggleable from hamburger
- [x] Hamburger menu — About, Documentation (stub), App Icon picker, Theme picker, topic/nick/emoji toggles
- [x] Persistent Preferences dialog — hamburger now opens a non-modal QDialog; stays open while browsing themes/toggles; replaces dismiss-on-click QMenu
- [x] Preferences UI rework (v0.9.2) — Manage Servers + Documentation at top; theme as collapsible stay-open list (arrow keys browse, Enter/click applies); App Icon as radio buttons; About removed; Nick Brackets dropdown compact; Enter-triggers-Font-Config bug fixed
- [x] Hamburger restored as dropdown — ☰ opens About Uplink, Documentation, Preferences, Open Config, Reload Config
- [x] Open Config / Reload Config — Open Config opens config.toml in system editor; Reload Config restarts the app to apply all changes (v0.17.1: changed from partial hot-reload to full restart)
- [x] Input commands — /join, /j, /part, /nick, /me, /msg, /quote, /raw, /quit, /ping, /invite, /mode, /op, /deop, /voice, /devoice, /ban, /unban, /clear
- [x] Raw command passthrough — unrecognized /CMD inputs sent directly as raw IRC lines; /REHASH, /SAMODE, /GLOBOPS etc. work without /quote prefix (v0.16.1)
- [x] System tray — minimize to tray on close (× button or Ctrl+W), left-click shows window, right-click menu (Show/Quit)
- [x] Unread badge — tray icon gets red dot on unread messages; green dot for mention/PM when window not focused
- [x] Theme loader — 55 TOML themes, applies as QSS stylesheet, live switching from hamburger
- [x] App icon — 3 switchable styles (Node N, Tower, Hub Spoke); SVG dark / PNG light variants; auto-selects based on active theme palette brightness; tray icon locked to hub-spoke SVG (v0.16.2)
- [x] About dialog — static Node-Relay-Tower.png logo (128×128), version, description (v0.16.2)
- [x] Signal ordering fix — config loads after MainWindow connects signals
- [x] Version baked into binary — `version.h` generated from CMake `PROJECT_VERSION` at build time
- [x] Full documentation — configuration.md, commands.md, faq.md, ircv3.md, keyboard-shortcuts.md (beginner-friendly, real examples)
- [x] How-To guide — docs/howto.html: left-side nav tree, step-by-step from install to tweaks, platform tabs, callout boxes, scroll-spy; linked from GitHub Pages and README
- [x] GitHub repo — public, branch-protected, invite-only contributions; renamed to uplink/UplinkIRC (v0.16.2)
- [x] GitHub Pages landing page — https://uplink.github.io/Uplink/
- [x] README beautification — badges, icon gallery, feature tables, annotated config, commands table, download buttons
- [x] Full rebrand to Uplink — name, binary, config path, icons, docs, CI, GitHub repo (v0.16.2)
- [x] Downloadable icon set on docs site — 8 PNGs (3 styles × dark/light + avatar + banner) with inline preview (v0.16.2)
- [x] Local directory renamed to Uplink; all paths, hooks, and memory updated (v0.16.2 housekeeping)
- [x] Release CI fixed — all three platform builds produce Uplink-v0.16.2-* artifacts (v0.16.2 housekeeping)
- [x] Nick completion — Tab key completes nicks in input bar
- [x] Slash command tab completion — Tab also completes /commands (e.g. /pi → /ping)
- [x] Input history — Up/Down arrow cycles through sent messages
- [x] Colored nicks — consistent hash color per nick in chat and nick list
- [x] In-app Documentation panel — Hamburger → Documentation opens tabbed help viewer
- [x] Documentation search — live search bar in the tab bar (right of Shortcuts); jumps to first match in active tab; re-runs on tab switch
- [x] About dialog centered on app window — overlay QFrame positioned in parent-local coordinates; works on Wayland and X11
- [x] Save theme + icon choice to config.toml (persists across restarts)
- [x] about.png background normalized — logo visible on both light and dark backgrounds
- [x] IRC color code rendering — mIRC bold, italic, underline, strikethrough, reverse, 16 colors (fg+bg)
- [x] Per-widget font size config — Font Config dialog in hamburger; independent sizes for all UI zones; persists to config.toml
- [x] Typing indicator — IRCv3 draft/typing TAGMSG; debounced send + receive; hamburger toggle
- [x] Extended slash commands — /away, /back, /motd, /whois, /topic, /kick, /notice, /version, /ctcp, /sysinfo, /help
- [x] /sysinfo rewritten (v2) — OS/CPU/MEM/GPU/UP format; GPU via vulkaninfo (deviceName + renderer), lspci fallback; uptime from /proc/uptime
- [x] CTCP auto-replies — incoming VERSION and PING answered automatically
- [x] Channel info bar — shows #channel (modes) ServerName; modes from 324 RPL_CHANNELMODEIS
- [x] Sidebar server item clickable — opens server buffer, shows config short name
- [x] Server buffer unread indicator — server name turns purple in sidebar when unread notices arrive (CTCP replies, NickServ, etc.); clears on focus; CTCP replies routed through noticeReceived so they count as unread
- [x] Colored nicks hamburger toggle — was config-only, now in UI
- [x] GitHub Actions CI — builds on every push across Linux, Windows, macOS
- [x] GitHub Actions release workflow — builds platform binaries on tag push, attaches to GitHub releases
- [x] v0.2.0 released
- [x] PM tabs — /msg opens sidebar buffer; incoming PMs land in sender's buffer
- [x] Nick list right-click menu — Message, Send File, Whois, Invite, Give Op, Take Op, Give Voice, Take Voice, Version, Ping (CTCP RTT), Copy Nick, Kick, Ban, Kick & Ban
- [x] Right-click nick in chat view — same context menu as nick list; nicks rendered as nick: anchors in HTML
- [x] Tray icon left-click toggles window visibility
- [x] Unread dot indicator in sidebar — 🔥 for activity, ⚡ yellow bolt for nick mentions; clears on focus
- [x] Sidebar flat list — Halloy-style: no arrows, servers as section headers, connected icon, dock titles removed
- [x] Font Config: Network Name and Typing Indicator size controls
- [x] Info bar always visible with channel, modes, network, user count
- [x] Topic display — separate drop-down area with chat-window background, toggled by Show Topic
- [x] Panel detach/float — sidebar panel could be detached and re-docked (removed in v0.7.12; sidebar is now a fixed embedded panel)
- [x] Topic bar layout — network/user count next to channel label, not far right
- [x] /topic channel-name parsing — `/topic #channel text` correctly separates channel from topic text
- [x] Topic bar background matches input area — info bar visually joins the input row
- [x] Mode string spacing — gap between `(modes)` and `* Network` fixed
- [x] Clickable URLs in topic display — http/https links open in browser
- [x] Toolbar removed — nothing above the info bar; all panels start flush at top
- [x] Minimal dock title bar — sidebar had a slim bar with a ⧉ float button (replaced in v0.7.12 by gear toggle in the info bar)
- [x] Hamburger menu relocated — now in topic bar, left of #channel label
- [x] Status bar text shrunk — 7pt via QSS
- [x] Clickable URLs in chat messages — http/https links in PRIVMSG, actions, and notices open in browser
- [x] Reconnect with exponential backoff — auto-reconnect on unexpected disconnect; 5s→10s→20s→40s→60s; deliberate quit disables it
- [x] Sidebar right-click context menus — server items: Disconnect/Reconnect/Close Server/Move Up/Move Down; channel items: Rejoin/Leave/Close; PM queries: Close Query
- [x] v0.3.0 released
- [x] macOS release CI fixed — MACOSX_BUNDLE property set; .app bundle produced; macdeployqt succeeds; v0.3.0 re-tagged and all three platform builds pass
- [x] Link preview — hover tooltip shows domain + page title; inline card with og:image thumbnail auto-appears below live URL messages
- [x] Link preview fixed for regular URLs — redirect following, proper UA, 32 KB buffer, multi-line title regex
- [x] Link preview redesigned (v0.12.0) — title+domain on top, image below; 360×220 scale; ChatBrowser subclass decodes data: URIs so images actually render; WhatsApp/2 UA fixes YouTube and heavy sites
- [x] Link right-click menu (v0.12.0) — Copy URL / Open URL / Hide Preview; left-click opens browser; double-menu bug fixed by moving handler to QContextMenu event
- [x] Link preview show/hide toggle (v0.12.1) — right-click a hidden-preview URL shows "Show Preview"; card restores from cache without re-fetch; hiddenPreviews QSet tracks state per channel
- [x] Notification icon-only indicators (v0.12.0) — removed color changes on channel names; ⚡/🔥 icons are the sole unread indicators; eliminates layout shift
- [x] Dark banner — uplink-top-banner-dark.svg used in README and About dialog
- [x] `/nick` label update — changing nick now immediately reflects in the input bar nick label
- [x] Direct image URL preview — `.png/.jpg/.jpeg/.gif/.webp` links show a thumbnail card without HTML parsing
- [x] Typing indicator layout — moved from chat-view overlay (covered text) to dedicated layout row; collapses when idle
- [x] FreeBSD download badge in README — links to build-from-source instructions; signals FreeBSD support
- [x] Cross-platform OS characters image on GitHub project page
- [x] TLS certificate verification — certificates are now verified; invalid certs disconnect with an error message instead of being silently accepted
- [x] Credential redaction — `PASS`, `AUTHENTICATE`, and NickServ `IDENTIFY` payloads never appear in the raw log
- [x] Config file hardened — `config.toml` written with owner-only `0600` permissions; save is atomic via `QSaveFile`; all string values properly TOML-escaped
- [x] Link preview LAN protection — private/loopback/link-local addresses blocked from auto-fetch; prevents internal network probing via chat links
- [x] CTCP rate limiting — `VERSION` and `PING` replies limited to once per nick per 5 s; PING echo payload capped at 32 bytes
- [x] Inbound DoS protection — input buffer capped at 64 KB; oversized IRC lines (> 8 KB) dropped; `BATCH` open count (max 8) and message count (max 1 000) bounded
- [x] QTextBrowser block count bounded — chat view kept in sync with message model cap; no unbounded RAM growth on long sessions
- [x] ChatView scrollback cap — `kMaxLines = 5 000`; oldest lines trimmed on append; cumulative height array rebuilt after trim; RAM stays bounded even on busy channels running all day (`chatview.cpp`, `chatview.h`)
- [x] Typing indicator timer leak — `onNickRemoved` now cancels and deletes any active typing timer for the departing nick; `m_typingNicks` entry also removed; `updateTypingLabel()` called immediately so the indicator clears on part/quit/kick instead of waiting up to 6 s (`mainwindow.cpp`)
- [x] Duplicate disconnect signal fixed — `onErrorOccurred` no longer double-emits or double-schedules reconnect
- [x] Link preview image decode safety — `QImageReader` dimension check before decode; images > 4096×4096 rejected; scaled during decode, not after
- [x] Channel preview hash capped — per-channel link preview store evicts oldest at 100 entries

---

## Planned — Near Term

- [x] /help command — lists available commands in chat view
- [x] SASL authentication — PLAIN mechanism (CAP negotiation, AUTHENTICATE, 903/904/906)
- [x] SASL EXTERNAL — certificate-based auth; sasl_external + client_cert + client_key config keys; RSA and EC PEM keys; Server dialog browse buttons
- [x] NickServ IDENTIFY auto — `nickserv_password` in config; sent to NickServ on RPL_WELCOME
- [x] Server error routing — 482 and other server errors shown in active channel buffer, not just (server)
- [x] Multiple servers — Manage Servers dialog: add, edit, remove, reorder (▲/▼) with live connect/disconnect
- [x] /connect and /server commands — connect to any server on the fly by host[:port] with auto SSL detection
- [x] Reconnect logic — auto-reconnect with backoff on disconnect
- [x] Connection status indicator — signal bars widget in topic bar; latency-based bar count (4=<50ms … 1=>300ms); blue flash=connecting, red flash=disconnected
- [x] Mention notifications — ⚡ yellow bolt sidebar indicator when nick is mentioned in inactive channel; 🔥 for general activity; self-nick highlighted red bold in chat
- [x] Desktop notifications — green dot on tray icon on mention/PM when window not focused; clears on window focus; toggle in Preferences

---

## Planned — Medium Term

- [x] Hanging indent — wrapped message lines align past the timestamp+nick column; per-message pixel-accurate alignment via QTextLayout prefix measurement (2026-06-13 rewrite; original used QTextBlockFormat)
- [x] Message search — Ctrl+F opens search bar; Enter/Shift+Enter next/prev with wrap; Escape closes
- [x] Full-history log search — Ctrl+Shift+F scans the current buffer's on-disk log on a worker thread (newest 1000 matches, cancellable), substring or regex, newest-first; notice when logging is off (2026-07-06)
- [x] Full-history search v2: all buffers — "All buffers" toggle in the Ctrl+Shift+F window scans every log under `logs/` on the worker thread; results grouped per buffer (bold header with match count, newest 200 matches each); double-click a result to jump to that buffer (`SessionModel::resolveLogBuffer()` maps sanitized log path components back to the live buffer) (2026-07-11)
- [x] Hamburger Close Menu button — explicit dismiss action at bottom of ☰ menu
- [x] Logging — per-channel log files at `~/.config/uplink/logs/<server>/<channel>.log`; `log_messages` config key; Preferences toggle (v0.15.0)
- [x] Ignore list — client-side suppress of incoming PRIVMSGs/NOTICEs/ACTIONs from specific nicks; right-click → Ignore/Unignore; /ignore /unignore /ignored commands; persists as [ignore] nicks in config.toml (v0.14.0)
- [x] msgid — IRCv3 unique message IDs stored on every received message; full signal chain (messageReceived, noticeReceived, actionReceived, batch delivery); prerequisite for reply threading, reactions, and redaction
- [x] echo-message — server echoes sent messages back; duplicate local echo suppressed; PM routing corrected for self-echoes; also fixes ZNC self-message PM routing; typing TAGMSG and DCC CTCP offers filtered by `msg.nick != m_nick` to prevent self-echo of typing indicators and incoming DCC dialogs (v0.25.0)
- [x] draft/reply — right-click any message → Reply; reply bar above input; ↩ origNick shown on received replies; @+draft/reply= tag sent on outgoing messages
- [x] draft/message-redaction — right-click own message timestamp → Delete; REDACT sent; received redactions render as [message deleted] (v0.16.0)
- [x] account-notify + extended-join — track NickServ account per nick; populate from JOIN + ACCOUNT commands; account tooltip in nick list (v0.16.0)
- [x] account-tag — per-message account= tag; account field on Message struct; hover tooltip on nick in chat view; four sources: account-tag, account-notify, extended-join, WHOX (v0.16.3)
- [x] Monitor — /monitor add|del|list|clear|status; watch list persisted in config.toml; MONITOR + sent on connect; 730/731 online/offline posted to server buffer (v0.16.0)
- [x] chghost — CAP negotiated; CHGHOST parsed; quiet "nick changed host" status line in shared channels; no fake QUIT+JOIN noise (v0.15.0)
- [x] invite-notify — CAP negotiated; channel invite broadcasts post to channel buffer; direct invites post to server buffer (v0.16.0)
- [x] setname — CAP negotiated; SETNAME parsed; "nick changed their realname" posted to shared channels (v0.16.0); /setname send side added (v0.25.2)
- [x] WHOX — WHO <channel> %cnfa,42; 354 RPL_WHOSPCRPL parsed; account names populated on join; bot flags preserved (v0.16.0); parser corrected for Ergo's 5-field format (no token) — was silently dropping all WHO replies (v0.25.0)
- [x] userhost-in-names — CAP negotiated; !user@host stripped from NAMES entries before display (v0.16.0)
- [x] STS (Strict Transport Security) — auto-upgrade plaintext connections to TLS; policy cached to ~/.config/uplink/sts.ini; stsstore.h/cpp; m_stsUpgrade flag; downgrade prevention (v0.16.3)
- [x] netsplit/netjoin batch types — collapse netsplit noise into a single folded entry (v0.16.4)
- [x] Standard Replies — structured error/warning/note messages from server (v0.16.5)
- [x] UTF8ONLY — detect server UTF-8-only signal, enforce encoding, warn on violations (v0.16.8)
- [x] URL detection + click to open — http/https links in chat open in browser (v0.3.0)
- [x] Link preview persistence — cards survive channel switches (stored in Channel struct)
- [x] Link preview entity decoding — QTextDocument decodes &amp; &#39; &lt; etc. in og:title and &lt;title&gt; fallback paths
- [x] DCC Send File — right-click nick → Send File; DccSend TCP listener + ACK protocol; DccReceive connect + ACK send; progress dialogs; 60s/30s timeouts. NAT limitation: local IP advertised; works on LAN, blocked by NAT on WAN.
- [x] DCC passive / NAT traversal — passive DCC (port=0 + token); DccSend::initPassive/connectOut + DccReceive::listenPassive; new signals dccPassiveOfferReceived / dccPassiveSendReply routed through SessionModel; "Send File (Passive)" in nick right-click menu
- [x] Detachable channel panes (v0.17.0) — right-click sidebar channel → Open in Pane; each pane has its own chat view, nick list, per-pane topic bar (toggle), and input bar; auto layout (2=side-by-side, 3=primary+two stacked, 4=2×2 grid); max 4 total; primary column header + close button when panes are open
- [x] Pane drag-to-rearrange — drag header bar to swap with another pane (v0.18.0); native QDrag/QMimeData under the hood (2026-07-11 — the original grabMouse()-based mechanism silently failed to track drags under some Wayland/KDE sessions). Dropping on the primary view uses a reflow: the dragged pane stays put, primary joins it, and the pane it displaces is promoted to the vacated lone column, instead of a blind position swap.
- [x] Pane layout persistence — save/restore pane arrangement across sessions
- [x] Horizontal pane splits — `pane_stack_rows` config option (Preferences → Interface → Stack Panes in Rows) transposes the auto layout: panes stack in horizontal rows instead of columns. Same shapes for 1-4 panes, rotated 90°.
- [x] Pane/pop-out review hardening (2026-07-11) — full code review of the pane and pop-out layer; every finding fixed:
  - Drop-target highlight is now a frame around the entire pane/primary (new `dropframe.h` overlay) instead of a header tint; the overlay carries its own `background: transparent` because the theme's generic QWidget rule would otherwise paint it opaque
  - Fix: dragging the pane stacked alongside the primary onto the primary was a silent no-op (siblingSlot == primarySlot self-swap); now a plain swap
  - Fix: Alt+Left/Right was dead after the sidebar/primary decoupling; now cycles keyboard focus across pane inputs + primary input in layout order
  - Fix: channels open in a pane/window accumulated unread badges forever (they can never become active to clear them) — new `SessionModel::markRead()`; badges never accrue while visible
  - Fix: popped-out (previously docked) panes rendered topic/typing/input strips in the system palette color — reparent misses the QSS background repolish; forced subtree unpolish/polish in floatPane + `#paneWindow` buffer-bg fallback stylesheet on the window
  - Link previews now fetch for channels living only in a pane/window (shared `appendPreviewCards()`; PreviewController already dedupes by URL)
  - Hidden primary column no longer resurrects on pane rearrange/open/close
  - Panes on other servers highlight that server's nick, not the active server's (`selfNickReFor()`)
  - Ctrl+F routes to the focused docked pane's search bar; popped-out windows get their own window-scoped Ctrl+F
  - Floating pane windows no longer offer header drags or accept pane drops (both were dead ends)
  - Pop-out window geometry persisted per channel in QSettings `paneWinGeom/` (was fixed 820×620 every time)
- [x] mactahoe26 + mactahoe26-light themes — macOS Tahoe system-color themes (2026-07-11); theme count 295 → 297
- [x] Message reactions — IRCv3 draft/react; receive + store per-msgid; render inline below messages; right-click timestamp → React; /react command; IrcClient::sendReact (v0.15.0)
- [x] Multiline messages — IRCv3 draft/multiline; compose and render multi-line message blocks
- [x] /list command — sends LIST to server; RPL_LIST (322) and RPL_LISTEND (323) displayed in server buffer with user count and topic (v0.25.2)
- [x] /list dialog — QDialog channel browser; streams RPL_LIST (322) results into a sortable QTableWidget (channel, users, topic); live filter box; double-click to join; batched UI inserts to stay responsive on large networks; guard against concurrent LIST requests
- [x] /whowas — RPL_WHOWASUSER (314) and RPL_ENDOFWHOWAS (369); results in active buffer alongside WHOIS (v0.25.2)
- [x] /stats — routes 211–219 and 241–244 to active buffer; /stats u=uptime, o=opers, m=commands (v0.25.2)
- [x] /time (server) — bare /time sends IRC TIME command; RPL_TIME (391) handled; /time <nick> CTCP already existed (v0.25.2)
- [x] Soju bouncer network selection — SASL AUTHENTICATE payload now sends user/network when bouncerNetwork is set; LISTNETWORKS END handled; initial network list shown as formatted summary (v0.25.2)
- [x] IRCv3 WebSocket transport — connect to servers over wss:// in addition to plain TCP+TLS
- [x] User metadata — IRCv3 draft/metadata-2: display-name and avatar keys; avatar image rendered inline in nick list tooltip (fetched async, cached, no upscale); local file paths supported; Preferences Profile section + /displayname /avatar commands; auto-publish on connect; avatar updates in Preferences now reflected immediately without reconnect
- [x] Per-server quit message — `quit_message` in server block; used by /quit with no arg and on normal disconnect; GUI field in Edit Server
- [x] Per-server away message — `away_message` in server block; used by /away with no arg; GUI field in Edit Server; /back always clears
- [x] Disabled server flag — `disabled = true` in server block keeps it in config but skips startup; GUI checkbox in Edit Server; prevents config-save data loss vs commented-out blocks
- [x] Virtual scrolling blank view fix — replaced `processEvents` with `QTimer::singleShot(0,…)` in `loadOlderMessages`; fixes blank unscrollable chat on busy channels caused by stale `sb->maximum()` after `clear()`
- [x] cap-notify — server can notify client of capability changes mid-session; CAP NEW triggers REQ for wanted caps, CAP DEL removes from active set; m_registered flag prevents spurious CAP END on mid-session ACK (v0.25.4)
- [x] draft/chathistory compatibility — Ergo IRCd uses draft cap name; Uplink now requests both names and handles either on history delivery and batch recognition (v0.25.5)

---

## Planned — Polish + Distribution

- [x] Material Design sidebar indicators — mi-bolt.svg (yellow) for mentions, mi-forum.svg for unread; drawn after channel name by SidebarDelegate; text position stable (icon changed from lightbulb_2 to bolt in v0.25.29)
- [x] Server globe icon — Material Symbols "public" replaces hand-drawn dot; stored in UserRole+2 so SidebarDelegate renders it; turns red on server status pane unread
- [x] METADATA SUBSCRIBE removed — Ergo does not support the subcommand; metadata is pushed automatically; fixes [FAIL] SUBCOMMAND_INVALID on ergo.chat
- [x] Mention detection fix — emit selfNickChanged on RPL_WELCOME so mentionRe is built on connect, not only on mid-session NICK change
- [x] Send button — paper-plane SVG in input bar right of emoji button; calls onInputSubmit
- [x] Send button floats inside input area — child widget of input, right-edge centred, repositions on resize; QTextFrameFormat right margin keeps text clear (2026-06-11)
- [x] Tab completion cycling fixed — repeated Tab now cycles all alphabetical matches; stored m_tabWordStart prevents empty-prefix bail-out after first completion (2026-06-11)
- [x] GitHub Pages site redesigned — Gruvbox Light + IBM Plex Mono; Quick Start how-to section embedded on landing page; dark/light mode toggle on both index.html and howto.html (2026-06-11)
- [x] Fade scrollbars — appear on scroll, fade out after 3.5 s; applied to chat view, nick list, sidebar, all channel panes (2026-06-11)
- [x] TAGMSG cap guard — sendTyping/sendReact no longer fire on servers that haven't ACKed message-tags; fixes Unknown command spam on Undernet (2026-06-11)
- [x] FadeScrollBar hardened — unconditional enterEvent wake; sliderReleased connected; timer polling loop removed; leaveEvent guards on opacity>=kVisible; sendTyping cap corrected to draft/typing; sendReact target validated (2026-06-11)
- [x] Long-session memory/fd leaks fixed — log fd leak on closeBuffer; nickMeta unbounded growth; ctcpTimestamps clear-all on overflow; channelpane doc cap misaligned (2026-06-11)
- [x] v0.25.16 released (2026-06-11)
- [x] FadeScrollBar post-release regression fixed — reverted to QGraphicsOpacityEffect (QSS painter path bypasses QPainter::setOpacity; effect composites after all painting); reverted to valueChanged (actionTriggered never fires for wheel events on parent QAbstractScrollArea) (2026-06-11)
- [x] AppImage self-integrates on first run — writes .desktop entry + copies icon to ~/.local/share/; Uplink appears in launchers without manual setup (2026-06-11)
- [x] v0.25.17 released (2026-06-11)
- [x] WHOX ISUPPORT guard — WHO #channel %cnfa,42 now gated behind WHOX in RPL_ISUPPORT; plain WHO fallback for servers that don't support it (e.g. Rizon); fixes spurious "Unknown command" on connect (v0.25.18, 2026-06-12)
- [x] v0.25.18 released (2026-06-12)
- [x] Send button disabled when input empty — re-enables on non-whitespace text; matches onInputSubmit guard (2026-06-12)
- [x] NickDelegate::sizeHint hotspot fixed — returns fixed QSize(width, 16) directly; eliminates ~17K QTextEngine::itemize calls per session found via heaptrack audit (2026-06-12)
- [x] Virtual scrolling — refreshChatView renders last 150 messages (kRenderWindow); grey divider shows older count; scroll-to-top loads 50 more (kRenderChunk) with position restore; ~3x faster channel switches (2026-06-12)
- [x] v0.25.19 released (2026-06-12)
- [x] RPL_ENDOFWHO (315) suppressed from server window — eliminates WHO list spam on busy networks like Libera.chat (2026-06-12)
- [x] Preferences toggle for send button — "Show Send Button" in Preferences; show_send_button in [ui]; default true (2026-06-13)
- [x] Nick hover tooltips in chat — hovering any nick link in chat now shows display name + account + 32×32 avatar, same as nick list; primary view and all split panes (2026-06-13)
- [x] Quit Uplink — hamburger "Exit" renamed, wired to QCoreApplication::quit() (2026-06-13)
- [x] Tooltip avatar capped at 32×32 px — was showing full-size cached images (2026-06-13)
- [x] Window state persistence — sidebar width and nick panel width saved via QSettings on quit, restored on launch; sidebar is drag-resizable; geometry clamped to available screen on startup (v0.25.1); settings path corrected to ~/.config/uplink/uplink.conf (v0.25.1)
- [x] Nick panel width persistence — QSplitter position saved via QSettings on quit, restored on launch
- [x] Config editor UI — Manage Servers dialog covers server-level editing
- [x] Emoji picker — searchable popup grid with :shortcode: autocomplete and auto-substitution; expanded to 1,906 emojis from Unicode 16.0 + gemoji shortcodes (v0.25.47); generator script at scripts/generate_emojidata.py
- [x] Bot nick indicators — SVG robot/alien icon drawn inline to the right of +B nicks in the user list; randomly assigned per nick per session, stable for the connection duration; colorized to theme accent (v0.25.0; replaces emoji font approach which was unreliable across fontconfig configurations)
- [x] Configurable nick brackets — `nick_brackets` in `[ui]`; `"<>"` `"[]"` `"::::"` `""` supported; also configurable live from Hamburger → Nick Brackets
- [x] Autojoin regression fix — editing a server in the GUI no longer wipes auto-join channels; Auto-join field added to Add/Edit Server dialog
- [x] Autojoin Windows fix — config loading now uses Qt file I/O instead of toml::parse_file; paths with non-ASCII characters load correctly on Windows
- [x] Simplified channel config — `channels = "#uplink, #linux"` replaces `[[server.channels]]` array-of-tables; Reload Config syncs changes live via SessionModel::syncServers()
- [x] Channel key persistence — channels saved as `[[server.channel]]` tables with `name` + `key`; backward-compatible load of old comma-string format (v0.13.0)
- [x] Close/Close Query in sidebar — right-clicking a channel shows Close (PARTs + removes buffer); right-clicking a PM query shows Close Query (removes buffer)
- [x] Channel focus on join — joining a channel now always switches focus to it
- [x] Nick panel redesign — replaced detachable QDockWidget with embedded panel in QSplitter; gear button (⚙) in header animates a full spin before toggling the user list; gear and user count remain visible when collapsed; panel background matches chat buffer color across all themes
- [x] Sidebar gear toggle — ⚙ button in the topic bar collapses the server/channel list; hamburger and gear stay pinned in the topic bar at all times (kBtnZoneMinW=48px floor); only the list collapses beneath them
- [x] Native Windows style — windows11 Qt style by default; no alien dark theme on fresh installs
- [x] FreeBSD port skeleton — `packaging/freebsd/`: Makefile, pkg-descr, pkg-plist; uses GH source tarball; needs `make makesum` + qt6-keychain port verification before submission
- [x] AppImage packaging for Linux — build-appimage.sh; linuxdeploy + Qt plugin; zsync metadata embedded; release.yml uploads .AppImage + .AppImage.zsync; plugin strip patched for .relr.dyn (v0.15.0)
- [x] Auto-bump release docs — `scripts/sync-site.sh` replaces all version strings in README.md and docs/index.html; `update-docs` job in release.yml runs it automatically on every tag push; also run manually at session close
- [x] Service shortcuts — /ns /cs /bs /ms aliases for NickServ/ChanServ/BotServ/MemoServ; /query opens PM without sending; /oper for IRC operator login (v0.15.0)
- [x] Right-click copy in chat view — selecting text and right-clicking shows Copy (v0.14.0)
- [x] Theme-aware icons — hamburger menu and gear buttons use m_theme.text; link preview card hardcoded dark (v0.14.0)
- [x] In-app update check UI — "Check for Updates" in hamburger menu; fetches GitHub releases API, parses tag_name, compares UPLINK_VERSION_MAJOR/MINOR/PATCH; shows "Update Available" or "Up to Date" message box
- [x] Auto-update — "Check for Updates" now downloads and installs the release automatically: AppImage replaces in-place + relaunches (no sudo); Windows saves ZIP to Downloads; macOS opens DMG in Finder; source/FreeBSD show informational message (v0.25.54)
- [x] `/sysinfo` — removed confirmation dialog; posts system info directly like any other send command (v0.25.54)
- [x] `compile_commands.json` — CMake now exports compilation database; symlinked to project root so clangd gets accurate include paths and diagnostics (v0.25.54)
- [x] Event message visual polish — join/part/quit/nick lines render at 82% font size; part/quit/kick color softened to #e06b6b for dark backgrounds (v0.18.2)
- [x] QSS visual polish overhaul — rounded inputs with focus rings, themed checkboxes/radio/tabs/tooltips, floating scrollbars, pill-shaped sidebar selection, menu border-radius and separators (v0.19.0)
- [x] Sidebar fitted pill highlight — custom SidebarDelegate draws rounded rect sized to text width; no full-row highlight; hover shift eliminated; selected text always readable (v0.19.0)
- [x] Nick list pill highlight — NickDelegate mirrors SidebarDelegate; selected/hovered nicks get pill-shaped background sized to text width; QSS transparent-background fix so delegate shows through; vPad=1 radius=6 for proper oblong pill (v0.25.1)
- [x] Topic toggle button theme integration — both primary panel and channel pane topic buttons use #topicToggle QSS rule; removed palette(mid)/palette(highlight) inline stylesheets; always visible in single-window mode (v0.19.0)
- [x] Topic button speech bubble icon — custom QPainter icon replaces text label; muted when collapsed, accent when open; ChannelPane::setTopicIcon() API; updates on theme change (v0.19.1)
- [x] Floating rounded input bar — separator line removed; bar blends into chat area via bufferBg; QLineEdit gets border-radius pill shape (v0.19.1)
- [x] Rounded chat panel with padding — RoundedPane widget clips all chat content (panes splitter + children) to a 10 px rounded rect via setMask(); 8 px right/bottom padding shows sidebarBg for contrast; sidebar toggle balances left margin to match (v0.19.2)
- [x] Sidebar unified with chat card — server/channel sidebar moved inside the RoundedPane floating card alongside the chat area and nick list; sidebar background changed from sidebarBg to bufferBg so all three columns share one seamless surface; inputBar spans the full card width at the bottom, anchoring all columns; 8 px left margin restored on both expanded and collapsed states (v0.25.25, 2026-06-13)
- [x] MD3-inspired UI pass — pill buttons (20px radius via PillButton QPainter subclass), rounder inputs/menus/tooltips, Material Symbols SVG icons for hamburger/gear/all menu items, sidebar pill highlight padding fixed, row heights via sizeHint delegates (v0.20.0)
- [x] Self-signed cert fingerprint pinning — per-server accept/reject/pin dialog on first connect; SHA-256 fingerprint saved to config; mismatch disconnects with warning; IrcClient::abort() for clean reject (v0.20.0)
- [x] Unified channel header — standalone top bar removed; hamburger (☰) and sidebar toggle (⊞) move into the primary header row inline with #channel (+modes); floating card gains symmetric 8 px margins on all four sides (v0.25.27, 2026-06-14)
- [x] Server name in nick panel header — "* NetworkName" moved from top bar into the nick panel header row in v0.25.27; removed entirely in v0.25.29 (header is cleaner without it; network is identifiable from the sidebar tree)
- [x] Sidebar tree alignment — dynamic sidebarHeader spacer at top of sidebar panel keeps server tree items vertically aligned with the nick panel header; spacer height updated reactively via eventFilter on topicDisplay resize/show/hide (v0.25.27, 2026-06-14)
- [x] Topic + input anchored to chatSection — channel header, topic display, and input bar are all inside chatSection (right of the draggable sidebar divider); they no longer span the full card width including the sidebar column (v0.25.27, 2026-06-14)
- [x] IRC mask in join/part/quit messages — user and host fields threaded through userJoined/userParted/userQuit signals; membership events display as "nick (~user@host) has joined/left the channel" (v0.25.28, 2026-06-14)
- [x] soju FAIL noise suppressed — METADATA KEY_INVALID, BOUNCER UNKNOWN_COMMAND, and ACCOUNT_REQUIRED FAIL replies silently dropped instead of appearing in channel buffers (v0.25.28, 2026-06-14)
- [x] soju no-implicit-names support — explicit NAMES request on self-join when soju.im/no-implicit-names is active so the nick list populates correctly (v0.25.28, 2026-06-14)
- [x] Reconnect credential bug fixed — "Reconnect" context menu now calls cl->reconnect() to reuse the already-resolved in-memory SASL password instead of re-passing the raw "<keychain>" sentinel to connectToServer() (v0.25.28, 2026-06-14)
- [x] soju documentation rewritten — configuration.md soju section now covers SASL-based setup with single and multi-network examples, Option A/B for network selection, and a BouncerServ tip (v0.25.28, 2026-06-14)
- [x] Bolt mention icon — mi-bolt.svg replaces mi-lightbulb-2.svg for the nick mention indicator; shown in yellow (#FFD700) in the sidebar; MenuIcons::mention() updated (v0.25.29, 2026-06-14)
- [x] Nick panel server label removed — "* NetworkName" label cleared from the nick panel header in all channel/query views; header only shows the toggle button, groups icon, and user count (v0.25.29, 2026-06-14)
- [x] Nick panel user count repositioned — user count sits directly right of the groups icon with addSpacing(4); consistent visual gap between all three header elements (v0.25.29, 2026-06-14)
- [x] Single-word PRIVMSG/NOTICE text fix — ircclient.cpp reads msg.params.last() when msg.trailing is empty; fixes invisible body text and missed mention detection for messages where the server omits the colon prefix on a single-word trailing parameter (v0.25.29, 2026-06-14)
- [x] Body text foreground always explicit — chatrenderer.cpp Privmsg and Action handlers unconditionally set QTextCharFormat foreground (themeText or #cccccc fallback); prevents invisible text when addSelfNickHighlight overlaps a body segment that had no foreground set (v0.25.29, 2026-06-14)
- [x] Mention detection rewritten — postMessage() uses msg.text.contains(sess->nick, Qt::CaseInsensitive) instead of QRegularExpression; simpler and consistent with the tray notification path (v0.25.29, 2026-06-14)
- [x] Lag meter relocated to sidebar header — SignalBars widget moved from orphaned state into shBox (sidebar header HBoxLayout), right-aligned after the stretch, vertically centered; always visible alongside ☰ and ⚙; updates on server switch, latency ping, connect/disconnect/reconnect (v0.25.31, 2026-06-15)
- [x] Lag meter tooltip — hovering the signal bars shows exact ms latency ("42 ms") or connection state ("Connecting…" / "Reconnecting…" / "Disconnected") (v0.25.31, 2026-06-15)
- [x] Host icon — connected server icon changed from globe (mi-public.svg) to host (mi-host.svg) in MenuIcons::connectedServer() (v0.25.31, 2026-06-15)
- [x] WALLOPS support — WALLOPS command parsed in IrcClient; wallopsReceived signal added; posts to server buffer as MessageType::Wallops (amber #e09030, "[nick] text" format) (v0.25.31, 2026-06-15)
- [x] Server buffer message color differentiation — MessageType::Reply (steel blue #6090c0) for numeric command responses via onContextualMessage (/whois, /stats, /time); MessageType::Server (gray) for status lines; MessageType::Wallops (amber) for operator broadcasts; MessageType::Error (red) unchanged (v0.25.31, 2026-06-15)
- [x] Unread count badges in sidebar — SidebarDelegate renders Qt::UserRole+3 (int count) as small bold 86% font after the bolt/forum indicator icon; onUnreadChanged stores count on item (v0.25.31, 2026-06-15)
- [x] Preferences toggle: Show Unread Message Counts — global on/off for sidebar count badges; persisted as show_unread_counts in [ui]; SidebarDelegate::setShowCounts(); clears stored counts immediately on disable (v0.25.31, 2026-06-15)
- [x] Server and PM buffers hide irrelevant UI — switchToChannel() detects buffer type; server buffers and PM/query buffers hide nick panel and topic button; both restore on switch to a channel (v0.25.31, 2026-06-15)
- [x] Search button toggle — clicking 🔍 while search bar is open closes it and clears the query; was open-only before (v0.25.31, 2026-06-15)
- [x] /list on large networks fixed — kMaxPendingBuffer check moved to after the line-processing loop; large /list responses (thousands of 322 replies) no longer disconnect; only a genuinely unterminated partial line > 64 KB triggers the error (v0.25.31, 2026-06-15)
- [x] New messages separator — "── N new messages ──" divider before first unread on channel switch; view scrolls to it automatically; clears on focus (v0.25.33, 2026-06-15)
- [x] Scroll position memory — non-bottom positions saved per-channel, restored on return; separator takes priority (v0.25.33, 2026-06-15)
- [x] mIRC formatting input — Ctrl+B/I/U/S/O apply formatting visually in the input box (QTextCharFormat); IRC control codes generated from document state at send time; formats stack (Ctrl+B + Ctrl+U = bold+underline); Ctrl+O resets all (v0.25.33/v0.25.39, 2026-06-15/2026-06-16)
- [x] Nick list filter — always-visible filter input above nick list; startsWith match, clears on channel switch (v0.25.33, 2026-06-15)
- [x] SVG icon cache — fromSvg() caches rendered QPixmaps; eliminates 18.89 MB peak / 5,986 allocs per heaptrack session (v0.25.34, 2026-06-15)
- [x] Nick list setUniformItemSizes(true) — eliminates 17,628 sizeHint calls per layout pass (v0.25.34, 2026-06-15)
- [x] QLatin1String in makeNickItem() — eliminates ~11,400 temporary QString::fromUtf8 allocs (v0.25.34, 2026-06-15)
- [x] WHO skip with extended-join — onUserJoined() skips redundant WHO when extended-join CAP is active; eliminates 400–700 B/s upload on busy channels (v0.25.35, 2026-06-15)
- [x] 306 built-in themes — 251 base16 themes added alongside original 55, named with -base16 suffix (v0.25.36, 2026-06-15)
- [x] `[buffer] separator` theme key — controls divider line color; falls back to border color (v0.25.36, 2026-06-15)
- [x] Topic font-size QSS fix — setStyleSheet("font-size: Xpt") on topic label so Qt6 QSS engine doesn't override setFont() (v0.25.37, 2026-06-15)
- [x] macOS Preferences padding — QCheckBox/QRadioButton padding: 3px 0 to stop smooshed appearance on native controls (v0.25.37, 2026-06-15)
- [x] Dedicated Manage Servers button in sidebar header — domain_add SVG icon button next to ⚙; MenuIcons::manageServers/hamburger/gear added (v0.25.38, 2026-06-16)
- [x] Right-click anywhere on message for React/Reply — contextMenuEvent falls back to ChatLine::id so any click on the message body triggers the menu, not just the narrow timestamp (v0.25.38, 2026-06-16)
- [x] Local reaction echo — sendReact() immediately applies reaction to ch->reactions before server echo; emoji row appears under message on sender's own client (v0.25.38, 2026-06-16)
- [x] Icon sizing unified — all sidebar header and panel toolbar buttons explicitly sized; hamburger/gear/domain-add routed through DPR-aware MenuIcons::fromSvg (v0.25.38, 2026-06-16)
- [x] nickDock font platform split — defaults to 9 pt on Linux/FreeBSD, 13 pt on macOS (Retina); config key font_nick_dock (v0.25.38, 2026-06-16)
- [x] /nick label fix — selfNickChanged was never emitted because m_nick was set preemptively in setNick(); NICK handler now also checks newNick == m_nick (v0.25.38+, 2026-06-16)
- [x] Unread count badge theme color — SidebarDelegate::m_unreadColor set from sidebarUnread theme key; full opacity instead of 60%-alpha text color (v0.25.38+, 2026-06-16)
- [x] Reaction cross-client fix — sendReact returns bool; local echo is conditional on actual send; !known guard removed from onReactReceived so reactions for messages outside chathistory window are no longer silently dropped (v0.25.38+, 2026-06-16)
- [x] Format indicator — floating B I U S pill label appears at bottom-left of input box while any formatting is active; hides on send and when all formats cleared; reads currentCharFormat() so it reflects cursor position (v0.25.39, 2026-06-16)
- [x] Keychain bundle + \x1F separator — all four server credentials stored as a single OS keychain item (serverName:bundle) separated by ASCII unit separator \x1F; reduces macOS prompts from up to 4 per server to 1; fixes KWallet NUL-truncation bug that caused empty reads on SASL-only servers (v0.25.39, 2026-06-16)
- [x] Search icon 24×24 — search button icon size increased from 20×20 to 24×24 (v0.25.39, 2026-06-16)
- [x] Close channel refreshes UI — onChannelRemoved calls onSidebarSelectionChanged() so info bar and nick list update immediately to the auto-selected channel (v0.25.40, 2026-06-16)
- [x] Sidebar header icon swap — Preferences → manage_accounts; Manage Servers → add_link at 32×32/28×28 (v0.25.40, 2026-06-16)
- [x] Link preview image limit raised — 200 KB → 2 MB; retina screenshots no longer truncated and fail to decode (v0.25.44, 2026-06-18)
- [x] Link preview partial image rejection — truncated downloads (timeout/size cap) emit title-only card instead of displaying broken grey-filled thumbnail; image timeout 6s → 15s (v0.25.44, 2026-06-18)
- [x] Config hot-reload — QFileSystemWatcher on config.toml; server additions/removals from external text editors picked up automatically without restarting (v0.25.44, 2026-06-18)
- [x] Font zoom — Ctrl+mousewheel/Ctrl+± per UI region (chat, nick list, sidebar, input, topic, nick panel); server vs channel independent in sidebar; 0.5pt steps; FontSizes int→double; Font dialog QDoubleSpinBox (v0.25.44, 2026-06-18)
- [x] Clean config formatting — config writer and all docs stripped of column-alignment padding; plain key = value (v0.25.44, 2026-06-18)
- [x] Preferences dialog redesigned (v0.25.45) — Konversation-style flat nav list + stacked content pages; 6 pages: Appearance, Chat Window, Interface, Notifications, Logging, Profile
- [x] 15 app icon variants (v0.25.45) — visual grid picker in Preferences; flat, colorful, gruvbox, original styles; live update to window, tray, and KDE Plasma taskbar; old dark/light config values auto-migrated
- [x] Manage Servers dialog redesigned (v0.25.45) — integrated two-panel layout; server list on left, inline editing form on right; no more separate Edit Server popup
- [x] Website icon rotation (v0.25.45) — docs pages randomly rotate through 15 app icons on each page load

- [x] Keyboard navigation — Alt+Up/Down cycles channels, Alt+Left/Right cycles panes; works from any widget (v0.25.49, 2026-06-20)
- [x] Quick channel switcher (Ctrl+K) — floating popup with live filtering to jump to any joined channel (v0.25.49, 2026-06-20)
- [x] Scrollback history loading — CHATHISTORY BEFORE on scroll-to-top; infinite scrollback until server exhausted (v0.25.49, 2026-06-20)
- [x] Jump-to-bottom button — floating fade-in/out button when scrolled up; click to return to live chat (v0.25.49, 2026-06-20)
- [x] Scroll-up lock on busy channels — sticky m_userScrolledAway flag prevents auto-scroll snap-back; isAtBottom() respects the flag (v0.25.49, 2026-06-20)
- [x] mIRC color codes in topic bar — linkifyTopic() now runs through ircToHtml() (v0.25.49, 2026-06-20)
- [x] Keyboard shortcuts documentation rewritten — quick reference table, navigation section, macOS notes (v0.25.49, 2026-06-20)
- [x] Static analysis cleanup — duplicate /caps handler removed; compiler warnings fixed (old-style casts, narrowing, platform ifdef) (v0.25.49, 2026-06-20)
- [x] Font zoom row height fix — NickDelegate/SidebarDelegate sizeHint derived from fontMetrics instead of hardcoded pixels; topic bar header mapped in fontFieldForWidget for live Ctrl+scroll (v0.25.50, 2026-06-20)
- [x] CONTRIBUTING.md — contribution guidelines: open issue first, match code style, project goals, testing requirements, what won't be merged (2026-06-21)
- [x] MainWindow refactor — extracted context menus (contextmenus.cpp) and nick panel (nickpanel.cpp); moved isCondensable and buildReactionLine to chatrenderer.h; mainwindow.cpp 5281→4447 lines (2026-06-23)
- [x] Config and ignore type unit tests — tst_config.cpp (12 tests), tst_ignoretypes.cpp (6 tests); covers load/save round-trip, defaults, backward compat, flags (2026-06-23)
- [x] Kinetic touch scrolling — QScroller on ChatView, sidebar, nick list, channel panes; flick-to-scroll with momentum for touchscreen/tablet use (2026-06-23)
- [x] User script bindings — Preferences → Scripts tab to link external executables to custom slash commands; QProcess worker thread, env vars, 10s timeout, 5-line output cap; cannot shadow built-ins (2026-06-24)
- [x] Bundled scripts — /music (MPRIS2/nowplaying-cli), /weather (wttr.in), /uptime, /roll (tabletop dice); auto-installed on first launch to ~/.config/uplink/scripts/; Restore Defaults button (2026-06-24); fixed missing scripts on existing configs (2026-06-26)
- [x] Windows script support — .sh scripts launched via bash.exe on Windows; /music queries GSMTC via PowerShell; /uptime queries WMI via PowerShell; fixed Linux /music D-Bus metadata parsing (2026-06-24)

---

## Planned — Future

- [ ] Accessibility — QAccessibleInterface for ChatView so screen readers can read chat
- [ ] Spellcheck — hunspell integration for the input box (on hold)
- [ ] Long-press context menus — touch-friendly alternative to right-click for tablets
- [ ] More bundled scripts — /calc, /8ball, /shrug, /tableflip, etc.
- [x] Send colored text — 16-color mIRC picker for text + background via Ctrl+Shift+K or right-click input → Color; stored as QTextCharFormat brushes like bold/italic and encoded (`\x03fg[,bg]`) at send time; format indicator shows a colored `A` (2026-07-09)

---

## Security Backlog

- [x] Config file permissions — 0600 enforced via QSaveFile (done v0.8.0)
- [x] Outbound IRC injection prevention — stripCrlf/validIrcToken/ircv3TagEscape helpers; all send paths validated (v0.13.0)
- [x] openUrl scheme guard — only http/https links passed to QDesktopServices::openUrl (v0.13.0)
- [x] Link preview SSRF — og:image and direct image fetches blocked for private/loopback addresses (v0.13.0)
- [x] DCC receive write cap — reads bounded to remaining advertised byte count (v0.13.0)
- [x] DCC send 4 GiB guard — files over UINT32_MAX rejected; ACK comparison uses qint64 (v0.13.0)
- [x] DCC local bind — listener binds to IRC socket's local interface, not all interfaces (v0.13.0)
- [x] DCC cancel cleanup — cancel() methods added; partial files removed on user cancel (v0.13.0)
- [x] DCC offer validation — zero port / non-positive filesize rejected before accept dialog (v0.13.0)
- [x] Reconnect socket abort — socket abort()ed before reconnect if not already unconnected (v0.13.0)
- [x] Password field encryption — don't store plaintext passwords (OS keychain integration) (v0.16.6)
- [x] Password field show/hide toggle — eye icon (Material Symbols) inside each password field in Add/Edit Server dialog; toggles echo mode (v0.24.2)
- [x] Keychain sentinel UX — fields with keychain-stored passwords show placeholder text instead of sentinel as dots; empty-on-save preserves the keychain entry instead of deleting it (v0.24.2)
- [x] Keychain missing-entry warning — if config has `<keychain>` but no OS keychain entry exists, a red error in the server buffer directs the user to re-enter the password in Edit Server (v0.24.2+)
- [x] Memory optimizations — message buffer cap 2000→500, link preview cap 100→20, emoji picker lazy-loaded, QTextDocument block count aligned to buffer cap (v0.24.2)
- [x] Qt6Keychain CI integration — FetchContent fallback in CMakeLists.txt; all three platform CI + release builds passing (v0.16.6 follow-up)
- [x] Theme-aware link preview cards — bg/border/text/timestamp colours from active theme (v0.16.7)
- [x] Right-click Copy + Reply together — both available from message body right-click; no need to aim at the timestamp (v0.16.7)
- [x] NickServ credential redaction extended — IDENTIFY/REGISTER/GHOST/RECOVER/RELEASE/REGAIN/SETPASS all redacted in local echo (v0.16.7)
- [x] Service NOTICE routing — ChanServ/NickServ/BotServ/MemoServ replies route to their PM tab when open (v0.16.7)
- [x] Contextual command replies — /whois, /version, and /ping responses appear in the active channel/PM window instead of the server buffer; implemented via contextualMessage signal from IrcClient through SessionModel (v0.25.0)
- [x] howto.html full-text search — live search box in nav sidebar, match count, keyboard navigation (v0.16.7)
- [x] /leave and /close commands — close query buffer or part channel (v0.16.8)
- [x] NickServ credential redaction complete — echo-message server echo path also redacted (v0.16.8)
- [x] Self-signed cert option — per-server accept/reject + fingerprint-pin UI (v0.20.0)
- [x] SOCKS5 proxy support — per-server proxy_host/port/user/pass; GUI in server dialog (v0.16.8)
- [x] OPER password redaction — `redactRawForLog` now covers OPER credentials same as PASS (2026-06-05)
- [x] DCC passive filename sanitization — strip all control chars from remote-supplied DCC filenames to prevent CTCP envelope injection (2026-06-05)
- [x] DCC passive receive: peer validation skipped when expectedIP is private/zero — first inbound TCP connection accepted without identity check; add per-transfer write timeout and document the limitation (`dccreceive.cpp:98-103`)
- [x] DCC send: no receiver IP validation — first peer to connect gets the file; add expected-peer check to `DccSend::listen()`, reject others (`dccsend.cpp:102-111`)
- [x] Link preview SSRF: DNS rebinding gap — extended `addresscheck.h` to cover CG-NAT, documentation, unspecified, IPv4-mapped private ranges; DNS pre-check verifies all resolved IPs are non-private before fetch. IP pinning (`pinRequestToAddress`) was added (2026-06-18) then removed (2026-06-19) because it broke HTTP/2 `:authority` headers and TLS SNI on macOS, causing 404s on major sites. HTTP 4xx/5xx responses now silently dropped as defense-in-depth.
- [x] `sendRaw`: no 512-byte IRC line cap — add `.left(510)` before write; oversized lines are partially delivered or rejected by server (`ircclient.cpp`)
- [x] `onConnected`: `NICK`/`USER` only call `stripCrlf`, not `validIrcToken` — malformed nick from bad config produces a broken `USER` line (`ircclient.cpp`)
- [x] DCC send filename: normal send path only replaces spaces, not control chars — create shared `safeDccFilename()` helper used by all DCC SEND paths (`dccsend.cpp:92-95` vs `mainwindow.cpp:1469-1477`)
- [x] Unknown slash commands sent as raw IRC by default — change to "unknown command" message; keep `/raw` and `/quote` for power users; add opt-in `advanced_raw_passthrough` config key (`commanddispatcher.cpp:572-575`)
- [x] `linkpreview`: apply `isBlockedBySchemeOrLiteral` in `extractImageUrl` before returning URL — defense-in-depth; currently only checked by the caller (`linkpreview.cpp`)
- [x] `Channel::previews`: cap stored HTML string length per entry — `kPreviewCap=100` limits count but not per-entry size (`channel.h`)
- [x] KWallet/keychain popup on every preference change — `Config::save()` called `storePassword()` unconditionally; now guarded by `migratePasswords` flag, only true for server dialog saves (v0.24.3)
- [x] README vs code: README says "plaintext not supported" but `connectToHost()` is reachable when `ssl=false` — enforce TLS-only in code or update README (`ircclient.cpp:103-107`)
- [x] Keychain migration delimiter bug — first-time migration joined with `\x00` but split with `\x1F`, stuffing all four passwords into slot 0; fixed to use `\x1F` consistently (2026-06-18)
- [x] DCC double-cleanup — `m_done`/`m_finished` terminal-state guards in DccReceive/DccSend; QPointer in all MainWindow DCC lambdas; prevents use-after-free on cancel/error/finish races (2026-06-18)
- [x] Passive DCC timeout — stale `m_pendingPassiveSends` entries cleaned up after 120s (2026-06-18)
- [x] DCC SEND quoted filename parsing — handles `"my file.txt"` from other clients; control characters stripped (2026-06-18)
- [x] TLS certificate prompt accuracy — no longer labels hostname mismatches as "self-signed"; generic "could not be verified" text with accurate action wording (2026-06-18)
- [x] CodeQL CI — `security-and-quality` query suite on push, PR, and weekly schedule (2026-06-18)
- [x] CI ctest on all platforms — tests now run on Linux, Windows, and macOS matrix jobs, not just sanitizer (2026-06-18)
- [x] CMake `UPLINK_VENDOR_DEPS` option — default ON; set OFF for packaging to fail on missing system deps (2026-06-18)

---

## Performance & Maintainability Backlog

Items from the lightweight code review (2026-06-04). Ordered roughly by value / effort.

### Near term
- [x] `/sysinfo` async — run process collection off the UI thread; cache result for the session; add hard global timeout
- [x] DCC receive hardening — configurable max receive size; write to `.part` file and rename on success; delete partial on failure/cancel; check available disk space before starting
- [x] Link preview queue — replace abort-on-new-fetch with a small queue (max concurrency 1–2); cap `m_previewChannels`; add per-channel/per-minute throttling
- [x] Compiler warning cleanup — fix `-Wconversion` narrowing (qsizetype→int), `-Wshadow` locals, `-Wold-style-cast` casts flagged by the new warning flags
- [x] `m_buffer` in `onReadyRead`: keep as `QByteArray` through line-split; call `QString::fromUtf8` per-line only — eliminates per-event UTF-16 conversion and O(n) string shift (`ircclient.cpp`)
- [x] `logMessage`: cache one `QFile*` per active log target in a `QHash`; flush periodically — open/close per message is the dominant bottleneck during history replay (`sessionmodel.cpp:156-180`)
- [x] `m_ctcpTimestamps`: grows unbounded on long sessions with many unique nicks — cap at ~500 entries or evict on insert (`ircclient.cpp`)
- [x] `removeNick`: patch `nickIndex` in-place (decrement indices > removed position) instead of full `rebuildNickIndex()` — O(n²) during netsplit QUIT storms (`channel.h`)
- [x] Batch nick-index rebuild on netjoin bursts — `addNicks()` mirrors `removeNicks()`; `onNetjoinDetected` groups nicks per channel and rebuilds once, O(m·n) → O(m + n log n) (`channel.h`, v0.25.56)
- [x] `prependMessages`: copy only the messages that survive the cap into a pre-reserved list instead of concatenating the whole buffer on CHATHISTORY backfill (`channel.h`, v0.25.56)

### Medium term
- [x] Incremental nick-list updates — emit specific `nickAdded`/`nickRemoved`/`nickRenamed`/`nickModeChanged` signals; replace `clear()`/repopulate with targeted updates; batch during NAMES/netsplit bursts
- [x] Per-channel nick index — `QHash<QString, int> nickIndex` (lower nick → index) to replace O(n) scans in `onMessage`, `onNotice`, `onAction`, `onAccountChanged`
- [x] Batch chat/nick refreshes — debounce `refreshChatView()` and `refreshNickList()` with a short single-shot timer for burst updates
- [x] Split `ChatRenderer` out of `MainWindow` — `formatMessage`, `ircToHtml`, `linkifyHtml`, emoji rendering as a separate class
- [x] Split `CommandDispatcher` out of `MainWindow` — `/join`, `/msg`, `/dcc`, `/sysinfo`, etc. as a separate class
- [x] Parser and model unit tests — `IrcParserTest` (prefixes/tags/CTCP/malformed), `ChatFormatTest` (HTML escaping/linkification); `SessionModelTest` deferred (needs mock IrcClient)

### Bigger / architectural

**MainWindow refactor** — extract controllers from `mainwindow.cpp` (~5 000 lines) one at a time, no visual changes. Each extraction moves a self-contained block of logic into its own class:
- [x] `DccController` — all DCC send/receive UI, lifecycle, progress dialogs, passive pending map (dcccontroller.cpp, 2026-07-02)
- [x] `PreviewController` — link preview queue, watchdog timer, card storage; MainWindow keeps view insertion via cardStored signal (previewcontroller.cpp, 2026-07-02)
- [x] `UpdateChecker` — auto-update check/download/install extracted to updatechecker.cpp (2026-07-02)
- [x] `InputController` — input bar setup, submit, tab completion, history, emoji autocomplete, send button split to inputbar.cpp (file split, key routing stays in eventFilter) (2026-07-02)
- [x] `NickContextMenuBuilder` — nick right-click actions extracted to contextmenus.cpp (2026-06-23)
- [x] `ChatRenderController` — message append, reaction/redaction updates, event condensation, view refresh + scrollback pagination split to chatupdates.cpp (2026-07-02)

mainwindow.cpp reduced 4 669 → 3 184 lines across the 2026-07-02 extractions.

**ChatView performance** — reduce cost of relayout and rendering on large backlogs (5 000-line cap):
- [x] Skip full relayout on height-only resize (width unchanged) (2026-06-18)
- [x] Deferred layout — on width change only the visible window is relaid synchronously; remaining lines processed in idle-timer chunks with the top visible line anchored (2026-07-02)
- [x] Incremental relayout on width change — preview cards, empty lines, and single-line messages that still fit skip the QTextLayout pass; per-line naturalTextWidth cached (2026-07-02)
- [x] Profile `drawLine` paint path — QTextLayout now cached per line and reused across repaints (was rebuilt per visible line per paint); evicted when lines leave the painted window so memory stays bounded; anchorAt/hitTest reuse the same cached layout (2026-07-02)

- [x] Keychain operations async — remove nested `QEventLoop` in `KeychainHelper`; load non-secret config first, then resolve secrets before connecting; avoids reentrancy hazard
- [x] Targeted chat block updates — `BlockMsgid` userData tags each QTextBlock; onReactionsChanged and onMessageRedacted do targeted insert/replace/remove instead of full rebuild (v0.23.3)
- [x] IrcParser fuzz target — libFuzzer harness around `IrcParser::parseLine()` for parser regression coverage
- [x] Custom painter ChatView — replaced QTextBrowser/QTextDocument with QAbstractScrollArea + QTextLayout-per-visible-line; eliminates persistent off-screen layout trees; O(log n) scroll mapping via cumulative height prefix sum; full text selection (click-drag, Ctrl+C), Ctrl+F find with yellow highlight, link preview cards, event group toggle, reactions, load-older pagination (2026-06-13)
- [x] glibc arena cap — `mallopt(M_ARENA_MAX, 2)` in main(); default 8×cores = 96 arenas × 3–4 MiB each = ~275 MiB wasted RSS on idle; cap brings cold-start RSS from ~538 MiB to ~263 MiB (2026-06-13)
- [x] Memory usage reduction — periodic `malloc_trim(0)` (60s timer + channel switch); `M_TRIM_THRESHOLD` 64 KB; removed redundant avatar base64 cache; tightened cache caps (avatar 200→80, nick meta 1000→300, ChatView lines 5000→2000, link preview 50→20, per-channel preview 8→4, QPixmapCache 4→2 MB) (2026-06-19)
- [x] Per-message hang indent — `ChatLine::hangIndentChars` stores char offset of body start; `layoutLine` measures prefix pixel width via QTextLayout (bold-nick-accurate); continuation lines align with the first letter of the message body, not just the timestamp column (2026-06-13)
- [x] `ServerId` / `BufferId` strong types — replace stringly-typed host/channel routing in MainWindow, CommandDispatcher, ChannelPane, ChannelListDialog; eliminates 17 unwrapping lambdas and ~80 redundant wrapping sites (v0.25.44, 2026-06-18)
- [x] `selfNickRe` (`QRegularExpression`): verify compiled once per nick change at call site, not reconstructed per `formatMessage` call (`chatrenderer.cpp`) — already correct
- [x] DCC ACK coalescing: send ACK every 64 KB and on completion, not every `readyRead` — reduces write syscalls on fast links (`dccreceive.cpp`)
- [x] CI: add Linux ASan/UBSan sanitizer job (`-DUPLINK_ENABLE_SANITIZERS=ON` Debug build); add `clang-tidy` or CodeQL static analysis (`.github/workflows/ci.yml`, `CMakeLists.txt:145-149`)
- [x] Consolidate `kKeychainSentinel` — was defined in 4 files; moved to `config.h` as a single `inline const QString` (2026-06-18)
- [x] Cap `m_botIconIdx` — unbounded growth per unique bot nick; capped at 500 entries with FIFO eviction (2026-06-18)
- [x] Update check JSON parsing — replaced regex `tag_name` extraction with `QJsonDocument`; added 15s transfer timeout (2026-06-18)

### PR #30 review follow-ups (2026-07-10)
- [x] Shared `paneKey()` / `isChannelName()` helpers (`model/ids.h`) — replace ~35 hand-rolled `host|channel` key constructions and inconsistent channel-prefix checks; prefix checks widened to the full RFC set (`#&+!`)
- [x] Shared `SearchBar` / `NickFilterEdit` widgets (`searchbar.cpp`, `nickfilteredit.cpp`) — replace duplicated search-bar and nick-filter code between `MainWindow` and `ChannelPane`; also fixes pane-search next/prev and nick-filter Escape-to-clear parity gaps
- [x] Shared header-button hover QSS — `UiStyle::headerButtonStyle()` replaces 13 copies (`uistyle.h`)
- [x] Fixed duplicate-buffer display — a channel already docked in a pane no longer also loads into the primary view

---

## Stability Backlog

- [x] Ping watchdog timeout: `sendPing` ignores stale pending pings indefinitely — abort + reconnect after ~90s; improves recovery from dead Wi-Fi, VPN changes, NAT timeouts (`ircclient.cpp:431-437`)
- [x] Early PING during SASL registration — removed `singleShot(2000, sendPing)` from `onConnected()`; was causing 2-minute disconnect cycle on solanum servers (Libera.Chat) where pre-registration PINGs are silently ignored
- [x] `sendRaw` disconnected: silently drops messages during reconnect — show local "Not connected; message not sent" error for user-visible commands (`ircclient.cpp:276-281`)
- [x] DCC write failure: `onReadyRead` emits error but doesn't call `cancel()` — call cancel directly on write failure; guard against multiple error/finished emissions with `m_done` flag (`dccreceive.cpp:151-154`)
- [x] DCC progress divide-by-zero: `checkTransferPrecon` does not reject `total <= 0` — malformed DCC offer with size=0 can divide-by-zero in UI progress callback (`dccreceive.cpp:21-47`, `mainwindow.cpp:1484-1485`)
- [x] `DccSend::sendNextChunk`: no socket state check after `cancel()` — `m_socket` non-null in `ClosingState` is currently safe but fragile; check state explicitly or null `m_socket` in `cancel()` (`dccsend.cpp`)
- [x] `IrcParser::isValid()`: add explicit `!command.isEmpty()` check — empty-command lines fall through to numeric default handler silently (`ircparser.cpp`) — already implemented
- [x] STS upgrade race: add comment explaining ordering assumption between `abort()`, synchronous `onDisconnected`, and the `singleShot` lambda (`ircclient.cpp`)

---

## Known Issues

- DCC over internet (active mode) — active DCC advertises your local IP; if both sides are behind NAT the connection still fails. Use **Send File (Passive)** so the receiver opens the port instead.
- DCC passive receive over NAT — the receiver's port must be reachable from outside. If the receiver is also behind NAT, passive DCC will not work either (both sides blocked). No relay mechanism implemented.
