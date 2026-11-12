#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
python3 - <<'PY'
from pathlib import Path
import re, subprocess, tempfile
s = Path("src/menu.c").read_text()

def function(name):
    m = re.search(r"static [^\n]+\b" + name + r"\([^\n]*\)\s*\{", s)
    assert m, "missing function: " + name
    end = m.end()
    depth = 1
    while depth:
        depth += (s[end] == "{") - (s[end] == "}")
        end += 1
    return s[m.start():end]

code = r"""
#include <assert.h>
#include <math.h>
#include <stdio.h>
static int tamanho = 0;
static float topoFocoEscala = 1.0f;
static int ajustes_menu_topo_tamanho(void) { return tamanho; }
""" + function("topoEscalaTamanho") + function("topoEscalaTexto") + r"""
int main(void) {
  float pequeno, medio, grande;
  tamanho = 0; pequeno = topoEscalaTexto();
  tamanho = 1; medio = topoEscalaTexto();
  tamanho = 2; grande = topoEscalaTexto();
  assert(fabsf(pequeno - 0.612f) < 0.001f);
  assert(fabsf(medio - 1.15f) < 0.001f);
  assert(fabsf(grande - 1.30f) < 0.001f);
  assert(pequeno < medio && medio < grande);
  puts("menu-topo: escala de texto pequeno proporcional PASS");
  return 0;
}
"""
with tempfile.TemporaryDirectory() as d:
    source = Path(d) / "test.c"
    source.write_text(code)
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", str(source),
                    "-lm", "-o", str(Path(d) / "test")], check=True)
    subprocess.run([str(Path(d) / "test")], check=True)
PY
