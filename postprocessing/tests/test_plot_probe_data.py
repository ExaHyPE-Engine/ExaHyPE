# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import argparse
from pathlib import Path
import tempfile
import unittest

import matplotlib.pyplot as plt
import pandas as pd

from postprocessing import plot_probe_data
from postprocessing.plot_probe_data import (
    BASE_COLUMNS_2D,
    BASE_COLUMNS_3D,
    _build_argument_parser,
    _create_plot,
    _format_probe_label,
    _load_csv,
    _normalise_header,
    _read_header,
    _resolve_output_path,
    _resolve_title,
    _select_data_columns,
    _select_probe,
    _validate_base_columns,
    _write_outputs,
    main,
)

HEADER_2D = "number(0),number(1),t,x(0),x(1),data(0)\n"
HEADER_3D = "number(0),number(1),t,x(0),x(1),x(2),data(0)\n"
TWO_PROBES = (
    "0,0,0.0,0.1,0.2,1.0\n"
    "0,0,1.0,0.1,0.2,2.0\n"
    "1,0,0.0,0.3,0.4,3.0\n"
    "1,0,1.0,0.3,0.4,4.0\n"
)


def write_csv(directory, text, name="probe.csv"):
    path = Path(directory) / name
    path.write_text(text, encoding="utf-8")
    return path


class PlotProbeDataTest(unittest.TestCase):
    def setUp(self):
        self._directory = tempfile.TemporaryDirectory()
        self.directory = Path(self._directory.name)

    def tearDown(self):
        self._directory.cleanup()

    def test_normalise_header_strips_and_drops_missing_names(self):
        self.assertEqual(_normalise_header([" t ", "data(0)", None]), ["t", "data(0)"])

    def test_read_header_returns_the_first_row(self):
        path = write_csv(self.directory, HEADER_2D + TWO_PROBES)
        self.assertEqual(_read_header(path)[0], "number(0)")

    def test_read_header_rejects_an_empty_file(self):
        with self.assertRaises(SystemExit):
            _read_header(write_csv(self.directory, ""))

    def test_validate_base_columns_recognises_two_dimensions(self):
        base, data = _validate_base_columns(
            list(BASE_COLUMNS_2D) + ["data(0)", "data(1)"], Path("probe.csv")
        )
        self.assertEqual(base, list(BASE_COLUMNS_2D))
        self.assertEqual(data, ["data(0)", "data(1)"])

    def test_validate_base_columns_recognises_three_dimensions(self):
        base, data = _validate_base_columns(
            list(BASE_COLUMNS_3D) + ["data(0)"], Path("probe.csv")
        )
        self.assertEqual(base, list(BASE_COLUMNS_3D))

    def test_validate_base_columns_rejects_a_header_without_data(self):
        with self.assertRaises(SystemExit):
            _validate_base_columns(list(BASE_COLUMNS_2D), Path("probe.csv"))

    def test_validate_base_columns_rejects_a_foreign_header(self):
        with self.assertRaises(SystemExit):
            _validate_base_columns(["t", "data(0)"], Path("probe.csv"))

    def test_load_csv_returns_the_frame_and_its_column_groups(self):
        path = write_csv(self.directory, HEADER_2D + TWO_PROBES)
        frame, base_columns, data_columns = _load_csv(path)
        self.assertEqual(base_columns, list(BASE_COLUMNS_2D))
        self.assertEqual(data_columns, ["data(0)"])
        self.assertEqual(len(frame), 4)

    def test_load_csv_sorts_by_probe_and_time(self):
        path = write_csv(
            self.directory,
            HEADER_2D
            + "1,0,1.0,0.3,0.4,4.0\n0,0,1.0,0.1,0.2,2.0\n0,0,0.0,0.1,0.2,1.0\n",
        )
        frame, _, _ = _load_csv(path)
        self.assertEqual(list(frame["number(0)"]), [0, 0, 1])
        self.assertEqual(list(frame["t"]), [0.0, 1.0, 1.0])

    def test_load_csv_rejects_a_file_without_data_rows(self):
        with self.assertRaises(SystemExit):
            _load_csv(write_csv(self.directory, HEADER_2D))

    def test_load_csv_rejects_non_numeric_values(self):
        path = write_csv(self.directory, HEADER_2D + "0,0,0.0,0.1,0.2,none\n")
        with self.assertRaises(SystemExit):
            _load_csv(path)

    def test_resolve_output_path_defaults_next_to_the_csv(self):
        self.assertEqual(
            _resolve_output_path(Path("/tmp/probe.csv"), None), Path("/tmp/probe.png")
        )

    def test_resolve_output_path_forces_a_png_suffix(self):
        self.assertEqual(
            _resolve_output_path(Path("probe.csv"), "out/probe.pdf"),
            Path("out/probe.png"),
        )

    def test_resolve_title_falls_back_to_the_file_stem(self):
        arguments = argparse.Namespace(no_title=False, title=None)
        self.assertEqual(_resolve_title(Path("a/probe.csv"), arguments), "probe")

    def test_resolve_title_prefers_an_explicit_title(self):
        arguments = argparse.Namespace(no_title=False, title="Gauges")
        self.assertEqual(_resolve_title(Path("probe.csv"), arguments), "Gauges")

    def test_resolve_title_honours_no_title(self):
        arguments = argparse.Namespace(no_title=True, title="Gauges")
        self.assertIsNone(_resolve_title(Path("probe.csv"), arguments))

    def test_select_probe_keeps_every_row_without_a_request(self):
        frame, _, _ = _load_csv(write_csv(self.directory, HEADER_2D + TWO_PROBES))
        self.assertEqual(len(_select_probe(frame, None)), 4)

    def test_select_probe_keeps_the_requested_pair(self):
        frame, _, _ = _load_csv(write_csv(self.directory, HEADER_2D + TWO_PROBES))
        selected = _select_probe(frame, (1, 0))
        self.assertEqual(len(selected), 2)
        self.assertEqual(set(selected["number(0)"]), {1})

    def test_select_probe_rejects_an_absent_pair(self):
        frame, _, _ = _load_csv(write_csv(self.directory, HEADER_2D + TWO_PROBES))
        with self.assertRaises(SystemExit):
            _select_probe(frame, (7, 7))

    def test_select_data_columns_defaults_to_every_column(self):
        self.assertEqual(
            _select_data_columns(["data(0)", "data(1)"], None), ["data(0)", "data(1)"]
        )

    def test_select_data_columns_keeps_the_requested_order(self):
        self.assertEqual(
            _select_data_columns(["data(0)", "data(1)"], ["data(1)"]), ["data(1)"]
        )

    def test_select_data_columns_rejects_an_unknown_column(self):
        with self.assertRaises(SystemExit):
            _select_data_columns(["data(0)"], ["data(9)"])

    def test_format_probe_label_names_the_pair(self):
        group = pd.DataFrame({"number(0)": [2], "number(1)": [3]})
        self.assertEqual(_format_probe_label(group), "Probe (2, 3)")

    def test_create_plot_draws_one_line_per_probe_and_column(self):
        frame, _, data_columns = _load_csv(
            write_csv(self.directory, HEADER_2D + TWO_PROBES)
        )
        figure = _create_plot(frame, data_columns, "Gauges", None, None)
        try:
            axes = figure.axes[0]
            self.assertEqual(len(axes.get_lines()), 2)
            self.assertEqual(axes.get_title(), "Gauges")
        finally:
            plt.close(figure)

    def test_create_plot_applies_the_requested_limits(self):
        frame, _, data_columns = _load_csv(
            write_csv(self.directory, HEADER_2D + TWO_PROBES)
        )
        figure = _create_plot(frame, data_columns, None, [0.0, 2.0], [0.0, 5.0])
        try:
            axes = figure.axes[0]
            self.assertEqual(axes.get_xlim(), (0.0, 2.0))
            self.assertEqual(axes.get_ylim(), (0.0, 5.0))
        finally:
            plt.close(figure)

    def test_write_outputs_creates_the_target_directory(self):
        figure = plt.figure()
        try:
            target = self.directory / "figures" / "probe.png"
            _write_outputs(figure, target, 100, False)
        finally:
            plt.close(figure)
        self.assertTrue(target.is_file())

    def test_configure_pgf_reports_a_missing_tex_engine(self):
        original = plot_probe_data.shutil.which
        plot_probe_data.shutil.which = lambda name: None
        try:
            with self.assertRaises(SystemExit):
                plot_probe_data._configure_pgf()
        finally:
            plot_probe_data.shutil.which = original

    def test_argument_parser_defaults(self):
        arguments = _build_argument_parser().parse_args(["probe.csv"])
        self.assertIsNone(arguments.probe)
        self.assertIsNone(arguments.data_columns)
        self.assertEqual(arguments.dpi, plot_probe_data.DEFAULT_DPI)

    def test_argument_parser_reads_a_probe_pair(self):
        arguments = _build_argument_parser().parse_args(
            ["probe.csv", "--probe", "1", "0"]
        )
        self.assertEqual(arguments.probe, [1, 0])

    def test_main_writes_a_plot_next_to_the_csv(self):
        path = write_csv(self.directory, HEADER_2D + TWO_PROBES)
        self._run_main([str(path), "--dpi", "50"])
        self.assertTrue(path.with_suffix(".png").is_file())

    def test_main_rejects_a_missing_file(self):
        with self.assertRaises(SystemExit):
            self._run_main([str(self.directory / "absent.csv")])

    def test_main_rejects_a_non_positive_resolution(self):
        path = write_csv(self.directory, HEADER_2D + TWO_PROBES)
        with self.assertRaises(SystemExit):
            self._run_main([str(path), "--dpi", "0"])

    def _run_main(self, arguments):
        """Runs main() on a prepared command line without touching sys.argv."""
        original = plot_probe_data._build_argument_parser
        plot_probe_data._build_argument_parser = lambda: _FixedArgumentParser(arguments)
        try:
            main()
        finally:
            plot_probe_data._build_argument_parser = original


class _FixedArgumentParser:
    """Parses a prepared command line, so main() runs without touching sys.argv."""

    def __init__(self, arguments):
        self._parser = _build_argument_parser()
        self._arguments = arguments

    def parse_args(self):
        return self._parser.parse_args(self._arguments)


if __name__ == "__main__":
    unittest.main()
