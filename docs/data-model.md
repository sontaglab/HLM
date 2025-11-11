# Data Model

Hybrid Logic Minimizer relies on a curated set of MATLAB `.mat` files bundled in
`matlab/`. The Python loader (`hybrid_logic.data.loader`) ingests these files and
normalises them into Python-native structures.

## Summary bundle (`summary.mat`)

Provides a holistic view of every Boolean function:

- `summary`: 65,536 rows describing canonical representatives, input counts, and
  permutation metadata.
- `dec_rep`: maps each decimal id to its canonical equivalent.
- `inputs_2/inputs_3/inputs_4`: lookup tables for locating graph templates by
  canonical id.
- `graphs_?inputs`: ASCII art templates for the NOR trees grouped by number of
  inputs.

The loader wraps the ASCII templates in a `GraphLibrary` that exposes
`GraphLibrary.get(nb_inputs)` and handles missing entries defensively.

## Minimisation tables

| File | Loader field | Description |
| --- | --- | --- |
| `PI.mat` | `prime_implicants` | Prime implicant strings for each canonical function |
| `nb_norgates.mat` | `nor_gate_counts` | Minimum NOR2 counts recorded during minimisation |
| `maxgates.mat` | `max_gates` | Precomputed worst-case gate usage by input count |
| `permuted_terms.mat` | `permuted_terms` | Flags used to rebuild the canonical permutation |
| `QS_nb.mat` | `qs_nb` | Quasi-symmetry classification for each decimal |

Each array is reshaped and typecast to a concrete `numpy` dtype (`uint16`,
`int16`, etc.) to ensure deterministic behaviour and compatibility with SciPy's
`loadmat` output.

## Derived structures

- `SummaryBundle`: encapsulates the summary arrays and the `GraphLibrary`.
- `MinimisationTables`: groups the minimisation artefacts, ready for use by
  `tree_builder` and the CLI scripts.
- `MatlabRepository`: the top-level container returned by `load_repository()`,
  combining both bundles.

## Versioning and reproducibility

The MATLAB data ships alongside the codebase, meaning HLM operates offline by
default. If you regenerate the MATLAB run, keep the same filenames so the loader
continues to function. Consider storing regenerated artefacts under version
control to guarantee reproducible research results.

For a walkthrough of how these datasets flow through the runtime, read
{doc}`how-it-works`.
