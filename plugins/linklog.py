#!/usr/bin/env python3
# linklog.py — saves every link you see on IRC to a text file.
#
# The safest possible first plugin: it only watches. It never sends
# anything to any channel, so nothing can go wrong in public.
#
# Links are appended to ~/uplink-links.txt, one per line:
#   2026-07-16 19:12  #uplinkirc  alice  https://example.com/cool-thing
#
# Try it without Uplink:
#   echo '{"event":"message","server":"s","buffer":"#c","nick":"alice","text":"see https://example.com","kind":"message","self":false}' | ./linklog.py
#   cat ~/uplink-links.txt
#
# Full protocol reference: https://uplinkirc.chat → Docs → Plugins

import json
import re
import sys
import time
from pathlib import Path

LOG_FILE = Path.home() / "uplink-links.txt"
URL_RE = re.compile(r"https?://\S+")

for line in sys.stdin:
    try:
        event = json.loads(line)
    except json.JSONDecodeError:
        continue                      # never crash on unexpected input

    if event.get("event") != "message":
        continue

    for url in URL_RE.findall(event.get("text", "")):
        stamp = time.strftime("%Y-%m-%d %H:%M")
        with LOG_FILE.open("a", encoding="utf-8") as f:
            f.write(f"{stamp}  {event['buffer']}  {event['nick']}  {url}\n")
