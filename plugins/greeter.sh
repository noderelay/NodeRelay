#!/usr/bin/env bash
# greeter.sh — welcomes people who join your channel.
#
# This one is bash, to show plugins can be any language that reads
# stdin and prints JSON. It needs jq for the JSON work:
#   Arch: sudo pacman -S jq    Debian: sudo apt install jq    FreeBSD: pkg install jq
#
# Change CHANNEL below to a channel you actually moderate before
# enabling this — greeting strangers in channels you don't run is a
# good way to get kicked.
#
# Try it without Uplink:
#   echo '{"event":"join","server":"s","buffer":"#mychannel","nick":"alice","self":false}' | ./greeter.sh

CHANNEL="#mychannel"

command -v jq >/dev/null || { echo "greeter.sh needs jq installed" >&2; exit 1; }

while IFS= read -r line; do
  event=$(printf '%s' "$line" | jq -r '.event // empty' 2>/dev/null)
  [ "$event" = "join" ] || continue

  buffer=$(printf '%s' "$line" | jq -r '.buffer')
  nick=$(printf '%s'   "$line" | jq -r '.nick')
  self=$(printf '%s'   "$line" | jq -r '.self')
  server=$(printf '%s' "$line" | jq -r '.server')

  [ "$buffer" = "$CHANNEL" ] || continue   # only our channel
  [ "$self" = "false" ]      || continue   # never greet yourself

  # Emit one action line. jq -c keeps it on a single line, which is
  # what Uplink expects.
  jq -cn --arg server "$server" --arg buffer "$buffer" --arg nick "$nick" \
    '{action:"say", server:$server, buffer:$buffer, text:("Welcome, " + $nick + "!")}'
done
