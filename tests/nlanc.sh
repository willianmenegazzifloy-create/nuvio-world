#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
CC=${CC:-cc}
TMP=$(mktemp "${TMPDIR:-/tmp}/nuvio-nlanc.XXXXXX")
trap 'rm -f "$TMP"' EXIT HUP INT TERM
"$CC" -std=c11 -Wall -Wextra -Werror -I"$ROOT/src" "$ROOT/tests/nlanc.c" -o "$TMP"
"$TMP"
