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
    patch_centres_along_curve,
    render_curve,
)
from postprocessing.solution_io import MeshSnapshot


def make_grid_snapshot(
    cells_per_axis: int, subdomains, curve_indices=None
) -> MeshSnapshot:
    """Builds a unit-square grid whose cells are written row by row along x.

    Every cell is its own patch with its position in the file as curve index unless
    curve_indices names the curve index of every cell.
    """
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
    if curve_indices is None:
        curve_indices = np.arange(len(subdomains))
    return MeshSnapshot(
        name="test",
        time=0.25,
        points=points,
        cells=np.asarray(cells),
        node_data={},
        cell_data={
            "Subdomain": subdomains,
            "CurveIndex": np.asarray(curve_indices, dtype=float),
        },
        field_order=[("Subdomain", "cell"), ("CurveIndex", "cell")],
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

    def test_the_patch_centres_follow_the_curve_index_not_the_snapshot_order(self):
        # The file stores the cells as (0.75, 0.25), (0.25, 0.25), (0.75, 0.75), (0.25, 0.75).
        snapshot = make_grid_snapshot(2, [0, 0, 0, 0], curve_indices=[1, 0, 2, 3])
        snapshot.cells = snapshot.cells[[1, 0, 3, 2]]
        centres = patch_centres_along_curve(snapshot, cell_polygons(snapshot))
        np.testing.assert_allclose(
            centres, [[0.25, 0.25], [0.75, 0.25], [0.75, 0.75], [0.25, 0.75]]
        )

    def test_the_cells_of_one_patch_join_into_one_centre(self):
        # The left and the right column form one patch each. The right one comes first on the curve.
        snapshot = make_grid_snapshot(2, [0, 0, 0, 0], curve_indices=[7, 3, 7, 3])
        centres = patch_centres_along_curve(snapshot, cell_polygons(snapshot))
        np.testing.assert_allclose(centres, [[0.75, 0.5], [0.25, 0.5]])

    def test_render_curve_joins_the_patch_centres_for_different_rank_labels(self):
        snapshot = make_grid_snapshot(2, [0, 0, 0, 0], curve_indices=[1, 0, 2, 3])
        centres = patch_centres_along_curve(snapshot, cell_polygons(snapshot))
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

    def test_render_curve_writes_an_image_and_counts_ranks_patches_and_cells(self):
        snapshot = make_grid_snapshot(
            4, [0] * 8 + [1] * 8, curve_indices=np.arange(16) // 2
        )
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "curve.png"
            summary = render_curve(snapshot, "unit test", output)
            self.assertTrue(output.exists())
            self.assertGreater(output.stat().st_size, 0)
        self.assertEqual(summary["subdomains"], 2)
        self.assertEqual(summary["patches"], 8)
        self.assertEqual(summary["cells"], 16)

    def test_render_curve_counts_labels_that_start_at_one_and_skip_values(self):
        snapshot = make_grid_snapshot(4, [1] * 6 + [2] * 6 + [4] * 4)
        with tempfile.TemporaryDirectory() as directory:
            summary = render_curve(snapshot, "unit test", Path(directory) / "curve.png")
        self.assertEqual(summary["subdomains"], 3)


if __name__ == "__main__":
    unittest.main()
