# Genesis production testing (local/private)

This workflow keeps commercial ROM testing local while still making regressions reproducible.

## 1) Create a private ROM set file

Use `tools/genesis_romset.private.example.json` as a template and write your private file:

```bash
cp tools/genesis_romset.private.example.json tools/genesis_romset.private.json
```

Each entry defines:
- `id`: stable identifier used in reports/baselines.
- `path`: absolute local path to the private ROM.
- `milestones`: frame counts to sample (`genesis_rom_probe` is run once per milestone).

## 2) Freeze a baseline

```bash
make genesis-baseline \
  GENESIS_ROMSET=tools/genesis_romset.private.json \
  GENESIS_BASELINE=tools/genesis_baseline.private.json
```

Artifacts:
- report: `build/genesis_regression_report.json`
- baseline: `tools/genesis_baseline.private.json`

## 3) Run local regressions

```bash
make genesis-regression \
  GENESIS_ROMSET=tools/genesis_romset.private.json \
  GENESIS_BASELINE=tools/genesis_baseline.private.json
```

The command fails with exit code `1` if framebuffer hashes or video size differ from baseline.

## 4) Local gates

Quick pre-merge:

```bash
make genesis-gate-quick
```

Nightly extended:

```bash
make genesis-gate-nightly
```

Both gates are intentionally local/private and do not require storing commercial ROMs in the repository.
