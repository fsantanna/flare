#!/bin/sh
# Runs the patched original, with no screen and no audio, in a fresh home.
#
#   [FLARE_*=...] tools/bench/run.sh [options of flare]
#
# The FLARE_* variables are described in Bench.h (in original.patch).
# Defaults: FLARE_SEED=1, FLARE_FAST=1, FLARE_NEWGAME=0, and the mods
# fantasycore,alpha_demo. The log of flare goes to stderr, and a trace
# to "-" goes to stdout.

set -e

HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)
ORIG=${FLARE_ORIG:-$ROOT/../flare-orig}
ORIG=$(cd "$ORIG" && pwd)

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
mkdir -p "$TMP/c/flare" "$TMP/d"
cat > "$TMP/c/flare/settings.txt" <<SETTINGS
fullscreen=0
resolution_w=1280
resolution_h=720
renderer=sdl
hwsurface=0
vsync=0
max_fps=60
combat_text=1
mouse_move=0
mouse_aim=1
enable_joystick=0
language=en
show_fps=0
SETTINGS

export HOME="$TMP" XDG_CONFIG_HOME="$TMP/c" XDG_DATA_HOME="$TMP/d"
export SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy
export FLARE_SEED=${FLARE_SEED:-1}
export FLARE_FAST=${FLARE_FAST-1}
export FLARE_NEWGAME=${FLARE_NEWGAME-0}

MODS=--mods=fantasycore,alpha_demo
for a in "$@"; do
	case "$a" in --mods=*) MODS= ;; esac
done

# a trace to stdout goes through a file, since flare logs to stdout too
OUT=
if [ "$FLARE_TRACE" = "-" ]; then
	OUT="$TMP/trace"
	export FLARE_TRACE="$OUT"
fi

RC=0
"$ORIG/flare-engine/build/flare" --data-path="$ORIG/data" \
	--no-audio --renderer=sdl $MODS "$@" 1>&2 || RC=$?

if [ -n "$OUT" ] && [ -f "$OUT" ]; then
	cat "$OUT"
fi
exit $RC
