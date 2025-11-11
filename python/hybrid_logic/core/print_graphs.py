"""High-level helpers that mirror the MATLAB print_graphs routine."""
from __future__ import annotations

from dataclasses import dataclass
from typing import Iterable, List, Sequence

from hybrid_logic.core.tree_builder import TreeBuildResult, reconstruct_tree
from hybrid_logic.data.loader import MatlabRepository


@dataclass(slots=True)
class GraphBuildConfig:
    nb_inputs: int
    index: int
    permutation: Sequence[int]
    decimal: int


def build_tree_from_repo(
    repo: MatlabRepository,
    cfg: GraphBuildConfig,
) -> TreeBuildResult:
    """Retrieve the ASCII graph, apply the permutation, and build the tree."""

    graph_rows = repo.summary.graphs.get(cfg.nb_inputs)[cfg.index]
    permuted_rows = _apply_permutation(graph_rows, cfg.permutation)
    max_gate_value = int(repo.tables.max_gates[cfg.nb_inputs][cfg.index])
    result = reconstruct_tree(permuted_rows, max_gate_value=max_gate_value)
    if getattr(repo.tables, "qs_nb", None) is not None:
        result.nb_qs = int(repo.tables.qs_nb[cfg.decimal])
    return result


# ---------------------------------------------------------------------------
# Internal helpers


def _apply_permutation(lines: Sequence[str], permutation: Sequence[int]) -> List[str]:
    perm_lines = list(lines)
    temp_replacements = [(f"x_{idx}", f"y_{target}") for idx, target in enumerate(permutation)]
    permuted = [_multi_replace(line, temp_replacements) for line in perm_lines]
    final = [_multi_replace(line, [(f"y_{idx}", f"x_{idx}") for idx in range(4)]) for line in permuted]
    return final


def _multi_replace(line: str, replacements: Iterable[tuple[str, str]]) -> str:
    result = line
    for old, new in replacements:
        result = result.replace(old, new)
    return result


__all__ = ["GraphBuildConfig", "build_tree_from_repo"]
