# Hybrid Logic Minimizer (Python Port)

This package provides a Python implementation of the legacy MATLAB tooling for
analysing NOR-only logic realisations.  The modules consume the MATLAB `.mat`
artifacts in `matlab/` and expose high-level scripts for running the canonical
analyses or visualising individual circuits.

## Environment

Create the local virtual environment and install the pinned dependencies:

```bash
python3.11 -m venv .venv
source .venv/bin/activate
pip install -r python/requirements.txt
```

> The repository already contains a ready-to-use `.venv/` created during the
> port; feel free to remove it and recreate the environment from scratch.

## Key Modules

- `hybrid_logic.data.loader` — loads all MATLAB datasets and normalises them
  into Python-friendly containers.
- `hybrid_logic.core.tree_builder` — rebuilds the NOR gate tree from the ASCII
  diagrams and computes gate counts.
- `hybrid_logic.viz.tree_plot` — renders the reconstructed trees with
  Matplotlib.
- `hybrid_logic.core.config` — derives the canonical graph index and input
  permutations for a given Boolean function.
- `hybrid_logic.verilog` — parses behavioural Verilog, derives the truth table,
  and emits structural NOR netlists using the canonical library.
- `hybrid_logic.scripts.test_all_nor_circuits` — Python port of
  `test_allNORcircuits.m` that produces the aggregate histogram and cumulative
  distribution plot.
- `hybrid_logic.scripts.print_nor_circuit` — renders the NOR tree for an
  individual Boolean function.
- `hybrid_logic.scripts.hlm_compile_verilog` — end-to-end workflow that maps a
  behavioural Verilog module to the minimised NOR-only implementation.

## Examples

Generate the aggregate histogram (optionally limiting the iteration count for
quick tests):

```bash
PYTHONPATH=python ./.venv/bin/python -m hybrid_logic.scripts.test_all_nor_circuits \
    --limit 5000 \
    --output figure1.png
```

Visualise a specific Boolean function:

```bash
PYTHONPATH=python ./.venv/bin/python -m hybrid_logic.scripts.print_nor_circuit 42964 \
    --output nor_42964.png
```

Compile small behavioural Verilog modules directly into structural NOR2
netlists (optional plot saved alongside the netlist):

```bash
PYTHONPATH=python ./.venv/bin/python -m hybrid_logic.scripts.hlm_compile_verilog \
    path/to/module.v \
    --emit-plot path/to/module_tree.png
```

Interactively explore the same flow inside Jupyter:

```bash
PYTHONPATH=python jupyter notebook python/examples/verilog_to_nor.ipynb
```

The scripts rely solely on the `.mat` files that ship with this repository, so
no MATLAB runtime is required.

## Tests

Run the lightweight parity checks against representative MATLAB outputs:

```bash
PYTHONPATH=python ./.venv/bin/python -m unittest discover -s python/tests -t .
```
