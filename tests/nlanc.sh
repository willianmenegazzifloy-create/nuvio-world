#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
CC=${CC:-cc}
TMP=$(mktemp "${TMPDIR:-/tmp}/nuvio-nlanc.XXXXXX")
trap 'rm -f "$TMP"' EXIT HUP INT TERM
"$CC" -std=c11 -Wall -Wextra -Werror -I"$ROOT/src" "$ROOT/tests/nlanc.c" -o "$TMP"
"$TMP"

# The preference was removed from the UI and is now a permanent product rule.
if grep -Fq 'ESC("Ocultar não lançados"' "$ROOT/src/ajustes.c"; then
  echo "Ocultar não lançados ainda aparece nos ajustes" >&2
  exit 1
fi
grep -Fq 'int ajustes_ocultar_nao_lancados(void) { return 1; }' "$ROOT/src/ajustes.c"
if sed -n '/static int descOcultaNaoLancado/,/^}/p' "$ROOT/src/descoberta.c" | grep -q 'ajustes_ocultar_nao_lancados'; then
  echo "Filtro de estreias ainda depende de preferência" >&2
  exit 1
fi
grep -Fq 'nlanc_futuro(it->dataEstreia, it->meta, hoje, tmv.tm_year + 1900)' "$ROOT/src/descoberta.c"
grep -Fq 'deMeta(p, f, tipo, &saida[n]) && !descOcultaNaoLancado(&saida[n])' "$ROOT/src/descoberta.c"
grep -Fq 'deMeta(p, f, tipo, &saida[n]) && !descOcultaNaoLancado(&saida[n])' "$ROOT/src/descoberta.c"
grep -Fq 'deMeta(p, f, a->tipo, it) && !descOcultaNaoLancado(it)' "$ROOT/src/descoberta.c"
echo "unreleased titles: hidden permanently"
