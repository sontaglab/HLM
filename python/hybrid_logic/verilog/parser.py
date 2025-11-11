"""Parse a restricted subset of behavioural Verilog into an AST."""
from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, Iterator, List, Optional, Sequence

__all__ = [
    "ParsedModule",
    "VerilogParseError",
    "Expr",
    "parse_verilog_module",
    "parse_verilog_file",
]

_RESERVED = {"input", "output", "wire", "reg", "logic", "signed"}
_BINARY_PRECEDENCE = {"|": 1, "^": 2, "xnor": 2, "&": 3}


class VerilogParseError(ValueError):
    """Raised when the module cannot be parsed under the supported subset."""


@dataclass(slots=True)
class Expr:
    """Base class for expression nodes."""

    def evaluate(self, context: Dict[str, bool]) -> bool:  # pragma: no cover - abstract
        raise NotImplementedError

    def identifiers(self) -> Sequence[str]:  # pragma: no cover - abstract
        raise NotImplementedError


@dataclass(slots=True)
class Literal(Expr):
    value: bool

    def evaluate(self, context: Dict[str, bool]) -> bool:
        return self.value

    def identifiers(self) -> Sequence[str]:
        return ()


@dataclass(slots=True)
class Variable(Expr):
    name: str

    def evaluate(self, context: Dict[str, bool]) -> bool:
        try:
            return bool(context[self.name])
        except KeyError as exc:  # pragma: no cover - defensive guard
            raise VerilogParseError(f"Missing signal value for '{self.name}'") from exc

    def identifiers(self) -> Sequence[str]:
        return (self.name,)


@dataclass(slots=True)
class UnaryOp(Expr):
    operand: Expr

    def evaluate(self, context: Dict[str, bool]) -> bool:
        return not self.operand.evaluate(context)

    def identifiers(self) -> Sequence[str]:
        return self.operand.identifiers()


@dataclass(slots=True)
class BinaryOp(Expr):
    op: str
    left: Expr
    right: Expr

    def evaluate(self, context: Dict[str, bool]) -> bool:
        left = self.left.evaluate(context)
        right = self.right.evaluate(context)
        if self.op == "&":
            return left and right
        if self.op == "|":
            return left or right
        if self.op == "^":
            return left ^ right
        if self.op == "xnor":
            return not (left ^ right)
        raise VerilogParseError(f"Unsupported binary operator '{self.op}'")

    def identifiers(self) -> Sequence[str]:
        return (*self.left.identifiers(), *self.right.identifiers())


@dataclass(slots=True)
class ParsedModule:
    name: str
    inputs: List[str]
    output: str
    expression: Expr
    ports: List[str]

    def evaluate(self, values: Dict[str, bool]) -> bool:
        return self.expression.evaluate(values)


class _TokenStream:
    def __init__(self, tokens: Sequence[str]):
        self._tokens = list(tokens)
        self._index = 0

    def peek(self) -> Optional[str]:
        if self._index >= len(self._tokens):
            return None
        return self._tokens[self._index]

    def advance(self) -> Optional[str]:
        token = self.peek()
        if token is not None:
            self._index += 1
        return token


def parse_verilog_file(path: Path | str) -> ParsedModule:
    source = Path(path).read_text(encoding="utf-8")
    return parse_verilog_module(source, source_name=str(path))


def parse_verilog_module(source: str, *, source_name: str | None = None) -> ParsedModule:
    cleaned = _strip_comments(source)
    match = re.search(r"\bmodule\b\s+([A-Za-z_][\w]*)\s*\((.*?)\)\s*;", cleaned, flags=re.DOTALL)
    if not match:
        raise VerilogParseError("Unable to locate module header")

    name = match.group(1)
    ports_raw = match.group(2)
    body_start = match.end()
    end_match = re.search(r"\bendmodule\b", cleaned[body_start:], flags=re.DOTALL)
    if not end_match:
        raise VerilogParseError("Missing 'endmodule' terminator")

    body = cleaned[body_start : body_start + end_match.start()]

    port_entries = _parse_port_entries(ports_raw)
    dir_overrides = _parse_port_directions(body)

    inputs: List[str] = []
    output: Optional[str] = None
    ordered_ports: List[str] = []

    for signal, direction in port_entries:
        if direction is None:
            direction = dir_overrides.get(signal)
        if direction is None:
            raise VerilogParseError(f"Unable to determine direction for port '{signal}'")
        if direction == "input":
            inputs.append(signal)
        elif direction == "output":
            if output is not None:
                raise VerilogParseError("Only single-output modules are supported")
            output = signal
        else:  # pragma: no cover - defensive guard
            raise VerilogParseError(f"Unsupported port direction '{direction}'")
        ordered_ports.append(signal)

    if output is None:
        raise VerilogParseError("Module must declare exactly one output port")

    assign_match = _locate_assign(body, output)
    expression = _parse_expression(assign_match)

    declared = set(inputs) | {output}
    undeclared = {identifier for identifier in expression.identifiers() if identifier not in declared}
    if undeclared:
        raise VerilogParseError(
            "Expression references undeclared signal(s): " + ", ".join(sorted(undeclared))
        )

    return ParsedModule(
        name=name,
        inputs=inputs,
        output=output,
        expression=expression,
        ports=ordered_ports,
    )


def _strip_comments(source: str) -> str:
    no_block = re.sub(r"/\*.*?\*/", "", source, flags=re.DOTALL)
    return re.sub(r"//.*?$", "", no_block, flags=re.MULTILINE)


def _parse_port_entries(ports_raw: str) -> List[tuple[str, Optional[str]]]:
    entries: List[tuple[str, Optional[str]]] = []
    for raw_segment in _split_comma_separated(ports_raw):
        tokens = re.findall(r"[A-Za-z_][A-Za-z0-9_]*", raw_segment)
        if not tokens:
            continue
        direction: Optional[str] = None
        signal: Optional[str] = None
        for token in tokens:
            if token in {"input", "output"}:
                direction = token
            elif token not in _RESERVED:
                signal = token
        if signal is None:
            raise VerilogParseError(f"Unable to parse port declaration segment '{raw_segment.strip()}'")
        entries.append((signal, direction))
    return entries


def _parse_port_directions(body: str) -> Dict[str, str]:
    direction_map: Dict[str, str] = {}
    for match in re.finditer(r"\b(input|output)\b\s+([^;]+);", body):
        direction = match.group(1)
        identifiers = re.findall(r"[A-Za-z_][A-Za-z0-9_]*", match.group(2))
        for identifier in identifiers:
            if identifier in _RESERVED:
                continue
            direction_map[identifier] = direction
    return direction_map


def _split_comma_separated(text: str) -> Iterable[str]:
    depth = 0
    start = 0
    for idx, ch in enumerate(text):
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth = max(depth - 1, 0)
        elif ch == "," and depth == 0:
            yield text[start:idx]
            start = idx + 1
    yield text[start:]


def _locate_assign(body: str, output: str) -> str:
    pattern = re.compile(r"\bassign\b\s+([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(.*?);", flags=re.DOTALL)
    matches = [m for m in pattern.finditer(body)]
    if not matches:
        raise VerilogParseError("Expected an 'assign' statement")

    relevant = [m for m in matches if m.group(1) == output]
    if not relevant:
        raise VerilogParseError(f"No assignment found for output '{output}'")
    if len(relevant) > 1:
        raise VerilogParseError(f"Multiple assignments found for output '{output}'")

    expr = relevant[0].group(2).strip()
    expr = re.sub(r"1'b0", "0", expr)
    expr = re.sub(r"1'b1", "1", expr)
    expr = re.sub(r"1'h0", "0", expr)
    expr = re.sub(r"1'h1", "1", expr)
    return expr


def _parse_expression(expr: str) -> Expr:
    tokens = list(_tokenize_expression(expr))
    stream = _TokenStream(tokens)
    node = _parse_subexpression(stream, 0)
    if stream.peek() is not None:
        raise VerilogParseError(f"Unexpected token '{stream.peek()}' in expression")
    return node


def _tokenize_expression(expr: str) -> Iterator[str]:
    idx = 0
    length = len(expr)
    while idx < length:
        ch = expr[idx]
        if ch.isspace():
            idx += 1
            continue
        if expr.startswith("^~", idx) or expr.startswith("~^", idx):
            yield "xnor"
            idx += 2
            continue
        if ch in "~&|^()":
            yield ch
            idx += 1
            continue
        if ch in "01":
            yield ch
            idx += 1
            continue
        match = re.match(r"[A-Za-z_][A-Za-z0-9_]*", expr[idx:])
        if match:
            token = match.group(0)
            yield token
            idx += len(token)
            continue
        raise VerilogParseError(f"Invalid character '{ch}' in expression")


def _parse_subexpression(stream: _TokenStream, min_prec: int) -> Expr:
    token = stream.advance()
    if token is None:
        raise VerilogParseError("Unexpected end of expression")

    if token == "~":
        node = UnaryOp(_parse_subexpression(stream, 4))
    elif token == "(":
        node = _parse_subexpression(stream, 0)
        if stream.advance() != ")":
            raise VerilogParseError("Missing ')' in expression")
    elif token == "0":
        node = Literal(False)
    elif token == "1":
        node = Literal(True)
    elif re.match(r"[A-Za-z_][A-Za-z0-9_]*", token):
        node = Variable(token)
    else:
        raise VerilogParseError(f"Unexpected token '{token}' in expression")

    while True:
        lookahead = stream.peek()
        if lookahead is None or lookahead == ")":
            break
        if lookahead == "~":
            break
        prec = _BINARY_PRECEDENCE.get(lookahead)
        if prec is None:
            raise VerilogParseError(f"Unsupported token '{lookahead}' in expression")
        if prec < min_prec:
            break
        stream.advance()
        rhs = _parse_subexpression(stream, prec + 1)
        node = BinaryOp(lookahead, node, rhs)
    return node
