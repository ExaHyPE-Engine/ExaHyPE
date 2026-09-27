# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import numpy as np

from postprocessing.render_curve import (
    cell_polygons,
    label_colours,
    render_curve,
)
from postprocessing.solution_io import MeshSnapshot


def make_grid_snapshot(cells_per_axis: int, subdomains) -> MeshSnapshot:
    """Builds a unit-square grid whose cells are written row by row along x."""
    axis = np.linspace(0.0, 1.0, cells_per_axis + 1)
    points = np.asarray([[x, y] for y in axis for x in axis])
    cells = []
    for row in range(cells_per_axis):
        for column in range(cells_per_axis):
            lower_left = row * (cells_per_axis + 1) + column
            cells.append(
                [
                    lower_left,
                    lower_left + 1,
                    lower_left + cells_per_axis + 2,
                    lower_left + cells_per_axis + 1,
                ]
            )
    subdomains = np.asarray(subdomains, dtype=float)
    return MeshSnapshot(
        name="test",
        time=0.25,
        points=points,
        cells=np.asarray(cells),
        node_data={},
        cell_data={"Subdomain": subdomains},
        field_order=[("Subdomain", "cell")],
    )


class RenderCurveTest(unittest.TestCase):
    def test_cell_polygons_take_the_first_four_corners(self):
        snapshot = make_grid_snapshot(2, [0, 0, 1, 1])
        polygons = cell_polygons(snapshot)
        self.assertEqual(polygons.shape, (4, 4, 2))
        np.testing.assert_allclose(
            polygons[0], [[0.0, 0.0], [0.5, 0.0], [0.5, 0.5], [0.0, 0.5]]
        )
        np.testing.assert_allclose(
            polygons[3], [[0.5, 0.5], [1.0, 0.5], [1.0, 1.0], [0.5, 1.0]]
        )

    def test_cell_polygons_reject_three_dimensional_points(self):
        snapshot = make_grid_snapshot(1, [0])
        snapshot.points = np.hstack(
            [snapshot.points, np.zeros((len(snapshot.points), 1))]
        )
        with self.assertRaises(ValueError):
            cell_polygons(snapshot)

    def test_render_curve_follows_snapshot_order_for_different_rank_labels(self):
        snapshot = make_grid_snapshot(2, [0, 0, 0, 0])
        snapshot.cells = snapshot.cells[[3, 0, 2, 1]]
        centres = cell_polygons(snapshot).mean(axis=1)
        with tempfile.TemporaryDirectory() as directory:
            for labels in ([0, 0, 0, 0], [1, 1, 0, 0]):
                snapshot.cell_data["Subdomain"] = np.asarray(labels)
                with patch("matplotlib.axes.Axes.plot") as plot:
                    render_curve(snapshot, "unit test", Path(directory) / "curve.png")
                plot.assert_called_once()
                np.testing.assert_allclose(plot.call_args.args[0], centres[:, 0])
                np.testing.assert_allclose(plot.call_args.args[1], centres[:, 1])

    def test_render_curve_rejects_an_empty_snapshot(self):
        snapshot = make_grid_snapshot(1, [])
        snapshot.cells = snapshot.cells[:0]
        with self.assertRaisesRegex(ValueError, "at least one cell"):
            render_curve(snapshot, "unit test", Path("unused.png"))

    def test_label_colours_give_one_colour_per_cell_and_the_same_colour_per_label(self):
        colours = label_colours(np.asarray([0.0, 1.0, 0.0, 12.0]))
        self.assertEqual(colours.shape, (4, 4))
        np.testing.assert_allclose(colours[0], colours[2])
        self.assertFalse(np.allclose(colours[0], colours[1]))

    def test_render_curve_writes_an_image_and_counts_ranks_and_cells(self):
        snapshot = make_grid_snapshot(4, [0] * 8 + [1] * 8)
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "curve.png"
            summary = render_curve(snapshot, "unit test", output)
            self.assertTrue(output.exists())
            self.assertGreater(output.stat().st_size, 0)
        self.assertEqual(summary["subdomains"], 2)
        self.assertEqual(summary["cells"], 16)

    def test_render_curve_counts_labels_that_start_at_one_and_skip_values(self):
        snapshot = make_grid_snapshot(4, [1] * 6 + [2] * 6 + [4] * 4)
        with tempfile.TemporaryDirectory() as directory:
            summary = render_curve(snapshot, "unit test", Path(directory) / "curve.png")
        self.assertEqual(summary["subdomains"], 3)


if __name__ == "__main__":
    unittest.main()
