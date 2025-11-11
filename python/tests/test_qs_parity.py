import unittest

import scipy.io

from hybrid_logic.data.loader import load_repository
from hybrid_logic.core.config import build_config_for_decimal
from hybrid_logic.core.print_graphs import build_tree_from_repo


class TestQSParity(unittest.TestCase):
    SAMPLE_DECIMALS = [3, 4, 9, 10, 12, 232, 42964]

    def test_nb_qs_matches_matlab_samples(self) -> None:
        repo = load_repository()
        qs_expected = scipy.io.loadmat('matlab/QS_nb.mat')['QS_nb'].flatten()
        for nb_decimal in self.SAMPLE_DECIMALS:
            with self.subTest(decimal=nb_decimal):
                cfg = build_config_for_decimal(repo, nb_decimal)
                tree = build_tree_from_repo(repo, cfg)
                self.assertEqual(int(qs_expected[nb_decimal]), tree.nb_qs)


if __name__ == '__main__':
    unittest.main()
