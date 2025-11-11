"""Convert NOR trees into structural Verilog netlists."""
from __future__ import annotations

from dataclasses import dataclass
from typing import Dict, List, Optional

from hybrid_logic.core.tree_builder import TreeBuildResult
from hybrid_logic.verilog.parser import ParsedModule

__all__ = ["NetlistRenderError", "render_structural_verilog"]


class NetlistRenderError(RuntimeError):
    """Raised when the NOR tree cannot be converted into Verilog."""


def render_structural_verilog(module: ParsedModule, tree: TreeBuildResult) -> str:
    if not tree.nodes:
        raise NetlistRenderError("Empty NOR tree")

    root_idx = _find_root_index(tree)
    root_node = tree.nodes[root_idx]

    if root_node.is_terminal:
        source = _resolve_terminal(module, root_node.name)
        return _emit_module(
            module,
            body_lines=[f"  assign {module.output} = {source};"],
            wires=[],
        )

    children = _build_children(tree)
    representatives: Dict[str, int] = {}
    for idx, node in enumerate(tree.nodes):
        if node.is_terminal:
            continue
        if children[idx] and node.name not in representatives:
            representatives[node.name] = idx
    signal_cache: Dict[int, str] = {}
    name_cache: Dict[str, str] = {}
    wires: List[str] = []
    instances: List[str] = []
    instance_counter = 0

    def emit(node_idx: int) -> str:
        nonlocal instance_counter
        if node_idx in signal_cache:
            return signal_cache[node_idx]

        node = tree.nodes[node_idx]
        if not node.is_terminal:
            canonical_idx = representatives.get(node.name)
            if canonical_idx is not None and canonical_idx != node_idx:
                signal = emit(canonical_idx)
                signal_cache[node_idx] = signal
                name_cache[node.name] = signal
                return signal
            if node.name in name_cache:
                signal = name_cache[node.name]
                signal_cache[node_idx] = signal
                return signal
        if node.is_terminal:
            signal = _resolve_terminal(module, node.name)
            signal_cache[node_idx] = signal
            return signal

        child_indices = sorted(children[node_idx], key=lambda idx: tree.nodes[idx].start_col)
        if not child_indices:
            raise NetlistRenderError(f"Gate node '{node.name}' lacks inputs")
        child_signals = [emit(idx) for idx in child_indices]

        if len(child_signals) > 2:
            raise NetlistRenderError(
                f"Gate node '{node.name}' has {len(child_signals)} fan-in; only NOR2 is supported"
            )

        target = module.output if node_idx == root_idx else f"n{len(wires)}"
        if node_idx != root_idx:
            wires.append(target)

        instances.append(_render_nor_instance(instance_counter, target, child_signals))
        instance_counter += 1

        signal_cache[node_idx] = target
        name_cache[node.name] = target
        return target

    emit(root_idx)
    return _emit_module(module, body_lines=instances, wires=wires)


def _render_nor_instance(index: int, target: str, inputs: List[str]) -> str:
    if len(inputs) == 1:
        a = inputs[0]
        return f"  nor NOR_{index} ({target}, {a}, {a});"
    if len(inputs) == 2:
        a, b = inputs
        return f"  nor NOR_{index} ({target}, {a}, {b});"
    raise NetlistRenderError("Internal error: unexpected fan-in count")


def _find_root_index(tree: TreeBuildResult) -> int:
    for idx, parent in enumerate(tree.parents):
        if parent == 0:
            return idx
    raise NetlistRenderError("Unable to locate root node")


def _build_children(tree: TreeBuildResult) -> List[List[int]]:
    children: List[List[int]] = [[] for _ in tree.nodes]
    for idx, parent in enumerate(tree.parents):
        parent_idx = parent - 1
        if parent_idx >= 0:
            children[parent_idx].append(idx)
    return children


def _resolve_terminal(module: ParsedModule, name: str) -> str:
    try:
        suffix = int(name.split("_")[1])
    except (IndexError, ValueError) as exc:
        raise NetlistRenderError(f"Unexpected terminal identifier '{name}'") from exc
    if suffix >= len(module.inputs):
        raise NetlistRenderError(
            f"Tree references input index {suffix}, but module only has {len(module.inputs)} input(s)"
        )
    return module.inputs[suffix]


def _emit_module(module: ParsedModule, *, body_lines: List[str], wires: List[str]) -> str:
    port_specs: List[str] = []
    for port in module.ports:
        if port == module.output:
            port_specs.append(f"output {port}")
        elif port in module.inputs:
            port_specs.append(f"input {port}")
        else:
            raise NetlistRenderError(f"Unknown port '{port}' in module declaration")

    lines: List[str] = [f"module {module.name} ({', '.join(port_specs)});"]
    if wires:
        unique_wires = list(dict.fromkeys(wires))
        lines.append(f"  wire {', '.join(unique_wires)};")
    lines.extend(body_lines)
    lines.append("endmodule")
    return "\n".join(lines)
