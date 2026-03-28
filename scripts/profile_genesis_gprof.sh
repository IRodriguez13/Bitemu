#!/usr/bin/env bash
# Perfilado gprof con ROM Genesis (sin GUI). Requiere ROM en disco (p. ej. scripts/fetch_gentest_rom.sh).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
ROM="${1:-$ROOT/tools/gentest_game.bin}"
FRAMES="${2:-5000}"
if [[ ! -f "$ROM" ]]; then
  echo "No está la ROM: $ROM"
  echo "Ejecutá: bash scripts/fetch_gentest_rom.sh"
  exit 1
fi
make profile-cli
rm -f gmon.out gprof.out
./bitemu-cli -rom "$ROM" -frames "$FRAMES"
make profile-report
echo "--- Flat profile (primeras líneas) ---"
grep -A 25 'Flat profile' gprof.out | head -30
