# CLI Recipes

The `hybrid_logic.scripts` package bundles command-line entry points for common
HLM tasks. All commands expect the repository root as the working directory and a
configured virtual environment.

```{tip}
Prefix each invocation with `PYTHONPATH=python` so Python can locate the package
without installation.
```

## Histogram of NOR2 usage

```bash
PYTHONPATH=python python -m hybrid_logic.scripts.test_all_nor_circuits \
  --limit 5000 \
  --output figure1.png
```

- Iterates through Boolean functions (2–4 inputs) and records QS metrics.
- Generates the histogram + cumulative distribution figure found in the original
  MATLAB workflow.
- Use `--limit` during smoke testing to shorten the run.

## Render a specific NOR tree

```bash
PYTHONPATH=python python -m hybrid_logic.scripts.print_nor_circuit 42964 \
  --output nor_42964.png
```

- Reconstructs the canonical NOR tree for decimal 42964.
- Saves a Matplotlib visualisation if `--output` is supplied; otherwise opens a
  window.

## Compile Verilog to NOR2 netlist

```bash
PYTHONPATH=python python -m hybrid_logic.scripts.hlm_compile_verilog \
  python/examples/circuits/parity3.v \
  --emit-plot parity3_tree.png
```

- Parses the behavioural Verilog, derives the truth table, and looks up the
  canonical tree.
- Writes a structural NOR2 netlist alongside the source.
- Optionally exports a PNG of the tree for documentation.

## Troubleshooting commands

| Error | Likely fix |
| --- | --- |
| `Failed to parse Verilog` | Check for unsupported constructs (multiple outputs, procedural code). |
| `Unable to locate matching NOR realisation` | Ensure the function uses ≤4 inputs or add coverage to the dataset. |
| `Gate node 'g_X' has 3 fan-in; only NOR2 is supported` | Inspect the ASCII template; it may be corrupted. |
| `Tree references input index N` | The canonical permutation expects more inputs than the module declares. |

For additional detail on parser constraints and canonical lookup rules, see
{doc}`verilog-flow`.
