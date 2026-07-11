# Keyboard Shortcuts

All shortcuts use **Ctrl** on Linux and Windows. On macOS, **Ctrl** maps to **Cmd** and **Alt** maps to **Option** automatically.

---

## Quick reference

| Shortcut | Action |
|---|---|
| **Ctrl+K** | Quick channel switcher |
| **Alt+Up / Down** | Previous / next channel |
| **Alt+Left / Right** | Move focus to the previous / next pane |
| **Ctrl+F** | Search current buffer |
| **Ctrl+Shift+F** | Search full logged history |
| **Ctrl+W** | Minimize to tray |
| **Ctrl+B / I / U / S** | Bold / italic / underline / strikethrough |
| **Ctrl+Shift+K** | Pick text / background color |
| **Ctrl+O** | Clear all formatting |
| **Ctrl+Plus / Minus** | Zoom font in / out |
| **Tab** | Complete nick or command |
| **Enter** | Send message |
| **Shift+Enter** | Insert newline |
| **Escape** | Cancel reply, close search, or dismiss emoji list |

---

## Navigation

Navigate between channels and panes without touching the mouse. These shortcuts work from any focused widget — the input box, nick list, or anywhere else.

| Shortcut | Action |
|---|---|
| **Ctrl+K** | Quick channel switcher — type to filter, Enter to jump |
| **Alt+Up** | Switch to the previous channel in the sidebar |
| **Alt+Down** | Switch to the next channel in the sidebar |
| **Alt+Left** | Move keyboard focus to the previous open pane |
| **Alt+Right** | Move keyboard focus to the next open pane |

**Ctrl+K** opens a floating search popup listing all joined channels. Start typing to filter by channel name or server — the list narrows as you type. Use Up/Down to select and Enter to switch. Press Escape to dismiss.

**Alt+Left / Alt+Right** cycles keyboard focus through the open panes' input bars — the primary view's input included — in layout order. The panes stay exactly where they are; only the focus (and therefore where your typing goes) moves.

Alt+arrow shortcuts wrap around — pressing Alt+Down on the last channel jumps back to the first, and Alt+Right on the last pane wraps to the first.

---

## Message input

| Shortcut | Action |
|---|---|
| **Enter** | Send the message |
| **Shift+Enter** | Insert a newline (multi-line compose) |
| **Up Arrow** | Previous message in send history (from the first line only) |
| **Down Arrow** | Next message in send history (from the last line only) |
| **Tab** | Complete the nick or command at the cursor — press again to cycle |
| **Shift+Tab** | Cycle backwards through completions |
| **Escape** | Cancel a pending reply |

### Tab completion

Tab completion works anywhere in the line. Type a few letters and press Tab to complete to the first matching nick in the channel. Press Tab again to cycle alphabetically through all matches.

At the start of a message, a colon is appended automatically (`alice:`) following IRC convention. Mid-sentence, no colon is added.

Slash commands complete the same way:

```
/pi     → Tab →  /ping
/mo     → Tab →  /mode  → Tab →  /monitor  → Tab →  /motd
```

### Text formatting

Apply mIRC formatting as you type. The input box shows formatted text live — bold looks bold, italic looks italic. IRC control codes are generated when you send. A small `B I U S` indicator at the bottom-left of the input shows which formats are active.

| Shortcut | Format |
|---|---|
| **Ctrl+B** | Toggle bold |
| **Ctrl+I** | Toggle italic |
| **Ctrl+U** | Toggle underline |
| **Ctrl+S** | Toggle strikethrough |
| **Ctrl+Shift+K** | Pick text / background color |
| **Ctrl+O** | Reset all formatting |

Formats stack — press Ctrl+B then Ctrl+U to type bold underlined text. Press the same shortcut again to turn off just that format. Use Ctrl+O to clear everything at once.

**Ctrl+Shift+K** — or **right-clicking the input box** and opening the **Color** submenu — shows the 16 mIRC colors, where you pick a **Text color** and, optionally, a **Background**. The chosen colors apply to what you type next (or to a selection), just like bold, and appear as a colored `A` in the format indicator. Choose **Reset color** to go back to the default.

```
This is [Ctrl+B]important[Ctrl+B] now
[Ctrl+B][Ctrl+U]really important[Ctrl+O]
```

### Emoji shortcodes

Type a colon to start an emoji shortcode:

| Input | Result |
|---|---|
| `:fire` | Completion list appears — navigate with Up/Down |
| **Enter** or **Tab** | Insert the selected emoji |
| **Escape** | Dismiss the list |
| `:fire:` | Substituted to 🔥 immediately, no list needed |

Any `:shortcode:` patterns still in the text when you send are substituted automatically.

---

## Search (Ctrl+F)

| Shortcut | Action |
|---|---|
| **Ctrl+F** | Open the search bar |
| **Type anything** | Jump to the first match as you type |
| **Enter** | Next match (wraps around) |
| **Shift+Enter** | Previous match |
| **Escape** | Close search and clear highlights |

Search is case-insensitive and covers the full buffer for the current channel. It does not search across channels.

Panes have their own search: with focus inside a docked pane, **Ctrl+F** opens that pane's search bar instead of the primary one. Popped-out channel windows respond to **Ctrl+F** too. The magnifier button in each pane or window header does the same thing.

### Full history (Ctrl+Shift+F)

**Ctrl+Shift+F** opens a separate window that searches the current channel's entire **on-disk log**, not just the messages currently loaded in the buffer. Type to search (case-insensitive substring, or tick **Regex** for a regular expression); matches are listed newest-first. This requires message logging to be enabled in Preferences — without it, only the visible buffer exists to search.

For example, with **Regex** enabled, `error|timeout` finds lines mentioning either word, `<alice>` finds messages from *alice*, and `^\[2026-07-04` matches everything logged on that date.

---

## Window and display

| Shortcut | Action |
|---|---|
| **Ctrl+W** | Minimize to system tray |
| **Close button (×)** | Same as Ctrl+W — minimizes to tray, does not quit |
| **Ctrl+Plus** / **Ctrl+=** | Increase font size (0.5 pt) for the focused UI region |
| **Ctrl+Minus** | Decrease font size (0.5 pt) for the focused UI region |
| **Ctrl+Scroll wheel** | Zoom font size for the region under the cursor |

Font zoom is per-region — you can set different sizes for the chat view, nick list, sidebar, and input box independently.

---

## macOS notes

Qt automatically translates modifier keys on macOS:

| This doc says | macOS physical key |
|---|---|
| **Ctrl** | **Cmd** (⌘) |
| **Alt** | **Option** (⌥) |
| **Shift** | **Shift** (⇧) |

So **Alt+Up** (navigate channels) is **Option+Up** on a Mac, and **Ctrl+F** (search) is **Cmd+F**.
