"""Plot gate count and score comparisons from metrics.csv."""
from __future__ import annotations

import csv
from pathlib import Path
from typing import Dict, List

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

PYTHON_DIR = Path(__file__).resolve().parents[1]
EXPERIMENT_DIR = PYTHON_DIR / "experiments"
METRICS_CSV = EXPERIMENT_DIR / "metrics.csv"
OUTPUT_PNG = EXPERIMENT_DIR / "metrics.png"


def load_metrics() -> List[Dict[str, str]]:
    rows: List[Dict[str, str]] = []
    with METRICS_CSV.open() as fh:
        reader = csv.DictReader(fh)
        for row in reader:
            if row.get("status") != "ok":
                continue
            rows.append(row)
    return rows


def group_by_circuit(rows: List[Dict[str, str]]):
    grouped: Dict[str, Dict[str, Dict[str, float]]] = {}
    for row in rows:
        circuit = row["circuit"]
        pipeline = row["pipeline"]
        grouped.setdefault(circuit, {})[pipeline] = {
            "gates": float(row["gates"]),
            "score": float(row["score"]),
        }
    return grouped


def plot(grouped) -> None:
    circuits = sorted(grouped.keys())
    pipelines = ["baseline", "hlm"]

    fig, axes = plt.subplots(1, 2, figsize=(10, 4))
    for idx, metric in enumerate(["gates", "score"]):
        ax = axes[idx]
        for offset, pipeline in enumerate(pipelines):
            xs = []
            ys = []
            for i, circuit in enumerate(circuits):
                data = grouped[circuit]
                if pipeline not in data:
                    continue
                xs.append(i + offset * 0.35)
                ys.append(data[pipeline][metric])
            ax.bar(xs, ys, width=0.3, label=pipeline if idx == 0 else "")
        ax.set_xticks([i + 0.15 for i in range(len(circuits))])
        ax.set_xticklabels(circuits, rotation=30, ha="right")
        ax.set_ylabel(metric)
    axes[0].legend()
    fig.tight_layout()
    fig.savefig(OUTPUT_PNG)
    plt.close(fig)


def main() -> None:
    rows = load_metrics()
    if not rows:
        print("No completed runs to plot.")
        return
    grouped = group_by_circuit(rows)
    plot(grouped)
    print(f"Wrote {OUTPUT_PNG}")


if __name__ == "__main__":
    main()
