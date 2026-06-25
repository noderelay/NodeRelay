#!/bin/bash
# /music — show the currently playing track
#
# Dependencies:
#   macOS:  brew install nowplaying-cli
#   Linux:  pacman -S playerctl  (or apt install playerctl)
#
# The script auto-detects the platform and uses the appropriate tool.
# If nowplaying-cli is not in PATH on macOS, it falls back to
# /opt/homebrew/bin/nowplaying-cli (default Homebrew location).

case "$(uname)" in
    Darwin)
        NPC=$(command -v nowplaying-cli || echo /opt/homebrew/bin/nowplaying-cli)
        if [ ! -x "$NPC" ]; then
            echo "nowplaying-cli not found — install with: brew install nowplaying-cli"
            exit 1
        fi
        title=$($NPC get title 2>/dev/null)
        artist=$($NPC get artist 2>/dev/null)
        album=$($NPC get album 2>/dev/null)
        ;;
    Linux)
        if ! command -v playerctl &>/dev/null; then
            echo "playerctl not found — install with your package manager"
            exit 1
        fi
        title=$(playerctl metadata title 2>/dev/null)
        artist=$(playerctl metadata artist 2>/dev/null)
        album=$(playerctl metadata album 2>/dev/null)
        ;;
    *)
        echo "Unsupported platform: $(uname)"
        exit 1
        ;;
esac

if [ -z "$title" ]; then
    echo "Nothing playing."
    exit 0
fi

out="♫ Now playing: $title"
[ -n "$artist" ] && out="$out — $artist"
[ -n "$album" ] && out="$out ($album)"
echo "$out"
