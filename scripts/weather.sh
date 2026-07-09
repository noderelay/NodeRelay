#!/usr/bin/env bash
# /weather [city] — current weather from wttr.in
#
# Dependencies: curl (should be installed on any system)
#
# Usage:
#   /weather            — weather for your IP-detected location
#   /weather Portland   — weather for Portland

location="${UPLINK_ARGS:-}"
result=$(curl -s "wttr.in/${location}?format=%l:+%C+%t+%h+humidity+%w+wind" 2>/dev/null)

if [ -z "$result" ] || echo "$result" | grep -qi "unknown"; then
    echo "Could not fetch weather${location:+ for $location}."
    exit 1
fi

echo "🌤 $result"
