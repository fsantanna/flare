#!/bin/sh
# Runs the port on the scripted runs of tst/orig and compares its trace
# with the stored trace of the original, line by line.
#
#   tools/compare.sh [name ...]         (default: all runs of tst/orig)
#
# The kinds of lines that the port does not print yet are left out of
# both (SKIP, a regular expression on the kind). Prints, per run, "same"
# or the first lines that differ.

ROOT=$(cd "$(dirname "$0")/.." && pwd)
DIR="$ROOT/tst/orig"
SKIP=${SKIP:-'^$'}
NAMES="$*"
if [ -z "$NAMES" ]; then
	NAMES=$(cd "$DIR" && ls *.in | sed 's/\.in$//')
fi
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
FAIL=0
for n in $NAMES; do
	ENV=$(sed -n '1s/^# env: //p' "$DIR/$n.in")
	env SDL_VIDEODRIVER=dummy FLARE_MUTE=1 FLARE_FAST=1 FLARE_SEED=1 $ENV \
		FLARE_INPUT="$DIR/$n.in" FLARE_TRACE="$TMP/port" "$ROOT/run.sh" main.atm > "$TMP/out" 2>&1
	grep -Ev "^[0-9]+ ($SKIP) " "$DIR/$n.trace" > "$TMP/a"
	grep -Ev "^[0-9]+ ($SKIP) " "$TMP/port" > "$TMP/b" 2>/dev/null
	if cmp -s "$TMP/a" "$TMP/b"; then
		echo "same  $n ($(wc -l < "$TMP/a") lines)"
	else
		echo "DIFF  $n"
		diff "$TMP/a" "$TMP/b" | head -6
		FAIL=1
	fi
done
exit $FAIL
