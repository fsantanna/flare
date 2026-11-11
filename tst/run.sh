#!/bin/sh
# Runs the tests of the port, with no screen.
#
#   tst/run.sh          runs and compares with the stored outputs (tst/golden)
#   tst/run.sh update   stores the new outputs as the expected ones
#   ASAN=1 tst/run.sh   the same, built with the address sanitizer
#
#   - ini: the INI reader on small files (tst/ini.atm);
#   - engine: images, a text, the generator, the trace, scripted input,
#     and a screenshot (tst/engine.atm), compared by its checksum;
#   - data: every text file of alpha_demo and fantasycore, and of
#     empyrean_campaign and fantasycore, read as the original reads them
#     (the dumps are compared by their size and checksum, which are those
#     of the original: see tools/compare-ini.sh).

ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
OUT=$ROOT/out
mkdir -p "$OUT" tst/golden
FAIL=0
export SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy
[ -n "$ASAN" ] && export ATMOS_ASAN=1 ATMOS_CFLAGS="-O1 -g"
DATA=${FLARE_DATA:-$ROOT/../flare-orig/data}

check () {     # name, file
	if [ "$UPDATE" = 1 ]; then
		cp "$2" "tst/golden/$1.out"
		echo "stored  $1"
	elif cmp -s "$2" "tst/golden/$1.out"; then
		echo "ok      $1"
	else
		echo "FAIL    $1"; FAIL=1
		diff "tst/golden/$1.out" "$2" | head -10
	fi
}

[ "$1" = "update" ] && UPDATE=1

# ini
./run.sh tst/ini.atm > "$OUT/ini.out" 2>&1
check ini "$OUT/ini.out"

# engine
FLARE_FRAMES=30 FLARE_FAST=1 FLARE_SEED=1 FLARE_TRACE=- FLARE_MUTE=1 \
FLARE_INPUT=tst/engine.in FLARE_SHOT=20:"$OUT/engine.bmp" \
	./run.sh tst/engine.atm > "$OUT/engine.out" 2>&1
echo "shot md5 $(md5sum < "$OUT/engine.bmp" | cut -c1-32)" >> "$OUT/engine.out"
check engine "$OUT/engine.out"

# data
for mods in fantasycore,alpha_demo fantasycore,empyrean_campaign; do
	for m in $(echo "$mods" | tr ',' ' '); do
		(cd "$DATA/mods/$m" && find . -name '*.txt' | sed 's|^\./||')
	done | LC_ALL=C sort -u > "$OUT/files.txt"
	./run.sh tools/ini-dump.atm "$DATA" "default,$mods" "$OUT/files.txt" > "$OUT/dump.txt" 2>&1
	echo "$(wc -l < "$OUT/files.txt") files, $(wc -l < "$OUT/dump.txt") lines, md5 $(md5sum < "$OUT/dump.txt" | cut -c1-32)"
done > "$OUT/data.out"
check data "$OUT/data.out"

exit $FAIL
