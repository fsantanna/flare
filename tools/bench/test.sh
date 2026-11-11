#!/bin/sh
# Runs the patched original on each scripted run of tst/orig and compares
# its trace with the stored one. A run is <name>.in (the input) and
# <name>.trace (the trace); the first line of the .in may set variables:
#   # env: FLARE_FRAMES=240 FLARE_DUMP=1
#
#   tools/bench/test.sh [-u] [name...]    (-u: store the traces again)

HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)
DIR="$ROOT/tst/orig"

UPDATE=
if [ "$1" = "-u" ]; then UPDATE=1; shift; fi

NAMES="$*"
if [ -z "$NAMES" ]; then
	NAMES=$(cd "$DIR" && ls *.in | sed 's/\.in$//')
fi

FAIL=0
for n in $NAMES; do
	ENV=$(sed -n '1s/^# env: //p' "$DIR/$n.in")
	OUT=$(mktemp)
	env $ENV FLARE_INPUT="$DIR/$n.in" FLARE_TRACE=- "$HERE/run.sh" 2>/dev/null > "$OUT"
	if [ -n "$UPDATE" ]; then
		cp "$OUT" "$DIR/$n.trace"
		echo "stored $n"
	elif cmp -s "$OUT" "$DIR/$n.trace"; then
		echo "ok   $n"
	else
		echo "FAIL $n"
		diff "$DIR/$n.trace" "$OUT" | head -5
		FAIL=1
	fi
	rm -f "$OUT"
done
exit $FAIL
