#!/bin/sh
# Compares the animation loader of the port (src/anim.atm) with
# AnimationSet of the patched original, on every file of `animations/` of
# the given mods.
#
#   tools/compare-anim.sh [mod,mod,...]    (default: fantasycore,alpha_demo)

ROOT=$(cd "$(dirname "$0")/.." && pwd)
MODS=${1:-fantasycore,alpha_demo}
DATA=${FLARE_DATA:-$ROOT/../flare-orig/data}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

for m in $(echo "$MODS" | tr ',' ' '); do
	(cd "$DATA/mods/$m" && find . -path './animations/*' -name '*.txt' | sed 's|^\./||')
done | LC_ALL=C sort -u > "$TMP/files"

FLARE_ANIM="$TMP/files" FLARE_TRACE=- "$ROOT/tools/bench/run.sh" --mods="$MODS" \
	2>/dev/null > "$TMP/orig"
SDL_VIDEODRIVER=dummy FLARE_MUTE=1 FLARE_FRAMES=1 \
	"$ROOT/run.sh" tools/anim-dump.atm "$DATA" "default,$MODS" "$TMP/files" 2>/dev/null > "$TMP/port"

echo "$(wc -l < "$TMP/files") files, $(wc -l < "$TMP/orig") lines"
if cmp -s "$TMP/orig" "$TMP/port"; then
	echo "same"
else
	diff "$TMP/orig" "$TMP/port" | head -20
	exit 1
fi
