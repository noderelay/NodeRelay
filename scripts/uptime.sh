#!/bin/bash
# /uptime — show system uptime
#
# No dependencies — uses built-in commands on macOS and Linux.

case "$(uname)" in
    Darwin)
        boot=$(sysctl -n kern.boottime | awk -F'[ ,]' '{print $4}')
        now=$(date +%s)
        secs=$((now - boot))
        days=$((secs / 86400))
        hours=$(( (secs % 86400) / 3600 ))
        mins=$(( (secs % 3600) / 60 ))
        echo "⏱ Uptime: ${days}d ${hours}h ${mins}m"
        ;;
    Linux)
        up=$(uptime -p 2>/dev/null || uptime)
        echo "⏱ ${up}"
        ;;
    MINGW*|MSYS*|CYGWIN*)
        info=$(powershell.exe -NoProfile -NonInteractive -Command '
$up = (Get-Date) - (Get-CimInstance Win32_OperatingSystem).LastBootUpTime
$parts = @()
if ($up.Days) { $parts += "$($up.Days) days" }
if ($up.Hours) { $parts += "$($up.Hours) hours" }
$parts += "$($up.Minutes) minutes"
"up " + ($parts -join ", ")
' 2>/dev/null | tr -d '\r')
        if [ -z "$info" ]; then
            echo "Could not determine uptime."
            exit 1
        fi
        echo "⏱ $info"
        ;;
    *)
        echo "Unsupported platform: $(uname)"
        exit 1
        ;;
esac
