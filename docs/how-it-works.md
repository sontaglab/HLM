# How It Works

The Python port mirrors the original MATLAB Hybrid Logic Minimizer but trades
MATLAB scripts for typed Python modules. Each stage transforms the same dataset
stored under `matlab/` into usable insights for NOR-only logic design.

## 1. Data ingestion (`hybrid_logic.data.loader`)

- Loads `.mat` artefacts such as `summary.mat`, `PI.mat`, `nb_norgates.mat`, and
  `permuted_terms.mat`.
- Normalises MATLAB cell arrays into Python primitives (lists, `numpy.ndarray`).
- Exposes `MatlabRepository`, a cohesive container passed to downstream modules.

```python
from hybrid_logic.data.loader import load_repository
repo = load_repository()
print(len(repo.summary.summary))  # -> 65536 functions
```

## 2. Canonical lookup (`hybrid_logic.core.config`)

- Consumers request a Boolean function by decimal index.
- The module derives the canonical graph index, input permutation, and metadata
  required to build the NOR tree.
- Supports 2-, 3-, and 4-input functions; single-input rows short-circuit to
  trivial results.

```python
from hybrid_logic.core.config import build_config_for_decimal
cfg = build_config_for_decimal(repo, 42964)
print(cfg)
```

## 3. Tree reconstruction (`hybrid_logic.core.print_graphs`, `hybrid_logic.core.tree_builder`)

- Fetches ASCII art templates, applies the canonical permutation, and rebuilds a
  structured tree.
- Tracks parent relationships, node metadata, and the number of NOR gates.
- Computes the QS (quasi-symmetry) metric to stay true to the MATLAB analysis.

```python
from hybrid_logic.core.print_graphs import build_tree_from_repo

tree = build_tree_from_repo(repo, cfg)
print(len(tree.nodes), tree.nb_qs)
```

## 4. Verilog transformation (`hybrid_logic.verilog.*`)

- `parser.py` consumes a strict, single-output subset of behavioural Verilog and
  produces an AST.
- `truth_table.py` evaluates the AST over all 4-input input vectors to recover
  the canonical decimal index.
- `netlist_writer.py` converts the NOR tree back into structural Verilog, taking
  care to deduplicate repeated subtrees and enforce NOR2 fan-in.

```python
from hybrid_logic.verilog import parse_verilog_module, derive_truth_table, render_structural_verilog

module = parse_verilog_module("""
module example (input a, input b, output y);
  assign y = ~(a & b);
endmodule
""")
truth = derive_truth_table(module)
netlist = render_structural_verilog(module, tree)
print(truth.decimal)
print(netlist.splitlines()[:3])
```

## 5. Visualisation (`hybrid_logic.viz.tree_plot`)

- Provides a deterministic layout for any reconstructed tree.
- Highlights fan-in relationships with stylised edges and colours to separate
  terminals from NOR gates.
- Supports optional highlighting of paths or subtrees for documentation.

```python
from hybrid_logic.viz.tree_plot import draw_tree
import matplotlib.pyplot as plt

fig, ax = plt.subplots(figsize=(8, 5), dpi=150)
draw_tree(tree, ax=ax)
fig.savefig("nor_tree.png")
```

## 6. Scripts and automation

The `hybrid_logic.scripts` package bundles higher-level entry points that stitch
these modules together:

- `test_all_nor_circuits`: generates the canonical NOR gate histogram and
  cumulative plot for all Boolean functions.
- `print_nor_circuit`: renders the NOR tree for any decimal id.
- `hlm_compile_verilog`: runs the entire behavioural-Verilog-to-NOR flow end to
  end, including optional plotting.

Refer to {doc}`cli` for usage examples and recommended invocation patterns.
