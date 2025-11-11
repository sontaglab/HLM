"""Orchestrate baseline vs HLM-minimised Cello2 runs."""
from __future__ import annotations

import json
import subprocess
from pathlib import Path
from typing import Dict, Iterable, Tuple

try:
    # Prefer tqdm if available; fall back to no-op wrappers otherwise.
    from tqdm.auto import tqdm
except ImportError:  # pragma: no cover - tqdm may not be installed.
    class _DummyProgress:  # type: ignore
        """Minimal tqdm-compatible object for console-only feedback."""

        def __init__(self, iterable=None, desc: str | None = None) -> None:
            self._iterable = iterable
            self._desc = desc
            if desc:
                print(f"[progress] {desc}")

        def __iter__(self):
            if self._iterable is None:
                return iter(())
            for item in self._iterable:
                yield item

        def update(self, *_args, **_kwargs) -> None:
            pass

        def set_description(self, desc: str, *_args, **_kwargs) -> None:
            if desc != self._desc:
                self._desc = desc
                print(f"[progress] {desc}")

        def set_postfix(self, *_args, **_kwargs) -> None:
            pass

        def set_postfix_str(self, *_args, **_kwargs) -> None:
            pass

        def close(self) -> None:
            pass

    def tqdm(iterable=None, **kwargs):
        return _DummyProgress(iterable, desc=kwargs.get("desc"))


PYTHON_DIR = Path(__file__).resolve().parents[1]
CIRCUIT_DIR = PYTHON_DIR / "examples" / "circuits"
EXPERIMENT_DIR = PYTHON_DIR / "experiments"
BASELINE_DIR = EXPERIMENT_DIR / "baseline"
HLM_DIR = EXPERIMENT_DIR / "hlm"
RESOURCES_FILE = EXPERIMENT_DIR / "resource_paths.json"
OPTIONS_FILE = EXPERIMENT_DIR / "options.csv"


CIRCUITS: Dict[str, Path] = {
    "parity3": CIRCUIT_DIR / "parity3.v",
}


def load_resources() -> Dict[str, Path]:
    data = json.loads(RESOURCES_FILE.read_text())
    return {key: (PYTHON_DIR / Path(value)).resolve() for key, value in data.items()}


def run_hlm(source: Path, dest: Path) -> None:
    dest.parent.mkdir(parents=True, exist_ok=True)
    venv_python = PYTHON_DIR.parent / ".venv" / "bin" / "python"
    cmd = [
        str(venv_python),
        "-m",
        "hybrid_logic.scripts.hlm_compile_verilog",
        str(source),
        "-o",
        str(dest),
    ]
    print(f"  ↳ Compiling NOR netlist via HLM: {' '.join(cmd)}")
    subprocess.run(cmd, check=True, cwd=PYTHON_DIR)


def run_cello(input_netlist: Path, output_dir: Path, resources: Dict[str, Path]) -> int:
    output_dir.mkdir(parents=True, exist_ok=True)

    cmd = [
        "docker",
        "run",
        "--rm",
        "--platform",
        "linux/amd64",
        "-v",
        f"{PYTHON_DIR}:/workspace",
        "-w",
        "/workspace",
        "-t",
        "cidarlab/cello-dnacompiler:latest",
        "java",
        "-classpath",
        "/root/app.jar",
        "org.cellocad.v2.DNACompiler.runtime.Main",
        "-inputNetlist",
        f"/workspace/{input_netlist.relative_to(PYTHON_DIR)}",
        "-userConstraintsFile",
        f"/workspace/{resources['ucf'].relative_to(PYTHON_DIR)}",
        "-inputSensorFile",
        f"/workspace/{resources['input_sensor'].relative_to(PYTHON_DIR)}",
        "-outputDeviceFile",
        f"/workspace/{resources['output_device'].relative_to(PYTHON_DIR)}",
        "-pythonEnv",
        "python",
        "-options",
        f"/workspace/{OPTIONS_FILE.relative_to(PYTHON_DIR)}",
        "-outputDir",
        f"/workspace/{output_dir.relative_to(PYTHON_DIR)}",
    ]
    print(f"  ↳ Launching Cello2 container: {' '.join(cmd)}")
    result = subprocess.run(cmd, check=False, cwd=PYTHON_DIR)
    return result.returncode


def pipeline(circuits: Iterable[Tuple[str, Path]]) -> None:
    resources = load_resources()

    circuit_items = list(circuits)
    circuit_bar = tqdm(circuit_items, desc="Circuits", unit="circuit")
    for name, source in circuit_bar:
        circuit_bar.set_postfix_str(name)
        step_bar = tqdm(total=3, desc=f"{name} stages", unit="stage", leave=False)

        print(f"[pipeline] Running baseline for {name}")
        baseline_out = BASELINE_DIR / name
        rc = run_cello(source, baseline_out, resources)
        if rc != 0:
            print(f"Warning: baseline run for {name} exited with code {rc}")
        step_bar.update(1)
        step_bar.set_description(f"{name} stages — HLM compile")

        print(f"[pipeline] Running HLM compile for {name}")
        hlm_netlist = HLM_DIR / name / f"{name}_nor.v"
        run_hlm(source, hlm_netlist)
        step_bar.update(1)
        step_bar.set_description(f"{name} stages — HLM Cello run")

        print(f"[pipeline] Running HLM pipeline for {name}")
        rc = run_cello(hlm_netlist, HLM_DIR / name, resources)
        if rc != 0:
            print(f"Warning: HLM run for {name} exited with code {rc}")
        step_bar.update(1)
        step_bar.close()


def main() -> None:
    pipeline(CIRCUITS.items())


if __name__ == "__main__":
    main()
