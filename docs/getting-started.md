# Getting Started

Set up a local Python environment and confirm that the Hybrid Logic Minimizer
can import the MATLAB artefacts.

## Prerequisites

- Python 3.11 (minimum supported version)
- GNU Make (optional, for building C++ utilities in `code/`)
- Graphviz system binaries if you plan to run the original MATLAB workflows (not required for HLM)

## Install the Python environment

```bash
python3.11 -m venv .venv
source .venv/bin/activate
pip install -r python/requirements.txt
```

You will also need the documentation dependencies when building the docs locally:

```bash
pip install -r docs/requirements.txt
```

## Prepare the repository

All MATLAB artefacts are already vendored under `matlab/`, so no additional
downloads are required. If you want to regenerate the `.venv/` shipped with the
repo, simply remove it and recreate as shown above.

## Smoke test the toolchain

```bash
# Activate your virtual environment first
source .venv/bin/activate
PYTHONPATH=python python -m unittest discover -s python/tests -t .
```

You should see the Verilog round-trip and QS correlation tests pass. These tests
exercise the most important integration points: data loading, canonical lookup,
and structural netlist rendering.

## Next steps

1. Follow the {doc}`quickstart` to rebuild a NOR tree and compile a simple
   Verilog module.
2. Browse {doc}`how-it-works` for a deeper explanation of the data model and
   pipeline.
3. Open {doc}`examples/index` to explore plots and the accompanying Jupyter
   notebook.
