#!/bin/sh
# Where the original draws random numbers: a build of the patched original
# that prints a backtrace at each call to rand(), and a run of it that
# counts the draws of the given frames by their three innermost callers.
#
#   tools/bench/draws.sh build            builds it in ../flare-orig/draws
#   [FLARE_*=...] tools/bench/draws.sh N  runs N logic frames (FLARE_MAP...)

HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)
ORIG=${FLARE_ORIG:-$ROOT/../flare-orig}
ORIG=$(cd "$ORIG" && pwd)
B="$ORIG/draws"

if [ "$1" = "build" ]; then
	mkdir -p "$B"
	sed 's|calls()++;|calls()++; if (getenv("FLARE_BT")) { void* b[8]; int n = backtrace(b, 8); backtrace_symbols_fd(b, n, 2); write(2, "--\\n", 3); }|; s|#include <vector>|#include <vector>\n#include <execinfo.h>\n#include <unistd.h>|' \
		"$ORIG/flare-engine/src/Bench.h" > "$B/Bench.h"
	cd "$B" && cmake "$ORIG/flare-engine" -DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_CXX_FLAGS="-include $B/Bench.h" -DCMAKE_EXE_LINKER_FLAGS="-rdynamic" > /dev/null \
		&& make -j"$(nproc)" > /dev/null && echo "$B/flare"
	exit
fi

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
mkdir -p "$TMP/c/flare" "$TMP/d"
printf 'fullscreen=0\nresolution_w=1280\nresolution_h=720\nrenderer=sdl\nmouse_move=0\nmouse_aim=1\nenable_joystick=0\nlanguage=en\ncombat_text=1\n' \
	> "$TMP/c/flare/settings.txt"
HOME="$TMP" XDG_CONFIG_HOME="$TMP/c" XDG_DATA_HOME="$TMP/d" SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
FLARE_SEED=${FLARE_SEED:-1} FLARE_FAST=1 FLARE_NEWGAME=${FLARE_NEWGAME-0} FLARE_FRAMES=${1:-1} FLARE_BT=1 \
	"$B/flare" --data-path="$ORIG/data" --mods=fantasycore,alpha_demo --no-audio --renderer=sdl 2>&1 >/dev/null \
	| grep -v 'INFO\|ERROR' | c++filt | sed 's/\[0x.*//; s/.*flare(//; s/+0x.*//; s/(.*//' \
	| awk 'BEGIN{RS="--\n"; FS="\n"} {print $1" < "$2" < "$3}' | sort | uniq -c | sort -rn
