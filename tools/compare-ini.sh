#!/bin/sh
# Compares the INI reader of the port (src/ini.atm) with FileParser of the
# patched original, on every text file of the given mods.
#
#   tools/compare-ini.sh [mod,mod,...]     (default: fantasycore,alpha_demo)
#
# The mod `default` of the engine always comes first, as in the original.

ROOT=$(cd "$(dirname "$0")/.." && pwd)
MODS=${1:-fantasycore,alpha_demo}
DATA=${FLARE_DATA:-$ROOT/../flare-orig/data}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

for m in $(echo "$MODS" | tr ',' ' '); do
	(cd "$DATA/mods/$m" && find . -name '*.txt' | sed 's|^\./||')
done | LC_ALL=C sort -u > "$TMP/files"

FLARE_INI="$TMP/files" FLARE_TRACE=- "$ROOT/tools/bench/run.sh" --mods="$MODS" \
	2>/dev/null > "$TMP/orig"
"$ROOT/run.sh" tools/ini-dump.atm "$DATA" "default,$MODS" "$TMP/files" > "$TMP/port"

echo "$(wc -l < "$TMP/files") files, $(wc -l < "$TMP/orig") lines"
if cmp -s "$TMP/orig" "$TMP/port"; then
	echo "same"
else
	diff "$TMP/orig" "$TMP/port" | head -20
	exit 1
fi
