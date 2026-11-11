#!/bin/sh
# Compares the drawing of maps by the port (tst/map.atm) with the patched
# original (FLARE_ONLYMAP): only the layers, at the hero of each map, at a
# logic frame. Prints, per map, "same" or the number of differing pixels.
#
#   tools/compare-map.sh [frame] [map ...]    (default: 30, all of alpha_demo)

ROOT=$(cd "$(dirname "$0")/.." && pwd)
DATA=${FLARE_DATA:-$ROOT/../flare-orig/data}
N=${1:-30}
[ $# -gt 0 ] && shift
MAPS="$*"
if [ -z "$MAPS" ]; then
	MAPS=$(cd "$DATA/mods/alpha_demo/maps" && ls *.txt | sed 's/\.txt$//' | grep -v '^spawn$')
fi
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
FAIL=0
for m in $MAPS; do
	FLARE_ONLYMAP=1 FLARE_MAP=maps/$m.txt FLARE_FRAMES=$((N+1)) FLARE_SHOT=$N:"$TMP/orig.bmp" \
		"$ROOT/tools/bench/run.sh" 2>/dev/null
	SDL_VIDEODRIVER=dummy FLARE_MUTE=1 FLARE_FAST=1 FLARE_MAP=maps/$m.txt FLARE_FRAMES=$((N+1)) \
		FLARE_SHOT=$N:"$TMP/port.bmp" "$ROOT/run.sh" tst/map.atm > /dev/null 2>&1
	if cmp -s "$TMP/orig.bmp" "$TMP/port.bmp"; then
		echo "same  $m"
	else
		echo "DIFF  $m $(python3 -c "
from PIL import Image, ImageChops
import numpy as np
a = Image.open('$TMP/orig.bmp').convert('RGB'); b = Image.open('$TMP/port.bmp').convert('RGB')
x = np.asarray(ImageChops.difference(a, b)).sum(axis=2)
print((x > 0).sum(), 'pixels')" 2>&1)"
		FAIL=1
	fi
done
exit $FAIL
