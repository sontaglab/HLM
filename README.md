# Hybrid Logic Minimizer (HLM)

HLM is a modern, Python-first reimplementation of a legacy MATLAB workflow for
minimising and exploring NOR-only realisations of small Boolean functions
(2–4 inputs). It ships with the original MATLAB datasets and provides an
end‑to‑end Verilog→NOR2 flow, reproducible analyses, and publication‑quality
plots.

## Highlights

- Verilog to structural NOR2 netlist in one command
- Deterministic tree reconstruction and Matplotlib visualisation
- Canonical dataset bundled locally (no MATLAB runtime required)
- Scripts to reproduce the NOR2 gate histogram and QS (quasi‑symmetry) analysis

## Repository Layout

- `python/` — Main Python package and scripts (recommended entry point)
  - `hybrid_logic/` — data loaders, core algorithms, Verilog tools, plotting
  - `examples/` — example circuits and a Jupyter notebook
  - `tests/` — lightweight unit tests
- `matlab/` — MATLAB `.mat` artefacts used by the original workflow
- `docs/` — Sphinx documentation (configured for Read the Docs)
- `code/`, `minimumcircuits/`, `new_workflow/` — historical prototypes and
  legacy assets kept for reference

If you’re here to use HLM, start with the Python package. The legacy folders are
not required for normal usage.

## Quick Start (Python)

Create a fresh virtual environment and install the pinned requirements:

```bash
python3.11 -m venv .venv
source .venv/bin/activate
pip install -r python/requirements.txt
```

Run the tests to validate your environment:

```bash
PYTHONPATH=python python -m unittest discover -s python/tests -t .
```

## Common Tasks

- Render the NOR tree for a specific Boolean function (decimal index):

  ```bash
  PYTHONPATH=python python -m hybrid_logic.scripts.print_nor_circuit 42964 \
    --output nor_42964.png
  ```

- Compile behavioural Verilog to a structural NOR2 netlist (and optional plot):

  ```bash
  PYTHONPATH=python python -m hybrid_logic.scripts.hlm_compile_verilog \
    python/examples/circuits/parity3.v \
    --emit-plot parity3_tree.png
  ```

- Reproduce the histogram and cumulative distribution figure (optionally limit
  for speed):

  ```bash
  PYTHONPATH=python python -m hybrid_logic.scripts.test_all_nor_circuits \
    --limit 5000 \
    --output figure1.png
  ```

## How It Works (at a glance)

1. Load canonical MATLAB artefacts into Python (`hybrid_logic.data.loader`).
2. Map a function (by Verilog or decimal) to its canonical representative and
   input permutation (`hybrid_logic.core.config`).
3. Reconstruct the NOR tree from ASCII templates and compute gate/QS metadata
   (`hybrid_logic.core.print_graphs`, `hybrid_logic.core.tree_builder`).
4. Visualise the tree (`hybrid_logic.viz.tree_plot`) or emit a structural
   NOR2 netlist (`hybrid_logic.verilog.netlist_writer`).

## Documentation

- Local HTML docs: build with
  `sphinx-build -b html docs docs/_build/html` and open `docs/_build/html/index.html`.
- Read the Docs: the repo includes `.readthedocs.yaml` — if connected, the docs
  will be published automatically on push.

For a Python‑specific overview and additional examples, see `python/README.md`.

## Troubleshooting

- Ensure `PYTHONPATH=python` is set when invoking modules from the repo root.
- If RTD fails config validation, verify `.readthedocs.yaml` (only `htmlzip`,
  `pdf`, or `epub` are valid under `formats:` — or omit the key).
- Some doc builds mock heavy imports; install `docs/requirements.txt` locally
  for a complete experience.

