#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
python3 tests/ajustes_grade_colunas.py
