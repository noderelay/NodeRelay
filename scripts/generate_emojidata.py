#!/usr/bin/env python3
"""
Generate src/ui/emojidata.h from Unicode emoji-test.txt and GitHub gemoji.

Sources:
  - emoji-test.txt: https://unicode.org/Public/emoji/16.0/emoji-test.txt
  - gemoji:         https://raw.githubusercontent.com/github/gemoji/master/db/emoji.json

Usage:
  python3 scripts/generate_emojidata.py

Outputs src/ui/emojidata.h with all fully-qualified emojis (no skin tone
variants), grouped by category, using gemoji shortcodes where available
and Unicode-name-derived shortcodes for the rest.
"""

import json
import re
import sys
import urllib.request
from pathlib import Path

EMOJI_TEST_URL = "https://unicode.org/Public/emoji/16.0/emoji-test.txt"
GEMOJI_URL = "https://raw.githubusercontent.com/github/gemoji/master/db/emoji.json"
OUTPUT = Path(__file__).resolve().parent.parent / "src" / "ui" / "emojidata.h"

SKIN_TONES = {0x1F3FB, 0x1F3FC, 0x1F3FD, 0x1F3FE, 0x1F3FF}


def fetch(url):
    with urllib.request.urlopen(url) as r:
        return r.read().decode("utf-8")


def has_skin_tone(codepoints):
    return bool(SKIN_TONES & set(codepoints))


def name_to_shortcode(name):
    s = name.lower()
    s = s.replace("'", "").replace(",", "").replace(".", "")
    s = s.replace(":", "").replace("!", "").replace("&", "and")
    s = s.replace(" ", "_").replace("-", "_").replace("__", "_")
    s = re.sub(r"[^a-z0-9_]", "", s)
    return s.strip("_")


def parse_emoji_test(text):
    """Return list of (emoji_char, group, subgroup, name) for fully-qualified, non-skin-tone entries."""
    entries = []
    group = ""
    subgroup = ""
    for line in text.splitlines():
        line = line.strip()
        if line.startswith("# group:"):
            group = line.split(":", 1)[1].strip()
            continue
        if line.startswith("# subgroup:"):
            subgroup = line.split(":", 1)[1].strip()
            continue
        if not line or line.startswith("#"):
            continue
        if "; fully-qualified" not in line:
            continue
        parts = line.split(";")
        cp_str = parts[0].strip()
        codepoints = [int(x, 16) for x in cp_str.split()]
        if has_skin_tone(codepoints):
            continue
        after_hash = line.split("#", 1)[1].strip()
        # Format: emoji Eversion name
        m = re.match(r"(\S+)\s+E\d+\.\d+\s+(.*)", after_hash)
        if not m:
            continue
        emoji_ch = m.group(1)
        name = m.group(2).strip()
        entries.append((emoji_ch, group, subgroup, name))
    return entries


def load_gemoji(text):
    """Return dict: emoji_char -> (shortcode, category)."""
    data = json.loads(text)
    result = {}
    for entry in data:
        emoji = entry.get("emoji", "")
        aliases = entry.get("aliases", [])
        category = entry.get("category", "")
        if emoji and aliases:
            result[emoji] = (aliases[0], category)
    return result


def generate_header(entries, gemoji_map):
    lines = []
    lines.append("#pragma once")
    lines.append("#include <QString>")
    lines.append("#include <QStringView>")
    lines.append("#include <QVector>")
    lines.append("")
    lines.append("struct EmojiEntry { QStringView shortcode; QStringView ch; };")
    lines.append("")
    lines.append("// Emoji data lives in .rodata as string literals — no heap allocation and no")
    lines.append("// first-use construction cost.")
    lines.append("inline constexpr EmojiEntry kEmojiTable[] = {")

    current_group = ""
    seen_shortcodes = set()
    count = 0

    for emoji_ch, group, subgroup, name in entries:
        if group != current_group:
            if current_group:
                lines.append("")
            lines.append(f"    // {group}")
            current_group = group

        if emoji_ch in gemoji_map:
            shortcode = gemoji_map[emoji_ch][0]
        else:
            shortcode = name_to_shortcode(name)

        if shortcode in seen_shortcodes:
            shortcode = name_to_shortcode(name)
        if shortcode in seen_shortcodes:
            shortcode = shortcode + "_" + format(ord(emoji_ch[0]), "x")
        if not shortcode:
            continue

        seen_shortcodes.add(shortcode)
        pad = max(1, 40 - len(shortcode))
        lines.append(f'    {{u"{shortcode}",{" " * pad}u"{emoji_ch}"}},')
        count += 1

    lines.append("};")
    lines.append("")
    lines.append("// Exact shortcode → emoji; empty if unknown.")
    lines.append("inline QString emojiForCode(QStringView code)")
    lines.append("{")
    lines.append("    for (const auto &e : kEmojiTable)")
    lines.append("        if (e.shortcode == code)")
    lines.append("            return e.ch.toString();")
    lines.append("    return {};")
    lines.append("}")
    lines.append("")
    lines.append("inline QVector<EmojiEntry> emojiMatching(QStringView prefix)")
    lines.append("{")
    lines.append("    QVector<EmojiEntry> result;")
    lines.append("    for (const auto &e : kEmojiTable)")
    lines.append("        if (e.shortcode.startsWith(prefix, Qt::CaseInsensitive))")
    lines.append("            result.append(e);")
    lines.append("    return result;")
    lines.append("}")
    lines.append("")

    return "\n".join(lines), count


def main():
    print("Fetching emoji-test.txt ...")
    emoji_test = fetch(EMOJI_TEST_URL)

    print("Fetching gemoji ...")
    gemoji_text = fetch(GEMOJI_URL)

    entries = parse_emoji_test(emoji_test)
    gemoji_map = load_gemoji(gemoji_text)

    print(f"Parsed {len(entries)} emojis from emoji-test.txt (no skin tones)")
    print(f"Loaded {len(gemoji_map)} gemoji shortcodes")

    header, count = generate_header(entries, gemoji_map)

    OUTPUT.write_text(header, encoding="utf-8")
    print(f"Wrote {count} entries to {OUTPUT}")


if __name__ == "__main__":
    main()
