"""Generate a NOR tree visualisation for a specific Boolean function."""
from __future__ import annotations

import argparse
from pathlib import Path

import matplotlib.pyplot as plt

from hybrid_logic.core.config import build_config_for_decimal
from hybrid_logic.core.print_graphs import build_tree_from_repo
from hybrid_logic.data.loader import HybridDataError, load_repository
from hybrid_logic.viz.tree_plot import draw_tree


def main() -> None:
    parser = argparse.ArgumentParser(description="Render the NOR tree for a Boolean function.")
    parser.add_argument("decimal", type=int, help="Decimal representation of the Boolean function (0-65535).")
    parser.add_argument("--output", type=Path, default=None, help="Optional path to save the plot instead of displaying.")
    args = parser.parse_args()

    repo = load_repository()
    try:
        cfg = build_config_for_decimal(repo, args.decimal)
    except HybridDataError as exc:
        raise SystemExit(str(exc))

    tree = build_tree_from_repo(repo, cfg)
    fig, ax = plt.subplots(figsize=(8, 5), dpi=150)
    draw_tree(tree, ax=ax)
    ax.set_title(f"NOR realisation for decimal {args.decimal}")

    if args.output:
        fig.savefig(args.output)
        print(f"Saved plot to {args.output}")
    else:
        plt.show()


if __name__ == "__main__":
    main()
