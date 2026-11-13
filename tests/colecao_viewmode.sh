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

  // FOLLOW_LAYOUT follows the preference selected in the Nuvio app.
  pasta.viewMode = COL_VIEW_FOLLOW_LAYOUT;
  padrao = VT_LAY_MURAL;
  assert(layAuto() == VT_LAY_MURAL);
  padrao = VT_LAY_VITRINE;
  assert(layAuto() == VT_LAY_VITRINE);

  // Explicit Rows always uses the row-based presentation.
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


# Verify that a disabled add-on cannot reuse stale Collection results.
python3 - <<'PY'
from pathlib import Path
import re
import subprocess
import tempfile

source = Path("src/descoberta.c").read_text(encoding="utf-8")
match = re.search(
    r"void desc_vertudo_filtro_desde\s*\([^;]*?\)\s*\{",
    source,
    re.S,
)
assert match, "desc_vertudo_filtro_desde() missing"
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

prefix = r"""
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

static pthread_mutex_t vtTrava = PTHREAD_MUTEX_INITIALIZER;
static char vtBase[128], vtTipo[16], vtCat[64], vtGenre[64];
static int vtN, vtPagina, vtFim, vtErro, vtProvedor, disparos;
static unsigned vtGeracao;
static int catalogoAtivo;

static int addons_catalogo_base_ativo(const char *base) {
  (void)base;
  return catalogoAtivo;
}
static int ajustes_ocultar_filmes_vistos(void) { return 0; }
static int vtProjetar(void) { return 0; }
static void vtDisparar(void) { disparos++; }
"""

main = r"""
int main(void) {
  strcpy(vtBase, "https://addon.invalid");
  strcpy(vtTipo, "movie");
  strcpy(vtCat, "popular");
  vtN = 4;
  vtGeracao = 7;
  catalogoAtivo = 0;

  desc_vertudo_filtro_desde("https://addon.invalid", "movie", "popular", "", 0);
  assert(vtN == 0);
  assert(vtFim == 1);
  assert(vtGeracao == 8);
  assert(disparos == 0);

  // Re-enabling the same add-on starts a fresh query instead of using cache.
  catalogoAtivo = 1;
  desc_vertudo_filtro_desde("https://addon.invalid", "movie", "popular", "", 0);
  assert(vtN == 0);
  assert(vtFim == 0);
  assert(vtGeracao == 9);
  assert(disparos == 1);
  return 0;
}
"""

with tempfile.TemporaryDirectory(prefix="nuvio-collection-addon-cache-") as folder:
    cfile = Path(folder) / "cache.c"
    binary = Path(folder) / "cache"
    cfile.write_text(prefix + function + main, encoding="utf-8")
    subprocess.run(
        ["cc", "-std=c11", "-Wall", "-Wextra", "-pthread", str(cfile), "-o", str(binary)],
        check=True,
    )
    subprocess.run([str(binary)], check=True)

print("collection add-on cache: disabled clears stale results; re-enable reloads")
PY

# The current addonId must override an old saved base URL; an unresolved ID
# must not silently become the source identity.
python3 - <<'PY'
from pathlib import Path
import re
import subprocess
import tempfile

source = Path("src/vertudo.c").read_text(encoding="utf-8")
match = re.search(r"static const char \*baseDaFonte\s*\(\s*const ColSource \*s\s*\)\s*\{", source)
assert match, "baseDaFonte() missing"
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

prefix = r"""
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "colecoes.h"
#include "addonurl.h"

static int idKnown;
static const char *currentBase = "https://catalog.nuvio.tv/current";
int addons_base_por_id_copiar(const char *id, char *dst, unsigned tam) {
  if (!idKnown || !id || strcmp(id, "com.nuvio.tmdb.catalogs") || !dst || !tam)
    return 0;
  snprintf(dst, tam, "%s", currentBase);
  return 1;
}
"""
main = r"""
int main(void) {
  ColSource source = {0};
  snprintf(source.addonId, sizeof source.addonId, "com.nuvio.tmdb.catalogs");
  snprintf(source.base, sizeof source.base, "https://catalog.nuvio.tv/old");
  idKnown = 1;
  assert(!strcmp(baseDaFonte(&source), currentBase)); // id is authoritative

  idKnown = 0;
  assert(!strcmp(baseDaFonte(&source), source.base)); // fallback remains for legacy refs
  source.addonId[0] = 0;
  assert(!strcmp(baseDaFonte(&source), source.base));
  source.base[0] = 0;
  assert(!baseDaFonte(&source)[0]);
  return 0;
}
"""
with tempfile.TemporaryDirectory(prefix="nuvio-collection-source-id-") as folder:
    cfile = Path(folder) / "source.c"
    binary = Path(folder) / "source"
    cfile.write_text(prefix + function + main, encoding="utf-8")
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Isrc", str(cfile), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print("collection source identity: current addonId base overrides stale persisted URL")
PY

# Test the real active-catalog predicate against enabled, disabled, non-catalog,
# and removed add-ons.
python3 - <<'PY'
from pathlib import Path
import re
import subprocess
import tempfile

source = Path("src/addons.c").read_text(encoding="utf-8")
match = re.search(r"int addons_catalogo_base_ativo\s*\(\s*const char \*base\s*\)\s*\{", source)
assert match, "addons_catalogo_base_ativo() missing"
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

prefix = r"""
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#define NV_ADDON_URL_MAX 4096
static struct { char base[NV_ADDON_URL_MAX]; int ativo, catalogo; } addon[4];
static int nAddon;
static pthread_mutex_t addonTrava = PTHREAD_MUTEX_INITIALIZER;
static void baseNormalizada(const char *url, char *dst, size_t tam) {
  snprintf(dst, tam, "%s", url ? url : "");
}
"""
main = r"""
int main(void) {
  snprintf(addon[0].base, sizeof addon[0].base, "https://catalog.nuvio.tv");
  addon[0].ativo = 1;
  addon[0].catalogo = 1;
  nAddon = 1;
  assert(addons_catalogo_base_ativo("https://catalog.nuvio.tv"));

  addon[0].ativo = 0;
  assert(!addons_catalogo_base_ativo("https://catalog.nuvio.tv"));
  addon[0].ativo = 1;
  addon[0].catalogo = 0;
  assert(!addons_catalogo_base_ativo("https://catalog.nuvio.tv"));
  addon[0].catalogo = 1;
  assert(!addons_catalogo_base_ativo("https://removed-addon.invalid"));
  nAddon = 0;
  assert(!addons_catalogo_base_ativo("https://catalog.nuvio.tv"));
  return 0;
}
"""
with tempfile.TemporaryDirectory(prefix="nuvio-collection-addon-capability-") as folder:
    cfile = Path(folder) / "active.c"
    binary = Path(folder) / "active"
    cfile.write_text(prefix + function + main, encoding="utf-8")
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-pthread", str(cfile), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print("collection catalog capability: active catalog required; disabled, non-catalog and removed rejected")
PY
