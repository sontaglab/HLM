"""Tree reconstruction utilities ported from the MATLAB ``build_tree`` routine."""
from __future__ import annotations

from dataclasses import dataclass
from typing import Dict, Iterable, List, Sequence, Set

from hybrid_logic.data.loader import HybridDataError


@dataclass(slots=True)
class NodeInfo:
    """Metadata for a single node in the NOR tree."""

    name: str
    row: int
    start_col: int
    end_col: int
    index: int = -1

    @property
    def center(self) -> int:
        return self.start_col + (self.end_col - self.start_col) // 2

    @property
    def is_terminal(self) -> bool:
        return self.name.startswith("x_")


@dataclass(slots=True)
class TreeBuildResult:
    parents: List[int]
    nodes: List[NodeInfo]
    gate_counts: List[int]
    nb_qs: int


def reconstruct_tree(
    lines: Sequence[str],
    *,
    max_gate_value: int | None = None,
    threshold: int = 7,
) -> TreeBuildResult:
    """Reconstruct parent pointers, gate counts, and QS metadata."""

    nodes = _extract_nodes(lines)
    if not nodes:
        raise HybridDataError("Empty graph description")

    parents = [0] * len(nodes)
    nodes_by_line = _group_nodes_by_line(nodes)

    root = min(nodes, key=lambda n: (n.row, n.start_col))
    parents[root.index] = 0

    node_lines = sorted(nodes_by_line)
    for idx in range(len(node_lines) - 1):
        upper_line = node_lines[idx]
        lower_line = node_lines[idx + 1]
        connector_line = upper_line + 1
        if connector_line >= len(lines):
            continue
        events = _build_connector_events(lines[connector_line], nodes_by_line[upper_line])
        children = nodes_by_line[lower_line]
        if len(events) != len(children):
            raise HybridDataError(
                "Mismatch between connector count and child nodes (line "
                f"{lower_line}): {len(events)} vs {len(children)}"
            )
        for parent_idx, child in zip(events, children):
            parents[child.index] = parent_idx + 1

    gate_counts, nb_qs = _analyse_tree_matlab(
        parents_one_based=parents,
        node_names=[node.name for node in nodes],
        max_gate_value=max_gate_value,
        threshold=threshold,
    )

    return TreeBuildResult(parents=parents, nodes=nodes, gate_counts=gate_counts, nb_qs=nb_qs)


# ---------------------------------------------------------------------------
# Internal helpers


def _extract_nodes(lines: Sequence[str]) -> List[NodeInfo]:
    import re

    pattern = re.compile(r"(?:g|x)_\d+")
    nodes: List[NodeInfo] = []
    for row, line in enumerate(lines):
        for match in pattern.finditer(line):
            node = NodeInfo(
                name=match.group(),
                row=row,
                start_col=match.start(),
                end_col=match.end(),
            )
            nodes.append(node)
    nodes.sort(key=lambda n: (n.row, n.start_col))
    for idx, node in enumerate(nodes):
        node.index = idx
    return nodes


def _group_nodes_by_line(nodes: Sequence[NodeInfo]) -> Dict[int, List[NodeInfo]]:
    grouped: Dict[int, List[NodeInfo]] = {}
    for node in nodes:
        grouped.setdefault(node.row, []).append(node)
    for items in grouped.values():
        items.sort(key=lambda n: n.start_col)
    return grouped


def _build_connector_events(line: str, parents: Sequence[NodeInfo]) -> List[int]:
    events: List[tuple[int, int, int]] = []
    for pos in _find_positions(line, "/"):
        events.append((pos, _closest_parent(pos, parents), 2))
    for pos in _find_positions(line, "|"):
        events.append((pos, _closest_parent(pos, parents), 1))
    events.sort(key=lambda item: item[0])

    expanded: List[int] = []
    for _, parent_idx, multiplicity in events:
        expanded.extend([parent_idx] * multiplicity)
    return expanded


def _find_positions(line: str, token: str) -> List[int]:
    positions: List[int] = []
    start = 0
    while True:
        idx = line.find(token, start)
        if idx == -1:
            break
        positions.append(idx)
        start = idx + 1
    return positions


def _closest_parent(position: int, parents: Sequence[NodeInfo]) -> int:
    if not parents:
        raise HybridDataError("Connector encountered without parents")
    return min(parents, key=lambda parent: abs(parent.center - position)).index


def _analyse_tree_matlab(
    *,
    parents_one_based: Sequence[int],
    node_names: Sequence[str],
    max_gate_value: int | None,
    threshold: int,
) -> tuple[List[int], int]:
    n = len(node_names)
    names = [""] + list(node_names)
    nodes_vec = [0] + list(parents_one_based)

    terminal = [0] * (n + 1)
    for i in range(1, n + 1):
        terminal[i] = 1 if names[i].startswith("x_") else 0
    terminal_copy = terminal.copy()
    gates = [0] + [1 - terminal[i] for i in range(1, n + 1)]
    gates_number = [0] * (n + 1)

    unique_nodes = sorted(set(nodes_vec[1:]))
    for value in unique_nodes:
        if value <= 0:
            continue
        idx_list = [idx for idx in range(1, n + 1) if nodes_vec[idx] == value]
        if idx_list and all(terminal[idx] == 1 for idx in idx_list):
            gates_number[value] = 1
            gates[value] = 0
            terminal[value] = 1
            for j in range(1, n + 1):
                if names[value] == names[j]:
                    gates_number[j] = 1
                    gates[j] = 0

    for i in range(1, n + 1):
        if gates_number[i] == 1:
            terminal[i] = 1

    for _ in range(100):
        for i in range(1, n + 1):
            if gates[i] == 0:
                terminal[i] = 1
            if gates[i] > 0:
                children = [idx for idx in range(1, n + 1) if nodes_vec[idx] == i]
                if children and all(terminal[idx] == 1 for idx in children):
                    gates_number[i] = gates_number[i] + 1
                    for child in children:
                        gates_number[i] = gates_number[i] + gates_number[child]
                    gates[i] = 0
                    for j in range(1, n + 1):
                        if names[i] == names[j]:
                            gates_number[j] = gates_number[i]
                            gates[j] = 0

    if any(gates[1:]):
        raise HybridDataError("Could not resolve gate counts for all nodes")

    root_idx = next((idx for idx in range(1, n + 1) if nodes_vec[idx] == 0), 1)
    total_gates = gates_number[root_idx]

    nb_qs = -1
    qs_check = 0
    if max_gate_value is not None and max_gate_value <= threshold:
        nb_qs = 0
        qs_check = 1

    def _path_names(idx: int) -> List[str]:
        names_above: List[str] = []
        next_idx = idx
        while next_idx > 0:
            if next_idx != idx:
                names_above.append(names[next_idx])
            parent = nodes_vec[next_idx]
            if parent > 0:
                next_idx = parent
            else:
                next_idx = 0
        return _unique_preserve(names_above)

    if max_gate_value is not None and max_gate_value > threshold:
        for i in range(1, n + 1):
            if not (gates_number[i] > 0 and gates_number[i] < threshold):
                continue
            nb_repeats = sum(1 for j in range(1, n + 1) if names[i] == names[j])
            gates_above = _path_names(i)
            for j in range(1, n + 1):
                for entry in gates_above:
                    if terminal_copy[j] == 1 and names[j] == entry:
                        nb_repeats += 1
            final_total = total_gates - nb_repeats * gates_number[i]
            if final_total <= threshold:
                qs_check = 1
                if nb_qs != 0:
                    nb_qs = 1
                break

    if qs_check == 0:
        for i in range(2, n + 1):
            for j in range(2, n + 1):
                if i == j:
                    continue
                if names[i] == names[j]:
                    continue
                if not (gates_number[i] >= gates_number[j] and gates_number[j] > 0):
                    continue

                gates_above_i = _path_names(i)
                gates_above_j = _path_names(j)
                check_concatenated = 1 if names[i] in gates_above_j else 0

                nb_repeats = 0
                nb_repeats2 = 0
                for k in range(1, n + 1):
                    if names[i] == names[k]:
                        nb_repeats += 1
                    if names[j] == names[k]:
                        nb_repeats2 += 1
                    for entry in gates_above_i:
                        if terminal_copy[k] == 1 and names[k] == entry:
                            nb_repeats += 1
                    for entry in gates_above_j:
                        if terminal_copy[k] == 1 and names[k] == entry:
                            nb_repeats2 += 1

                if check_concatenated == 0:
                    final_total = (
                        total_gates
                        - nb_repeats * gates_number[i]
                        - nb_repeats2 * gates_number[j]
                    )
                    if (
                        final_total <= threshold
                        and gates_number[i] <= threshold
                        and gates_number[j] <= threshold
                    ):
                        return gates_number[1:], 2
                else:
                    diff = gates_number[i] - gates_number[j]
                    if diff > threshold:
                        continue
                    final_total = (
                        total_gates
                        - nb_repeats * diff
                        - nb_repeats2 * gates_number[j]
                    )
                    if final_total <= threshold and gates_number[j] <= threshold:
                        return gates_number[1:], 2

    return gates_number[1:], nb_qs


def _unique_preserve(values: Iterable[str]) -> List[str]:
    seen: Set[str] = set()
    ordered: List[str] = []
    for value in values:
        if value not in seen:
            seen.add(value)
            ordered.append(value)
    return ordered


__all__ = ["NodeInfo", "TreeBuildResult", "reconstruct_tree"]
