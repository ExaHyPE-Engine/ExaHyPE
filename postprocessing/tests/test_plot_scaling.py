# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import pathlib
import tempfile
import unittest

from postprocessing.plot_scaling import (
    Run,
    _create_efficiency_plot,
    _create_strong_scaling_plot,
    _grid_label,
    _parallel_efficiency,
    _read_run,
    _read_runs,
)

LOG = """
Rank:0 Info │     Dimensions              3                                    │
Rank:0 Info │     MPI                     512 Ranks on 256 Nodes                │
Rank:0 Info │     Cells                531,441  (Refined 20,440)               │
Rank:0 Info │     Load Balance         99.85 %  (1,026 .. 1,053 Cells/Rank)     │
Rank:0 Info │     Average Time Step    0.0404 s  (49 Steps)                   │
Rank:0 Info │     Average Performance  1.3147e+07 Cell/s · 1.1501e+11 DoF/s    │
"""


def run_of(nodes, time, cells=531441, dimensions=3):
    return Run(nodes, dimensions, cells, time, cells * 8748 / time)


RUNS = [run_of(2, 4.0), run_of(4, 2.1), run_of(8, 1.1)]


class PlotScalingTest(unittest.TestCase):
    def test_a_log_reports_nodes_grid_time_and_rate(self):
        run = _read_run(LOG)
        self.assertEqual(run.nodes, 256)
        self.assertEqual(run.dimensions, 3)
        self.assertEqual(run.cells, 531441)
        self.assertAlmostEqual(run.time_per_step, 0.0404)
        self.assertAlmostEqual(run.degrees_of_freedom_per_second, 1.1501e11)

    def test_a_run_on_one_node_counts(self):
        run = _read_run(LOG.replace("512 Ranks on 256 Nodes", "1 Rank on 1 Node"))
        self.assertEqual(run.nodes, 1)

    def test_a_run_that_ended_early_is_left_out(self):
        self.assertIsNone(_read_run("512 Ranks on 256 Nodes\nKilled by signal 9\n"))
        with tempfile.TemporaryDirectory() as directory:
            folder = pathlib.Path(directory)
            (folder / "run-1.log").write_text(LOG, encoding="utf-8")
            (folder / "run-2.log").write_text("Killed\n", encoding="utf-8")
            runs = _read_runs([str(folder / "run-*.log")])
        self.assertEqual(len(runs), 1)

    def test_the_runs_come_sorted_by_their_node_count(self):
        with tempfile.TemporaryDirectory() as directory:
            folder = pathlib.Path(directory)
            (folder / "a.log").write_text(LOG, encoding="utf-8")
            (folder / "b.log").write_text(
                LOG.replace("512 Ranks on 256 Nodes", "8 Ranks on 4 Nodes"),
                encoding="utf-8",
            )
            runs = _read_runs([str(folder / "*.log")])
        self.assertEqual([run.nodes for run in runs], [4, 256])

    def test_two_logs_of_one_node_count_stop_the_script(self):
        with tempfile.TemporaryDirectory() as directory:
            folder = pathlib.Path(directory)
            (folder / "round-1.log").write_text(LOG, encoding="utf-8")
            (folder / "round-2.log").write_text(LOG, encoding="utf-8")
            with self.assertRaises(SystemExit):
                _read_runs([str(folder / "round-*.log")])

    def test_a_label_names_the_cells_per_axis_of_the_logged_dimension(self):
        self.assertEqual(_grid_label(run_of(2, 1.0, 729, dimensions=2)), "$27^2$")
        self.assertEqual(_grid_label(run_of(2, 1.0, 729, dimensions=3)), "$9^3$")
        self.assertEqual(_grid_label(run_of(2, 1.0, 1000, dimensions=2)), "1,000 cells")

    def test_the_efficiency_follows_the_rate_against_the_fewest_nodes(self):
        efficiency = _parallel_efficiency(
            [run_of(2, 4.0), run_of(4, 2.0), run_of(8, 1.25)]
        )
        self.assertEqual(len(efficiency), 3)
        for value, expected in zip(efficiency, [1.0, 1.0, 0.8]):
            self.assertAlmostEqual(value, expected)

    def test_the_strong_scaling_plot_draws_the_run_and_the_ideal_line(self):
        figure = _create_strong_scaling_plot(RUNS, ("Number of Cores", 48.0))
        axis = figure.axes[0]
        self.assertEqual(axis.get_yscale(), "log")
        self.assertEqual(len(axis.child_axes), 1)
        self.assertEqual(axis.child_axes[0].get_xlabel(), "Number of Cores")
        labels = [text.get_text() for text in axis.get_legend().get_texts()]
        self.assertEqual(labels, ["$81^3$", "Ideal"])
        self.assertEqual(list(axis.get_lines()[0].get_xdata()), [2, 4, 8])

    def test_the_efficiency_plot_draws_the_run_and_the_ideal(self):
        figure = _create_efficiency_plot(RUNS, None)
        axis = figure.axes[0]
        self.assertEqual(axis.get_yscale(), "linear")
        self.assertEqual(len(axis.child_axes), 0)
        labels = [text.get_text() for text in axis.get_legend().get_texts()]
        self.assertEqual(labels, ["$81^3$", "Ideal"])


if __name__ == "__main__":
    unittest.main()
