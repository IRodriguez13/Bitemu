#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

ROMSET="${GENESIS_ROMSET:-tools/genesis_romset.private.json}"
BASELINE="${GENESIS_BASELINE:-tools/genesis_baseline.private.json}"

if [[ ! -f "$ROMSET" ]]; then
  echo "Missing quick romset: $ROMSET"
  echo "Copy tools/genesis_romset.private.example.json and adjust local ROM paths."
  exit 2
fi

if [[ ! -f "$BASELINE" ]]; then
  echo "Missing baseline: $BASELINE"
  echo "Run: make genesis-baseline GENESIS_ROMSET=$ROMSET GENESIS_BASELINE=$BASELINE"
  exit 2
fi

make test-core
make genesis-regression GENESIS_ROMSET="$ROMSET" GENESIS_BASELINE="$BASELINE"
