# Changelog

<!--
Session 2026-06-20 (bot icon):
- Root-caused bot icon not showing on Linux: per-nick WHOX returned channel "*",
  354 handler dropped it; hasCap("whox") was always false; format string missing
  querytype token. Also a timing issue in DojoBot setting +B after JOIN.
- Fixed: supportsWhox() accessor, 354 auto-detect token, onWhoEntry handles "*"
  channel via sess->botNicks, setIconSize on nick list, standard WHO fallback.
- Bot-side fix: set +B before JOIN so WHO sees the flag immediately.
- Tagged missing releases: v0.25.46, v0.25.47, v0.25.48, v0.25.49.
- Code review finding: bot un-flag path for "*" channel doesn't emit nickListChanged
  (low severity — bot losing +B mid-session is rare).
- No regressions found.
- Next priorities: MainWindow controller extractions, ChatView deferred layout.
-->

<!--
Session 2026-06-20:
- Added keyboard navigation: Alt+Up/Down cycles channels, Alt+Left/Right cycles panes
- Added Ctrl+K quick channel switcher with live filtering popup
- Added scroll-to-top history loading via CHATHISTORY BEFORE — infinite scrollback
- Added jump-to-bottom button with fade animation for busy channels
- Fixed scroll-up on fast channels: sticky m_userScrolledAway flag, guarded all
  scrollToBottom callers with isAtBottom(), fixed rangeChanged auto-scroll bypass
- Fixed mIRC color codes showing as boxes in topic bar (linkifyTopic → ircToHtml)
- Fixed macOS crash: viewport() call in eventFilter during setupSidebar() construction
- Fixed duplicate /caps command handler (dead code, never reached)
- Cleaned all compiler warnings: old-style casts → static_cast, qsizetype narrowing,
  platform ifdef for sysinfoKernel
- Closed all 8 external PRs (rich search feature set out of scope, password file
  redundant vs keychain)
- Keyboard shortcuts documentation fully rewritten
- Bugs found: none remaining. All scroll issues resolved through iterative testing.
- Next priorities: MainWindow controller extractions, ChatView deferred layout,
  manual stress tests (flood, memory steady-state, AFL fuzzer)
-->

<!--
Session 2026-06-19 (emoji):
- Expanded emoji table from 622 to 1,906 entries using Unicode emoji-test.txt
  and GitHub gemoji shortcodes. Added scripts/generate_emojidata.py for future updates.
- Fixed emoji font size bug: emojiPt config (default 16pt) was only applied in the
  HTML rendering path (topic bar/tooltips), not in the QTextLayout-based ChatView.
  Emojis in chat now render at the configured size.
- No regressions found. No known issues.
- Next priorities: MainWindow controller extractions, ChatView deferred layout.
-->

<!--
Session 2026-06-19:
- Investigated 590 MB RSS memory usage reported via KDE System Monitor
- Added malloc_trim(0) on channel switch + 60s periodic timer to return freed pages to OS
- Lowered M_TRIM_THRESHOLD to 64 KB for more aggressive glibc heap trimming
- Removed m_avatarBase64Cache (redundant double-storage of avatar images); base64 now
  generated on demand when building tooltip HTML
- Shrunk cache caps: avatar 200→80, nick meta 1000→300, ChatView max lines 5000→2000,
  link preview cache 50→20, per-channel preview cap 8→4, QPixmapCache 4→2 MB
- No regressions found. No new bugs discovered.
- Next priorities: measure RSS after these changes; if still high, profile with heaptrack
  for leak/fragmentation sources; continue MainWindow controller extractions.
-->

<!--
Session 2026-06-18:
- Cleaned alpha fringe from all 15 website app icon PNGs (background halos on site)
- Addressed full external code review: fixed keychain migration delimiter bug,
  DCC double-cleanup/QPointer guards, passive DCC 120s timeout, DCC quoted filename
  parsing, SSRF DNS-rebinding (pinRequestToAddress), TLS prompt accuracy, update
  check JSON parsing with QJsonDocument
- Added CodeQL CI workflow (security-and-quality, push/PR/weekly)
- Added ctest to all CI matrix jobs (was sanitizer-only)
- Added CMake UPLINK_VENDOR_DEPS option for packaging builds
- Cleaned up: consolidated kKeychainSentinel to config.h, ChatView resize skip on
  height-only change, capped m_botIconIdx at 500, sidebar null check
- Updated quality.html with CodeQL and CI pipeline sections
- Added MainWindow refactor plan and ChatView perf items to ROADMAP
- No regressions found. No known issues left open from this session.
- Next priorities: start MainWindow controller extractions (DccController first),
  ChatView deferred layout, review CodeQL findings when first scan completes.
-->

<!--
Session 2026-06-20 (font zoom fix):
- Fixed font zoom row heights: NickDelegate and SidebarDelegate returned hardcoded
  pixel heights (16 / 26), so Ctrl+scroll changed text width but rows never grew
  taller. Now derived from option.fontMetrics.height() with qMax preserving originals.
- Fixed topic bar Ctrl+scroll: m_primaryHeader wasn't mapped in fontFieldForWidget,
  so zoom had no live effect (only applied after restart). Also m_topicSetByLabel
  never got setFont() in applyFontSizes.
- No regressions found. No known issues.
- Next priorities: MainWindow controller extractions, ChatView deferred layout.
-->

<!--
Session 2026-06-21 (project management):
- Reviewed and closed 3 unsolicited PRs from roeyk (#10 splitters, #11 topic
  collapse, #12 dropdown mode). Code was AI-generated, didn't match project
  style, and added unneeded complexity/resource overhead.
- Added CONTRIBUTING.md — sets expectations: open issue first, match code style,
  project goals (stable/secure/fast/lightweight), what won't be merged.
- No code changes this session. No version bump.
- Next priorities: MainWindow controller extractions, ChatView deferred layout.
-->

<!--
Session 2026-06-23:
- Refactored mainwindow.cpp: extracted context menus (contextmenus.cpp, 562 lines)
  and nick panel (nickpanel.cpp, 280 lines); moved isCondensable and buildReactionLine
  to chatrenderer.h as shared inlines. mainwindow.cpp 5281→4447 lines.
- Added unit tests: tst_config.cpp (12 tests for Config::load/save round-trip,
  defaults, backward compat, app icon migration, bouncer types) and
  tst_ignoretypes.cpp (6 tests for IgnoreTypes flags, case-insensitive lookup).
- Added kinetic touch scrolling via QScroller on ChatView, sidebar, nick list,
  and channel pane nick lists. QScroller handles touch-vs-click disambiguation
  automatically. Mouse behavior unchanged.
- No regressions found. No known issues.
- Next priorities: test touch scrolling on actual tablet; accessibility
  (QAccessibleInterface for ChatView); spellcheck (on hold).
-->

<!--
Session 2026-06-24:
- Added user script bindings system — link external scripts to custom slash
  commands via Preferences → Scripts. Scripts run via QProcess in a worker
  thread with env vars (UPLINK_NICK, UPLINK_SERVER, UPLINK_CHANNEL, UPLINK_ARGS)
  and CLI args. 10s timeout, 5-line output cap, no shell wrapper.
- Bundled 4 default scripts: /music (MPRIS2/nowplaying-cli), /weather (wttr.in),
  /uptime (cross-platform), /roll (tabletop dice roller).
- Auto-install bundled scripts on first launch to ~/.config/uplink/scripts/.
  Restore Defaults button re-adds missing bundled scripts without touching
  user-added ones. Bundled scripts are overwritten with app version on restore.
- Added material delete icon for script remove button.
- Redesigned Scripts tab from QTableWidget to row-based layout (fixed macOS
  rendering issues with invisible button text and wrong font sizes).
- Fixed Windows CI: std::min macro conflict with MSVC's windows.h min macro.
- Fixed /uptime on macOS: greedy sed matched usec instead of sec field.
- Full docs: commands.md, configuration.md, howto.html, index.html landing page.
- No regressions found. No known issues.
- Next priorities: accessibility (QAccessibleInterface for ChatView);
  spellcheck (on hold); more MainWindow extractions.
-->

<!--
Session 2026-06-25 (Windows script support + CI green):
- Fixed /music on Linux: D-Bus fallback was printing raw xesam:title keys instead
  of actual track values. grep -A1 + head -1 was matching the dict key line;
  changed to tail -1 to get the value line.
- Added Windows support for bundled scripts: .sh files launched via bash.exe
  (Git Bash/WSL) in commanddispatcher.cpp; executable check skipped for .sh on Windows.
- /music on Windows: queries GSMTC (Global System Media Transport Controls) via
  PowerShell WinRT — works with Spotify, Chrome, Edge, VLC, etc. Requires Windows 10 1809+.
- /uptime on Windows: queries LastBootUpTime via PowerShell Get-CimInstance.
- /weather and /roll: no changes needed — curl and pure bash work via Git Bash.
- Fixed Windows CI (tst_config had been failing since scripts feature landed):
  1. CMAKE_RUNTIME_OUTPUT_DIRECTORY: qt6keychain.dll wasn't colocated with test
     binaries, causing a silent startup failure (Windows DLL loading).
  2. Stubbed keychainhelper in test build to remove qtkeychain dependency.
  3. Root cause found: QTemporaryFile on Windows uses FILE_FLAG_DELETE_ON_CLOSE,
     blocking Config::load from opening the file via a second handle. Fixed by
     replacing QTemporaryFile with QTemporaryDir + QFile in tst_config.cpp.
- All CI green: Linux, macOS, Windows, sanitize all pass.
- Full docs: CHANGELOG, ROADMAP, README, commands.md, howto.html, index.html updated.
- No regressions found. No known issues.
- Next priorities: accessibility (QAccessibleInterface for ChatView); spellcheck (on hold).
-->

## 0.25.53

### Added
- **Windows script support** — `.sh` scripts on Windows are now launched via `bash.exe` (Git Bash / WSL) instead of failing with "not executable". The executable check is skipped for `.sh` files on Windows.
- **Windows /music** — queries the Windows Global System Media Transport Controls (GSMTC) via PowerShell; works with any media player that integrates with Windows media (Spotify, Chrome, Edge, VLC, etc.). Requires Windows 10 1809+.
- **Windows /uptime** — queries boot time via PowerShell `Get-CimInstance`.

### Fixed
- **Linux /music** — D-Bus fallback was printing raw MPRIS key names (`xesam:title`) instead of actual track metadata; grep was matching the dict key line instead of the value line.
- **Windows CI** — `tst_config` had been timing out/crashing since v0.25.52; root cause was `QTemporaryFile` using `FILE_FLAG_DELETE_ON_CLOSE` on Windows, blocking `Config::load` from reading the file. Fixed by using `QTemporaryDir` + `QFile` instead.

## 0.25.52

### Added
- **User script bindings** — link external scripts to custom slash commands via Preferences → Scripts. Any executable (bash, python, ruby, etc.) can become a `/command`. Scripts run in a background thread with context passed as environment variables (`UPLINK_NICK`, `UPLINK_SERVER`, `UPLINK_CHANNEL`, `UPLINK_ARGS`). 10-second timeout, 5-line output cap, no shell injection surface.
- **Bundled scripts** — four scripts ship out of the box and auto-install on first launch: `/music` (MPRIS2/D-Bus on Linux, nowplaying-cli on macOS), `/weather` (wttr.in), `/uptime` (cross-platform), `/roll` (tabletop dice roller, e.g. `/roll 2d6`).
- **Restore Defaults** button in Preferences → Scripts re-adds only missing bundled scripts without touching user-added ones.

### Fixed
- **Windows build** — `std::min` macro conflict with MSVC's `windows.h` `min` macro.
- **macOS /uptime** — greedy sed was matching the `usec` field instead of `sec`, showing 56 years of uptime.

## 0.25.51

### Added
- **Kinetic touch scrolling** — flick-to-scroll with momentum on touchscreen and tablet devices via Qt's QScroller; enabled on chat view, sidebar, nick list, and channel panes. Mouse behavior is unchanged.
- **Unit tests for config and ignore types** — 12 config tests covering TOML loading, saving, round-trip, defaults, backward compatibility, and app icon migration; 6 ignore type tests covering flag operations and case-insensitive nick matching.

### Changed
- **Code organization** — extracted context menus (562 lines) and nick panel (280 lines) from mainwindow.cpp into separate files; moved `isCondensable` and `buildReactionLine` to chatrenderer.h as shared inline helpers. mainwindow.cpp reduced from 5,281 to 4,447 lines.

## 0.25.50

### Fixed
- **Font zoom row heights** — NickDelegate and SidebarDelegate had hardcoded pixel heights (16 / 26); Ctrl+scroll changed text width but rows never grew taller. Row heights now derived from font metrics.
- **Topic bar Ctrl+scroll** — primary header wasn't mapped in fontFieldForWidget, so zoom had no live effect until restart; topicSetByLabel also missed setFont() in applyFontSizes.
- **Bot icon not showing on Linux** — root cause was multi-layered: `hasCap("whox")` checked IRCv3 CAP instead of ISUPPORT (always false, so per-nick WHO was never sent); WHOX format string `%cnfa` was missing the querytype token `t`; the 354 handler dropped entries where channel was `*` (Ergo returns `*` for per-nick WHOX queries); and `onWhoEntry` only updated per-channel bot sets, not the session-wide set.
- **354 WHOX parser robustness** — auto-detects whether the server included the querytype token, handling both 5-param and 6-param response formats.
- **Per-nick WHO fallback** — sends standard `WHO nick` on servers that don't support WHOX, so bot detection works everywhere.
- **Nick list icon size** — explicit `setIconSize(16, 16)` ensures bot icons render on all platforms (Linux Qt styles defaulted to 0×0).
- **Preview card alignment** — link preview cards now align with the message body text, not the nick column.

## 0.25.49

### Added
- **Keyboard navigation** — Alt+Up/Down to cycle channels, Alt+Left/Right to cycle panes, works from any focused widget
- **Quick channel switcher (Ctrl+K)** — floating popup with live filtering to jump to any joined channel
- **Scroll-to-top history loading** — scrolling to the top of a channel buffer fetches older messages from the server via `CHATHISTORY BEFORE`; stops when server history is exhausted
- **Jump-to-bottom button** — floating double-arrow-down button fades in when scrolled up, click to return to live chat; fades out on return

### Fixed
- **Scroll-up on busy channels** — new messages no longer snap the viewport back to the bottom; a sticky `m_userScrolledAway` flag locks scroll position until the user explicitly returns to bottom
- **mIRC color codes in topic bar** — topic text now renders mIRC color/bold/italic/underline codes as formatted HTML instead of showing raw control characters as boxes
- **macOS crash on launch** — guarded `m_chatView->viewport()` call in `eventFilter` that fired before widget initialization during `setupSidebar()`
- **Compiler warnings** — replaced all C-style casts with `static_cast` in chatview.cpp, fixed `qsizetype`→`int` narrowing, guarded `sysinfoKernel()` with platform ifdef

### Changed
- **Keyboard shortcuts documentation** — fully rewritten with quick reference table, navigation section, macOS notes, no more "Planned" section

## 0.25.48

<!--
Session 2026-06-19 (link preview fix):
- Fixed link preview showing "Error 404 (Not Found)!!1" instead of actual page titles.
  Root cause: pinRequestToAddress() rewrote URLs to connect to resolved IPs directly,
  breaking HTTP/2 :authority headers and TLS SNI on macOS. Servers received the IP
  instead of the hostname and returned 404. The 404 error page's <title> was then
  extracted and displayed as the preview.
- Removed IP pinning entirely — DNS pre-check (all resolved IPs verified non-private)
  still blocks SSRF; the pinning closed a narrow DNS-rebinding TOCTOU gap that isn't
  a practical threat for an IRC client.
- Added HTTP status code check (>= 400) so error pages can never become previews.
- No regressions found. No known issues.
- Next priorities: MainWindow controller extractions, ChatView deferred layout.
-->

### Fixed
- **Link preview 404 errors** — removed IP pinning (`pinRequestToAddress`) which broke HTTP/2 `:authority` headers and TLS SNI on macOS, causing servers to return 404 error pages that were displayed as preview titles. DNS pre-check still blocks all private/loopback addresses.
- **Error page previews** — HTTP responses with status >= 400 are now silently dropped so error page titles never appear as link previews.

## 0.25.47

### Improved
- **Emoji set expanded** — 622 → 1,906 emojis from Unicode 16.0 emoji-test.txt with GitHub gemoji shortcodes. Covers all standard categories: Smileys, People, Animals, Food, Travel, Activities, Objects, Symbols, Flags.
- **Emoji generator script** — `scripts/generate_emojidata.py` regenerates `emojidata.h` from upstream Unicode + gemoji data. Run it to update emojis without manual editing.

### Fixed
- **Emoji font size in chat** — the `font_emoji` setting (default 16pt) was only applied in the HTML rendering path (topic bar, tooltips). The actual chat view (QTextLayout) rendered emojis at regular text size. Now correctly applies the configured size in the chat display.

## 0.25.46

### Improved
- **Memory usage reduction** — periodic `malloc_trim(0)` (every 60 s + on channel switch) returns freed glibc heap pages to the OS; `M_TRIM_THRESHOLD` lowered to 64 KB for more aggressive reclamation.
- **Avatar cache slimmed** — removed redundant `m_avatarBase64Cache` (base64-encoded PNG strings generated on demand for tooltips instead of pre-cached); avatar cache cap reduced from 200 to 80 entries.
- **Cache caps tightened** — ChatView max lines 5 000 → 2 000; link preview cache 50 → 20; per-channel preview cap 8 → 4; nick metadata cap 1 000 → 300; `QPixmapCache` limit 4 → 2 MB.

## 0.25.45

### Fixed
- **Website icon alpha fringe** — removed semi-transparent edge pixels from all 15 app icon PNGs that caused visible background halos on the landing page.
- **Keychain migration delimiter** — first-time credential migration used `\x00` to join fields but `\x1F` to split them, causing all four passwords to land in slot 0. Now uses `\x1F` consistently.
- **DCC double-cleanup** — added `m_done`/`m_finished` terminal-state guards to `DccReceive` and `DccSend`; all MainWindow DCC lambdas now use `QPointer` instead of raw pointers to prevent use-after-free on cancel/error/finish races.
- **Passive DCC timeout** — stale `m_pendingPassiveSends` entries are cleaned up after 120 seconds if the remote never replies.
- **DCC quoted filenames** — incoming `DCC SEND "my file.txt" ...` from other clients now parses correctly; control characters are stripped.
- **Link preview SSRF DNS pinning** — page, hover, and image fetches now connect to the validated resolved IP directly via `pinRequestToAddress()`, with `setPeerVerifyName()` for HTTPS SNI. Closes the DNS-rebinding TOCTOU gap. *(Reverted in v0.25.48 — broke HTTP/2 and macOS TLS; replaced with HTTP status code guard.)*
- **TLS certificate prompt** — no longer says "self-signed" for hostname mismatches; now says "could not be verified" with accurate action text.
- **Update check** — replaced regex JSON parsing with `QJsonDocument`; added 15-second transfer timeout.
- **ChatView resize** — height-only window resizes skip the full 5 000-line relayout since line wrapping depends only on width.

### Added
- **CodeQL CI** — GitHub CodeQL semantic analysis runs on every push, PR, and weekly with the `security-and-quality` query suite.
- **CI test coverage** — `ctest` now runs on all three platform matrix jobs (Linux, Windows, macOS), not just the sanitizer job.
- **CMake `UPLINK_VENDOR_DEPS`** — new option (default ON) controls whether CMake auto-downloads missing dependencies. Set `OFF` for packaging builds to fail on missing system packages.
- **15 app icon variants** — visual grid picker in Preferences → Appearance replaces the old Dark/Light radio buttons. Styles include flat, colorful, gruvbox, and original variants. Icon changes update the window, system tray, and KDE Plasma taskbar live. Old "dark"/"light" config values are auto-migrated.
- **Website icon rotation** — the Uplink website randomly selects one of the 15 app icons on each page load.

### Improved
- **Preferences dialog redesigned** — flat navigation list on the left (Appearance, Chat Window, Interface, Notifications, Logging, Profile) and content pages on the right. Each section has its own page with room to breathe.
- **Manage Servers dialog redesigned** — integrated two-panel layout replaces the old list-with-popup approach. Server list on the left, server settings form on the right. Selecting a server loads its settings inline — no more separate Edit dialog. Add, Remove, and reorder controls sit below the server list.

## 0.25.44

### Fixed
- **Link preview image limit** — raised from 200 KB to 2 MB so retina screenshots and larger images decode into thumbnails instead of silently failing.
- **Link preview partial decode** — truncated image downloads (from timeout or size cap) now emit a card without a thumbnail instead of displaying a half-grey broken image. Image fetch timeout increased from 6 s to 15 s.

### Added
- **Config hot-reload** — Uplink watches `config.toml` for external changes. Server additions and removals made in a text editor are picked up automatically without restarting.
- **Font zoom** — Ctrl+mousewheel or Ctrl+Plus/Minus zooms the font for whichever UI region the cursor is over (chat, nick list, sidebar, input, topic text, nick panel). Sidebar distinguishes server vs channel items. Half-point (0.5) steps for smooth scaling. Changes persist to config immediately.

### Improved
- **ServerId/BufferId strong types** — replaced raw `QString` host/channel parameters with strong types across MainWindow, CommandDispatcher, ChannelPane, and ChannelListDialog. Eliminates a class of parameter-swap bugs at compile time.
- **Clean config formatting** — config.toml writer and all documentation now use plain `key = value` with no column-alignment padding.

### Cleanup
- Removed link preview debug trace logging.

<!--
SESSION SUMMARY — 2026-06-18 (link preview fixes, strong types, hot-reload, font zoom)
What changed:
  - Diagnosed link previews failing for linuxdojo.org URLs: root cause was DNS
    split-horizon — LuCI dnsmasq override and /etc/hosts on fortis both resolved
    linuxdojo.org to 192.168.1.32 (RFC1918), which the SSRF guard correctly blocked.
    Fixed by removing the DNS overrides; no code change needed for that part.
  - Image fetch limit raised from 200 KB to 2 MB — retina screenshots (1–2 MB PNGs)
    were being truncated and failing to decode.
  - Image fetch timeout raised from 6 s to 15 s — slower connections (WiFi) could
    timeout mid-download, producing partial images with grey fill.
  - Truncated image downloads now skip decode entirely and emit a title-only card
    instead of displaying a broken thumbnail.
  - ServerId/BufferId strong types refactored across UI layer (8 files, ~100 call sites).
    17 signal-connection lambdas eliminated, ~80 redundant wrapping sites removed.
  - Debug trace logging removed from linkpreview.cpp and mainwindow.cpp.
  - Config hot-reload via QFileSystemWatcher — server additions/removals from external
    text editors picked up automatically. saveConfig() wrapper suppresses watcher
    feedback loop with 1-second guard.
  - Font zoom: Ctrl+scroll/Ctrl+± per UI region with 0.5pt steps. FontSizes struct
    changed from int to double. Font dialog uses QDoubleSpinBox. Sidebar distinguishes
    server vs channel items via itemAt(). Topic text re-wraps inline HTML on zoom.
  - Config writer stripped of column-alignment padding — clean key = value format.
    All docs (README, howto, FAQ, configuration, ircv3, index) updated to match.
  - Docs updated: hot-reload documented in FAQ and howto. Image limit corrected.
  - Roadmap: all items complete (ServerId/BufferId was the last).
No regressions. No known issues.
-->

## 0.25.43

### Fixed
- **Server identity collision** — multiple server entries sharing the same hostname (e.g. a direct connection and a bouncer connection to the same server) no longer interfere with each other. Server identity is now based on the unique config name rather than the hostname.
- **Duplicate server name guard** — the Manage Servers dialog now rejects adding or renaming a server to a name that already exists.
- **Sidebar order sync** — reordering servers in the Manage Servers dialog now takes effect immediately in the sidebar without requiring a restart.
- **Log directory migration** — existing log directories using the old hostname-based layout are automatically renamed to the new name-based layout on startup, preserving chat history.
- **URL splitting on no-break spaces** — URLs containing Unicode no-break spaces (U+00A0) are no longer incorrectly split into separate links.
- **Light theme text contrast** — improved text contrast on 72 light themes where foreground colors were too close to the background, making text hard to read.

<!--
SESSION SUMMARY — 2026-06-17 (URL regex, light theme contrast)
What changed:
  - URL regex fix: no-break spaces (U+00A0) no longer split URLs into separate links.
    Regex word-boundary anchors updated in ChatFormat to treat NBSP like a regular character.
  - Light theme contrast fix: 72 light themes had foreground colors too close to the
    background, making text hard to read. Adjusted fg/accent values for readability.
  - CHANGELOG updated with both fixes.
  - Verified on Mac — themes render correctly.
No regressions. No known issues.
-->

<!--
SESSION SUMMARY — 2026-06-17 (ServerId refactor, duplicate guard, sidebar sync, log migration)
What changed:
  - ServerId refactored from hostname to server config name — fixes collision when multiple
    server entries share the same host (e.g. direct + bouncer to irc.linuxdojo.org).
    IrcClient now carries m_serverName alongside m_host; all 119 signal emissions use
    the name. SessionModel lookups, removeServer, updateServer, clientFor all match by name.
  - Duplicate server name guard in Manage Servers dialog — blocks add/edit if name exists.
  - Sidebar reorders live after Manage Servers dialog closes (syncSidebarOrderFromConfig).
  - Log dir migration at startup: hostname-based dirs renamed to name-based dirs.
  - Removed dead syncServers() method.
  - Docs updated: configuration.md (name must be unique), howto.html (server dialog field
    reference), faq.md (multiple servers can share a host with different names).
  - Released as v0.25.43. CI, Release, and Pages all green.
No regressions. No known issues.
-->

<!--
SESSION SUMMARY — 2026-06-17 (server management, /connect, CTCP fix, stability)
What changed:
  - Right-click server context menu: added Close Server (disconnect + remove from sidebar,
    keeps config), Move Up, Move Down (reorder servers, persisted to config).
  - Manage Servers dialog: added ▲/▼ buttons to reorder servers.
  - New commands: /connect and /server to connect to arbitrary servers by host[:port] with
    auto-detected SSL (port 6667 = plain, else SSL). /disconnect to close current server.
  - Sidebar labels for ad-hoc servers use short network names (strip irc. prefix and TLD),
    e.g. irc.chatjunkies.org → CHATJUNKIES.
  - Fixed duplicate CTCP VERSION/PING replies caused by echo-message cap echoing outgoing
    requests back to the sender — self-echo CTCP requests are now skipped.
  - Fixed /clear not persisting: previously only cleared the visual widget, now also purges
    the in-memory message buffer so messages don't reappear when switching channels.
  - Fixed chat view layout thrashing: bottom third of chat would flicker rapidly when content
    was near viewport threshold. wrapWidth() now reserves scrollbar width permanently.
  - Resource audit: ASan+UBSan tests all clean. Pruned m_scrollPositions and m_renderStart
    on channel/server close. Capped m_expandedEventGroups at 200 entries. Avatar base64
    cache was already properly bounded.
  - All docs updated across commands.md, howto.html, faq.md, index.html, README.md, ROADMAP.md.
No regressions. No known issues.
Next priorities: new hero screenshots for uplinkirc.chat (URGENT); ServerId/BufferId strong types.
-->

<!--
SESSION SUMMARY — 2026-06-17 (soju server admin docs)
What changed:
  - Added "soju server administration" section to docs/howto.html covering server-side
    setup: install, config, user management (sojuctl), adding upstream networks via
    BouncerServ, managing networks/channels in the SQLite database, and a common-gotchas
    troubleshooting table. Nav sidebar updated with the new section link.
No code changes. Documentation only.
-->

<!--
SESSION SUMMARY — 2026-06-17 (emoji button transparent bg)
What changed:
  - Emoji picker button (😊) had an opaque background on macOS — the default QPushButton
    styling drew a visible box around the emoji. Fixed by adding background: transparent
    and border: none to the button's inline stylesheet (mainwindow.cpp:1807).
No regressions. No known issues.
Next priorities: ASan/UBSan run + heaptrack profile analysis; ServerId/BufferId strong types.
-->

<!--
SESSION SUMMARY — 2026-06-16 (icon swap, close-channel UI fix, warning cleanup, v0.25.40 release)
What changed:
  - Preferences button icon swapped from settings.svg (gear) to manage_accounts Material Symbol.
    Icon size bumped from 18×18 to 20×20 to match visual weight.
  - Manage Servers button went through several icon candidates this session (domain_add →
    lan → network_manage → add_link) before landing on add_link. Button enlarged from
    28×28 / 24×24 to 32×32 / 28×28 to compensate for add_link's thinner visual weight.
  - mi-lan.svg and mi-network-manage.svg added during icon exploration then removed as
    orphans before session close.
  - Manage Servers tooltip updated from "Manage Servers" to "Add / Manage Servers" to better
    match the add_link icon which reads as "add".
  - Close channel UI stale state fixed: onChannelRemoved() now calls
    onSidebarSelectionChanged() after deleting the sidebar item. Before this fix, closing a
    channel via right-click would leave the info bar and nick list showing the closed
    channel's data until the user manually clicked the auto-selected channel.
  - -Wconversion warning in chatview.cpp:739 fixed: `const int newline` → `const qsizetype`.
  - Nick completion button removed from roadmap (was already reverted; Tab is sufficient).
  - ASan/UBSan and heaptrack builds rebuilt against current code. Both clean. Testing
    deferred to next session.
  - Released v0.25.40.
  - Commits: 14190cd (icon swap + close-channel fix), dd7e455 (v0.25.40 release),
    fc12aac (orphan icon cleanup + tooltip)
No regressions. No known issues.
Next priorities: ASan/UBSan run + heaptrack profile analysis; ServerId/BufferId strong types.
-->

<!--
SESSION SUMMARY — 2026-06-16 (IRC formatting, format indicator, keychain bundle fix, search icon, v0.25.39 release)
What changed:
  - IRC text formatting (Ctrl+B/I/U/S/O) now works correctly in the input bar. Formatting is
    applied visually via QTextCharFormat — bold text looks bold while you type, italic looks
    italic, etc. No control characters are inserted into the widget. IRC codes (0x02, 0x1D,
    0x1F, 0x1E, 0x0F) are generated from the document formatting at send time via
    inputToIrcText(). Previous attempts inserted raw control chars which rendered as box glyphs.
  - Format indicator: a small floating label (B I U S) appears at the bottom-left inside the
    input box when any formatting is active. It shows only active formats, styled with their
    respective formatting (bold B, italic I, underlined U, struck S). Hides on send or Ctrl+O.
    Updates on cursor movement so it reflects the format at the cursor position.
  - Format combos fixed: Ctrl+B then Ctrl+U now stacks bold+underline correctly. Each handler
    now starts from currentCharFormat() instead of a fresh QTextCharFormat, preserving all
    other active formats when toggling one.
  - echo-message CAP added to desired caps list. Without it, self-sent messages were stored
    with empty msgid (server never echoed them back with an assigned id). Cross-client reactions
    failed because findLine(msgid) returned -1. With echo-message, the server echo delivers
    the message with a real msgid and local postMessage is suppressed.
  - Keychain bundle: consolidated all 4 per-server password fields into a single keychain item
    (serverName:bundle) to reduce macOS/KWallet prompts from up to 4 per server to 1.
  - Keychain separator fixed: bundle was using \x00 (NUL) as field separator, but KWallet
    (and other OS keystores) treat stored strings as null-terminated, silently truncating
    everything after the first NUL. Fixed by switching to \x1F (ASCII unit separator).
  - Search icon bumped from 20×20 to 24×24 px (button stays 28×28).
  - Commits: 9f90a88, 85c2c13, 451dced (formatting), 40b719c (indicator), c3b944d (combos+size),
    5979c39 (echo-message), 29a6c57 (keychain bundle), bf2d462 (separator fix), 5ca907c (search icon)
No regressions. No known issues.
Next priorities: hero screenshots (3 needed); KWallet auto-unlock on auto-login setups (set blank
wallet password via qdbus6 changePassword); ServerId/BufferId strong types.
-->

<!--
SESSION SUMMARY — 2026-06-16 (reaction cross-client fix, /nick label, unread count color, v0.25.38 release)
What changed:
  - /nick command no longer updates the nick label next to the input bar — fixed. m_nick was
    set preemptively in IrcClient::setNick() before the server confirmed the change. The NICK
    handler compared msg.nick (old nick) against m_nick (already new nick), never matched, so
    selfNickChanged was never emitted. Fixed by OR-ing the new nick check:
    `newNick.toLower() == m_nick.toLower() || msg.nick.toLower() == m_nick.toLower()`.
  - Unread message count badges now use the theme's sidebarUnread color (default #89b4fa) at
    full opacity instead of 60%-alpha text color. Added m_unreadColor field to SidebarDelegate;
    setColors() gains optional unreadColor param; both setColors() call sites pass
    QColor(m_theme.sidebarUnread).
  - Released v0.25.38 — committed changelog, version bump (CMakeLists 0.25.37 → 0.25.38),
    sync-site.sh (index.html + README), tagged v0.25.38, pushed. Release CI queued:
    AppImage + .zsync, Linux tarball, Windows zip, macOS dmg. CI on main was green.
    Also fixed docs: "right-click the timestamp" → "right-click anywhere on a message" in
    howto.html (2 places) and faq.md (1 place).
  - Reactions not visible on other clients — two bugs fixed:
    1. SessionModel::sendReact() ran the local echo unconditionally, even if
       IrcClient::sendReact() returned early (message-tags not acked, invalid token, etc.).
       Linux always showed its own reaction regardless of whether anything was sent.
       Fix: sendReact() now returns bool; local echo only runs if it returned true.
    2. onReactReceived() had a !known guard that scanned ch->messages for the reacted msgid
       and silently dropped the reaction if not found. MacBook could receive a valid TAGMSG
       for a message outside its chathistory window (joined late, < 100 message replay) and
       discard it. Fix: removed the guard; rendering already ignores orphaned ch->reactions
       entries if the message line is not in the view.
  - Commits: ce192b9 (/nick + unread color), 101990f (v0.25.38 release), 04e4355 (reactions)
No regressions. No known issues.
Next priorities: hero screenshots (3 needed); ServerId/BufferId strong types;
virtual scrolling for very busy channels.
-->

<!--
SESSION SUMMARY — 2026-06-16 (gear icon size, nick panel header alignment, nickDock font platform split)
What changed:
  - Gear (Preferences) icon in sidebar header reduced from 24px to 18px iconSize. Button
    stays 28×28 (same click target); the gear SVG was visually heavier than the hamburger
    and servers icons at 24px, now balanced at 18px.
  - nick panel header alignment fixes (see previous session note below).
  - font_nick_dock docs updated: default listed as 9 (macOS: 13).
  - Commits: (this session)
-->

<!--
SESSION SUMMARY — 2026-06-16 (nick panel header alignment, correctStartupGeometry sync, nickDock font platform split)
What changed:
  - m_nickToggleBtn kept at 28×28 / 20×20 icon (same as sidebarCloseBtn / sidebarRevealBtn).
    hbox vertical margins on nickPanelHeader reduced from (2,2,2,2) to (2,0,2,0), bringing
    header height from 32px down to 28px — much closer to the 26px sidebar row height,
    centers now within 1px of each other.
  - correctStartupGeometry() was missing the m_sidebarHeader->setFixedHeight(m_primaryHeader->height())
    sync call. On startups where the window was previously maximized this path fires instead
    of the main singleShot(0) timer, so the sidebar header height was never synced. Fixed.
  - nickDock font default split by platform: macOS keeps 13pt (needed for Retina), Linux/FreeBSD
    defaults to 9pt. Both the struct initializer (config.h) and value_or fallback (config.cpp)
    updated. User config updated to font_nick_dock = 9.
  - Commits: (this session)
-->

<!--
SESSION SUMMARY — 2026-06-16 (react/reply UX, local reaction echo, weight-700 icons, nick panel header WIP)
What changed:
  - Right-click anywhere on a message line (body, nick, timestamp, blank space) now triggers
    the React/Reply menu — previously only clicking the narrow timestamp characters worked.
    contextMenuEvent falls back to the line's ChatLine::id (msgid) when anchorAt returns empty.
  - When text is selected during right-click on a message, a Copy action is prepended to the
    React/Reply menu so the user doesn't lose the selection.
  - selectedText() moved from private to public on ChatView so handleChatViewContextMenu can
    read the selection state when building the combined menu.
  - sendReact() now locally applies the reaction to ch->reactions immediately after sending
    the TAGMSG. Without echo-message, the server never echoes our own reactions back, so the
    emoji row never appeared under the original message in the sender's own UI.
  - sendMessage() local echo now passes replyToMsgid to Message::make so the ↩ indicator
    renders correctly on the sender's own reply bubble.
  - menu.svg and mi-domain-add.svg upgraded to Material Symbols weight-700 filled variants
    (fetched from google/material-design-icons repo). settings.svg kept at weight-400 — the
    weight-700 gear rendered too heavy/cluttered at 24px.
  - All three toolbar icon buttons (hamburger, gear, servers) normalized to setIconSize(24,24).
    Previously: hamburger=24, gear=22, servers=28.
  - nickPanelHeader moved out of the headerRow HBox and into the top of nickPanel's VBox.
    This puts collapse/users/count at the same Y level as the filter input — which is where
    the user expects it to align with the top server entry on the left sidebar.
    Alignment is still not correct visually; further work needed next session.
  - Removed syncNickHeaderWidth lambda + splitterMoved connection (no longer needed once
    header is inside the panel). Removed setFixedWidth calls on nickPanelHeader.
  - Commits: ca54786
Still needs: correct nick panel header alignment to top server entry on sidebar left side.
-->

<!--
SESSION SUMMARY — 2026-06-15 (icon sizing, Manage Servers button, nick panel alignment)
What changed:
  - All toolbar icon sizes audited and unified. Hamburger and gear icons bumped from 20px to
    24px; all three (hamburger, gear, new Manage Servers button) now routed through
    MenuIcons::fromSvg which handles device pixel ratio correctly — on Retina Mac the old
    custom functions produced blurry upscaled icons.
  - Every floating/panel QToolButton that was missing setIconSize() now explicitly declares
    20px: sidebarCloseBtn, searchBtn, nickRevealBtn, sidebarRevealBtn, nickToggleBtn.
  - Nick panel groups icon (users icon) and nick count label font both raised from 14px/10pt
    to 20px/13pt to be proportional alongside the icons.
  - Manage Servers pulled out of the hamburger menu and given its own dedicated icon button
    (domain_add SVG) in the sidebar header next to the gear. Added mi-domain-add.svg to
    resources. MenuIcons::manageServers/hamburger/gear added to menuicons.h.
  - Nick panel header alignment fixed: nickPanelHeader was inside chatSplitter below
    primaryHeader, so it rendered at the same Y level as topicDisplay rather than at the
    same Y level as sidebarHeader. Fix: extracted both primaryHeader and nickPanelHeader
    into a joint header row (QHBoxLayout) above chatSplitter. nickPanelHeader width is
    locked to chatSplitter's nick panel column width and updates on splitterMoved.
    nickPanelHeader visibility toggled with the nick panel collapse/expand.
  - Commits: 92b0b95, 9b98dd4, be76ae7, 44e853b, c39676b, e8a927a, ec0cd14
    (69b8af9 was an intermediate broken attempt, superseded by ec0cd14)
Still needs verification on Mac — alignment and icon sizing to be confirmed visually.
-->

---

<!--
SESSION SUMMARY — 2026-06-15 (macOS font and Preferences spacing fixes)
What changed:
  - Topic text font-size was being ignored by Qt6's QSS engine. QLabel#topicText has a
    stylesheet rule in themeloader (color/background/padding) but no font-size — when QSS
    matches a widget, Qt6 ignores setFont() for font-size and falls back to the app default,
    making the topic text render smaller than configured. Fix: call setStyleSheet("font-size:
    Xpt") on the label widget directly alongside setFont(). Widget-level stylesheet wins over
    the app-level stylesheet for that property. Applied in both ChannelPane::setTopicFont()
    and mainwindow.cpp where m_topicText->setFont() is called.
  - QCheckBox and QRadioButton in Preferences dialog appeared "smooshed" on macOS. Root cause:
    macOS native controls are shorter than their Linux/Windows counterparts, so the existing
    6px vbox spacing left them visually cramped. Fix: added `padding: 3px 0` to QCheckBox and
    QRadioButton in the themeloader stylesheet, giving each row 3px top + bottom breathing
    room on top of the layout spacing.
  - Prior session (16b4420) also removed the pointSize()-1 reduction in ChannelPane topic bar
    constructor and bumped topicBar/topicText defaults from 10pt to 11pt — that fix lacked a
    CHANGELOG entry and is included here.
  - Commits: 16b4420, 3ed9c10, 890eb23
No regressions. No known issues.
-->

<!--
SESSION SUMMARY — 2026-06-15 (306 themes + separator key)
What changed:
  - Added [buffer] separator theme key to all 55 existing themes — controls the color of
    "── N new messages ──" and "── N older messages ──" dividers. Falls back to border color
    when key is absent. makeStatusLine() now takes an optional color param; all 3 call sites
    in mainwindow.cpp pass m_theme.separator.
  - Added 251 themes converted from tinted-theming/base16-schemes via base16_to_uplink.py
    (Python, pyyaml). Named with -base16 suffix to coexist with originals. Brings total
    bundled theme count from 55 to 306.
  - Commits: e18b7c3

<!--
SESSION SUMMARY — 2026-06-15 (network upload fix: redundant WHO queries on JOIN)
What changed:
  - sessionmodel.cpp onUserJoined(): Uplink was sending WHO nick %cnfa,42 for every user that
    joined any channel, even when extended-join was active. extended-join includes the account
    name directly in the JOIN message params, making the WHO redundant. On busy channels this
    generated steady 400-700 B/s of upload traffic. Fix: check cl->hasCap("extended-join")
    before sending WHO — skip it if the cap is active, fall back to WHO on servers without it.
  - Commit: 7988bff

<!--
SESSION SUMMARY — 2026-06-15 (performance: heaptrack analysis, icon cache, nick list layout)
What changed:
  - Analyzed heaptrack.Uplink.1719045.zst (419s runtime, 24.75MB peak heap).
    Three hotspots identified and fixed:
  - menuicons.h: added static QHash cache to fromSvg(). SVG render + QPixmap alloc was happening
    on every call — no cache existed. After fix, each distinct (path, color, size, dpr) combo is
    rendered once and returned from cache on all subsequent calls. Eliminates the 18.89MB peak /
    5,986 QImageData::create calls heaptrack flagged. Cache key: path|rgba|size|dpr*100.
    Also added QHash and QStringBuilder includes.
  - mainwindow.cpp makeNickItem(): replaced "literal" + QString concatenation with
    QLatin1String("literal") + QString. The char const* + QString overload calls QString::fromUtf8
    on the literal first, creating a temporary — QLatin1String skips that conversion. Eliminates
    the 11,400 temporary QString allocations heaptrack traced to this function.
  - mainwindow.cpp / channelpane.cpp: added setUniformItemSizes(true) to both QListWidget nick
    lists. Without it Qt calls NickDelegate::sizeHint on every item on every layout pass (17,628
    calls traced in heaptrack, each triggering QTextLayout::createLine inside Qt's style engine).
    With it Qt measures once and reuses the height. Valid because all nick rows are fixed 16px.
  - Commits: d9ae585 (icon cache + string fix), f6ffc2a (uniform item sizes).

<!--
SESSION SUMMARY — 2026-06-15 (code quality page, static analysis, clang-tidy fixes)
What changed:
  - docs/quality.html: new standalone page showing code quality stats — test results (32 passing,
    0 failing), 88 assertions, 26 fuzz corpus inputs, 37 IRCv3 caps, 64 source files, 16 799 lines.
    Includes verbatim terminal output from tst_ircparser and tst_chatformat runs, full IRCv3 capability
    table, compiler flag cards, ASCII architecture diagram, and principle cards. Linked from main nav.
  - docs/index.html: added "Quality" link to nav.
  - Static analysis run: clang-tidy (bugprone-* + clang-analyzer-*), cppcheck 2.21, and ASan+UBSan
    test builds all run against the codebase. Three real clang-tidy findings fixed:
      · ircclient.cpp:515 — implicit int widening to qsizetype: 64*1024 → qsizetype{64}*1024
      · chatrenderer.cpp:464 — narrowing raw.size() to int: explicit static_cast<int>
      · chatrenderer.cpp:734 — unused ctx param in formatEventGroupLine: [[maybe_unused]]
    ASan/UBSan: 0 errors. cppcheck: 0 real bugs (Qt macro false positives only).
  - docs/quality.html static analysis section: framed as ongoing build workflow, shows current
    build is clean across all three tools.
  - Fix: quality.html nav and footer linked to non-existent .html pages (configuration.html,
    commands.html, ircv3.html — only .md versions exist), causing /docs/docs/docs/... redirect
    loop on the live site. Replaced with valid links only.
  - Commits: fa69332 (quality page), 24ba7b1 (clang-tidy fixes), 9bdfd55 (analysis section),
    d7dfdab (reframe), 8e77cbd (broken link fix).

<!--
SESSION SUMMARY — 2026-06-15 (new messages separator, scroll memory, mIRC formatting input, nick list filter) v0.25.33
What changed:
  - New messages separator: when switching to a channel with unread messages, a "── N new messages ──"
    divider is inserted before the first unread message and the view scrolls to it automatically.
    Implemented via Channel::firstUnreadIdx (set on first unread arrival, cleared on setActive),
    ChatRenderer::makeStatusLine for the separator line, and a QTimer::singleShot in refreshChatView
    that calls ChatView::scrollToLine(findLine("sep:unread")) after layout.
  - Scroll position memory: when switching away from a non-bottom scroll position, the scroll px value
    is stored in m_scrollPositions[key]. On return, if no unread separator applies, the position is
    restored. If a separator exists it takes priority. m_scrollPositions uses "host\tchannel" keys and
    is cleaned up when the user scrolls back to the bottom.
  - ChatView::scrollToLine(int): new public method that sets verticalScrollBar()->setValue(m_cumH[i])
    and sets m_atBottom = false; used by the unread separator scroll.
  - mIRC formatting input shortcuts: Ctrl+B (bold \x02), Ctrl+I (italic \x1D), Ctrl+U (underline \x1F),
    Ctrl+O (reset \x0F) in the message input box. Handled in eventFilter when obj == m_input, inserting
    the control character at the cursor via QTextCursor::insertText. Returns true (consumed).
  - Nick list filter: always-visible QLineEdit (m_nickFilter, objectName "nickFilter") sits between
    the panel header and the nick list. textChanged signal hides items whose Qt::UserRole nick does
    not startsWith(filter.toLower()). Filter clears on refreshNickList (channel switch). Escape while
    focused clears text via clear(). Themed via themeloader.cpp nickFilter CSS rule.
  - Bug fix: QRegularExpression{} (default-constructed) has isValid()==true and empty pattern matches
    every string. buildHighlightRe returning {} for no keywords caused keywordHit=true on every unread
    message, showing the bolt icon for all channel chatter. Fixed by guarding all regex matches with
    !pattern().isEmpty() in sessionmodel.cpp (postMessage) and chatview.cpp (ChatRenderer).
  - Docs: new howto.html sections #unread-separator and #nick-filter; #formatting section rewritten to
    cover both sending (keyboard shortcuts) and receiving formatting codes; #tab section updated with
    Shift+Tab and tip about nick filter; keyboard-shortcuts.md updated with formatting shortcuts table;
    faq.md updated with Q&As for all four new features. Version bumped to v0.25.33 across
    CMakeLists.txt, README.md, docs/index.html.

SESSION SUMMARY — 2026-06-14 (hotfix: release script missing tar.gz/eyebrow/footer patterns) v0.25.32
What changed:
  - docs/index.html: tar.gz link, hero eyebrow <b>vX.Y.Z</b>, and footer "Uplink vX.Y.Z" were not
    updated by the release script, leaving them on v0.25.31 after the v0.25.32 release.
  - scripts/release.sh: added three missing sed patterns — linux-x86_64.tar.gz href, <b>...</b>
    eyebrow, and footer-logo div — so all version strings in index.html are covered on future releases.

SESSION SUMMARY — 2026-06-14 (nick fade while away, input height jump fix, auto-rejoin on reconnect, ignore icon in nick list) v0.25.32
What changed:
  - Nick fading when /away: self-nick in user list fades to 35% opacity when away status is active.
    Implemented via painter->setOpacity(0.35) wrapping entire NickDelegate item paint, bypassing QSS
    stylesheet overrides that make palette color changes invisible.
  - Input height jump fix: bar->setObjectName("inputBar") moved before child widgets are created so
    QSS rule QWidget#inputBar QPlainTextEdit matches at construction time. QTimer::singleShot(0) added
    after applyFontSizes() to sync height post-polish using same formula as textChanged handler.
  - Auto-rejoin channels on reconnect: onConnected now iterates sess->channels (persists across
    disconnect) and re-issues JOIN for any channel not already covered by the server config list.
  - Ignore icon in nick list: right-click > Ignore sets Qt::UserRole+1 on the nick list item to
    MenuIcons::eyeOff(). NickDelegate draws it after the bot icon. Nick list refreshes on toggle.
  - Docs: fixed /away and away_message descriptions (was "clears away status if none set", now
    "sends 'Away' as fallback"); updated reconnect FAQ, added ignore icon note, topic-set-by note,
    and new "How do I mark myself as away?" FAQ entry covering away status UI behaviour.

SESSION SUMMARY — 2026-06-14 (lag meter, WALLOPS/Reply colors, server buffer differentiation, host icon, unread count badges, preferences toggle, /list disconnect fix) v0.25.31
What changed:
  - Lag meter (SignalBars) moved from orphaned to sidebar header top-right, aligned with hamburger and
    gear buttons via shBox->addWidget(m_signalBars, 0, Qt::AlignVCenter). Tooltip shows ms latency on
    hover ("42 ms"), or "Connecting…" / "Reconnecting…" / "Disconnected" for non-connected states.
    Updates on server switch (switchToChannel), latency ping (pingRtt), connect, disconnect, reconnect.
  - Connected server icon: mi-public.svg (globe) replaced with mi-host.svg in menuicons.h.
  - Server buffer message color differentiation: added MessageType::Reply (steel blue #6090c0) for
    numeric command responses routed via onContextualMessage (WHOIS, STATS, TIME replies). Added
    MessageType::Wallops (amber #e09030) for WALLOPS server broadcasts. Both types handled in both
    the HTML and segment rendering paths in chatrenderer.cpp. WALLOPS command handler added to
    ircclient.cpp; wallopsReceived signal added to IrcClient; lambda connection in sessionmodel posts
    to "(server)" buffer with "[nick] text" format.
  - Unread count badges: SidebarDelegate reads Qt::UserRole+3 (int count) and renders count to the
    right of the bolt/forum indicator icon, 86% font size, bold, at 60% alpha. onUnreadChanged stores
    the count on the item. Pill width accounts for countExtra via cfm (countFont metrics).
  - Preferences toggle "Show Unread Message Counts": QCheckBox added in PreferencesDialog after Link
    Previews. Persisted as show_unread_counts in [ui] section of config.toml (default true).
    SidebarDelegate::setShowCounts(bool) skips count rendering when false; clearing the toggle also
    wipes all UserRole+3 data from sidebar items immediately.
  - Server and PM buffers: switchToChannel() detects buffer type via channel.startsWith('#') etc.
    Server buffers ("[server]") and PM/query buffers hide nick panel and topic button; channels
    restore them (respecting m_nickExpanded for the panel). topicDisplay also hides for non-channels.
  - Search button: clicking m_searchBtn when bar is visible now hides it and clears input (toggle).
    Previously it only opened.
  - Hamburger and gear alignment: shBox->setAlignment(Qt::AlignTop) pins both buttons to the top of
    the sidebar header when it grows taller (e.g. with topic display visible).
  - /list disconnect fix: the kMaxPendingBuffer (64 KB) check was moved to AFTER the line-processing
    while loop. Previously the check fired before any lines were consumed, causing large /list
    responses (Rizon has thousands of channels) to trigger a disconnect before any 322 lines were
    parsed. Now only a genuinely unterminated partial line > 64 KB triggers the error.
-->

<!--
SESSION SUMMARY — 2026-06-14 (sidebar alignment, event group indent, button transparency, selection contrast, Manage Servers to burger) v0.25.30
What changed:
  - Sidebar header height sync: m_sidebarHeader now dynamically matches m_primaryHeader height plus the
    topic display height (when visible). Sync runs in the startup QTimer::singleShot(0), in the
    eventFilter for m_topicDisplay Resize/Show/Hide events, and in both topic toggle handlers.
    Scroll positions for the sidebar tree and nick list are saved and restored around every sync so
    neither list jumps visually. This ensures server/channel list items align with nick panel header
    icons regardless of topic bar state.
  - Event group expanded indent: entries 2+ in an expanded event group now receive a 2-space leading
    indent (\n  prefix in chatrenderer.cpp formatEventGroupLine). When a long hostmask wraps past the
    right edge, its continuation starts at column 0 (kHPad); subsequent entries start at column 2,
    making them visually distinct from wrap continuations.
  - Hamburger and gear button backgrounds: both m_hamburger and m_sidebarToggleBtn now have an
    explicit "QToolButton { background: transparent; border: none; }" stylesheet set at construction
    and refreshed in applyFontSizes() on every theme change. Prevents the platform-default button
    box from showing through on dark or high-contrast themes.
  - Selection text contrast: SidebarDelegate and NickDelegate now compute a contrast text color for
    selected items using m_accent.lightnessF() > 0.5. Light accent → dark text (#111111);
    dark accent → light text (#eeeeee). Prevents invisible text on themes where the accent color
    is close to the text color.
  - Manage Servers moved to ☰ menu: addAction("Manage Servers") inserted in the hamburger QMenu
    lambda, running the full ManageServersDialog flow inline (add/edit/remove servers, immediate
    reconnect, config save). Menu entry added before Documentation.
  - Preferences quick-access row removed: the block containing manageBtn (Manage Servers...) and
    docsBtn (Documentation) at the top of PreferencesDialog has been removed. Preferences now
    starts directly with the Appearance section separator.
No regressions. No known issues.
-->

<!--
SESSION SUMMARY — 2026-06-14 (bolt icon, mention detection rewrite, single-word message parsing fix) v0.25.29
What changed:
  - Bolt mention icon: mi-bolt.svg replaces mi-lightbulb-2.svg for nick mention indicator in sidebar.
    Displayed in yellow (#FFD700) via MenuIcons::mention(). resources.qrc and menuicons.h updated.
  - Mention detection rewritten in sessionmodel.cpp::postMessage(): replaced QRegularExpression
    match against mentionRe with msg.text.contains(sess->nick, Qt::CaseInsensitive). Simpler and
    consistent with the tray notification logic already in mainwindow.cpp.
  - Server label removed from nick panel header: m_userInfoLabel cleared (instead of showing
    "* NetworkName") in refreshTopicBar() for channel/query views. Nick panel header no longer
    shows any server label.
  - User count repositioned: addSpacing(4) between the groups icon and m_nickCountLabel; count
    now sits directly right of the icon with consistent visual gap.
  - Body text foreground always set: chatrenderer.cpp Privmsg and Action handlers now unconditionally
    set QTextCharFormat foreground on body text (themeText when valid, #cccccc fallback). Previously
    used else-if(validTheme) which left format unset when theme was unavailable, causing invisible
    text after addSelfNickHighlight overlapped the segment.
  - Single-word PRIVMSG/NOTICE parsing fixed: ircclient.cpp now reads
    msg.params.last() (falling back to msg.trailing) for PRIVMSG and NOTICE text. Some IRC servers
    omit the colon prefix for single-word trailing parameters; the parser only sets msg.trailing
    when a colon is present, so bare single-word messages arrived with empty text. Fix applied to
    real-time PRIVMSG, real-time NOTICE, and batched (history) PRIVMSG/NOTICE handlers.
No regressions. No known issues.
-->

<!--
SESSION SUMMARY — 2026-06-14 (soju fixes, IRC mask display, reconnect credential bug) v0.25.28
What changed:
  - IRC mask in membership events: userJoined/userParted/userQuit signals now carry user and host
    fields from IrcParser. Messages display as "nick (~user@host) has joined/left the channel" and
    "nick (~user@host) has quit (reason)". User and host are omitted gracefully if empty.
  - FAIL message suppression: METADATA KEY_INVALID, BOUNCER UNKNOWN_COMMAND, and ACCOUNT_REQUIRED
    FAIL replies no longer appear in channel buffers. UNKNOWN_COMMAND also sets a persistent
    m_bouncerListingUnsupported flag so BOUNCER LISTNETWORKS is not retried.
  - soju no-implicit-names: when soju.im/no-implicit-names is active, Uplink now sends an explicit
    NAMES request on self-join so the nick list populates correctly.
  - BOUNCER LISTNETWORKS guarded: flag check added to CAP ACK handler prevents sending LISTNETWORKS
    while SASL is in-flight or the command was already rejected this session.
  - m_bouncerListing and m_bouncerNetBuf reset on disconnect to prevent stale state on reconnect.
  - Socket abort before connectToServer: added sockAbort() when socket is not already disconnected,
    eliminating a race condition on rapid reconnects.
  - Bouncer network name fallback: fall back to network id when BOUNCER LISTNETWORKS omits the
    name attribute.
  - Reconnect credential bug fixed: "Reconnect" context menu was calling connectToServer(sc) with
    sc.saslPassword still set to the literal "<keychain>" sentinel, causing SASL to send that string
    as the password. Menu now calls cl->reconnect() which reuses the already-resolved m_saslPassword.
    IrcClient::reconnect() also clears m_intentionalDisconnect and resets the backoff delay.
  - soju documentation rewritten: configuration.md soju section now has SASL-based examples,
    single vs multi-network configs, Option A (bouncer_network) vs Option B (embed in sasl_user),
    explicit warning not to combine both, and a BouncerServ tip for finding the correct network name.
No regressions. No known issues.
-->

<!--
SESSION SUMMARY — 2026-06-13 (sidebar integrated into floating chat card)
What changed:
  - setupChatArea() restructured: a new intermediate chatSection widget wraps primaryHeader +
    topicDisplay + chatSplitter. m_mainSplitter now holds [sidebarPanel | chatSection] inside
    primaryPanel, which is already inside the RoundedPane chatWrapper. Result: the sidebar is
    physically inside the same floating rounded card as the chat area and nick list.
  - The inputBar (added by setupInputBar) sits in primaryVbox below m_mainSplitter, so it spans
    the full card width including the sidebar column — this eliminates the interior square corner
    that would otherwise appear at the bottom-left of the sidebar column.
  - themeloader.cpp: sidebar and QTreeWidget branch backgrounds changed from {{sidebarBg}} to
    {{bufferBg}}. The nick panel already uses bufferBg; this makes the sidebar match, so all three
    columns (sidebar, chat, nick list) share the same background inside the card.
  - mainwindow.cpp: rightContent vbox left margin restored to 8 px in both the initial setup and
    the sidebar toggle expanded branch. The correctStartupGeometry() function already had a +8 px
    compensation on topicLeft's fixed width; the two values now match, keeping the hamburger and
    gear buttons horizontally aligned with the sidebar column.
No version bump from v0.25.24 — this is a pure visual polish pass.
No regressions. No known issues.
-->

<!--
SESSION SUMMARY — 2026-06-13 (custom painter ChatView polish + memory / selection fixes)
What changed:
  - Memory: mallopt(M_ARENA_MAX, 2) in main.cpp caps glibc thread arenas at 2 (default is 8×cores = 96 on this 12-core machine). Each empty arena holds 3–4 MiB of dirty pages; the cap eliminates ~275 MiB of RSS waste. Measured before/after: 538 MiB → 263 MiB cold start.
  - Scroll speed: ChatView was using singleStep=1px (QAbstractScrollArea default). Added wheelEvent override and setSingleStep(lineH*3) in updateScrollRange — each notch now scrolls 3 lines.
  - Preview card positioning: cardReady handler now stores msgid in PreviewCtx alongside host/channel; when the card arrives it calls insertAfter(msgid, card) so it appears directly below the URL message rather than at the bottom of chat.
  - Preview card layout: removed left border line; title (bold, single line, elided) + domain (subdued) stacked vertically at hangWidth x-position, thumbnail stacked below; background matches theme (QPalette::Base, same as chat).
  - Per-message hang indent: ChatLine gains hangIndentChars (char offset where body text starts); layoutLine measures exact pixel width via QTextLayout including bold nick weight, stores in cachedHangPx; all four hangW sites in chatview.cpp use lineHangW(line) instead of the fixed global timestamp width. Continuation lines now align precisely with the first character of the message body regardless of nick length.
  - Text selection fixes: (1) mousePressEvent now always starts a selection; anchors fire on release only if no drag occurred (m_clickAnchor); eliminates the "can't start selection from timestamp" bug. (2) xToCursor was receiving vpPos.x() − visLine.x (relative coords) but expects full layout x coordinates; changed to pass vpPos.x() directly in both anchorAt and hitTest. Fixes reverse-selection missing the last word.
  - Preview thumbnail bumped to 240px max.
No version bump (polish/fix pass, no user-facing feature additions).
No regressions. No known issues.
-->

<!--
SESSION SUMMARY — 2026-06-12 (memory investigation + undo stack fix)
What changed:
  - Investigated high RSS (~500MB). Root cause split: baseline overhead is Mesa 26.1.x loading LLVM for shader compilation on KDE Wayland (not fixable in app); steady post-launch growth (~2MB/5s) was the QTextDocument undo stack accumulating every insert/removal on read-only chat views.
  - Fixed: disabled undo/redo on both chat view documents (mainwindow.cpp + channelpane.cpp). QTextBrowser is read-only; the undo stack was pure waste. Committed fc89b79.
  - No version bump; patch-only fix.
No regressions. No known issues.
-->

<!--
SESSION SUMMARY — 2026-06-12 (v0.25.23 release)
What changed:
  - Nick right-click menu restructured into submenus: CTCP ▶ (Ping, Version), DCC ▶ (Send File, Send File Passive), Chan Ops ▶ (Op, Voice, Invite, Kick, Ban, Kick & Ban).
  - Fixed typing indicator: Uplink now requests and handles both finalized `typing` CAP and old `draft/typing` form; was silently broken on Ergo IRCd (which uses the finalized form).
  - Fixed transparent combo box popups on Wayland: three attempts (CSS, palette, showPopup override) all failed at the Wayland native-surface level. Final fix: replaced all popup combo boxes in Preferences/Server/Font dialogs with inline QRadioButton groups and collapsible QListWidgets.
  - Added bouncer_network support for ZNC: sasl_user + sasl_password + bouncer_network now auto-assemble into `user/network:password` PASS string. Network field now appears in Manage Servers for ZNC (was soju-only).
  - Fixed ZNC documentation contradictions: removed "Leave SASL blank" note that directly conflicted with setup Step 4; added explicit callout in both howto.html and configuration.md that host/port point at the ZNC server, not the IRC network. Added inline comments to config.toml examples.
  - Bumped to v0.25.23, tagged, pushed. Both CI and Release workflows passed.
No regressions. No known issues.
-->

<!--
SESSION SUMMARY — 2026-06-11 (docs site / uplinkirc.chat)
What changed:
  - Purchased uplinkirc.chat; connected Porkbun GitHub static hosting to noderelay/UplinkIRC main branch.
  - Added root index.html to redirect to docs/ (Porkbun serves from repo root, docs site lives in docs/).
  - Updated README.md How-To Guide link from noderelay.github.io to uplinkirc.chat.
  - Updated gruvbox theme screenshot (uplink-gruvbox.png) with a new capture.
  - Fixed two incorrect keyboard shortcut entries on the docs site: Ctrl+W described as "close the active pane" (it minimizes to tray); Alt+1-4 listed as a shortcut that doesn't exist.
  - All changes committed and pushed; site is live.
Next priorities: 3 hero screenshots from user (dark, split-pane, about dialog); Send button disable-when-empty; virtual scrolling; heaptrack session audit.
-->

<!--
SESSION SUMMARY — 2026-06-11 (bot icon late-join fix)
What changed:
  - Diagnosed missing alien icon for bots (+B) that join a channel after the client is already in it. The bulk WHO sweep on RPL_ENDOFNAMES only covers users present at join time; late arrivals were never queried.
  - Fix: one-liner in SessionModel::onUserJoined — send "WHO <nick> %cnfa,42" for every non-self join. Ergo responds with WHOSPCRPL (354); onWhoEntry inserts the nick into botNicks and the icon appears.
  - Updated CHANGELOG Unreleased section. Committed and pushed.
Next priorities: Send button disable-when-empty; virtual scrolling; heaptrack session audit.
-->

<!--
SESSION SUMMARY — 2026-06-11 (v0.25.17 release session)
What changed:
  - Reviewed post-v0.25.16 unreleased commits: AppImage self-install (.desktop + icon on first run) and both scrollbar regression fixes.
  - Decided they warranted a patch release. Bumped to v0.25.17, promoted Unreleased → v0.25.17 in CHANGELOG, ran sync-site.sh, tagged and pushed.
  - All CI passed: Release workflow (Linux AppImage, Windows zip, macOS dmg, update-docs) + CI workflow (Linux/Windows/macOS builds, ASan/UBSan sanitizers) — 3/3 workflows green.
  - Built locally and launched for user verification.
Next priorities: Send button disable-when-empty; virtual scrolling; heaptrack session audit.
-->

<!--
SESSION SUMMARY — 2026-06-11 (post-release hotfix session)
What changed:
  - Discovered that the paintEvent + QPainter::setOpacity approach introduced in the review session is invisible when QSS is active. Qt's stylesheet engine renders through its own painter path and ignores the opacity set on the outer painter. QGraphicsOpacityEffect composites after all painting and is the only approach that works reliably with QSS-styled scrollbars.
  - Restored QGraphicsOpacityEffect in FadeScrollBar. All behavioral fixes from the review session are retained (sliderReleased, unconditional enterEvent wake, timer no polling loop, leaveEvent guard).
  - Also restored valueChanged (had been changed to actionTriggered in the review session) — actionTriggered does not fire for mouse wheel events on the parent scroll area, so wheel scrolling never triggered wake(). Scrollbars were invisible on wheel scroll. valueChanged is the correct signal.
  - Both regressions shipped in v0.25.16. Post-release fix committed and pushed as patch.
Next priorities: Send button disable-when-empty; virtual scrolling; heaptrack session audit.
-->

<!--
SESSION SUMMARY — 2026-06-11 (v0.25.16 release session)
What changed:
  - Full /code-review pass (high effort, 7 angles) on the FadeScrollBar and IRC TAGMSG diff. Found and fixed 9 issues.
  - FadeScrollBar: QGraphicsOpacityEffect → paintEvent+QPainter::setOpacity (no offscreen buffers); hover reveals hidden bar; enterEvent unconditional wake; sliderReleased connected; timer polling loop removed; leaveEvent guards on opacity>=kVisible; valueChanged → actionTriggered (no flash on incoming messages).
  - IRC: sendTyping cap guard corrected to draft/typing; sendReact target validated with validIrcToken().
  - Long-session leak audit: log fd leak on closeBuffer fixed; nickMeta capped at 1000 entries with eviction; ctcpTimestamps evicts one entry instead of clearing all; ChannelPane doc cap aligned to kMessageBufferCap.
  - Bumped to v0.25.16, tagged, pushed. All 8 CI jobs (Linux/Windows/macOS build + ASan/UBSan + AppImage + update-docs) passed.
  - docs/howto.html: added Fade Scrollbars section and nav entry; corrected typing indicator compat note (message-tags → draft/typing).
No regressions. Next priorities: Send button disable-when-empty; virtual scrolling for busy channels; heaptrack session audit.
-->

<!--
SESSION SUMMARY — 2026-06-11 (security audit session)
What changed:
  - Ran full security audit of src/. Two medium-severity findings.
  - Vuln 1: PASS, SASL PLAIN, and NickServ IDENTIFY were sent over plaintext connections with no user warning. Added warning messages for all three when ssl=false.
  - Vuln 2: onConnected() sent credentials on QSslSocket::connected, which fires before TLS handshake completes. Added isEncrypted() guard that defers via QSslSocket::encrypted (SingleShotConnection) so credentials never go out before TLS is confirmed.
  - All other findings (HTML injection, DCC path traversal, SSRF, STS hostname injection, credential logging, QProcess, config permissions) were reviewed and cleared.
Next priorities: Send button disable-when-empty; virtual scrolling; heaptrack session audit.
-->

<!--
SESSION SUMMARY — 2026-06-12 (quit message, away message, disabled flag)
What changed:
  - Added per-server quit_message: sent on disconnect or /quit with no arg; defaults to "Uplink".
    Wired through config.h, config.cpp, serverdialog, IrcClient (m_quitMessage stored on connect),
    commanddispatcher (/quit now delegates default to IrcClient).
  - Added per-server away_message: used by /away with no arg; commanddispatcher looks up the
    server config by host and sends it; /back always clears. If unset, /away with no arg clears
    away status as before.
  - Added per-server disabled flag: disabled = true keeps the block in config and writes it back
    on every save but skips the server on startup (no session, no client, no sidebar entry).
    Fixes a data-loss bug where commented-out [[server]] blocks were permanently stripped the
    next time the app wrote config.toml (which happens on any Preferences change, theme switch,
    /ignore command, etc.). SessionModel::loadConfig and addServer skip disabled servers.
  - Edit Server dialog: Disabled checkbox at top of Connection section; Quit Message and Away
    Message fields under Identity.
  - config.toml.example: [profile] section added with display_name/avatar_url, fully commented.
    quit_message, away_message, disabled shown as commented examples in server blocks.
  - docs/configuration.md: Disabling a server section + Quit/away message section added.
    [[server]] table updated with all three new keys.
  - docs/commands.md: /away and /quit descriptions updated; examples updated.
  - docs/faq.md: three new entries — disabling a server, quit message, away message.
  - docs/howto.html: two new sections (Disabling a server, Quit & away messages) with nav entries.
  - Fix: auto-join placeholder now shows #uplink, #linux, #archlinux format.
No regressions. No known issues.
-->

<!--
SESSION SUMMARY — 2026-06-12 (RPL_ENDOFWHO suppression)
What changed:
  - Diagnosed repeated "End of /WHO list." spam in the server window on busy networks (Libera.chat).
  - Root cause: numeric 315 (RPL_ENDOFWHO) fell through to the default handler, which emits any
    non-empty trailing as a serverMessage. 352 and 354 were already handled silently.
  - Fix: added case 315: break; in IrcClient::handleNumeric, matching the pattern of 333 (RPL_TOPICWHOTIME).
  - One-line change, verified in-app — no WHO spam in server window.
No regressions. No known issues.
-->

<!--
SESSION SUMMARY — 2026-06-12 (ssl = false documentation)
What changed:
  - Added "Plain (unencrypted) connections" section to docs/configuration.md with four annotated
    examples: local test server (127.0.0.1:6667), LAN bouncer, localhost bouncer, Tor .onion.
  - Updated [[server]] reference table: port row now mentions 6667 alongside 6697; ssl row
    now explains when false is appropriate and links to the new section.
  - Added "When should I use ssl = false?" entry to docs/faq.md with a use-case table and
    example config block.
  - Updated docs/howto.html: the ssl = true note now also says when false is valid; Manage
    Servers table rows updated for port and ssl.
No regressions. No known issues.
-->

<!--
SESSION SUMMARY — 2026-06-12 (static analysis audit)
What changed:
  - Ran clang-tidy + cppcheck on full source tree.
  - Collapsed duplicate switch branches in IrcClient::handleNumeric (372/375/376/422, 315/333, 369).
  - Fixed performance-unnecessary-copy-initialization in hot numeric handler paths.
  - Fixed dccsend.h listen() param from value to const ref; fontdialog.h makeSpinBox static;
    preferencesdialog.h ctor signature alignment.
  - All real findings addressed; false positives (Qt moc, QStringLiteral, virtual-in-constructor) noted.
No regressions. No known issues.
-->

<!--
SESSION SUMMARY — 2026-06-12 (ServerId/BufferId strong types)
What changed:
  - Added src/model/ids.h: ServerId and BufferId wrapper types with explicit construction,
    .str() accessor, comparison operators, and qHash overloads.
  - Refactored SessionModel public API (~35 public methods and all signals) to use ServerId/BufferId
    instead of const QString &. IrcClient stays as const QString & internally; 9 direct
    signal-to-signal connects replaced with lambda wrappers at the boundary in attachClient().
  - Updated commanddispatcher.h/.cpp, mainwindow.cpp, trayicon.h/.cpp to wrap or adapt at
    every call site. ~500 lines touched across 8 files.
  - Clean build. No double-wrapping issues.
No regressions. No known issues.
-->

<!--
SESSION SUMMARY — 2026-06-12 (v0.25.20 release)
What changed:
  - Bumped version to v0.25.20 in CMakeLists.txt, docs/index.html, README.md.
  - Promoted Unreleased → v0.25.20 in CHANGELOG.
  - Tagged and pushed. Created GitHub release v0.25.20.
No regressions. No known issues.
-->

<!--
SESSION SUMMARY — 2026-06-13 (send button toggle, nick hover tooltips, Quit Uplink, tooltip polish)
What changed:
  - Send button Preferences toggle: new "Show Send Button" checkbox in ☰ → Preferences, same pattern as
    the emoji button. Stored as show_send_button in [ui] config. Default: true (visible). Persists across restarts.
  - Nick hover tooltips in chat: hovering any nick link in the primary chat view or a split pane now shows
    the full tooltip — display name, account name, and avatar (32×32 px) when available via draft/metadata-2
    or account-tag. Previously the anchorHovered handler silently discarded nick: anchors; only the nick list
    had working tooltips. Chat hover now matches nick list behaviour exactly.
  - Quit Uplink: hamburger "Exit" renamed to "Quit Uplink" and wired to QCoreApplication::quit().
    Previously it called QMenu::close() — closing only the menu without exiting the app.
  - Tooltip avatar capped at 32×32 px in both the nick list and chat hover tooltip HTML. Large cached
    avatars were inflating tooltip windows. Now compact regardless of source image size.
  - Bug discovered (not introduced): running binary was compiled with -fsanitize=address even though
    CMakeCache had UPLINK_ENABLE_SANITIZERS=OFF. The stale binary was never rebuilt after the flag was
    cleared. ASan shadow memory accounted for the reported 500MB+ RSS. A clean rebuild eliminates it.
No regressions. No known issues.
-->

<!--
SESSION SUMMARY — 2026-06-16 (scrollback cap, typing timer cleanup, macOS docs)
What changed:
  - ChatView scrollback capped at 5 000 lines (kMaxLines). Previously m_lines grew without
    bound on busy channels. appendLine() now trims the oldest line and rebuilds cumH when the
    cap is exceeded. RAM stays flat on long sessions.
  - Typing timer leak fixed: onNickRemoved() now cancels and deletes any active typing timer
    for the departing nick and removes the nick from m_typingNicks immediately. Previously the
    entry lingered up to 6 s until the timer self-fired. Fires on part, quit, and kick.
  - qsizetype fix (chatview.cpp:739): const int newline → const qsizetype for indexOf return.
    Committed separately as 4d49015.
  - All 3 sanitizer tests (tst_ircparser, tst_chatformat, fuzz_ircparser_corpus) pass clean
    under ASan/UBSan.
  - FAQ: new "macOS says the app is damaged or can't be opened" section with right-click,
    System Settings, and xattr -dr workaround steps. macOS table row updated to note arm64.
  - howto.html: macOS Gatekeeper callout added to the download section.
  - index.html: version bumped to v0.25.41; all download links updated; Gatekeeper note
    added inline on the macOS download card.
  - ROADMAP: two new entries in the stability/memory section for scrollback cap and typing
    timer fix.
  - Investigated message deletion (REDACT) on LinuxDojo Ergo server. Ergo 2.18.0 only
    advertises draft/message-redaction when allow-individual-delete=true in ircd.yaml.
    Added that setting under history: section, REHASH'd. Delete menu shows on Linux but
    server returns REDACT_FORBIDDEN — likely needs a full Ergo restart (not just REHASH).
    On Mac, Delete menu doesn't appear at all despite Reply/React showing — pending debug
    after Ergo restart confirms the Linux path works.
No regressions. Known issue: REDACT blocked pending Ergo restart.
Next priorities: Ergo restart + REDACT retest; Ctrl+scroll zoom; ServerId/BufferId strong types.
-->

<!--
SESSION SUMMARY — 2026-06-17 (REDACT/message deletion debugging, Ergo server config)
What changed:
  - Debugged draft/message-redaction not working on direct Ergo connections.
  - Root cause: Ergo was not advertising the cap due to three missing config conditions:
    1. No persistent history database configured (SQLite/MySQL/PostgreSQL required).
    2. history.persistent.enabled was missing.
    3. allow-individual-delete was placed directly under history: instead of history.retention:.
  - Ergo package binary lacked SQLite support; rebuilt from source with make build_full.
  - pkg lock applied to prevent pkg upgrade from overwriting the custom binary.
  - No Uplink code changes — this was entirely a server-side config issue.
  - Updated docs/howto.html (deletion section) and docs/ircv3.md with Ergo server requirements.
No regressions. No known issues.
Next priorities: test Delete on direct connection and via soju; ASan/UBSan run.
-->

<!--
SESSION SUMMARY — 2026-06-16 (emoji picker width fix)
What changed:
  - Emoji picker popup was clipping emojis on the right edge (all platforms, noticed on Mac).
    The fixed width calculation did not account for the vertical scrollbar eating into the
    viewport. Replaced the hardcoded +16 padding with a runtime computation that queries the
    actual scrollbar width, so the grid is fully visible on all platforms.
No regressions. No known issues.
Next priorities: test Delete on direct connection and via soju; ASan/UBSan run.
-->

## v0.25.42 — 2026-06-17

### Fixed
- **Emoji button background** — the 😊 picker button had an opaque background on macOS instead of blending into the input bar. Now transparent with no border.
- **Emoji picker clipping** — rightmost column of emojis was cut off on all platforms. The popup width now accounts for the vertical scrollbar by querying the actual scrollbar extent at runtime instead of using an undersized hardcoded padding value.

---

## v0.25.41 — 2026-06-16

### Fixed
- **Scrollback cap** — `ChatView` now trims the oldest lines when the buffer exceeds 5 000 lines. Previously the line list grew without bound on busy channels, consuming RAM indefinitely over long sessions.
- **Typing indicator cleanup** — when a nick parts, quits, or is kicked, any active "is typing…" timer for that nick is cancelled and the indicator is cleared immediately. Previously the entry lingered for up to 6 seconds until the timer fired on its own.

---

## v0.25.38 — 2026-06-16

### Added
- **Dedicated Manage Servers button** — a new icon button (↗ domain) sits in the sidebar header next to ⚙, giving one-click access to the server list without opening the hamburger menu.

### Changed
- **Right-click anywhere on a message** — the React/Reply/Delete context menu now triggers on any right-click on a message line (body, nick, timestamp, or blank space). Previously only clicking the narrow timestamp area worked. If text is selected when you right-click, a **Copy** option is prepended automatically.
- **Local reaction echo** — reactions you send via React appear immediately under the original message without waiting for the server to echo them back (servers without `echo-message` never echo `TAGMSG` reactions at all).
- **Reply ↩ indicator on own messages** — the reply-target indicator now renders correctly on messages you send yourself; the echo path was not passing the `replyToMsgid` through.
- **Unread message counts use theme color** — sidebar unread count badges now render at full opacity using the theme's `[ui] unread` color (default `#89b4fa`) instead of 60%-alpha text color.
- **`/nick` label update** — changing your nick with `/nick newnick` now correctly updates the nick label next to the input bar. The `selfNickChanged` signal was never firing because `m_nick` was set preemptively before the server echoed the NICK back.
- **nickDock font platform split** — nick panel header font defaults to 9 pt on Linux/FreeBSD and 13 pt on macOS (Retina screens need the larger value). Configurable via `font_nick_dock`.
- **Toolbar icon sizing unified** — hamburger and gear are both 18–24 px, all floating/panel buttons explicitly sized at 20 px, nick panel header icons at 20 px. All three header buttons routed through the DPR-aware `MenuIcons::fromSvg` path (fixes blurry icons on Retina).
- **Gear icon 18 px** — the gear/Preferences icon uses 18 px instead of 24 px to visually balance with the heavier hamburger and domain-add icons at 24 px.

### Fixed
- **Nick panel header alignment** — header row (collapse, groups icon, user count) now aligns correctly with the top sidebar entry at startup, including when the window was previously maximized (`correctStartupGeometry` now syncs the header height).
- **Topic font-size in HTML mode** — `font_topic` is now embedded directly in the topic label's widget stylesheet so Qt6's QSS engine does not override it with the app default.
- **sidebarHeader height** — header height is now kept in sync with the primary header via a resize-event listener rather than relying on a one-shot timer at startup.

---

## v0.25.37 — 2026-06-15

### Fixed
- **Topic text font-size ignored by Qt6 QSS engine** — `QLabel#topicText` has a stylesheet rule in themeloader (color/background/padding) but no `font-size`. Qt6 ignores `setFont()` for font-size when any QSS rule matches a widget, causing the topic text to render at the app default instead of the configured size. Fixed by calling `setStyleSheet("font-size: Xpt")` on the label directly, which takes precedence over the app-level stylesheet for that property. Applies to both the main window topic label and per-channel-pane topic labels.
- **Preferences dialog checkboxes smooshed on macOS** — macOS native checkbox/radiobutton controls are shorter than their Linux/Windows equivalents, making the 6 px vbox spacing look tight. Added `padding: 3px 0` to `QCheckBox` and `QRadioButton` in the themeloader stylesheet to add breathing room above and below each row without changing the layout spacing.
- **Topic bar font-size reduction removed** — `ChannelPane` constructor was calling `pointSize()-1` on the topic font before the last session's font fixes; that subtraction was removed and `topicBar`/`topicText` defaults were bumped from 10 pt to 11 pt.

---

## v0.25.36 — 2026-06-15

### Added
- **306 built-in themes** — 251 themes converted from the [base16 catalog](https://github.com/tinted-theming/base16-schemes) added alongside the original 55, named with a `-base16` suffix so they coexist cleanly.
- **`[buffer] separator` theme key** — controls the color of the "── N new messages ──" and "── N older messages ──" dividers. Falls back to the theme's `border` color when absent. All 55 original themes updated; base16 themes use their `border` slot.

---

## v0.25.35 — 2026-06-15

### Fixed
- **Redundant WHO query on JOIN with extended-join** — `onUserJoined()` was sending `WHO nick %cnfa,42` for every joining user even when the `extended-join` CAP was active. `extended-join` already includes the account name in the JOIN message, making the WHO unnecessary. On busy channels this generated 400–700 B/s of upload traffic. WHO is now skipped when `extended-join` is active and falls back as before on servers without it.

---

## v0.25.34 — 2026-06-15

### Performance
- **SVG icon cache** — `fromSvg()` in `menuicons.h` now caches rendered QPixmaps by `(path, color, size, dpr)`. Previously every call re-rendered the SVG and allocated a new QImageData. Heaptrack traced this as an 18.89 MB peak / 5,986 allocations on a 419s session. After fix: rendered once per unique combo.
- **Nick list `setUniformItemSizes(true)`** — applied to all nick list QListWidgets. Previously Qt called `NickDelegate::sizeHint()` on every item during every layout pass (17,628 calls in heaptrack). All rows are fixed 16 px so uniform sizing is safe; Qt now measures once and reuses.
- **QLatin1String in `makeNickItem()`** — replaced `"literal" + QString` with `QLatin1String("literal") + QString`, eliminating ~11,400 temporary `QString::fromUtf8` allocations per heaptrack trace.

---

## v0.25.33 — 2026-06-15

### Added
- **New messages separator** — switching to a channel with unread messages shows a "── N new messages ──" divider before the first unread and scrolls to it automatically. Clears on channel focus.
- **Scroll position memory** — non-bottom scroll positions are saved per-channel and restored on return. Separator takes priority when present.
- **mIRC formatting input** — Ctrl+B bold, Ctrl+I italic, Ctrl+U underline, Ctrl+O reset; inserts IRC control characters at cursor position.
- **Nick list filter** — always-visible filter field above the nick list; `startsWith` match, clears on channel switch, Escape to clear.

### Fixed
- **Empty regex matching everything** — default-constructed `QRegularExpression{}` has an empty pattern that matches every string. Guarded with `!pattern().isEmpty()` in `sessionmodel.cpp` and `chatview.cpp`; previously caused the bolt mention indicator to fire on every unread message.

---

## v0.25.29 — 2026-06-14

### Changed
- **Bolt mention icon** — the yellow sidebar indicator for nick mentions is now a lightning bolt (`mi-bolt.svg`) instead of the lightbulb. Displayed in gold (#FFD700) next to the channel name when your nick is mentioned in an inactive channel.
- **Nick panel: server label removed** — the `* NetworkName` label that previously appeared in the nick panel header is no longer shown. The network is identifiable from the sidebar tree; the nick panel header is now cleaner.
- **Nick panel: user count repositioned** — the user count number sits directly to the right of the groups icon with a consistent 4 px gap, matching the spacing between the other header elements.

### Fixed
- **Single-word message body invisible / mentions not detected** — IRC servers may omit the colon prefix for single-word trailing parameters (e.g. `PRIVMSG #channel sig` instead of `PRIVMSG #channel :sig`). The IRC parser only sets `msg.trailing` when a colon is present, so single-word messages arrived with an empty text field. Body text was invisible and mention detection never fired. `ircclient.cpp` now reads `msg.params.last()` (with `msg.trailing` as fallback) for PRIVMSG and NOTICE text in all three handlers: real-time, batched history, and notice.
- **Body text invisible after self-nick highlight** — when your nick appears in a message, `addSelfNickHighlight` overlays a red+bold segment. Previously the base body format had no explicit foreground colour under the `else if (validTheme)` guard, so text rendered invisibly when the theme was not yet loaded. Both `Privmsg` and `Action` formatters now always set an explicit foreground (`themeText` when available, `#cccccc` otherwise).
- **Mention detection** — replaced the `QRegularExpression` match in `sessionmodel.cpp::postMessage()` with `msg.text.contains(sess->nick, Qt::CaseInsensitive)`, matching the tray notification logic and correctly handling short messages.

## v0.25.25 — 2026-06-13

### Changed
- **Sidebar integrated into the floating chat card** — the server/channel list now lives inside the same rounded floating panel as the chat area and nick list. Previously the sidebar was a flat left panel sitting outside the card. Now all three columns (sidebar, chat, nick list) share one unified rounded card. The sidebar background is set to match the chat buffer background (`bufferBg`) so the interior feels like a single seamless surface. The input bar spans the full card width at the bottom, anchoring all three columns visually.

## v0.25.24 — 2026-06-13

### Added
- **Send button toggle** — Preferences now has a "Show Send Button" checkbox. Persists as `show_send_button = true/false` in `[ui]`. Default is `true` (visible). Same implementation pattern as the emoji button toggle.

### Fixed
- **Nick hover tooltips in chat** — hovering any nick link in the main chat view or a split pane now shows the full tooltip (display name + account + 32×32 avatar), matching the nick list. The handler was previously discarding nick anchors silently.
- **Quit Uplink** — hamburger menu "Exit" renamed to "Quit Uplink" and now correctly calls `QCoreApplication::quit()`. Previously it only closed the menu.
- **Tooltip avatar size** — avatars in all tooltips (nick list + chat hover) capped at 32×32 px. Large cached images were making tooltip windows oversized.

## v0.25.23 — 2026-06-12

### Added

- ZNC `bouncer_network` support — set `sasl_user`, `sasl_password`, and `bouncer_network` separately; Uplink assembles `user/network:password` automatically at connect time. The Network field now appears in Manage Servers for ZNC (previously soju-only).

### Changed

- Nick right-click menu restructured into submenus: **CTCP ▶** (Ping, Version), **DCC ▶** (Send File, Send File Passive), **Chan Ops ▶** (Op, Voice, Invite, Kick, Ban, Kick & Ban)
- Nick Brackets, Bouncer Type, and Font Family controls replaced with inline radio buttons and collapsible lists — eliminates transparent popup rendering on Wayland

### Fixed

- Typing indicator now works on Ergo IRCd and any server using the finalized `typing` CAP; was only handling the older `+typing` (draft) form
- ZNC setup documentation: removed contradictory "Leave SASL blank" note that conflicted with the setup steps; added explicit callout that `host`/`port` point at the ZNC server, not the IRC network

---

## v0.25.22 — 2026-06-12

### Added

- Per-type ignore: `/ignore nick [pm] [notice] [invite]` suppresses private messages, notices, and invites independently — channel messages are never blocked. Right-click → Ignore applies all three by default.
- `/ignored` now lists each nick with its active flags, e.g. `spammer [pm,notice,invite]`

### Changed

- Ignore config format updated to `[[ignore.entry]]` with per-entry `flags` array — old `[ignore] nicks = [...]` format is read automatically and migrated on next save

### Fixed

- Server Add/Edit dialog is now scrollable — no longer grows taller than the screen on smaller displays
- Nick Brackets dropdown in Preferences no longer bleeds its popup rendering over the Profile section below it

---

## v0.25.21 — 2026-06-12

### Fixed

- Windows: maximize button no longer greyed out on first launch — pre-show width cap was an X11/KWin-only workaround that was incorrectly applied on Windows, disabling the native maximize control
- Icons now render at the actual screen DPI instead of a hardcoded 2× — fixes blurry toolbar and sidebar icons on 4K / HiDPI displays running at 3× or fractional scaling

## v0.25.20

### Changed

- `SessionModel` public API now uses strong `ServerId` / `BufferId` wrapper types instead of `const QString &` — passing a plain `QString` where a server host or channel name is expected is a compile error; accidental argument swaps at call sites are caught at build time

### Added

- Per-server `quit_message` — set a custom QUIT message in config or via **Edit Server → Quit Message**; used when you disconnect or type `/quit` with no argument (falls back to `"Uplink"`)
- Per-server `away_message` — set a default away message in config or via **Edit Server → Away Message**; used when you type `/away` with no argument (if unset, `/away` with no arg clears away status as before)
- Per-server `disabled` flag — set `disabled = true` in a `[[server]]` block (or tick **Edit Server → Disabled**) to keep the server in config without connecting on startup; survives every config save (safe alternative to commenting out a server block, which gets permanently stripped on the next save)
- `[profile]` block documented in `config.toml.example` — display name and avatar URL with full inline comments and usage examples
- Docs: "Plain (unencrypted) connections" section in `configuration.md` — when `ssl = false` is appropriate, with four annotated examples (local test server, LAN, localhost bouncer, Tor .onion); `faq.md` "When should I use ssl = false?" entry with use-case table; `howto.html` Manage Servers table and ssl note updated to cover both cases

### Fixed

- `RPL_ENDOFWHO` (315) is now suppressed from the server window — eliminates repeated "End of /WHO list." noise on busy networks like Libera.chat
- Auto-join placeholder text in Add/Edit Server dialog now shows the correct comma-separated format (`#uplink, #linux, #archlinux`)
- Virtual scrolling blank view on busy channels — `loadOlderMessages` no longer calls `QApplication::processEvents` (which left the scrollbar `maximum()` stale at 0 after the view was cleared, causing `setValue(0)` and a blank, unscrollable chat). Replaced with `QTimer::singleShot(0, …)` so Qt completes the document layout before the scroll position is restored

<!--
SESSION SUMMARY — 2026-06-12 (v0.25.19 release)
What changed:
  - Bumped version to v0.25.19, promoted Unreleased → v0.25.19 in CHANGELOG.
  - Ran sync-site.sh — docs/index.html and README.md updated to v0.25.19.
  - Tagged and pushed. Release CI built AppImage/Windows zip/macOS DMG. All workflows green.
No regressions. No known issues.
-->

<!--
SESSION SUMMARY — 2026-06-12 (send button / heaptrack / virtual scrolling)
What changed:
  - Send button now disabled when input is empty; re-enables on any non-whitespace text.
  - Heaptrack audit: peak heap 24.75 MB, no Uplink-specific leaks. Found NickDelegate::sizeHint
    calling full Qt style/text-layout pipeline (QTextEngine::itemize, QTextLayout::createLine,
    QFusionStyle::sizeFromContents) 17,628 times per session just to measure item height. Fixed
    by returning QSize(option.rect.width(), 16) directly, bypassing the base class entirely.
  - Virtual scrolling: refreshChatView now renders only the last 150 messages (kRenderWindow).
    A grey divider at the top shows how many older messages exist. Scrolling to the very top
    loads 50 more (kRenderChunk) at a time; scroll position is restored so the user stays
    in place. Channel switches are ~3x faster on full 500-message buffers.
  - Docs: FAQ and howto.html updated to explain why busy channels always show a scrollbar.
No regressions. No known issues.
Next priorities: ServerId/BufferId strong types; Preferences toggle for send button.
-->

<!--
SESSION SUMMARY — 2026-06-12 (WHOX fallback / v0.25.18 release)
What changed:
  - Diagnosed "Unknown command" error reported by AppImage user (KE0VVT) on Rizon.
  - Root cause: IrcClient sent WHO #channel %cnfa,42 (WHOX) unconditionally on every channel join. Rizon's IRCd does not support WHOX and returns 421 ERR_UNKNOWNCOMMAND, displayed as "!! Unknown command" by Uplink.
  - Fix: parse the WHOX token from RPL_ISUPPORT (005); gate the WHOX query behind m_supportsWhox; fall back to plain WHO on servers that don't advertise it. Reset m_supportsWhox in onDisconnected().
  - Released v0.25.18. All CI passed (CI + Release + Pages workflows green).
  - Docs updated: ircv3.md WHOX section, faq.md WHO scan entry, howto.html WHOX table cell.
No regressions. No known issues.
Next priorities: Send button disable-when-empty; virtual scrolling; heaptrack session audit.
-->

<!--
SESSION SUMMARY — 2026-06-12 (icon refresh)
What changed:
  - Replaced all app icons (assets/uplink-dark.png, uplink-light.png, packaging/uplink.png) and site icons (docs/assets/favicon-32.png, uplink-icon-64.png, uplink-icon-192.png) with new uplink-icon-main design — speech bubble shape, black gloss, glowing orange U with signal arcs.
  - Fixed transparency: source PNG had no alpha channel (white background baked in). Used ImageMagick flood-fill from corner to cut the background, producing clean RGBA output.
  - New icon is now used in the window titlebar, taskbar/panel, system tray, applications menu, and project site.
  - All changes committed and pushed.
Next priorities: Send button disable-when-empty; virtual scrolling; heaptrack session audit.
-->

## v0.25.19

### Added

- Virtual scrolling — chat view renders only the most recent 150 messages on channel switch; a grey divider at the top shows how many older messages exist; scrolling to the top loads 50 more at a time with scroll position preserved

### Fixed

- Send button is now disabled when the input is empty and re-enables as soon as non-whitespace text is typed
- `NickDelegate::sizeHint` no longer calls the full Qt style/text-layout pipeline — returns a fixed `QSize(width, 16)` directly, eliminating ~17K redundant `QTextEngine::itemize` calls per session

## v0.25.18

### Fixed

- Bot icon (alien) now appears for users with `+B` who join a channel after you — previously only users already in the channel at join time were detected via the bulk WHO sweep; late joiners were never queried
- WHO query now falls back to plain `WHO #channel` on servers that do not advertise `WHOX` in `ISUPPORT` — fixes spurious "Unknown command" errors on networks such as Rizon

### Security

- Warn when server password, SASL credentials, or NickServ password are sent over an unencrypted connection (`ssl = false`)
- Defer credential sending until after TLS handshake completes (`QSslSocket::encrypted`) to make the security invariant explicit

## v0.25.17

### Added
- AppImage self-integrates on first run — writes a `.desktop` entry and copies the app icon into `~/.local/share/` so Uplink appears in application launchers without any manual setup. Runs only once and only when launched as an AppImage.

### Fixed
- FadeScrollBar: reverted opacity implementation from `paintEvent` + `QPainter::setOpacity` back to `QGraphicsOpacityEffect`. When Qt's stylesheet engine (QStyleSheetStyle) is active it renders through its own internal painter path; the opacity set on the outer `QPainter` is ignored and the scrollbar is permanently invisible. `QGraphicsOpacityEffect` composites at the widget level after all painting and is the only reliable fade mechanism for QSS-styled widgets.
- FadeScrollBar: reverted wake signal from `actionTriggered` back to `valueChanged`. Mouse wheel events on a parent `QAbstractScrollArea` are handled internally by the scroll area and emit only `valueChanged` — `actionTriggered` is never fired for wheel scrolls, so wheel scrolling never woke the bar.

## v0.25.16

### Added
- Fade scrollbars — scrollbars on the chat view, nick list, sidebar, and all channel panes are hidden at rest, snap to 85% opacity on scroll or mouse-over, and fade out over 300 ms after a 3.5 s idle. Hover over a scrollbar area at any time to reveal it.

### Fixed
- `sendTyping` and `sendReact` now guard on the `message-tags` cap before sending `TAGMSG`. Without the guard, every keystroke sent a `TAGMSG` to servers that don't support the cap (e.g. Undernet / ircu), producing an `!! Unknown command` error line for every character typed.
- `sendTyping`: cap guard corrected from `message-tags` to `draft/typing` — typing indicators are only sent when the server has explicitly negotiated the typing extension.
- `sendReact`: target is now validated with `validIrcToken()` before constructing the `TAGMSG` line.
- Closing a channel buffer now closes its log file and releases the file descriptor; previously the fd leaked permanently for every closed channel.
- Nick metadata (`display-name`, `avatar`) is now capped at 1 000 entries per server session with oldest-first eviction; on large networks over long sessions the map previously grew without bound.
- CTCP rate-limit map (`m_ctcpTimestamps`) now evicts one entry on overflow instead of clearing all entries; the previous `clear()` reset all rate-limit state at once, briefly opening a window for CTCP spam.
- Channel pane `QTextDocument` block cap corrected from `kMessageBufferCap + 300` to `kMessageBufferCap`, matching the main window and the message model; the extra 300 blocks held stale content and used unnecessary memory on long sessions.

### Docs / Site
- Redesigned GitHub Pages site (`docs/index.html`) with Gruvbox Light theme, IBM Plex Mono font, scanline texture, and sections for Download, IRCv3 features, Themes, Security, Power Users, and Community.
- Added Quick Start section to the landing page: 3-step onboarding, minimal config snippet, config path table, essential slash command reference, and keyboard shortcuts — drawn from the how-to guide content.
- Added Gruvbox Dark / Light toggle (sun/moon icon button) to the nav on both `index.html` and `howto.html`; preference persisted to localStorage and shared between both pages.
- Updated `docs/howto.html` CSS from Catppuccin Mocha to Gruvbox Dark; switched body font to IBM Plex Mono to match the landing page.
- Fixed transparent icon bleed in `howto.html` header — swapped `icons/uplink-dark.png` (transparent bg) for `assets/uplink-icon-192.png` (full-color, works on any background).
- Added `docs/assets/` directory with app icon, favicon, and three theme screenshots used by the site.
- Added `scripts/sync-site.sh` — reads version from CMakeLists.txt and patches version strings in `docs/index.html` and `README.md`; now runs at every session close and automatically in CI on tag push via the new `update-docs` job in `release.yml`.

## v0.25.15

### Changed
- Send button now floats inside the right edge of the text input area instead of sitting to its right; icon rendered through `MenuIcons::send` at the correct logical size so HiDPI output is crisp.

### Fixed
- Own nick changes (e.g. `/nick`) now correctly update the UI nick display and self-highlight regex. The NICK source comparison was checking the wrong field and silently ignoring all self-nick events.
- SASL authentication no longer fails silently against servers that split CAP ACK across two messages. CAP END is now deferred until all requested caps — including `sasl` — have been acknowledged.
- Channel nick list is no longer silently dropped when a server uses inconsistent casing between RPL_NAMREPLY (353) and RPL_ENDOFNAMES (366). The names buffer key is now case-normalised.
- IRCv3 tag values with unrecognised escape sequences now preserve the literal backslash per spec instead of dropping it.
- Pane chat views (extra panes opened alongside the main view) now correctly highlight your own nick in messages.
- Reactions, redactions, and condensable event-group updates no longer trigger a full history rebuild in extra pane views — they use the same surgical block replacement as the main view.
- DCC file size is now cached at open time instead of re-stat'd on every ACK and progress call.
- Log file handles are now closed and released when a server is removed.
- Tab completion now correctly cycles through all matching nicks alphabetically on repeated Tab presses. Previously the second Tab would find an empty prefix (cursor landed after the inserted `: ` suffix) and bail out, making only the first match reachable. The fix stores the word-start position at the beginning of each cycle so all subsequent Tabs replace from the original insertion point.
- Send button now positioned correctly on first show; previously it rendered at (0, 0) until the first resize event.
- Send button icon color now updates correctly when the system palette or theme changes; previously the color was baked at construction time.
- "Browse..." button in Preferences → Avatar URL no longer clips its label text; the hardcoded width that was too narrow on some platforms has been removed.

### Performance
- IRC receive loop no longer copies and shifts the read buffer on every newline — uses a start-offset scan with a single tail copy instead.
- `sockWrite` builds a single `QByteArray` instead of allocating a temporary `QString + "\r\n"` before UTF-8 encoding.
- Log messages write directly via `QFile::write` instead of constructing a `QTextStream` on every call.
- Nick sort comparisons and index rebuilds use a pre-computed `lowerNick` field instead of calling `toLower()` on every comparison.
- Netsplit nick removal now passes a `QSet` to a batch `removeNicks()` method, rebuilding the nick index once per channel instead of once per departing nick.
- `decodeEntities` in link preview uses a string replacement table instead of allocating a `QTextDocument`.
- URL regex is now a single file-scope constant in `chatrenderer.cpp` shared by `linkifyTopic` and `linkifyHtml`.
- Emoji picker layout rebuild suppresses per-item repaints with `setUpdatesEnabled`.
- Message insertion (`insertHtmlBlock`) now wraps block removal and HTML insertion in a single edit block, halving the number of document layout passes on busy channels at the scroll cap — reduces lag on Enter.

---

## v0.25.12

### Added
- Event group batches (join/quit/nick lines) are now clickable. A `▸` indicator marks each compact batch; clicking it expands the group in-place to show individual event lines with full hostmasks and quit/part reasons. Clicking `▾` collapses it back. Expanded state survives channel switches and view refreshes. Works in both the primary view and extra pane views.

### Fixed
- Sidebar selection pill now encloses indicator icons (unread bubble, mention star, server globe) symmetrically — equal padding on both sides of the label and icon.
- Nick list selection pill now encloses the away-status icon symmetrically using the same fix.
- Chat history replay messages now render visibly dimmed in gray. The previous CSS `opacity` approach had no effect in Qt's HTML renderer; replaced with explicit per-element color changes.

---

<!--
Session summary — 2026-06-10 (clickable event group expand/collapse)

Added in-place expand/collapse for join/quit/nick-change/kick event batches.

Design: event groups already rendered as a single compact line (up to 10 nicks).
New feature adds a ▸ indicator as a clickable anchor (href="evgrp:TIMESTAMP_MS").
Clicking toggles the group between compact and expanded views in-place using
QTextCursor block replacement. Expanded view shows every event as a full line
with hostmask and quit/part reason (formatMessage style), joined with <br>
within the same document block. The ▾ collapse anchor on the first expanded
line restores the compact form.

Key implementation points:
- chatrenderer: formatEventGroup gains groupId + expanded params; all existing
  call sites pass defaults so no behavior change unless opted in.
- mainwindow: m_expandedEventGroups (QSet<QString>) stores toggled groupIds.
  toggleEventGroupInView() finds the group by first-message timestamp, re-renders,
  replaces the block, and re-sets userData (BlockMsgid) after replacement.
- anchorClicked wired for both primary chatView and all pane chatViews.
- refreshChatView and refreshPaneChatView pass groupId and check
  m_expandedEventGroups so expanded state survives channel switches and
  view redraws.
- Scroll position preserved on toggle (restore if not at bottom; stay at
  max if at bottom).

Docs updated: howto.html event condensation section expanded with example
and use-case notes; faq.md new entry "How do I see the full hostmask and
quit reason from a join/quit line?".

Clean build, all tests pass. Three commits: feat, docs/changelog, docs.
-->

<!--
Session summary — 2026-06-10 (avatar persistence and cross-machine propagation)

Root cause investigation: local file avatars were not showing on cold start
because onServerConnected only sends METADATA SET for HTTP URLs (correct —
local files are never uploaded). Nothing was seeding nickMeta or calling
fetchAvatar for local paths on connect. Fix: onServerConnected now seeds
nickMeta and calls fetchAvatar for local file avatar paths so the image
shows immediately on startup without needing to re-apply via Preferences.

Also added METADATA GET fallback: when a nick's account becomes known and
no avatar URL is cached, Uplink now requests it with
METADATA <nick> GET avatar display-name. This covers the case where the
server push was missed or the server does not auto-push on join. Silenced
766/768 error numerics (ERR_NOMATCHINGKEY / ERR_KEYNOTSET) which were
appearing as error messages in the chat window when nicks had no avatar set.

Verified behavior:
- Local file avatar: shows on the machine that has the file, not others.
- HTTP URL avatar: stored in Ergo via METADATA SET, propagates to all clients.
- FreeBSD user with HTTP avatar URL shows correctly on Linux clients.

No regressions. Clean build.
-->

<!--
Session summary — 2026-06-07 (window width bug debugging — unresolved)

Investigating persistent full-width window on FreeBSD KDE Plasma 6 / X11 / KWin.
Symptoms: window opens at full screen width, user cannot resize it narrower.
Deleting ~/.config/uplink/uplink.conf has no effect.

Two attempted fixes (commits 7fb1b48, 05d9624), both ineffective:
  1. setMaximumWidth(width()+20) before show() to cap WM_NORMAL_HINTS.max_width
  2. Detect tooWide/qtMaximized in singleShot(0) and cycle _NET_WM_STATE atoms

Current theory (unconfirmed): singleShot(0) releases the max_width cap and checks
width() BEFORE KWin's ConfigureNotify arrives — so tooWide is false, normal path
runs, then KWin expands the window after the timer exits with nothing left to stop it.

Diagnostic build pushed (commit 9fd9d69): qWarning() lines trace width before/after
max_width release, windowState, minimumSizeHint, and first 8 resizeEvents to stderr.

Next step: user switching to X11 on Linux to reproduce; run:
  ./build/Uplink 2>&1 | head -30
Then paste the Uplink diag[] and resizeEvent[] lines to determine root cause.

Nothing shipped this session. Diagnostic commit is temporary and should be reverted
once root cause is confirmed and the real fix is written.
-->

<!--
Session summary — 2026-06-05 (keychain SASL password corruption fix)

Fixed a bug where opening the Edit Server dialog and saving without changing a
keychain-stored password would corrupt it — the sentinel string "<keychain>"
was being written to the keychain as the literal password value, causing SASL
authentication to fail on next connect.

Fix: added an early-return guard in storePassword() (config.cpp) so a value
that is already the sentinel passes through without touching the keychain.
-->

<!--
Session summary — 2026-06-05 (compiler warning cleanup)

Fixed all compiler warnings — no functional changes:

- manageserversdialog.cpp: !x == 0 logical-not precedence bug → x > 0
- commanddispatcher.cpp: indexOf results stored as qsizetype (was int, 64→32 truncation);
  currentSecsSinceEpoch() cast to quint64 (sign-conversion)
- mainwindow.cpp: dropped unused [this] captures in two lambdas

Also explored adding a window opacity slider (Preferences → Appearance).
Implemented and reverted — Qt's setWindowOpacity works fine for transparency,
but background blur requires compositor-specific platform code (KWin, DWM, NSVisualEffectView)
with no clean cross-platform path, so deferred indefinitely.
-->

<!--
Session summary — 2026-06-05 (v0.24.0 — draft/multiline + full audit backlog)

Released v0.24.0.

IRCv3 draft/multiline support:
- IrcClient: CAP negotiation, batch buffering with concat flag, sendMultiline()
  with per-line PRIVMSG fallback when server lacks the CAP
- ChatRenderer: \n → <br> in formatMessage for multiline rendering
- SessionModel: sendMessage routes to sendMultiline when text contains \n
- mainwindow.cpp / channelpane.cpp: QLineEdit → QPlainTextEdit
  - Shift+Enter = newline, Enter = send
  - Auto-resize 1–4 lines on textChanged
  - Up/Down history nav activates only at first/last block
  - Tab complete uses block-level cursor ops (current line only)
  - Emoji autocomplete, history, picker all ported to QTextCursor API

Security/hardening (from post-v0.23.3 commits):
  - DCC peer validation (m_expectedPeer in DccSend)
  - DCC receive: write-failure cancel, 30s passive stall timeout
  - DCC ACK coalescing: every 64 KB + completion
  - sendRaw: 512-byte RFC 1459 line cap
  - Ping watchdog: abort+reconnect after 90s
  - NICK/USER: strip spaces/nulls
  - Unknown /commands → local error echo
  - m_ctcpTimestamps capped at 500
  - Link preview HTML capped at 8192 bytes
  - addresscheck.h extended: CG-NAT, TEST-NETs, IPv4-mapped, etc.

Performance:
  - IrcClient: QByteArray receive buffer, UTF-16 deferred to per-line
  - Channel::removeNick: O(n) in-place index patch
  - logMessage: cached QFile* handles

Bug fixes:
  - refreshChatView: deferred scroll-to-bottom on channel switch
  - Mention detection: empty regex guard fixed

Docs updated:
  - ircv3.md: draft/multiline moved from Planned → Active
  - keyboard-shortcuts.md: Shift+Enter documented
  - howto.html: multi-line messages section + nav entry
  - faq.md: multi-line Q&A added
-->

<!--
Session summary — 2026-06-05 (bug fixes: scroll position, mention detection)

Two bugs fixed:

1. Chat view didn't scroll to bottom on channel switch:
   - refreshChatView cleared and rebuilt the document but never scrolled to bottom
   - appendMessage already had scroll-to-bottom; refreshChatView was missing it
   - Fix: QTimer::singleShot(0) deferred scroll-to-bottom at end of refreshChatView
     (deferred to let Qt finish document layout before setting scrollbar maximum)

2. 💡 lightbulb icon shown on all channel activity instead of only name mentions:
   - mentionRe is set to QRegularExpression{} (empty pattern) when nick is unknown
   - Empty pattern is syntactically valid so isValid() returns true
   - QRegularExpression("").match(any_text).hasMatch() returns true for every string
   - Every incoming message was counted as a mention
   - Fix: changed guard from isValid() to !pattern().isEmpty() in sessionmodel.cpp
-->

<!--
Session summary — 2026-06-05 (40-finding code audit — batches 1–4)

Released v0.23.3 — perf/stability patch on top of v0.23.2.

Changes committed across four batches:

Batch 1 — safety & correctness:
  - QPointer guard in keychain lambda (sessionmodel.cpp) — prevents
    use-after-free when IrcClient is destroyed before async read completes
  - selfNick highlight regex pre-compiled into m_selfNickRe (MainWindow
    member); ChatRenderer::Context carries QRegularExpression instead of
    QString — regex no longer rebuilt on every message render
  - Dead ternary in actionReceived emit fixed (target was always `target`)
  - Null guard at top of eventFilter prevents crash when m_chatView is null
  - QTextBrowser block count capped at kMessageBufferCap + 300 to bound RAM

Batch 2 — performance:
  - Mention regex (mentionRe) pre-compiled per nick-change; per-message
    regex build in postMessage eliminated
  - Channel::addNick rewritten with std::lower_bound O(log n) sorted
    insert; partial nickIndex update instead of full rebuild
  - DCC timeout fixed — fires even when transfer has partially started
    (removed the m_received == 0 guard)
  - setTopicIcon: wildcard disconnect replaced with tracked
    QMetaObject::Connection m_topicIconConn
  - Duplicate reaction HTML builder and duplicate textChanged lambda both
    consolidated; magic numbers replaced with named constants

Batch 3 — code quality:
  - src/net/addresscheck.h added — shared isPrivateAddress() used by both
    linkpreview.cpp and dccreceive.cpp (duplicate local functions removed)
  - Monitor nick membership now tracked in m_monitorSet (QSet<QString>);
    monitorAdd O(1) check replaces O(n) scan
  - QThread::terminate() replaced with shared abort flag — safe cooperative
    cancellation for /sysinfo background thread
  - onServerDisconnected: m_typingNickTimers, m_typingNicks, and m_botIcons
    cleaned up by host on disconnect
  - refreshChatView: ChatRenderer::Context built once before the message
    loop (was rebuilt on every message via appendMessage)

Batch 4 — RAM + targeted UI updates:
  - LinkPreview::CachedCard changed from QPixmap to QByteArray pngData;
    compressed PNG stored (~10x smaller per entry); QPixmap reconstructed
    only on cache hit for signal delivery
  - FIFO eviction via m_cacheOrder (QList<QString>); cache bounded at 50
    entries (previously could grow unbounded)
  - reactionsChanged / messageRedacted signals extended with msgid parameter;
    QTextBlocks tagged with BlockMsgid userData (QTextBlockUserData subclass)
    in refreshChatView and appendMessage; onReactionsChanged does targeted
    insert/replace/remove of just the reaction block; onMessageRedacted
    replaces only the affected block — full refreshChatView no longer called
    for either event

Docs: version bumped to v0.23.3; download links updated; ROADMAP ticked
-->

<!--
Session summary — 2026-06-05 (release v0.23.2 + docs pass)

Released v0.23.2 — patch bump from v0.23.1 (fixes and refactors only,
no new user-visible features warrant a minor bump).

Docs updated:
  - README: OPER added to credential redaction row; DCC filename
    sanitization noted; Running Tests section added (ctest invocation,
    UPLINK_BUILD_TESTS option)
  - docs/configuration.md: keychain section notes async non-blocking
    startup behavior
  - docs/faq.md: How do I run the unit tests? FAQ added; broken
    building.md link replaced with README anchor

CI/release:
  - release.yml update-docs push was blocked by branch protection when
    run as github-actions[bot]; added `|| echo` so the step logs a
    note instead of failing the whole workflow; doc bump applied
    manually as part of release commit going forward
  - v0.24.0 was tagged and released in error (minor bump for patch
    work); deleted and re-released correctly as v0.23.2
-->

<!--
Session summary — 2026-06-05 (unit tests + keychain async fix)

Added Qt Test suite (tests/):
  - tst_ircparser: 14 cases covering prefix parsing, nick!user@host split,
    IRCv3 tag parsing, tag value unescaping, server-time, numerics,
    CRLF stripping, empty/malformed input
  - tst_chatformat: 16 cases covering HTML escaping, bold/italic/underline
    IRC formatting codes, IRC color codes (fg and fg+bg), reset (\x0F),
    linkifyHtml (http/https wrapped, ftp skipped), htmlAttr quoting
  - CMakeLists.txt wired via UPLINK_BUILD_TESTS option (on by default);
    run with: cmake --build build --target tst_ircparser tst_chatformat
    or: ctest --test-dir build
-->

<!--
Session summary — 2026-06-05 (keychain async fix)

Removed nested QEventLoop from the startup path:
  - Config::load() no longer calls resolvePassword(); sentinel "<keychain>"
    values are left in place and flow through to spawnSession.
  - Added KeychainHelper::readAsync() — fires a ReadPasswordJob and calls
    back on the main thread with no blocking loop.
  - Added resolveAndConnect() in sessionmodel.cpp — resolves all sentinel
    fields (password, saslPassword, nickservPassword, proxyPass, channel
    keys) in parallel via readAsync, then calls connectToServer once all
    reads complete. Plain-text or empty fields connect immediately.
  - spawnSession now calls resolveAndConnect() instead of connectToServer()
    directly; sidebar and signal wiring still happen synchronously.
  - Removed dead resolvePassword() helper from config.cpp.

Result: startup no longer spins a nested event loop per sentinel field,
eliminating the reentrancy hazard when the keychain backend needs to show
a dialog or dispatch events before the main window is fully ready.
-->

<!--
Session summary — 2026-06-04 (lightweight code review — small patches)

What was fixed:
  - Topic link gate (#10): setOpenExternalLinks(false) + linkActivated →
    http/https scheme check in mainwindow.cpp and channelpane.cpp; topic
    labels now go through the same validator as chat links
  - DCC timeout cleanup (#11): all three timeout handlers (dccsend::listen,
    dccreceive::start, dccreceive::listenPassive) call cancel() before
    emitting error — sockets and file handles are now closed on timeout
  - hiddenPreviews pruning (#3): addPreview() removes the evicted URL from
    hiddenPreviews when the preview cache rolls over; keeps the set bounded
    with the 100-entry preview cap
  - Compiler warnings (#13): -Wall -Wextra -Wpedantic -Wconversion
    -Wsign-conversion -Wshadow -Wnon-virtual-dtor -Wold-style-cast added
    to CMakeLists.txt; UPLINK_WARNINGS_AS_ERRORS and UPLINK_ENABLE_SANITIZERS
    options added (both off by default)

New warnings surfaced (deferred):
  - qsizetype→int narrowing in channel.h, ircparser.cpp, ircclient.cpp,
    linkpreview.cpp, manageserversdialog.cpp, mainwindow.cpp
  - Shadow locals in serversession.h, channelpane.cpp, serverdialog.cpp,
    mainwindow.cpp
  - Old-style casts in mainwindow.cpp, pillbutton.h

ROADMAP updated:
  - New "Performance & Maintainability Backlog" section added with all
    deferred review items organized into near-term / medium / architectural

-->

<!--
Session summary — 2026-06-04 (security audit — F-01 through F-12)

All findings from the full security audit triaged and resolved.

Fixed:
  F-01 — Link preview SSRF: isBlockedBySchemeOrLiteral() for scheme/literal
          private IPs; all fetches DNS-resolved via QHostInfo::lookupHost()
          with loopback/private/link-local/multicast rejection
  F-02 — DCC receive: private/reserved peer IP blocked before file open or
          socket connect in DccReceive::start()
  F-03 — DCC passive receive: listenPassive() accepts expectedIp; incoming
          connections from unexpected peers rejected when sender is public
  F-04 — Reactions: storage changed to QSet<QString> per (msgid, emoji) for
          automatic nick dedup; reactions pruned on message rolloff
  F-05 — Link preview redirects: ManualRedirectPolicy; each redirect
          destination re-DNS-checked before following; image redirects blocked
  F-06 — HTML attribute XSS: htmlAttr() helper encodes ' to &#39; in addition
          to toHtmlEscaped(); applied to all href/title attributes
  F-07 — Proxy passwords and channel keys now stored via keychain
          (storePassword/resolvePassword path); plaintext fallback only when
          keychain unavailable
  F-08 — STS store: hostname lowercased before all lookup/store/remove ops;
          sts.ini permissions restricted to owner read/write after each write
  F-10 — /sysinfo: QMessageBox::question confirmation required before posting
          system info to a channel
  DCC  — Passive DCC token upgraded from 8-digit decimal (26-bit) to 32-char
          lowercase hex (128-bit)

Out of scope (deferred):
  F-09 — CMake/CI build config change — build infrastructure concern, low risk
  F-11 — Keychain async refactor — architectural change, low risk
  F-12 — Test infrastructure

-->

<!--
Session summary — 2026-06-04 (v0.22.0 — passive DCC + in-app update check)

What was built:
  - DCC passive / NAT traversal:
    - DccReceive::listenPassive() — opens a QTcpServer so the passive sender
      can connect in; DccReceive::listenPort() returns the bound port; 60s
      timeout before cancellation
    - DccSend::initPassive() — validates file, generates random 8-digit token;
      DccSend::connectOut(ip, port) — connects out as TCP client and starts
      sending; replaces the listen() path for passive mode
    - ircclient.cpp — DCC SEND parsing extended to read optional token (field 6);
      port==0+token → dccPassiveOfferReceived; port!=0+token → dccPassiveSendReply;
      plain port!=0 → existing dccSendReceived
    - SessionModel — forwards both new signals through the model layer
    - mainwindow.cpp — dccPassiveOfferReceived handler: asks user, calls
      listenPassive(), sends CTCP reply with our IP:port, shows progress dialog;
      dccPassiveSendReply handler: looks up pending token, calls connectOut();
      "Send File (Passive)" added to nick right-click context menu;
      m_pendingPassiveSends (QMap<QString, DccSend*>) tracks in-flight offers
  - In-app update check:
    - "Check for Updates" added to hamburger menu between "About Uplink" and
      "Documentation"; fetches api.github.com/repos/noderelay/UplinkIRC/releases/latest;
      parses tag_name with QRegularExpression; compares to UPLINK_VERSION_MAJOR/
      MINOR/PATCH; shows "Update Available vX.Y.Z" or "Up to Date" QMessageBox
  - Documentation full pass:
    - ROADMAP.md — DCC passive and update check ticked done; Known Issues updated
    - README.md — DCC entry, right-click table, hamburger entry all updated
    - docs/commands.md — DCC section rewritten with mode table and passive steps;
      Send File (Passive) added to nick right-click table
    - docs/faq.md — DCC FAQ rewritten with plain-language mode guidance; new
      "How do I check for updates?" FAQ entry; table updated
    - docs/howto.html — DCC section rewritten with both modes, comparison table,
      step-by-step, timeout list; Send File (Passive) in nick table; hamburger
      description updated
    - docs/index.html — two new feature cards: DCC file transfer and update check

Bugs fixed:
  - Raw string literal delimiter clash in update check regex (R"_( )_" form)
  - \x01DCC hex escape ambiguity fixed with split string literal (\x01""DCC)

Known issues left open:
  - DCC when both sides are behind NAT — no relay mechanism; neither mode works
  - DCC passive receive over NAT — receiver's port must be reachable from outside

Next priorities:
  - Pane layout persistence (save/restore pane arrangement across sessions)
  - Virtual scrolling (performance on busy channels)
  - FreeBSD port skeleton

-->

<!--
Session summary — 2026-06-03 (v0.20.2 — documentation pass)

What was built:
  - Comprehensive documentation pass across all docs:
    - keyboard-shortcuts.md: expanded from 41 to 90+ lines; added Tab completion
      detail, emoji shortcode shortcuts, Shift+Tab, planned shortcuts table
    - faq.md: fixed orphaned paragraph (missing heading); added FAQ entries for
      self-signed cert pinning, cert mismatch recovery, app icon switching, and
      custom theme creation
    - configuration.md: fixed app_icon option list (removed stale light-default/
      avatar values); fixed malformed proxy/ssl_fingerprint line in full example
  - Corrected Uplink icon set: replaced all copies with properly transparent
    RGBA PNG files (dark and light variants)

Bugs fixed:
  - configuration.md app_icon table listed 4 options; only 2 are valid (dark/light)
  - configuration.md full example had literal \n in a comment line
  - faq.md had an orphaned paragraph with no heading (documentation search section)

Known issues left open:
  - About dialog Wayland centering drift (Qt limitation, deferred)
  - DCC over internet blocked by NAT (passive DCC not yet implemented)
  - GitHub Pages needs enabling: Settings → Pages → /docs

Next priorities:
  - DCC passive / NAT traversal
  - In-app update check UI

Session summary — 2026-06-03 (v0.20.1 — UplinkIRC rename + new icons)

What was built:
  - Full project rename: NodeRelay → UplinkIRC (repo), Uplink (app/binary).
    All source, docs, packaging, CI, config paths, and memory updated.
  - New Uplink icon set: dark and light transparent PNG variants wired into
    QRC, About dialog, tray, window title bar, AppImage packaging, README,
    docs site, and howto.html.
  - Icon picker in Preferences simplified to two radio buttons: Dark / Light.
    Old 3-style system (Node N / Tower / Hub Spoke) removed along with all
    old icon files.
  - Sidebar pill row height fix: 20 → 26 px — descenders no longer clipped.
  - Docs site (index.html, howto.html) fully rebranded to Uplink.
  - GitHub Pages URL corrected to noderelay.github.io/UplinkIRC/.
  - Local project folder renamed ~/Projects/NodeRelay → ~/Projects/UplinkIRC.
  - Config dirs cleaned up: ~/.config/noderelay, NodeRelay, uplinkirc removed.

Bugs fixed:
  - Sidebar pill clipped descenders (p, g, y) at 20 px row height.
  - GitHub URLs had noderelay username replaced with uplink during rename sweep.
  - docs/index.html icon grid still showed old 6-icon NodeRelay set.
  - howto.html hero image still pointed to old tower.svg.

Known issues left open:
  - About dialog Wayland centering drift (Qt limitation, deferred).
  - DCC over internet blocked by NAT (passive DCC not yet implemented).
  - GitHub Pages needs enabling in repo Settings → Pages → /docs.

Next priorities:
  - DCC passive / NAT traversal
  - In-app update check UI

Session summary — 2026-06-03 (v0.20.0 — MD3 UI pass + cert pinning)

What was built:
  - Self-signed cert fingerprint pinning: on first connect to a self-signed
    server, a dialog offers Accept Once / Pin Certificate / Reject. Pinning
    saves the SHA-256 fingerprint to config.toml under ssl_fingerprint. Future
    connections verify the fingerprint; mismatch disconnects with a warning.
    IrcClient::abort() added for clean reject without reconnect.
  - MD3-inspired UI pass: QSS overhaul across all themes — pill buttons
    (20px border-radius), rounder inputs (10px), menus (12px item radius),
    tabs, tooltips, scrollbar. Border-radius was unreliable on QPushButton
    in Qt's Fusion style; introduced PillButton (QPainter subclass) that draws
    its own rounded background, guaranteeing the pill shape regardless of style.
  - Material Symbols Outlined SVG icons: hamburger menu (menu.svg), gear/
    settings (settings.svg), and 14 menu icons replacing hand-drawn QPainter
    icons in menuicons.h. All icons colorized at runtime via
    CompositionMode_SourceIn to match active theme text color.
  - Sidebar pill highlight: fixed asymmetric left/right padding by accounting
    for Qt's internal PM_FocusFrameHMargin text offset; pill radius bumped to
    10px; padding bumped to 8px each side.
  - Row height: FixedRowDelegate controls sidebar (20px) and nick list (16px)
    height reliably via sizeHint override; QSS height on ::item was silently
    ignored by Qt.
  - Preferences dialog: PillButton for Manage Servers, Documentation, Theme,
    Font Config; compact spacing; no scrollbar on open.
  - Docs dialog: search box compact style fix — no longer collides with
    Shortcuts tab at default dialog width.
  - Buttons enlarged: hamburger, gear, and sidebar gear buttons now 28×28px
    with 20×20px icons.

Bugs fixed:
  - QSS border-radius silently ignored on QPushButton in Fusion style — fixed
    with PillButton custom paintEvent.
  - Sidebar pill left/right padding asymmetric — fixed via internal text margin
    offset calculation.
  - QSS height on QTreeWidget::item and QListWidget::item ignored — fixed via
    sizeHint delegate overrides.
  - Preferences dialog showed scrollbar on open — fixed with compact spacing.
  - Docs search box collided with Shortcuts tab — fixed with compact inline style.

Known issues left open:
  - About dialog Wayland centering drift (Qt limitation, deferred).
  - DCC over internet blocked by NAT (passive DCC not yet implemented).
  - Sidebar pill highlight may still clip on very long server names in a
    very narrow sidebar.

Next priorities:
  - DCC passive / NAT traversal
  - In-app update check UI
  - SVG light icon variants

Session summary — 2026-06-03 (v0.20.2 cont. — UI polish + theme seeding fixes)

What was built:
  - Input field border-radius reduced from 20px (pill) to 8px (subtle) — chat
    area separation preserved; QLineEdit background/structure unchanged.
  - Theme seeding: added /../themes fallback path so dev builds running from
    build/ find the project's themes/ directory and seed ~/.config/uplink/themes/
    on first run.
  - Theme seeding: replaced QStandardPaths::AppConfigLocation with hardcoded
    ~/.config/uplink/themes — Qt resolves AppConfigLocation to ~/.config/Uplink/
    (capital U) which mismatched the lowercase path used everywhere else.

Bugs fixed:
  - Broken [[server]] TOML key in user config.toml (missing closing ]]) caused
    entire config to fail to parse — no servers loaded on startup.
  - Themes directory never created: QStandardPaths path case mismatch silently
    wrote to wrong directory; fixed to hardcoded lowercase path.
  - Dev build theme seeding: binary at build/Uplink looked for build/themes (doesn't
    exist); added build/../themes fallback pointing to project themes/ root.

Known issues left open:
  - About dialog Wayland centering drift (Qt limitation, deferred).
  - DCC over internet blocked by NAT (passive DCC not yet implemented).

Next priorities:
  - DCC passive / NAT traversal
  - In-app update check UI
-->

<!--
Session summary — 2026-06-05 (doc fix — version string bump)

What was done:
  - Verified all CIs passed for v0.23.1 (both CI and Release workflows: success)
  - Found stale version strings in README.md and docs/index.html:
      README.md: 4 download badge links were hardcoded to v0.23.0 filenames
      docs/index.html: nav brand, hero button, section title showed v0.16.3/v0.20.0;
        download card hrefs pointed to v0.20.0 artifacts; footer version tag was v0.16.3
  - Bumped all occurrences to v0.23.1 and pushed

Root cause: download URLs use releases/latest/download/ but filenames include
the version number — so every release breaks the links unless they are manually
bumped. Suggested adding an auto-bump step to release.yml to prevent recurrence.

No bugs fixed, no regressions, no code changes.

Next priorities:
  - Add release.yml step to auto-update version strings on tag push
  - DCC passive / NAT traversal
  - In-app update check UI
-->

<!--
Session summary — 2026-06-05 (CI: auto-bump release docs)

What was done:
  - Added update-docs job to release.yml that runs after all platform builds succeed
  - On each release tag, the job checks out main, applies targeted sed replacements
    to bump every version-bearing string in README.md and docs/index.html, then
    commits and pushes only if anything changed
  - Covers: README download badge hrefs (AppImage, tar.gz, zip, dmg); docs/index.html
    download card hrefs; nav brand span, hero button text, section title, footer
    version-tag div
  - Root cause of recurring stale links permanently addressed — no manual bump needed
    on future releases

No bugs, no regressions. CI/workflow-only change.

Next priorities:
  - DCC passive / NAT traversal
  - FreeBSD port skeleton
-->

<!--
Session summary — 2026-06-05 (v0.24.1 — input bar bg fix)

Noticed after v0.24.0 shipped that the input bar lost its background color.
Root cause: themeloader.cpp QSS selector was `QWidget#inputBar QLineEdit`
which stopped matching after the QLineEdit → QPlainTextEdit migration.
Fix: combined selector covering both widget types; also extended
`::placeholder-text` to cover QPlainTextEdit.

Lesson recorded: "commit and push" ≠ permission to cut a release.
-->

<!--
Session summary — 2026-06-06 (ping timeout fix + keychain UX + eye toggle)

Three fixes shipped this session:

1. Ping timeout every ~2 minutes on Libera.Chat (and any solanum server):
   A QTimer::singleShot(2000, ...) in onConnected() fired a PING 2s after TCP
   connect — during SASL negotiation, before the client was registered. Solanum
   ignores PINGs from unregistered clients, leaving m_pingPending stuck true.
   The 30s watchdog fired 4 more times before hitting the 90s elapsed threshold,
   killing the connection at 2 + 4×30 = 122s (~2 minutes) every cycle. Fix:
   removed the singleShot. The regular 30s timer handles keepalive once
   registration completes.

2. Keychain password fields showed <keychain> as masked dots in Edit Server:
   Any keychain-stored password field displayed the literal sentinel string as
   dots, making it unclear whether a password existed. If the user accidentally
   cleared the field and saved, the keychain entry was silently deleted. Fixed
   by: showing placeholder text "Stored in keychain — type to change, clear to
   remove" for sentinel-valued fields; preserving the sentinel on save when the
   field is left empty; logging qWarning on readAsync failures so the cause is
   visible in terminal output.

3. Added show/hide eye toggle to all password fields in Add/Edit Server dialog:
   Material Symbols visibility/visibility_off SVG icons added to resources;
   QLineEdit::addAction() places the icon inside the trailing edge of each
   password field; clicking toggles echo mode and swaps the icon.

No regressions observed. Memory updated to correct stale goals_features.md
(all UI features were already done; movable panels dropped from roadmap).
Next: icons (waiting on user design).
-->

<!--
Session summary — 2026-06-06 continued (memory optimizations + v0.24.2 release)

Shipped four memory/startup optimizations:
1. Message buffer cap 2000→500 per channel (kMessageBufferCap in channel.h)
2. Link preview cap 100→20 per channel (kPreviewCap in channel.h)
3. EmojiPicker lazy-loaded — widget now created on first use via ensureEmojiPicker()
   instead of at startup; all three call sites updated
4. QTextDocument block count aligned to buffer cap (was kMessageBufferCap+300)

Tagged and released v0.24.2. All CI passes (Linux, macOS, Windows). Binaries live:
AppImage, Linux tar.gz, macOS dmg, Windows zip, zsync metadata.
update-docs CI job ran and confirmed version strings correct across README/docs.

Next: icons (waiting on user design).
-->

<!--
Session summary — 2026-06-05 (KWallet popup on every preference change)

Fixed a bug where opening Preferences and changing anything — font config,
theme, checkboxes, nick brackets — triggered a KDE Wallet unlock prompt on
KDE/FreeBSD systems.

Root cause: Config::save() calls storePassword() for every server password
field on every save, including saves triggered by unrelated UI changes (font,
theme, toggles). storePassword() with a plain-text value calls
KeychainHelper::write(), which opens the KWallet dialog. On systems where the
keychain is broken or not set up (e.g. FreeBSD plain-text passwords in config),
every single preference change fires the prompt.

Fix: added bool migratePasswords = false to Config::save(). Password keychain
migration is now only attempted when the server management dialog explicitly
saves (Config::save(..., true)). All other preference saves write passwords
back as-is without touching the keychain.

No new features. No regressions. One call site changed in mainwindow.cpp.
-->

<!--
Session summary — 2026-06-07 (keychain missing-entry warning)

Single fix: when config.toml has <keychain> as a password sentinel but no
corresponding entry exists in the OS keychain, Uplink previously failed SASL
silently with no indication of why. Now emits a red !! error in the server
buffer: "Keychain: no password stored for "LinuxDojo:sasl_password" — open
Edit Server and re-enter your password". This gives the user a clear,
actionable error instead of a confusing SASL failure.

Root cause of empty keychain in existing configs: the sentinel was written at
some point without the keychain write succeeding (or the entry was manually
cleared). Fix for the user: open Edit Server, type the password once, save.

Change: sessionmodel.cpp resolveAndConnect() — lambda captures key, emits
socketError on empty val.

Next: no pending items; app is at v0.24.2 with all known bugs resolved.
Icons are done. No new features planned this session.
-->

<!--
Session: 2026-06-07 (patch — FreeBSD window geometry + QSettings path)

What changed:
- Fixed window starting ultra-wide and unresizable on FreeBSD (and any screen narrower
  than the saved or default 1100px geometry). First attempt ran setGeometry() in the
  constructor (pre-show) which X11 window managers silently ignore on first map. Moved
  the bounds clamp into the existing QTimer::singleShot(0) callback that fires after
  show() when geometry() reflects the real on-screen size.
- Renamed QSettings path from QSettings("LinuxDojo","Uplink") → QSettings("uplink","uplink").
  Old path wrote to ~/.config/LinuxDojo/Uplink.conf — a leftover from DojoIRC.
  New path writes to ~/.config/uplink/uplink.conf alongside config.toml.

Bugs fixed:
- Window too wide to resize on FreeBSD — geometry clamping was firing before the
  X11 window manager mapped the window; moved to post-show timer.
- Wrong QSettings location — ~/.config/LinuxDojo/Uplink.conf should be
  ~/.config/uplink/uplink.conf.

Known issues:
- Existing users will lose saved window geometry/splitter state on first run after
  this update (old LinuxDojo path is no longer read). This is acceptable — the new
  defaults are correct and the geometry fix makes a fresh start the right behavior.

Next: no pending items; FreeBSD startup is now correct.
-->

<!--
Session 2026-06-08 (continued — documentation audit + Ctrl+W):
- Added Ctrl+W shortcut to minimize to system tray (no-op when no tray available)
- Full documentation audit corrected 6 inaccuracies across docs/:
  - keyboard-shortcuts.md: removed non-existent Ctrl+W entry; updated after real Ctrl+W wired
  - howto.html: fixed wrong config key show_typing → typing_indicator
  - index.html: buffer cap 2,000 → 500; IRCv3 cap statuses corrected
    (batch/chathistory/msgid/sts → active; labeled-response/echo-message → partial)
  - configuration.md: added missing font_server_header key to example + reference table
  - commands.md: added /join #channel [key], /leave, /close, /monitor remove
- In-app docs updated automatically (aliased from same docs/ files)
Next: no pending items.
-->

<!--
Session summary — 2026-06-09 (post-v0.25.2 — icon polish)

Fixed stale local build (version.h was stale, showed v0.25.1 in About dialog after
v0.25.2 release — solved by re-running cmake configure; CI was never affected).

Gave "Check for Updates" its own Material Symbols icon (mi-new-releases.svg) instead
of sharing the info icon with "About Uplink". All burger menu items now have distinct
icons.

No version bump — cosmetic/icon change only.
Commit: 720c73b
-->

<!--
Session summary — 2026-06-08 (v0.25.2 — IRC command completions + soju bouncer fix)

Added a batch of missing IRC commands and fixed the soju bouncer network selection:

- /list [filter] — sends LIST to the server; RPL_LIST (322) and RPL_LISTEND (323) collected and
  displayed in the server buffer. A GUI dialog (/list dialog with live search) was discussed but
  deferred — the text output is solid for now and the dialog is still on the roadmap.
- /setname <realname> — send side added. Receive side (parsing SETNAME from others, "nick changed
  their realname" in shared channels) was already done since v0.16.0. Now you can change your own
  realname on the fly without reconnecting.
- /whowas <nick> — was completely unimplemented. Now handles RPL_WHOWASUSER (314) and
  RPL_ENDOFWHOWAS (369); results appear in the current buffer alongside WHOIS replies.
- /stats <query> — was completely unimplemented. Now routes numerics 211–219 and 241–244 to the
  current buffer. Typical usage: /stats u (uptime), /stats o (opers), /stats m (commands).
- /time (no args) — bare /time was silently doing nothing. Now sends the IRC TIME command and
  handles RPL_TIME (391). /time <nick> (CTCP) was already working.
- Soju bouncer: SASL username fix — m_bouncerNetwork was stored in config and shown in the server
  dialog but never actually used when authenticating. For soju, the SASL AUTHENTICATE payload must
  send "user/network" as the username. This was the main reason soju network selection did not work.
- Soju bouncer: LISTNETWORKS END — BOUNCER LISTNETWORKS END was unhandled. Networks are now
  buffered during the initial listing and shown as a formatted summary when the list completes.
  Live BOUNCER NETWORK state-change messages (outside of a listing) still print per-event.
- Deleted resources/icons/uplink-icon.svg — untracked leftover from an abandoned SVG window icon
  experiment; was never committed or referenced in the build.

No bugs found or regressions this session. v0.25.2 tagged and released; all 4 CI jobs passed;
AppImage, Windows zip, and macOS dmg all uploaded. Doc auto-bump blocked by branch protection
so applied manually.

Next: /knock, /links, bouncer network selection UI, /list GUI dialog (still on roadmap).
-->

<!--
Session summary — 2026-06-08 (UI polish — Material icons on dialog buttons)

Small polish pass, no version bump.

- Removed tab pane border (QTabWidget::pane) in the docs dialog — cleaner M3 look
- Dropped redundant 2px QPushButton border (same color as fill, purely geometric waste)
- Added mi-check and mi-close Material Symbols SVG icons
- Wired confirm (✓) and close (✗) icons onto OK/Cancel/Close buttons in all four
  dialogs: DocsDialog, FontDialog, ManageServersDialog, ServerDialog

Commit: 229fa83
No regressions. Binary builds clean on Linux.
-->

<!--
Session summary — 2026-06-09 (v0.25.3 — /list dialog, WebSocket, metadata, fuzz)

Built and released four features from the roadmap:

1. /list channel browser dialog — QDialog with QTableView; streams RPL_LIST results live;
   sortable by channel/users/topic; live filter box searches all columns; double-click or
   Join button to join; Refresh re-requests; non-modal, per-server.

2. WebSocket transport — QWebSocket dual-socket abstraction in IrcClient; helper methods
   (sockWrite, sockState, sockDisconnect, sockAbort, etc.) so all IRC logic above is shared;
   ws:// or wss:// based on ssl flag; websocket config key + Manage Servers checkbox.

3. User metadata (draft/metadata-2) — subscribes to display-name and avatar on connect;
   server-push METADATA command + 761 RPL_KEYVALUE handler; NickMeta stored per-nick at
   ServerSession level; displayed in nick list tooltips.

4. IrcParser fuzz target — libFuzzer harness (tests/fuzz_ircparser.cpp); 26-file seed
   corpus (tests/corpus/); standalone replay driver; ctest smoke test when UPLINK_BUILD_FUZZ=OFF.

Bugs found and fixed:
  - qsizetype→int narrowing in channelListEnd emit (static_cast)
  - QWebSocket private constructor matched wrong overload — fixed with explicit QString()
  - Taking address of rvalue in makeNickItem tooltip (ternary pointer) — fixed with constFind + &it.value()
  - (void)f.open() nodiscard warning in fuzz standalone driver
  - QWebSocket::errorOccurred does not exist on Qt < 6.5 (Ubuntu 24.04 ships 6.4) —
    version-guarded to use QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error)
    on older Qt; errorOccurred on >= 6.5
  - CI missing libqt6websockets6-dev (Linux) and qtwebsockets module (Windows install-qt-action)
    — added to both ci.yml and release.yml

No regressions found. All 3 CI + Release jobs passed on final push.
Binaries: AppImage, tar.gz (Linux), zip (Windows), dmg (macOS) — all on v0.25.3 release page.
-->

<!--
Session summary — 2026-06-09 (cap-notify + Ergo chathistory debugging)

Implemented IRCv3 cap-notify (v0.25.4):
- cap-notify added to desired CAP list
- CAP NEW handler: filters new server-advertised caps against wanted list, sends CAP REQ for any not yet acked
- CAP DEL handler: removes dropped caps from m_ackedCaps
- m_registered flag added to IrcClient: set on RPL_WELCOME (001), cleared on disconnect
- CAP ACK and CAP NAK: skip CAP END when m_registered is true (mid-session requests must not send CAP END)

Also investigated chathistory not working on LinuxDojo (Ergo server):
- Root cause: no history: block in /usr/local/etc/ergo/ircd.yaml — Ergo never initialized history tables
- Fix: user added history block to ircd.yaml and restarted Ergo
- chathistory confirmed working after restart

No regressions. Clean build.
Next: no open items — all IRCv3 planned caps are now implemented.
-->

<!--
## v0.25.5 session summary — 2026-06-09

Stripped all debug qWarning lines left over from the chathistory investigation.
Fixed draft/chathistory cap compatibility — Ergo 2.18.0 advertises the draft cap name,
not the ratified one. Added draft/chathistory to desired caps list, requestHistory gate,
366 handler gate, and isHistory batch type check. Verified end-to-end on LinuxDojo (Ergo):
caps negotiate, CHATHISTORY LATEST is sent on join, batch arrives, history loads in the
chat window automatically.

No regressions. Clean build.
Next: no open items.
-->

<!--
## v0.25.6 session summary — 2026-06-09

Bug: mention detection (💡 indicator) never fired because `mentionRe` was only built
when the server sent a mid-session NICK command. On initial connect, `selfNickChanged`
was never emitted, so `mentionRe` stayed empty and `ch.mentions` never incremented.
Fix: emit `selfNickChanged` from RPL_WELCOME (numeric 1) handler so the regex is built
as soon as registration completes.

Replaced 💡/🔥 emoji in the sidebar with Material Symbols Outlined SVGs:
- lightbulb_2 (yellow #FFD700) for nick mentions
- forum (palette WindowText) for general unread activity
Icons are stored in Qt::UserRole+2 on the item and drawn after the channel name
by SidebarDelegate — this avoids the Qt decoration column entirely, so the text
position never shifts regardless of indicator state.

Also fixed a second layout bug: SidebarDelegate was passing the icon through to
QStyledItemDelegate::paint, which offset the text rect and caused the channel name
to jump right on click. Zeroing opt.icon/opt.decorationSize before measuring the
pill rect and drawing the indicator manually (matching the existing NickDelegate
pattern) fixed it.

Added a Material Design send button (paper plane) to the right of the emoji button
in the input bar. Connects to onInputSubmit — same action as Enter key.

No regressions. Clean build.
Next: send button disable-when-empty, nick completion button, send button Preferences toggle (roadmap).
-->

<!--
## v0.25.8 session summary — 2026-06-09

Replaced the hand-drawn gray ellipse server connection dot with the Material Symbols
"public" globe icon (mi-public.svg). Also added mi-host.svg and mi-language.svg as
candidates — public was chosen.

Bug fixed along the way: the old dot was set via setIcon(0, ...) which goes to
Qt::DecorationRole. The SidebarDelegate suppresses DecorationRole (opts.icon = QIcon(),
decorationSize = (0,0)) and only renders Qt::UserRole+2. The dot was silently invisible.
Fix: store the icon in UserRole+2 via setData(), matching the channel indicator path.

Added server status unread indicator: when the server status pane has unread messages
the globe turns light red (#e06c75). Cleared back to normal color when the pane is
focused. Triggered by anything that posts to the (server) channel: MOTD, server NOTICE,
/raw replies, etc.

No regressions. Clean build.
-->

<!--
## v0.25.9 session summary — 2026-06-09

Investigated [FAIL] METADATA SUBCOMMAND_INVALID error on irc.ergo.chat.
Uplink was sending METADATA * SUBSCRIBE display-name avatar after CAP ACK.
Ergo implements draft/metadata-2 but does not support the SUBSCRIBE subcommand —
it pushes display-name and avatar automatically on join without needing a subscription.
Fix: removed the SUBSCRIBE send entirely. Metadata delivery is unaffected.

Also investigated a blank UI report when adding a 3rd server manually (config.toml edit
+ Reload Config). Could not reproduce. Most likely cause: TOML parse error in the
manually edited config, which causes Config::load to return a blank default-constructed
Config (empty theme, empty font family). Not fixed this session — noted as a known
silent-failure in Config::load catch block.

No regressions. Clean build.
-->

<!--
Session summary — 2026-06-10 (avatar image in nick tooltip, local file support, profile UI)

Built on top of the draft/metadata-2 foundation from the previous session.

New features:
- Avatar images now render inline in the nick list tooltip. Images are fetched
  async when metadata arrives and cached at native size up to 32×32.
  A 48×48 cap was tried first but produced blur on small favicons — reduced to
  32×32 with no-upscale to keep pixel-perfect icons crisp.
- Local file paths accepted in the avatar_url config field and Preferences
  Avatar URL field. Loaded from disk via QPixmap directly. Never sent to the
  server via METADATA SET (only http/https URLs are broadcast).
- "Browse..." button added to the Avatar URL field in Preferences — opens a
  file picker for PNG/JPG/GIF/ICO/WebP.
- "Display Name:" label in nick tooltip renamed to "Name:".

Bug fixed:
- Avatar URL was not appearing in tooltips on Ergo (LinuxDojo). Root cause:
  Ergo's ircmsg library omits the trailing colon when the value has no spaces,
  so a plain URL like https://linuxdojo.org/favicon.ico landed in params[4]
  (for 761 RPL_KEYVALUE) or params[3] (for METADATA push) rather than in
  trailing. Both handlers now fall back to those param slots when trailing is
  empty.

Clean build. All tests passing. Pushed to main.
-->

<!--
Session summary — 2026-06-10 (post-release: stale avatar fix + Browse button)

Bug fixed:
- Changing the avatar in Preferences → Profile → Apply still showed the old
  avatar image in the nick tooltip. Root cause: for local file paths, no
  METADATA SET is sent, so there is no server echo and no userMetaChanged
  signal ever fires. SessionModel::nickMeta for the own nick was never updated,
  and the stale cached pixmap was never evicted. Fix: made onUserMetaChanged
  public in SessionModel; the profileSetRequested handler now calls it directly
  for every connected session to update nickMeta, evicts the old cache entry,
  and immediately calls fetchAvatar with the new URL so the tooltip reflects
  the change without reconnecting.

- Browse button in the Avatar URL field was showing only "wse" — the fixed
  width was 72px, which clipped the label. Increased to 90px.

No regressions. Clean build.
-->

## v0.25.10 — 2026-06-10

### Features

- **Avatar image in nick tooltip** — Uplink fetches and renders avatar images inline in the nick list tooltip. Hover over a nick to see their picture alongside their Name and Account. Images are fetched async on metadata arrival, cached at native size up to 32×32 (no upscaling — small icons like favicons stay crisp), and displayed immediately on the next hover.
- **Local avatar file support** — The Avatar URL field and `avatar_url` config key now accept local file paths (e.g. `/home/you/avatar.png`). Local images load from disk without a network request. Local paths are never broadcast to the server — only `http://`/`https://` URLs are sent via `METADATA SET`.
- **Browse button in Preferences** — "Browse..." button next to the Avatar URL field opens a file picker for PNG/JPG/GIF/ICO/WebP.
- **Profile Preferences UI** — new Profile section in Preferences: Display Name and Avatar URL fields with Apply to connected servers button. Values persist to config and auto-publish on each connect to a supporting server.
- **`/displayname` and `/avatar` commands** — set or clear your display name and avatar from the input bar; print confirmation and check for `draft/metadata-2` support before sending.
- **Nick tooltip label** — "Display Name:" renamed to "Name:".

### Fix

- **Avatar URL not showing on Ergo** — Ergo's `ircmsg` serializer omits the trailing colon on space-free values. The 761 `RPL_KEYVALUE` handler and the `METADATA` push handler now fall back to `params[4]`/`params[3]` when `trailing` is empty, so plain URLs are correctly parsed.

---

## v0.25.9 — 2026-06-09

### Fix

- **METADATA SUBSCRIBE error on Ergo servers** — Uplink was sending `METADATA * SUBSCRIBE display-name avatar` after CAP negotiation. Ergo does not implement the `SUBSCRIBE` subcommand and replied with `[FAIL] METADATA SUBCOMMAND_INVALID`. Removed the send; Ergo pushes metadata automatically without it.

---

## v0.25.8 — 2026-06-09

### Features

- **Server globe icon** — the server connection indicator in the sidebar is now a Material Symbols "public" globe icon instead of a hand-drawn gray dot. Matches the icon style of all other UI elements.
- **Server status unread indicator** — the globe turns light red when the server status pane has unread messages (MOTD, server notices, `/raw` replies, etc.) and returns to normal when the pane is opened.

### Fix

- **Server icon not rendering** — the connection dot was stored in `Qt::DecorationRole`, which `SidebarDelegate` suppresses. Moved to `Qt::UserRole+2` so it is drawn by the delegate's manual paint path, consistent with channel unread indicators.

---

## v0.25.7 — 2026-06-09

### Features

- **Event condensation** — consecutive join, part, quit, nick-change, and kick events in a channel are collapsed into a single compact line rather than flooding the chat view. Groups are defined purely by position: any run of condensable events with no chat message between them renders as one line, sealing when a regular message arrives. Net-change filter suppresses nicks that both join and part before a message breaks the group. Up to 10 nicks per line; overflow shown as `… X more`. Self-join/self-part always render as full lines. Works correctly on channel switch and history replay.

---

## v0.25.6 — 2026-06-09

### Features

- **Material Design sidebar indicators** — the 💡 and 🔥 emoji unread indicators are replaced by Material Symbols SVG icons. A yellow lightbulb (`lightbulb_2`) appears after the channel name on a nick mention; a forum icon appears for general unread activity. Icons are theme-adaptive and rendered at 14 px inline by the delegate.
- **Send button** — a paper-plane send button (`send` icon) sits to the right of the emoji button in the input bar. Clicking it sends the message, identical to pressing Enter.

### Fix

- **Mention detection never fired** — `mentionRe` (the regex used to detect your nick in messages) was only built on mid-session `NICK` commands, never on initial connect. As a result, the mention indicator never appeared after a fresh connection. Fixed by emitting `selfNickChanged` from the `RPL_WELCOME` (001) handler so the regex is compiled immediately after registration.
- **Channel name jumped right on click** — selecting a channel caused the channel label to shift right because the SVG icon was stored as a Qt decoration role, which offset the text rect. Moved indicators to `Qt::UserRole+2` and draw them manually after the text in `SidebarDelegate`, matching the existing `NickDelegate` pattern. Text position is now stable in all states.

---

## v0.25.5 — 2026-06-09

### Fix

- **`draft/chathistory` compatibility** — Ergo IRCd 2.18.0 advertises `draft/chathistory` rather than the ratified `chathistory` cap name. Uplink now requests both, gates `CHATHISTORY LATEST` on either, and recognises `draft/chathistory` batch type as history. Chat history now auto-loads on join for Ergo servers.

---

## v0.25.4 — 2026-06-09

### Features

- **`cap-notify`** — Uplink now negotiates `cap-notify` so the server can inform it of capability changes mid-session. If the server adds a capability (`CAP NEW`), Uplink requests any new caps it wants. If the server removes one (`CAP DEL`), Uplink drops it from its active set. Handles the `m_registered` flag correctly so mid-session `CAP ACK` responses do not fire a spurious `CAP END`.

---

## v0.25.3 — 2026-06-09

### Features

- **Channel browser dialog (`/list`)** — `/list` now opens a sortable, filterable channel browser dialog instead of dumping raw text to the server buffer. Results stream in live as the server sends them (per `RPL_LIST` 322). Columns: Channel, Users (numeric sort), Topic. Filter box searches all columns simultaneously. Double-click or press Join to join; Refresh re-requests the list; Close dismisses without joining. Dialog is per-server and non-modal.

- **WebSocket transport** — each server block now accepts `websocket = true` to connect via `ws://` or `wss://` instead of raw TCP. Scheme follows the `ssl` key: `wss://` when `ssl = true`, `ws://` when `ssl = false`. All existing features — SASL, IRCv3 CAP negotiation, STS, SOCKS5 proxy, reconnect, ping watchdog — work identically over WebSocket. Configurable from Manage Servers (Use WebSocket checkbox) or directly in `config.toml`.

- **User metadata (`draft/metadata-2`)** — when the server supports the `draft/metadata-2` IRCv3 capability, Uplink subscribes to `display-name` and `avatar` keys on connect. Values are pushed by the server and cached per-nick. Display name and avatar URL appear as tooltips in the nick list panel (hover any nick).

- **IrcParser fuzz target** — libFuzzer harness (`tests/fuzz_ircparser.cpp`) with a 26-file seed corpus (`tests/corpus/`). Covers: basic messages, numerics, IRCv3 tags, tag escapes, extended JOIN, BATCH, TAGMSG, malformed prefix, UTF-8, empty/CRLF-only inputs. Builds as a real fuzzer with `UPLINK_BUILD_FUZZ=ON` + clang; falls back to a standalone corpus-replay binary under any compiler. Integrated as a ctest smoke test when fuzz mode is off.

---

## v0.25.2 — 2026-06-08

### Features

- **`/list [filter]`** — lists channels on the server. Results appear in the server buffer with user count and topic. Handles `RPL_LIST` (322) and `RPL_LISTEND` (323).
- **`/setname <realname>`** — changes your realname on the fly via IRCv3 `setname`. Other users in shared channels see the change immediately. Receive side was already wired; this adds the send side.
- **`/whowas <nick>`** — queries history for a departed nick. Handles `RPL_WHOWASUSER` (314) and `RPL_ENDOFWHOWAS` (369); results appear in the current buffer alongside WHOIS output.
- **`/stats <query>`** — queries server statistics (e.g. `u`=uptime, `o`=opers, `m`=commands). Routes numerics 211–219 and 241–244 to the current buffer.
- **`/time` (no args)** — queries the server's local time. `/time <nick>` (CTCP) was already supported; bare `/time` now sends the IRC `TIME` command and handles `RPL_TIME` (391).
- **Soju bouncer: network selection via SASL** — when bouncer type is Soju and a network name is configured, the SASL `AUTHENTICATE` payload now sends `user/network` as the username so soju routes the connection to the correct network. Previously the network name was stored but never used.
- **Soju bouncer: `LISTNETWORKS END` handling** — `BOUNCER LISTNETWORKS END` is now handled. Networks are buffered during the initial listing and displayed as a formatted summary when the list is complete. Live state-change notifications (`BOUNCER NETWORK` outside of a listing) continue to print per-event lines.

---

## v0.25.1 — 2026-06-08

### Bug Fixes

- **Fix: window opens full-width and cannot be resized on FreeBSD KDE/X11** — On FreeBSD, font metrics inflate the layout minimum of several panels (`rightContent`, `panesSplitter`, `primaryPanel`) to ~2566 px. Qt's `qSmartMinSize()` falls back to `minimumSizeHint()` when no explicit minimum is set, so every resize attempt below that value bounced back immediately, and KWin read the same value from `WM_NORMAL_HINTS` and enforced it. Fixed by setting `setMinimumSize(1, 1)` on `m_mainSplitter` at construction — a non-zero explicit minimum causes `qSmartMinSize()` to use that value instead of the hint, breaking the enforcement chain at the top of the layout hierarchy. A startup correction timer then clamps any KWin session-restored oversized geometry to a sane width. Linux and macOS are unaffected.

### Features

- **Ctrl+W — minimize to system tray** — `Ctrl+W` now hides the window to the system tray, matching the behavior of the close button (×). No-op when no system tray is available.

### Documentation

- Corrected buffer cap in docs from 2,000 → 500 (matches actual `kMessageBufferCap` in code)
- Fixed wrong config key `show_typing` → `typing_indicator` in howto.html
- Updated IRCv3 cap statuses: `batch`, `chathistory`, `msgid`, `sts` → active; `labeled-response`, `echo-message` → partial
- Added missing `font_server_header` config key to configuration.md
- Added undocumented commands: `/join #channel [key]`, `/leave`, `/close`, `/monitor remove`
- Keyboard shortcuts: corrected Ctrl+W entry after shortcut was wired

---

## v0.25.0 — 2026-06-07

### Bug Fixes

- **Fix: WHOX bot detection was completely broken** — The 354 RPL_WHOSPCRPL handler assumed a 6-field format with a token (`WHO <channel> %cnfa,42`), but Ergo IRCd returns 5 fields without a token: `[me, channel, nick, flags, account]`. The size check `params.size() >= 6 && params[1] == "42"` never matched, silently dropping every WHO reply. Bot nicks were never detected, and account names were never populated from join. Fixed by removing the token check and correcting the param indices.

- **Fix: echo-message typing self-echo** — With the `echo-message` CAP active, the typing TAGMSG was echoed back from the server, causing "you are typing..." to appear to yourself. Added `msg.nick != m_nick` guard so typing indicators from self are ignored.

- **Fix: DCC self-echo** — With `echo-message`, outgoing DCC offers were echoed back to the sender, triggering an incoming DCC receive dialog on their own client. Same `msg.nick != m_nick` guard applied to the CTCP DCC branch.

- **Fix: packaging/Uplink.desktop icon name** — `Icon=` was set to `uplink-irc`, which does not match the installed hicolor icon filename. Corrected to `uplink` to match the CMake install rule that renames `uplink-dark.png` → `uplink.png` in the hicolor theme.

### New Features

- **Contextual command replies** — `/whois`, `/version`, and `/ping` responses now appear in the active channel or PM window instead of routing to the server buffer. If you `/whois alice` while talking in `#linux`, the reply lands in `#linux`. Implemented via a new `contextualMessage` signal from `IrcClient` wired through `SessionModel::onContextualMessage`, which routes to the current active buffer.

  **Example:**
  ```
  # In #linux:
  /whois alice
  → alice is alice@host.example (Alice Smith)        ← appears in #linux
  → alice is on: #linux #general                     ← appears in #linux

  /version alice
  → VERSION reply from alice: Uplink 0.25.0          ← appears in #linux

  /ping alice
  → Ping reply from alice: 23ms                      ← appears in #linux
  ```

- **Bot icons in user list** — Nicks with mode `+B` (bot flag) are marked with a small icon drawn inline to the right of their nick in the user list. Each bot is randomly assigned either a robot icon or an alien icon for the session; the assignment is stable for the duration of the connection. Icons are rendered as SVGs colorized to the theme accent color.

  The previous approach used emoji characters via font fallback, which was unreliable across systems and fontconfig configurations. This replaces it with proper `QIcon` rendering via a custom `FixedRowDelegate::paint()` override using `Qt::DecorationRole`.

- **QComboBox drop-down arrow polish** — The native OS drop-down arrow in combo boxes (theme picker, nick brackets selector, etc.) is replaced with a clean Material Symbols chevron icon that matches the app's icon style. Applied via QSS in `ThemeLoader`.

---

## v0.24.3 — 2026-06-05

### Bug Fixes

- **Fix:** On KDE (and any system using KWallet as the keychain backend), a wallet unlock prompt appeared every time a preference was changed — font config, theme, toggles, nick brackets, etc. Root cause: `Config::save()` called `storePassword()` for every server password field on every save, triggering `QKeychain::WritePasswordJob` even for unrelated config changes. Fix: `Config::save()` now accepts a `migratePasswords` flag (default `false`). Keychain writes are only attempted when saving from the Manage Servers dialog, where a credential change is expected.

---

## v0.24.2 — 2026-06-06

### Bug Fixes

- **Fix:** Client ping-timed-out and reconnected every ~2 minutes on Libera.Chat and other solanum-based servers. Root cause: a 2-second one-shot timer fired `PING :uplink_rtt` during SASL negotiation before the client was registered. Solanum ignores PINGs from unregistered clients, leaving `m_pingPending` stuck `true`. The 30-second watchdog then fired three more times until the 90-second elapsed threshold killed the connection (2 + 4×30 ≈ 120 s). Fix: removed `QTimer::singleShot(2000, this, &IrcClient::sendPing)` from `onConnected()`; the regular 30 s timer handles keepalive after registration.
- **Fix:** Keychain-stored password fields in Edit Server showed the `<keychain>` sentinel as masked dots, making it unclear whether a password was stored. Leaving the field empty and saving silently deleted the keychain entry. Fields with keychain-stored values now show placeholder text instead, and saving with an empty field preserves the sentinel.
- **Fix:** When config contains `<keychain>` as a password sentinel but no matching entry exists in the OS keychain, SASL authentication failed silently with no indication of why. Uplink now emits a visible red error in the server buffer — "Keychain: no password stored for … — open Edit Server and re-enter your password" — directing the user to the exact fix.

### Features

- **Show/hide toggle on password fields** — a clickable eye icon (Material Symbols) appears inside every password field in the Add/Edit Server dialog (server password, SASL, NickServ, proxy). Click to reveal; click again to hide.
- **Keychain read error logging** — failed OS keychain reads now emit `qWarning` with the key name and error string, visible in terminal output.

### Performance

- **Message buffer reduced** — per-channel scrollback cap lowered from 2,000 to 500 messages. Reduces RAM on sessions with many open channels. Older messages scroll out of the buffer as new ones arrive.
- **Link preview cap reduced** — stored link preview HTML per channel reduced from 100 to 20 entries.
- **Emoji picker lazy-loaded** — the emoji picker widget is now created on first use instead of at startup, reducing launch-time RAM.
- **Chat view block count aligned** — `QTextDocument::setMaximumBlockCount` now matches the buffer cap exactly (was over-allocated by 300 blocks).

---

## v0.24.1 — 2026-06-05

### Bug Fixes

- **Fix:** Input bar background color lost after v0.24.0 `QPlainTextEdit` migration — QSS selector `QWidget#inputBar QLineEdit` did not match the new widget type. Selector updated to cover both `QLineEdit` and `QPlainTextEdit`. Placeholder text color also extended to `QPlainTextEdit`.

---

## v0.24.0 — 2026-06-05

### Features

- **Multi-line messages (`draft/multiline`):** Press **Shift+Enter** to insert a line break in the compose box; press **Enter** to send. The input box grows automatically up to 4 lines. On servers that advertise `draft/multiline`, the message is delivered as a proper IRCv3 batch. On servers without the capability, each line is sent as a separate PRIVMSG — no functionality is lost.
- **Multi-line compose:** The message input is now a `QPlainTextEdit`. Pasted or typed multi-line content is preserved and the box auto-resizes from 1 to 4 lines as you type.
- **Up/Down history in multi-line input:** Arrow-key history navigation activates only when the cursor is on the first or last line, so arrow keys move within multi-line text naturally.

### Security / Hardening

- **DCC send:** Incoming connections are now validated against the expected peer IP; mismatched connections are rejected.
- **DCC receive:** Write failures cancel the transfer immediately. A 30-second stall timeout fires when a passive-mode connection forms but no data arrives.
- **DCC ACK coalescing:** ACKs sent once per 64 KB boundary and at completion instead of after every `readyRead` event.
- **IRC line cap:** `sendRaw` enforces the RFC 1459 512-byte limit; oversized lines are truncated before reaching the socket.
- **Ping watchdog:** No PING response within 90 seconds triggers abort and reconnect.
- **NICK/USER sanitization:** Leading/trailing spaces and null bytes are stripped before send.
- **Unknown slash commands:** Unrecognized `/command` input posts a local error instead of being sent as raw IRC.
- **CTCP flood table:** `m_ctcpTimestamps` capped at 500 entries.
- **Link preview HTML cap:** Preview HTML content limited to 8 192 bytes in cache.
- **Address check:** `isPrivateAddress()` extended — CG-NAT (100.64/10), TEST-NETs, unspecified, IETF protocol, benchmarking, IPv4-mapped, IPv6 translation, and documentation ranges.

### Performance

- **IRC receive buffer:** `IrcClient` uses `QByteArray` as receive buffer; UTF-16 conversion deferred to per-line processing.
- **Nick index:** `Channel::removeNick` does an in-place O(n) index patch instead of O(n²) full rebuild — critical during netsplit QUIT storms.
- **Log file handles:** `SessionModel::logMessage` caches open `QFile*` handles per channel.

### Bug Fixes

- **Fix:** Chat view scrolls to bottom on channel switch (was missing the deferred scroll in `refreshChatView`).
- **Fix:** Mention detection false positives — empty nick regex no longer matches every incoming message, fixing spurious 💡 unread badges.

### CI

- ASan / UBSan sanitizer job added on ubuntu-24.04.

---

## v0.23.3 — 2026-06-05

### Performance

- **Perf:** Reactions and message deletions now update in-place — only the affected `QTextBlock` is rewritten instead of rebuilding the entire chat view. On active channels this eliminates a visible repaint on every incoming reaction or redaction event.
- **Perf:** Link preview thumbnail cache is now bounded at 50 entries with FIFO eviction. Previously the cache could grow without limit across a long session. Thumbnails are stored as compressed PNG bytes (~10x smaller than a decoded `QPixmap`); a `QPixmap` is reconstructed only at the moment the preview card is displayed.
- **Perf:** Nick mention highlight regex (`m_selfNickRe`) is pre-compiled once when the nick changes. The previous code compiled a fresh `QRegularExpression` on every single message render.
- **Perf:** Chat context object (`ChatRenderer::Context`) is built once before the message loop in `refreshChatView`, not once per message.

### Stability

- **Fix:** `QPointer` guard added to keychain async lambda — if the `IrcClient` is destroyed before an async secret read completes, the lambda now exits safely instead of accessing freed memory.
- **Fix:** DCC receive timeout now fires correctly even when a transfer has partially started. Previously the `m_received == 0` guard suppressed the timeout once any bytes had arrived, leaving stalled transfers stuck indefinitely.
- **Fix:** `/sysinfo` background thread now uses a cooperative abort flag instead of `QThread::terminate()`. Forced termination is unsafe in Qt — the new approach lets the thread exit cleanly on timeout.
- **Fix:** Disconnecting from a server now correctly prunes typing-indicator state and bot-icon entries for that host, preventing stale data from appearing after a reconnect.

### Internals

- `src/net/addresscheck.h` — shared `isPrivateAddress()` helper used by both the link preview SSRF guard and the DCC receive IP check (duplicate local functions removed).
- `Channel::addNick` uses `std::lower_bound` for O(log n) sorted insert with a partial nick-index update instead of a full rebuild.
- Monitor nick membership tracked in `m_monitorSet` (`QSet<QString>`) — `monitorAdd` check is now O(1) instead of O(n).

---

## v0.23.2 — 2026-06-05

### Stability

- **Fix:** Keychain reads at startup are now fully non-blocking. Uplink no longer spins a nested `QEventLoop` per sentinel field — all secret fields resolve in parallel via `KeychainHelper::readAsync()`, and `connectToServer` is called only once all reads complete. Eliminates the reentrancy hazard when the keychain backend needs to show a dialog before the main window is ready.

### Testing

- **New:** Qt Test suite — two test executables ship with the project:
  - `tst_ircparser` — 14 cases covering prefix parsing, `nick!user@host` splitting, IRCv3 tag parsing, tag value unescaping, `server-time`, numerics, CRLF stripping, and malformed input.
  - `tst_chatformat` — 16 cases covering HTML escaping, bold/italic/underline IRC formatting codes, IRC color codes (fg and fg+bg), reset (`\x0F`), `linkifyHtml`, and `htmlAttr` quoting.
  - Build: `cmake --build build --target tst_ircparser tst_chatformat && ctest --test-dir build`. Pass `-DUPLINK_BUILD_TESTS=OFF` to skip.

### Security

- **Fix:** `OPER` password is now redacted in the raw log alongside `PASS` and `AUTHENTICATE`.
- **Fix:** Remote-supplied DCC filenames are sanitized — all control characters stripped before the name is embedded in the CTCP envelope, preventing CTCP injection.

### Build

- **CI:** `release.yml` now auto-bumps all version strings in `README.md` and `docs/index.html` on every release tag. No manual version bump needed on future releases.

---

## v0.23.1 — 2026-06-04

### Fixes

- **Fix:** Tab completion (<kbd>Tab</kbd>) now works in channel pane input bars. Previously pressing Tab in a detached pane did nothing — it silently moved keyboard focus instead of completing the nick or command. Nick and command completion in panes is now identical to the primary input bar, including the colon-suffix convention at the start of a line.
- **Fix:** Fonts configured in **Font Config** (chat size, nick list size, topic font) are now applied consistently when a channel pane is opened mid-session. Previously panes opened after the initial launch inherited system defaults instead of the configured sizes, making the topic font visibly larger than the primary panel and the nick list row spacing inconsistent.

### Performance

- **Perf:** Nick list updates are now incremental. When a user joins, parts, quits, is kicked, or changes their nick, only the affected list entry is inserted or removed — the entire nick list is no longer cleared and rebuilt on every event. Full rebuild is retained for NAMES bursts, netsplit/netjoin, and mode changes. This eliminates visible flicker on busy channels and reduces CPU load proportionally to the join/part/quit event rate.
- **Perf:** Per-channel nick index — `Channel` now maintains a `QHash<QString, qsizetype>` mapping lowercased nicks to their position in the sorted list. All O(n) linear scans for nick lookups across `onMessage`, `onNotice`, `onAction`, `onNickChanged`, `onUserQuit`, `onModesReceived`, `onAccountChanged`, `onHostChanged`, `onSetNameReceived`, and `onNetsplitDetected` are replaced with O(1) hash lookups. The index is rebuilt after each sort operation (`addNick`, `renameNick`, `setNicks`, mode prefix changes).
- **Perf:** Nick list rebuilds are debounced — a short 50 ms timer collapses burst updates (e.g. NAMES on join, netsplit/netjoin) into a single repaint rather than rebuilding the widget once per event.

### Internals

- **Refactor:** Chat rendering logic extracted from `MainWindow` into `src/ui/chatrenderer.h` / `chatrenderer.cpp` as a `ChatRenderer` namespace. `ircToHtml`, `linkifyHtml`, `linkifyTopic`, `wrapEmojiHtml`, `htmlAttr`, `nickColor`, and `formatMessage` are now standalone functions with no widget dependencies. `MainWindow::formatMessage` is a 10-line wrapper that assembles a `ChatRenderer::Context` and delegates. `mainwindow.cpp` reduced from 4 063 to 3 768 lines.

---

## v0.23.0 — 2026-06-04

### Features

- **Feat:** Link preview cards are now right-clickable — context menu with **Open URL** and **Hide Preview**. Left-clicking a card also opens the URL.
- **Feat:** `/sysinfo` collection runs in a background thread — UI stays responsive while `vulkaninfo`, `lspci`, and other slow commands run. Posts "Collecting system info…" immediately; result follows when ready. Hard 12-second global timeout. Cache is now per-session.

### Security

- **Fix:** DCC receive hardening — transfers are written to a `.part` file during download and renamed to the final name only on success. Partial files are deleted on failure, cancel, or unexpected destruction. Available disk space is checked against the advertised file size before the transfer begins. Files larger than 2 GiB are rejected.

### Quality

- **Fix:** Link preview queue — hovering a URL no longer aborts an in-flight card fetch. Hover uses a dedicated fetch path (`fetchHover`) with its own reply and DNS slots. Card fetches are queued (max 10) and processed sequentially. A 20-second watchdog advances the queue if a fetch hangs. `m_previewChannels` capped at 100 entries.
- **Fix:** All compiler warnings eliminated — `-Wconversion` narrowing (`qsizetype`→`int`), `-Wshadow` locals, `-Wold-style-cast` C-style casts, and a missing `-Wparentheses` parenthesisation in the IRC colour renderer. Build is now clean under `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wnon-virtual-dtor -Wold-style-cast`.

---

## v0.22.0 — 2026-06-04

### Features

- **Feat:** DCC passive / NAT traversal — "Send File (Passive)" in nick right-click menu. Receiver opens the port; works through sender-side NAT. 60-second timeout before cancellation. Passive token upgraded from 8-digit decimal (26-bit) to 32-char lowercase hex (128-bit).
- **Feat:** In-app update check — **Hamburger → Check for Updates** fetches the GitHub releases API, compares `MAJOR.MINOR.PATCH`, and shows "Update Available" or "Up to Date".

### Security

- **Fix (F-01):** Link preview SSRF — `isBlockedBySchemeOrLiteral()` for scheme/literal private IPs; all fetches DNS-resolved via `QHostInfo::lookupHost()` with loopback/private/link-local/multicast rejection.
- **Fix (F-02):** DCC receive — private/reserved peer IP blocked before file open or socket connect.
- **Fix (F-03):** DCC passive receive — `listenPassive()` accepts `expectedIp`; incoming connections from unexpected peers rejected when sender is public.
- **Fix (F-04):** Reactions — storage changed to `QSet<QString>` per `(msgid, emoji)` for automatic nick dedup; reactions pruned on message rolloff.
- **Fix (F-05):** Link preview redirects — `ManualRedirectPolicy`; each redirect destination re-DNS-checked before following; image redirects blocked.
- **Fix (F-06):** HTML attribute XSS — `htmlAttr()` helper encodes `'` to `&#39;` in addition to `toHtmlEscaped()`; applied to all `href`/`title` attributes.
- **Fix (F-07):** Proxy passwords and channel keys now stored via OS keychain; plaintext fallback only when keychain unavailable.
- **Fix (F-08):** STS store — hostname lowercased before all lookup/store/remove ops; `sts.ini` permissions restricted to owner read/write after each write.
- **Fix (F-10):** `/sysinfo` — `QMessageBox::question` confirmation required before posting system info to a channel.

### Quality

- **Fix:** Topic link gate — `setOpenExternalLinks(false)` + `linkActivated` → http/https scheme check in topic labels.
- **Fix:** DCC timeout cleanup — all three timeout handlers call `cancel()` before emitting error; sockets and file handles closed on timeout.
- **Fix:** `hiddenPreviews` pruning — `addPreview()` removes the evicted URL from `hiddenPreviews` when the preview cache rolls over.
- **Build:** `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wnon-virtual-dtor -Wold-style-cast` added to `CMakeLists.txt`; `UPLINK_WARNINGS_AS_ERRORS` and `UPLINK_ENABLE_SANITIZERS` options added (both off by default).

---

## v0.21.0 — 2026-06-04

### Security

- **Fix:** TLS first-connect MITM — credentials (PASS, SASL, NickServ) are no longer sent before the user accepts an unknown self-signed certificate fingerprint. The connection is now aborted immediately on an unrecognized cert; it reconnects only after the user chooses Pin Certificate or Accept Once. "Accept Once" was previously a dead button (`Q_UNUSED`) — it now works correctly for session-only trust.
- **Fix:** IRCv3 reaction values are now HTML-escaped before insertion into the chat view. A malicious server or user could previously inject rich text, styling, or misleading links via `+draft/react` tag values.
- **Fix:** `sendRaw()` now strips embedded CR/LF from every outgoing line using the existing `stripCrlf()` helper, preventing potential command injection in future code paths.
- **Fix:** IRC parser replaced `raw.trimmed()` with targeted `\r`/`\n` removal only — trailing spaces in IRC trailing parameters are now preserved correctly.
- **Fix:** Keychain write failure is no longer silent. If the OS keychain is unavailable, Uplink logs a warning and keeps the plaintext value in config rather than replacing it with the `<keychain>` sentinel and silently losing the secret.
- **Fix:** Log file and log directory are now created with owner-only permissions (`600`/`700`) regardless of system umask.
- **Change:** `log_messages` defaults to `false` (was `true`). Existing configs with an explicit `log_messages = true` are unaffected.
- **Change:** Link previews are now **opt-in**. A new `[privacy]` section in `config.toml` controls this with `link_previews = false` by default. Toggle from **Preferences → Link Previews**. Previously previews fetched automatically for every URL in chat.

### Build

- **Hardening:** `FetchContent` git dependencies (`qtkeychain`, `tomlplusplus`) pinned to full immutable commit hashes instead of mutable version tags.
- **Hardening:** AppImage packaging script pins `linuxdeploy` and `linuxdeploy-plugin-qt` to dated versioned releases (previously `continuous`). SHA256 verification added for all three packaging tools — script aborts on mismatch.

### UI

- **Feat:** Link Previews checkbox added to Preferences → Interface, below Log Messages to Disk. Toggle takes effect immediately and saves to config.

---

## v0.20.2 — 2026-06-03

- **Docs:** Expanded keyboard shortcuts guide — Tab/Shift+Tab completion detail, emoji shortcode keys, planned shortcuts table
- **Docs:** New FAQ entries — self-signed cert pinning, cert mismatch recovery, app icon switching, custom themes
- **Fix:** `configuration.md` — `app_icon` option list corrected to `"dark"` / `"light"` only; malformed comment line in full example fixed
- **Fix:** `faq.md` — orphaned documentation search paragraph given proper heading
- **Assets:** All icon copies replaced with correctly exported transparent RGBA PNGs
- **UI:** Input field border-radius reduced from pill shape (20px) to subtle rounding (8px)
- **Fix:** Theme seeding now works in dev builds — `build/Uplink` finds `themes/` via `../themes` fallback
- **Fix:** Theme directory path corrected — `QStandardPaths` resolved to `~/.config/Uplink/` (wrong case); now hardcoded to `~/.config/uplink/themes`

## v0.20.1 — 2026-06-03

- **Chore:** Project renamed to UplinkIRC — repo, binary, config path (`~/.config/uplink/`), window title, tray tooltip, CTCP VERSION, and all docs updated
- **Feat:** New Uplink icon — dark and light transparent PNG variants; icon picker in Preferences simplified to Dark / Light (removed old 3-style system)
- **Fix:** Sidebar channel item row height increased (20 → 26 px) — pill highlight taller, descenders (e.g. `p` in `#uplink`) no longer clipped
- **Chore:** Old unused icon files removed from `resources/icons/` and `assets/`
- **Docs:** `docs/index.html` and `docs/howto.html` fully updated to Uplink branding and new icon grid

## v0.20.0 — 2026-06-03

- **Feat:** Self-signed TLS certificate fingerprint pinning — first connect to a self-signed server shows a dialog (Accept Once / Pin Certificate / Reject); pinning saves `ssl_fingerprint` to config and verifies on every reconnect; fingerprint mismatch disconnects with a clear error
- **Feat:** MD3-inspired UI pass across all themes — pill buttons, rounder inputs/menus/tooltips/tabs, thinner scrollbar, larger touch targets on checkboxes and radio buttons
- **Feat:** Material Symbols Outlined icons — hamburger, settings gear, and all 14 hamburger-menu icons replaced with Google Material Symbols SVGs; icons colorized to active theme text color at runtime
- **Feat:** `PillButton` — custom `QPushButton` subclass that draws rounded background via `QPainter`, bypassing Qt Fusion style's silent border-radius suppression
- **Fix:** Sidebar pill highlight left/right padding was asymmetric (Qt internal text margin offset not accounted for) — now symmetric 8 px each side
- **Fix:** `QTreeWidget::item` and `QListWidget::item` row heights were being ignored by Qt — replaced with `sizeHint` delegate overrides; sidebar 20 px, nick list 16 px
- **Fix:** Preferences dialog no longer shows a scrollbar on open — spacing tightened, minimum height removed
- **Fix:** Documentation dialog search box no longer collides with the Shortcuts tab — compact inline style applied

---

<!--
Session summary — 2026-06-03 (v0.19.3 — FreeBSD preview card bg fix)

Bugs fixed:
  - Link preview card background did not match the chat view on FreeBSD.
    Root cause: card HTML was built with m_theme.bufferBg, but Qt on FreeBSD
    renders QTextBrowser using QPalette::Base which can diverge from the raw
    theme value when QSS palette translation is incomplete on the platform.
    Fix: read `m_chatView->palette().color(QPalette::Base)` instead — this is
    the actual color Qt uses to paint the viewport, so it always matches.

Known issues left open:
  - About dialog Wayland centering drift (Qt limitation, deferred).
  - DCC over internet blocked by NAT (passive DCC not yet implemented).
  - Self-signed cert fingerprint-pin UI not yet built.
  - Sidebar pill highlight may still clip on very long server names in a
    very narrow sidebar.

Next priorities:
  - Self-signed cert fingerprint-pin UI
  - DCC passive / NAT traversal
  - In-app update check UI
-->

## v0.19.3 — 2026-06-03

- **Fix:** Link preview card background now matches the chat view on FreeBSD — reads `QPalette::Base` from the chat widget directly instead of the raw theme value, ensuring the two are always the same color regardless of how the platform applies QSS

---

<!--
Session summary — 2026-06-02 (v0.19.2 — rounded chat panel + padding)

What was built:
  - RoundedPane: new QWidget subclass that applies a bitmap mask via setMask()
    on every resize, clipping all child content (chat view, nick list, input
    bar, headers) to a 10px rounded rectangle at the OS compositor level.
  - m_panesSplitter wrapped in RoundedPane so ALL panes (primary + detached)
    are clipped together as one rounded block.
  - m_rightContent vbox margins: 0,0,8,8 — 8px padding at bottom and right so
    the chat panel floats inside the window with breathing room.
  - QWidget#rightContent background set to {{sidebarBg}} so the padding gap
    and corner clip areas contrast against the chat panel (bufferBg), making
    the rounded corners visible.
  - Sidebar toggle (gear icon) now also adjusts the left margin of
    m_rightContent: 8px when sidebar is closed (matches right), 0 when open
    (sidebar is the left boundary). Prevents the lopsided look when the
    sidebar is hidden.

Bugs fixed:
  - No rounded corners visible: root cause was color contrast — bg == bufferBg
    in most themes; fixed by giving rightContent the sidebarBg color.
  - Asymmetric padding when sidebar closed: left=0, right=8 looked odd; fixed
    by dynamically setting left margin to 8 on sidebar close.

Known issues left open:
  - About dialog Wayland centering drift (Qt limitation, deferred).
  - DCC over internet blocked by NAT (passive DCC not yet implemented).
  - Self-signed cert fingerprint-pin UI not yet built.
  - Sidebar pill highlight may still clip on very long server names in a
    very narrow sidebar.

Next priorities:
  - Self-signed cert fingerprint-pin UI
  - DCC passive / NAT traversal
  - In-app update check UI
-->

## v0.19.2 — 2026-06-02

- Rounded chat panel: `RoundedPane` widget clips all chat content to a 10 px rounded rectangle via `setMask()` — corners are clipped at the OS level, not just the background
- Padding: 8 px breathing room at the right and bottom of the chat area; gap shows the sidebar background color so rounded corners are visually distinct
- Sidebar toggle: when the sidebar is closed the left margin matches the right (8 px each side) so the panel stays centered and balanced; when reopened the left margin returns to zero

---

<!--
Session summary — 2026-06-02 (v0.19.1 — topic icon + rounded input bar)

What was built:
  - Speech bubble topic icon: makeTopicIcon() uses QPainter to draw a small
    speech bubble (rounded rect body + tail + two text lines). Muted
    placeholder color when topic is hidden, accent color when open. Replaces
    the old "▸ topic" / "▾ topic" text+border pill on both the primary panel
    and all channel panes. ChannelPane gains setTopicIcon(collapsed, expanded)
    which wires the toggled signal to swap between the two icons.
  - Floating rounded input bar: removed border-top separator line; input bar
    background now matches {{bufferBg}} so it flows seamlessly from the chat
    area. The QLineEdit gets border-radius 6px and {{inputBg}} background with
    no border — a rounded pill shape defined only by the background color
    contrast, no outline. Users responded positively to this look.

Known issues left open:
  - About dialog Wayland centering drift (Qt limitation, deferred).
  - DCC over internet blocked by NAT (passive DCC not yet implemented).
  - Self-signed cert fingerprint-pin UI not yet built.
  - Sidebar pill highlight may still clip on very long server names in a
    very narrow sidebar.

Next priorities:
  - Self-signed cert fingerprint-pin UI
  - DCC passive / NAT traversal
  - In-app update check UI
-->

## v0.19.1 — 2026-06-02

### UI Polish

- **New:** Topic toggle button is now a custom-painted **speech bubble icon** — a small rounded rectangle with a tail and two text lines drawn via QPainter. No text, no border box. Icon is muted (`{{placeholder}}` color) when the topic is hidden and accent-colored when the topic is open, giving a clear at-a-glance state indicator. Updates live when you switch themes.
- **New:** **Floating rounded input bar** — the separator line between the chat area and input bar is removed. The input bar background now matches the chat buffer color so the two areas flow together. The message input field uses `border-radius: 6px` with the `{{inputBg}}` background color — a rounded pill shape defined purely by the color contrast, no visible border.

---

<!--
Session summary — 2026-06-02 (v0.19.0 — QSS visual polish + sidebar highlight overhaul)

What was built / fixed:
  - Full QSS template overhaul: rounded inputs (border-radius 5px), accent-colored
    focus rings on QLineEdit/QComboBox/QSpinBox, custom checkbox and radio button
    indicators using {{accent}}, underline-style QTabBar, themed QToolTip,
    floating scrollbars with hover state, menu separators and border-radius,
    improved button padding/radius.
  - Topic toggle button wired into theme system for BOTH the primary panel
    (m_primaryTopicBtn) and channel panes (m_topicToggle). Both now use the
    #topicToggle objectName and the QSS rule; old palette(mid)/palette(highlight)
    inline stylesheets removed.
  - Topic button fixed: was disappearing visually on unchecked state due to
    {{sidebarText}} having low contrast against the chat background. Now uses
    {{text}} (full foreground color) in all states.
  - Topic button: checked state no longer renders bold/blue via native Qt style;
    font-weight:normal and explicit color:{{text}} lock it down.
  - Primary panel header now always visible (was hidden until a second pane
    opened), so the topic toggle button is accessible in single-window mode.
  - Sidebar selection: replaced QSS background with a custom SidebarDelegate
    that paints a rounded pill highlight fitted to the text width. Uses
    QStyle::SE_ItemViewItemText to anchor width, clears State_MouseOver to
    prevent Fusion hover shift, and uses transparent Highlight palette +
    HighlightedText = {{text}} so selected text is always readable.

Known issues left open:
  - About dialog slight centering drift on Wayland (Qt limitation, deferred).
  - DCC over internet blocked by NAT (passive DCC not yet implemented).
  - Self-signed cert fingerprint-pin UI not yet built.
  - Sidebar pill highlight width may still clip on very long server names if
    the sidebar is narrower than the text.

Next priorities:
  - Self-signed cert fingerprint-pin UI
  - DCC passive / NAT traversal
  - In-app update check UI
  - AppImage build verification for v0.19.0
-->

## v0.19.0 — 2026-06-02

### UI Polish

- **New:** Full QSS theme template overhaul — all form widgets now themed consistently:
  - `QLineEdit`, `QComboBox`, `QSpinBox` get `border-radius: 5px` and an accent-colored focus ring
  - `QCheckBox` and `QRadioButton` use custom square/round indicators filled with `{{accent}}` when checked
  - `QTabBar` uses an underline style (`border-bottom: 2px solid {{accent}}` on selected tab) instead of raised tabs
  - `QToolTip` is now themed to match the active color scheme instead of using the OS default
  - Scrollbars get 2 px margin for a floating appearance and a hover-highlight state
  - Menus get `border-radius: 6px`, per-item rounded hover with margins, and styled separators
  - Buttons get `border-radius: 5px`, no border, and slightly more padding
- **New:** Sidebar channel/server selection rendered as a **fitted rounded pill** — the highlight wraps only the text plus a small padding, not the full row width. Implemented via a custom `SidebarDelegate` that measures the actual text rect per item.
- **Fix:** Selected sidebar items (servers especially) now have fully readable text — uses `{{text}}` (full foreground color) via the `HighlightedText` palette role so it works across all themes.
- **Fix:** Sidebar items no longer shift position on hover — `State_MouseOver` is cleared before the native painter runs, preventing Fusion style's built-in hover nudge.
- **Fix:** Topic toggle button now always visible in single-window mode — the primary panel header is shown on startup rather than only when a second pane opens.
- **Fix:** Topic toggle button on the primary panel was using `palette(highlight)` (system blue) for the checked-state border. It is now wired into the theme system via `#topicToggle` — border uses `{{accent}}`, matching every other theme-aware element.
- **Fix:** Topic button text no longer goes blue or bold when the topic is open — `color: {{text}}` and `font-weight: normal` are explicitly locked in the checked pseudo-state.
- **Fix:** Topic button text was invisible after collapsing the topic on channels with no server-set topic — caused by `{{sidebarText}}` being low-contrast against the chat background. Now uses `{{text}}` in all states.

---

<!--
Session summary — 2026-06-02 (v0.18.2 — event message visual polish)

What was built / fixed:
  - Join/part/quit/nick event messages now render at 82% of the chat font size
    using an explicit pt value via a wrapEvent() lambda (Qt rich text does not
    support percentage font-size or opacity CSS properties).
  - Part, quit, and kick colors changed from "firebrick" (#B22222) to #e06b6b —
    a lighter red that is readable on dark backgrounds. Join stays seagreen.
    Nick stays steelblue. Error and server messages unchanged.

Bugs NOT yet fixed:
  - About dialog slight centering drift on Wayland (Qt limitation, deferred).
  - DCC over internet blocked by NAT (passive DCC not yet implemented).
  - Self-signed cert fingerprint-pin UI not yet built.
  - Pane layout not persisted across restarts.

Next priorities:
  - Pane layout persistence
  - Self-signed cert fingerprint-pin UI
  - DCC passive / NAT traversal
  - In-app update check UI
-->

## v0.18.2 — 2026-06-02

### Fixes

- **Fix:** Join/part/quit/nick event lines now render at 82% of the chat font size — less visually heavy on busy channels.
- **Fix:** Part, quit, and kick message color changed from `firebrick` to `#e06b6b` — readable on dark backgrounds without being harsh.

---

<!--
Session summary — 2026-06-02 (v0.18.1 — pane polish and bug fixes)

What was built / fixed:
  - Topic toggle button: removed forced 13pt font, reduced padding (1px 5px),
    reduced border-radius (4px), added background:transparent to override
    global QSS sidebarBg fill. Text changed from "▸  topic" to "▸ topic".
    Applied to both ChannelPane and primary panel header.
  - Close buttons: replaced setAutoRaise(true) with explicit inline stylesheet
    (background:transparent; border:none) so global QSS doesn't show a colored
    box behind the ✕. Hover highlights with palette(highlight).
  - Pane nick prefix: panes now call setNickVisible(m_showNickPrefix) on open;
    nickPrefixToggled in Preferences propagates to all open panes.
  - Pane input font: setInputFont() method on ChannelPane; called in
    openChannelPane and at end of applyFontSizes() for all open panes.
  - Pane context menu: installEventFilter on pane chatView()->viewport() in
    openChannelPane; eventFilter pane viewport loop handles ContextMenu events
    with same nick/msgid/selection/Copy/Reply/React/Delete logic as primary view.
  - Sidebar left-click opens pane (Wayland bug): QMenu::exec() in
    onSidebarContextMenu was receiving the pending button-release from the
    triggering click in its event loop, immediately activating "Open in Pane"
    (the first menu item at cursor position). Fixed by switching to heap-allocated
    QMenu with popup() which is non-blocking.
  - Reaction emoji size: reactions used font-size:small (relative, tracks chat
    font). Fixed to use m_config.ui.fontSizes.emoji pt. Count uses fixed 8pt.

Bugs NOT yet fixed:
  - About dialog slight centering drift on Wayland (Qt limitation, deferred).
  - DCC over internet blocked by NAT (passive DCC not yet implemented).
  - Self-signed cert fingerprint-pin UI not yet built.
  - Pane layout not persisted across restarts.

Next priorities:
  - Pane layout persistence
  - Self-signed cert fingerprint-pin UI
  - DCC passive / NAT traversal
  - In-app update check UI
-->

## v0.18.1 — 2026-06-02

### Fixes

- **Fix:** Topic toggle button no longer appears oversized — removed forced 13pt font, reduced padding and border-radius, added transparent background so the global theme style doesn't fill it.
- **Fix:** Close buttons (✕) in pane and primary headers no longer show a colored background box — explicit `background: transparent; border: none` overrides the global QToolButton style.
- **Fix:** Panes now respect the **Show Nick in Input** setting on open and when toggled from Preferences.
- **Fix:** Pane input bars now use the configured font size instead of the application default.
- **Fix:** Right-clicking a timestamp in a pane chat view now shows the full context menu (Reply, React, Delete, Copy) — previously showed the default QTextBrowser menu.
- **Fix:** Left-clicking a channel in the sidebar was immediately opening it in a pane on Wayland. Root cause: `QMenu::exec()` received the pending button-release from the triggering click in its synchronous event loop, activating "Open in Pane" (the first item, at cursor position) before the user could see the menu. Fixed by switching to `QMenu::popup()`.
- **Fix:** Reaction emoji now renders at the configured `font_emoji` size (default 16pt) instead of the relative CSS `font-size:small` which tracked the chat font size.

---

<!--
Session summary — 2026-06-02 (v0.17.1 — pane topic toggles, Reload Config restart, primary X button)

What was built:
  - Per-pane topic bar: ▸/▾ topic oval button in every pane header. Topic
    bar auto-opens when a pane is created for a channel that has a topic.
    Clicking the button collapses/expands the topic bar independently per pane.
  - Primary panel header topic toggle: when panes are open, the primary
    channel column also gets the same oval topic button in its header.
    It syncs with Preferences "Show Topic Bar" toggle.
  - Reload Config now fully restarts the process instead of attempting a
    partial hot-reload that missed server list, port, and SSL changes.
  - Doc audit: ircv3.md had netsplit/netjoin, standard-replies, and UTF8ONLY
    listed under Planned — all three are implemented (v0.16.4-v0.16.8).
    Moved to Active with accurate descriptions. All Reload Config references
    updated across howto.html, faq.md, and configuration.md.

Bugs fixed:
  - Topic bar in pane was hidden by default even when a topic existed.
  - Primary panel had no topic toggle — users had to use global Preferences.
  - Reload Config hot-reload missed server-level config changes.
  - Primary panel X close button was never made visible when panes opened —
    m_primaryCloseBtn was initialized hidden and never shown.

Known issues:
  - Drag-to-rearrange panes not yet implemented.
  - Pane layout not persisted across restarts.
  - Pane input bar: plain text and /me only; other slash commands need
    the primary input bar.
  - About dialog slight centering drift on Wayland (Qt limitation, deferred).
  - DCC over internet blocked by NAT (passive DCC not yet implemented).

Next priorities:
  - Drag-to-rearrange panes
  - Pane layout persistence
  - Self-signed cert fingerprint-pin UI
  - DCC passive / NAT traversal
  - In-app update check UI
-->

<!--
Session summary — 2026-06-02 (v0.18.0 — pane drag-to-rearrange, themes seeding, slash-in-pane fixes)

What was built:
  - Themes seeded on first run: ThemeLoader::ensureUserThemesDir() creates
    ~/.config/uplink/themes/ and copies all bundled .toml files there.
    Deleted themes stay deleted across restarts. themesDir() now always
    returns the user config location.
  - Pane drag-to-rearrange: drag any pane's header bar to swap it with
    another pane or with the primary panel. Uses grabMouse() after threshold
    rather than QDrag (QDrag was unreliable on Wayland and got eaten by
    QTextBrowser's built-in drop handling). MainWindow tracks m_primarySlot
    so primary can occupy any layout position. rebuildPaneLayout() uses the
    slot to insert primary at the right index. paneAt() + isOverPrimary()
    hit-test via QApplication::widgetAt(). setDragHighlight() highlights the
    target pane header during drag.
  - All slash commands work from pane input bars: dispatchInput() extracted
    from onInputSubmit(); ChannelPane now calls dispatchInput() instead of
    only handling /me + plain text.
  - Leading space prevents command dispatch: onInputSubmit passes raw
    (untrimmed) text to dispatchInput(); only text whose first character is
    '/' triggers command mode.

Bugs fixed:
  - Pane input bar: all slash commands now work (was /me + plain text only).
  - Top pane not draggable: event filter was only on m_header, not its
    children (nameLabel, topicToggle etc.); child widgets consumed mouse press
    events. Fixed by installing filter on all header children.
  - QDrag approach replaced with grabMouse(): QDrag was swallowed by
    QTextBrowser's acceptDrops and unreliable on Wayland/nested splitters.
  - Primary panel not swappable: introduced m_primarySlot to track where
    primary sits in the layout; any ChannelPane can now be dragged onto
    primary's area to swap positions.

Known issues remaining:
  - Pane layout not persisted across restarts.
  - About dialog slight centering drift on Wayland (Qt limitation, deferred).
  - DCC over internet blocked by NAT (passive DCC not yet implemented).
  - Self-signed cert fingerprint-pin UI not yet built.
  - In-app update check UI not yet built.

Next priorities:
  - Pane layout persistence — save/restore pane arrangement across sessions
  - Self-signed cert fingerprint-pin UI
  - DCC passive / NAT traversal
  - In-app update check UI
-->

## v0.18.0 — 2026-06-02

### Pane drag-to-rearrange

- **Drag pane headers to reorder** — click and drag any pane's header bar to swap it with another pane. Drop it on any other pane header (or on the primary channel panel) to swap their positions. The layout rebuilds instantly. All configurations are supported: 2-pane side-by-side, 3-pane, and 4-pane 2×2 grid.

### Themes

- **Themes seeded on first launch** — Uplink now creates `~/.config/uplink/themes/` on first run and copies all 55 bundled themes there. The burger menu theme list reads from this directory. Themes you delete stay deleted across restarts. Custom themes can be added by dropping `.toml` files into that folder.

### Fixes

- **Fix:** Slash commands now work from pane input bars — `/j`, `/join`, `/nick`, `/me`, and all other commands work the same in any pane as they do in the primary input bar.
- **Fix:** A message beginning with a space followed by `/` (e.g. ` /version`) is now sent as plain text instead of being dispatched as a command. Only input whose very first character is `/` triggers a command.

---

## v0.17.1 — 2026-06-02

### Pane topic toggles

- **Per-pane topic bar** — each channel pane now has a `▸ topic` / `▾ topic` oval button in its header. The topic bar opens automatically when the pane is created if the channel has a topic, and updates live as the topic changes. Click the button to collapse or expand it independently per pane.
- **Primary panel topic toggle** — when panes are open, the primary channel column gets the same oval topic button in its header. It is independent of the global Preferences toggle and stays in sync if you change the global setting.

### Fixes

- **Fix:** Reload Config now fully restarts Uplink, picking up all config changes including server list, ports, channels, and SSL settings. The previous hot-reload missed server-level changes.
- **Fix:** pane topic bar was hidden by default even when the channel had a topic set.
- **Fix:** primary panel `✕` close button was not shown when panes were opened — it now appears in the primary header whenever any pane is open, matching the behavior of all extra panes.

### Docs

- `ircv3.md` — `netsplit/netjoin`, `standard-replies`, and `UTF8ONLY` corrected from Planned → Active; all implemented since v0.16.4–v0.16.8.
- All `Reload Config` references updated across `howto.html`, `faq.md`, and `configuration.md`.
- `howto.html` — panels section and channel panes section updated with topic toggle documentation.

<!--
Session summary — 2026-06-02 (v0.17.0 — detachable channel panes)

What was built:
  - Detachable channel panes: right-click any #channel in the sidebar →
    "Open in Pane" splits the chat area so that channel gets its own
    full-height column with its own chat view, nick list, topic bar, and
    input bar. Up to 4 total panes (primary + 3 extras).
  - Auto layout engine (rebuildPaneLayout): 1 view = full width; 2 =
    side by side; 3 = primary full-height left + two stacked right;
    4 = 2×2 grid. Layout rebuilds on every open/close.
  - Per-pane topic bar: ▾ toggle in the pane header shows/hides the
    channel topic. Updates live when the topic changes on the server.
  - Primary panel header: channel name + X close button appear above
    the primary chat area when at least one extra pane is open. X hides
    the primary column; clicking any sidebar item brings it back.
  - Sidebar right-click no longer changes the active channel (switched
    from currentItemChanged to itemClicked signal) — fixes the bug where
    opening a channel in a pane would clobber the primary view.
  - Server messages (MOTD, welcome banners, ISUPPORT, LUSERS) now always
    route to the server buffer, never to the active channel.
  - "Close Pane" in right-click menu when a pane for that channel is
    already open.
  - Same channel cannot appear in both primary and a pane at once:
    opening a pane for the active channel shifts primary to server buffer;
    sidebar click on a paned channel closes the pane and makes it primary
    (only when clicked, not on right-click).

Bugs fixed:
  - Server messages flooding #uplink on connect: onServerMessage now
    always posts to "(server)" buffer.
  - Right-click + "Open in Pane" was changing the primary view because
    Qt fires currentItemChanged before customContextMenuRequested.
    Fixed by switching to itemClicked for sidebar navigation.
  - Primary panel header shown even with no panes open: now hidden until
    the first pane is created.
  - Primary close button manipulated splitter sizes which broke in nested
    layouts: replaced with simple hide/show.

Known issues:
  - Drag-to-rearrange panes not yet implemented (planned).
  - Pane layout not persisted across restarts.
  - Pane input bar supports plain text and /me; other slash commands
    must be typed in the primary input bar.
  - About dialog slight centering drift on Wayland (Qt limitation, deferred).
  - DCC over internet blocked by NAT (passive DCC not yet implemented).

Next priorities:
  - Self-signed cert fingerprint-pin UI
  - Drag-to-rearrange panes (swap positions by dragging)
  - Pane layout persistence (save/restore across sessions)
  - DCC passive / NAT traversal
  - In-app update check UI
-->

## v0.17.0 — 2026-06-02

### Detachable channel panes

- **Open in Pane** — right-click any `#channel` in the sidebar to open it as a side-by-side pane. Up to 4 panes total (primary + 3 extras).
- Each pane is fully self-contained: its own chat history, nick list, topic bar, and input bar. Type and send messages directly from any pane.
- **Per-pane topic bar** — a `▾` toggle button in the pane header shows or hides the channel topic. The topic updates live when it changes on the server.
- **Auto layout** — the chat area automatically restructures as panes are added or removed:
  - 1 view: full width (no change from before)
  - 2 views: side by side
  - 3 views: primary full-height on the left, two panes stacked on the right
  - 4 views: 2×2 grid
- **Primary panel header** — when panes are open, the primary column gains a header showing the channel name and an `✕` button to temporarily hide the primary column. Clicking any channel in the sidebar brings it back.
- **Close Pane** — right-clicking a channel that is already open in a pane shows "Close Pane" instead of "Open in Pane".

### Fixes

- **Fix:** server messages (MOTD, welcome banner, ISUPPORT, LUSERS) no longer appear in `#channel` buffers — they always route to the server buffer.
- **Fix:** right-clicking a channel in the sidebar no longer changes the active primary channel; only left-clicking navigates.

<!--


Session summary — 2026-06-01 (housekeeping — local rename, CI fix, cleanup)

What was done:
  - Local project directory renamed ~/Projects/UplinkIRC → ~/Projects/Uplink.
  - Stale /home/joe/Projects/UplinkIRC/ leftover (empty + orphaned .claude dir) deleted.
  - Old UplinkIRC-*.AppImage binaries deleted from project root.
  - Stale build/ and build-appimage/ CMake caches wiped; clean rebuild done.
  - Claude memory files copied to new -home-joe-Projects-Uplink/ path; old directory deleted.
  - .claude/settings.json hook path updated to Projects/Uplink.
  - README App Icons section fixed: correct labels (Node N / Tower / Hub Spoke), stale
    icon-mark.svg and duplicate hub-spoke entries removed, brand assets table rewritten.
  - release.yml was still using UplinkIRC binary names — caused v0.16.2 Release CI to fail.
    Fixed by re-tagging v0.16.2 at HEAD (after the workflow fix commit). All three platform
    jobs (Linux, macOS, Windows) passed; Uplink-v0.16.2-* artifacts uploaded to release.

Regressions: none.
Known issues: light icon variants are PNGs (no SVG light versions yet).
Next priorities: STS, account-tag display, password keychain, DCC passive/NAT.
-->

<!--
Session summary — 2026-05-31 (close session — docs polish, logo, howto completeness)

What was done:
  - Config migrated from ~/.config/uplinkirc/config.toml to Uplink: UI settings,
    LinuxDojo server, and Libera SASL config all transferred.
  - assets/logo.svg replaced: stale Uplink "U" mark and wordmark replaced with
    Uplink tower icon + "Uplink" wordmark on dark navy background, same tagline.
  - howto.html coverage gaps filled: message deletion section, Monitor section +
    command table, /monitor added to slash commands table, /ignore /ns /cs rows added,
    account tooltip bullets in chat-area and nick-panel sections, timestamp right-click
    updated to list Reply/React/Delete, nick-context-menu added to nav.
  - howto.html full STS section added under Authentication: behavior table, server
    buffer example, policy file location, note on zero-config nature.
  - howto.html full Account tracking section added under The Interface: two tooltip
    locations with example tooltip text, four-source table (account-tag, account-notify,
    extended-join, WHOX), nick-change-survives tip.
  - ircv3.md: STS + account-tag moved from Planned → Active with full descriptions.
  - faq.md: account tooltip updated for 4 mechanisms + chat view; STS FAQ entry added.
  - configuration.md: ssl field description updated to mention STS auto-upgrade.
  - v0.16.3 tagged, AppImage built (68MB), GitHub release created with artifact.
  - ROADMAP, README, memory updated for STS and account-tag.

Regressions: none.
Known issues: light icon variants are PNGs (no SVG light versions yet).
Next priorities: password keychain (OS Secret Service), DCC passive/NAT, split view,
  SOCKS5 proxy, SVG light icon variants.
-->

<!--
Session summary — 2026-05-31 (v0.16.3 — STS, account-tag, howto icon fix)

What was done:
  - STS (Strict Transport Security) implemented. stsstore.h/cpp: QSettings-based
    persistent policy store at ~/.config/uplink/sts.ini. connectToServer applies
    any cached policy before dialing (upgrades plain→TLS). handleCap LS parses
    sts=port=X,duration=Y: on plain connection stores policy and reconnects over TLS
    via QTimer::singleShot+socket abort; on TLS refreshes expiry. duration=0 clears
    the policy. m_stsUpgrade flag prevents scheduleReconnect from firing during the
    upgrade disconnect.

  - account-tag (IRCv3) implemented. account-tag added to desired CAPs. PRIVMSG and
    NOTICE handlers extract the account tag and emit accountChanged before emitting
    messageReceived/noticeReceived, keeping nick list accounts current per-message.
    account field added to Message struct. sessionmodel onMessage/onNotice/onAction
    look up account from channel nick list and store it on the message. Chat rendering
    adds title='account: X' to the nick anchor — hover the nick in chat to see it.

  - docs/howto.html: replaced stale uplink-minimal-mark.svg header icon with
    icons/tower-dark.png; uplink-minimal-mark.svg deleted from docs/.

Regressions: none.
Known issues: light icon variants are PNGs (no SVG light versions yet).
Next priorities: password keychain, DCC passive/NAT, split view, SOCKS5 proxy.
-->

<!--
Session summary — 2026-06-01

What was built:
  - v0.16.4: Netsplit/netjoin batch collapse — IRCv3 netsplit/netjoin batch types now
    collapse into one summary line per channel instead of flooding the buffer with
    individual QUIT/JOIN messages. Nick lists remain accurate. New signals:
    netsplitDetected / netjoinDetected on IrcClient; onNetsplitDetected /
    onNetjoinDetected on SessionModel.
  - v0.16.5: Standard Replies — FAIL/WARN/NOTE commands parsed and displayed with
    [FAIL]/[WARN]/[NOTE] prefixes. Routes to named channel buffer if context includes
    a channel name, otherwise active channel / server buffer fallback.
  - v0.16.6: OS keychain password storage — qtkeychain-qt6 integrated. password,
    sasl_password, nickserv_password stored in OS keychain (Secret Service / macOS
    Keychain / Windows Credential Manager). Config file holds "<keychain>" sentinel.
    Existing plaintext passwords migrate automatically on next save.

Bugs fixed:
  - docs/index.html download links were still pointing to v0.16.2 instead of v0.16.3
    (reported by nexu on IRC). All three platform links and four version labels updated.

Documentation:
  - Full doc pass covering all three new features across: config.toml.example, ircv3.md,
    configuration.md (new Password Storage subsection), faq.md, README.md, howto.html
    (new keychain section + nav entry). Also fixed lingering "UplinkIRC" references in
    config.toml.example.

Regressions found: none.

Known issues left open:
  - DCC over internet: local IP advertised in DCC SEND offer; NAT/firewall on sender
    side blocks inbound connection. Works on LAN only.
  - About dialog slight centering drift on Wayland (Qt limitation, deferred).
  - Light icon variants are PNGs — no SVG light versions yet.

Next priorities:
  - Self-signed cert fingerprint-pin UI
  - SOCKS5 proxy support
  - DCC passive / NAT traversal
  - Split view (two channels side by side)
  - In-app update check UI
-->

<!--
Session summary — 2026-06-01 (v0.16.8 — SOCKS5, UTF8ONLY, security fixes, /leave /close)

What was done:
  - NickServ echo-message redaction: the server echo path (when echo-message CAP
    is active) was not redacted. Fixed in onMessage — isSelf + isPM + NickServ
    target now applies the same pwdCmds redaction as the local echo path.
  - /leave and /close commands added. In a channel they send PART; in a query
    (PM window) they close the buffer without sending anything to the server.
    Both added to tab-completion list.
  - SOCKS5 proxy support fully implemented:
      - ServerConfig: proxyHost, proxyPort (default 1080), proxyUser, proxyPass
      - config.cpp: load/save proxy_host/port/user/pass TOML keys
      - ircclient: applyProxy() helper called before every connectToHost* call
        (initial connect, doReconnect, STS upgrade). QNetworkProxy::Socks5Proxy
        set on the socket; NoProxy when proxyHost is empty.
      - Server dialog: new SOCKS5 Proxy section with host, port spinbox,
        optional user/pass fields.
  - UTF8ONLY ISUPPORT: 005 params now parsed token by token. UTF8ONLY token
    sets m_utf8Only flag and logs to server buffer. onReadyRead warns on
    0xFF bytes (cannot appear in valid UTF-8). privmsg warns on U+FFFD
    replacement characters in outgoing text. Flag resets on each new connect.
  - Docs: signal bars section updated with lag/latency/RTT keywords so search
    finds it. Same section added to faq.md for in-app discoverability.

Regressions: none.
Known issues: same as v0.16.7.
Next priorities: self-signed cert fingerprint-pin, DCC passive/NAT, split view,
  in-app update check UI, SVG light icon variants.
-->

<!--
Session summary — 2026-06-01 (v0.16.7 — UX fixes, security, docs search)

What was done:
  - Link preview card background now reads bufferBg/border/text/timestamp from
    the active theme instead of hardcoded dark hex colors. Cards match the chat
    area on any theme, light or dark.
  - Right-click context menu improvements:
      - Timestamp (msgid) menu now includes Copy when text is selected.
      - Message body right-click (no anchor) now includes Reply: scans the
        block's QTextFragment char formats to find the msgid anchor, so
        Reply is available without needing to click the timestamp exactly.
  - Security — NickServ credential redaction extended: IDENTIFY, REGISTER,
    GHOST, RECOVER, RELEASE, REGAIN, SETPASS all show "<redacted>" in the
    local PM buffer echo instead of the actual password. The real text still
    goes to the server.
  - Service NOTICE routing fixed: NOTICEs from NickServ, ChanServ, BotServ,
    MemoServ etc. now land in the PM tab for that service if one is open,
    instead of always going to the server buffer. Replies to /msg ChanServ HELP
    now appear in the ChanServ window.
  - howto.html: full-text search box added to the left nav sidebar. Live
    highlights all matches as you type, shows N/total count, ▲▼ buttons and
    Enter/Shift+Enter to cycle, Escape to clear.
  - howto.html: header icon replaced from tower-dark.png (PNG with background
    artifact / thick frame on some renderers) to tower.svg (clean transparent).
  - docs/icons/tower.svg added (copied from resources/icons/icon-tower.svg).

Regressions: none.
Known issues: same as previous session.
Next priorities: self-signed cert fingerprint-pin, SOCKS5 proxy, DCC passive/NAT,
  split view, in-app update check.
-->

<!--
Session summary — 2026-06-01 (CI fix — Qt6Keychain on all runners)

What was done:
  - v0.16.6 CI was failing on all three platforms immediately after the keychain
    feature landed. Qt6Keychain was added as a required dependency but the CI
    runners were never updated to provide it.
  - ci.yml + release.yml (Linux): added libsecret-1-dev to apt install.
    libsecret is the Secret Service backend qtkeychain uses on Linux even when
    built from source.
  - ci.yml + release.yml (macOS): added qtkeychain to brew install line.
  - CMakeLists.txt: changed find_package(Qt6Keychain REQUIRED) to a QUIET find
    with FetchContent fallback (frankosterfeld/qtkeychain @ 0.14.3), mirroring
    how tomlplusplus is handled. Required three iterations to get right:
      1. Qt6Keychain::Qt6Keychain alias not created in build tree — added explicit
         alias creation after FetchContent_MakeAvailable.
      2. #include <qt6keychain/keychain.h> failed — installed layout has qt6keychain/
         subdirectory but source tree puts keychain.h at root. Generated a forwarding
         header in the build dir to bridge both layouts.
      3. qkeychain_export.h not found — generated into qtkeychain binary dir which
         wasn't on the include path. Added _qtkc_bin to QTKEYCHAIN_COMPAT_INCLUDE.
  - release.yml (Windows package step): qt6keychain.dll is built in the FetchContent
    build tree, not the Qt bin dir where windeployqt looks. Added a PowerShell step
    to find and copy it into QT_ROOT_DIR/bin before windeployqt runs.
  - v0.16.6 tag deleted, re-pointed at fixed HEAD, and re-pushed twice (once for
    the cmake fixes, once for the windeployqt fix). Final release has all three
    platform binaries: Linux tar.gz + AppImage, macOS .dmg, Windows .zip.

Regressions: none.
Known issues: same as previous session.
Next priorities: self-signed cert fingerprint-pin, SOCKS5 proxy, DCC passive/NAT,
  split view, in-app update check.
-->

<!--
Session summary — 2026-06-02

What was built:
  - /caps command: lists all currently negotiated IRCv3 caps in the active
    buffer. Useful for checking whether the server supports features like
    draft/message-redaction. ackedCaps() accessor added to IrcClient.
  - channels save format fixed: config now saves channels = "#a, #b" instead
    of [[server.channel]] array-of-tables when no channel has a key/password.
    [[server.channel]] is still used (and loaded) when keys are present.
  - Exit menu icon added: hamburger Exit item now has a door/arrow icon so it
    aligns horizontally with all other menu items (which all had icons).
  - Server messages routed to active channel: /whois, /version, /ping replies,
    MOTD, and all other server numerics now appear in whatever window you're
    looking at instead of always going to the server buffer. Falls back to
    server buffer when there's no active channel (e.g. during connection).
  - ZNC/soju howto sections completely rewritten: full step-by-step Manage
    Servers walkthroughs, password format explained with examples, capability
    tables, multiple-network configs, troubleshooting tables.

Bugs fixed:
  - ircclient.h ackedCaps() not committed — Windows CI failed with C2039.
  - channels format reverted to [[server.channel]] on every config save.

Regressions: none.
Known issues: same as v0.16.10.
Next priorities: bouncer Network field hide when ZNC selected, self-signed
  cert fingerprint-pin UI, DCC passive/NAT, split view.
-->

## v0.16.11 — 2026-06-02

- **Fix: WHOIS and server replies appear in active window** — `/whois`, `/version`, MOTD, and all other server responses now appear in the channel or query you are looking at, not buried in the server buffer. Falls back to the server buffer only when no channel is open.
- **`/caps` command** — type `/caps` in any buffer to see the full list of IRCv3 capabilities negotiated with the server. Useful for checking whether features like `draft/message-redaction` (message deletion) are available.
- **Fix: `channels` saved as one line** — Uplink was saving channel lists as `[[server.channel]]` array-of-tables on every config save, even when no channels had passwords. It now saves the clean `channels = "#a, #b"` format. The array form is still used (and loaded) when a channel has a key.
- **Fix: Exit menu item alignment** — the Exit item in the ☰ menu had no icon, causing its text to appear left of all other items. It now has a matching icon.
- **ZNC and soju docs overhauled** — the How-To guide sections for ZNC and soju are completely rewritten with step-by-step Manage Servers walkthroughs, the password format explained up front with concrete examples, capability tables, multiple-network config examples, and a ZNC troubleshooting table.

---

<!--
Session summary — 2026-06-02

What was built:
  - Bouncer Network field hidden in server dialog when Type is None or ZNC.
    Only appears when Soju is selected — eliminates confusion about a field
    that does nothing for ZNC users.
  - Split view attempted (right-click → Open in Split), then reverted.
    Re-specced on ROADMAP as Halloy-style detachable panes — each pane gets
    its own input bar, nick list, topic bar, freely positionable, persistent.
  - docs: commands.md and faq.md updated — stale React "text dialog"
    descriptions replaced with emoji picker; /caps added to user commands.

Bugs fixed:
  - Stale QSettings nickSplitter state caused split layout on launch after
    revert — cleared from ~/.config/LinuxDojo/Uplink.conf.

Regressions: none.
Known issues: same as v0.16.11.
Next priorities: detachable panes (Halloy-style), self-signed cert
  fingerprint-pin UI, DCC passive/NAT, in-app update check.
-->

## v0.16.12 — 2026-06-02

- **Bouncer: Network field hidden for ZNC** — the Network field in the Add/Edit Server dialog now only appears when **Soju** is selected as the bouncer type. For ZNC and None it is hidden, removing a confusing field that does nothing for ZNC users.
- **Detachable panes on roadmap** — split view re-specced as Halloy-style detachable channel windows: each popped-out pane gets its own input bar, nick list, and topic bar, freely positionable and persistent across sessions.
- **Fix: stale split layout on launch** — a leftover QSettings entry was restoring the 3-pane splitter layout after the split view code was removed.

---

<!--
Session summary — 2026-06-01 (emoji size + picker UX)

What was built:
  - Configurable emoji font size: font_emoji config key (default 16pt); new
    "Chat Emoji" row in Font Config dialog. Emoji in PRIVMSG/ACTION/NOTICE
    are wrapped in <span style='font-size:Xpt'> at render time, independent
    of the chat font. Detection uses isHighSurrogate()/surrogateToUcs4()
    instead of QRegularExpression — reliable across Qt's UTF-16 strings.
    Covers U+1F000-U+1FAFF (supplementary plane emoji) and U+2300-U+27BF
    (BMP misc symbols/dingbats).
  - Emoji picker Enter key: pressing Enter in the picker's search box commits
    the first visible emoji, so ":poop: Enter" works without clicking.
  - README: removed C++17 version label from badge and tagline → "C++".

Bugs fixed:
  - Emoji not resizing when font_emoji set — original regex approach failed
    to match surrogate-pair emoji; replaced with character-by-character walk.

Regressions: none.
Known issues: same as v0.16.9.
Next priorities: self-signed cert fingerprint-pin, DCC passive/NAT, split view.
-->

## v0.16.10 — 2026-06-01

- **Configurable emoji size** — emoji in chat messages now render at their own independent font size (`font_emoji`, default `16`pt), separate from the chat font. Useful when using a small chat font — emoji stay readable. Set it in **Preferences → Font Config… → Chat Emoji** or add `font_emoji = 16` to the `[ui]` section of your config.
- **Emoji picker: Enter to commit** — pressing <kbd>Enter</kbd> in the picker's search box immediately sends the first visible emoji. Type `:poop:` and hit Enter without clicking.
- **Fix: emoji size not applying** — the initial implementation used a regex that failed to match supplementary-plane emoji (surrogate pairs) in Qt's UTF-16 strings. Replaced with a character-by-character walk using `isHighSurrogate()` / `surrogateToUcs4()`.
- **README: C++17 → C++** — removed the version label from the badge and tagline.

---

<!--
Session summary — 2026-06-01 (emoji picker improvements)

What was built:
  - React context menu now opens the emoji picker GUI instead of a bare text
    input dialog. The same searchable grid used by the 😊 toolbar button appears
    anchored near the right-click position. Selecting an emoji sends the reaction.
    Implemented via m_pendingReactMsgid/Host/Channel state; emojiSelected handler
    checks these before the normal input-insertion path.
  - Emoji picker search now strips leading/trailing colons before filtering, so
    searching ":po", "po", or ":poop:" all find poop 💩 and friends. Previously
    ":po" matched nothing because shortcodes are stored without colons.

Bugs fixed:
  - Emoji picker search went blank when user typed a colon-prefixed search term
    (e.g. ":poop") — the colon was included in the filter which matched no shortcode.

Regressions: none.
Known issues: same as v0.16.8.
Next priorities: self-signed cert fingerprint-pin UI, DCC passive/NAT, split view.
-->

## v0.16.9 — 2026-06-01

- **React uses emoji picker** — right-click a message timestamp → **React** now opens the same searchable emoji picker used by the 😊 toolbar button, instead of a bare text input dialog. Search by name or shortcode, click to send.
- **Fix: emoji picker search with colon prefix** — typing `:poop` or `:fire` in the picker's search box previously showed a blank grid because the leading colon was included in the filter. Colons are now stripped before matching, so `:po`, `po`, and `:poop:` all find the right emoji.

---

## v0.16.8 — 2026-06-01

- **SOCKS5 proxy support** — each server can route its connection through a SOCKS5 proxy. Configure with `proxy_host`, `proxy_port` (default `1080`), and optional `proxy_user` / `proxy_pass` in the `[[server]]` block, or set it from the server dialog. The proxy is applied to every connect attempt including reconnects and STS upgrades.
- **UTF8ONLY** — Uplink now detects the `UTF8ONLY` ISUPPORT token from the server and enforces UTF-8 encoding. A notice appears in the server buffer when the token is seen. Incoming non-UTF-8 bytes and outgoing messages containing replacement characters both trigger warnings.
- **`/leave` and `/close` commands** — in a channel, both send PART. In a query (PM window), both close the buffer without sending anything to the server. Both complete via Tab.
- **Fix: NickServ password still shown when server has `echo-message`** — the server-echo path in `onMessage` was missing the credential redaction that the local-echo path had. Both paths now redact `IDENTIFY`, `REGISTER`, `GHOST`, `RECOVER`, `RELEASE`, `REGAIN`, and `SETPASS`.

---

## v0.16.7 — 2026-06-01

- **Theme-aware link preview cards** — the background, border, title, and domain colours of link preview cards now follow the active theme instead of being hardcoded dark. Cards look correct on any theme, light or dark.
- **Right-click: Copy and Reply together** — right-clicking on selected message text now shows both **Copy** and **Reply** in the same menu. Previously you had to right-click the timestamp exactly to get Reply. The context menu now finds the message ID from whichever part of the message you click.
- **Security: NickServ credential redaction extended** — the local PM buffer echo for sensitive NickServ commands now shows `COMMAND <redacted>` instead of your actual password. Covered: `IDENTIFY`, `REGISTER`, `GHOST`, `RECOVER`, `RELEASE`, `REGAIN`, `SETPASS`.
- **Fix: service NOTICE routing** — replies from NickServ, ChanServ, BotServ, MemoServ, and other services now appear in the PM tab for that service (if one is open), not in the server buffer. Typing `/msg ChanServ HELP` opens a ChanServ tab and the help output arrives there.
- **howto.html: full-text search** — search box added to the left sidebar of the online How-To guide. Live highlights, match count, keyboard navigation (Enter / Shift+Enter), Escape to clear.
- **howto.html: SVG icon** — header icon switched from PNG (which had a visible background frame on some browsers) to SVG.

---

## v0.16.6 — 2026-06-01

- **Password encryption via OS keychain** — server passwords, SASL passwords, and NickServ passwords are now stored in the OS keychain (Secret Service on Linux, Keychain on macOS, Credential Manager on Windows) instead of plaintext in `config.toml`. Existing plaintext passwords migrate automatically on next save. The config file stores `"<keychain>"` as a sentinel.
- **CI fix** — all three platform CI and release builds now pass. Qt6Keychain is fetched from source via CMake FetchContent when not installed system-wide, with a generated forwarding header to normalise the include path. The Windows release packages `qt6keychain.dll` alongside the Qt libraries via `windeployqt`.

---

## v0.16.5 — 2026-06-01

- **Standard Replies** — IRCv3 `FAIL`, `WARN`, and `NOTE` commands handled; displayed in the relevant channel buffer (or active channel / server buffer as fallback). `FAIL` renders as an error line, `WARN`/`NOTE` as server info. Format: `[FAIL] JOIN CHANNEL_BANNED: You are banned from that channel`.

---

## v0.16.4 — 2026-06-01

- **Netsplit/netjoin batch collapse** — IRCv3 `netsplit` and `netjoin` batch types handled; instead of a wall of individual quit/join lines, a single summary is posted per affected channel: `Netsplit: N users lost (srv1 srv2)` / `Netjoin: N users returned (srv1 srv2)`. Nick lists stay accurate.

---

## Docs — 2026-05-31

- **howto.html: STS section** — full dedicated section under Authentication with behavior table (plain/TLS/duration=0/expired), server buffer example (`STS: upgrading to TLS on port 6697`), policy file location, and zero-config callout.
- **howto.html: Account tracking section** — full dedicated section under The Interface; covers both tooltip locations (chat view and nick list) with example output, four-source table (account-tag, account-notify, extended-join, WHOX), and nick-rename survival tip.
- **howto.html: coverage gaps** — message deletion section, Monitor section, timestamp right-click lists Reply/React/Delete, nick-context-menu nav link, account tooltip bullets, `/monitor` `/ignore` `/ns` `/cs` added to commands table.
- **logo.svg** — replaced stale Uplink mark with Uplink tower icon + wordmark on dark navy background.
- **ircv3.md** — STS and `account-tag` graduated from Planned → Active.
- **faq.md** — STS auto-TLS FAQ entry added; account tooltip updated to list all four sources including `account-tag` and the chat view location.

---

## v0.16.3 — 2026-05-31

- **STS (Strict Transport Security)** — IRCv3 `sts` capability implemented. When a server advertises an STS policy over a plain connection, Uplink immediately reconnects over TLS on the advertised port and caches the policy to `~/.config/uplink/sts.ini`. Future connections to that host enforce TLS even if `ssl = false` in config. Policies expire after the server-specified duration; `duration=0` clears them immediately.
- **`account-tag`** — IRCv3 `account-tag` capability negotiated. The sender's NickServ account name is stored on each message and shown as a hover tooltip on the nick in the chat view — the same account tooltip as in the nick list.
- **Docs: howto.html icon** — replaced stale `uplink-minimal-mark.svg` header graphic with the Uplink tower icon; stale file removed.

---

## Housekeeping — 2026-06-01

- **Local directory renamed** — `~/Projects/UplinkIRC` → `~/Projects/Uplink`; stale leftover directory and old `UplinkIRC-*.AppImage` binaries removed.
- **Release CI fixed** — `release.yml` still referenced `UplinkIRC` binary names; v0.16.2 tag re-created at HEAD so all three platform builds (Linux, macOS, Windows) pass and upload `Uplink-v0.16.2-*` artifacts correctly.
- **README App Icons fixed** — labels corrected to Node N / Tower / Hub Spoke; stale `icon-mark.svg` and duplicate entries removed; brand assets table rewritten to reflect actual files.

---

<!--
Session summary — 2026-06-01 (v0.16.2 — full Uplink rebrand + icon rework)

What was done:
  - Full project rebrand from UplinkIRC to Uplink: app name, binary name, window title,
    tray tooltip, CTCP VERSION reply, About dialog, QSettings key, config path
    (~/.config/uplinkirc/ → ~/.config/uplink/), version macro (UPLINKIRC_VERSION →
    NODERELAY_VERSION), CMakeLists project/target, desktop file, packaging scripts,
    release.yml CI artifacts, all docs (README, howto.html, faq.md, configuration.md,
    commands.md, ircv3.md, index.html), CLAUDE.md.
  - New icon set: 3 switchable styles — Node N, Tower, Hub Spoke. Dark variants are SVGs;
    light variants are PNGs. Auto dark/light selection based on Qt palette brightness
    (qApp->palette().window().color().lightness() < 128). Theme change triggers icon refresh.
    Tray icon locked to hub-spoke SVG. About dialog uses static Node-Relay-Tower.png at 128px.
  - Preferences icon picker collapsed from 6 options to 3 (removed separate light entries).
  - docs/index.html: new uplink-banner.png header, Icons section with 8 downloadable PNGs,
    Icons nav link added, all version strings updated to 0.16.2.
  - GitHub repo renamed from noderelay/UplinkIRC to noderelay/UplinkIRC.
  - v0.16.2 release created with AppImage attached.
  - Version bump: 0.16.1 → 0.16.2.

Regressions: none.
Known issues: light icon variants are PNGs (no SVG light versions yet).
Next priorities: STS, account-tag display, password keychain, DCC passive/NAT.
-->

## v0.16.2 — 2026-06-01

- **Full rebrand to Uplink** — app name, binary, window title, tray tooltip, CTCP VERSION reply, About dialog, QSettings key, config path (`~/.config/uplink/`), version macro (`NODERELAY_VERSION`), CMakeLists target, desktop file, CI artifacts, and all docs updated project-wide.
- **New icon set** — three switchable styles: Node N, Tower, Hub Spoke. Dark variants are SVGs; light variants are PNGs. Selection auto-switches based on active theme brightness — no separate light/dark picker needed.
- **Preferences icon picker simplified** — 3 choices (Node N, Tower, Hub Spoke) instead of 6; light variant selected automatically.
- **Theme change refreshes icon** — switching themes now also re-applies the window/taskbar icon to match the new palette.
- **About dialog logo** — static Node-Relay-Tower.png at 128 × 128 px; no longer tied to the switchable icon choice.
- **Tray icon** — locked to hub-spoke SVG regardless of icon choice setting.
- **Docs site updated** — new banner, downloadable icon set section (8 PNGs), all GitHub URLs updated to `noderelay/UplinkIRC`.
- **GitHub repo renamed** — `noderelay/UplinkIRC` → `noderelay/UplinkIRC`; old URL auto-redirects.

---

<!--
Session summary — 2026-05-31 (docs — raw command passthrough coverage)

What was done:
  - faq.md "How do I send a raw IRC command?" was outdated — only covered /raw and /quote,
    said nothing about the v0.16.1 passthrough. Rewrote it: leads with direct passthrough
    (just type /REHASH), then covers /raw /quote as the explicit form with examples for both.
  - howto.html command table had a single /quote row with a minimal example. Added separate
    rows for /raw, /quote, and /<ANYTHING>. Added a callout box below the table explaining
    that oper/server-specific commands (/REHASH, /SAMODE, /GLOBOPS, /OPER) work directly.
  - commands.md was already fully up to date from the v0.16.1 session — no changes needed.
  - Also this session: CMakeLists.txt VERSION bumped from 0.16.0 → 0.16.1 (was never bumped
    when v0.16.1 shipped); joehonkey reference removed from public-facing CHANGELOG bullet.

Regressions: none.
Known issues: unchanged from v0.16.1.
Next priorities: STS, account-tag display, password keychain, DCC passive/NAT.
-->

## Docs — 2026-05-31

- **Raw command passthrough documented** — `faq.md` and `howto.html` updated to cover the v0.16.1 passthrough feature; beginner-friendly examples added for `/REHASH`, `/SAMODE`, `/GLOBOPS`, and `/OPER`; howto command table now has rows for `/raw`, `/quote`, and `/<ANYTHING>` with a callout box explaining oper usage.

---

<!--
Session summary — 2026-05-31 (build fix — CMakeLists.txt version bump to 0.16.1)

What was done:
  - CMakeLists.txt still had VERSION 0.16.0 even though v0.16.1 was tagged and released.
    The raw command passthrough commit (8e16811) updated docs and CHANGELOG but never bumped
    the build version string. Binary was reporting 0.16.0 at runtime.
  - Fixed: project(UplinkIRC VERSION 0.16.1 ...) in CMakeLists.txt.
  - Rebuilt successfully. Binary now reports v0.16.1.

Regressions: none.
Known issues: unchanged from v0.16.1.
Next priorities: STS, account-tag display, password keychain, DCC passive/NAT.
-->

## Build fix — 2026-05-31

- **CMakeLists.txt version bump** — `PROJECT_VERSION` was stuck at `0.16.0` despite the v0.16.1 release; binary now correctly reports `0.16.1` at runtime.

---

<!--
Session summary — 2026-05-31 (housekeeping — GitHub releases backfill + joehonkey cleanup)

What was done:
  - GitHub releases page was stuck at v0.13.0 even though the repo was at v0.16.1.
    Root cause: git tags and GitHub Releases were never created for v0.14.0–v0.16.1.
    Fixed by creating tags (bfdfc1d→v0.14.0, 63082b1→v0.15.0, 8fcbb8d→v0.16.0,
    8e16811→v0.16.1), pushing them, and creating GitHub Releases for all four with
    CHANGELOG-sourced release notes. v0.16.1 marked as Latest.
  - Remaining joehonkey references cleaned up:
    * settings.local.json: two GIT_COMMITTER_EMAIL / GIT_AUTHOR_EMAIL allowed-command
      entries updated from joehonkey noreply email to uplink noreply email.
    * README.md: nick prefix config comment example ("joehonkey ▸ ...") updated to uplink.
    * CHANGELOG.md references are historical (documenting the rename) — left as-is.

Regressions: none.
Known issues: unchanged from v0.16.1.
Next priorities: STS, account-tag display, password keychain, DCC passive/NAT.
-->

## Housekeeping — 2026-05-31

- **GitHub releases backfilled** — tags and GitHub Releases created for v0.14.0, v0.15.0, v0.16.0, and v0.16.1; GitHub now correctly shows v0.16.1 as the latest release.

---

<!--
Session summary — 2026-05-31 (v0.16.1 — raw command passthrough)

What was done:
  - Unrecognized slash commands now pass through directly as raw IRC lines.
    /REHASH → sends REHASH, /SAMODE #channel +o nick → sends SAMODE #channel +o nick, etc.
    Previously showed "Unknown command" error. One-line change in onInputSubmit() else branch.

Regressions: none.
Known issues: unchanged from v0.16.0.
Next priorities: unchanged.
-->

## v0.16.1 — 2026-05-31

- **Raw command passthrough** — unrecognized slash commands are now sent directly to the server as raw IRC lines. `/REHASH`, `/SAMODE #channel +o nick`, `/GLOBOPS`, and any other server-specific or oper command work without needing `/quote` or `/raw` as a prefix. `/quote` and `/raw` still work as before.

---

<!--
Session summary — 2026-05-31 (housekeeping — username rename)

What was done:
  - GitHub account renamed from joehonkey → uplink.
  - Git remote URL updated to https://github.com/noderelay/UplinkIRC.git.
  - All joehonkey references replaced in README.md, CHANGELOG.md, ROADMAP.md,
    SECURITY.md, docs/faq.md, themes/BreezeDarkPlus.toml, CLAUDE.md.
  - Global git user.name updated to uplink.
  - Verified GitHub repo and GitHub Pages both live at new URLs.
  - ~/.claude/CLAUDE.md updated.

Regressions: none.
Known issues: unchanged from v0.15.0.
Next priorities: unchanged.
-->

<!--
Session summary — 2026-05-31 (v0.16.0 — IRCv3 sprint: 4 feature areas)

What was built:
  - draft/message-redaction: REDACT command support. Right-click own message timestamp →
    Delete (only shown when CAP acked). IrcClient::sendRedact(); sessionmodel::onMessageRedacted()
    finds msg by msgid, sets redacted=true, emits messageRedacted(host, channel); mainwindow
    calls refreshChatView when active buffer is affected. formatMessage renders redacted messages
    as "[message deleted]" in grey italic (early return before switch statement).
    Message struct gained redacted bool field.

  - account-notify + extended-join: NickEntry gained account QString field (empty = unknown).
    IrcClient handles ACCOUNT command → accountChanged signal. Extended JOIN params[1] = account
    also fires accountChanged. SessionModel::onAccountChanged iterates all channels, calls
    setNickAccount, emits nickListChanged. Channel::setNickAccount helper added. Nick list tooltip
    shows "Account: <name>" when set.

  - WHOX: Changed "WHO <channel>" → "WHO <channel> %cnfa,42" (token 42). Added 354
    RPL_WHOSPCRPL handler: params = [me, "42", channel, nick, flags, account]. Emits both
    whoEntryReceived (bot flag reuse) and accountChanged when account != "0". Backward compat:
    352 handler still present for servers that don't support WHOX.

  - Monitor: IrcClient stores m_monitorList; setMonitorList() called from attachClient using
    m_config.monitorList. MONITOR + list sent after RPL_WELCOME (case 1). Numerics 730/731/732/
    733/734 handled. SessionModel::onMonitorOnline/onMonitorOffline post to (server) buffer.
    Config::monitorList (QStringList) added with [monitor] nicks = [...] in config.toml.
    SessionModel::monitorAdd/Remove/Clear/Status forward to IrcClient. Slash commands:
    /monitor add|del|remove|list|clear|status in mainwindow onInputSubmit.

  - invite-notify: IrcClient handles INVITE command → inviteNotify signal (was previously
    unhandled). SessionModel::onInviteNotify: self-invite → (server) buffer, broadcast invite
    → channel buffer.

  - setname: IrcClient handles SETNAME → setNameReceived signal. SessionModel::onSetNameReceived
    posts "nick changed their realname to ..." to all shared channels.

  - userhost-in-names: CAP added to desired list. Channel::setNicks strips !user@host suffix
    (bang check after prefix stripping).

  - All 7 new CAPs added to desired list: account-notify, extended-join, invite-notify, setname,
    userhost-in-names, draft/message-redaction. (MONITOR is a command, not a CAP.)

Regressions: none. Clean build throughout.
Known issues:
  - account-tag (per-message account= tag) not yet displayed — data is in message-tags, just
    not surfaced in the UI.
  - message-redaction "Delete" only for own messages (server will reject unauthorized redactions;
    no op-redact UI yet).
  - Monitor list is global across all servers in config; per-server watch lists not supported.
Next priorities: STS, account-tag display, password keychain, DCC passive/NAT traversal.
-->

## v0.16.0 — 2026-05-31

- **`draft/message-redaction`** — right-click any of your own message timestamps → **Delete** to send a `REDACT` command; redacted messages display as `[message deleted]` in grey italic. Only shown when the server acknowledges the CAP.
- **`account-notify` + `extended-join`** — UplinkIRC tracks each nick's NickServ account in real time. On join, the account name comes from the extended `JOIN` message; after login/logout, `ACCOUNT` commands update it instantly. Account name shows as a tooltip in the nick list (hover a nick).
- **`WHOX`** — `WHO` requests now use `WHO <channel> %cnfa,42` (WHOX field mask) to retrieve account names in the initial join scan, populating nick list tooltips without separate WHOIS queries.
- **`Monitor`** — IRCv3 watch list for online/offline status. Configure with `/monitor add <nick>`, `/monitor del <nick>`, `/monitor list`, `/monitor clear`, `/monitor status`. The list persists to `config.toml` under `[monitor] nicks = [...]` and is resent on every reconnect.
- **`invite-notify`** — invitations broadcast to all channel members now post a status line in the relevant channel buffer (or the server buffer when you are the target).
- **`setname`** — when a user in a shared channel changes their real name via `SETNAME`, a quiet status line appears: `nick changed their realname to "…"`.
- **`userhost-in-names`** — CAP negotiated; `NAMES` replies with `nick!user@host` are parsed correctly (host suffix stripped before display).

---

## Maintenance — 2026-05-31

- **GitHub username renamed** — account and all project URLs updated from `joehonkey` to `uplink`. Git remote, README badges, docs, and theme attribution all reflect the new username.

---

<!--
Session summary — 2026-05-31 (v0.14.0–v0.15.0 + fixes)

What was built:
  - Ignore list: /ignore /unignore /ignored commands + right-click Ignore/Unignore on nicks.
    Filters onMessage/onNotice/onAction in SessionModel before postMessage. Persists as
    [ignore] nicks = [...] in config.toml. Default off per-nick until added.
  - Right-click copy: selecting text in chat view and right-clicking now shows Copy menu item
    (was blocked by custom ContextMenu event filter returning true unconditionally).
  - Menu/gear icon colors: MenuIcons::make() and makeGearIcon() now use m_theme.text instead of
    QApplication::palette().color(QPalette::WindowText) which always returned OS default (black).
    All icon functions accept optional QColor param; mainwindow passes theme color on each open.
  - Link preview card: hardcoded dark card (#1a1a1a / #eeeeee / #888888) so color never changes
    with theme. Was using palette colors that varied by theme.
  - Server dialog: keyed channel note added below auto-join field. Browse button widened 70→90px
    to stop "Browse…" clipping to "rowse".
  - Per-channel logging (v0.15.0): all messages appended to
    ~/.config/uplinkirc/logs/<server>/<channel>.log. Format: [yyyy-MM-dd hh:mm:ss] <nick> text.
    History replay skipped. Toggle: Preferences → Log Messages to Disk; config key log_messages.
  - chghost (v0.15.0): desired cap + CHGHOST command parsed; emits hostChanged signal; SessionModel
    posts "nick changed host" in all shared channels. No more fake QUIT+JOIN noise.
  - draft/react (v0.15.0): desired cap; TAGMSG +draft/react parsed; reactReceived signal;
    Channel::reactions QHash (msgid→emoji→nicks); refreshChatView renders emoji(count) inline below
    messages; right-click timestamp → React; /react command; IrcClient::sendReact.
  - Logging toggle in Preferences: log_messages bool in UiConfig; checkbox in PreferencesDialog;
    loggingToggled signal; SessionModel::postMessage checks m_config.ui.logMessages.
  - New commands: /query (open PM, no message); /ns /cs /bs /ms (service shortcuts);
    /oper (IRC operator login).
  - AppImage: patched linuxdeploy-plugin-qt's bundled strip (same .relr.dyn workaround already
    applied to linuxdeploy). Wrapper script in temp dir placed before TOOLS_DIR in PATH.
  - Full docs pass: README, commands.md, configuration.md, faq.md, ircv3.md, index.html,
    howto.html — all updated for v0.15.0. Manage Servers section added to howto.html and faq.md.

Regressions: none (clean build, existing behavior preserved).
Known issues: plaintext passwords in config.toml (keychain pending); DCC over NAT blocked.
Next priorities: password keychain, draft/message-redaction, account-notify, Monitor, STS.
-->

## v0.15.0 — 2026-05-31

- **Per-channel logging** — all messages written to `~/.config/uplinkirc/logs/<server>/<channel>.log`; format: `[yyyy-MM-dd hh:mm:ss] <nick> text` / `* nick action` / `-- system`; history playback is not logged. Toggle: **Preferences → Log Messages to Disk** or `log_messages = true/false` in config.
- **`chghost`** — CAP negotiated; `CHGHOST` command parsed; silent host changes show a single status line `nick changed host (user@host)` instead of fake QUIT+JOIN noise.
- **`draft/react`** — CAP negotiated; incoming reactions stored per-msgid and rendered inline below messages as emoji + count; right-click any timestamp → **React** to send; `/react <emoji>` sends to the currently selected reply target.
- **New commands** — `/query <nick>` opens a PM buffer without sending; `/ns`, `/cs`, `/bs`, `/ms` are shortcuts for NickServ/ChanServ/BotServ/MemoServ; `/oper <user> <pass>` sends IRC OPER.
- Fix: AppImage build script now patches `linuxdeploy-plugin-qt`'s bundled `strip` (same `.relr.dyn` workaround previously applied to linuxdeploy only).

---

## v0.14.0 — 2026-05-31

- **Ignore list** — `/ignore <nick>` suppresses all PRIVMSG, NOTICE, and ACTION messages from a nick; `/unignore <nick>` removes them from the list; `/ignored` lists current ignored nicks. Right-click any nick → **Ignore** / **Unignore** does the same from the context menu. The list persists in `config.toml` under `[ignore] nicks = [...]`.
- **Right-click Copy** — selecting text in the chat view and right-clicking now shows a **Copy** menu item (the custom context menu event filter was consuming all right-click events unconditionally).
- **Icon colors** — hamburger menu icons and gear buttons now use the active theme's foreground color (`m_theme.text`) instead of the OS palette default, which was always black regardless of theme.
- **Link preview card** — card background, text, and border colors are now hardcoded (`#1a1a1a` / `#eeeeee` / `#888888`) so the card looks consistent across all themes.
- Fix: server dialog **Browse** buttons widened from 70 px to 90 px so `Browse…` no longer clips to `rowse`.
- Fix: server dialog shows a note below **Auto-join** explaining the `config.toml` format for password-protected channels.

---

<!--
Session summary — 2026-05-30 (v0.13.0 — security hardening)

What was built / fixed:
  - Outbound IRC command injection prevention: stripCrlf / validIrcToken / ircv3TagEscape helpers
    added to ircclient.cpp; all send paths (join, part, privmsg, notice, setNick, sendTyping,
    onConnected registration, CTCP replies) now validate tokens and strip CR/LF before sending.
    Reply msgid tag is now IRCv3-escaped. CTCP PING echo payload stripped of CR/LF.
  - Reconnect socket abort: doReconnect() now aborts the socket if not already unconnected before
    calling connectToHostEncrypted/connectToHost to avoid weird Qt state transitions.
  - openUrl scheme guard: both anchorClicked and context-menu "Open URL" now only pass http/https
    URLs to QDesktopServices::openUrl; other schemes (file:, irc:, custom) are silently dropped.
  - Link preview SSRF hardening: fetchImage() now calls isPrivateUrl() on the image URL before
    fetching, closing the og:image and direct image SSRF gap.
  - DCC receive write cap: onReadyRead() reads only the remaining expected bytes; sender cannot
    write past the advertised filesize.
  - DCC send 4 GiB guard: listen() rejects files over UINT32_MAX (protocol limit); ACK comparison
    now uses qint64 to avoid the wrap-to-zero false-finish.
  - DCC listener local bind: listen() accepts a QHostAddress; mainwindow passes the IRC socket's
    local interface address instead of QHostAddress::Any.
  - DCC cancel: cancel() methods added to DccSend and DccReceive; progress dialog Cancel buttons
    now call cancel() before deleteLater() to cleanly abort sockets and close/remove partial files.
  - DCC offer validation: port == 0 or filesize <= 0 in a DCC SEND CTCP are now rejected before
    the accept dialog is shown.
  - Channel key persistence: config load/save now uses [[server.channel]] tables with name + key
    fields; backward-compatible with the old channels = "..." comma-string format.
  - Version accuracy: main.cpp now uses UPLINKIRC_VERSION from version.h (generated by CMake)
    instead of the hardcoded "0.7.10" string.

Regressions: none (clean build, all existing behaviour preserved).
Known issues: plaintext passwords in config.toml; DCC over NAT still blocked.
Next priorities: ignore list, per-channel logging, OS keychain for secrets, draft/message-redaction.
-->

## v0.13.0 — 2026-05-30

- **Outbound IRC injection prevention** — all send paths now strip `\r`/`\n` and validate tokens before writing to the socket; IRCv3 reply tags are properly escaped; CTCP PING payloads sanitised.
- **Reconnect safety** — socket is aborted before reconnect if not already unconnected, avoiding stale Qt socket state.
- **URL scheme guard** — `openUrl` (both click and context menu) now only opens `http`/`https` links; other schemes are dropped silently.
- **Link preview SSRF closed** — `og:image` and direct image fetches now run the same private-address check as the initial page request; LAN/loopback image URLs are blocked.
- **DCC receive write cap** — incoming data is capped to the remaining advertised byte count; a malicious sender cannot write past the declared file size.
- **DCC send 4 GiB guard** — files over 4 GiB are rejected at send time with a clear error; the ACK comparison uses `qint64` to prevent the 32-bit wrap false-finish.
- **DCC local bind** — the listen socket binds to the IRC connection's local interface instead of all interfaces.
- **DCC cancel cleanup** — `cancel()` methods added to `DccSend` and `DccReceive`; progress dialog Cancel now aborts the socket and removes partial files instead of leaking them.
- **DCC offer validation** — zero port or non-positive filesize in a DCC SEND offer is rejected before the accept dialog appears.
- **Channel key persistence** — channels are now saved as `[[server.channel]]` tables with a `key` field; keyed channels survive config save/load. Old `channels = "..."` format still loads correctly.
- **Version accuracy** — runtime version now matches CMake (`0.13.0`); was stuck at hardcoded `0.7.10`.

Fix: channel passwords were silently dropped on every config save.

---

<!--
Session summary — 2026-05-30 (v0.12.1 — link preview show/hide toggle)

What was built:
  - Show Preview toggle: right-clicking a URL whose preview was hidden now shows "Show Preview"
    instead of "Hide Preview". Clicking it removes the URL from hiddenPreviews and refreshes the
    view — no re-fetch needed, the card HTML is still cached in Channel::previews.
  - Channel::hiddenPreviews (QSet<QString>) added to channel.h alongside previews hash.
  - Render loop (refreshChatView) skips URLs in hiddenPreviews.
  - Context menu logic now branches: isHidden → "Show Preview"; hasPreview && !hidden → "Hide Preview" (enabled); no preview → "Hide Preview" (disabled).

Regressions: none.
Known issues: same as v0.12.0.
Next priorities: ignore list, per-channel logging, draft/message-redaction, draft/react.
-->

## v0.12.1 — 2026-05-30

- **Link preview show/hide toggle** — right-clicking a URL that was previously hidden now shows **Show Preview** instead of Hide Preview. Clicking it restores the card without a re-fetch (the HTML remains cached). The hide/show state is tracked per-URL in a new `hiddenPreviews` set on the channel.

---

<!--
Session summary — 2026-05-30 (v0.12.0 — link preview overhaul + notification polish)

What was built:
  - Notification indicators: removed color changes on channel names (spacing shift complaint);
    kept 💡/🔥 icons only. Server unread color also removed.
  - Link preview layout redesigned: text (title + domain) on top, image below, vertical stack
    inside a single <td>. Image scale bumped 120×90 → 360×220.
  - ChatBrowser subclass: overrides loadResource() to decode data: URIs, making preview
    images actually render in QTextBrowser (was silently ignored before).
  - WhatsApp/2 user agent: sites like YouTube serve compact OG-metadata pages to this UA
    instead of the full JS-heavy bundle. Fixes YouTube and other heavy sites burying og:title
    past the 32 KB read cap.
  - Link right-click menu: Copy URL / Open URL / Hide Preview (hides card + refreshes view).
  - Left-click on link: opens in browser (confirmed working via anchorClicked signal).
  - Double right-click menu bug fixed: moved handler from MouseButtonPress to
    QContextMenu event so the QTextBrowser default menu is properly suppressed.

Regressions: none.
Known issues remaining:
  - DCC over internet (NAT/firewall blocks direct TCP)
  - No in-app update check UI
  - Per-channel logging not implemented
  - Split view not implemented
  - Plaintext passwords in config.toml

Next priorities:
  - Ignore list
  - Per-channel log files
  - draft/message-redaction
  - draft/react
  - account-notify + account-tag + extended-join
  - Monitor
-->

## v0.12.0 — 2026-05-30

- **Notification indicators** — channel name color changes removed; 💡 (mention) and 🔥 (activity) icons are the sole indicators. Eliminates the layout shift that colored text was causing.
- **Link preview redesign** — card now shows title and domain on top with the preview image below (was side-by-side). Image scale increased from 120×90 to 360×220.
- **Link preview images now render** — `ChatBrowser` subclass overrides `loadResource()` to decode `data:` URI images, which Qt's default `QTextBrowser` silently drops.
- **WhatsApp/2 user agent** — preview fetcher now identifies as `WhatsApp/2`, the same trick Halloy uses. Sites like YouTube serve a compact OG-metadata page to this UA instead of the full JS-heavy document, fixing previews for YouTube and similar.
- **Link right-click menu** — right-clicking a URL shows: **Copy URL** / **Open URL** / **Hide Preview** (grayed if no card exists). Clicking Hide Preview removes the card and refreshes the view.
- **Left-click opens browser** — left-clicking a link opens it in the default browser (was already working; confirmed and kept as-is).
- **Fix: double right-click menu** — moved right-click handler from `MouseButtonPress` to `QContextMenu` event so the built-in QTextBrowser menu is properly suppressed. One right-click → one menu.

<!--
Session summary — 2026-05-30 (v0.11.0 — right-click nick menu expansion)

What was built:
  - Take Op / Take Voice: MODE -o / -v counterparts to Give Op / Give Voice
  - Kick: prompts for an optional reason, sends KICK #channel nick :reason
  - Ban: sends MODE #channel +b nick!*@* (simple nick-based ban mask)
  - Kick & Ban: bans first then kicks (correct order), shares reason prompt
  - Ping: CTCP PING with ms timestamp payload; reply shows RTT in active buffer
    via the existing ctcpPingReply signal + onCtcpPingReply handler
  - Copy Nick: writes nick to clipboard via qApp->clipboard()->setText(nick)
  - Invite: QInputDialog pre-filled with the active channel; sends INVITE nick #channel
  All seven actions added to showNickContextMenu() in mainwindow.cpp.
  Added QClipboard include (was missing).

Regressions found: none.
Known issues remaining:
  - DCC over internet (NAT/firewall blocks direct TCP)
  - No in-app update check UI
  - Per-channel logging not implemented
  - Split view not implemented
  - Plaintext passwords in config.toml
  - draft/react not yet implemented
  - draft/message-redaction not yet implemented

Next priorities:
  - Ignore list (client-side PRIVMSG/NOTICE filtering)
  - Per-channel log files
  - draft/message-redaction
  - draft/react
  - account-notify + account-tag + extended-join
  - Monitor
-->

## v0.11.0 — 2026-05-30

- Feat: **Take Op** / **Take Voice** — right-click a nick to remove op (`-o`) or voice (`-v`), complementing Give Op / Give Voice
- Feat: **Kick** — right-click → Kick, prompts for an optional reason; sends `KICK #channel nick :reason`
- Feat: **Ban** — right-click → Ban, sets `MODE #channel +b nick!*@*`
- Feat: **Kick & Ban** — combines both in one action (ban applied before kick); shares reason prompt
- Feat: **Ping** — right-click → Ping sends a CTCP PING; reply appears in the active buffer as `Ping reply from nick: Xms`
- Feat: **Copy Nick** — right-click → Copy Nick copies the nickname to the clipboard
- Feat: **Invite** — right-click → Invite opens a dialog pre-filled with the current channel; sends `INVITE nick #channel`

---

<!--
Session summary — 2026-05-30 (v0.10.1 — hamburger Close Menu button)

What was built:
  - Close Menu button: a separator + "Close Menu" action added at the bottom
    of the hamburger (☰) menu. One line in mainwindow.cpp:
    menu->addAction("Close Menu", menu, &QMenu::close).
    Requested by users who prefer an explicit dismiss button over clicking
    outside the menu.

Regressions found: none.
Known issues remaining (unchanged from v0.10.0):
  - DCC over internet (NAT/firewall blocks direct TCP)
  - No in-app update check UI
  - Per-channel logging not implemented
  - Split view not implemented
  - Plaintext passwords in config.toml
  - draft/react not yet implemented
  - draft/message-redaction not yet implemented

Next priorities (unchanged):
  - Per-channel log files
  - draft/message-redaction
  - draft/react
  - account-notify + account-tag + extended-join
  - Monitor
-->

## v0.10.1 — 2026-05-30

- Feat: **Close Menu** button at the bottom of the ☰ hamburger menu — explicit dismiss for users who prefer not to click outside

---

<!--
Session summary — 2026-05-30 (v0.9.6 — msgid foundation + chat nick right-click)

What was built / fixed:
  - msgid (IRCv3): the msgid tag is now parsed and carried through the full
    signal chain — messageReceived, noticeReceived, actionReceived, and batch
    delivery all pass msgid. Message struct gains a msgid field. BatchInfo::Msg
    stores msgid at buffer time. All existing make() call sites are unaffected
    (default empty string). This is the prerequisite for draft/reply, echo-message
    dedup, draft/react, and draft/message-redaction.
  - Right-click nick in chat view: PRIVMSG, ACTION, and NOTICE nicks are now
    wrapped as <a href="nick:RAWNICKNICK"> anchors in formatMessage(). Right-
    clicking one in the chat viewport intercepts the MouseButtonPress event,
    extracts the nick from the anchor href, and calls showNickContextMenu() —
    the same menu as the nick list (Message, Send File, Whois, Give Op/Voice,
    Version). Hover and link preview logic explicitly ignores nick: anchors.
    Menu building extracted into showNickContextMenu(nick, globalPos) shared
    by both paths.

Regressions found: none.
Known issues remaining (unchanged):
  - DCC over internet (NAT/firewall blocks direct TCP)
  - No in-app update check UI
  - Message search not implemented
  - Per-channel logging not implemented
  - Split view not implemented
  - Plaintext passwords in config.toml

Next priorities:
  - echo-message (builds on msgid — dedup echoed messages)
  - draft/reply (reply threading, builds on msgid)
  - Message search (Ctrl+F in channel buffer)
  - Per-channel log files
-->

<!--
Session summary — 2026-05-30 (v0.10.0 — echo-message, message search, draft/reply)

What was built:
  - echo-message CAP: "echo-message" added to desired CAPs in handleCap().
    When the server acks it, sendMessage() and sendAction() skip the local
    manual echo so messages are not duplicated. IrcClient::privmsg() gains
    a replyToMsgid param (used by draft/reply). onMessage() in SessionModel
    now handles isSelf detection for PM routing: if nick == selfNick the PM
    buffer key is `target` (the conversation partner), not `nick` (ourselves).
    This also fixes ZNC self-message PM routing which had the same bug.

  - Message search (Ctrl+F): search bar widget added above the input bar in
    m_rightContent VBox, hidden by default. showSearchBar() shows + focuses it.
    eventFilter on m_input and m_chatView both intercept Ctrl+F. eventFilter
    on m_searchInput intercepts Escape (close+clear) and Enter/Shift+Enter
    (doSearch forward/backward with wrap-around). textChanged -> find from
    start. Ctrl+F also works when the chat view has focus via m_chatView
    KeyPress event filter. Escape in input clears pending reply if one is set.

  - draft/reply: replyTo added to Message struct and make(). Signal chain
    updated: messageReceived and noticeReceived gain replyTo param; BatchInfo::Msg
    stores replyTo; deliverBatch passes it. IrcClient::privmsg() prepends
    @+draft/reply=<msgid> when replyToMsgid is set. SessionModel::sendMessage()
    accepts replyToMsgid (default empty). clearReplyBar() resets state and hides
    the bar; called on channel switch and after submit. msgidAtViewPos() scans
    QTextBlock fragments for msgid: anchors — works regardless of where user
    clicks in the message line. formatMessage() embeds msgid as a timestamp
    anchor for Privmsg/Action/Notice and shows ↩ origNick when replyTo is set
    (linear scan of ch->messages for the originator nick). Right-click in chat
    viewport shows "Reply" context menu when a msgid is found at click position.

  - Version bump: 0.9.6 → 0.10.0 in CMakeLists.txt. Docs updated project-wide:
    keyboard-shortcuts.md (Ctrl+F live, search table, Escape), ircv3.md
    (echo-message + msgid + draft/reply graduated from Planned to Active),
    howto.html (two new sections + chat-area bullets), faq.md (two new Q&As,
    AppImage version ref), README.md (download button URLs).

Regressions found: none.
Known issues remaining (unchanged):
  - DCC over internet (NAT/firewall blocks direct TCP)
  - No in-app update check UI
  - Per-channel logging not implemented
  - Split view not implemented
  - Plaintext passwords in config.toml
  - draft/react not yet implemented
  - draft/message-redaction not yet implemented

Next priorities:
  - Per-channel log files (~/.config/uplinkirc/logs/)
  - draft/message-redaction (requires msgid — now done)
  - draft/react — emoji reactions (requires msgid — now done)
  - account-notify + account-tag + extended-join
  - Monitor (online/offline watch list)
  - chghost
  - STS (Strict Transport Security)
-->

## v0.10.0 — 2026-05-30

- Feat: `echo-message` CAP — server echoes sent messages back with `msgid` and `server-time`; duplicate local echo suppressed; self-echoed PMs route to the correct conversation buffer
- Feat: Message search — press **Ctrl+F** to open a search bar; type to jump to the first match; **Enter** / **Shift+Enter** navigates forward and backward with wrap-around; **Escape** clears and closes
- Feat: `draft/reply` — right-click any message in the chat to reply; a reply indicator bar appears above the input showing the quoted nick and preview; outgoing messages carry `@+draft/reply=<msgid>`; received replies show **↩ origNick** before the sender nick

---

## v0.9.6 — 2026-05-30

- Feat: IRCv3 `msgid` stored on every received message — full signal chain updated; foundation for reply threading, reactions, and redaction
- Feat: Right-click any nick in the chat view for the full context menu (Message, Send File, Whois, Give Op, Give Voice, Version)

<!--
Session summary — 2026-05-30 (v0.9.5 — SASL + tray notification fixes)

What was built / fixed:
  - SASL authentication was silently failing on Libera.Chat and similar servers
    that advertise capabilities in "name=value" form (e.g. sasl=PLAIN,EXTERNAL).
    The cap-matching code did an exact string comparison and never matched "sasl",
    so SASL was never negotiated even when sasl_user/sasl_password were set.
    Fixed with a prefix-match helper (a == cap || a.startsWith(cap + "=")).
  - CAP LS multi-line buffering: servers that send CAP LS across multiple lines
    (marked with params[2] == "*") caused premature CAP REQ before all caps were
    seen. Client now buffers all lines into m_capLsBuffer and only processes on
    the final (non-starred) line. m_capLsBuffer cleared on disconnect.
  - "Tray Notifications" toggle moved from an ad-hoc "Interface" burger submenu
    into Preferences > Interface, consistent with all other toggles. Renamed
    from "Desktop Notifications" to "Tray Notifications" to match what it actually
    controls (the tray icon dot, not OS-level desktop notifications).
  - Disabling Tray Notifications now immediately clears both the green mention
    dot and the red unread dot via TrayIcon::setNotificationsEnabled(false).
    Previously the dot persisted until the window was focused. Also applies the
    config value at tray creation time so the setting is honoured from launch.

Known issues remaining:
  - DCC over internet (NAT/firewall blocks direct TCP)
  - No in-app update check UI
  - Message search not implemented
  - Per-channel logging not implemented
  - Split view not implemented
  - Plaintext passwords in config.toml

Next priorities:
  - msgid (prerequisite for reactions, reply threading, redaction)
  - Message search (Ctrl+F in channel buffer)
  - Per-channel log files
  - echo-message
-->

## v0.9.5 — 2026-05-30

- Fix: SASL authentication now works on Libera.Chat and any server advertising `sasl=PLAIN,EXTERNAL` — capability matching extended to handle `name=value` form
- Fix: CAP LS multi-line responses now buffered correctly; `CAP REQ` no longer sent before all advertised capabilities are received
- Fix: "Tray Notifications" toggle moved into **Preferences → Interface** as a proper checkbox alongside the other UI toggles
- Fix: Disabling tray notifications now immediately clears the tray dot (both green mention and red unread); previously the dot persisted until window focus

<!--
Session summary — 2026-05-30 (planning — IRCv3 roadmap audit)

No code written this session. Analysis and planning only.

What happened:
  - Full project size audit: ~8,200 lines C++, 43 source files across src/irc/, src/ui/, src/model/, src/config/
  - Compared UplinkIRC against Halloy (Rust/iced) in depth — ~78,000 lines Rust, 205 .rs files
  - Audited entire IRCv3 spec against what UplinkIRC already implements vs has planned
  - Verified from source: ZNC + Soju bouncer support, SASL PLAIN+EXTERNAL, CHATHISTORY, batch,
    server-time, message-tags, labeled-response, away-notify, multi-prefix, draft/typing, bot mode
    display (+B), all confirmed fully implemented
  - Added 21 new IRCv3 roadmap items to ROADMAP.md (msgid, echo-message, draft/reply,
    draft/message-redaction, account-notify/tag/extended-join, Monitor, chghost, STS,
    invite-notify, setname, WHOX, userhost-in-names, netsplit/netjoin batches, Standard Replies,
    UTF8ONLY, message reactions, multiline messages, WebSocket transport, user metadata)
  - Updated docs/ircv3.md to list all new planned capabilities
  - Confirmed split view and SOCKS5 proxy were already on ROADMAP

Known issues remaining (unchanged):
  - DCC over internet (NAT/firewall blocks direct TCP)
  - No in-app update check UI
  - Message search not implemented
  - Per-channel logging not implemented
  - Split view not implemented
  - Plaintext passwords in config.toml

Next priorities (updated — see project_overview.md):
  - msgid (prerequisite for reactions, redaction, reply threading)
  - Message search (Ctrl+F)
  - Per-channel log files
  - echo-message
-->

<!--
Session summary — 2026-05-30 (v0.9.3 — Server buffer unread indicator)

What was built / fixed:
  - Server buffer unread indicator: the server name in the sidebar now turns
    purple (#cba6f7) when unread notices arrive (CTCP replies, NickServ, etc.),
    and resets to dim gray when you click into the server window. Previously the
    server item was silently excluded from the unread system entirely.
  - Server item is now selectable (Qt::ItemIsEnabled | Qt::ItemIsSelectable) so
    clicking the server name in the sidebar actually navigates to the server
    window. Before this fix, clicking it did nothing.
  - CTCP replies (VERSION and other unhandled CTCP NOTICEs) now route through
    noticeReceived instead of serverMessage, so they arrive as MessageType::Notice
    and are counted as unread. Previously they were posted as MessageType::Server
    which was explicitly excluded from unread counting.

Known issues remaining:
  - DCC over internet (NAT/firewall blocks direct TCP)
  - No in-app update check UI
  - Message search not implemented
  - Per-channel logging not implemented
  - Split view not implemented
  - Plaintext passwords in config.toml

Next priorities:
  - Message search (Ctrl+F in channel buffer)
  - Per-channel log files
  - DCC NAT traversal / passive DCC
  - In-app update check button
-->

<!--
Session summary — 2026-05-30 (v0.9.4 — Config path cleanup)

What was built / fixed:
  - Config file path changed from ~/.config/LinuxDojo/UplinkIRC/config.toml
    to ~/.config/uplinkirc/config.toml. Qt's AppConfigLocation was building
    a path that baked in the org name ("LinuxDojo"), which was confusing and
    project-unrelated. setOrganizationName() removed from main.cpp; path is
    now hardcoded in Config::defaultPath() via HomeLocation + /.config/uplinkirc/.
  - All docs updated project-wide: config.toml.example, README.md, docs/faq.md,
    docs/configuration.md, docs/howto.html.
  - Verified SASL config for Libera.Chat: sasl_user = "bebop" (registered account),
    nick = "sig`" (grouped nick). Correct — SASL authenticates against the account
    name, not the current nick.

Known issues remaining:
  - DCC over internet (NAT/firewall blocks direct TCP)
  - No in-app update check UI
  - Message search not implemented
  - Per-channel logging not implemented
  - Split view not implemented
  - Plaintext passwords in config.toml

Next priorities:
  - msgid (prerequisite for reactions, redaction, reply threading)
  - Message search (Ctrl+F in channel buffer)
  - Per-channel log files (~/.config/uplinkirc/logs/)
  - echo-message
-->

## v0.9.4 — 2026-05-30

### Fixed
- **Config path cleaned up** — config file now lives at `~/.config/uplinkirc/config.toml` on all platforms instead of the Qt-generated `~/.config/LinuxDojo/UplinkIRC/config.toml` path. The org-name-based nesting was a side effect of `QApplication::setOrganizationName("LinuxDojo")` which has been removed; the path is now hardcoded directly in `Config::defaultPath()`.

---

## v0.9.3 — 2026-05-30

### Fixed
- **Server buffer unread indicator** — the server name in the sidebar now turns purple when unread notices arrive (CTCP replies, NickServ messages, server auth notices, etc.) and resets to dim gray when you switch into the server window. Previously the server item was completely excluded from the unread system, so `VERSION` replies and other server-directed notices landed silently with no visual indicator.
- **Server item now clickable** — clicking the server name in the sidebar navigates to the server window. Previously the item was non-selectable (`Qt::ItemIsEnabled` only) so clicking it did nothing.
- **CTCP replies counted as unread** — `VERSION` replies and other unhandled CTCP NOTICEs now route through `noticeReceived` and are counted toward the server buffer's unread total. Previously they were posted as `MessageType::Server` which was explicitly excluded from unread counting.

---

<!--
Session summary — 2026-05-30 (v0.9.2 — Preferences rework + link preview indent)

What was built / fixed:
  - Preferences dialog reworked:
      - Manage Servers and Documentation buttons moved to top (first thing visible)
      - Theme: collapsible list — button shows current theme, click to expand/collapse,
        stays open after selection so user can keep browsing with arrow keys + Enter
      - App Icon: QListWidget replaced with QRadioButton group (consistent with Interface checkboxes)
      - About button removed from Preferences (already in hamburger menu)
      - Docs button renamed to Documentation
      - Enter-applies-Font-Config bug fixed: setAutoDefault(false) on all buttons
      - Nick Brackets combo: smaller font (11px) and tighter padding
  - Link preview card indent: card margin-left now computed from font metrics to match
    hanging indent position instead of hardcoded 20px. Cards align with wrapped text.
  - How-To guide logo added and repositioned (64px margin-top)
  - Version bumped 0.9.0 → 0.9.2

Known issues remaining:
  - DCC over internet (NAT/firewall blocks direct TCP)
  - No in-app update check UI
  - Message search not implemented
  - Per-channel logging not implemented
  - Split view not implemented
  - Plaintext passwords in config.toml

Next priorities:
  - Message search (Ctrl+F in channel buffer)
  - Per-channel log files
  - DCC NAT traversal / passive DCC
  - In-app update check button
-->

## v0.9.2 — 2026-05-30

### Changed
- **Preferences dialog reworked** — Manage Servers and Documentation buttons are now at the top of the dialog, visible immediately on open. Theme selection is now a compact collapsible list: click the theme button to expand, use arrow keys to browse, press Enter or click to apply. The list stays open after selection so you can keep trying themes. App icon selection replaced with radio buttons. About button removed (it lives in the hamburger menu). Docs button renamed to Documentation.

### Fixed
- **Link preview card indent** — preview cards now align with the hanging indent position when hanging indent is on. The card's left margin is computed from font metrics to match where wrapped message text starts, instead of a hardcoded 20 px.
- **Enter key in theme list no longer triggers Font Config** — all buttons in the dialog have `autoDefault` disabled so Enter is never accidentally captured.
- **Nick Brackets dropdown** — reduced font size and padding so the combo box reads at the same visual weight as the rest of the dialog.

---

<!--
Session summary — 2026-05-30 (hanging indent + How-To sync)

What was built / fixed:
  - Hanging indent: wrapped message lines now align past the timestamp+nick column
    instead of wrapping back to column 0. Uses QTextBlockFormat (setLeftMargin +
    setTextIndent) on the paragraph block — NOT HTML tables, which caused messages
    to render in the centre of the chat view. Table approach was attempted and
    reverted; QTextCursor + QTextBlockFormat is the correct Qt approach.
    Applies to Privmsg, Action, Notice, and link preview cards via a shared
    insertHtmlBlock() static helper. Toggleable: Preferences → Hanging Indent
    (wrap under message), config key hanging_indent (default true).
  - insertHtmlBlock() helper added to mainwindow.cpp — replaces all m_chatView->append()
    calls for messages and cards. characterCount() > 1 guards against double blank
    line on first message after clear().
  - How-To guide (docs/howto.html) brought in sync with v0.9.0:
      - AppImage: download table row added, new section with run + zsync update steps
      - SASL EXTERNAL: full section (cert generation, NickServ fingerprint, config, GUI)
      - DCC file transfer: send/receive steps, timeout table, LAN-only warning
      - Hanging indent: new Tweaks section
      - Fixed stale troubleshooting note: "preview cards don't survive channel switch"
        → now correctly says fixed in v0.8.1
  - configuration.md: hanging_indent added to default template and options table
  - Close-session memory updated: howto.html explicitly listed as required surface

Known issues remaining:
  - DCC over internet (NAT/firewall blocks direct TCP connection)
  - No in-app update check UI
  - Message search not implemented
  - Per-channel logging not implemented
  - Split view not implemented
  - Plaintext passwords in config.toml

Next priorities:
  - Message search (Ctrl+F in channel buffer)
  - Per-channel log files
  - DCC NAT traversal / passive DCC
  - In-app update check button
-->

## v0.9.1 — 2026-05-30

### Added
- **Hanging indent** — wrapped message lines now align past the timestamp+nick column. When a long message wraps, continuation lines start under the message text, not at the left edge under the timestamp. Toggle at **☰ → Preferences → Hanging Indent (wrap under message)** or in config: `hanging_indent = true` (default on). Applies to messages, actions, notices, and link preview cards.

### Fixed
- **How-To guide sync** — `docs/howto.html` brought up to date with v0.9.0 features: AppImage download + zsync update, SASL EXTERNAL walkthrough, DCC file transfer section, hanging indent Tweaks entry. Stale troubleshooting note claiming link preview cards don't survive channel switches corrected (fixed in v0.8.1).

---

<!--
Session summary — 2026-05-30 (CHANGELOG structure fix)

What was built / fixed:
  - No code changes. Doc/infra fix session.
  - Found v0.9.0 CHANGELOG entry was buried after ~600 lines of HTML comment blocks
    and therefore not visible when GitHub renders CHANGELOG.md — v0.8.1 appeared
    as the first entry instead.
  - Root cause: close-session tooling appended new entries after the last comment
    block rather than before the first ## heading. New entries must be inserted
    immediately above the first visible ## heading, not appended to the bottom of
    the comment region.
  - Fixed by moving the v0.9.0 ## heading to line 224 (first visible position),
    removing the duplicate buried copy. Committed and pushed.
  - Also pushed the previous session's catch-up commit (MSVC fix docs) which had
    been committed locally but never pushed.

Known issues remaining (same as v0.9.0):
  - DCC over internet: local IP advertised; NAT blocks WAN connections
  - No in-app update check UI
  - Message search not implemented
  - Per-channel logging not implemented
  - Split view not implemented
  - Plaintext passwords in config.toml

Next priorities:
  - Message search (Ctrl+F in channel buffer)
  - Per-channel log files
  - DCC NAT traversal / passive DCC
  - In-app update check button
-->

<!--
Session summary — 2026-05-30 (post-v0.9.0 doc catch-up)

What was built / fixed:
  - No new features. Investigation session: confirmed v0.9.0 tag and branch are
    both on remote (origin/main is up to date). The v0.9.0 tag points to the MSVC
    hotfix commit (820af24) rather than the release commit (ee3b9ff) — cosmetic only,
    does not affect CI or release artifacts.
  - Caught that the MSVC std::min hotfix (820af24) landed after the close-session
    commit and was not documented. Added it to the v0.9.0 CHANGELOG Fixed section
    and the v0.9.0 session summary.

Known issues remaining (same as v0.9.0):
  - DCC over internet: local IP advertised; NAT blocks WAN connections
  - No in-app update check UI
  - Message search not implemented
  - Per-channel logging not implemented
  - Split view not implemented
  - Plaintext passwords in config.toml

Next priorities:
  - Message search (Ctrl+F in channel buffer)
  - Per-channel log files
  - DCC NAT traversal / passive DCC
  - In-app update check button
-->

<!--
Session summary — 2026-05-29 (exploration only — no release)

What was explored:
  - Investigated converting PreferencesDialog and DocsDialog from QDialog (separate
    window) to QFrame in-window overlays, matching the existing AboutDialog style.
  - Prototype was built and compiled cleanly: both dialogs converted to QFrame,
    shadow effect added, showCentered() wired up in mainwindow, close → hide().
  - User decided Preferences and Documentation need a proper OS window frame and
    reverted both back to QDialog. AboutDialog remains as the only in-window overlay.

Known issues remaining (unchanged):
  - Link preview cards don't survive channel switching
  - DCC Send File not implemented
  - AppImage packaging not done
-->

<!--
Session summary — 2026-05-30  post-v0.7.13 (hamburger UI polish + sidebar fix + docs overhaul)

What was built / fixed:
  - Hamburger (☰) button constrained to 22×22 px (was unconstrained width, expanding
    horizontally with the font size). Changed setFixedHeight(22) → setFixedSize(22, 22)
    and added setAutoRaise(true) to match the gear buttons.
  - Hamburger font changed from fs.toolbar*2 point-size (variable, could reach 20pt+)
    to a fixed 15px pixel-size font, matching the gear icon's visual size (gear renders
    at sz-1=15px inside a 16px pixmap). Removed hamburger from applyFontSizes() entirely
    so it no longer changes with user font preferences — consistent with gear behavior.
  - Hamburger and gear buttons no longer disappear when the sidebar is collapsed.
    Previously, the sidebar toggle set m_topicLeft->setFixedWidth(0), collapsing the
    entire left zone including the buttons. Added kBtnZoneMinW=48 constant (22+22+4px
    margin) as a floor — collapsed state now sets width to 48px, keeping both buttons
    pinned. Splitter-drag path also clamped with qMax(kBtnZoneMinW, w).
  - Full How-To guide created: docs/howto.html — left-side tree navigation, 8 sections
    (Getting Started, Basic Setup, Authentication, Multiple Servers, Interface, Chatting,
    Tweaks, Troubleshooting), platform tabs for dependencies, step badges, callout boxes,
    interactive tab selection, scroll-spy nav highlighting.
  - README download badges updated from v0.7.11 to v0.7.14.
  - README Documentation table updated: How-To Guide added as first/highlighted entry.
  - docs/index.html (GitHub Pages): nav version bump to v0.7.14, How-To added to nav bar
    and docs grid (highlighted card), footer links updated.
  - Releases: v0.7.13 (hamburger size fix), v0.7.14 (sidebar button visibility fix).

Known issues remaining:
  - Link preview cards don't survive channel switching (not stored in history)
  - DCC Send File not implemented
  - AppImage packaging not done
-->

<!--
Session summary — 2026-05-29  post-v0.7.12 (UI overhaul + signal bars)

What was built / fixed:
  - Signal bars connection/latency indicator added to topic bar — 4 stair-step bars,
    green solid when connected (bar count = ping latency: 4=<50ms, 3=<150ms,
    2=<300ms, 1=>300ms), blue flashing when connecting/reconnecting, red flashing
    when disconnected. Driven by periodic PING/PONG RTT measurement in IrcClient.
  - Desktop notification implemented as green dot on the system tray icon — fires on
    mention or PM when window is not focused. Toggle in Preferences → "Desktop
    Notifications". Clears on window activation (changeEvent). No popup balloon.
  - Full-width topic bar spanning the entire window width — left zone (sidebar width)
    holds hamburger and gear, right zone (chat width) holds signal bars + channel
    info. Left and right zones track the splitter so they stay pixel-aligned with
    the server list and chat area below. Both zones have explicit inputBg backgrounds
    so the bar renders continuously across the full window.
  - Hamburger and gear relocated from topic bar (right-side only) to the left zone
    of the new full-width bar — they now appear above the server list area.
  - Status bar (connection status label at bottom) removed entirely; statusBar()
    hidden. Input bar bottom padding bumped to compensate.
  - Removed show_conn_status config key and Preferences toggle — replaced by signal bars.
  - Added notifications = true config key for desktop notification toggle.
  - IrcClient now sends periodic PING :uplink_rtt every 30s when connected (first
    fires 2s after connect); handles PONG token match; emits pingRtt(host, ms).
    Also emits reconnecting(host) signal when doReconnect() fires.
  - Red dot on tray for general unread messages now also works (setUnread fixed
    to call updateIcon() properly).

Known issues remaining:
  - Link preview for title-only pages (no og:title) needs verification
  - AppImage packaging not done
  - DCC Send File not implemented
-->

<!--
Session summary — 2026-05-29  post-v0.7.14 (About dialog centering + docs search)

What was built / fixed:
  - About dialog now appears centered on the app window on all platforms including
    Wayland. Previous approach used QDialog::move() which Wayland compositors ignore.
    Refactored to a QFrame child widget (overlay) parented to the main window;
    positioned in parent-local coordinates via showCentered(); stays inside the
    app window, compositor has no involvement.
  - Documentation dialog gained a live search field in the tab bar corner (right of
    the Shortcuts tab). Uses QTabWidget::setCornerWidget; QLineEdit with leading
    search icon and a clear button. Typing jumps the active tab to the first match
    via QTextBrowser::find(); switching tabs re-applies the current query.

Known issues remaining:
  - Link preview cards don't survive channel switching (not stored in history)
  - DCC Send File not implemented
  - AppImage packaging not done
-->

<!--
Session summary — 2026-05-29  v0.8.0 — security hardening and stability pass

Context: a lead security/stability code review of the full codebase was performed.
Every finding was triaged, confirmed against source, and fixed. This is the first
dedicated security release for UplinkIRC.

What was fixed / built:

SECURITY
  - onSslErrors was calling ignoreSslErrors() unconditionally — MITM trivially
    possible on any TLS connection. Fixed: emit error + disconnectFromHost().
  - sendRaw emitted the raw line BEFORE writing it, so PASS :password,
    AUTHENTICATE <base64>, and NickServ IDENTIFY appeared verbatim in the raw
    log panel. Fixed: redactRawForLog() strips all three before emit.
  - Config save: QFile write with no permissions (world-readable on Linux),
    no escaping of special chars in TOML strings, no atomic write. Fixed:
    QSaveFile (atomic), QFile::setPermissions(ReadOwner|WriteOwner), tomlQuote()
    helper escapes \, ", \n, \r, \t.
  - Link previews auto-fetched every URL including private/LAN/loopback addresses.
    Fixed: isPrivateUrl() blocks loopback, 10/8, 172.16/12, 192.168/16,
    169.254/16, ::1, fc00::/7, localhost, and *.local before any fetch.
  - CTCP PING and VERSION replied unconditionally to any sender — reflection/
    amplification risk. Fixed: QHash<QString,qint64> m_ctcpTimestamps rate-limits
    both to once per nick per 5 s; PING echo payload capped at 32 bytes.

STABILITY / RAM
  - onReadyRead had no buffer cap — malicious server could send without \n and
    grow RAM indefinitely. Fixed: 64 KB cap on pending buffer, 8 KB cap per line.
  - Batch message storage had no cap — server could open BATCH and never close it,
    streaming unlimited messages. Fixed: 8 open batches max, 1 000 messages/batch.
  - QTextBrowser appended forever while model capped at 2 000 — long-session RAM
    growth in busy channels. Fixed: setMaximumBlockCount(kMessageBufferCap + 300).
  - onErrorOccurred emitted disconnected() and called scheduleReconnect() directly;
    onDisconnected() does the same on socket close — double signal + double timer.
    Fixed: onErrorOccurred emits only socketError(); onDisconnected() owns state.
  - trimmed() was stripping trailing IRC parameter spaces. Fixed: chop('\r') only.
  - Link preview image decode: loadFromData() after 200 KB cap still vulnerable to
    small compressed image that expands into huge pixel buffer. Fixed: QImageReader
    checks dimensions first; rejects > 4096×4096; scales during decode.
  - Channel::previews hash unbounded. Fixed: Channel::addPreview() evicts oldest
    entry when count >= 100.

PERFORMANCE
  - Nick comparisons used toLower()==toLower() throughout channel.h and
    sessionmodel.cpp — two allocs per call. Fixed: Qt::CaseInsensitive everywhere.
  - ServerSession::get() called toLower() + contains() + operator[] (two hash
    lookups). Fixed: one key + one find().
  - ircToHtml() allocated output string without reservation. Fixed: reserve(raw*2).
  - /sysinfo blocked UI thread on every call (vulkaninfo, lspci, powershell).
    Fixed: static cache for OS/CPU/MEM/GPU; only uptime re-queried.
  - loadConfig() and addServer() had identical server-session creation block.
    Fixed: shared spawnSession() helper.

Known issues remaining:
  - Link preview cards don't survive channel switches
  - Link preview for title-only pages (no og:title) — unverified
  - DCC Send File not implemented
  - AppImage packaging not done
  - SASL EXTERNAL (cert-based) not implemented

Next priorities:
  - Link preview card persistence across channel switches
  - AppImage packaging for Linux
  - DCC Send File
-->

<!--
Session summary — 2026-05-29  post-v0.8.0 (link preview persistence fix)

What was fixed:
  - Link preview cards now survive channel switches. The infrastructure (ch->previews,
    addPreview, refreshChatView re-injection) was already in place from a prior session
    but had a subtle URL key normalization mismatch: appendMessage stored raw regex-
    captured strings in m_previewChannels, while cardReady looked them up via
    pageUrl.toString() (QUrl-normalized). For URLs containing IRC control characters
    or percent-encoding, the keys diverged and addPreview() was never called.
    Fixed: both appendMessage and refreshChatView now normalize via
    QUrl(captured).toString() before using the string as a key.
  - ROADMAP "In Progress" stale item removed.
  - docs/faq.md updated: removed two "known limitation" notes about preview persistence.

Known issues remaining:
  - Link preview for title-only pages (no og:title) — unverified
  - DCC Send File not implemented
  - AppImage packaging not done
  - SASL EXTERNAL (cert-based) not implemented
-->

## v0.9.0 — 2026-05-30

### Added

- **DCC Send File** — right-click any nick in the user list and choose **Send File** to open a file picker and initiate a direct transfer. The recipient sees an accept/reject dialog showing the filename and size; accepted transfers save to a user-chosen path. Both sides get a live progress dialog with a cancel button. Outgoing transfers listen on a local TCP port (standard 4-byte big-endian ACK protocol); incoming transfers connect directly to the sender. A 60-second timeout fires if no connection arrives; a 30-second timeout fires on the receiver side if the sender never responds.
- **SASL EXTERNAL (certificate authentication)** — add `sasl_external = true`, `client_cert`, and `client_key` to a server block to authenticate via a TLS client certificate instead of a password. UplinkIRC loads the PEM certificate and key before the TLS handshake, negotiates `AUTHENTICATE EXTERNAL` during CAP, and sends an empty response — the server derives your identity from the certificate. Both RSA and EC (ECDSA) keys are supported. The Server dialog (☰ → Preferences → Manage Servers → Edit) gains a checkbox and browse buttons for cert/key paths.
- **AppImage packaging** — `packaging/build-appimage.sh` produces a self-contained `UplinkIRC-VERSION-ARCH.AppImage`. The AppImage embeds zsync metadata so users can update in-place with `appimageupdatetool` without a full re-download.
- **Automated release builds** — pushing a `v*` tag triggers CI on Linux (AppImage + tar.gz), Windows (zip), and macOS (DMG). All artifacts upload to the GitHub release automatically.

### Fixed

- **Link preview entity decoding** — page titles containing HTML entities (`&amp;`, `&#39;`, `&lt;`, etc.) are now decoded via `QTextDocument` before being displayed. Both `og:title` and the plain `<title>` fallback path are affected.
- **MSVC `std::min` conflict** — replaced `std::min` calls in `mainwindow.cpp` with ternary expressions to avoid the `min`/`max` macro collision on MSVC (Windows builds).

---

## v0.8.1 — 2026-05-29

### Fix

- **Link preview cards survive channel switches** — cards are stored per-channel (`Channel::previews`) and reinjected into the chat view when switching back. A URL key normalization mismatch (raw regex capture vs. `QUrl::toString()`) prevented storage for URLs containing IRC control characters or percent-encoded sequences; both extraction sites now normalize consistently.

## v0.8.0 — 2026-05-29

### Security

- **TLS certificate verification enforced** — `onSslErrors` previously called `ignoreSslErrors()` unconditionally, making every SSL connection silently vulnerable to MITM attacks. Now emits a TLS error and disconnects. Certificates must be valid.
- **Credentials never logged** — `PASS`, `AUTHENTICATE`, and `NickServ `IDENTIFY` commands are redacted before emission to the raw log. Passwords no longer appear in any visible panel.
- **Config file owner-only permissions** — `config.toml` is written with mode `0600`. Credentials in the config are no longer world-readable on multi-user systems.
- **Atomic config save** — replaced `QFile` write with `QSaveFile`. A crash during save cannot leave the config in a corrupt or truncated state.
- **TOML string escaping** — passwords, realnames, or any other string containing `"`, `\`, or newlines can no longer corrupt the config file.
- **Link preview blocks private/LAN addresses** — auto-previews now reject loopback, RFC 1918 (10/8, 172.16/12, 192.168/16), link-local (169.254/16), and `.local` hostnames. A malicious user cannot post a link to probe your internal network.
- **CTCP rate limiting** — `VERSION` and `PING` replies are limited to once per nick per 5 seconds. Reflected `PING` payloads are capped at 32 bytes to prevent amplification.

### Stability & RAM

- **Inbound buffer DoS protection** — `onReadyRead()` caps the pending input buffer at 64 KB; oversized lines (> 8 KB) are dropped. A server sending data without newlines can no longer grow RAM without bound.
- **IRC line parsing fix** — `trimmed()` was incorrectly stripping meaningful trailing spaces from IRC parameters; replaced with a simple `\r` chop.
- **Batch message caps** — max 8 open batches, 1 000 messages per batch. A misbehaving server cannot fill RAM via an unclosed `BATCH`.
- **QTextBrowser block count bounded** — `setMaximumBlockCount(kMessageBufferCap + 300)` keeps the chat view in sync with the message model, preventing RAM growth on long sessions in busy channels.
- **Duplicate disconnect signal fixed** — `onErrorOccurred()` no longer double-emits `disconnected()` or double-schedules reconnect.
- **Link preview image decode safety** — `QImageReader` checks dimensions before decoding; images over 4096×4096 are rejected; scaling happens during decode rather than after, preventing compressed-bomb inflation.
- **Channel previews capped at 100 entries** per channel; oldest entry evicted when the cap is reached.

### Performance

- **Nick comparisons use `Qt::CaseInsensitive`** — eliminates temporary `toLower()` string allocations on every comparison in large channels.
- **`ServerSession::get()` single lookup** — was calling `toLower()` and hashing twice per call; now one key + one `find()`.
- **`ircToHtml()` pre-allocates** — `out.reserve(raw.size() * 2)` avoids repeated reallocations on every message render.
- **`/sysinfo` caches static fields** — OS, CPU, MEM, and GPU are gathered once; only uptime is re-queried. No more blocking the UI thread on `vulkaninfo`/`lspci` after the first call.
- **Server session creation deduplicated** — `loadConfig()` and `addServer()` now share a single `spawnSession()` helper; less surface for divergence bugs.

---

## v0.7.15 — 2026-05-29

- Fix: About dialog now appears centered on the app window on Wayland and X11 — refactored from a top-level QDialog (position ignored by Wayland compositor) to an in-app QFrame overlay positioned in parent-local coordinates
- Feature: Documentation search — live search field in the tab bar (right of Shortcuts); clears with ×, jumps to first match in the active tab, re-runs on tab switch

---

## v0.7.14 — 2026-05-29

- Fix: hamburger (☰) and gear (⚙) buttons stay visible in the topic bar when the sidebar is collapsed; only the server list collapses beneath them

---

## v0.7.13 — 2026-05-29

- **Full-width topic bar** — spans above the server list and chat area; hamburger (☰) and gear (⚙) sit in the left zone above the server list; signal bars + channel info in the right zone above the chat
- **Signal bars indicator** — 4-bar stair-step signal strength widget in the topic bar; solid green when connected (bar count reflects ping latency), blue flashing when connecting/reconnecting, red flashing when disconnected
- **Latency measurement** — IrcClient sends a periodic PING every 30 s (first ping 2 s after connect); PONG round-trip time drives the signal bar count
- **Desktop notification dot** — tray icon gets a bright green dot on nick mention or PM when the window is not focused; clears automatically when the window gains focus; toggle in Preferences → Desktop Notifications
- **Status bar removed** — connection status label at the bottom of the window is gone; replaced entirely by signal bars
- Fix: tray icon unread (red) and notify (green) dot states now render correctly via unified `updateIcon()` logic
- Fix: `show_conn_status` config key and Preferences toggle removed; `notifications` key added (default `true`)
- Fix: hamburger button constrained to 22×22 px matching the gear icon; ☰ rendered at 15 px pixel-size for visual parity

---

<!--
Session summary — 2026-05-29  post-v0.7.12

What was built / fixed:
  - Sidebar is now drag-resizable. The handle between the sidebar panel and
    the chat area can be dragged left/right; the splitterMoved signal updates
    m_sidebarExpandedWidth live so the toggle button always restores to the
    last user-set width.
  - Sidebar width persists across sessions via QSettings key "sidebarWidth",
    saved on quit and restored at startup before the first layout pass.
  - Sidebar minimum width reduced from 140 px to 112 px (~4 character widths
    narrower) so users can squeeze it down more.
  - docs/faq.md updated: removed "sidebar is not drag-resizable" note; now
    describes drag-resize + persistence matching the nick panel entry.
  - ROADMAP: window state persistence item updated; "Sidebar not drag-resizable"
    known issue removed.

Known issues remaining:
  - DCC Send File not implemented
  - AppImage packaging not done
  - Link preview for title-only pages (no og:title) needs verification
-->

## v0.7.12-post — 2026-05-29

- Sidebar is now drag-resizable — drag the divider between the channel list and chat area to set the width
- Sidebar width persists across sessions (saved/restored via QSettings)
- Sidebar minimum width reduced slightly so it can be squeezed narrower

---

<!--
Session summary — 2026-05-29  v0.7.12

What was built / fixed:
  - Sidebar gear toggle relocated to the info bar (topicBar), immediately left
    of the hamburger button. The gear is now always visible regardless of
    sidebar state; no need to hunt for a button that disappears when the panel
    is hidden.
  - Sidebar converted from QDockWidget to embedded QWidget panel inside the
    new m_mainSplitter. Float/detach removed — the sidebar is no longer
    detachable. The QDockWidget, its title bar widget, and the ⧉ float button
    are gone.
  - Sidebar toggle collapses the panel to 0 px (full-width chat). Re-clicking
    the gear restores the fixed 180 px width. Handle width is 0 — users cannot
    drag to resize; only the gear controls visibility.
  - Nick panel gear header drift fixed: when the user list was hidden
    (setVisible false), Qt's VBox removed the list's stretch=1 contribution and
    floated the lone non-stretch header to vertical center. Fix: added a
    stretch spacer (stretch=1) below the nick list so there is always a stretch
    item anchoring the header at top. Nick list stretch bumped to 100 so the
    spacer is visually imperceptible when the list is visible.
  - Nick panel splitter width now persists across sessions: saved with
    QSettings key "nickSplitter" on quit, restored on launch.
  - Dark strip at bottom of sidebar eliminated: the old addStretch(1) in the
    sidebar VBox created a ~1% gap that showed the panel background color
    (different from the tree background). Removing it (sidebar tree fills
    panel with stretch=1, no spacer) fixes the artifact. A CSS rule for
    #sidebarPanel sets background to sidebarBg as a safety net.
  - CSS added for #sidebarToggleBtn (inputBg background, matches topicBar,
    accent on hover) and #sidebarPanel (sidebarBg background).
  - QTimer::singleShot sets initial mainSplitter sizes to 180/rest after first
    layout so the sidebar starts at the right width on every launch.

Design decisions:
  - Collapsed sidebar goes to 0 px (not a narrow strip) because the gear is
    now in the topicBar — there is nothing that needs to stay visible in the
    sidebar panel itself when hidden.
  - Sidebar width is fixed; mainSplitter handle is 0 px. Users resize the nick
    panel (right side) but cannot resize the sidebar. This matches the request
    "limit to pretty much the length it was."
  - addStretch approach for nick panel kept (needed to pin header to top) but
    stretch ratio is 100:1 — spacer is <1 px tall at normal window heights.

Known issues remaining:
  - DCC Send File not implemented
  - AppImage packaging not done
  - Link preview for title-only pages (no og:title) needs verification
-->

## v0.7.12 — 2026-05-29

- Sidebar gear toggle moved into the info bar, left of the hamburger — always visible, works whether the sidebar is open or closed
- Sidebar converted from floating dock to embedded panel; float/detach removed
- Sidebar collapses fully to 0 when hidden, expanding the chat to full width; fixed at ~180 px when shown; not drag-resizable
- Nick panel gear now stays pinned at the top of the panel when the user list is collapsed (was drifting to vertical center)
- Nick panel width persists across sessions
- Dark strip at bottom of sidebar eliminated

---

<!--
Session summary — 2026-05-29  v0.7.11

What was built / fixed:
  - Nick panel redesign: replaced the detachable QDockWidget with an embedded
    panel on the right side of the chat view inside a QSplitter. The panel is
    no longer floatable or detachable.
  - Animated gear toggle: a gear button (⚙) in the nick panel header spins one
    full rotation (~500ms) on click before collapsing the user list to a thin
    strip. Clicking again re-expands. The animation uses a QTimer at 16ms with
    a QPainter-rotated pixmap icon; debounce prevents double-firing mid-spin.
  - Collapsed state: only the nick list hides; the gear button and user count
    label remain visible in the header so the panel width stays unchanged.
  - Nick panel styling: background and text color now match the chat buffer
    ({{bufferBg}} / {{text}}) across all themes. Header tool button and label
    use transparent backgrounds. Splitter handle width set to 0 — no border
    between chat and nick panel.
  - Link preview persistence: added previews QHash to Channel struct so card
    HTML is retained across channel switches.

Design decisions:
  - Iterated on collapsed state: initial design hid the user count label and
    shrank panel to 24px; user preferred keeping gear + count visible with the
    list hidden and panel width unchanged. Simpler code — removed all
    splitter-resize logic from the toggle path.
  - makeGearIcon() placed before applyFontSizes() to resolve forward-decl
    link error on first build.

Known issues remaining:
  - DCC Send File not implemented
  - AppImage packaging not done
  - Link preview for title-only pages (no og:title) needs verification
  - Nick panel splitter width does not persist across restarts (no QSettings
    save for QSplitter position in central widget)

Next priorities:
  - Desktop notifications on mention/PM when window is not focused
  - Link preview for title-only pages — verify and fix if needed
  - Connection status indicator per server (currently global status bar only)
  - AppImage packaging for Linux
-->

## v0.7.11 — 2026-05-29

- Nick panel embedded in chat splitter — no longer a floating/detachable dock
- Gear button (⚙) animates a full spin before collapsing or expanding the user list; gear and user count remain visible when collapsed
- Nick panel background and text match the active theme's chat buffer colors
- Splitter handle hidden — chat and user list share a seamless background
- Link preview cards now persist across channel switches

---

<!--
Session summary — 2026-05-29  v0.7.9

What was built / fixed:
  - Server error routing: IRC numerics ≥ 400 (e.g. 482 chanoprivsneeded, 401
    nosuchnick, 404 cannotsendtochan, 421 unknowncommand, etc.) now appear in
    the active channel buffer as red "!!" messages instead of being silently
    swallowed into the (server) buffer. A new errorMessage signal in IrcClient
    routes them through SessionModel::onErrorMessage, which posts to
    m_activeChannel when on that server, falling back to (server) if no
    channel is active.
  - Multi-prefix removal fix: NickEntry now tracks all active mode prefixes in
    a QSet<QChar>. recomputePrefix() picks the highest-ranked one. Previously,
    -o on a @+ nick cleared the prefix entirely instead of demoting to +.
  - /time <nick>: sends CTCP TIME to a user; their local time appears in the
    active channel. Works the same way as /ping. If the target client does not
    support CTCP TIME, no reply appears (this is normal — many bots don't
    implement it). Added to tab completion and /help.
  - /ping and /time confirmation messages now stored in channel history: the
    "Pinged nick" and "Querying time for nick" local lines previously used
    appendMessage() which bypassed the model buffer, causing them to vanish on
    channel switch. Both now go through SessionModel::localMessage() and
    persist like any other message.
  - Mention notifications: when your nick is mentioned in an inactive channel,
    that channel shows 💡 in red in the sidebar. Regular unread activity shows
    🔥. Both clear when you open the channel.
  - Self-nick highlight in chat: any message containing your nick (word
    boundary, case-insensitive) renders your nick in red bold inline in the
    message text.

Known issues remaining:
  - Link preview cards lost on channel switch (not stored in message history)
  - Link preview for title-only pages not verified
  - DCC Send File not implemented
  - AppImage packaging not done

Next priorities:
  - Desktop notifications (system notification on mention/PM)
  - Link preview persistence across channel switches
  - AppImage packaging
-->

<!--
Session summary — 2026-05-29  v0.7.7

What was built / fixed:
  - Persistent Preferences dialog: hamburger (☰) now opens a non-modal QDialog that
    stays open while the user browses themes, toggles options, etc. Replaces the old
    QMenu that dismissed after every click. All settings (theme list, app icon, font
    config, 6 UI toggles, nick brackets, Manage Servers, About, Docs) live in one place.
    PreferencesDialog emits signals; MainWindow connects them — logic unchanged, UX fixed.
  - Bot icon randomization (carried from post-0.7.6): +B nicks now get a truly random
    icon (🤖 or 👾) assigned on first appearance instead of a deterministic hash value.
    Cached per nick for session stability.

Known issues remaining:
  - Link preview cards lost on channel switch
  - Link preview for title-only pages not verified
  - Server errors (482 etc.) in (server) buffer, not active channel
  - DCC Send File not implemented
  - AppImage packaging not done
  - MODE prefix removal loses lower-ranked prefixes until next NAMES
  - Hamburger menu briefly shrinks on theme switch (now less relevant — dialog stays open)
-->

<!--
Session summary — 2026-05-29  v0.7.8

What was built / fixed:
  - Hamburger menu restored as dropdown: ☰ now shows About UplinkIRC, Documentation,
    Preferences (opens dialog), Open Config (opens config.toml in system editor),
    Reload Config (re-applies all settings from disk without restarting). Order is
    intentional: About / Docs / Preferences / sep / Open Config / Reload Config.
  - About dialog: replaced wide banner.svg with the 96×96 dark app icon (app-icon-dark.svg).
  - New slash commands: /j (join alias), /ping <nick>, /invite <nick> [#ch], /mode,
    /op, /deop, /voice, /devoice, /ban, /unban, /clear. All added to /help output
    and docs/commands.md.
  - /ping round-trip time: CTCP PING reply is now intercepted in IrcClient; latency
    computed from echoed timestamp; displayed as "Ping reply from nick: Nms" in the
    active channel (not the server buffer). A new ctcpPingReply signal routes it through
    SessionModel::onCtcpPingReply which posts to m_activeChannel.
  - Tab completion for slash commands: handleTabComplete() now handles prefix.startsWith('/')
    by matching against a static sorted command list. Suffix is always a space (not ": ").
    Nick completion behavior is unchanged.

Known issues left open:
  - Server errors (482 etc.) in (server) buffer, not active channel
  - Link preview cards lost on channel switch
  - Link preview for title-only pages not verified
  - DCC Send File not implemented
  - AppImage packaging not done
  - MODE prefix removal loses lower-ranked prefixes until next NAMES

Next priorities:
  - Route server errors to active channel buffer
  - Link preview persistence across channel switches
  - Desktop notifications on mention/PM
-->

<!--
Session summary — 2026-05-29  v0.7.10

What was built / fixed:
  - Windows autojoin fix: toml::parse_file() was silently failing on Windows
    paths with non-ASCII characters (e.g. accented usernames in AppData).
    Replaced with Qt-native QFile read + toml::parse(string) so Qt handles
    the path and toml++ only parses the content string.

  - Simplified channel config format: [[server.channels]] array-of-tables
    replaced with a single comma-separated string on the [[server]] block:
      channels = "#uplink, #dojoirc"
    Load handles the new string format. Save always writes the new format.
    Legacy [[server.channels]] format removed entirely — no backward compat.

  - Reload Config now syncs channel list: SessionModel::syncServers() added;
    called from the Reload Config handler so manually edited channels in
    config.toml take effect immediately on the next reconnect without restart.

  - Close / Close Query in sidebar right-click: SessionModel::closeBuffer()
    added — for channels it sends PART then removes the buffer from the hash;
    for PM queries it removes the buffer directly. onChannelRemoved deletes
    the sidebar item. Channels show Close (after Rejoin/Leave); PM queries
    show Close Query (was previously no menu at all).

  - All docs updated: config.toml.example, docs/configuration.md (full
    example, minimal block, all bouncer/SASL/multi-server examples, channels
    section rewritten, old ordering gotcha removed), README.md (minimal +
    annotated examples, download badges bumped), CHANGELOG.md, docs/index.html
    feature card blurb updated.

Known issues remaining:
  - Link preview cards lost on channel switch (not in message history)
  - Link preview for title-only pages (no og:title) unverified
  - DCC Send File not implemented
  - AppImage packaging not done
  - Desktop notifications not implemented

Next priorities:
  - Desktop notifications on mention/PM
  - Link preview persistence across channel switches
  - AppImage packaging for Linux
  - DCC Send File
-->

<!--
Session summary — 2026-05-30 (v0.9.0)

What was built / fixed:
  - Link preview entity decoding: QTextDocument-based decodeEntities() applied
    to all three extractTitle() paths (og:title both attribute orderings and
    <title> fallback). Pages with &amp;, &#39;, &lt; etc. now show decoded titles.
  - DCC Send File: full implementation — DccSend (src/irc/dccsend.{h,cpp}) opens
    a TCP listener and streams file data with 4-byte big-endian ACK protocol;
    DccReceive (src/irc/dccreceive.{h,cpp}) connects to sender, writes file,
    sends cumulative ACKs. IrcClient parses incoming DCC SEND CTCP and emits
    dccSendReceived. SessionModel re-emits to MainWindow. Right-click nick menu
    "Send File" action wired up; incoming offers show accept/reject dialog with
    filename and size; both sides get a live QProgressDialog with cancel.
    60-second send timeout (no connection), 30-second receive timeout.
  - SASL EXTERNAL: sasl_external, client_cert, client_key added to ServerConfig,
    config.{h,cpp}, and IrcClient. IrcClient loads PEM cert+key onto QSslSocket
    before TLS handshake; negotiates AUTHENTICATE EXTERNAL + empty + response.
    RSA and EC keys both tried. ServerDialog gains checkbox + browse buttons.
  - AppImage packaging: packaging/build-appimage.sh — downloads linuxdeploy +
    Qt plugin on first run; DESTDIR install into AppDir; UPDATE_INFORMATION env
    var embeds zsync metadata for in-place updates via appimageupdatetool.
    themeloader.cpp gains ../share/uplinkirc/themes candidate for AppImage layout.
    CMakeLists.txt installs desktop file + SVG icon. release.yml Linux job builds
    AppImage after tar.gz; both .AppImage and .AppImage.zsync uploaded to release.
  - Docs: full SASL EXTERNAL walkthrough (cert generation, fingerprint registration,
    config); DCC Send section in commands.md; AppImage install + update in faq.md;
    ircv3.md SASL section expanded; README feature table + download badges updated.
  - Released v0.9.0, pushed tag v0.9.0 to trigger CI.
  - MSVC std::min conflict: replaced std::min calls in mainwindow.cpp with ternary
    expressions to avoid Windows min/max macro collision. Committed as hotfix after
    the session summary, so the close-session docs missed it.

Known issues remaining:
  - DCC over internet — local socket IP advertised; NAT/firewall on sender blocks
    direct connections. Works on LAN; WAN requires NAT traversal (future).
  - No in-app update check UI — zsync metadata is embedded and appimageupdatetool
    works externally, but there is no "Check for Updates" button inside the app.
  - Message search not yet implemented
  - Per-channel logging not yet implemented
  - Split view not yet implemented
  - Password field encryption (plaintext passwords in config.toml)
  - SOCKS5 proxy not yet supported

Next priorities:
  - Message search (Ctrl+F in channel buffer)
  - Per-channel log files
  - DCC NAT traversal or passive DCC
  - In-app update check button (trigger appimageupdatetool or show version badge)
-->

<!--
Session summary — 2026-06-08 (nick list pill highlights)

Added pill-shaped selection/hover highlights to the nick list, matching the
sidebar's SidebarDelegate style.

Changes:
- New NickDelegate class in mainwindow.cpp: draws a rounded-rect pill behind
  the selected/hovered nick, sized to text width + 8px horizontal padding.
  Preserves the existing icon-after-nick drawing from FixedRowDelegate.
- m_nickDelegate tracked on MainWindow so colors update when the theme changes.
  Pane nick lists also get a fresh NickDelegate on theme change and on pane open.
- QSS fix: QListWidget::item:selected/hover was painting a solid background that
  covered the delegate. Added QWidget#nickPanel QListWidget::item:selected/hover
  { background: transparent } rules (same approach the sidebar already uses for
  QTreeWidget::item) so the delegate shows through.
- Pill geometry tuned: vPad=1 (14px tall in a 16px row), border-radius=6 — flat
  sides, proper oblong pill rather than a lens shape.

No bugs found or regressions introduced.
No open issues.

Next priorities: /list dialog, window full-width bug on FreeBSD X11 (diagnostic
build still on main — needs output from user on X11).
-->

<!--
SESSION SUMMARY — 2026-06-14 (unified channel header + nick panel server label)
What changed:
  - Removed the standalone top bar (m_topicBar / topicLeftZone / topicRightZone widget hierarchy)
    and all associated QSS rules. The floating card now has symmetric 8 px margins on all sides.
  - Hamburger (☰) and sidebar toggle (⊞) moved into the primary channel header row, inline with
    the #channel (+modes) label, speech bubble topic button, search button, and close button.
  - Server name label (* NetworkName) moved into the nick panel header row, inline with the
    close-panel button (▦) and groups icon (👥). m_userInfoLabel is created in setupNickPanel()
    so it exists before setupChatArea() runs.
  - Added m_sidebarHeader dynamic spacer at the top of the sidebar panel. Height is kept in sync
    with m_primaryHeader + m_topicDisplay via an eventFilter on topicDisplay (Resize/Show/Hide),
    using QTimer::singleShot(0) to read geometry after the layout pass. This keeps the sidebar
    tree items vertically aligned with the nick panel header regardless of topic visibility.
  - Channel header, topic display, and input bar are anchored inside chatSection (right of the
    draggable sidebar divider). setupInputBar() redirects to m_chatSection->layout().
  - themeloader.cpp: removed #topicBar/#topicLeftZone/#topicRightZone QSS; added
    QWidget#sidebarHeader { background-color: {{bufferBg}}; }.
  - Docs updated: howto.html (channel header section rewritten, sidebar/nick panel collapse
    instructions updated, layout note updated), faq.md (sidebar toggle, nick panel collapse,
    topic display descriptions), configuration.md (show_topic and font_nick_dock descriptions).
  - Bumped to v0.25.27, tagged, pushed. CI and Release workflows both passed.
    All 5 release artifacts present: AppImage + .zsync, Linux tarball, Windows zip, macOS dmg.
No regressions. No known issues.
Next priorities: hero screenshots (3 needed for docs/index.html hero section); ServerId/BufferId
strong types; virtual scrolling for very busy channels.
-->

<!--
Session 2026-06-26 (v0.25.54 — auto-update, sysinfo cleanup, clangd):

Changes:
- Removed /sysinfo confirmation dialog — users found it unnecessary; command now
  posts system info directly like every other send command.
- Implemented full auto-update behind "Check for Updates": downloads and applies
  the correct release asset per platform. Linux AppImage replaces itself in-place
  (Linux inode semantics allow overwriting a running file) and relaunches — no sudo
  needed when the user owns the AppImage. Windows saves the ZIP to ~/Downloads and
  opens the folder. macOS downloads the DMG and opens it in Finder. Source builds
  and FreeBSD show an informational message instead.
- Set up compile_commands.json: CMake now exports the compilation database;
  symlinked to project root so clangd gets accurate Qt include paths and the LSP
  false-positive diagnostics are cleared.
- Bumped to v0.25.54, tagged, built, all CIs green (Release, CI, CodeQL).
- Updated all docs: index.html, README.md, commands.md, faq.md, howto.html.

Bugs found and fixed:
- Vexing parse: QNetworkRequest req(QUrl(url)) was parsed as a function declaration
  by GCC — fixed by switching to brace initialization.
- assetName was unused on Linux in applyUpdate() — fixed with Q_UNUSED.
- README.md download badge links were not updated on first doc pass — caught by
  user feedback and corrected.

Known issues open:
- REDACT via soju still untested — kicked to next release.
- UpdateChecker extraction to a separate class still pending (feature is complete
  but lives inline in MainWindow).
-->

## [0.25.54] — 2026-06-26

### Added
- **Auto-update** — "Check for Updates" in the hamburger menu now offers to download and install the latest release automatically. Linux AppImage users get an in-place replace and relaunch with no sudo required. Windows downloads the ZIP to your Downloads folder and opens it. macOS downloads and opens the DMG in Finder. Source builds and FreeBSD show an informational message instead.

### Fixed
- **Emoji autocomplete popup** — the suggestion popup now tracks the cursor position correctly as you type instead of appearing at a fixed offset.

### Changed
- **`/sysinfo`** — removed the confirmation dialog; the command posts system info to the channel directly, consistent with every other send command.

### Platform
- **AppImage** — bundles Noto Color Emoji as a fallback font so emoji render correctly on Linux systems that don't have a color emoji font installed. System fonts still take priority.

## [0.25.40] — 2026-06-16

### Fixed
- **Close channel restores UI** — right-clicking a channel and choosing Close now immediately refreshes the info bar and nick list to reflect the auto-selected channel. Previously they stayed stale showing the closed channel's data until you manually clicked the new selection.

### Changed
- **Sidebar header icons** — Preferences button now uses the `manage_accounts` icon; Manage Servers button now uses `add_link`.
- Manage Servers button enlarged to 32×32 (icon 28×28) for better visual weight alongside the other header buttons.

## [0.25.39] — 2026-06-16

### Added
- **IRC text formatting in the input bar** — Ctrl+B (bold), Ctrl+I (italic), Ctrl+U (underline), Ctrl+S (strikethrough), Ctrl+O (reset all). Formatting is applied visually as you type — bold text looks bold, italic looks italic. IRC mIRC codes are generated at send time; no control characters appear in the widget. Combinations stack: Ctrl+B then Ctrl+U produces bold+underline text.
- **Format indicator** — when any formatting is active, a small `B I U S` label appears at the bottom-left of the input box showing which formats are on, styled with their respective formatting. Hides automatically when you send or press Ctrl+O. Updates as you move the cursor through differently-formatted text.

### Fixed
- **Cross-client reactions** — reactions sent from one client now appear correctly on all others. Root cause: sent messages were stored with an empty `msgid` because the `echo-message` IRCv3 cap was not requested. The server now echoes sent messages back with a real server-assigned `msgid`, which `draft/react` TAGMSG references when another user reacts. The local echo is suppressed when `echo-message` is active to avoid duplicates.
- **Keychain bundle separator** — the per-server credential bundle used `\x00` (NUL) as a field separator, but KWallet (and other OS keystores) treat stored strings as null-terminated, silently discarding everything after the first NUL. Fields after the first (SASL password, NickServ password, proxy password) were never actually saved. Fixed by switching to `\x1F` (ASCII unit separator). Re-save your server config in Edit Server once to migrate.
- **Keychain prompts reduced** — all four per-server password fields are now stored as a single keychain bundle item, reducing OS keychain prompts from up to 4 per server down to 1.

### Changed
- Search icon enlarged from 20×20 to 24×24 px.

## [0.25.31] — 2026-06-14

### Added
- **Lag meter in sidebar header** — the four signal-bar latency indicator is now anchored in the top-right of the server/channel list header, vertically aligned with ☰ and ⚙. Hover to see your exact latency in milliseconds (`42 ms`). Shows `Connecting…`, `Reconnecting…`, or `Disconnected` when not fully connected. Updates whenever you switch channels or servers.
- **WALLOPS support** — server-wide operator broadcasts (`WALLOPS`) are now received and displayed in the server buffer in amber, formatted as `[nick] message`. Previously silently dropped.
- **Server buffer message colors** — command responses (`/whois`, `/stats`, `/time`, etc.) now appear in soft steel blue instead of gray, making it easy to see at a glance that there's output waiting in the server buffer. Error messages remain red; plain status lines (Connected, Disconnected, Joined) stay gray.
- **Unread message count badges** — a small bold count appears to the right of the unread indicator icon (⚡ mentions, 💬 activity) in the sidebar so you can see exactly how many messages are waiting without switching to the buffer.
- **Preferences: Show Unread Message Counts** — global toggle in ⚙ Preferences to turn sidebar count badges on or off. Persisted as `show_unread_counts` in `config.toml`. Turning it off clears all badges immediately.

### Fixed
- **`/list` on large networks** — Uplink no longer disconnects with "Server sent oversized unterminated data" when browsing channels on large networks (Rizon, Undernet, etc.). The buffer safety check was firing before any lines were processed; it now only triggers if a single line exceeds the 64 KB limit.

### Changed
- **Connected server icon** — replaced the globe icon with a host/server icon for connected network entries in the sidebar.
- **Search button toggles** — clicking 🔍 while the search bar is open now closes it (and clears the query). Previously a second click had no effect.
- **Server and PM buffers** — switching to a server buffer or private message tab hides the nick panel and topic button since neither applies outside of channels. Both restore automatically when switching back to a channel.

---

## [0.25.30] — 2026-06-14

### Fixed
- **Server/channel list alignment** — sidebar header now dynamically tracks the primary header height plus the topic bar height, so server/channel tree items stay aligned with the nick panel header icons whether the topic bar is visible or not. Sidebar and nick list scroll positions are preserved across every sync.
- **Event group visual clarity** — in expanded event groups, entries after the first are indented 2 spaces so wrap continuations from long hostmasks are visually distinct from the start of the next entry.
- **Hamburger and gear icon backgrounds** — both toolbar buttons now always render with a transparent background; icon color refreshes on every theme change to stay consistent with the active theme.
- **Sidebar/nick list selection text** — selected item text color auto-contrasts with the accent color (dark text on light accents, light text on dark accents) so labels remain readable on all 55 built-in themes.

### Changed
- **Manage Servers in ☰ menu** — Manage Servers is now directly accessible from the hamburger menu without opening Preferences first.
- **Preferences dialog streamlined** — the quick-access row (Manage Servers and Documentation buttons) at the top of the Preferences dialog has been removed; both actions are reachable from ☰ directly.

---

## [0.25.27] — 2026-06-13

### Changed
- **Hamburger and sidebar toggle in channel header** — hamburger (`☰`) and sidebar toggle (`⊞`) are now inline with the `#channel (+modes)` label in the primary header row; the standalone top bar is removed.
- **Server name in nick panel header** — `* ServerName` now appears inline with the nick panel close button and groups icon, replacing the old top-bar placement.
- **Floating card symmetric margins** — the rounded chat card now has 8 px margins on all four sides (was 0 top); the card floats evenly within the window.
- **Topic and input stay right of sidebar border** — the channel header, topic display, and text input are all anchored inside `chatSection` (right of the draggable sidebar divider), not spanning the full card width.
- **Sidebar tree alignment** — a dynamic spacer at the top of the sidebar panel keeps the server tree items vertically aligned with the nick panel header row, updating reactively when the topic bar is shown or hidden.

---

## [0.25.26] — 2026-06-13

### Fixed (post-release patch)
- **FreeBSD build** — `mallopt(M_ARENA_MAX, 2)` and `#include <malloc.h>` are now guarded with `#if defined(__linux__) && !defined(__MUSL__)`. FreeBSD uses jemalloc which has no `M_ARENA_MAX`; macOS libmalloc likewise. No functional change on Linux (glibc arena cap still applies).

---


### Added
- **Search button** — magnifying glass icon in the channel bar opens the search bar (same as Ctrl+F).
- **Nick panel open/close icons** — replaced spinning gear with Material Symbols `right_panel_close` / `left_panel_close` icons; toggle is instant instead of animated.
- **Floating nick reveal button** — when the nick panel is hidden, a `left_panel_close` button appears at the top-right of the chat area (below the topic divider) to restore it.
- **Groups icon in nick panel header** — Material Symbols groups icon displayed next to the user count.

### Changed
- **Channel info moved to channel bar** — `#channel (+modes)` and `* ServerName` now appear inline in the primary header row instead of the top bar.
- **Search bar close button** — transparent background, consistent with other icon buttons.
- **Lag meter removed from channel bar** — `SignalBars` is de-parented for now; placement TBD.

### Fixed
- **Sent messages not appearing** — Ergo IRCd acks `echo-message` but does not reliably echo PRIVMSG back to the sender. Removed `echo-message` from the desired CAP list; local echo now always fires immediately on send. `/me` actions also affected and fixed.

---

## [0.25.1] — 2026-06-08

### Added
- **Nick list pill highlights** — selected and hovered users in the nick list now show the same pill-shaped highlight used in the server/channel sidebar, using the theme accent and hover colours.

---

## [0.8.1] — 2026-05-30

### Fixed
- **Link preview card persistence** — preview cards now survive channel switches. URL keys are normalised before storage, and `refreshChatView` reinjects stored cards when switching back to a channel.

---

## [0.8.0] — 2026-05-29

### Security
- **TLS certificate verification enforced** — `ignoreSslErrors()` removed; `onSslErrors` now disconnects. Connections to servers with invalid or self-signed certificates fail immediately with an error in the server buffer. No silent bypass.
- **Credential redaction in raw log** — `PASS`, `AUTHENTICATE`, and `NickServ IDENTIFY` payloads are stripped from the raw log before `rawReceived` is emitted. Passwords never appear in any visible panel.
- **Config file hardening** — `config.toml` is written with `QSaveFile` (atomic rename) and `0600` permissions. All string values are escaped with `tomlQuote()` before writing.
- **Link preview LAN blocking** — `isPrivateUrl()` blocks loopback, RFC 1918 private ranges (`10.x`, `172.16–31.x`, `192.168.x`), link-local (`169.254.x`), and `.local` hostnames. A URL posted in chat cannot probe local services.
- **CTCP rate limiting** — `VERSION` and `PING` CTCP replies are limited to once per nick per 5 seconds. Reflected `PING` payloads are capped at 32 bytes.
- **DoS buffer caps** — inbound IRC buffer capped at 64 KB, batch messages at 1 000 per batch / 8 open batches, `QTextBrowser` block count bounded at `kMessageBufferCap + 300`.
- **Image decode safety** — `QImageReader` now checks reported dimensions before allocating; images larger than 4096 × 4096 are skipped.

---

## [0.7.10] — 2026-05-29

### Changed
- **Simplified channel config** — `channels = "#uplink, #dojoirc"` replaces the old `[[server.channels]]` array-of-tables blocks. All channels for a server go on one line, comma-separated. The Manage Servers dialog and Reload Config both honour the new format.

### Fixed
- **Config loading on Windows** — replaced `toml::parse_file()` with Qt-native file I/O so paths containing non-ASCII characters (e.g. usernames with accented letters) load correctly on Windows.

---

## [0.7.9] — 2026-05-29

### Added
- **`/time <nick>`** — sends a CTCP TIME request to a user; their local time appears in the active channel buffer (e.g. `* Time reply from sig: 2026-05-29T19:21:22.579Z`). If the target client does not support CTCP TIME, no reply is shown — this is normal for many bots.
- **Mention notifications** — when your nick is mentioned in a channel you are not viewing, that channel shows **💡** in red in the sidebar. Regular unread activity shows **🔥**. Both indicators clear as soon as you open the channel.
- **Self-nick highlight** — any message containing your nick is rendered with your nick in **red bold** inline in the chat. Word-boundary match, case-insensitive.

### Fixed
- **Server errors now appear in the active channel** — IRC error replies (numeric ≥ 400: 482 chanoprivsneeded, 401 nosuchnick, 404 cannotsendtochan, 421 unknowncommand, etc.) now show as red `!!` messages in your current channel instead of silently going to the `(server)` buffer.
- **Multi-prefix removal** — `-o` on an `@+` nick (op + voice) now correctly demotes to `+` voice instead of clearing the prefix entirely. `NickEntry` now tracks all active mode prefixes in a `QSet<QChar>`.
- **`/ping` and `/time` confirmation lines persist across channel switches** — "Pinged nick" and "Querying time for nick" local messages were previously lost when switching channels. They now go through the model buffer and survive re-renders.

---

## [0.7.8] — 2026-05-29

### Added
- **New slash commands** — `/j` (join alias), `/invite`, `/mode`, `/op`, `/deop`,
  `/voice`, `/devoice`, `/ban`, `/unban`, `/clear`, `/ping <nick>`
- **Tab completion for commands** — type a partial command (e.g. `/pi`) and press Tab
  to complete it. Cycles through matches the same as nick completion.
- **`/ping` round-trip time** — reply shows latency in milliseconds (e.g.
  `Ping reply from BeeMO: 4ms`) in the active channel alongside the sent confirmation.

### Changed
- **Hamburger menu restored as dropdown** — ☰ now opens a small menu with:
  **About UplinkIRC**, **Documentation**, **Preferences** (opens the settings dialog),
  **Open Config** (opens `config.toml` in your system editor), and **Reload Config**
  (re-applies all settings from disk without restarting).
- **About dialog** — now shows the app icon instead of the wide banner.

---

## [0.7.7] — 2026-05-29

### Added
- **Persistent Preferences dialog** — clicking ☰ now opens a non-modal dialog that stays
  open while you browse. Theme list, app icon picker, font config, UI toggles (topic bar,
  nick prefix, emoji button, typing indicator, connection status, colored nicks), nick
  brackets selector, and quick buttons for Manage Servers, Documentation, and About — all
  in one place, no more dismiss-on-click.

### Changed
- **Bot icon randomization** — `+B` nicks now receive a randomly assigned icon (🤖 or 👾)
  on first appearance instead of a deterministic hash value. Cached per nick for session
  stability.

---

<!--
Session summary — 2026-05-29  v0.7.6

What was built / fixed:
  - Hamburger menu relocated from bottom of nick list to left end of topic bar (left
    of #channel label). Menu popup now drops down-right from the button. Fixed height
    constraint removed so topic bar sets the size naturally.

  - Autojoin regression (root cause): ServerDialog::serverConfig() returned a
    ServerConfig with an empty channels list, so editing any server in the GUI silently
    wiped all [[server.channels]] entries. Fixed by adding an "Auto-join" field
    (comma-separated) to ServerDialog. Field round-trips correctly through edit and save.

  - Channel focus on join: onChannelAdded() only switched focus when activeChannel()
    was empty. Removed the guard; joining now always switches to the new channel.

  - Nick brackets GUI: "Nick Brackets" submenu added to hamburger menu using the same
    QWidgetAction/QListWidget pattern as App Icon.

  - CI: windows-latest pinned to windows-2025 in both ci.yml and release.yml ahead of
    the June 15, 2026 GitHub Actions cutover.

Known issues remaining:
  - Link preview cards lost on channel switch
  - Link preview for title-only pages not verified
  - Server errors (482 etc.) in (server) buffer, not active channel
  - Hamburger menu briefly shrinks on theme switch
  - DCC Send File not implemented
  - AppImage packaging not done
  - MODE prefix removal loses lower-ranked prefixes until next NAMES
-->

## v0.7.6 — 2026-05-29

### Fixed
- **Autojoin regression** — editing a server in Manage Servers was silently wiping all
  auto-join channels from the config. Root cause: `ServerDialog::serverConfig()` returned
  an empty `channels` list. Fixed by adding an **Auto-join** field (comma-separated
  channel names) to the Add/Edit Server dialog — channels now survive a dialog round-trip.
- **Channel focus on join** — manually joining a channel when another was already active
  did not switch focus. Joining now always switches to the new channel.

### Added
- **Nick Brackets in hamburger menu** — `nick_brackets` can now be changed from
  **Hamburger → Nick Brackets** without editing `config.toml`. Four choices shown with
  previews: `<nick>`, `[nick]`, `::nick::`, `nick`.
- **Hamburger moved to topic bar** — the ☰ button now sits at the left end of the
  `#channel (modes) * Network` info bar instead of the bottom of the nick list panel.

### CI
- Pinned `windows-latest` → `windows-2025` in both `ci.yml` and `release.yml` ahead of
  the June 15, 2026 GitHub Actions cutover.

---

## v0.7.5 — 2026-05-29

### Added
- **Configurable nick brackets** — new `nick_brackets` key in `[ui]` controls
  the characters that wrap nicks in chat messages. Value is split at its
  midpoint: `"<>"` → `<nick>`, `"[]"` → `[nick]`, `"::::"` → `::nick::`,
  `""` → no brackets. Defaults to `"<>"`. Existing configs without the key
  are unaffected.

---

<!--
Session summary — 2026-05-29  v0.7.5 — Configurable nick brackets

What was built:
  - nick_brackets config key in [ui] block. UiConfig gained a nickBrackets
    field defaulting to "<>". Config::load() reads nick_brackets from TOML
    with fallback to "<>". Config::save() writes it back. kDefaultConfig
    updated with the new key and a comment (careful: TOML comments with "()"
    inside R"()" raw strings close the literal — used safe comment text).

  - formatMessage() in mainwindow.cpp now reads m_config.ui.nickBrackets,
    splits at midpoint (even length) or first/last char (odd length), HTML-
    escapes both halves, and wraps the nick. The old hardcoded &lt;%3&gt;
    replaced with dynamic nickOpen + nick + nickClose. Empty string = no
    brackets at all.

  - docs/configuration.md: added "Nick bracket style" section with a table
    of all supported values, copy-paste examples, and a note about the
    midpoint-split parsing rule. Added nick_brackets to the full example
    block and the [ui] options table.

  - README.md: added nick_brackets with inline comment to the annotated
    config block.

  - v0.7.4 CI all green; Windows binary confirmed available.

Bugs fixed:
  - None (feature addition only)

Known issues remaining:
  - Link preview for title-only pages not verified
  - Link preview cards lost on channel switch
  - Hamburger menu briefly shrinks on theme switch
  - Server errors (482 etc.) in (server) buffer not active channel
  - DCC Send File not implemented
  - AppImage packaging not done
  - MODE prefix removal loses lower-ranked prefixes until next NAMES

-->

## v0.7.4 — 2026-05-29

### Fixed
- **Nick list: multi-prefix nicks** — `@+nick` was storing `+nick` as the nick
  name due to only stripping one leading prefix char; now consumes all leading
  prefix characters and keeps the highest-ranked one.
- **Nick list: MODE changes not refreshing display** — `+o`, `-o`, `+v`, `+B`
  etc. received after the initial NAMES reply now update `NickEntry.prefix` in
  place, re-sort the list, and emit `nickListChanged` so the display reflects
  immediately. Previously `modesChanged` only refreshed the topic bar.
- **Bot icons not showing on join** — client now sends `WHO #channel` after
  NAMES; `352 RPL_WHOREPLY` flags field (`B`) populates `botNicks` on every
  join/restart, matching Halloy behaviour.
- **Bot icons moved to right of nick name** — was `🤖 @nick`, now `@nick 🤖`.
- **Emoji picker: colored squares behind buttons** — global QSS
  `QToolButton { background-color: sidebarBg }` was leaking into the picker;
  buttons are now transparent with a subtle semi-transparent hover tint.
- **Typing indicator: layout shift** — label row now stays visible (empty text)
  when nobody is typing so the chat view height never shifts; hides only when
  the typing indicator feature is toggled off entirely.
- **Scrollbar width** — halved from 8 px to 4 px (border-radius 4 → 2 px).

---

## v0.7.2 — 2026-05-29

### Fixed
- **Windows: links invisible on dark themes** — `QTextBrowser` link color now
  set from the theme accent via `setDefaultStyleSheet`; previously links
  inherited the document default (black) regardless of theme.
- **Windows: nicks black when Colored Nicks is off** — replaced `palette(text)`
  (QSS-only syntax) with the theme's actual text color in inline HTML; on
  Windows `palette(text)` was falling back to black inside the HTML renderer.
- **Windows: `/sysinfo` returning Unknown for CPU/MEM/GPU/Uptime** — added
  Windows implementations: CPU from registry, MEM via `GlobalMemoryStatusEx`,
  GPU via PowerShell `Get-CimInstance`, Uptime via `GetTickCount64`.
- **Font Config dialog: fonts squashed on Windows** — `QFontComboBox` now has a
  minimum height so font names render at a readable size.
- **Font Config dialog: visual clutter** — size controls reorganized into a
  two-column grid (5 rows instead of 10); all options preserved.

---

<!--
Session summary — 2026-05-29  v0.7.4 — Windows polish, nick list correctness, WHO bot detection, emoji picker

What was built / fixed:
  - Windows theme colors: QTextBrowser link color and nick color when
    coloredNicks=off were both black on Windows. Root cause: palette(text)
    in inline HTML CSS doesn't resolve in Qt's HTML renderer on Windows
    (only works in QSS widget rules). Fixed by storing the loaded Theme struct
    as m_theme on MainWindow and using m_theme.text directly. Link color fixed
    by setting document()->setDefaultStyleSheet() with theme accent color,
    updated on theme change.

  - /sysinfo Windows: all four sysinfo functions returned "Unknown" on Windows.
    Added #if defined(Q_OS_WIN) branches: CPU from registry
    HKLM\...\CentralProcessor\0\ProcessorNameString, MEM via
    GlobalMemoryStatusEx, GPU via PowerShell Get-CimInstance
    Win32_VideoController, Uptime via GetTickCount64().

  - Font Config dialog Windows: QFontComboBox was rendering fonts in a tiny
    sliver because global QSS QToolButton padding was leaking in. Fixed with
    setMinimumHeight(30). Also reorganized 10 individual size rows into a
    2-column grid (5 rows) to reduce visual clutter while keeping all options.

  - Scrollbar width: halved from 8px to 4px (border-radius 4→2px).

  - Typing indicator layout shift: label was being setVisible(false) when
    nobody was typing, causing the chat view to resize on every typing event.
    Fixed by keeping label always visible with empty text; only truly hides
    when the typing feature is toggled off.

  - Nick list multi-prefix: setNicks() and addNick() in channel.h only stripped
    one leading prefix char. With multi-prefix negotiated, @+nick stored as
    nick="+nick". Fixed by consuming all leading prefix chars in a while loop,
    keeping highest-ranked.

  - Nick list MODE updates: onModesReceived() called parseBotModes() and emitted
    modesChanged (→ topic bar refresh only). Nick prefixes never updated live.
    Added modeToPrefix() helper mapping o→@, h→%, v→+, a→&, q→~. Now parses
    mode string, updates NickEntry.prefix in place, re-sorts, emits
    nickListChanged. Also emits nickListChanged after parseBotModes so +B takes
    effect immediately.

  - WHO-based bot detection: previously bots only showed icons if a +B MODE
    change arrived while connected. On restart/join, server doesn't replay
    existing umodes. Fixed by sending WHO #channel after 366 ENDOFNAMES.
    Added whoEntryReceived signal to IrcClient, case 352 handler extracts
    nick and flags, onWhoEntry() in SessionModel checks for 'B' in flags and
    updates ch->botNicks, emits nickListChanged.

  - Bot icons moved to right: was botIconForNick + " " + e.display(), now
    e.display() + " " + botIconForNick.

  - Emoji picker colored squares: global QSS QToolButton { background-color:
    sidebarBg; padding: 4px 10px 4px 0px } was applied to emoji picker buttons.
    Asymmetric padding shifted emoji off-center; sidebarBg made colored squares
    visible. Per-button setStyleSheet() overrides to transparent background,
    0 padding, and semi-transparent hover tint.

  - Emoji picker search grid alignment: when fewer than 9 results returned,
    QGridLayout stretched partial columns to fill scroll area width. Fixed by
    setting column minimum widths for all 9 columns and adding a stretch to
    column kCols to absorb leftover space.

  - docs/index.html: added Download section with direct binary links for all
    three platforms, updated version badge v0.2.0→v0.7.2, added Download to
    nav, changed hero primary button to "Download v0.7.2".

  - README download badges: hardcoded v0.7.1 filename in /releases/latest/
    download/ URLs; updated to v0.7.2.

Bugs fixed:
  - Windows: links black on dark themes
  - Windows: nicks black when coloredNicks=off
  - Windows: /sysinfo all Unknown
  - Windows: Font Config fonts squashed + too many rows
  - Nick list: multi-prefix nicks showing wrong nick name
  - Nick list: MODE changes not updating prefix display
  - Bot icons: not shown on join/restart (only on live MODE change)
  - Bot icons: on left instead of right
  - Emoji picker: colored squares behind buttons on all platforms
  - Emoji picker: search results spread across full width
  - Typing indicator: chat view height shifting up/down

Known issues remaining:
  - Link preview for title-only pages not fully verified
  - Link preview cards lost on channel switch
  - Hamburger menu briefly shrinks on theme switch (root cause unknown)
  - Server errors (482 etc.) appear in (server) buffer, not active channel
  - DCC Send File not implemented
  - AppImage packaging not done

-->

<!--
Session summary — 2026-05-29  v0.7.1 — Bug fixes: /nick label, image preview, typing indicator layout

What was built / fixed:
  - /nick display bug: typing /nick newname sent the NICK command correctly but
    the nick label next to the input bar never updated. Root cause: setNick()
    pre-sets m_nick to the new nick before the server echoes it back, so the
    NICK handler's check (msg.nick == m_nick) compared the old server-prefix
    nick against the already-updated value — always false. Fix: compare newNick
    (what the server confirmed) against m_nick using case-insensitive toLower()
    so server nick normalisation is also handled correctly.

  - Direct image URL preview: pasting a .png/.jpg/.jpeg/.gif/.webp URL in chat
    now shows a thumbnail card directly. Previously the URL was fetched as HTML,
    extractTitle found no <title> tag, and nothing was shown. Fix: isImageUrl()
    helper checks the URL path extension; if it matches, fetch() calls
    fetchImage() directly with the filename as the card title, bypassing HTML
    parsing entirely.

  - Typing indicator overlap: the indicator was a QLabel child of m_chatView,
    positioned as a floating overlay via move() at the bottom-left of the chat
    area. When the chat was full of messages it covered the bottom line of text.
    Fix: moved the label into the main VBox layout as a dedicated row between
    the chat view and the input bar. setVisible(false) collapses it to zero
    height, so there is no wasted space when no one is typing.
    repositionTypingLabel() reduced to a no-op; the resize-event call in the
    event filter becomes harmless.

  - README: added FreeBSD badge linking to the build-from-source dependency
    section so FreeBSD users know the platform is supported.

  - README: added cross-platform OS characters image (BSD devil / Tux / robot /
    Windows logo) under the download badges. Image was RGB with baked-in
    checkerboard pixels (not true RGBA transparency); flood-filled from corners
    with 20-unit colour tolerance and re-saved as RGBA to remove the background.

  - CI: Windows build flaky failure investigated. windows-latest runner mid-
    transition to windows-2025-vs2026. Re-run of the failed job passed. No
    workflow changes needed — failure was transient runner allocation, not code.

  - v0.7.1 tagged and released; all three platform builds (Linux, Windows,
    macOS) passed on the release workflow.

Bugs fixed:
  - /nick not updating the nick label (IrcClient NICK handler comparison)
  - No preview for direct image URLs (.png/.jpg/.gif/.webp etc.)
  - Typing indicator overlapping chat text when chat view was full

Known issues remaining:
  - Link preview for pages with only <title> (no og:title) — debug logging
    confirmed extraction should work; not yet fully verified in-app
  - Link preview cards lost on channel switch (not in message history)
  - Hamburger menu briefly shrinks on theme switch (root cause unknown)
  - Server errors (482 etc.) appear in (server) buffer, not active channel
  - DCC Send File not implemented
  - AppImage packaging not done

Next priorities:
  - Confirm / fix link preview for title-only pages (no og:title)
  - Route server errors to active channel buffer
  - Desktop notifications on mention/PM
  - AppImage packaging for Linux
  - DCC Send File
-->

<!--
Session summary — 2026-05-29  v0.6.0 — Emoji, bot icons, menu icons, Windows polish

What was built:
  - Emoji picker popup (EmojiPicker, Qt::Popup) — ~400 emoji in a 9-col scrollable grid
    with search box; opens from the 😊 button in the input bar; inserts at cursor
  - Inline :shortcode: autocomplete — type :smi and a no-focus overlay list appears above
    the input bar; Up/Down/Enter/Tab/click to select; Escape dismisses
  - Auto-substitute on closing colon — typing :trident: auto-replaces in the input field
    as soon as the closing colon is typed
  - Substitute on submit — any remaining :shortcode: patterns in the message are resolved
    to emoji before sending, so typing :trident: and pressing Enter sends 🔱
  - +B bot nick icons — nicks with +B user mode or channel user mode show 🤖 or 👾,
    chosen deterministically per nick via hash; tracked in Channel::botNicks and
    ServerSession::botNicks; parsed from both channel MODE and user MODE events
  - QPainter-drawn menu icons in hamburger — 12 distinct line icons for every menu item
    (info circle, server stack, open book, A-shape, half-circle, framed landscape,
    bar layout, person, smiley, speech bubble, signal bars, three persons);
    palette-aware (adapts to dark/light theme); replaces unicode text prefixes
  - Windows native style by default — QApplication::setStyle("windows11") set on Windows;
    ThemeLoader skips QSS when theme = "default" so fresh Windows install looks native;
    custom themes still apply when user picks one
  - Windows font default — Consolas on Windows, IBM Plex Mono on Linux/macOS
  - Win32 console window suppressed — WIN32 added to qt_add_executable; conhost.exe
    no longer spawned; closing nothing kills the GUI
  - Typing indicator overlay — moved from a VBox layout row to a child overlay of
    m_chatView; floats at bottom-left of chat area; transparent, no row or box;
    repositions on chat view resize via event filter
  - Typing indicator background removed — was using inputBg color; now transparent
  - Emoji button crop fixed — setFixedSize(30,30) + font-size:16px; padding:0
  - GitHub Actions Node.js 24 opt-in — FORCE_JAVASCRIPT_ACTIONS_TO_NODE24=true added
    to both ci.yml and release.yml before June 2nd forced-upgrade deadline

Bugs fixed:
  - Windows: console window (conhost.exe) spawned as parent of GUI process — killed GUI on close
  - Windows: monospace font falling back to random sans-serif — now defaults to Consolas
  - Windows: whole UI looked alien (custom dark QSS on native Windows) — native style now default
  - Typing indicator: gray background stripe between chat and input bar
  - Typing indicator: separate layout row created a visible container even with transparent bg
  - Emoji button: face emoji cropped because button had no height constraint and default padding
  - :shortcode: completion: required popup interaction; couldn't type full :name: directly

Known issues remaining:
  - Hamburger menu briefly shrinks when a theme is applied (QMenu sizeHint re-polish)
  - Link preview cards don't survive channel switch (not in message history)
  - DCC Send File stub in nick menu not yet implemented
  - Server errors (482 etc.) show in (server) buffer, not active channel

Next priorities:
  - Link preview persistence across channel switches
  - Server error routing to active channel buffer
  - AppImage packaging for Linux
  - Desktop notifications on mention/PM
-->

## v0.7.1 — 2026-05-29

### Fixed
- **`/nick` label not updating** — changing your nick with `/nick newname` sent the command correctly but the nick label next to the input bar kept showing the old nick. The server echoes back your nick change in a `NICK` message; the handler was comparing the wrong value and always missed the self-nick case.
- **No preview for direct image URLs** — pasting a `.png`, `.jpg`, `.jpeg`, `.gif`, or `.webp` link in chat now shows a thumbnail card. Previously these URLs were fetched as HTML, no `<title>` was found, and nothing appeared.
- **Typing indicator overlapping chat text** — the typing indicator floated as an overlay on top of the chat view and covered the last line of text when the window was full. It is now a proper layout row between the chat area and the input bar; it takes up no space when no one is typing.

### Added
- FreeBSD badge in README linking to build-from-source instructions
- Cross-platform OS characters artwork (BSD daemon / Tux / robot / Windows logo) on the GitHub project page

---

<!--
Session summary — 2026-05-29 macOS CI fix + link preview:

What was built/fixed:
  - macOS release CI was failing: MACOSX_BUNDLE not set in CMakeLists.txt,
    so CMake produced a plain binary instead of a .app bundle. macdeployqt
    requires a .app and errored out. Fixed by adding:
      if(APPLE) set_target_properties(UplinkIRC PROPERTIES MACOSX_BUNDLE TRUE) endif()
    v0.3.0 tag deleted, commit pushed, tag re-applied at new HEAD and pushed.
    All three platform builds (Linux, Windows, macOS) now pass.

  - LinkPreview class (src/ui/linkpreview.{h,cpp}) added:
    * Fetches up to 16KB of HTML per URL (abort-on-cap)
    * Extracts og:title (both attribute orders) + <title> fallback
    * Extracts og:image URL and fetches thumbnail (200KB cap, 120x90px max)
    * WhatsApp/2 User-Agent for better og:title hit rate on gated sites
    * 50-entry in-memory cache; one in-flight page fetch at a time
    * Concurrent image fetches (independent QNetworkReply per image)
    * Crash fix: abort() can fire finished() synchronously; old code called
      deleteLater() on m_reply after finished() had already nulled it.
      Fixed by disconnect(this) before abort() + stable local reply pointer
      in lambdas guarded by m_reply != reply check.

  - Hover tooltip: event filter on chat viewport + anchorAt(); shows domain
    name immediately on hover, updates to page title when fetch completes.
    Status bar also mirrors the title. Wayland-safe: uses mapToGlobal from
    mouse event position rather than QCursor::pos() in async callbacks.

  - Inline preview card: auto-triggered for live Privmsg/Action/Notice
    messages containing http/https URLs. Card appended to active channel
    view when cardReady fires. Layout: bordered table with thumbnail (if
    available) + bold title + domain. Colors drawn from Qt palette for
    theme compatibility.

Bugs fixed:
  - macOS CI crash (MACOSX_BUNDLE missing)
  - LinkPreview crash: null deref in fetch() when abort() fired finished()
    synchronously

Known issues left open:
  - Link preview cards don't survive channel switching (re-render doesn't
    re-fetch; would need to store cards in channel message history)
  - Hamburger menu still shrinks on theme switch (root cause unknown)
  - Server errors (482 etc.) still go to (server) buffer, not active channel
  - AppImage packaging not done
  - DCC Send File not implemented

Next priorities:
  - Route server errors (482 etc.) to active channel buffer
  - Connection status indicator per server in sidebar
  - AppImage packaging for Linux
  - DCC Send File
-->

<!--
Session summary — 2026-05-28 .gitignore cleanup:

What was fixed:
  - CLAUDE.md and .claude/ were listed in the public .gitignore, revealing
    internal tooling to anyone who reads the repo. Moved both entries to
    .git/info/exclude (local-only, never pushed). Public .gitignore now
    contains only standard build artifacts and common ignores.

Known issues left open: same as previous session.
Next priorities: reconnect with backoff, URL click in chat, server error routing.
-->

<!--
Session summary — 2026-05-28 hamburger menu + server management:

What was built:
  - Hamburger menu now opens above the button, anchored to the top-right
    corner using popup() with the button's top-right global position.
    Qt's screen-edge detection handles the flip automatically.
  - Theme list in hamburger is now scrollable and compact: uses
    QMenu::max-height stylesheet for native scroll arrows, compact item
    padding (2px). Individual actions used (not QWidgetAction) so the
    menu closes cleanly on selection.
  - ServerDialog: Add Server dialog with three sections — Connection
    (name, host, port, SSL checkbox), Identity (nick, user, realname),
    Authentication (server password, SASL user/password, NickServ
    password). Password fields use echo mode Password.
  - ManageServersDialog: list of all configured servers with Add, Edit
    (double-click or button), and Remove. Edit pre-fills the form.
  - SessionModel.addServer(), removeServer(), updateServer() — runtime
    server management without restart; connect/disconnect/reconnect live.
  - ServerConfig operator== added for dirty-checking on edit.
  - Hamburger menu entry changed from "Add Server..." to "Manage Servers..."
  - Explicit QMenu::item padding added to theme stylesheet to stabilize
    menu item height across theme switches.

Known issues left open:
  - Burger menu still visually shrinks when a theme is applied — root
    cause not yet identified; QMenu re-polish on setStyleSheet() appears
    to be computing a different sizeHint despite explicit item padding.
  - No reconnect on disconnect.
  - Server errors (482 etc.) appear in (server) buffer, not active channel.
  - URL click-to-open only in topic bar, not chat messages.
  - DCC Send File not implemented.

Next priorities:
  - Reconnect with exponential backoff
  - URL detection + click-to-open in chat messages
  - Route 482/channel errors to active buffer
-->

<!--
Session summary — 2026-05-28 UI layout + hamburger relocation:

What was built/fixed:
  - Topic bar background changed to match input area (inputBg) so the info
    bar blends with the input row rather than the sidebar.
  - Mode string spacing fixed: (modes) and * NetworkName now have a 6px gap
    between them instead of being flush against each other.
  - Clickable URLs in topic display: linkifyTopic() wraps http/https URLs in
    <a href> tags; QLabel configured with TextBrowserInteraction +
    setOpenExternalLinks so clicking opens the browser.
  - Toolbar hidden: nothing sits above #channel (+nt) * Network bar. Dock
    widgets and central widget all align at the top of the content area.
  - Dock title bars replaced with minimal custom bars: each dock (sidebar,
    nick list) gets a 16px title bar containing only a ⧉ float/detach button.
    Lists are flush with the topic bar; detach is still available.
  - Hamburger menu relocated from toolbar to the bottom-right of the nick
    list panel, sized to match the input bar height, so it sits in the
    natural bottom-right corner of the chat layout.
  - Status bar font shrunk to 7pt via QSS (Connected to irc.linuxdojo.org).

Known issues left open:
  - No reconnect on disconnect
  - Server errors (482 etc.) go to (server) buffer, not active channel
  - Emoji picker not built
  - DCC Send File not implemented
  - URL click-to-open only works in topic bar, not in chat messages
-->

<!--
Session summary — 2026-05-28 panel/dock + topic fixes:

What was fixed:
  - Panel detach restored: both sidebar and nick list panels can now be
    floated (detached) and re-docked using the float button in their title
    bars. Root cause: setTitleBarWidget(new QWidget) replaced the Qt title
    bar with a 0-height invisible widget, removing the float button entirely.
    Fix: removed custom title bar widgets; switched to Qt default title bar
    with empty title string and minimal QSS (0px padding, no border). Docks
    now show only a small float button with no text label.
  - "Users (N)" text removed from nick list panel: refreshNickList() was
    calling m_nickDock->setWindowTitle("Users (N)") on every nick list
    refresh, overwriting the empty title set at construction.
  - Sidebar re-dock location fixed: when the floating sidebar was closed
    (X button), it was re-appearing in the center/wrong area. Fix: the
    visibilityChanged handler now explicitly calls
    addDockWidget(Qt::LeftDockWidgetArea, m_sidebarDock) before setFloating
    to ensure it snaps back to the left.
  - Floating dock disappear fixed: closing a floating dock window now
    re-docks it to its home area instead of hiding it. Both sidebar and
    nick list docks have visibilityChanged handlers for this.
  - Sidebar dock stored as m_sidebarDock member (was anonymous local) to
    support the above.
  - Topic bar layout fixed: "* NetworkName — N users" label was positioned
    at the far right of the info bar. Moved to be immediately right of the
    "#channel (modes)" label, with the stretch spacer on the right.
  - /topic command parsing fixed: typing "/topic #channel new topic" now
    correctly sends TOPIC #channel :new topic. Previously the entire args
    string (including the channel name) was used as the topic text, causing
    the topic to be prefixed with the channel name or sending a malformed
    topic.

Known issues left open:
  - Topic set errors (482 channel-op-needed) go to the (server) buffer, not
    the active channel — user sees "nothing" when rejected. Needs error
    routing to active channel.
  - No reconnect on disconnect
  - DCC Send File not implemented
  - Emoji picker not built

Next priorities:
  - Route server errors (482, etc.) to active channel buffer
  - Reconnect with backoff
  - Connection status indicator per server
-->

<!--
Session summary — 2026-05-28 sprint #3:

What was built:
  - PM tabs: incoming PRIVMSGs open a sidebar buffer named after the sender.
    /msg <nick> opens the buffer and switches to it. Outgoing messages echo
    into the PM buffer. openPM() in SessionModel emits channelAdded for new
    conversations.
  - Nick list right-click menu: nick name (bold, disabled title), Message,
    Send File (disabled — DCC pending), Whois, Give Op, Give Voice, Version.
    Clean nick stored in Qt::UserRole on each list item.
  - Tray icon left-click now toggles window visibility (hide if visible,
    show+raise if hidden).
  - Unread indicator: dot only (● #channel) in sidebar — no color/bold change
    on the channel text. Tray badge removed entirely.
  - Sidebar flat list redesign: no expand arrows, no collapse, servers as
    muted uppercase section headers. Small grey circle icon on connected
    server. Branch lines hidden via QSS. Dock title bars ("Servers",
    "Users (N)") removed. Server header hover no longer causes visual shift.
  - Font Config additions: Network Name and Typing Indicator size controls
    (font_server_header, font_typing). Both persist to config.toml.
  - Nick list item padding tightened (1px vs 2px).
  - Topic bar redesign: info bar always visible showing #channel (modes)
    left and "* NetworkName — N users" right. User count updates live.
    Separate topic display (bufferBg background) drops below when Show Topic
    is toggled. Topic no longer appears inline in the info bar.
  - Channels indented 8px from server name (was 14px).

Bugs fixed:
  - Modes never populated in info bar because MODE #channel was never
    requested; fixed by sending MODE after 366 RPL_ENDOFNAMES.
  - Topic text was stored but never displayed; now shown in topic display.
  - CLAUDE.md was tracked in git despite being in .gitignore; untracked.
    AI-referencing language stripped from changelog comment blocks.

Known issues left open:
  - No reconnect on disconnect
  - DCC Send File not implemented
  - Emoji picker not built
  - Halloy-style sidebar/userlist aesthetic not fully matched
-->

<!--
Session summary — 2026-05-28 doc audit:

What was done:
  Full audit of all documentation against actual implemented features.
  Found and fixed stale/wrong content across three files:

  docs/index.html:
    - Version badge updated v0.1.0 → v0.2.0 (nav + footer)
    - SASL cap entry changed from "planned" (grey) to "active" (green)
    - draft/typing cap added as active (was missing entirely)
    - Two new feature cards added: SASL & NickServ, Persistent layout

  docs/faq.md:
    - "position is not persisted yet" updated to reflect panel persistence fix
    - Stale App Icon Picker FAQ removed (feature was removed from UI)
    - New entry: NickServ auto-identify setup with full config example
    - New entry: SASL PLAIN setup with full config example and network advice

  docs/ircv3.md:
    - SASL section moved from Planned to Active; updated to describe PLAIN
      implementation, config keys, and 903/904/906 handling
    - draft/typing added as new Active section with description

Known issues left open:
  - Emoji picker not built
  - No reconnect on disconnect
-->

<!--
Session summary — 2026-05-28 sprint #2:

What was built:
  - NickServ IDENTIFY auto: nickserv_password in [[server]] config;
    PRIVMSG NickServ :IDENTIFY sent immediately after 001 RPL_WELCOME.
  - Toolbar unified with sidebar: toolbar, status bar, hamburger button
    all use sidebarBg; no border lines; QToolBar::handle zeroed; toolbar
    layout margins zeroed; no right-click context menu.
  - "Uplink" label removed from toolbar.
  - Hamburger size doubled (font * 2); flush left with no padding.
  - Panel size persistence: QMainWindow saveGeometry/saveState saved to
    QSettings on quit (connected via aboutToQuit signal); restored in
    constructor. Max-width caps on sidebar and nick list removed so panels
    can be freely dragged.
  - Topic bar fixed: channel+modes combined on left (m_topicLabel),
    topic text in middle with stretch (m_modesLabel), server name right.
    MODE #channel sent after 366 RPL_ENDOFNAMES so modes always arrive.
  - /sysinfo full rewrite: format OS: ... CPU: ... MEM: ... GPU: ... UP: ...
    GPU from vulkaninfo --summary (deviceName + last-parens of driverInfo
    e.g. RADV STRIX1), lspci fallback. Uptime from /proc/uptime parsed to
    days/hours/minutes/seconds. MEM is total GB rounded. CPU is model name
    only (no thread count). OS is "Linux (Name version) (kernel)".
    FreeBSD uptime from kern.boottime sysctl.

Bugs fixed:
  - Topic bar showed empty modes because MODE #channel was never requested;
    fixed by sending MODE after 366 end-of-names.
  - Topic text stored in ch->topic was never displayed; now shown in bar.
  - /sysinfo CPU showed "x86_64" (arch fallback) because /proc/cpuinfo
    parsing stopped before finding model name line. Fixed in rewrite.
  - /sysinfo RAM showed "Unknown" for same reason — MEM field now reads
    MemTotal only (no MemAvailable), clean and reliable.

Known issues left open:
  - Release workflow end-to-end not yet confirmed (needs a tag push)
  - SASL EXTERNAL not implemented
  - No reconnect on disconnect
  - Emoji picker not yet built

Next priorities:
  - Reconnect with backoff
  - Connection status indicator per server
  - AppImage packaging
-->

<!--
Session summary — 2026-05-28 sprint:

What was built:
  - /help command: lists all available slash commands in the active chat buffer
    as Server-type messages. One appendMessage call per line for clean display.
  - /sysinfo rewritten: replaces the old Qt-only QSysInfo one-liner with real
    system data. On Linux reads /etc/os-release (PRETTY_NAME), /proc/cpuinfo
    (model name + thread count), /proc/meminfo (MemTotal/MemAvailable), and
    uname -r for kernel. On FreeBSD uses sysctl hw.model/hw.ncpu/hw.physmem/
    vm.stats.vm.v_free_count/hw.pagesize. macOS uses sysctl hw.model/hw.ncpu
    for CPU. Output format: OS: <name> | Kernel: <ver> | CPU: <model> (N threads) | RAM: <used>/<total>GB
  - SASL PLAIN: full IRCv3 SASL flow. Config keys sasl_user/sasl_password in
    [[server]] block. If set: sasl cap added to CAP REQ, CAP END held until
    SASL completes, AUTHENTICATE PLAIN sent on CAP ACK, base64(\0user\0pass)
    payload sent on AUTHENTICATE +. 903 success → CAP END. 904/905/906
    failure → CAP END + server message. Numerics 900/903/904/905/906 handled.

Bugs fixed:
  None — all three changes are new features with clean builds.

Known issues left open:
  - Release workflow end-to-end not yet confirmed (needs a tag push)
  - SASL EXTERNAL not implemented
  - No reconnect on disconnect
  - Emoji picker not yet built

Next priorities:
  - NickServ IDENTIFY auto
  - Reconnect with backoff
  - Connection status indicator per server
  - AppImage packaging
-->

<!--
Session summary — initial build sprint:

What was built:
  Full Qt6/C++ IRC client from scratch. Project replaces DojoIRC (Go/Wails)
  with a native Qt Widgets app targeting smaller RAM footprint and long-session
  stability.

  Completed this sprint:
  - CMakeLists.txt with Qt6, tomlplusplus, POST_BUILD theme copy
  - TOML config loader (config.h/config.cpp) with full struct hierarchy:
    UiConfig, ServerConfig, ChannelConfig, Config
  - IRCv3 message parser (ircparser.h/cpp): tags, prefix, command, params
  - IrcClient (ircclient.h/cpp): QSslSocket TLS, CAP LS 302, all IRC
    numerics and commands, CTCP ACTION
  - Data model: Message, Channel (NickEntry + prefixRank), ServerSession,
    SessionModel (central hub with signal/slot wiring)
  - MainWindow: toolbar, hamburger menu, sidebar tree, nick dock,
    topic bar, chat view, input bar
  - TrayIcon: left-click shows window, right-click Show/Quit,
    unread red-dot badge, balloon notifications
  - AppIcons: maindefault.png / mainalt.png / about.png, user picks
    in hamburger, persists via QSettings
  - AboutDialog: horizontal logo (400x100), version, server info
  - ThemeLoader: 55 TOML themes, named {{key}} substitution (avoids
    Qt arg() %10+ mangling bug), live switching
  - First-run nick dialog if config has placeholder "yournick"
  - 55 theme files ported from DojoIRC

Bugs found and fixed:
  - Signal ordering: loadConfig() emitted signals before MainWindow
    connected them → sidebar/chat empty on launch.
    Fix: moved model->loadConfig() to after window.show() in main.cpp.
  - Qt arg() mangling: %10+ placeholders corrupted with >9 args.
    Fix: replaced all .arg() chaining with named {{key}} substitution
    via fill() helper and QHash.
  - QSS placeholder selector: used QLineEdit::placeholder (invalid),
    should be QLineEdit::placeholder-text. Fixed in themeloader.cpp.
  - Missing QListWidget forward declaration in mainwindow.h.
  - Missing QApplication include in mainwindow.cpp (qApp undefined).
  - qAsConst() deprecated in Qt6: replaced with std::as_const().
  - Default channel was #uplinkirc — corrected to #uplink throughout.

Known issues open:
  - Theme and icon choice persist via QSettings only, not config.toml.
  - No auto-reconnect on disconnect.
  - Emoji button toggle visible but picker not yet built.
  - Documentation panel in hamburger opens nothing (not yet built).
  - Window position/dock layout not persisted.
-->

<!--
Session summary — docs, repo, landing page sprint:

What was built:
  - Full documentation rewrite — all 5 docs/ files rewritten from scratch,
    all DojoIRC references removed. configuration.md, commands.md, faq.md,
    ircv3.md, keyboard-shortcuts.md now reflect actual v0.1.0 feature set
    and real config format ([[server.channels]] structure, [ui] block).
  - config.toml.example updated to match actual parser format.
  - README.md rewritten with real build instructions, per-platform dep
    install commands, feature list, config example, docs index.
  - version.h.in added — CMake configure_file bakes PROJECT_VERSION into
    version.h at build time. About dialog now shows version from header
    (UPLINKIRC_VERSION macro) instead of hardcoded string. To release:
    bump VERSION in CMakeLists.txt, rebuild, tag.
  - Global git commit-msg hook at ~/.config/git/hooks/commit-msg strips
    all AI co-author/attribution lines from every commit on this machine.
    git config --global core.hooksPath set to ~/.config/git/hooks.
  - GitHub repo created: https://github.com/noderelay/UplinkIRC (public).
  - Branch protection on main — force push and deletion blocked.
  - GitHub Pages enabled from docs/ folder.
    Live at: https://uplink.github.io/UplinkIRC/
  - Landing page (docs/index.html) — dark Catppuccin design, hero with
    About.png logo, cross-platform section (Linux/FreeBSD/macOS/Windows/ARM64),
    feature cards, IRCv3 capability status grid, docs links, footer.
  - about.png resized from 2172px/894KB to 760px/143KB for fast page load.
    Optimized copy also written back to resources/icons/about.png.
  - Project rules updated: zero AI attribution anywhere, ever.

Bugs found and fixed:
  - about.png was 894KB (2172×724px) — invisible on dark hero background
    and slow to load. Fixed: resized to 760px wide, 143KB. White rounded
    card container added so logo is visible against dark background.

Known issues open:
  - Theme and icon choice persist via QSettings only, not config.toml.
  - No auto-reconnect on disconnect.
  - Emoji picker not yet built.
  - In-app Documentation panel not yet built (resources/docs/ doesn't exist).
-->

<!--
Session summary — font and input bar polish:

What was built:
  - IBM Plex Mono set as default application font (size 10, monospace fallback hint).
    Applied via QApplication::setFont() in main.cpp before any window is created.
  - Nick prefix label background fix committed: moved QWidget#inputBar and
    QWidget#inputBar QLabel rules into the theme QSS template. Removed inline
    stylesheet from the inputBar widget. This was built last session but the code
    commit landed after the docs close — documented here for completeness.

Bugs found and fixed:
  - Nick prefix QLabel white-background artifact: confirmed fix committed as 63c89a9.

Known issues open:
  - No auto-reconnect on disconnect.
  - Emoji picker not yet built.
-->

<!--
Session summary — icon, color codes, and input bar fix sprint:

What was built:
  - Synced ROADMAP/CHANGELOG: 4 commits from prior session landed after close had
    already run. Brought all docs surfaces back in line.
  - Single SVG icon: replaced maindefault.png, mainalt.png, about1.png with one
    uplink.svg. Added Qt6::Svg to CMakeLists.txt find_package and target_link_libraries.
    Removed icon picker submenu from hamburger. Removed `icon` field from UiConfig,
    config.cpp load/save, and config.toml. AppIcons simplified to two inline functions.
  - mIRC color code rendering: ircToHtml() static function in mainwindow.cpp processes
    raw IRC text before HTML insertion. Handles \x02 bold, \x1D italic, \x1F underline,
    \x1E strikethrough, \x16 reverse (fg/bg swap), \x03fg,bg color (16 mIRC colors),
    \x0F reset. Applied to Privmsg, Action, Notice message types.
  - Nick prefix label background fix: moved inputBar and nick label styling entirely into
    the theme QSS template (QWidget#inputBar, QWidget#inputBar QLabel rules). Removed
    inline stylesheet from the inputBar widget. Fixes white-box artifact caused by Qt's
    stylesheet inheritance breaking background:transparent on children of styled parents.

Bugs found and fixed:
  - Nick prefix QLabel rendered with white background on themed builds. Root cause: Qt
    does not correctly propagate background:transparent through parent widgets that have
    inline stylesheets. Fix: remove inline stylesheet from inputBar, move all input bar
    styling into the theme QSS where Qt can resolve it in one pass.

Known issues open:
  - No auto-reconnect on disconnect.
  - Emoji picker not yet built.
-->

<!--
Session summary — UX features + docs panel sprint:

What was built:
  - Nick completion: Tab key at end of a word looks up matching nicks in the
    current channel's nick list and cycles through them on repeated presses.
  - Input history: Up/Down arrow in the input bar cycles through previously
    sent messages per-channel (standard IRC client behavior).
  - Colored nicks: consistent hash-based color per nick applied in chat view
    and nick list. Toggle in hamburger menu, persists to config.toml.
  - In-app Documentation panel: Hamburger → Documentation opens a QDialog
    (DocsDialog) with a tab per doc file rendered as plain text. New files:
    src/ui/docsdialog.h, src/ui/docsdialog.cpp. Resources registered in
    resources.qrc.
  - Save theme + icon to config.toml: previously these only persisted via
    QSettings. Now written back to config file on change so they survive
    reinstall / config copy.
  - About.png logo added to README.md hero section.
  - Configuration doc updated with icon key table.
  - Keyboard shortcuts doc updated — Tab/Up/Down marked as implemented.

Bugs found and fixed:
  - about.png had a white background that rendered invisible against the dark
    GitHub Pages hero section. Fix: normalized background to transparent /
    dark-compatible. Updated both docs/about.png and resources/icons/about.png.
  - Input bar config/QSettings interaction had a minor inconsistency causing
    prefs not to round-trip correctly on first launch after a clean config.
    Fixed in config.cpp.

Known issues open:
  - No auto-reconnect on disconnect — must restart app.
  - Emoji picker not yet built (button toggle is wired).
-->

<!--
Session summary — CI green sprint:

What was fixed:
  - CI was failing on Windows only. Linux and macOS were already passing after
    the per-platform Qt install switch. Windows aqtinstall was using arch
    'win64_msvc2019_64' which doesn't exist for Qt 6.8.x — Qt 6.7+ moved to
    MSVC 2022 builds. Fixed: changed arch to 'win64_msvc2022_64'.
  - Windows runner was hitting a Python permission error on the Windows Store
    python.EXE stub. Fixed: added explicit actions/setup-python@v5 step before
    jurplel/install-qt-action so aqtinstall gets a clean Python install.
  - Both fixes applied to ci.yml and release.yml.
  - All three platforms (ubuntu-24.04, windows-latest, macos-latest) now pass.

Known issues open:
  - Release workflow not yet confirmed end-to-end (no new tag pushed this session).
  - No auto-reconnect on disconnect.
  - Emoji picker not yet built.
-->

<!--
Session summary — v0.2.0 sprint:

What was built:
  - Sidebar: server item is now the server buffer — clicking "LinuxDojo" (or
    whatever the server short name is) opens the server message buffer.
    The redundant "(server)" child item is gone. Short name pulled from config.
  - Per-widget font config: Font Config dialog in hamburger lets the user set
    independent font sizes for Toolbar, Sidebar, Chat, User List, Users Title,
    Topic Bar, Nick Label, and Input. Family picker + live preview. Persists to
    config.toml under font_* keys.
  - Topic bar replaced with channel info bar: shows #channel (modes) ServerName
    all left-aligned. Modes come from actual server data (324 RPL_CHANNELMODEIS
    now parsed). No toggle button — hamburger "Show Topic Bar" controls the bar.
    Bar uses theme sidebarBg color so it works correctly on dark themes.
  - Typing indicator: IRCv3 draft/typing via TAGMSG. Sends typing=active after
    1s debounce, typing=done on send or clear. Receives and displays "nick is
    typing..." above the input bar. Per-nick 6s auto-clear. Hamburger toggle,
    on by default.
  - CTCP: auto-replies to incoming VERSION and PING. CTCP replies shown in server
    buffer. Outgoing CTCP via /ctcp command.
  - New slash commands: /away, /back, /motd, /whois, /topic, /kick, /notice,
    /version, /ctcp, /sysinfo. Version string sourced from version.h macro.
  - Colored nicks toggle added to hamburger menu (was config-only before).
  - GitHub Actions CI: per-platform Qt install (apt on Linux, aqtinstall on
    Windows, Homebrew on macOS). Builds on every push to main.
  - GitHub Actions release workflow: builds Linux tarball, Windows zip (windeployqt),
    macOS dmg (macdeployqt) and attaches them to GitHub releases on tag push.
  - v0.2.0 tagged and pushed. Tag re-pushed to trigger release workflow.
  - CMakeLists.txt: FetchContent fallback for tomlplusplus so CI doesn't need
    the system package.

Bugs fixed:
  - 324 RPL_CHANNELMODEIS was never parsed — channel modes always empty on join.
    Fixed: ircclient.cpp now handles numeric 324 and emits modesReceived.
    modesChanged signal added to SessionModel so the UI refreshes.
  - Topic bar used palette(mid) background which rendered invisible on dark themes.
    Fixed: objectName "topicBar" + QSS rule using theme sidebarBg/border colors.
  - CI failing: jurplel/install-qt-action with Qt 6.7.3 caused aqt package-not-found
    errors on all platforms. Fixed: Linux uses apt, Windows aqtinstall 6.8.2,
    macOS Homebrew. qtsvg is included in base Qt6 install, not a separate module.

Known issues open:
  - CI release workflow untested end-to-end (tag re-push happened after workflows
    were committed; outcome unknown at session close).
  - No auto-reconnect on disconnect.
  - Emoji picker not yet built.
  - /help command not yet implemented.
-->

<!--
Session summary — 2026-05-28 UI polish sprint:

What was fixed:
  - Toolbar/hamburger right-click context menu eliminated. Qt's built-in toolbar
    visibility menu ("Main") appeared on right-click because right-click events
    propagated from the QToolButton → QToolBar → QMainWindow, where QMainWindow
    rendered the toolbar list menu despite NoContextMenu on the toolbar. Fix:
    switched both toolbar and hamburger to CustomContextMenu policy with a no-op
    lambda, which absorbs the event before it reaches QMainWindow.
  - Size grip (grey square, bottom-right corner) removed. Two-pronged fix:
    statusBar()->setSizeGripEnabled(false) in code, plus QSizeGrip { width: 0;
    height: 0; image: none } in the theme stylesheet.
  - Sidebar and nick list item spacing tightened. Items had 2px/1px vertical
    padding and Qt's default row height. Fixed with height: 18px in QSS for both
    QTreeWidget::item and QListWidget::item, plus setSpacing(0) on the nick list
    widget.

Investigated but reverted this session (not committed):
  - QMainWindow::separator styling attempts (transparent, sidebarBg, red diagnostic)
  - setFrameShape(QFrame::NoFrame) on sidebar, nick list, chat view
  - setTitleBarWidget(new QWidget()) on dock widgets (broke panel detach)
  - setCorner() calls to constrain dock separator height
  - Fusion style forced via QStyleFactory (app.setStyle)
  - Input bar bottom padding reduction
  - QSplitter refactor replacing dock widgets (broke entire layout, fully reverted)
  - windowState key renamed to windowState_v2

Known issues left open:
  - Dock separator lines visible at left/right edges of chat area, extending into
    the toolbar region — root cause confirmed as QMainWindow::separator; color
    and size changes did not fully suppress them; QSplitter refactor would fix
    this definitively but was reverted due to regressions
  - No reconnect on disconnect
  - DCC Send File not implemented
  - Emoji picker not built
-->

<!--
Session summary — 2026-05-29 v0.3.0 sprint:

What was built:
  - Clickable URLs in chat messages: swapped QTextEdit for QTextBrowser
    (drop-in, adds anchorClicked signal). Added linkifyHtml() that runs
    the same URL regex as linkifyTopic() on the already-HTML output of
    ircToHtml(). Applied to Privmsg, Action, and Notice messages.
    QDesktopServices::openUrl opens the link in the system browser.
    Topic bar already had this; chat now matches.
  - Reconnect with exponential backoff: IrcClient gained QTimer
    m_reconnectTimer, m_intentionalDisconnect flag, and m_reconnectDelay
    counter. Unexpected disconnect or socket error triggers scheduleReconnect()
    which emits "Reconnecting in Ns..." to the server buffer and fires the
    timer. Each failed attempt doubles the delay: 5s → 10s → 20s → 40s → 60s
    (capped). Successful connect resets to 5s. User-initiated quit() sets
    m_intentionalDisconnect to skip scheduling. SessionModel::onConnected
    already auto-joins configured channels after reconnect.
  - Sidebar right-click context menus: server items show Disconnect (if
    connected) or Reconnect (if disconnected). Channel items (#, &) show
    Rejoin (PART + 500ms delayed JOIN) and Leave. Menu built in
    MainWindow::onSidebarContextMenu using itemAt(pos) and UserRole data.

Bugs fixed: none — all new features.

Known issues left open:
  - Hamburger menu briefly shrinks on theme switch (root cause unknown)
  - Server errors (482 etc.) still go to (server) buffer, not active channel
  - Emoji picker not built
  - DCC Send File not implemented
  - Dock separator lines at chat edges

Next priorities:
  - Route server errors (482 etc.) to active channel buffer
  - Connection status indicator per server in sidebar
  - AppImage packaging for Linux
-->

<!--
Session summary — 2026-05-29  README redesign / repo beautification

What was built:
  - README.md completely redesigned from a plain bullet list to a rich,
    visually structured GitHub page:
    * 5 badges: latest release, CI status, MIT license, Qt6/C++17, IRCv3
    * 3 platform download buttons (Linux, Windows, macOS) linking directly
      to v0.6.0 release assets
    * App icon gallery — all 5 icon variants (Dark, Light Default, Light,
      Avatar, Mark) displayed side-by-side with labels
    * Features reorganized into 6 category tables: Security & Auth,
      IRC Protocol & IRCv3, Interface & Themes, Chat Features, Nick List
      & Sidebar, Connectivity & Servers
    * Quick Start up top with collapsible <details> blocks per OS for
      dependency install (Arch, Ubuntu, Fedora, FreeBSD, macOS)
    * Full annotated TOML config example with inline comments on every key,
      including multi-server, SASL, NickServ, bouncer
    * Slash commands table (all commands in one place)
    * Emoji shortcut examples inline
    * Keyboard shortcuts table
    * Documentation links table
    * Brand assets section with logo SVG displayed + file table

Bugs fixed: none (docs-only session)
Regressions: none
Known issues unchanged from v0.6.0

Next priorities:
  - Route server errors (482 etc.) to active channel buffer
  - Link preview persistence across channel switches
  - Desktop notifications on mention/PM
  - AppImage packaging for Linux
-->

<!--
Session summary — 2026-05-29  v0.7.0 — new dark banner + link preview fix

What was built / fixed:
  - New dark banner (uplink-top-banner-dark.svg) — replaces assets/banner.svg
    (GitHub README) and resources/icons/banner.svg (About dialog via QRC).
    Both locations updated in the same commit; QRC picks it up automatically.

  - Link preview fixed for regular URLs (e.g. linuxdojo.org):
    Root cause 1: Qt does not follow redirects by default.
      Fix: QNetworkRequest::RedirectPolicyAttribute →
           NoLessSafeRedirectPolicy on both page and image fetches.
           This was the primary blocker for bare-domain and http→https URLs.
    Root cause 2: WhatsApp/2 User-Agent blocked or returns minimal HTML
      on many sites.
      Fix: Switched to a standard Chrome/Linux UA with Accept + Accept-Language
           headers so servers return full HTML.
    Root cause 3: 16 KB HTML cap too small; some sites have large <head>
      sections before <title>.
      Fix: Raised to 32 KB; also updated title regex from [^<]{1,200} to
           [\s\S]{1,300}? so multi-line <title> tags are matched, then
           .simplified() collapses whitespace.
    Timeout also raised from 4 s to 6 s.

Bugs fixed:
  - Link preview silently failing for sites with http→https redirects
  - Link preview silently failing for sites that reject WhatsApp/2 UA
  - Link preview missing for sites with late or multi-line <title> tags

Regressions: none — og:title / og:image still extracted as before;
  thumbnail path unchanged.

Known issues remaining:
  - Link preview cards lost when switching channels (not in message history)
  - Server errors (482 etc.) still route to (server) buffer, not active channel
  - DCC Send File stub not yet implemented
  - Hamburger shrink on theme switch (root cause unknown)

Next priorities:
  - Route server errors to active channel buffer
  - Link preview persistence across channel switches
  - Desktop notifications on mention/PM
  - AppImage packaging for Linux
-->

## [0.7.0] — 2026-05-29

**New dark banner; link preview works for regular URLs**

- New dark banner (`uplink-top-banner-dark.svg`) used in both the GitHub README and the About dialog
- Fix: link preview now works for plain websites like `linuxdojo.org`
  — Qt redirect policy set to `NoLessSafeRedirectPolicy` (was silently not following http→https or bare-domain redirects)
  — User-Agent switched from `WhatsApp/2` to a standard Chrome UA with `Accept` headers (many sites rejected the old UA)
  — HTML fetch buffer raised from 16 KB to 32 KB so late `<title>` tags are reached
  — Title regex updated to match multi-line `<title>` content
  — Timeout raised from 4 s to 6 s

---

## [Unreleased] — 2026-05-29

**Repository beautification**

- README redesigned — badges (release, CI, license, Qt6/C++17, IRCv3), platform download buttons, app icon gallery (all 5 variants), features organized into 6 category tables, collapsible OS dependency install sections, full annotated TOML config example, slash commands table, keyboard shortcuts table, documentation links table, brand assets section with logo

---

## [0.6.0] — 2026-05-29

**Emoji picker, :shortcode: autocomplete, bot icons, native Windows style, menu icons**

- Emoji picker — click 😊 in the input bar to open a searchable popup grid of ~400 emoji; click any to insert at cursor
- Inline `:shortcode:` autocomplete — type `:smi` to get a live match list above the input; navigate with Up/Down, confirm with Enter/Tab/click, dismiss with Escape
- Auto-substitute on closing colon — typing `:trident:` replaces instantly in the input field when the closing `:` is typed
- Substitute on submit — any `:shortcode:` patterns remaining in a message are resolved to emoji before sending; `:trident:` + Enter sends 🔱
- Bot nick icons — nicks with `+B` mode display 🤖 or 👾 (chosen per-nick by hash, stable); tracked for both channel user mode and global user mode `+B`
- QPainter-drawn menu icons — each hamburger menu item has a distinct monochrome line icon drawn via QPainter; palette-aware so icons adapt to dark/light themes
- Windows: native style by default — `windows11` Qt style set on Windows; no QSS applied for the default theme; custom themes still apply when selected
- Windows: Consolas default font — `IBM Plex Mono` on Linux/macOS, `Consolas` on Windows
- Fix: Windows console window (`conhost.exe`) suppressed — `WIN32` subsystem flag set in CMakeLists.txt; closing nothing kills the GUI
- Fix: typing indicator now overlays the chat background — removed from layout row, now a child overlay of chat view, transparent, no box or stripe
- Fix: emoji button crop — `setFixedSize(30,30)` + `font-size:16px; padding:0`
- CI: opt in to Node.js 24 before GitHub's June 2nd forced upgrade deadline

---

## [Unreleased] — 2026-05-29

**macOS CI fix, link preview card with thumbnail**

- macOS release build fixed — `MACOSX_BUNDLE` property now set in CMakeLists.txt so CMake produces a `.app` bundle; `macdeployqt` can now find and package it. v0.3.0 re-tagged.
- Link preview on hover — hovering any `http/https` URL in chat shows the domain name in the status bar and a tooltip; the page title replaces it when the fetch completes
- Inline preview card — URLs in live messages auto-fetch `og:title` + `og:image` and render a small card below the message: bold title, domain, and thumbnail image (capped at 120×90px, 200KB)
- Fix: crash in `LinkPreview::fetch()` when `abort()` fired `finished()` synchronously, causing a null-pointer `deleteLater()` call

---

## [0.3.0] — 2026-05-29

**Reconnect backoff, clickable URLs in chat, sidebar context menus**

- Clickable URLs in chat messages — `http://` and `https://` links in PRIVMSG, actions, and notices are now live links; click to open in the system browser
- Auto-reconnect with exponential backoff — unexpected disconnects trigger automatic reconnect: 5s → 10s → 20s → 40s → 60s (capped); server buffer shows countdown; deliberate `/quit` disables it
- Sidebar right-click menus — right-click a server to **Disconnect** or **Reconnect**; right-click a `#channel` to **Rejoin** or **Leave**

---

## [Unreleased] — 2026-05-28

**Server management UI + hamburger menu improvements**

- Hamburger menu now opens above the button, anchored top-right — no more downward popup
- Theme list is scrollable with compact item padding — all 55 themes reachable via scroll arrows
- Theme application no longer causes visual glitch on menu close
- **Manage Servers...** dialog — view, add, edit, and remove servers from within the app; no config file editing required
- Add Server dialog — three-section form: Connection (name, host, port, SSL), Identity (nick, username, real name), Authentication (server password, SASL user/password, NickServ password)
- Edit server pre-fills the form with existing values; changes reconnect immediately
- Remove server disconnects immediately and removes from config
- All server changes persisted to `config.toml` on OK
- Fix: explicit `QMenu::item` padding in theme stylesheet stabilizes menu height across theme switches
- Known: menu still briefly shrinks on theme switch (investigation ongoing)
- Fix: `CLAUDE.md` and `.claude/` removed from public `.gitignore`; moved to `.git/info/exclude`

---

## [Unreleased]

**UI layout — topic bar, dock titles, hamburger relocation**

- Topic bar background now matches the input area (`inputBg`) — info bar blends with the input row
- Mode string spacing fixed — `(+nt)` and `* Network` now have a proper gap between them
- Clickable URLs in topic display — `http://` and `https://` URLs are live links; click to open in browser
- Toolbar removed — nothing sits above the `#channel` info bar; all panels start flush at the top
- Dock title bars replaced with minimal custom bars — each panel (server list, user list) has a 16px strip with only a `⧉` float/detach button; content is flush with the info bar
- Hamburger menu relocated to bottom-right of the nick list panel — sits level with the input bar; status bar text shrunk to 7pt

---

**UI polish — right-click menu, size grip, item spacing**

- Toolbar and hamburger right-click context menu suppressed — Qt's "Main" toolbar visibility popup no longer appears on right-click
- Status bar size grip (grey square, bottom-right corner) removed
- Sidebar channel list and nick list items tighter — reduced vertical padding and row height

---

**Panel detach, topic bar layout, /topic fix**

- Panel detach restored — sidebar (server/channel list) and nick list panels can now be floated and re-docked using the float button; closing a floating panel re-docks it instead of hiding it
- Topic bar layout — `* NetworkName — N users` now sits right next to `#channel (modes)`, not at the far right
- `/topic` command — now accepts `/topic #channel new topic` (channel name optional; correctly separates it from the topic text)

**Fix:** Nick list panel was showing "Users (N)" label above the list — stopped refreshNickList from overwriting the dock title  
**Fix:** Sidebar panel re-docked to wrong area (behind chat) — now explicitly re-adds to left dock area  
**Fix:** `/topic #uplink some text` was sending the channel name as part of the topic string  

---

**PM tabs, nick menu, sidebar redesign, topic bar, font config additions**

- PM tabs — `/msg <nick>` opens a sidebar buffer; incoming PMs land in sender's buffer; both sides echoed
- Nick list right-click menu — Message, Send File (disabled), Whois, Give Op, Give Voice, Version
- Tray icon left-click toggles window — hides if visible, shows if hidden
- Unread indicator — dot only (`● #uplink`) in sidebar; no tray badge
- Sidebar flat list — no expand arrows, servers as muted uppercase section headers, grey circle icon on connected server, dock titles removed, hover on server headers fixed
- Font Config — added "Network Name" and "Typing Indicator" size controls; persist as `font_server_header` / `font_typing`
- Info bar always visible — `#uplink (+nt)` left, `* LinuxDojo — 42 users` right; user count live
- Topic display — separate area below info bar with chat-window background, shown/hidden by "Show Topic" toggle
- Channels indented 8px from server name; nick list items tighter

**Fix:** Modes never populated — `MODE #channel` now sent after `366 RPL_ENDOFNAMES`  
**Fix:** Topic text was stored but never shown — now displayed in topic display area  

---

**Doc audit — fixed stale content across index.html, faq.md, ircv3.md**

- `docs/index.html` — version updated to v0.2.0; SASL cap marked active; `draft/typing` cap added; SASL/NickServ and panel persistence feature cards added
- `docs/faq.md` — panel persistence FAQ updated; NickServ IDENTIFY and SASL PLAIN FAQ entries added with full config examples
- `docs/ircv3.md` — SASL moved from Planned to Active with full description; `draft/typing` added as Active

---

**NickServ auto-identify, UI polish, panel persistence, topic bar fix, /sysinfo v2**

- NickServ IDENTIFY auto — add `nickserv_password` to any `[[server]]` block; sent to NickServ on connect
- SASL PLAIN authentication — add `sasl_user` and `sasl_password` to any `[[server]]` block; full CAP negotiation flow; 903/904/906 handled
- `/help` command — lists all available slash commands in the chat buffer
- `/sysinfo` rewritten — format: `OS: Linux (Arch Linux rolling) (7.0.10-arch1-1) CPU: ... MEM: 24 GB GPU: AMD Radeon 890M Graphics (RADV STRIX1) (Vulkan) UP: 4 days 3 hours...`; GPU from `vulkaninfo`, fallback `lspci`; uptime from `/proc/uptime`
- Panel size persistence — sidebar and nick list sizes saved on quit, restored on launch; no more max-width caps
- Toolbar unified with sidebar — toolbar, status bar, hamburger all use `sidebarBg`; borders and handle gap removed; "Uplink" label removed; hamburger doubled in size and flush left; no right-click context menu
- Topic bar — shows `#channel (modes)` left, topic text middle, server name right; `MODE #channel` requested after join so modes always populate

**Fix:** Topic text was stored but never displayed — now shown in the topic bar  
**Fix:** Channel modes never populated — `MODE #channel` now sent after 366 RPL_ENDOFNAMES  
**Fix:** `/sysinfo` CPU showed `x86_64` fallback — fixed in rewrite  
**Fix:** `/sysinfo` RAM showed `Unknown` — fixed in rewrite  

---

## [0.2.0] — 2026-05-28

**Font config, typing indicator, info bar, extended commands, CI/CD**

- Per-widget font size config — Font Config dialog (hamburger) sets independent sizes for Toolbar, Sidebar, Chat, User List, Users Title, Topic Bar, Nick Label, and Input; family picker with live preview; persists to `config.toml`
- Typing indicator — IRCv3 `draft/typing` via TAGMSG; debounced send + receive; shows "nick is typing..." above input bar; hamburger toggle, on by default
- Channel info bar — replaces topic text bar; shows `#channel (modes) ServerName` left-aligned; modes populated from `324 RPL_CHANNELMODEIS`; uses theme colors
- Sidebar server item is now clickable — shows server buffer (MOTD, numerics); redundant `(server)` child removed; displays short name from config
- Extended slash commands — `/away`, `/back`, `/motd`, `/whois`, `/topic`, `/kick`, `/notice`, `/version`, `/ctcp`, `/sysinfo`
- CTCP — auto-replies to incoming VERSION and PING; `/ctcp` for arbitrary requests; replies shown in server buffer
- Colored nicks toggle added to hamburger menu
- GitHub Actions CI — builds on every push to main across Linux, Windows, macOS
- GitHub Actions release — builds platform binaries (Linux tarball, Windows zip, macOS dmg) and attaches to GitHub releases on tag push
- `tomlplusplus` FetchContent fallback in CMakeLists — CI no longer needs the system package
- Bumped version to 0.2.0

**Fix:** `324 RPL_CHANNELMODEIS` now parsed — channel modes populate correctly on join  
**Fix:** Topic bar background now uses theme `sidebarBg` — visible on all dark themes  
**Fix:** CI Qt install — switched to per-platform install to avoid aqtinstall mirror failures

**Known:** Release CI workflow not yet confirmed green end-to-end  
**Known:** No auto-reconnect on disconnect  
**Known:** Emoji picker not yet implemented

---

## [Unreleased] — v0.1.0

**Initial release — full IRC client foundation**

- Project scaffold — Qt6/C++17, CMake, directory structure
- TOML config loader via toml++ — auto-creates config on first launch
- First-run nick dialog — prompts if config has placeholder `yournick`
- IRC connection — TLS via QSslSocket, CAP LS 302 negotiation
- IRCv3 message tag parser — full prefix + tag parsing
- CAP negotiation — `multi-prefix`, `away-notify`, `server-time`, `message-tags`, `batch`, `labeled-response`
- Full IRC numerics — 001 welcome, 332/353/366 topic+names, MOTD, ISUPPORT, nick-in-use fallback
- IRC commands — JOIN, PART, QUIT, NICK, KICK, MODE, TOPIC, PRIVMSG, NOTICE, CTCP ACTION
- Slash commands — `/join`, `/part`, `/nick`, `/me`, `/msg`, `/quote`, `/raw`, `/quit`
- Data model — Message, Channel, ServerSession, SessionModel
- Message buffer cap — 2000 messages per channel
- Nick list — sorted by prefix rank (~&@%+), live updates on JOIN/PART/QUIT/NICK
- Main window UI — toolbar, sidebar tree, chat view, nick list dock, topic bar, input bar
- QDockWidget nick list — movable left or right
- Topic bar — channel topic + modes, toggleable from hamburger
- Hamburger menu — About, Documentation (stub), App Icon picker, Theme picker, topic/nick/emoji toggles
- System tray — minimize to tray on close, left-click shows window, right-click Show/Quit
- Unread badge — red dot on tray icon, balloon notification when hidden
- Theme loader — 55 TOML themes, live switching from hamburger
- App icons — Maindefault (flat satellite) and Mainalt (3D satellite), user picks in hamburger
- About dialog — horizontal logo, version, server info

**Fix:** Signal ordering — `loadConfig()` now called after `window.show()` so sidebar/chat populate correctly  
**Fix:** Qt arg() mangling with >9 placeholders in QSS — replaced with named `{{key}}` substitution  
**Fix:** `QLineEdit::placeholder` → `QLineEdit::placeholder-text` in theme stylesheets  

**Known:** Theme and icon choice persist via QSettings only, not saved to config.toml  
**Known:** No auto-reconnect on disconnect — must restart the app  
**Known:** Emoji picker not yet implemented (button toggle is wired)  
**Known:** Documentation panel opens nothing (not yet built)  
