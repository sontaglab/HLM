# Examples & Notebook

Curated examples highlight how HLM reconstructs NOR trees and compiles Verilog
modules. Use them as templates for your own explorations.

## Featured NOR trees

```{figure} ../_static/images/nor_42964.png
:name: fig-nor-42964
:class: hero-diagram

NOR realisation for decimal 42964 (parity-inspired function). Inputs are shown in
green, NOR gates in red.
```

More candidates to explore:

```{list-table}
:header-rows: 1

* - Decimal
  - Notes
  - Suggested command
* - 232
  - Dense fan-out tree; useful when testing QS classification
  - `python -m hybrid_logic.scripts.print_nor_circuit 232 --output nor_232.png`
* - 4095
  - Exhibits extensive gate sharing; ideal for inspecting deduplication
  - `python -m hybrid_logic.scripts.print_nor_circuit 4095 --output nor_4095.png`
* - 65535
  - Trivial top-level node (all ones) but exercises loader edge cases
  - `python -m hybrid_logic.scripts.print_nor_circuit 65535 --output nor_65535.png`
```

## Jupyter notebook

The repository ships with `python/examples/verilog_to_nor.ipynb`, an interactive
demonstration of the end-to-end pipeline. It is rendered in the documentation via
{mod}`myst_nb` without execution to keep builds fast.

```{nb-exec-table}
:label: notebook-metadata

| Notebook | Description |
| --- | --- |
| [verilog_to_nor.ipynb](verilog_to_nor.ipynb) | Walk through parsing a behavioural module, mapping it to the canonical tree, and exporting a structural netlist + plot. |
```

Open it locally for a live experience:

```bash
PYTHONPATH=python jupyter notebook python/examples/verilog_to_nor.ipynb
```

For more CLI-oriented flows, jump back to {doc}`cli`.
