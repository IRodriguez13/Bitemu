#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

ROMSET="${GENESIS_ROMSET_NIGHTLY:-tools/genesis_romset.nightly.private.json}"
BASELINE="${GENESIS_BASELINE_NIGHTLY:-tools/genesis_baseline.nightly.private.json}"

if [[ ! -f "$ROMSET" ]]; then
  echo "Missing nightly romset: $ROMSET"
  echo "Create a larger private romset JSON for nightly runs."
  exit 2
fi

if [[ ! -f "$BASELINE" ]]; then
  echo "Missing nightly baseline: $BASELINE"
  echo "Run: make genesis-baseline GENESIS_ROMSET=$ROMSET GENESIS_BASELINE=$BASELINE"
  exit 2
fi

make test-core
make genesis-regression GENESIS_ROMSET="$ROMSET" GENESIS_BASELINE="$BASELINE"
