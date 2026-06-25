#!/bin/bash
title=$(nowplaying-cli get title 2>/dev/null)
artist=$(nowplaying-cli get artist 2>/dev/null)
album=$(nowplaying-cli get album 2>/dev/null)

if [ -z "$title" ] || [ "$title" = "null" ]; then
    echo "Nothing playing."
    exit 0
fi

out="♫ Now playing: $title"
[ -n "$artist" ] && [ "$artist" != "null" ] && out="$out — $artist"
[ -n "$album" ] && [ "$album" != "null" ] && out="$out ($album)"
echo "$out"
