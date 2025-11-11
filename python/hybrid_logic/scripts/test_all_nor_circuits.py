"""Python port of ``test_allNORcircuits.m``.

This script iterates through every 4-input Boolean function (65,536 total),
looks up the canonical NOR2 realisations, and records the minimum gate usage
reported by the legacy MATLAB data.  It also generates the histogram and the
cumulative distribution plot found in the original workflow.

Usage:
    PYTHONPATH=python ./.venv/bin/python -m hybrid_logic.scripts.test_all_nor_circuits
"""
from __future__ import annotations

import argparse
from pathlib import Path
from typing import Tuple

import matplotlib.pyplot as plt
import numpy as np

from hybrid_logic.core.config import build_config_for_decimal
from hybrid_logic.core.print_graphs import build_tree_from_repo
from hybrid_logic.data.loader import HybridDataError, load_repository


def _compute_gate_statistics(limit: int | None = None) -> Tuple[np.ndarray, np.ndarray]:
    repo = load_repository()
    summary = repo.summary.summary
    qs_nb = np.full(65536, -1, dtype=np.int8)
    nb_gates = np.zeros(65536, dtype=np.int8)

    qs_nb[0] = 0
    qs_nb[-1] = 0

    iterator = range(1, 65535 if limit is None else min(limit, 65535))

    for i in iterator:
        nb_decimal = i
        nb_inputs = int(summary[nb_decimal, 1])
        if nb_inputs <= 1:
            qs_nb[nb_decimal] = 0
            nb_gates[nb_decimal] = 1 if nb_inputs == 1 else 0
            continue
        try:
            cfg = build_config_for_decimal(repo, nb_decimal)
        except HybridDataError:
            qs_nb[nb_decimal] = 0
            nb_gates[nb_decimal] = 0
            continue
        nb_gates[nb_decimal] = int(repo.tables.max_gates[nb_inputs][cfg.index])
        tree = build_tree_from_repo(repo, cfg)
        qs_nb[nb_decimal] = tree.nb_qs

    return qs_nb, nb_gates


def _plot_histogram(nb_gates: np.ndarray, output: Path) -> None:
    fig, axes = plt.subplots(1, 2, figsize=(10, 4), dpi=150)

    bins = np.linspace(0.5, 13.5, 14)
    axes[0].hist(nb_gates, bins=bins, edgecolor="black")
    axes[0].set_xlabel("Minimum number of required NOR2 gates", fontweight="bold")
    axes[0].set_ylabel("Number of Boolean functions", fontweight="bold")
    axes[0].grid(True, linestyle=":")
    axes[0].set_xticks(range(0, 15))

    counts, _ = np.histogram(nb_gates, bins=np.linspace(0.5, 14.5, 15))
    cumulative = np.cumsum(counts) / counts.sum()
    x = np.arange(1, 15)
    axes[1].plot(x, cumulative[:14], color="black", linewidth=1.5)
    axes[1].scatter(x, cumulative[:14], color="black")
    axes[1].set_xlabel("Minimum number of required NOR2 gates", fontweight="bold")
    axes[1].set_ylabel("Cumulative fraction of realisable Boolean functions", fontweight="bold")
    axes[1].grid(True, linestyle=":")
    axes[1].set_xticks(range(0, 15))

    fig.tight_layout()
    fig.savefig(output)
    plt.close(fig)


def main() -> None:
    parser = argparse.ArgumentParser(description="Analyse NOR2 gate counts for all Boolean functions.")
    parser.add_argument("--limit", type=int, default=None, help="Optional limit for debugging.")
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("figure1.png"),
        help="Path to save the histogram figure (default: figure1.png).",
    )
    args = parser.parse_args()

    qs_nb, nb_gates = _compute_gate_statistics(limit=args.limit)
    _plot_histogram(nb_gates, args.output)
    print(f"Saved histogram to {args.output}")


if __name__ == "__main__":
    main()
