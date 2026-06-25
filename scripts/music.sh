#!/bin/bash
NPC=$(command -v nowplaying-cli || echo /opt/homebrew/bin/nowplaying-cli)
title=$($NPC get title 2>/dev/null)
artist=$($NPC get artist 2>/dev/null)
album=$($NPC get album 2>/dev/null)

if [ -z "$title" ] || [ "$title" = "null" ]; then
    echo "Nothing playing."
    exit 0
fi

out="♫ Now playing: $title"
[ -n "$artist" ] && [ "$artist" != "null" ] && out="$out — $artist"
[ -n "$album" ] && [ "$album" != "null" ] && out="$out ($album)"
echo "$out"
