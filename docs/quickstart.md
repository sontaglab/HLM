# Quickstart

Reproduce the HLM pipeline in two short examples: build a NOR tree for a known
Boolean function and compile a small Verilog module into a structural NOR2
netlist.

```{important}
Ensure you are inside an activated virtual environment and set `PYTHONPATH=python`
when running the scripts directly. All snippets assume the repository root as the
working directory.
```

## 1. Rebuild a NOR tree from the MATLAB library

```python
from pathlib import Path

from hybrid_logic.core.config import build_config_for_decimal
from hybrid_logic.core.print_graphs import build_tree_from_repo
from hybrid_logic.data.loader import load_repository
from hybrid_logic.viz.tree_plot import draw_tree

import matplotlib.pyplot as plt

# Load the MATLAB artefacts into Python-native containers
repo = load_repository()

# Focus on decimal 42964 (a favourite in the legacy MATLAB demos)
cfg = build_config_for_decimal(repo, 42964)

# Reconstruct the NOR tree, including QS metadata and gate counts
tree = build_tree_from_repo(repo, cfg)
print(f"Gate count: {tree.gate_counts[cfg.index] if tree.gate_counts else 'n/a'}")
print(f"QS classification: {tree.nb_qs}")

# Render the tree with Matplotlib
fig, ax = plt.subplots(figsize=(8, 5), dpi=150)
draw_tree(tree, ax=ax)
ax.set_title(f"NOR realisation for decimal {cfg.decimal}")
fig.tight_layout()
fig.savefig(Path("docs/_static/images/nor_42964.png"))
plt.close(fig)
```

The resulting plot mirrors the MATLAB printout but benefits from a consistent
layout and colour palette. You can feature this saved image inside the docs or in
presentations.

## 2. Compile behavioural Verilog to NOR2 gates

```python
verilog_source = """
module or_gate (input a, input b, output y);
  assign y = a | b;
endmodule
"""

from hybrid_logic.verilog import parse_verilog_module, derive_truth_table, render_structural_verilog

module = parse_verilog_module(verilog_source)
truth = derive_truth_table(module)
cfg = build_config_for_decimal(repo, truth.decimal)
tree = build_tree_from_repo(repo, cfg)
netlist = render_structural_verilog(module, tree)
print(netlist)
```

Output (abridged):

```
module or_gate (input a, input b, output y);
  nor NOR_0 (y, a, b);
endmodule
```

Gate deduplication, fan-in checks, and canonical reuse happen automatically
inside the helper functions. For a full CLI experience, try the script in
{doc}`cli`.

## Where to go next

- {doc}`how-it-works` explains each module participating in the pipeline.
- {doc}`verilog-flow` dives into the supported Verilog subset and common parser
  failure modes.
- {doc}`examples/index` showcases additional plots and surfaces the original
  Jupyter notebook.
