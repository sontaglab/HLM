"""Translate a parsed module into a canonical truth table index."""
from __future__ import annotations

from dataclasses import dataclass
from typing import Dict, Iterable, List, Sequence

from hybrid_logic.verilog.parser import ParsedModule

__all__ = ["TruthTableResult", "derive_truth_table"]


@dataclass(slots=True)
class TruthTableResult:
    inputs: List[str]
    bitstring: str
    decimal: int


class TruthTableError(RuntimeError):
    """Raised when the module cannot be mapped into the 4-input library."""


def derive_truth_table(module: ParsedModule) -> TruthTableResult:
    if len(module.inputs) > 4:
        raise TruthTableError("Modules with more than four inputs are not supported")

    # Always expand to four inputs to align with the MATLAB artefacts.
    vectors = _condition_vectors(4)
    bit_chars: List[str] = []

    for row in vectors:
        context: Dict[str, bool] = {}
        for idx, name in enumerate(module.inputs):
            context[name] = bool(row[idx])
        result = module.evaluate(context)
        bit_chars.append("1" if result else "0")

    bitstring = "".join(bit_chars)
    decimal = int(bitstring, 2)
    return TruthTableResult(inputs=list(module.inputs), bitstring=bitstring, decimal=decimal)


def _condition_vectors(nb_inputs: int) -> List[List[int]]:
    size = 1 << nb_inputs
    table = [[0 for _ in range(nb_inputs)] for _ in range(size)]

    g_start = 2
    for m in range(1, nb_inputs + 1):
        step = 1 << m
        run_length = (step // 2) - 1
        column = nb_inputs - m
        g = g_start
        while g <= size:
            for k in range(run_length + 1):
                table[g + k - 1][column] = 1
            g += step
        g_start = step + 1
    return table
