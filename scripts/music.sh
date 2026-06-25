#!/bin/bash
# /music — show the currently playing track
#
# Dependencies:
#   macOS:  brew install nowplaying-cli
#   Linux:  Uses MPRIS2 via D-Bus (works with any MPRIS-compatible player:
#           Spotify, VLC, mpd, Firefox, Chrome/YouTube Music, etc.)
#           Optional: pacman -S playerctl (or apt install playerctl) for
#           a cleaner interface — falls back to dbus-send if not installed.
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
        if command -v playerctl &>/dev/null; then
            title=$(playerctl metadata title 2>/dev/null)
            artist=$(playerctl metadata artist 2>/dev/null)
            album=$(playerctl metadata album 2>/dev/null)
        else
            # Fallback: query MPRIS2 directly via D-Bus
            MPRIS_DEST=$(dbus-send --session --dest=org.freedesktop.DBus \
                --type=method_call --print-reply /org/freedesktop/DBus \
                org.freedesktop.DBus.ListNames 2>/dev/null \
                | grep -oP '"org\.mpris\.MediaPlayer2\.\K[^"]+' | head -1)
            if [ -z "$MPRIS_DEST" ]; then
                echo "No MPRIS player found. Install playerctl or start a media player."
                exit 1
            fi
            META=$(dbus-send --session --print-reply \
                --dest="org.mpris.MediaPlayer2.$MPRIS_DEST" \
                /org/mpris/MediaPlayer2 \
                org.freedesktop.DBus.Properties.Get \
                string:'org.mpris.MediaPlayer2.Player' \
                string:'Metadata' 2>/dev/null)
            title=$(echo "$META" | grep -A1 '"xesam:title"' | grep 'string' | head -1 | sed 's/.*string "\(.*\)"/\1/')
            artist=$(echo "$META" | grep -A2 '"xesam:artist"' | grep 'string' | head -1 | sed 's/.*string "\(.*\)"/\1/')
            album=$(echo "$META" | grep -A1 '"xesam:album"' | grep 'string' | head -1 | sed 's/.*string "\(.*\)"/\1/')
        fi
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
