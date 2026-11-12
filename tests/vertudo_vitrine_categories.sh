#!/bin/bash
# Regressao do modelo Follow Layout do Nuvio 1.1.0: uma linha estavel por fonte,
# inclusive em pastas de fonte unica, com categoria e identidade preservadas.
set -eu
cd "$(dirname "$0")/.."
grep -q '#define VIT_LIN_MAX COL_SOURCE_MAX' src/vertudo.c
grep -q 'vitMulti = 1;' src/vertudo.c
grep -q 'vitN = collection->nSources < VIT_LIN_MAX ? collection->nSources : VIT_LIN_MAX;' src/vertudo.c
grep -q 'vitFontes\[r\] = collection->sources\[r\];' src/vertudo.c
grep -q 'static int vitTipoFonte(const ColSource \*fonte)' src/vertudo.c
grep -q 'vitTipoFonte(&vitFontes\[r\])' src/vertudo.c
grep -q 'strcmp(it->tipo, tipoFonte ? "series" : "movie")' src/vertudo.c
grep -q 'static int vitMesmoItem(const CatItem \*a, const CatItem \*b)' src/vertudo.c
grep -q 'vitMesmoItem(ja, it)' src/vertudo.c
grep -q 'if (!noTudo && vitFR >= 0 && vitFR < vitN)' src/vertudo.c
grep -q 'ColSource fonteFocoAntes;' src/vertudo.c
grep -q 'memcmp(&vitFontes\[r\], &fonteFocoAntes, sizeof fonteFocoAntes)' src/vertudo.c
grep -q 'int r = i / vitCap, c = i % vitCap;' src/vertudo.c
grep -q 'r \* vitCap + c, vitPonteiroGeracao' src/vertudo.c
if grep -q 'vitAgrupar' src/vertudo.c; then
  echo "Vitrine ainda agrupa uma fonte por década/tipo" >&2
  exit 1
fi
echo "vertudo follow-layout collections: ok"
