import itertools
import unittest

from hybrid_logic.core.config import build_config_for_decimal
from hybrid_logic.core.print_graphs import build_tree_from_repo
from hybrid_logic.data.loader import load_repository
from hybrid_logic.verilog import derive_truth_table, parse_verilog_module, render_structural_verilog


class TestVerilogToNor(unittest.TestCase):
    def setUp(self) -> None:
        self.repo = load_repository()

    def test_or_gate_roundtrip(self) -> None:
        source = """
        module or_gate (input a, input b, output y);
          assign y = a | b;
        endmodule
        """
        self._assert_roundtrip(source)

    def test_three_input_expression(self) -> None:
        source = """
        module mixed (input a, input b, input c, output y);
          assign y = (a ^ b) | ~c;
        endmodule
        """
        self._assert_roundtrip(source)

    def _assert_roundtrip(self, source: str) -> None:
        module = parse_verilog_module(source)
        truth = derive_truth_table(module)

        cfg = build_config_for_decimal(self.repo, truth.decimal)
        tree = build_tree_from_repo(self.repo, cfg)

        netlist = render_structural_verilog(module, tree)
        self.assertIn("module", netlist)
        self.assertIn("endmodule", netlist)

        for vector in itertools.product([False, True], repeat=len(module.inputs)):
            assignment = {name: value for name, value in zip(module.inputs, vector)}
            expected = module.evaluate(assignment)
            actual = _evaluate_tree(tree, assignment, module.inputs)
            self.assertEqual(actual, expected)


def _evaluate_tree(tree, assignment, inputs):
    children = [[] for _ in tree.nodes]
    for idx, parent in enumerate(tree.parents):
        parent_idx = parent - 1
        if parent_idx >= 0:
            children[parent_idx].append(idx)

    cache = {}
    name_cache = {}
    representatives = {}
    for idx, node in enumerate(tree.nodes):
        if node.is_terminal:
            continue
        if children[idx] and node.name not in representatives:
            representatives[node.name] = idx

    def visit(idx: int) -> bool:
        if idx in cache:
            return cache[idx]

        node = tree.nodes[idx]
        if not node.is_terminal:
            canonical_idx = representatives.get(node.name)
            if canonical_idx is not None and canonical_idx != idx:
                value = visit(canonical_idx)
                cache[idx] = value
                name_cache[node.name] = value
                return value
            if node.name in name_cache:
                value = name_cache[node.name]
                cache[idx] = value
                return value
        if node.is_terminal:
            suffix = int(node.name.split("_")[1])
            value = assignment[inputs[suffix]]
        else:
            kid_indices = children[idx]
            values = [visit(child_idx) for child_idx in kid_indices]
            if len(values) == 1:
                value = not values[0]
            else:
                if len(values) != 2:
                    raise AssertionError("Unexpected fan-in in NOR tree")
                value = not (values[0] or values[1])
            name_cache[node.name] = value
        cache[idx] = value
        return value

    root_idx = next((i for i, p in enumerate(tree.parents) if p == 0), None)
    if root_idx is None:
        raise AssertionError("Tree lacks a root")
    return visit(root_idx)


if __name__ == "__main__":
    unittest.main()
