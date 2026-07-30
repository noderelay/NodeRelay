# Uplink Roadmap

A fast, secure, IRCv3-featured IRC client built with Qt6 and C++.
Default network: **irc.libera.chat:6697** — channel **#uplinkirc**

Overhauled 2026-07-14. Everything shipped through v2026.7.6 is summarized by area under **Completed**, with notes on how complete each area actually is. The old item-by-item checklist (roughly 450 entries) lives in this file's git history.

---

## Planned — Qt6 / C++ modernization

We're on Qt 6.11 but still writing a lot of Qt 5-era code. These are internal quality items, no user-visible features. The C++20 bump landed 2026-07-15; new-standard idioms (designated initializers, ranges, `Qt::StringLiterals`, chrono) get adopted opportunistically when touching a file, not as a sweep.

- [ ] IrcParser allocation pass — `parseLine()` splits every incoming line into a `QStringList`. Rework the hot path on `QStringView` slices and `qTokenize` (zero-allocation splitting), materializing `QString` only at the edges. Benchmark before/after on a busy-channel replay.
- [ ] QFuture continuations for history search — full-history search currently uses manual worker-thread plumbing. `QtConcurrent::run(...).then(this, ...)` gives off-thread scanning with results delivered back on the GUI thread for free. Search v3 shipped without it, so this is now a standalone cleanup — do it opportunistically.
- [ ] `Qt::StringLiterals` adoption — `u"..."_s` / `"..."_L1` instead of `QStringLiteral` / `QLatin1String` in new code; convert existing call sites opportunistically when touching a file, not as a big-bang sweep.
- [ ] std::chrono timeouts — `QTimer::singleShot(250ms, ...)`, `setTransferTimeout(10s)` etc. in reconnect, typing-debounce, and DCC timeout code. Ergonomic only, do alongside other edits.
- [ ] Color emoji rendering check — Qt 6.8+ ships a proper emoji segmenter (ZWJ sequences, skin tones). Verify ChatView renders combined emoji correctly and remove any workarounds that predate it.

## Planned — Features

- [ ] Input byte counter in pane inputs — the main input got it in #92; channelpane.cpp inputs still don't have it.
- [ ] Accessibility — QAccessibleInterface for ChatView so screen readers can read chat.
- [ ] Long-press context menus — touch-friendly alternative to right-click for tablets.
- [ ] More bundled scripts — /calc, /8ball, /shrug, /tableflip, etc.
- [ ] Spellcheck — hunspell integration for the input box (on hold).

## Planned — Distribution

- [ ] winget — Uplink is still not published upstream. `manifests/n/NodeRelay/Uplink` 404s in microsoft/winget-pkgs, and both PRs have sat open since 2026-07-17: #403545 (new package, 2026.7.6) and #403562 (version, 2026.7.7). The version PR depends on the package PR, so stacking further versions on top makes the queue worse. Wait for a moderator to merge the base package, then submit one clean version PR for the current release. AUR (×3) and the Homebrew tap are current and need no action.

## Planned — DCC

Mostly finished as of 2026-07-27. Transfers work on LAN (opt-in via `[dcc] allow_lan` or Preferences → File Transfers, off by default; verified between two real machines 2026-07-29), in the one-side-NATed case (passive mode), and through a router with port forwarding (`[dcc]` external IP + port range, with automatic external-IP discovery when the network reveals it — this path is not yet tested against a real outside peer). Both-sides-NATed without port forwarding still has no path.

- [ ] NAT traversal for the both-sides-NATed case — evaluate UPnP/NAT-PMP port mapping on the listening side; if we don't build it, promote the current Known Issues text into user-facing docs and call the limitation intentional.

---

## Completed

How complete "complete" is, by area. Detail is in git history of this file and in CHANGELOG.md.

**Core protocol & connection** — done. TLS with real certificate verification and per-server TOFU fingerprint pinning, SASL PLAIN + EXTERNAL, NickServ auto-identify, STS, SOCKS5 proxy, WebSocket (wss://) transport, soju/ZNC bouncer support (ZNC network attach via SASL `user/network` fixed 2026-07-16), auto-reconnect with backoff, ping watchdog, Latin-1 fallback for invalid-UTF-8 lines (2026-07-15). No known gaps.

**IRCv3** — effectively full coverage: server-time, message-tags, batch, labeled-response, echo-message, msgid, cap-notify, account-notify, account-tag, extended-join, chghost, invite-notify, setname, userhost-in-names, WHOX, MONITOR, standard replies, UTF8ONLY, netsplit/netjoin batches, chathistory (scrollback uses timestamp bounds since 2026-07-16 — soju rejects msgid bounds), plus the draft specs (typing, reply, react, message-redaction, multiline, metadata-2/-3, read-marker) and no-implicit-names in both its ratified and soju.im/ forms (2026-07-23, verified against Ergo 2.19.0). Metadata is hover-fetched on demand with retry on rejoin/failed lookup (2026-07-16); metadata-3 is preferred over -2 when both are offered. Status text shipped 2026-07-24 (/status + Profile field, italic tooltip line, cross-client verified). Channel avatars shipped 2026-07-24: ops set one with `/chanavatar`, members see it as the channel's sidebar icon, live-updating, with fetch failures reported in-buffer (persists only on ChanServ-registered channels — Ergo drops unregistered channel state when the room empties). Read markers are wired to unread badges as of 2026-07-23: reading a buffer advances the marker (coalesced sends), and a marker from another client clears the badge once it covers the buffer. Since 2026-07-25 marks only go out while an Uplink window has focus — a minimized client no longer wipes unread state on other devices; queued marks flush on refocus. Caveat: the draft/* specs track moving targets; revisit when they ratify or when Ergo changes behavior. Bouncer caveat: neither soju nor ZNC passes the metadata cap through, so metadata features light up on direct connections only. Both sides are optional as of 2026-07-24: `metadata = false` per server skips the capability entirely, and `show_avatars = false` keeps metadata but never fetches avatar images (an avatar URL is set by someone else, so the fetch exposes your IP to their host).

**UI** — done and stable. Menu bar (v2 with Bookmarks), detachable/pop-out channel panes with drag rearrange and persistence (rebuilt on a split tree 2026-07-24: any nesting, panes capped by readable space rather than a shape table, and the whole layout including divider positions restored on launch; hardened 2026-07-25 by a full quality sweep — pane-lifetime crash fixes, buffer routing that can't land in a hidden view, layout save/restore under test, and the sidebar highlight following keyboard focus), custom virtual-scrolling ChatView, model-based nick list, 297 themes, redesigned Preferences, emoji picker (Unicode 16.0), reactions/replies/redaction, link preview cards, mIRC formatting input with a byte counter near the wire limit (2026-07-16), per-buffer input drafts — unsent text survives channel switches (2026-07-18), font zoom, quick switcher, in-buffer search plus full-history log search v1/v2/v3 — v3 adds the context jump, opening a hit in place in its buffer's scrollback (shipped v2026.7.8, field-verified 2026-07-24).

**DCC file transfer** — done except UPnP: active and passive send/receive, progress UI, the full hardening list (offer validation, size caps, filename sanitization, peer checks, timeout/cleanup races), and NAT support as of 2026-07-27 — `[dcc]` config (advertised external IP, forwardable port range, LAN opt-in) plus best-effort external-IP discovery from the network's visible-host reply. Remaining: both-sides-NATed with no port forwarding, see Planned — DCC.

**Security** — backlog cleared as of 2026-06: outbound injection prevention, credential redaction across all paths, OS keychain storage, SSRF guard with DNS pre-check, inbound DoS bounds, URL scheme guard, CodeQL and ASan/UBSan in CI. Treated as ongoing work, not a finished list. Transfer-timeout audit done 2026-07-15: every outbound `QNetworkRequest` (previews, update check + download, avatar fetches) now sets an inactivity timeout. Avatar fetches hardened same day: full SSRF guard (scheme gate, DNS pre-check, IP-pinned request — shared with link previews via `net/addresscheck.h`), 1 MB size cap with dimension-gated decode, and local-path avatars honored only for the user's own profile.

**Performance & maintainability** — the 2026-06/07 review backlogs are fully cleared: MainWindow split into controllers (Preview, Dcc, UpdateChecker, and since 2026-07-18 Typing and Sidebar; per-buffer cache bundle and PaneManager are the remaining candidates), ChatView rewritten on QTextLayout with cached layouts, virtualized nick list, memory caps + glibc arena tuning (idle RSS roughly halved), unit tests (parser, chat format, config, ignore) and libFuzzer targets in CI. Builds as C++20 since 2026-07-15. Runtime-switchable protocol logging via QLoggingCategory (`uplink.irc` / `uplink.dcc` / `uplink.preview`) since 2026-07-14.

**Packaging & distribution** — CI and release builds on Linux, Windows, macOS; AppImage with desktop self-integration and in-place auto-update; release automation via scripts/release.sh; CalVer since v2026.7.0. On the AUR since 2026-07-16: uplink-irc (release, source), uplink-irc-bin (prebuilt), uplink-irc-git (main HEAD); release.sh bumps the first two, checksums are refreshed at AUR push time (packaging/archlinux/README.md). Also since 2026-07-16: Homebrew tap noderelay/homebrew-uplink (arm64 cask, not notarized — Apple Developer notarization is a known open option at $99/yr) and winget manifests (NodeRelay.Uplink) submitted to microsoft/winget-pkgs; per-release update recipes in packaging/homebrew and packaging/winget. App identity is the reverse-DNS AppStream id `io.github.noderelay.UplinkIRC` since 2026-07-16 — Wayland app id, desktop entry, hicolor icon, and (since 2026-07-17) the macOS bundle identifier all match, so icon themes shipping the Introversion game's `uplink` icon can no longer shadow ours (KIconLoader's dash-stripping fallback made any dashed name collide). Caveat: the FreeBSD port skeleton (`packaging/freebsd/`) was never submitted upstream — it still needs `make makesum` and qt6-keychain port verification.

**Scripting** — user script bindings (Preferences → Scripts) with sandboxed QProcess execution; four bundled scripts (/music, /weather, /uptime, /roll) working on Linux, Windows, macOS. More bundled scripts planned above.

**Docs & site** — full docs (configuration, commands, FAQ, IRCv3, shortcuts, howto), GitHub Pages site with theme cycler, README. Current through v2026.8.1 plus a full accuracy audit 2026-07-27.

---

## Known Issues

- Accepting a new server certificate reverts UI toggles changed since launch — `SessionModel::pinCertificate()` saves the model's own config copy, whose `[ui]` section never learns about toggles flipped while the app runs, so a TOFU accept writes launch-time values back to config.toml (found 2026-07-29 tracing a "missing unread counters" report). Fix: the pin should read-modify-write only the fingerprint instead of saving the whole stale copy.
- DCC over internet (active mode) — behind NAT, active DCC needs the `[dcc]` config block (external IP or auto-discovery, plus a forwarded port range); without it the advertised address is your LAN one. If both sides are behind NAT with no forwarding, the connection fails either way — use **Send File (Passive)** so the receiver opens the port instead.
- DCC passive receive over NAT — the receiver's port must be reachable from outside. If the receiver is also behind NAT, passive DCC will not work either (both sides blocked). No relay mechanism implemented.
- KDE global menu "Search" is broken (not Uplink's bug) — on Plasma Wayland the appmenu applet appends its own non-removable Search entry after Uplink's menus, and since Plasma ~6.6.3 it does nothing when used ([KDE bug 518161](https://bugs.kde.org/show_bug.cgi?id=518161), confirmed, unfixed as of 6.7.2). Uplink's DBusMenu export is complete and correct (verified with busctl GetLayout/AboutToShow against a live instance), so the search will find Uplink's menu actions with no app-side changes once KDE fixes the applet. Not to be confused with Uplink's own **Find** menu, which was renamed from "Search" precisely to avoid colliding with the Plasma entry.
