#!/bin/sh
# Measures the patched original in a few scenarios, one after the other.
#
#   tools/bench/measure.sh [frames]      (default: 600, 10 s at 60 fps)
#
# For each scenario, two runs:
#   capped:   60 fps, as the game runs; CPU (user+sys) and peak memory
#   uncapped: FLARE_FAST, no frame cap; wall time for the same frames
# Times include the start and the load of the map.

HERE=$(cd "$(dirname "$0")" && pwd)
N=${1:-600}

run() { # name, then variables
	name=$1; shift
	cap=$(env "$@" FLARE_FAST= FLARE_FRAMES=$N /usr/bin/time -f "%U %S %M %e" \
		"$HERE/run.sh" 2>&1 >/dev/null | tail -1)
	unc=$(env "$@" FLARE_FAST=1 FLARE_FRAMES=$N /usr/bin/time -f "%e" \
		"$HERE/run.sh" 2>&1 >/dev/null | tail -1)
	echo "$cap $unc" | awk -v n="$name" \
		'{ printf "| %-22s | %6.2f | %7.1f | %6.2f | %6.2f |\n", n, $1+$2, $3/1024, $4, $5 }'
}

echo "| Scenario ($N frames)     | CPU s  | Peak MB | Wall s | Uncapped s |"
echo "|------------------------|--------|---------|--------|------------|"
run title FLARE_NEWGAME=
run frontier_outpost
run frontier_plains FLARE_MAP=maps/frontier_plains.txt
run averguard_complex FLARE_MAP=maps/averguard_complex.txt
run cave1 FLARE_MAP=maps/cave1.txt
