#!/usr/bin/env python3
# pingpong.py — the "hello world" of Uplink plugins.
#
# When anyone says !ping in a channel, this plugin answers "pong".
#
# How Uplink talks to this script:
#   - Uplink writes one JSON object per line to our stdin (events).
#   - We write one JSON object per line to stdout (actions).
#   That's the whole API. Any language that can read stdin and print
#   JSON can be an Uplink plugin.
#
# Try it without Uplink (paste this in a terminal):
#   echo '{"event":"message","server":"s","buffer":"#c","nick":"alice","text":"!ping","kind":"message","self":false}' | ./pingpong.py
# You should see: {"action": "say", "server": "s", "buffer": "#c", "text": "pong"}
#
# Full protocol reference: https://uplinkirc.chat → Docs → Plugins

import json
import sys
import time

COOLDOWN_SECONDS = 5   # never answer more than once per 5s per channel
last_reply = {}        # channel -> when we last replied

def send(action):
    # One JSON line, then flush. Flushing matters: without it Python
    # buffers output and Uplink sees your reply late or never.
    print(json.dumps(action), flush=True)

for line in sys.stdin:
    try:
        event = json.loads(line)
    except json.JSONDecodeError:
        continue                      # never crash on unexpected input

    if event.get("event") != "message":
        continue                      # only react to chat messages
    if event.get("self"):
        continue                      # never react to our own lines —
                                      # without this a plugin can end up
                                      # talking to itself in a loop
    if event.get("text", "").strip() != "!ping":
        continue

    channel = event["buffer"]
    now = time.time()
    if now - last_reply.get(channel, 0) < COOLDOWN_SECONDS:
        continue                      # too soon, stay quiet
    last_reply[channel] = now

    send({
        "action": "say",
        "server": event["server"],
        "buffer": channel,
        "text": "pong",
    })
