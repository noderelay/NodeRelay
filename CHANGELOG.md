# Changelog

<!--
Session 2026-07-12 — post-release full audit (second of the day):
- Scope: everything changed since v2026.7.0 (49 commits) + re-check of the
  security paths audited in #36 (unchanged since). Tools: manual review,
  cppcheck (--library=qt), -Wconversion sweep, ASan/UBSan test run (6/6),
  45s live libFuzzer on both harnesses (~84k runs each, no crashes;
  generated corpus files cleaned per CLAUDE.md).
- Verified clean: log-search v2 worker lifecycle (superseded threads exit
  via cancel flag; queued results dropped when stale/destroyed), ChromePanel
  paint path, panel-cards QSS plumbing, resolveLogBuffer, all #36-era caps
  still intact (previews/reactions/batches/line buffer/ChatView).
- Fixed (all trivial):
  1. commanddispatcher.cpp qsizetype->int -Wconversion — the build's LAST
     compiler warning; builds are now warning-free.
  2. DccSend::connectOut guards against double connection (cppcheck).
  3. DccSend::token() returns const ref (cppcheck perf nit).
  4. ChatLine::VisLine members default-initialized (cppcheck).
- Observations, no action: cppcheck's dccreceive null-deref warning is a
  false positive (slot only fires from the live socket); all-buffers search
  results are bounded per-buffer (200) but not globally — acceptable, the
  input is the user's own logs; m_sidebarHeader is an unfilled ChromePanel
  (paints nothing by design — card starts at the tree).
-->

<!--
Session 2026-07-11 (late night) — Panel Cards toggle:
- New Preferences → Interface → "Panel Cards" checkbox (config key
  panel_cards, default true): toggles the new two-tone rounded-card side
  panels vs the classic flat pre-overhaul look, live, no restart.
- Plumbing: ThemeLoader::toStyleSheet/apply take a panelCards flag; the
  QSS template gained conditional vars (panelSidebarBg/panelNickBg/
  panelBackdrop/rcBackdrop/panelRadius). Off = tree/nick surfaces on
  bufferBg, radius 0, rightContent back to the old sidebarBg frame.
  ChromePanel::setFill gained a rounded flag for the paintEvent side.
- OFF state pixel-verified via scratch instance on the live display:
  fully flat, matches the pre-overhaul look.
- Docs: configuration.md ([ui] example + table), howto themes section.
-->

<!--
Session 2026-07-11 (night) — theming overhaul: per-section backgrounds,
full-height panels, rounded cards; the Wayland styled-background saga:
- Theme sections finally mean something: [sidebar]/[nicklist] backgrounds
  are honored (285/297 themes define distinct values that were previously
  flattened to bufferBg by the QSS template). [general] background is now
  the window backdrop around the panels ([sidebar] bg was painted there
  before — nobody could tell, see below).
- Layout: user list is a full-height column (compose strip — search/reply/
  typing/input — moved into the chat column in main window AND panes);
  rightContent bottom margin removed so both side panels run flush to the
  window bottom. Both panels drawn as cards with rounded TOP corners:
  sidebar card starts at the network tree (hamburger row stays on window
  bg, per user); nick card includes its icon header row (per user).
- THE BUG HUNT (half a quota window, lessons in
  memory/reference_wayland_ui_debugging.md): a "tiny gap" under the user
  list resisted FIVE fixes because three separate defects overlapped:
  1) rightContent had an 8px bottom margin (fixed, real but minor);
  2) RoundedPane's 1x-DPR QBitmap mask provably clips a radius-sized band
     at fractional scale (fixed via QRegion, then mask REMOVED entirely —
     never reintroduce widget masks, comment in mainwindowdelegates.h);
  3) the REAL gap: addStretch(1) after the nick list left extra/101 px
     below it, and joe's live KDE Wayland session silently drops styled-
     background painting for plain QWidgets, exposing {{bg}} through the
     hole. Headless repros (offscreen 1.25/1.45/1.5, nested kwin) all
     painted the styled bg and hid the gap — pixel-verified on the live
     session via scratch-HOME instance + spectacle + geometry dump.
- The styled-background drop also affected the icon header rows (sidebar +
  nick) and even LOCAL per-widget stylesheets (pixel-proven). Final
  mechanism: new ChromePanel widget (src/ui/chromepanel.h) paints its own
  fill + rounded top in paintEvent — QPainter can't be dropped by styling
  machinery. Used for nickPanel, nickPanelHeader (main + panes);
  applyPanelChrome() applies theme fills at startup/theme switch.
- Sidebar tree rounding via QTreeWidget border-top radii (QSS — scrollarea
  frames paint reliably, unlike plain QWidgets).
- Fix: Reload Config appended a duplicate argv[0] to the child's argument
  list on every reload (ps showed 12 copies) — arguments().mid(1).
- Docs: theme section semantics in howto.html themes section. 6/6 tests.
  No release tagged.
-->

<!--
Session 2026-07-11 (evening) — full-history search v2: all buffers:
- "All buffers" checkbox in the Ctrl+Shift+F window. When ticked, the
  worker thread scans every *.log under ~/.config/uplink/logs/<server>/
  instead of just the current buffer's log. Same streaming/cancel design
  as v1; memory bounded at the newest 200 matches per buffer.
- Results grouped per buffer: bold header "buffer — server (count)"
  followed by that buffer's matches newest-first. Double-click/Enter on
  any row (header or match) jumps to the buffer via new signal
  jumpRequested(serverPart, bufferPart).
- New SessionModel::resolveLogBuffer() maps the sanitized log path
  components back to a live (ServerId, BufferId) by re-sanitizing current
  session/channel names and comparing (sanitizeFilename is lossy, so the
  path is never parsed). Unresolvable results (logs of closed buffers)
  jump nowhere, silently.
- New SessionModel::logsRootPath() — the logs base dir was built inline
  in three places (logMessage, logFilePath, and now the dialog); one
  helper now.
- Jump uses the QuickSwitcher pattern (findChannelItem + sidebar select)
  so pane/pop-out routing in switchToChannel applies as usual.
- Dialog input is no longer disabled when the current buffer has no log —
  All buffers can still search other logs; the single-buffer no-log
  notice moved into startSearch.
- Docs: README, keyboard-shortcuts.md, faq.md, howto.html, ROADMAP.
  6/6 tests pass. No release tagged. v3 (CHATHISTORY context jump) still
  planned.
-->

<!--
Session 2026-07-11 (later) — whole-codebase surgical audit:
- Targeted review of security paths (SSRF guard, DCC, TLS, CTCP parsing),
  resource caps (message buffer, reactions, batches, previews, ChatView,
  log search), and the fresh #35 pane code. Almost everything already had
  caps/timeouts/validation; four findings, all fixed same session:
- Fix: DCC send stall guard was a one-shot 60s deadline that killed ANY
  active-mode transfer still running after 60s ("Transfer stalled: no ACK
  received" on healthy large/slow sends). Replaced with armStallGuard():
  recurring 60s timer that tracks the peer's cumulative ACK (new m_acked,
  recorded in onReadyRead) and aborts only when it hasn't advanced between
  ticks. Also armed on the passive connectOut() path, which had no guard.
- Fix: Channel::reactions grew without bound for fabricated msgids — the
  per-msgid caps (16 emojis x 50 nicks) never limited DISTINCT msgid keys,
  and eviction pruning only fires for msgids of real buffered messages. A
  hostile client sending TAGMSG reacts with random msgids leaked memory
  forever. onReactReceived now requires ch->hasMessage(msgid) (new helper,
  same linear-scan idiom as the redaction handler). No behavior loss:
  orphan reactions were never renderable anyway.
- Fix: DccReceive::listenPassive left a zero-byte .part file behind when
  the listen bind failed — now calls cancel(), which already does the
  close-and-remove.
- Cleanup: the (\bnick\b) self-nick regex was hand-built in three places
  (onSelfNickChanged, refresh path, selfNickReFor). All three now call
  SessionModel::buildHighlightRe(nick) — identical pattern for a single
  word, and finally consistent with the "build highlight regexes only via
  buildHighlightRe()" invariant.
- CLAUDE.md TLS invariant reworded: "ignoreSslErrors() is gone" was stale —
  the TOFU pinning flow legitimately calls it for a pinned-fingerprint
  match only. New wording says exactly that, so the pinning doesn't get
  "fixed" away later.
- Everything else surveyed came back clean: SSRF guard covers IPv4-mapped
  IPv6 + all special ranges, batches capped (count + per-batch size), raw
  line buffer capped 64 KB, ChatView trims to kMaxLines and evicts
  offscreen layouts, preview pipeline has queue caps + watchdog +
  generation counters, log search streams line-by-line on a cancelable
  worker.
- Docs: stall detection noted in README, commands.md, faq.md, howto.html;
  quality.html hardening blurb extended. 6/6 tests pass. No release tagged.
-->

<!--
Session 2026-07-11 — pane/pop-out review hardening + drop-frame highlight:
- Full code review of the pane / pop-out window layer; all findings fixed
  the same session, each verified by the user on Linux.
- Fix: dragging the pane that shares a stack with the primary onto the
  primary was a silent no-op — siblingSlot() returned the primary's own
  slot, so it swapped with itself. Falls back to a plain swap now.
- Fix: Alt+Left/Right pane navigation was dead code since the
  sidebar/primary decoupling (#34) — switchToChannel refuses to load pane
  channels, so selecting them via the sidebar did nothing. Reworked to
  cycle keyboard focus across pane input bars (primary input included) in
  layout slot order.
- Fix: channels open in a pane or pop-out accumulated sidebar unread
  badges forever — they can never become the active buffer, and setActive
  was the only place unread cleared. New SessionModel::markRead() zeroes
  unread/mentions without touching the active selection; called on pane
  message append, pane open, and float.
- Fix: popping out a previously-DOCKED pane rendered the topic bar, typing
  strip, and input strip in the system palette color instead of the theme
  buffer color (fresh pop-outs were fine — they get first-time polish under
  the new window; reparented panes rely on Qt's implicit repolish, which
  misses the QSS backgrounds on plain-QWidget surfaces). Two layers:
  explicit unpolish/polish of the pane subtree in floatPane, plus a
  #paneWindow-scoped buffer-bg stylesheet on the window as fallback.
  Diagnosed from screenshot pixel forensics: the wrong color matched no
  theme color, only the platform palette. Repro attempts offscreen all
  rendered correctly — platform-timing dependent.
- Drop highlight reworked per user request: dragging a pane now outlines
  the ENTIRE target pane/primary with a 3px palette-highlight frame (new
  src/ui/dropframe.h overlay widget) instead of tinting the thin header
  strip. Gotcha: the overlay must set its own `background: transparent` —
  the theme QSS has a generic `QWidget { background }` rule that otherwise
  paints the overlay opaque and blanks the pane under it (that shipped
  briefly and was caught by the user).
- Link previews now fetched for channels living only in a pane/window —
  card/enqueue logic extracted from appendMessage into shared
  appendPreviewCards(); PreviewController already dedupes by URL.
- Hidden primary column (✕ button) no longer resurrects on every pane
  rearrange/open/close — rebuildPaneLayout captures isHidden before the
  detach (which itself marks widgets hidden) and skips re-showing it.
- Panes on a different server than the active one now highlight THAT
  server's nick (selfNickReFor(host)) instead of the active server's.
- Ctrl+F now works in panes: routes to the focused docked pane's search
  bar; popped-out windows get their own window-scoped shortcut (docked
  panes deliberately don't — two live shortcuts on one key in one window
  makes Qt treat every press as ambiguous).
- Floating pane windows no longer start header drags or accept pane drops;
  both were dead ends that showed a working-looking drag cursor/highlight.
- Pop-out window geometry (size + position) persisted per channel in
  QSettings paneWinGeom/<key>; restored on re-float, saved on close/quit.
  Was a known open item (fixed 820x620 every launch).
- New themes from the user: mactahoe26 + mactahoe26-light (macOS Tahoe
  system colors). Theme count 295 -> 297 across all docs.
- No regressions found; 6/6 tests pass. No release tagged.
-->

<!--
Session 2026-07-10/11 — session close:
- Built/fixed this session, in order: PR #30 review cleanup (#32) — shared
  paneKey()/isChannelName() helpers, SearchBar/NickFilterEdit widgets,
  header-button QSS dedup, duplicate-buffer display fix. Horizontal pane
  splits (#33) — pane_stack_rows config option, panes stack in rows instead
  of columns. Docked-pane drag-to-rearrange properly fixed + sidebar
  decoupled from primary panel (#34) — see full details below.
- Bugs found and fixed: duplicate-buffer display when a channel was docked
  in a pane (#32, pre-existing); pane<->primary drag-to-rearrange could
  corrupt the layout in 3/4-pane arrangements (pre-existing since v0.18.0 —
  first patched with a refuse-the-swap guard in #33, then properly fixed by
  decoupling the sidebar in #34); docked-pane header drag not registering at
  all under genuine Wayland/KDE desktop identification, because the
  pre-existing mechanism relied on QWidget::grabMouse(), which Wayland only
  permits for popup surfaces (#34).
- Also resolved (not an app bug): a stale ~/.bashrc on this dev machine was
  spoofing XDG_CURRENT_DESKTOP=Hyprland and disabling window decorations for
  every terminal-launched app, masking/confusing the real drag bug above.
  Removed; unrelated to the Uplink codebase.
- Explored and deliberately abandoned: pop-out (floating) window
  drag-to-swap between separate top-level windows. Fully built and confirmed
  working, then reverted at user's request — not needed for independent
  floating windows.
- No regressions found. Known open items unchanged: floating-window
  geometry isn't persisted across restarts (reopens at fixed 820x620);
  /calc /8ball /shrug /tableflip bundled scripts, accessibility pass, and
  history-search v2/v3 remain on the backlog (see ROADMAP.md).
-->

<!--
Session 2026-07-10/11 (iv) — Fix docked-pane drag-to-rearrange:
- Root cause of drag-to-rearrange silently not working under this session's
  desktop environment: ChannelPane's header drag used QWidget::grabMouse(),
  which Wayland only permits for popup-type surfaces. Replaced with native
  QDrag/QMimeData -- the sanctioned cross-surface tracking mechanism.
- A second, independent issue surfaced once QDrag was in place: without an
  OS-level grab, once the cursor drifted off the (thin) header strip onto a
  sibling widget (e.g. the topic bar), further move/release events landed
  there instead of the header, starving the drag-start threshold check.
  Fixed by watching mouse move/release app-wide (via qApp) while a drag is
  pending for a given pane, installed/removed just-in-time rather than a
  permanent filter, and guarded against Qt's double-filter-invocation
  pitfall (a single click can legitimately deliver MouseButtonPress twice
  here, once to a header child label, once to the header itself).
- Architecture fix: m_primaryPanel had the sidebar physically embedded
  inside it (setupChatArea's m_mainSplitter held [sidebar, chat section] as
  primary's own child layout), so swapping primary into a shared/stacked
  pane slot squeezed the whole sidebar into a slot never built to hold it.
  Moved the sidebar one level up -- m_mainSplitter now wraps
  [sidebar, panesSplitter] as a permanent sibling, so primary is a
  self-contained pane like any other and can safely land in any slot.
- New reflow behavior for pane <-> primary drag: the dragged pane stays
  exactly where it is; primary takes over its stack-mate's slot instead of
  the dragged pane's own slot, so the stack-mate gets promoted to the
  vacated lone column. Generalizes across 2/3/4-pane layouts via one
  siblingSlot() helper; falls back to a plain swap when the dragged pane
  has no stack-mate.
-->

<!--
Session 2026-07-10 (iii) — Horizontal pane splits:
- New `pane_stack_rows` config option (Preferences -> Interface -> Stack Panes
  in Rows): transposes the docked-pane auto layout so panes stack in
  horizontal rows instead of columns. rebuildPaneLayout() now derives a
  main/cross axis pair from the setting instead of hardcoding
  horizontal-main/vertical-cross; same shapes for 1-4 panes, rotated 90 deg.
  Applies live via the existing rebuildPaneLayout() call, no restart needed.
- Docs: configuration.md (full example + options table), howto.html (Layout
  section + a stale line fixed: left-clicking a pane's channel no longer also
  loads it into primary, per the PR #32 fix — the how-to still described the
  old behavior).
- Explored and abandoned earlier this session: pop-out (floating) window
  drag-to-swap. Fully implemented and confirmed working (QDrag-based content
  swap between floating windows, since Wayland disallows client-side window
  repositioning), then reverted at user's request -- independent floating
  windows don't need this; docked-pane drag-to-rearrange remains as-is and
  gets more useful now that rows mode exists.
- Fix: pane <-> primary drag-to-rearrange (v0.18.0) could corrupt the layout
  in 3/4-pane arrangements. m_primaryPanel physically embeds the sidebar
  (setupChatArea's m_mainSplitter), so it must always land in a full,
  unshared splitter slot -- the old swap logic only moved m_primarySlot
  without relocating the dragged pane, so an uninvolved third pane would
  shift to the wrong slot, and swapping primary into a shared/stacked slot
  squeezed the whole sidebar+chat bundle into it. Rewrote the swap as a true
  slot exchange (every other pane's position is now untouched) and added
  isFullPaneSlot() to refuse any swap that would place primary in a shared
  slot (blocks all 4-pane primary swaps, and 3-pane swaps not targeting the
  one full slot) instead of corrupting the layout.
-->

<!--
Session 2026-07-10 (ii) — PR #30 review cleanup (#32):
- Shared paneKey()/isChannelName() helpers (model/ids.h) replace ~35 hand-rolled
  host|channel key constructions and inconsistent channel-prefix checks that
  had accumulated across ui/ and model/; channel-prefix checks widened to the
  full RFC set (#&+!).
- New SearchBar and NickFilterEdit widgets, shared by MainWindow and
  ChannelPane, replace duplicated search-bar and nick-filter code. Fixes two
  parity gaps: the docked-pane search bar now supports Enter/Shift+Enter
  next/prev (previously main-window only), and the pane nick filter now
  clears on Escape (previously main-window only).
- Fix: a channel already docked in a ChannelPane no longer also loads into
  the primary view when selected from the sidebar (switchToChannel/
  switchAwayFromChannel now check m_panes, not just the popped-out-window map).
- Header-button hover QSS deduped into UiStyle::headerButtonStyle() (13 copies
  -> 1).
- No version bump — internal cleanup + small bug fixes, no new features.
-->

<!--
Session 2026-07-10 — session close:
- Channel panes reach full parity with the main view:
  - Pane user list now has the main view's header (hide/reveal toggle, groups
    icon, live user count) and the "filter users…" box; count tracks
    join/part/quit, filter clears on list rebuild; reveal button floats over
    the chat when the list is hidden (repositions on resize/topic toggle).
  - Pane input height matched the main input: the QSS input padding only lands
    in contentsMargins() after polish, so ChannelPane now polishes before
    measuring (shared updateInputHeight()); height also recomputes on font
    changes.
  - Root cause of "pane fonts bigger than main": reparenting under the app
    stylesheet (dock/float/rebuild all setParent) resets programmatic fonts to
    the app default 12pt (QTBUG-45332), and on Wayland the repolish can land
    asynchronously after any re-apply. Two-layer fix: applyFontSizes() runs
    after rebuildPaneLayout()/floatPane(), and ChannelPane now guards every
    font it owns — on a FontChange that deviates in family/point size it
    re-asserts the configured font (re-entry latch; attribute compare, not
    QFont operator== which never matches resolved fonts).
  - Pane fonts built identically to the main window's (emoji fallback
    families; previously QFont(fam) + Monospace hint).
  - Pane nick header/count use font_nick_dock like the main panel.
- Docs: howto channel-panes notes the pane user-list controls.
- Roadmap: added horizontal pane splits and multi-window drag/arrange.
- Reminder: uplinkbot RAG needs a restart to pick up this session's doc changes.
-->

<!--
Session 2026-07-09 (i) — session close:
- Code review pass over the pop-out window work (PRs #28/#29): 8 finder angles +
  verification; 13 confirmed findings, all fixed:
  - closeEvent now closes floating windows on quit (app no longer stays alive
    headless when the main window closes with a window open, no-tray case);
    ~MainWindow deletes the parentless windows (shutdown leak).
  - Server removal (Manage Servers) now tears down that host's panes/windows —
    new closePanesForHost() helper, also used by onServerClosed; the floating-
    close reselect is skipped during server teardown (no churn).
  - New kMaxPaneWindows = 4 cap in popOutChannel/floatPane (the kMaxExtraPanes
    cap only covered docked panes; windows were unbounded).
  - Typing-state keys lowercased at write+read (restored mixed-case channels
    showed no typing indicator); settings now persist original-case
    host|channel for "panes"/"paneWindows" (window titles keep case).
  - Tray notifications suppressed when the channel's own popped-out window is
    focused; onChannelAdded no longer raises/steals focus for checked-out
    channels on (re)join; Alt+arrow navigation ignored from floating windows;
    checked-out dim color now m_theme.placeholder (was hardcoded #6c7086);
    floatPane skips the redundant re-render when floating a docked pane;
    new panes seed the typing label with current typers.
- Deferred (verified, not fixed): shared keyFor()/isCheckedOut() helper for the
  ~10 hand-rolled pane-key sites; shared SearchBar widget (inputbar/channelpane
  duplication); shared theme-aware header-button style (14 copies of the white
  rgba hover); duplicate-buffer display when primary shows a docked pane's
  channel (partly pre-existing).
- Docs: howto channel-panes limits note the 4-window cap.
- Reminder: uplinkbot RAG needs a restart to pick up this session's doc changes.
-->

<!--
Session 2026-07-09 (h) — session close:
- Feature: pop-out channel windows. Any channel can float into its own top-level
  window — via a new pop-out (picture-in-picture) button in the channel header
  (left of the search magnifier), the same button now added to every pane header,
  or right-click → "Open in Window". Popped-out channels are "checked out" of the
  main view: sidebar row dimmed/italic, clicking raises the window, channel-nav
  skips it; closing the window (exit-PiP icon) returns it. Floating panes stay
  fully live (messages, nick list, typing, tab-complete, search) and persist
  across restarts (settings key "paneWindows").
- Feature: per-pane search + typing indicator. Each ChannelPane header now has a
  search magnifier wired to its own ChatView (Esc closes), and its own typing
  indicator with reserved (non-jumping) space like the main window.
- Refactor: extracted createPane() (shared docked/floating setup) and floatPane()
  (docked pane → window); typing/font/tab loops now iterate m_panes so floating
  panes stay covered.
- UI: pane compose strip (typing + input) painted on bufferBg (#channelPane /
  #paneWindow) so it matches the main chat colour instead of the window colour.
- Icons: added resources/icons/mi-pip.svg + mi-pip-exit.svg with MenuIcons
  pipEnter()/pipExit() builders.
- Site: swapped the docs hero to a 4-shot crossfade (gruvbox-dark, flexor-dark,
  and two new Libera screenshots — docs/assets/kde.png, archlinux.png).
- Docs: howto.html channel-panes section (pop-out windows, per-pane search/typing,
  header buttons), README + docs/index.html feature entries.
- Reminder: uplinkbot RAG needs a restart to pick up this session's doc changes.
-->

<!--
Session 2026-07-09 (g) — session close:
- Themes: trimmed near-duplicate/redundant themes. Removed Discord-80-Saturation
  (near-identical to Discord) and all 11 black-metal-*-base16 variants (a cluster
  of near-identical dark themes named after bands). Bundled count 306 → 295
  (55 originals + 240 base16).
- Docs: updated theme count across README, docs/configuration.md, docs/faq.md,
  docs/index.html, docs/howto.html, docs/quality.html.
- Reminder: uplinkbot RAG needs a restart to pick up this session's doc changes.
-->

<!--
Session 2026-07-09 (f) — session close:
- Theme: added themes/Gently.toml, ported from the KDE "Gently" color scheme —
  deep midnight-blue base (#050e15), pale-cyan text (#d7f1f8), blue accents
  (selection #058bce, link #75aae0), softened red for mentions; op/voice/halfop
  kept in a cool blue→teal ramp since the source palette has no warm tones.
  Shipped in PR #24.
- A "GentlyT" unified-background variant was trialed locally only and discarded;
  not committed.
-->

<!--
Session 2026-07-09 (e) — session close:
- Docs: added a "Footprint" section to quality.html with a ~60 MB resident-memory
  stat (reusing the existing stat-grid/stat-cell styles) alongside a real system-
  monitor screenshot (docs/assets/memusage.png — Uplink at 60.5 MiB, under Chromium
  133.9 and Mailspring 61.7). Copy carries an honest caveat that usage scales with
  open networks/channels/scrollback. Shipped in PR #22, live on Pages.
- Reminder: uplinkbot RAG needs a restart to pick up this session's doc changes.
-->

<!--
Session 2026-07-09 (d) — session close:
- Screenshots landed: shots/mirc-color.png (colored messages in chat) and
  shots/typing-indicator.png (the "…is typing" line); both howto.html figures
  were already wired, checklist entries ticked.
- FreeBSD confirmed on the box (FreeBSD 15.1-RELEASE): /weather /roll /uptime
  /music all run, and /sysinfo GPU now reads "Intel Corporation Ivy Bridge
  mobile GT2 [HD Graphics 4000]" instead of Unknown.
- Reminder: uplinkbot RAG needs a restart to pick up this session's doc changes.
-->

<!--
Session 2026-07-09 (c):
- Fix: /sysinfo showed "GPU: Unknown" on FreeBSD — sysinfoGPU() had no FreeBSD
  branch and fell through to the Unknown default. Added a branch that parses
  `pciconf -lv` (FreeBSD base system) for the display-class (0x03xxxx) device and
  returns "<vendor> <device>". Confirmed on FreeBSD 15.1 (HD Graphics 4000).
-->

<!--
Session 2026-07-09 (b):
- Fix: user scripts failed on FreeBSD — every bundled script (/weather /roll
  /uptime /music) reported "Script timed out (10s limit)". Cause: the runner
  exec'd the script directly via its `#!/bin/bash` shebang, but FreeBSD keeps
  bash at /usr/local/bin, not /bin, so exec failed and was misreported as a
  timeout. Now .sh scripts launch through `bash` resolved from PATH on all
  platforms (was Windows-only), and a genuine failure-to-start now says so
  ("Could not start script — is its interpreter … in PATH?") via waitForStarted.
- FreeBSD support in bundled scripts: uptime.sh and music.sh case statements
  extended to *BSD/DragonFly (were Linux-only, would print "Unsupported
  platform"); shebangs changed to `#!/usr/bin/env bash`. Scripts are overwritten
  on launch, so users get the fixes automatically. Note: bash must be installed
  (pkg install bash).
-->

<!--
Session 2026-07-09 (a):
- Feature: send colored text. A 16-color mIRC picker (Text color + optional
  Background) for the input box, reachable two ways: Ctrl+Shift+K, or right-click
  the input box → Color submenu (added to the standard edit menu). Colors are
  held as QTextCharFormat brushes — same visual-format approach as Ctrl+B/I/U/S —
  and encoded to `\x03fg[,bg]` in inputToIrcText() at send time; format indicator
  shows a colored `A`. Ctrl+K stayed the quick switcher, so the shortcut is
  Ctrl+Shift+K. Palette exposed once via ChatRenderer::mircColor/mircColorIndex
  so input and renderer share the exact 16 hex values (round-trips cleanly). Menu
  built once in makeColorMenu() and reused by both entry points.
  ROADMAP "Send colored text" ticked.
- Docs sweep for colored text: howto.html (Ctrl+Shift+K row + "Applying color"
  subsection + shots/mirc-color.png figure), faq.md (formatting Q extended to
  color + right-click), keyboard-shortcuts.md, README feature row, and a
  shots-checklist.md entry for mirc-color.png (screenshot captured in (d)).
  Note: uplinkbot RAG needs a restart to pick up the doc changes.
- Cleanup: removed 3 stale local AppImages (0.25.32/33/43, ~207 MB, untracked).
-->

<!--
Session 2026-07-08 (h):
- How-to screenshots: added docs/shots/user-metadata.png (the nick-hover tooltip
  showing a display name + avatar). Already wired into howto.html #user-metadata;
  ticked off in docs/shots-checklist.md. Committed + pushed 3be416e.
- No code changes; docs-only.
- Corrected memory (project_overview): uplinkirc.chat is Porkbun Secure Static
  Hosting via GitHub Connect (auto-deploys on push to main; openresty origin),
  and it serves from the REPO ROOT — docs live at uplinkirc.chat/docs/... , so
  how-to shots resolve to /docs/shots/*.png (NOT /shots/...). Verified the new
  screenshot live at uplinkirc.chat/docs/howto.html.
-->

<!--
Session 2026-07-08 (g):
- ROADMAP: added a "Send colored text" backlog item (mIRC color code input —
  rendering is already supported; entry UX still open: Ctrl+K, an input-bar
  picker, or a /color command).
- How-to screenshots: added docs/shots-checklist.md as a tracked working doc
  (was untracked) — the filename manifest + per-shot notes for docs/howto.html.
  Committed + pushed 46446f8.
- Captured the first Tier-1 shot: docs/shots/main-window.png (the anchor image —
  full window: sidebar + chat + nick list). Ticked off in the checklist.
- Tier-1 still to shoot: sidebar.png, channel-panes.png, first-launch.png.
  howto.html figures stay held until enough PNGs exist to avoid broken images.
-->

<!--
Session 2026-07-08 (f):
- Manage Servers dialog layout fix. The whole form was shoved far to the right of
  the server list with a big empty band beside the list. Root cause (found by
  rendering the dialog offscreen and dumping widget geometry, not by eyeballing):
  the Add/Remove/▲/▼ button bar's preferred width exceeded the 180px list, so
  nothing constrained the left column and it ballooned to ~360px, pushing the
  form panel right. Fix: wrap the list + buttons in a fixed-width (230px) left
  container, give the ▲/▼ arrows a compact fixed width (34px) so all four buttons
  fit without clipping "Remove", and left-align the form labels with a small
  left margin. Also shortened the over-long "Disabled — keep in config…" checkbox
  to "Disabled" + tooltip.
  Lesson: for GUI layout bugs, render the widget (offscreen QWidget::grab) and
  read actual geometry instead of guessing at margins.
- Preferences dialog: added a Close button (QDialogButtonBox) at the bottom.
  Settings already apply live, so a single Close is all it needs; previously the
  only way out was the window's X.
- Still held / not committed: docs/howto.html screenshot scaffolding (pending the
  PNGs) and docs/shots-checklist.md (untracked working doc). See session (e).
-->

<!--
Session 2026-07-08 (e):
- Added .gitattributes to correct the GitHub Languages bar, which was reporting
  HTML/Python/Shell alongside C++. Marked docs/** + index.html as
  linguist-documentation, scripts/* + packaging/* as linguist-vendored, and
  src/ui/emojidata.h as linguist-generated so the bar reflects the C++/Qt client.
  Pushed 2191734 (bar recomputes on GitHub's side after the push).
- How-to screenshots — scaffolding in place, images pending. Added a framed +
  captioned figure.shot style (uses existing --surface/--subtle/--muted vars so it
  adapts to light/dark) and 42 <figure> blocks throughout docs/howto.html, one per
  visual feature, each pointing at docs/shots/<name>.png with loading="lazy".
  Split into Tier 1 (essential, 18) and Tier 2 (24). Shots are Nord for
  consistency, varying theme only on the theming/app-icon sections.
  NOT committed to main yet — held back so the live site doesn't show broken
  images until the PNGs exist. Filename manifest + per-shot "shoot this" notes
  live in docs/shots-checklist.md (kept untracked as a working doc). howto.html
  and its images get committed together once the screenshots are captured.
-->

<!--
Session 2026-07-08 (d):
- Post-release fix (user-reported): quality.html still showed v0.25.56 in the page eyebrow
  and footer. Root cause: release.sh and sync-site.sh only bumped index.html + README —
  quality.html was never in either file list, so its version lagged every release. Bumped
  to v2026.7.0 and added docs/quality.html to both scripts (release.sh uses targeted
  patterns that leave the Qt/GCC test versions "6.11.1"/"16.1.1" alone; sync-site's
  head -1 grab is safe because the eyebrow version sits above the test output).
- Fixed disappearing AI nav bot icon: index.html's "AI" nav link had the bot SVG but
  quality.html's was bare text, so the icon appeared to vanish when navigating between the
  two pages. Gave quality.html the same icon. (howto.html uses a different doc-sidebar nav
  with no AI link — no inconsistency there.)
- Pushed e1caa90; verified live on uplinkirc.chat (0 stale version strings, icon present).
-->

<!--
Session 2026-07-08 (c):
- RELEASED v2026.7.0 — first release under the new CALENDAR VERSIONING scheme
  (year.month.fix): first release of a month is 2026.M.0, urgent same-month fixes bump
  the last digit. Chosen by the user over semver/1.0; supersedes 0.25.x numbering.
  Update checker verified to parse the jump (numeric compare, 2026 > 0). Always use
  THREE components (e.g. 2026.7.0, never 2026.7) — the updater regex requires X.Y.Z.
  Released via scripts/release.sh 2026.7.0 (script needed zero changes); direct push to
  main works for release commits. Release notes hand-written (auto-generated ones missed
  the full-history search feature and the scheme explanation). All 5 assets published,
  releases/latest verified.
- Versioning documented: CONTRIBUTING (Versioning section), FAQ (what version numbers
  mean), SECURITY.md (how fixes are versioned), CLAUDE.md (release flow).
- Docs staleness sweep alongside: SECURITY.md had the old project name "DojoIRC", a wrong
  maintainer link (@uplink), and claimed passwords sit in plaintext config (stale since
  keychain migration). FreeBSD port Makefile was pinned to 0.23.2 with LICENSE=MIT on a
  GPLv3 repo — all fixed. quality.html "77 releases" claim dropped (stale since the June
  release cleanup).
- Site polish (uplinkirc.chat, reviewed via headless-chromium screenshots at 1440px and
  390px): hero eyebrow pill wrapped mid-token on phones (fixed with nowrap spans +
  flex-wrap); download cards gained per-platform notes so the grid fills evenly next to
  the tall macOS card; IRCv3 section said 21 caps vs hero's 37 (aligned to the 37
  documented in ircv3.md — that number = count of ### entries there); quick-start tables
  now scroll/wrap on mobile (Windows config path was clipped).
- Hero crossfade slowed: 15s cycle/~1s fade → 21s cycle/2s ease-in-out fade, same ~5s
  hold per screenshot; delays scale to -7s/-14s so fades stay aligned. Verified by
  freezing the animation mid-fade (animation-play-state:paused + negative delay) and
  screenshotting the blend. Reduced-motion exemption unaffected (it never kills
  animations, only transitions).
- uplinkirc.chat serves the repo docs (openresty, root redirect → /docs/), syncs from
  main within ~3 minutes of a push. Verified all changes live.
- Reminder for next session: uplinkbot needs a restart to pick up the new FAQ/howto/
  configuration content (highlight words, versioning).
-->

<!--
Session 2026-07-08 (b):
- Fix: highlight words never actually highlighted in the chat view. The regex was built as a
  bare \bword\b alternation with no capture group, but the renderer highlights capture group 1
  (addSelfNickHighlight reads capturedStart(1), which returns -1 with no group → zero-length
  segment → no visual effect). Found while writing the renderer fuzz harness. Fixed by
  consolidating the three duplicate regex builders (two in mainwindow.cpp, one in
  sessionmodel.cpp) into SessionModel::buildHighlightRe(), which wraps the alternation in a
  group. Two regression tests added to tst_chatformat (builder contract + end-to-end red/bold
  segment). Mention detection (hasMatch) was unaffected — only the visual highlight was broken.
- New fuzz target: tests/fuzz_chatformat.cpp covers the ChatRenderer formatting path
  (formatMessageLine/formatMessage/formatEventGroupLine/linkifyTopic/ircToHtml/wrapEmojiHtml)
  with hostile input — mIRC colour codes, unterminated sequences, emoji/ZWJ runs, RTL overrides,
  invalid UTF-8, redactions, replies. 11 seeds in tests/corpus_chatformat/. Ran under libFuzzer
  + ASan/UBSan: 149k executions, no crashes (parser: 540k, no crashes).
- Fix: the fuzz corpus replay smoke test in CI was replaying 0 inputs — the standalone driver
  couldn't open a directory argument, so fuzz_ircparser_corpus passed while testing nothing.
  Both drivers now expand directory args (libFuzzer semantics) and exit 1 on zero inputs.
  tests/CMakeLists.txt fuzz section refactored into a shared uplink_fuzz() function.
- RAM/startup: emojidata.h rewritten from ~1900 heap-allocated QString pairs (built on first
  use, plus a full duplicate QHash copy) to an inline constexpr QStringView table in
  .data.rel.ro (~61KB read-only, verified with readelf). emojiByCode() hash replaced by
  emojiForCode() linear scan — only runs on user typing/send. scripts/generate_emojidata.py
  updated to emit the new format so regeneration doesn't revert it.
- MainWindow shave: one-time construction/wiring (setupToolbar, connectPreferences,
  setupSidebar, setupNickPanel, setupChatArea, connectModel) split to mainwindow_setup.cpp
  following the chatupdates.cpp idiom; NickDelegate/SidebarDelegate/RoundedPane moved to
  mainwindowdelegates.h; five file-local icon builders folded into MenuIcons (topicBubble,
  groups added; gear/hamburger/connectedServer wrappers inlined; makeSvgIcon replaced by the
  cached DPR-aware MenuIcons::fromSvg). mainwindow.cpp 3200 → 1784 lines. Behavior-neutral;
  icons should render identically (crisper on HiDPI due to fromSvg).
- Known dead code left in place (not removed without asking): FixedRowDelegate in mainwindow.cpp.
- Docs: quality.html stats corrected (assertions 90 → 198, fuzz inputs 26 → 61, chatformat
  tests 15 → 17) and fuzz terminal now shows both harnesses; CONTRIBUTING.md gained a
  beginner-level "running the tests" + fuzzing walkthrough; CLAUDE.md architecture/invariants
  updated (mainwindow split, buildHighlightRe capture-group invariant, emojidata is generated).
- No release tagged.
-->

<!--
Session 2026-07-08:
- Docs site (docs/index.html) only — no app change, no release tagged.
- Hero crossfade "not fading" was a chain of issues, finally root-caused to the user's
  KDE reduce-motion setting. The reduced-motion reset '* { animation: none !important }'
  killed the opacity crossfade, so all three stacked screenshots showed at opacity:1 at
  once (shorter light theme sitting on a darker one). Firefox (unlike Chromium) will NOT
  let a more-specific !important rule re-enable an animation the universal reset disabled,
  so the "keep it running" override worked in Chrome but not Firefox. Fix: don't blanket
  animation:none under reduce-motion — heroFade is the page's only keyframe animation and
  is motion-free, so only transitions are killed now. Verified in Firefox headless with
  ui.prefersReducedMotion=1.
- Also fixed along the way: switched the hero frame from CSS aspect-ratio (Firefox sizes
  it too tall with border-box + border) to the padding-top ratio technique, which is
  computed identically across browsers.
- Screenshots: nord, gruvbox-light, harmonic16, humanoid-light were captured with the app
  window on a solid black macOS desktop, so they showed a dark square on the cream page.
  Replaced with transparent retina captures cropped to the window (1016x666).
- Hero letterbox: the crossfade mixed two screenshot shapes (1016x824 gruvbox-dark/dracula
  vs 1016x666 gruvbox-light), so the shorter one letterboxed in the fixed-ratio frame.
  Rebuilt the hero from three same-shape clean captures (nord, gruvbox-light, harmonic16)
  and sized the frame to 1016x666 so every layer fills it. gruvbox-dark/dracula remain in
  the themes gallery. Themes gallery still mixes shapes but shows one-at-a-time (no frame),
  so no letterbox; making the whole set uniform would mean re-shooting the six originals
  as transparent captures.
- Reference note saved: Firefox aspect-ratio + reduce-motion gotchas.
-->

<!--
Session 2026-07-07:
- Fix: auto-reconnect stopped retrying after the first failed attempt, so clients never
  came back on their own once a downed server returned (users had to reconnect manually).
  Root cause: the reconnect loop relied solely on Qt's disconnected() to re-arm, but Qt
  only emits that for a socket that reached ConnectedState — a failed connection *attempt*
  (server still down: ConnectionRefused/timeout) emits only errorOccurred. onErrorOccurred
  now calls scheduleReconnect() when the socket ends up UnconnectedState; scheduleReconnect()
  is idempotent (guards on m_intentionalDisconnect + timer active) so genuine drops don't
  double-schedule. Works for both TCP and WebSocket transports. ircclient.cpp only.
- Merged as PR #13 (squash, admin-merge past branch protection since self-approval isn't
  possible on a solo repo). CI green on ubuntu/windows/macos + sanitize.
- No release tagged. FAQ already documented auto-reconnect as working; this fix makes that true.
-->

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

<!--
Session 2026-07-03:
- ChatView perf landed: per-line QTextLayout cache, incremental width relayout, deferred chunked
  relayout on resize. Fixes for relayout staleness, preview-queue leak, Tab-at-column-0 guard.
- MainWindow decomposition: UpdateChecker, DccController, PreviewController extracted;
  input bar and chat-view update logic split to inputbar.cpp / chatupdates.cpp.
- Perf: batched nick-index rebuild on netjoin bursts (addNicks mirrors removeNicks) — O(m*n) → O(m+n log n);
  prependMessages no longer copies the whole buffer on CHATHISTORY backfill.
- Unified IRC format-code parser; fuzzer corpus expanded (24 new cases).
No regressions. No known issues. Version bumped 0.25.55 → 0.25.56.
-->

<!--
Session 2026-07-06:
- /sysinfo Linux CPU/MEM: /proc/cpuinfo and /proc/meminfo report size 0, so the QTextStream
  !atEnd() loops never ran — CPU fell back to the bare arch, MEM to Unknown. Now read line-by-line
  until readLine() returns null. os-release (real file) and uptime (direct readLine) were unaffected.
- /sysinfo GPU: prefer a hardware device over a software renderer (llvmpipe/lavapipe/softpipe/swrast),
  falling back to the first device when only software is present.
- Full-history search (Ctrl+Shift+F): new LogSearchDialog scans the current buffer's on-disk log on a
  worker thread (bounded to the newest 1000 matches, cancellable), substring or regex, newest-first.
  Shows a note when logging is off. Added SessionModel::logFilePath / messageLoggingEnabled. Existing
  Ctrl+F in-buffer find untouched. Docs updated (keyboard-shortcuts, faq, howto, index.html).
No regressions; 5/5 tests pass. No release tagged.
-->

## v2026.7.1 — 2026-07-11

- Theming overhaul: a theme's `[sidebar]` and `[nicklist]` backgrounds are now honored — 285 of 297 themes gain a layered, two-tone look. The side panels run the full window height and are drawn as cards with rounded top corners
- New **Preferences → Interface → Panel Cards** toggle: switch between the new card look and the classic flat rendering, live, per your taste (`panel_cards` in config)
- Full-history search v2: tick **All buffers** in the Ctrl+Shift+F window to search every logged channel and PM across all your servers at once — results are grouped per buffer with match counts; double-click one to jump straight to that buffer
- Detachable panes matured: header drag-to-rearrange now works reliably on Wayland/KDE, panes can stack in rows (`pane_stack_rows`), the drop target highlights with a full frame, Ctrl+F reaches docked panes, pop-out windows remember their size and position, and channels open in a pane no longer accumulate phantom unread badges
- Pop-out windows keep their themed colors after undocking, and panes on a different server than the active one now highlight the right nick
- New themes: mactahoe26 and mactahoe26-light (macOS Tahoe system colors) — 297 themes total
- Fix: the user list stopped a few pixels short of the window bottom on fractionally scaled displays (125–150%); side panels now render flush edge to edge
- Fix: DCC sends longer than 60 seconds were aborted as "stalled" — the guard now tracks transfer progress instead of a fixed deadline, and passive sends get the same protection
- Fix: reactions with fabricated message ids could grow memory without bound over a long session; reactions are only accepted for messages actually in the buffer
- Fix: the show-user-list button now appears right where the hide button was instead of jumping below the topic bar, and topic text wraps before it instead of running underneath
- Fix: ☰ → Reload Config no longer appends a duplicate program path to the process arguments on every reload
- Hardening: full audit of the security-sensitive paths (SSRF guard, DCC, TLS, CTCP parsing) and memory caps — everything else came back clean

## v2026.7.0 — 2026-07-08

> **New version scheme:** Uplink now uses calendar versioning — `year.month.fix`.
> The first release of a month is `2026.M.0`; urgent fixes within the same month
> increment the last digit. This release supersedes the old `0.25.x` numbering.

- Full-history log search: Ctrl+Shift+F searches your entire on-disk log for the current buffer, not just the loaded scrollback
- Fix: auto-reconnect kept retrying after a failed connection attempt — previously a server that was down when the first retry fired was never retried again, and you had to reconnect manually
- Fix: highlight words are now actually highlighted — keywords set in Preferences → Notifications → Highlight Words render red bold in chat, as always intended (mention counting was unaffected)
- Fix: /sysinfo reads CPU and memory info correctly on Linux and prefers the hardware GPU over a software renderer
- Lower memory use and faster startup: the emoji table (1,906 entries) moved from heap-built structures to read-only static data
- Hardening: a second fuzz harness now hammers the chat renderer with hostile input (mIRC codes, emoji sequences, RTL overrides, invalid UTF-8) — 149k sanitized executions, no crashes; the CI fuzz-corpus replay was also fixed to actually replay its inputs
- Internal: main window construction split into a dedicated setup unit; icon builders unified with HiDPI-aware caching

## v0.25.56 — 2026-07-03

- Smoother chat scrolling: per-line layout caching plus incremental and deferred relayout keep busy channels responsive
- Fix: chat view could keep a stale layout after resize in some cases; corrected
- Fix: link-preview queue no longer leaks its entry budget over a long session
- Fix: pressing Tab at the start of an empty input no longer clobbers text
- Topic "set by" label now shows just the nick instead of the full user@host mask
- Faster nick-list updates when a batch of users returns after a netsplit (netjoin), and lighter history backfill
- Internal: main window split into focused controllers (update checker, DCC, link previews, input bar, chat updates); unified IRC formatting-code parser with an expanded fuzz-test corpus

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
