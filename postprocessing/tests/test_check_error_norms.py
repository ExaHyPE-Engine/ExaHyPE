# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import math
import pathlib
import tempfile
import unittest

from postprocessing.check_error_norms import (
    DEFAULT_COLUMN,
    build_parser,
    read_error_norms,
    worst_measurement,
)

HEADER = ["t", "l1-error", "l2-error", "linf-error"]
ROWS = [
    ["0.000000e+00", "0.000000e+00", "0.000000e+00", "0.000000e+00"],
    ["3.000000e-01", "4.000000e-14", "3.000000e-15", "9.000000e-16"],
    ["6.000000e-01", "6.000000e-14", "5.000000e-15", "1.300000e-15"],
    ["9.000000e-01", "9.000000e-14", "7.000000e-15", "1.100000e-15"],
]


def write_database(directory, lines):
    path = pathlib.Path(directory) / "Error.csv"
    path.write_text("".join(lines), encoding="utf-8")
    return str(path)


class ReadErrorNormsTest(unittest.TestCase):
    def test_a_database_splits_into_its_header_and_its_measurements(self):
        with tempfile.TemporaryDirectory() as directory:
            path = write_database(
                directory,
                ["t, l1-error, l2-error, linf-error\n", "1.0, 2.0, 3.0, 4.0\n"],
            )
            header, rows = read_error_norms(path)
        self.assertEqual(header, HEADER)
        self.assertEqual(rows, [["1.0", "2.0", "3.0", "4.0"]])

    def test_a_database_without_measurements_yields_no_rows(self):
        with tempfile.TemporaryDirectory() as directory:
            path = write_database(directory, ["t, l1-error, l2-error, linf-error\n"])
            header, rows = read_error_norms(path)
        self.assertEqual(header, HEADER)
        self.assertEqual(rows, [])

    def test_an_empty_database_yields_neither_header_nor_rows(self):
        with tempfile.TemporaryDirectory() as directory:
            path = write_database(directory, [])
            header, rows = read_error_norms(path)
        self.assertEqual(header, [])
        self.assertEqual(rows, [])


class WorstMeasurementTest(unittest.TestCase):
    def test_the_worst_row_is_the_largest_one_and_not_the_last(self):
        time, worst = worst_measurement(HEADER, ROWS, "linf-error")
        self.assertAlmostEqual(time, 0.6)
        self.assertAlmostEqual(worst, 1.3e-15)

    def test_each_norm_is_selected_on_its_own(self):
        self.assertAlmostEqual(worst_measurement(HEADER, ROWS, "l1-error")[1], 9.0e-14)
        self.assertAlmostEqual(worst_measurement(HEADER, ROWS, "l2-error")[1], 7.0e-15)

    def test_a_column_outside_the_database_is_rejected(self):
        with self.assertRaises(ValueError):
            worst_measurement(HEADER, ROWS, "l_inf-error")

    def test_a_diverged_snapshot_is_the_worst_one(self):
        rows = ROWS + [["1.2", "0.0", "0.0", "nan"]]
        time, worst = worst_measurement(HEADER, rows, "linf-error")
        self.assertAlmostEqual(time, 1.2)
        self.assertTrue(math.isnan(worst))

    def test_a_short_row_is_rejected(self):
        with self.assertRaises(IndexError):
            worst_measurement(HEADER, [["1.2", "0.0", "0.0"]], "linf-error")


class ParserTest(unittest.TestCase):
    def test_the_maximum_norm_is_checked_unless_another_one_is_named(self):
        arguments = build_parser().parse_args(["Error.csv", "1e-10"])
        self.assertEqual(arguments.database, "Error.csv")
        self.assertAlmostEqual(arguments.threshold, 1e-10)
        self.assertEqual(arguments.column, DEFAULT_COLUMN)
        self.assertEqual(
            build_parser()
            .parse_args(["Error.csv", "1e-10", "--column", "l1-error"])
            .column,
            "l1-error",
        )

    def test_the_threshold_has_to_be_non_negative(self):
        for invalid_threshold in ("nan", "-1.0"):
            with self.subTest(threshold=invalid_threshold):
                with self.assertRaises(SystemExit):
                    build_parser().parse_args(["Error.csv", invalid_threshold])

    def test_an_infinite_threshold_reports_without_ever_failing(self):
        arguments = build_parser().parse_args(["Error.csv", "inf"])
        self.assertEqual(arguments.threshold, math.inf)


if __name__ == "__main__":
    unittest.main()
