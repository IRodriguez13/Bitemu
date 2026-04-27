#!/usr/bin/env python3
"""
Run private Genesis ROM regressions using tools/genesis_rom_probe output.
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from pathlib import Path
from typing import Any, Dict, List


FRAME_RE = re.compile(r"frames=(\d+)\s+video=(\d+)x(\d+)\s+hz=([0-9.]+)")
STATS_RE = re.compile(r"cpu_68k_cyc=(\d+)\s+z80_cyc=(\d+)\s+dma_stall_68k=(\d+)")
FB_RE = re.compile(r"sum_bytes=(\d+)\s+weighted=(\d+)")


def run_probe(probe: Path, rom_path: Path, frames: int) -> Dict[str, Any]:
    proc = subprocess.run(
        [str(probe), str(rom_path), str(frames)],
        check=False,
        capture_output=True,
        text=True,
    )

    out = proc.stdout + "\n" + proc.stderr
    frame_m = FRAME_RE.search(out)
    stats_m = STATS_RE.search(out)
    fb_m = FB_RE.search(out)

    result: Dict[str, Any] = {
        "frames": frames,
        "probe_exit_code": proc.returncode,
        "ok": proc.returncode == 0 and frame_m is not None and stats_m is not None and fb_m is not None,
        "stdout": proc.stdout.strip(),
        "stderr": proc.stderr.strip(),
    }
    if not result["ok"]:
        return result

    result.update(
        {
            "video_w": int(frame_m.group(2)),
            "video_h": int(frame_m.group(3)),
            "hz": float(frame_m.group(4)),
            "cpu_68k_cyc": int(stats_m.group(1)),
            "z80_cyc": int(stats_m.group(2)),
            "dma_stall_68k": int(stats_m.group(3)),
            "sum_bytes": int(fb_m.group(1)),
            "weighted": int(fb_m.group(2)),
        }
    )
    return result


def compare_results(current: Dict[str, Any], baseline: Dict[str, Any]) -> List[str]:
    diffs: List[str] = []
    hard_fields = ("video_w", "video_h", "sum_bytes", "weighted")
    for f in hard_fields:
        if current.get(f) != baseline.get(f):
            diffs.append(f"{f}: current={current.get(f)} baseline={baseline.get(f)}")
    if not current.get("ok", False):
        diffs.append("probe failed")
    return diffs


def main() -> int:
    ap = argparse.ArgumentParser(description="Genesis private ROM regression runner")
    ap.add_argument("--probe", required=True, help="Path to genesis_rom_probe binary")
    ap.add_argument("--romset", required=True, help="Path to private romset JSON file")
    ap.add_argument("--output", required=True, help="JSON output report")
    ap.add_argument("--baseline", help="JSON baseline to compare against")
    ap.add_argument("--write-baseline", help="Write current run as baseline")
    args = ap.parse_args()

    probe = Path(args.probe)
    romset_path = Path(args.romset)
    output_path = Path(args.output)
    baseline_path = Path(args.baseline) if args.baseline else None
    write_baseline_path = Path(args.write_baseline) if args.write_baseline else None

    if not probe.exists():
        print(f"Probe not found: {probe}", file=sys.stderr)
        return 2
    if not romset_path.exists():
        print(f"Romset file not found: {romset_path}", file=sys.stderr)
        return 2

    with romset_path.open("r", encoding="utf-8") as fh:
        romset = json.load(fh)

    entries = romset.get("entries", [])
    if not isinstance(entries, list) or not entries:
        print("Romset must define non-empty 'entries'", file=sys.stderr)
        return 2

    baseline_data: Dict[str, Any] = {}
    if baseline_path:
        if not baseline_path.exists():
            print(f"Baseline file not found: {baseline_path}", file=sys.stderr)
            return 2
        with baseline_path.open("r", encoding="utf-8") as fh:
            baseline_data = json.load(fh)

    report: Dict[str, Any] = {"entries": []}
    any_diff = False

    for entry in entries:
        entry_id = entry["id"]
        rom_path = Path(entry["path"])
        milestones = entry.get("milestones", [120])
        if not rom_path.exists():
            report["entries"].append(
                {"id": entry_id, "path": str(rom_path), "error": "ROM file does not exist", "runs": []}
            )
            any_diff = True
            continue

        runs = []
        for frames in milestones:
            res = run_probe(probe, rom_path, int(frames))
            if baseline_data:
                baseline_entry = baseline_data.get("entries", {}).get(entry_id, {}).get(str(frames))
                if baseline_entry:
                    diffs = compare_results(res, baseline_entry)
                    res["diffs"] = diffs
                    if diffs:
                        any_diff = True
                else:
                    res["diffs"] = ["missing baseline"]
                    any_diff = True
            runs.append(res)
        report["entries"].append({"id": entry_id, "path": str(rom_path), "runs": runs})

    output_path.parent.mkdir(parents=True, exist_ok=True)
    with output_path.open("w", encoding="utf-8") as fh:
        json.dump(report, fh, indent=2, sort_keys=True)

    if write_baseline_path:
        baseline_out: Dict[str, Any] = {"entries": {}}
        for entry in report["entries"]:
            ekey = entry["id"]
            baseline_out["entries"][ekey] = {}
            for run in entry["runs"]:
                frame_key = str(run["frames"])
                baseline_out["entries"][ekey][frame_key] = {
                    "ok": run.get("ok", False),
                    "video_w": run.get("video_w"),
                    "video_h": run.get("video_h"),
                    "hz": run.get("hz"),
                    "cpu_68k_cyc": run.get("cpu_68k_cyc"),
                    "z80_cyc": run.get("z80_cyc"),
                    "dma_stall_68k": run.get("dma_stall_68k"),
                    "sum_bytes": run.get("sum_bytes"),
                    "weighted": run.get("weighted"),
                }
        write_baseline_path.parent.mkdir(parents=True, exist_ok=True)
        with write_baseline_path.open("w", encoding="utf-8") as fh:
            json.dump(baseline_out, fh, indent=2, sort_keys=True)

    print(f"Regression report written to {output_path}")
    if write_baseline_path:
        print(f"Baseline written to {write_baseline_path}")
    if baseline_data and any_diff:
        print("Regressions detected against baseline", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
