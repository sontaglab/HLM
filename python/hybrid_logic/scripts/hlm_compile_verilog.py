"""Compile behavioural Verilog into a minimal NOR netlist using HLM data."""
from __future__ import annotations

import argparse
from pathlib import Path

import matplotlib.pyplot as plt

from hybrid_logic.core.config import build_config_for_decimal
from hybrid_logic.core.print_graphs import build_tree_from_repo
from hybrid_logic.data.loader import HybridDataError, load_repository
from hybrid_logic.verilog import (
    VerilogParseError,
    derive_truth_table,
    parse_verilog_file,
    render_structural_verilog,
)
from hybrid_logic.viz.tree_plot import draw_tree


def main() -> None:
    parser = argparse.ArgumentParser(description="Minimise a behavioural Verilog module using the HLM library.")
    parser.add_argument("input", type=Path, help="Path to the behavioural Verilog source file.")
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        default=None,
        help="Destination path for the structural Verilog netlist (default: <input>_nor.v).",
    )
    parser.add_argument(
        "--emit-plot",
        type=Path,
        default=None,
        help="Optional path to save a PNG of the NOR tree.",
    )
    args = parser.parse_args()

    try:
        module = parse_verilog_file(args.input)
    except VerilogParseError as exc:
        raise SystemExit(f"Failed to parse Verilog: {exc}")

    truth = derive_truth_table(module)

    repo = load_repository()
    try:
        cfg = build_config_for_decimal(repo, truth.decimal)
    except HybridDataError as exc:
        raise SystemExit(f"Unable to locate matching NOR realisation: {exc}")

    tree = build_tree_from_repo(repo, cfg)
    netlist = render_structural_verilog(module, tree)

    output_path = args.output or args.input.with_name(f"{args.input.stem}_nor.v")
    output_path.write_text(netlist, encoding="utf-8")
    print(f"Wrote structural netlist to {output_path}")

    if args.emit_plot:
        fig, ax = plt.subplots(figsize=(8, 5), dpi=150)
        draw_tree(tree, ax=ax)
        ax.set_title(f"NOR realisation for decimal {truth.decimal}")
        fig.savefig(args.emit_plot)
        plt.close(fig)
        print(f"Saved NOR tree plot to {args.emit_plot}")


if __name__ == "__main__":
    main()
