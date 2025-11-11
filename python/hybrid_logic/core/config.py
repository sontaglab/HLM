"""Convenience helpers to derive graph build configurations."""
from __future__ import annotations

import itertools
from dataclasses import dataclass
from typing import Dict, List

import numpy as np

from hybrid_logic.core.print_graphs import GraphBuildConfig
from hybrid_logic.data.loader import HybridDataError, MatlabRepository


_PERMUTATIONS: Dict[int, List[List[int]]] = {
    2: [list(p) for p in itertools.permutations([1, 2])],
    3: [list(p) for p in itertools.permutations([1, 2, 3])],
    4: [list(p) for p in itertools.permutations([1, 2, 3, 4])],
}


def build_config_for_decimal(repo: MatlabRepository, nb_decimal: int) -> GraphBuildConfig:
    summary = repo.summary.summary
    nb_inputs = int(summary[nb_decimal, 1])
    if nb_inputs <= 1:
        raise HybridDataError("Single-input functions do not have graph templates")

    canon = int(repo.summary.dec_rep[nb_decimal])
    canon_row = summary[canon]
    perm_index = int(canon_row[3]) - 1
    if perm_index < 0:
        perm_index = 0

    perm_table = _PERMUTATIONS[nb_inputs]
    if perm_index >= len(perm_table):
        raise HybridDataError(
            f"Permutation index {perm_index} out of bounds for {nb_inputs}-input table"
        )
    perm_row = perm_table[perm_index]

    permuted_terms = repo.tables.permuted_terms[canon]
    remaining_terms = [idx + 1 for idx, flag in enumerate(permuted_terms) if flag != 1]
    if len(remaining_terms) < nb_inputs:
        # fall back to first nb_inputs entries if metadata is sparse
        remaining_terms = list(range(1, nb_inputs + 1))
    ordered_terms = [remaining_terms[idx - 1] for idx in perm_row[:nb_inputs]]
    permutation = [value - 1 for value in ordered_terms]

    if nb_inputs == 4:
        lookup_value = int(canon_row[0])
    else:
        lookup_value = int(canon_row[2])

    inputs_array = repo.summary.inputs[nb_inputs]
    matches = np.where(inputs_array == lookup_value)[0]
    if matches.size == 0:
        raise HybridDataError(
            f"Unable to locate graph index for decimal {nb_decimal} (lookup {lookup_value})"
        )
    index = int(matches[0])

    return GraphBuildConfig(
        nb_inputs=nb_inputs,
        index=index,
        permutation=permutation,
        decimal=nb_decimal,
    )


__all__ = ["build_config_for_decimal"]
