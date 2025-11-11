"""Matplotlib helper for visualising NOR trees."""
from __future__ import annotations

from dataclasses import dataclass
from typing import Dict, Iterable, List, Optional, Sequence

import matplotlib.pyplot as plt

from hybrid_logic.core.tree_builder import NodeInfo, TreeBuildResult


@dataclass(slots=True)
class TreeLayout:
    x: List[float]
    y: List[float]
    children: List[List[int]]


def draw_tree(
    tree: TreeBuildResult,
    ax: Optional[plt.Axes] = None,
    highlight: Optional[Iterable[int]] = None,
) -> plt.Axes:
    """Render the NOR tree with a deterministic layered layout."""

    if ax is None:
        _, ax = plt.subplots(figsize=(6, 4))

    layout = _compute_layout(tree)
    highlight_set = set(highlight or [])

    for parent_idx, child_indices in enumerate(layout.children):
        for child_idx in child_indices:
            ax.plot(
                [layout.x[parent_idx], layout.x[child_idx]],
                [layout.y[parent_idx], layout.y[child_idx]],
                color="black",
                linewidth=2 if child_idx in highlight_set else 1,
            )

    for idx, node in enumerate(tree.nodes):
        color = "#2ca02c" if node.is_terminal else "#d62728"
        ax.scatter(
            layout.x[idx],
            layout.y[idx],
            s=120,
            c=color,
            edgecolors="black",
            zorder=3,
        )
        ax.text(
            layout.x[idx],
            layout.y[idx] + 0.03,
            node.name,
            ha="center",
            va="bottom",
            fontsize=10,
        )

    ax.set_axis_off()
    ax.set_ylim(min(layout.y) - 0.1, max(layout.y) + 0.15)
    ax.set_xlim(min(layout.x) - 0.1, max(layout.x) + 0.1)
    return ax


# ---------------------------------------------------------------------------
# Layout helpers


def _compute_layout(tree: TreeBuildResult) -> TreeLayout:
    parents = [p - 1 for p in tree.parents]
    children: List[List[int]] = [[] for _ in tree.nodes]
    for child_idx, parent_idx in enumerate(parents):
        if parent_idx >= 0:
            children[parent_idx].append(child_idx)

    depths = _compute_depths(parents)
    x_positions = [0.0] * len(tree.nodes)
    leaf_counter = 0

    def assign_positions(idx: int) -> float:
        nonlocal leaf_counter
        if not children[idx]:
            x_positions[idx] = float(leaf_counter)
            leaf_counter += 1
            return x_positions[idx]
        xs = [assign_positions(child) for child in children[idx]]
        x_positions[idx] = sum(xs) / len(xs)
        return x_positions[idx]

    assign_positions(0)

    # Normalise x coordinates to [0, 1]
    min_x = min(x_positions)
    max_x = max(x_positions)
    span = max(max_x - min_x, 1.0)
    norm_x = [(x - min_x) / span for x in x_positions]

    max_depth = max(depths) or 1
    norm_y = [1 - depth / max_depth for depth in depths]

    return TreeLayout(x=norm_x, y=norm_y, children=children)


def _compute_depths(parents: Sequence[int]) -> List[int]:
    depths = [0] * len(parents)
    for idx in range(len(parents)):
        depth = 0
        current = idx
        while parents[current] >= 0:
            depth += 1
            current = parents[current]
        depths[idx] = depth
    return depths


__all__ = ["draw_tree", "TreeLayout"]
