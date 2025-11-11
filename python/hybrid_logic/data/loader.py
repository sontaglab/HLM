"""Utilities for loading and normalising the legacy MATLAB artefacts.

The loader centralises ingestion of the .mat files that back the NOR circuit
minimizer workflow.  It exposes a lightweight Python-native view over the
MATLAB cell arrays so the downstream modules can focus on algorithms and
visualisations rather than data wrangling quirks.
"""
from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Sequence

import numpy as np
from scipy.io import loadmat

MATLAB_DIR = Path("matlab")


class HybridDataError(RuntimeError):
    """Raised when a MATLAB artefact cannot be interpreted as expected."""


@dataclass(slots=True)
class GraphLibrary:
    """ASCII graph templates grouped by number of inputs."""

    by_inputs: Dict[int, List[List[str]]]

    def get(self, nb_inputs: int) -> List[List[str]]:
        try:
            return self.by_inputs[nb_inputs]
        except KeyError as exc:  # pragma: no cover - defensive guard
            raise HybridDataError(f"Unknown number of inputs: {nb_inputs}") from exc


@dataclass(slots=True)
class SummaryBundle:
    """Summary tables that describe each Boolean function."""

    summary: np.ndarray
    dec_rep: np.ndarray
    inputs: Dict[int, np.ndarray]
    graphs: GraphLibrary


@dataclass(slots=True)
class MinimisationTables:
    """Results from the MATLAB minimisation step."""

    prime_implicants: Sequence[List[str]]
    nor_gate_counts: Sequence[np.ndarray]
    max_gates: Dict[int, np.ndarray]
    permuted_terms: np.ndarray
    qs_nb: np.ndarray


@dataclass(slots=True)
class MatlabRepository:
    """Aggregated view of the MATLAB source data."""

    summary: SummaryBundle
    tables: MinimisationTables


def load_repository(matlab_dir: Path | None = None) -> MatlabRepository:
    """Load all MATLAB data artefacts into Python-native containers."""

    matlab_dir = matlab_dir or MATLAB_DIR

    summary_bundle = _load_summary_bundle(matlab_dir)
    tables = _load_tables(matlab_dir)

    return MatlabRepository(summary=summary_bundle, tables=tables)


# ---------------------------------------------------------------------------
# Private helpers


def _load_summary_bundle(matlab_dir: Path) -> SummaryBundle:
    data = loadmat(matlab_dir / "summary.mat", squeeze_me=False, struct_as_record=False)

    summary = np.asarray(data["summary"], dtype=np.uint16)
    dec_rep = np.asarray(data["dec_rep"], dtype=np.uint16).reshape(-1)

    inputs = {
        2: np.asarray(data["inputs_2"], dtype=np.uint16).reshape(-1),
        3: np.asarray(data["inputs_3"], dtype=np.uint16).reshape(-1),
        4: np.asarray(data["inputs_4"], dtype=np.uint16).reshape(-1),
    }

    graphs = GraphLibrary(
        by_inputs={
            2: _normalise_graph_table(data["graphs_2inputs"]),
            3: _normalise_graph_table(data["graphs_3inputs"]),
            4: _normalise_graph_table(data["graphs_4inputs"]),
        }
    )

    return SummaryBundle(summary=summary, dec_rep=dec_rep, inputs=inputs, graphs=graphs)


def _load_tables(matlab_dir: Path) -> MinimisationTables:
    pi_raw = loadmat(matlab_dir / "PI.mat", squeeze_me=False, struct_as_record=False)
    nb_raw = loadmat(matlab_dir / "nb_norgates.mat", squeeze_me=False, struct_as_record=False)
    max_raw = loadmat(matlab_dir / "maxgates.mat", squeeze_me=False, struct_as_record=False)
    perm_raw = loadmat(matlab_dir / "permuted_terms.mat", squeeze_me=False, struct_as_record=False)
    qs_raw = loadmat(matlab_dir / "QS_nb.mat", squeeze_me=False, struct_as_record=False)

    prime_implicants = tuple(_normalise_string_vector(cell) for cell in pi_raw["PI"].reshape(-1))
    nor_gate_counts = tuple(_normalise_numeric_vector(cell) for cell in nb_raw["nb_norgates"].reshape(-1))

    max_gates = {
        2: np.asarray(max_raw["maxgates_2"], dtype=np.uint16).reshape(-1),
        3: np.asarray(max_raw["maxgates_3"], dtype=np.uint16).reshape(-1),
        4: np.asarray(max_raw["maxgates_4"], dtype=np.uint16).reshape(-1),
    }

    permuted_terms = np.asarray(perm_raw["permuted_terms"], dtype=np.uint8)
    qs_nb = np.asarray(qs_raw["QS_nb"], dtype=np.int8).reshape(-1)

    return MinimisationTables(
        prime_implicants=prime_implicants,
        nor_gate_counts=nor_gate_counts,
        max_gates=max_gates,
        permuted_terms=permuted_terms,
        qs_nb=qs_nb,
    )


def _normalise_graph_table(raw: np.ndarray) -> List[List[str]]:
    """Convert a MATLAB cell array of ASCII art rows into Python lists."""

    table: List[List[str]] = []
    for row in raw:
        line_set = []
        for cell in row:
            text = _coerce_matlab_cell(cell)
            if text:
                line_set.append(text)
        table.append(line_set)
    return table


def _normalise_string_vector(cell: np.ndarray) -> List[str]:
    if cell.size == 0:
        return []
    if cell.dtype.kind in {"U", "S"}:
        return [str(item).strip() for item in cell.tolist()]
    # Some cells double-nest arrays; flatten recursively.
    flat: List[str] = []
    for item in cell.reshape(-1):
        flat.extend(_normalise_string_vector(np.asarray(item)))
    return flat


def _normalise_numeric_vector(cell: np.ndarray) -> np.ndarray:
    if cell.size == 0:
        return np.zeros(0, dtype=np.int16)
    arr = np.asarray(cell, dtype=np.int16).reshape(-1)
    return arr


def _coerce_matlab_cell(cell: np.ndarray) -> str:
    if cell.size == 0:
        return ""
    if cell.dtype.kind in {"U", "S"}:
        raw = cell.tolist()[0]
        return raw if isinstance(raw, str) else str(raw)
    # Fallback: convert scalars to string
    if cell.shape == ():
        return str(cell.item())
    return "".join(map(str, cell.reshape(-1)))


__all__ = [
    "GraphLibrary",
    "HybridDataError",
    "MatlabRepository",
    "MinimisationTables",
    "SummaryBundle",
    "load_repository",
]
