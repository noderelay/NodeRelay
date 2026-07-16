# How-To screenshot checklist

Save each file into `docs/shots/` using the **exact** filename; the how-to page already
points at every one of these. Capture in **Nord** (to match the landing hero) except the
few marked _(vary theme)_.

Tick each off as you shoot it.

---

## Tier 1: essential (shoot these first)

- [x] **about-dialog.png**: Help → About Uplink dialog (version, Qt build, license). You can just re-save the existing `docs/about.png`.
- [x] **main-window.png**: The whole window: sidebar + chat area + nick list in one shot. This is the anchor image.
- [x] **manage-servers.png**: The Manage Servers dialog with two or three servers in the list, one selected so the settings form shows.
- [ ] **sidebar.png**: Just the left sidebar: a connected server (globe), a few channels, at least one with an unread/mention indicator.
- [x] **channel-header.png**: The header row across the top of chat: signal bars, topic toggle, `#channel +modes`, topic-set-by, pop-out, search (re-shot 2026-07-12 for the menu bar rework).
- [ ] **channel-panes.png**: Two to four channels tiled side by side in the chat area.
- [x] **nick-list.png**: The right-hand nick list showing op/voice prefixes (an `@op`, a `+voice`, some regulars).
- [x] **nick-context-menu.png**: Right-click menu open over a nick (Message, Whois, CTCP, DCC, Chan Ops…).
- [x] **emoji-picker.png**: The 😊 emoji picker popup open, showing the grid.
- [x] **link-preview.png**: A message with a URL and its preview card (title + thumbnail) below it.
- [x] **dcc-transfer.png**: A DCC transfer progress dialog mid-transfer.
- [x] **reactions.png**: A message with emoji reactions under it, e.g. `👍(2) ❤️(1)`.
- [x] **reply.png**: A reply in chat showing the `↩ nick` indicator pointing at the parent message.
- [x] **event-condensation.png**: A collapsed join/part/quit group shown as one compact line with the ▸ expander.
- [x] **quick-switcher.png**: The Ctrl+K quick-switcher popup open, with a filter typed and channels listed.
- [x] **theme-picker.png** _(vary theme)_: The Appearance page with the theme list expanded, a nice theme applied live.
- [x] **text-formatting.png**: Chat messages showing bold, italic, underline, strikethrough, and a colored line.
- [x] **mirc-color.png**: Chat messages sent with mIRC text/background colors.

---

## Tier 2: fill in later

- [ ] **first-launch.png**: The app right after first launch: connected to #uplinkirc, everything at defaults.
- [ ] **keychain-prompt.png**: Your OS keychain unlock/save prompt appearing when Uplink stores a password.
- [x] **event-condensation-expanded.png**: The same join/part group expanded to full lines (hostmasks + quit reasons).
- [ ] **unread-separator.png**: The `── N new messages ──` divider in the chat view.
- [x] **typing-indicator.png**: The `alice is typing…` line above the input bar.
- [ ] **nick-filter.png**: The nick list narrowed by the filter box (type a couple letters so it's clearly filtered).
- [ ] **account-badge.png**: Hover tooltip on a nick showing its NickServ account name.
- [x] **user-metadata.png**: Hover tooltip showing a display name + avatar thumbnail (needs Ergo/soju).
- [ ] **signal-bars.png**: Close-up of the signal bars in the sidebar header (ideally with the latency tooltip showing).
- [ ] **system-tray.png**: The tray icon with its right-click menu open (bonus: an unread dot on the icon).
- [ ] **notification.png**: A desktop notification toast for a mention/PM.
- [ ] **command-autocomplete.png**: The slash-command autocomplete dropdown after typing `/`.
- [ ] **whois.png**: A `/whois` response rendered inside a channel buffer.
- [ ] **channel-list.png**: The Channel Browser dialog (`/list`) with channels, user counts, and topics.
- [ ] **private-message.png**: A PM buffer open, with its entry in the sidebar.
- [ ] **emoji-autocomplete.png**: The inline `:shortcode:` autocomplete list in the input bar.
- [ ] **multiline-compose.png**: The input box expanded to several lines while composing (use Shift+Enter).
- [ ] **tab-completion.png**: Tab-completing a nick in the input (e.g. `alice:` at the start of a line).
- [ ] **message-search.png**: The Ctrl+F search bar open with matches highlighted in the chat.
- [ ] **history-search.png**: The Ctrl+Shift+F full-history search window listing matching log lines.
- [ ] **message-deletion.png**: A deleted message shown as the grey italic `[message deleted]` placeholder.
- [ ] **monitor.png**: A `Now online: nick` / `Now offline: nick` MONITOR notice in the server buffer.
- [ ] **app-icon-variants.png** _(vary theme)_: The app-icon variant picker grid in Appearance.
- [ ] **font-config.png**: The Font Config dialog with its per-zone size sliders.
