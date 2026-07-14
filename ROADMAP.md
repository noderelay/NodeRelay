# Uplink Roadmap

A fast, secure, IRCv3-featured IRC client built with Qt6 and C++.
Default network: **irc.linuxdojo.org:6697** — channel **#uplink**

Overhauled 2026-07-14. Everything shipped through v2026.7.5 is summarized by area under **Completed**, with notes on how complete each area actually is. The old item-by-item checklist (roughly 450 entries) lives in this file's git history.

---

## Planned — Qt6 / C++ modernization

We're on Qt 6.11 but still writing a lot of Qt 5-era code. These are internal quality items, no user-visible features.

- [ ] C++20 bump — raise `CMAKE_CXX_STANDARD` to 20. One-line change, unlocks designated initializers, ranges, and Qt's `std::format` integration for Qt types. Prerequisite for a couple of items below.
- [ ] IrcParser allocation pass — `parseLine()` splits every incoming line into a `QStringList`. Rework the hot path on `QStringView` slices and `qTokenize` (zero-allocation splitting), materializing `QString` only at the edges. Benchmark before/after on a busy-channel replay.
- [ ] Encoding fallback — Qt 6 dropped QTextCodec; we decode incoming lines as UTF-8 unconditionally. Use `QStringDecoder` to detect invalid UTF-8 per line and fall back to Latin-1, so legacy clients don't render as mojibake. Interacts with the existing UTF8ONLY handling.
- [ ] QFuture continuations for history search — full-history search currently uses manual worker-thread plumbing. `QtConcurrent::run(...).then(this, ...)` gives off-thread scanning with results delivered back on the GUI thread for free. Do this as part of search v3 rather than as a standalone rewrite.
- [ ] `Qt::StringLiterals` adoption — `u"..."_s` / `"..."_L1` instead of `QStringLiteral` / `QLatin1String` in new code; convert existing call sites opportunistically when touching a file, not as a big-bang sweep.
- [ ] std::chrono timeouts — `QTimer::singleShot(250ms, ...)`, `setTransferTimeout(10s)` etc. in reconnect, typing-debounce, and DCC timeout code. Ergonomic only, do alongside other edits.
- [ ] Link preview transfer timeout audit — confirm every `QNetworkRequest` in PreviewController and UpdateChecker sets a transfer timeout; a stalled server must not hold a queue slot indefinitely.
- [ ] QLoggingCategory — `uplink.irc`, `uplink.dcc`, `uplink.preview` categories so protocol debugging is runtime-switchable via `QT_LOGGING_RULES` instead of recompiling with qDebug.
- [ ] Color emoji rendering check — Qt 6.8+ ships a proper emoji segmenter (ZWJ sequences, skin tones). Verify ChatView renders combined emoji correctly and remove any workarounds that predate it.

## Planned — Features

- [ ] Full-history search v3 — CHATHISTORY context jump: open a search result in its buffer with surrounding history fetched from the server, not just the log line.
- [ ] Accessibility — QAccessibleInterface for ChatView so screen readers can read chat.
- [ ] Long-press context menus — touch-friendly alternative to right-click for tablets.
- [ ] More bundled scripts — /calc, /8ball, /shrug, /tableflip, etc.
- [ ] Spellcheck — hunspell integration for the input box (on hold).

## Planned — DCC

The one area of the old roadmap that isn't actually finished. Transfers work on LAN and in the one-side-NATed case (passive mode); both-sides-NATed has no path.

- [ ] NAT traversal for the both-sides-NATed case — evaluate UPnP/NAT-PMP port mapping on the listening side; if we don't build it, promote the current Known Issues text into user-facing docs and call the limitation intentional.
- [ ] External IP discovery for active DCC — active mode advertises the local interface address, which is wrong behind NAT even when the port is forwarded.

---

## Completed

How complete "complete" is, by area. Detail is in git history of this file and in CHANGELOG.md.

**Core protocol & connection** — done. TLS with real certificate verification and per-server TOFU fingerprint pinning, SASL PLAIN + EXTERNAL, NickServ auto-identify, STS, SOCKS5 proxy, WebSocket (wss://) transport, soju/ZNC bouncer support, auto-reconnect with backoff, ping watchdog. No known gaps.

**IRCv3** — effectively full coverage: server-time, message-tags, batch, labeled-response, echo-message, msgid, cap-notify, account-notify, account-tag, extended-join, chghost, invite-notify, setname, userhost-in-names, WHOX, MONITOR, standard replies, UTF8ONLY, netsplit/netjoin batches, chathistory, plus the draft specs (typing, reply, react, message-redaction, multiline, metadata-2, no-implicit-names). Caveat: the draft/* specs track moving targets; revisit when they ratify or when Ergo changes behavior.

**UI** — done and stable. Menu bar (v2 with Bookmarks), detachable/pop-out channel panes with drag rearrange and persistence, custom virtual-scrolling ChatView, model-based nick list, 297 themes, redesigned Preferences, emoji picker (Unicode 16.0), reactions/replies/redaction, link preview cards, mIRC formatting input, font zoom, quick switcher, in-buffer search plus full-history log search v1/v2 (v3 planned above).

**DCC file transfer** — mostly done: active and passive send/receive, progress UI, and the full hardening list (offer validation, size caps, filename sanitization, peer checks, timeout/cleanup races). Incomplete: NAT handling, see Planned — DCC.

**Security** — backlog cleared as of 2026-06: outbound injection prevention, credential redaction across all paths, OS keychain storage, SSRF guard with DNS pre-check, inbound DoS bounds, URL scheme guard, CodeQL and ASan/UBSan in CI. Treated as ongoing work, not a finished list.

**Performance & maintainability** — the 2026-06/07 review backlogs are fully cleared: MainWindow split into controllers (~5,300 → ~3,200 lines), ChatView rewritten on QTextLayout with cached layouts, virtualized nick list, memory caps + glibc arena tuning (idle RSS roughly halved), unit tests (parser, chat format, config, ignore) and libFuzzer targets in CI.

**Packaging & distribution** — CI and release builds on Linux, Windows, macOS; AppImage with desktop self-integration and in-place auto-update; release automation via scripts/release.sh; CalVer since v2026.7.0. Caveat: the FreeBSD port skeleton (`packaging/freebsd/`) was never submitted upstream — it still needs `make makesum` and qt6-keychain port verification.

**Scripting** — user script bindings (Preferences → Scripts) with sandboxed QProcess execution; four bundled scripts (/music, /weather, /uptime, /roll) working on Linux, Windows, macOS. More bundled scripts planned above.

**Docs & site** — full docs (configuration, commands, FAQ, IRCv3, shortcuts, howto), GitHub Pages site with theme cycler, README. Current through v2026.7.5.

---

## Known Issues

- DCC over internet (active mode) — active DCC advertises your local IP; if both sides are behind NAT the connection still fails. Use **Send File (Passive)** so the receiver opens the port instead.
- DCC passive receive over NAT — the receiver's port must be reachable from outside. If the receiver is also behind NAT, passive DCC will not work either (both sides blocked). No relay mechanism implemented.
