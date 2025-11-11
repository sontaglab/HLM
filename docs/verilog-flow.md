# Verilog Flow

Hybrid Logic Minimizer accepts a compact subset of behavioural Verilog and maps
it to the canonical NOR-only library. This page outlines the supported syntax,
the truth-table mapping strategy, and common failure modes.

## Supported subset

- Exactly one output port per module.
- Input ports defined inline or via `input` declarations (no buses or ranges).
- Operators: `~`, `&`, `|`, `^`, `^~`/`~^` (XNOR), parentheses.
- Constant literals: `0`, `1`, `1'b0/1`, `1'h0/1`.
- No procedural blocks, assignments outside of a single `assign` to the output,
or multi-bit expressions.

```verilog
module example (input a, input b, output y);
  assign y = (a ^ b) | ~a;
endmodule
```

## Truth-table derivation

`hybrid_logic.verilog.truth_table` evaluates the parsed module over every vector
of four boolean inputs, regardless of the module's declared arity. Inputs beyond
the module's ports are zero-padded to align with the MATLAB dataset.

Key points:

- Modules with more than four inputs raise `TruthTableError`.
- The resulting bitstring is interpreted as a 16-bit integer — the decimal index
  used throughout the canonical lookup tables.

## Structural netlist rendering

`hybrid_logic.verilog.netlist_writer` converts the reconstructed NOR tree into a
structural Verilog module:

1. Identify a representative node for each unique gate name to maximise
   sharing.
2. Recursively instantiate NOR2 primitives, introducing helper wires for
   intermediate nodes.
3. Preserve the original port ordering and naming from the behavioural module.

Limitations and safeguards:

```{tip}
Gate nodes with more than two distinct children trigger `NetlistRenderError`.
The legacy dataset only contains NOR2 trees, so any higher fan-in indicates a
corrupted description.
```

- Tree terminals reference input positions (`x_0`, `x_1`, ...). Out-of-range
  references cause an immediate error.
- Multiple assignments to the behavioural output or undeclared signals result in
  `VerilogParseError`.

## Debugging failures

| Symptom | Likely cause | Fix |
| --- | --- | --- |
| `Unable to locate matching NOR realisation` | Function not covered by canonical tables (e.g. >4 inputs) | Reduce input count or extend dataset |
| `Gate node 'g_X' has 3 fan-in` | ASCII template corruption | Regenerate MATLAB artefacts |
| `Expression references undeclared signal` | Missing port declaration in source | Declare the signal as an input |
| `Modules with more than four inputs are not supported` | Truth-table stage rejects arity | Decompose design into ≤4-input blocks |

For more hands-on examples, walk through the {doc}`quickstart` or inspect the
Jupyter notebook in {doc}`examples/index`.
