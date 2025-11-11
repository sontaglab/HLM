# Hybrid Logic Minimizer (Python)

Hybrid Logic Minimizer (HLM) is a Python port of the original MATLAB workflow for
exploring NOR-only implementations of small Boolean functions. It consumes the
legacy `.mat` artefacts, rebuilds the canonical NOR trees, and exposes
visualisation plus Verilog compilation utilities.

```{note}
If you're new to HLM, head straight to the quickstart — it walks through loading
the dataset, rebuilding a tree, and compiling a Verilog module to NOR2 gates.
```

```{raw} html
<div class="landing-grid">
  <a class="landing-card" href="getting-started.html">
    <h3>Get Started</h3>
    <p>Install the Python toolchain, prepare the dataset, and validate your setup with the built-in unit tests.</p>
  </a>
  <a class="landing-card" href="quickstart.html">
    <h3>Quickstart</h3>
    <p>Run the end-to-end flow: load MATLAB data, rebuild a NOR tree, and compile a behavioural module to NOR2 gates.</p>
  </a>
  <a class="landing-card" href="how-it-works.html">
    <h3>How It Works</h3>
    <p>Follow the pipeline from Verilog parsing through canonical lookup, tree reconstruction, and QS analysis.</p>
  </a>
  <a class="landing-card" href="api/index.html">
    <h3>API Reference</h3>
    <p>Browse the Python modules that power HLM: data loading, reconstruction helpers, Verilog translation, and plotting.</p>
  </a>
  <a class="landing-card" href="cli.html">
    <h3>CLI Recipes</h3>
    <p>Automate histogram generation and per-function renders with the packaged scripts.</p>
  </a>
  <a class="landing-card" href="examples/index.html">
    <h3>Examples &amp; Notebook</h3>
    <p>Explore curated NOR trees and open the original notebook used during the Python port.</p>
  </a>
  <a class="landing-card" href="faq.html">
    <h3>FAQ</h3>
    <p>Quick answers to common parser errors, tree mismatches, and netlist conversion issues.</p>
  </a>
</div>
```

## Inside the Pipeline

```{mermaid}
flowchart LR
  A[Behavioural Verilog] --> B[Parser]\n  B --> C[Truth-table derivation]\n  C --> D[Canonical lookup]\n  D --> E[ASCII graph + permutation]\n  E --> F[Tree reconstruction]\n  F --> G[Gate / QS analysis]\n  F --> H[Visualisation (Matplotlib)]\n  F --> I[Netlist writer]\n  H --> J[PNG export]\n  I --> K[Structural NOR2 Verilog]
```

HLM mirrors the original MATLAB tooling while leaning on modern Python libraries
and a fully reproducible dataset baked into the repository. Dive into
{doc}`how-it-works` for a narrated breakdown, or jump straight to
{doc}`quickstart` to see the pipeline in action.

```{toctree}
:hidden:
:maxdepth: 2

getting-started
quickstart
how-it-works
verilog-flow
data-model
cli
examples/index
examples/verilog_to_nor
api/index
faq
```
