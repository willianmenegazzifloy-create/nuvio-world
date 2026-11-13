#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."

python3 - <<'PY'
from pathlib import Path
import re
import subprocess
import tempfile

source = Path("src/vertudo.c").read_text(encoding="utf-8")
match = re.search(r"static int layAuto\s*\(\s*void\s*\)\s*\{", source)
assert match, "layAuto() missing"
start = match.start()
brace = source.index("{", match.start())
depth = 1
end = brace + 1
while depth:
    if source[end] == "{":
        depth += 1
    elif source[end] == "}":
        depth -= 1
    end += 1
function = source[start:end]

# Guard the production capacity and provider path as part of this regression.
assert "#define VIT_LIN_MAX COL_SOURCE_MAX" in source
assert "collection->viewMode == COL_VIEW_FOLLOW_LAYOUT" in source
assert "collection->viewMode == COL_VIEW_ROWS" in source
assert "Fontes nao-addon ficam visiveis, mas vazias" not in source

prefix = r"""
#include <assert.h>
#include <string.h>
#include "colecoes.h"

enum {
  VT_LAY_AUTO = 0, VT_LAY_GRADE, VT_LAY_LINHA,
  VT_LAY_RANKING, VT_LAY_SAGA, VT_LAY_DECADAS,
  VT_LAY_MURAL, VT_LAY_VITRINE, VT_LAY_FLUXO, VT_LAY_N
};

static ColFolder pasta;
static ColFolder *collection = &pasta;
static int semVitrineAuto;
static int padrao;
static int diretores;

static int ajustes_colecao_exibicao(void) { return padrao; }
static int grupo(const char *nome) {
  return diretores && strcmp(nome, "Directors") == 0;
}
"""

main = r"""
int main(void) {
  memset(&pasta, 0, sizeof pasta);

  // The account's Follow Layout must resolve to source rows.
  pasta.viewMode = COL_VIEW_FOLLOW_LAYOUT;
  assert(layAuto() == VT_LAY_VITRINE);

  // Explicit Rows also uses the World row-based presentation.
  pasta.viewMode = COL_VIEW_ROWS;
  assert(layAuto() == VT_LAY_VITRINE);

  // Tabbed Grid remains the source-tab grid.
  pasta.viewMode = COL_VIEW_TABBED_GRID;
  assert(layAuto() == VT_LAY_GRADE);

  // Older package collections without viewMode keep the local default.
  pasta.viewMode = COL_VIEW_UNSPECIFIED;
  padrao = VT_LAY_MURAL;
  assert(layAuto() == VT_LAY_MURAL);

  // AUTO still falls back to the historical Grade.
  padrao = VT_LAY_AUTO;
  assert(layAuto() == VT_LAY_GRADE);

  // The Directors automatic rule remains intact.
  diretores = 1;
  assert(layAuto() == VT_LAY_LINHA);

  // "See all" ignores the parent collection view and falls back to Grade.
  diretores = 0;
  semVitrineAuto = 1;
  pasta.viewMode = COL_VIEW_FOLLOW_LAYOUT;
  padrao = VT_LAY_MURAL;
  assert(layAuto() == VT_LAY_GRADE);

  collection = NULL;
  assert(layAuto() == VT_LAY_GRADE);
  return 0;
}
"""

with tempfile.TemporaryDirectory(prefix="nuvio-collection-viewmode-") as folder:
    cfile = Path(folder) / "viewmode.c"
    binary = Path(folder) / "viewmode"
    cfile.write_text(prefix + function + main, encoding="utf-8")
    subprocess.run(
        ["cc", "-std=c11", "-Wall", "-Wextra", "-Isrc", str(cfile), "-o", str(binary)],
        check=True,
    )
    subprocess.run([str(binary)], check=True)

print("collection viewMode: FOLLOW_LAYOUT, ROWS, TABBED_GRID and legacy defaults PASS")
PY
