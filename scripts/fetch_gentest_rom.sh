#!/usr/bin/env bash
# Descarga la ROM de ejemplo "gentest" (clbr/gentest, SAMPLE PROGRAM, ~3.4 MiB descomprimido).
# Licencia: ver repositorio https://github.com/clbr/gentest
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/tools/gentest_game.bin"
XZ="$ROOT/tools/gentest_game.bin.xz"
mkdir -p "$ROOT/tools"
if [[ -f "$OUT" && -s "$OUT" ]]; then
  echo "Ya existe: $OUT"
  exit 0
fi
curl -fsSL -o "$XZ" "https://raw.githubusercontent.com/clbr/gentest/master/game.bin.xz"
xz -dc "$XZ" > "$OUT"
rm -f "$XZ"
echo "OK: $OUT ($(wc -c < "$OUT") bytes)"
