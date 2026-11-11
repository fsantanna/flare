#!/bin/sh
# Builds the original Flare, patched for the comparison with the port.
#
#   tools/bench/setup.sh [dir]
#
# dir: where the original goes, outside this repository
#      (default: $FLARE_ORIG, or ../flare-orig next to the repository).
# It clones flare-engine and flare-game at the target commits, applies
# original.patch, builds a release with the software renderer, and makes
# a data directory with the default mod of the engine and the mods of
# the game.

set -e

ENGINE=56c747a
GAME=af6eee6

HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)
ORIG=${1:-${FLARE_ORIG:-$ROOT/../flare-orig}}
mkdir -p "$ORIG"
ORIG=$(cd "$ORIG" && pwd)

cd "$ORIG"
[ -d flare-engine ] || git clone -q https://github.com/flareteam/flare-engine.git
[ -d flare-game ]   || git clone -q https://github.com/flareteam/flare-game.git

git -C flare-game checkout -q $GAME
git -C flare-engine checkout -q -f $ENGINE
git -C flare-engine clean -q -f src
git -C flare-engine apply "$HERE/original.patch"

mkdir -p flare-engine/build
cd flare-engine/build
cmake .. -DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_CXX_FLAGS="-include $ORIG/flare-engine/src/Bench.h" > /dev/null
make -j"$(nproc)" > /dev/null

cd "$ORIG"
mkdir -p data/mods
ln -sfn "$ORIG/flare-engine/mods/default" data/mods/default
for m in flare-game/mods/*/; do
	ln -sfn "$ORIG/$m" "data/mods/$(basename "$m")"
done

echo "$ORIG/flare-engine/build/flare"
