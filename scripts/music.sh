#!/usr/bin/env bash
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
    Linux|*BSD|DragonFly)
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
            title=$(echo "$META" | grep -A1 '"xesam:title"' | grep 'string' | tail -1 | sed 's/.*string "\(.*\)"/\1/')
            artist=$(echo "$META" | grep -A2 '"xesam:artist"' | grep 'string' | tail -1 | sed 's/.*string "\(.*\)"/\1/')
            album=$(echo "$META" | grep -A1 '"xesam:album"' | grep 'string' | tail -1 | sed 's/.*string "\(.*\)"/\1/')
        fi
        ;;
    MINGW*|MSYS*|CYGWIN*)
        if ! command -v powershell.exe &>/dev/null; then
            echo "PowerShell not found — required for /music on Windows."
            exit 1
        fi
        info=$(powershell.exe -NoProfile -NonInteractive -Command '
try {
    Add-Type -AssemblyName System.Runtime.WindowsRuntime
    $at = ([System.WindowsRuntimeSystemExtensions].GetMethods() | Where-Object {
        $_.Name -eq "AsTask" -and $_.GetParameters().Count -eq 1 -and
        $_.GetParameters()[0].ParameterType.Name -eq "IAsyncOperation``1" })[0]
    function WrtAwait($op, $t) {
        $task = $at.MakeGenericMethod($t).Invoke($null, @($op))
        $task.Wait(-1) | Out-Null
        return $task.Result }
    [void][Windows.Media.Control.GlobalSystemMediaTransportControlsSessionManager,Windows.Media.Control,ContentType=WindowsRuntime]
    $mgr = WrtAwait ([Windows.Media.Control.GlobalSystemMediaTransportControlsSessionManager]::RequestAsync()) ([Windows.Media.Control.GlobalSystemMediaTransportControlsSessionManager])
    $s = $mgr.GetCurrentSession()
    if ($s) {
        [void][Windows.Media.Control.GlobalSystemMediaTransportControlsSessionMediaProperties,Windows.Media.Control,ContentType=WindowsRuntime]
        $p = WrtAwait ($s.TryGetMediaPropertiesAsync()) ([Windows.Media.Control.GlobalSystemMediaTransportControlsSessionMediaProperties])
        if ($p.Title) { "$($p.Title)|$($p.Artist)|$($p.AlbumTitle)" } }
} catch {}
' 2>/dev/null | tr -d '\r')
        if [ -n "$info" ]; then
            IFS='|' read -r title artist album <<< "$info"
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
