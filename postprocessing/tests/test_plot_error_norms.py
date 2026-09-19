# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import pathlib
import tempfile
import unittest

from postprocessing.check_error_norms import COLUMNS
from postprocessing.plot_error_norms import SERIES_STYLES, _load_csv

DATABASE = [
    "t, l1-error, l2-error, linf-error\n",
    "6.000000e-01, 6.0e-14, 5.0e-15, 1.3e-15\n",
    "3.000000e-01, 4.0e-14, 3.0e-15, 9.0e-16\n",
]


def write_database(directory, lines):
    path = pathlib.Path(directory) / "Error.csv"
    path.write_text("".join(lines), encoding="utf-8")
    return path


class PlotErrorNormsTest(unittest.TestCase):
    def test_every_plotted_series_names_a_column_the_database_holds(self):
        for column, _label, _colour in SERIES_STYLES:
            self.assertIn(column, COLUMNS)

    def test_the_measurements_come_back_ordered_by_time(self):
        with tempfile.TemporaryDirectory() as directory:
            data = _load_csv(write_database(directory, DATABASE))
        self.assertEqual(sorted(data), sorted(COLUMNS))
        self.assertAlmostEqual(data["t"][0], 0.3)
        self.assertAlmostEqual(data["t"][1], 0.6)
        self.assertAlmostEqual(data["linf-error"][0], 9.0e-16)
        self.assertAlmostEqual(data["linf-error"][1], 1.3e-15)

    def test_a_database_with_another_header_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            path = write_database(
                directory, ["t, l1-error, l2-error, l_inf-error\n", "0.1, 1, 1, 1\n"]
            )
            with self.assertRaises(SystemExit):
                _load_csv(path)

    def test_a_database_without_measurements_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            path = write_database(directory, [DATABASE[0]])
            with self.assertRaises(SystemExit):
                _load_csv(path)

    def test_a_row_with_a_missing_field_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            path = write_database(directory, [DATABASE[0], "0.1, 1, 1\n"])
            with self.assertRaises(SystemExit):
                _load_csv(path)

    def test_a_diverged_run_still_plots(self):
        with tempfile.TemporaryDirectory() as directory:
            path = write_database(directory, [DATABASE[0], "0.1, 1, 1, nan\n"])
            data = _load_csv(path)
        self.assertEqual(len(data["t"]), 1)


if __name__ == "__main__":
    unittest.main()
