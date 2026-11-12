#!/bin/bash
# Regressao estrutural: a Vitrine precisa respeitar o tipo da fonte e eliminar
# repeticoes dentro da propria fileira, sem misturar identidades de outras fontes.
set -eu
cd "$(dirname "$0")/.."
grep -q 'static int vitTipoFonte(const ColSource \*fonte)' src/vertudo.c
grep -q 'static int vitMesmoItem(const CatItem \*a, const CatItem \*b)' src/vertudo.c
grep -q 'vitTipoFonte(&vitFontes\[r\])' src/vertudo.c
grep -q 'strcmp(it->tipo, tipoFonte ? "series" : "movie")' src/vertudo.c
grep -q 'vitMesmoItem(ja, it)' src/vertudo.c
echo "vertudo vitrine categories: ok"
