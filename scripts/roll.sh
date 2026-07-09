#!/usr/bin/env bash
# /roll [NdS] — roll N dice with S sides (default: 1d6)
#
# No dependencies — pure bash.
#
# Usage:
#   /roll         — roll 1d6
#   /roll 2d6     — roll 2 six-sided dice
#   /roll 1d20    — roll 1 twenty-sided die
#   /roll 4d8     — roll 4 eight-sided dice

input="${UPLINK_ARGS:-1d6}"
input=$(echo "$input" | tr 'D' 'd')

if ! echo "$input" | grep -qE '^[0-9]+d[0-9]+$'; then
    echo "Usage: /roll NdS (e.g. 2d6, 1d20, 4d8)"
    exit 1
fi

num=${input%d*}
sides=${input#*d}

if [ "$num" -lt 1 ] || [ "$num" -gt 20 ]; then
    echo "Keep it between 1 and 20 dice."
    exit 1
fi
if [ "$sides" -lt 2 ] || [ "$sides" -gt 100 ]; then
    echo "Sides must be between 2 and 100."
    exit 1
fi

total=0
rolls=""
for ((i = 0; i < num; i++)); do
    val=$(( (RANDOM % sides) + 1 ))
    total=$((total + val))
    if [ -n "$rolls" ]; then
        rolls="$rolls + $val"
    else
        rolls="$val"
    fi
done

if [ "$num" -eq 1 ]; then
    echo "🎲 ${input}: $total"
else
    echo "🎲 ${input}: $rolls = $total"
fi
