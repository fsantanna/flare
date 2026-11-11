#!/bin/sh
# Builds and runs a program of the port with the Atmos compiler.
#
#   ./run.sh [file.atm] [args]      (default: main.atm)
#
# Environment:
#   ATMOS        the compiler (default: ../ceu-atmos/atmos, or `atmos`)
#   ATMOS_ASAN=1 builds with the address sanitizer
#   FLARE_DATA   the directory with the folder `mods` of the original
#                (default: ../flare-orig/data, made by tools/bench/setup.sh)

ROOT=$(cd "$(dirname "$0")" && pwd)

if [ -z "$ATMOS" ]; then
	if [ -x "$ROOT/../ceu-atmos/atmos" ]; then
		ATMOS="$ROOT/../ceu-atmos/atmos"
	else
		ATMOS=atmos
	fi
fi

export FLARE_DATA="${FLARE_DATA:-$ROOT/../flare-orig/data}"

FILE=${1:-main.atm}
[ $# -gt 0 ] && shift

export ATMOS_CFLAGS="${ATMOS_CFLAGS:--O2} -I$ROOT"
export ATMOS_LDFLAGS="-lSDL2 -lSDL2_image -lSDL2_mixer -lSDL2_ttf -lm"

cd "$ROOT" && exec "$ATMOS" "$FILE" "$@"
