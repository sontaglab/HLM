"""Utilities for extracting Cello2 metrics from run logs."""
from __future__ import annotations

import argparse
import json
import re
from pathlib import Path
from typing import Dict, Optional

_GATE_PATTERN = re.compile(r"Number of gates\s*:\s*(\d+)")
_SCORE_PATTERN = re.compile(r"Score:\s*([-+0-9.eE]+)")


def parse_log(log_path: Path) -> Dict[str, Optional[float]]:
    """Return gate count and score parsed from a Cello2 log."""
    gates: Optional[int] = None
    score: Optional[float] = None

    for line in log_path.read_text().splitlines():
        if gates is None:
            match = _GATE_PATTERN.search(line)
            if match:
                gates = int(match.group(1))
                continue
        if score is None:
            match = _SCORE_PATTERN.search(line)
            if match:
                score = float(match.group(1))
                continue
        if gates is not None and score is not None:
            break

    return {"gates": gates, "score": score}


def main() -> None:
    parser = argparse.ArgumentParser(description="Extract gate count and score from Cello2 logs.")
    parser.add_argument("log", type=Path, nargs="+", help="Path(s) to log.log files.")
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        default=None,
        help="Optional output path for JSON summary. Defaults to stdout.",
    )
    args = parser.parse_args()

    results = {str(path): parse_log(path) for path in args.log}

    payload = json.dumps(results, indent=2, sort_keys=True)
    if args.output:
        args.output.write_text(payload)
    else:
        print(payload)


if __name__ == "__main__":
    main()
