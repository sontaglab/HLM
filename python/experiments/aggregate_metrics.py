"""Aggregate gate count and score into a CSV summary."""
from __future__ import annotations

import csv
from pathlib import Path
from typing import Dict, Iterable, Tuple

from parse_metrics import parse_log

PYTHON_DIR = Path(__file__).resolve().parents[1]
EXPERIMENT_DIR = PYTHON_DIR / "experiments"
BASELINE_DIR = EXPERIMENT_DIR / "baseline"
HLM_DIR = EXPERIMENT_DIR / "hlm"
OUTPUT_CSV = EXPERIMENT_DIR / "metrics.csv"

CIRCUITS = ["majority3", "parity3", "four_input_combo"]
MODES: Dict[str, Path] = {
    "baseline": BASELINE_DIR,
    "hlm": HLM_DIR,
}


def collect_rows() -> Iterable[Tuple[str, str, Path]]:
    for circuit in CIRCUITS:
        for mode, base in MODES.items():
            yield circuit, mode, base / circuit / "log.log"


def main() -> None:
    rows = []
    for circuit, mode, log_path in collect_rows():
        if not log_path.exists():
            rows.append({
                "circuit": circuit,
                "pipeline": mode,
                "gates": "",
                "score": "",
                "status": "missing log",
            })
            continue
        metrics = parse_log(log_path)
        rows.append({
            "circuit": circuit,
            "pipeline": mode,
            "gates": metrics["gates"],
            "score": metrics["score"],
            "status": "ok",
        })

    with OUTPUT_CSV.open("w", newline="") as fh:
        writer = csv.DictWriter(fh, fieldnames=["circuit", "pipeline", "gates", "score", "status"])
        writer.writeheader()
        writer.writerows(rows)

    print(f"Wrote {OUTPUT_CSV}")


if __name__ == "__main__":
    main()
