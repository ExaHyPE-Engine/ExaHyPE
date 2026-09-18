# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
"""Tests for render_curve: Cell polygons, patch grouping along the curve, label colours and the image."""

from pathlib import Path
import tempfile
import unittest

import numpy as np

from postprocessing.render_curve import (
    cell_polygons,
    label_colours,
    patch_centres_along_curve,
    render_curve,
)
from postprocessing.solution_io import MeshSnapshot


def make_grid_snapshot(cells_per_axis: int, subdomains, subtrees=None) -> MeshSnapshot:
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
    subtrees = subdomains if subtrees is None else np.asarray(subtrees, dtype=float)
    return MeshSnapshot(
        name="test",
        time=0.25,
        points=points,
        cells=np.asarray(cells),
        node_data={},
        cell_data={"Subdomain": subdomains, "Subtree": subtrees},
        field_order=[("Subdomain", "cell"), ("Subtree", "cell")],
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

    def test_patch_centres_follow_the_curve_position_and_not_the_file_order(self):
        snapshot = make_grid_snapshot(4, np.zeros(16))
        polygons = cell_polygons(snapshot)
        # Four patches of one grid row each, written in the file in reverse curve order.
        snapshot.cell_data["CurvePosition"] = np.repeat([0.75, 0.5, 0.25, 0.0], 4)
        centres = patch_centres_along_curve(snapshot, polygons)
        np.testing.assert_allclose(
            centres, [[0.5, 0.875], [0.5, 0.625], [0.5, 0.375], [0.5, 0.125]]
        )

    def test_patch_centres_group_patches_of_any_cell_count(self):
        snapshot = make_grid_snapshot(4, np.zeros(16))
        polygons = cell_polygons(snapshot)
        # A Finite Volume patch, an ADER-DG cell and a coarser cell carry different cell counts.
        snapshot.cell_data["CurvePosition"] = np.concatenate(
            [np.full(9, 0.1), np.full(4, 0.4), np.full(3, 0.8)]
        )
        centres = patch_centres_along_curve(snapshot, polygons)
        cell_centres = polygons.mean(axis=1)
        self.assertEqual(centres.shape, (3, 2))
        np.testing.assert_allclose(centres[0], cell_centres[:9].mean(axis=0))
        np.testing.assert_allclose(centres[1], cell_centres[9:13].mean(axis=0))
        np.testing.assert_allclose(centres[2], cell_centres[13:].mean(axis=0))

    def test_patch_centres_without_the_field_keep_one_patch_per_cell(self):
        snapshot = make_grid_snapshot(4, np.zeros(16))
        polygons = cell_polygons(snapshot)
        np.testing.assert_allclose(
            patch_centres_along_curve(snapshot, polygons), polygons.mean(axis=1)
        )

    def test_label_colours_give_one_colour_per_cell_and_the_same_colour_per_label(self):
        colours = label_colours(np.asarray([0.0, 1.0, 0.0, 12.0]))
        self.assertEqual(colours.shape, (4, 4))
        np.testing.assert_allclose(colours[0], colours[2])
        self.assertFalse(np.allclose(colours[0], colours[1]))

    def test_render_curve_writes_an_image_and_counts_ranks_and_tasks(self):
        snapshot = make_grid_snapshot(4, [0] * 8 + [1] * 8, [0, 1, 2, 3] * 4)
        snapshot.cell_data["CurvePosition"] = np.repeat([0.0, 0.25, 0.5, 0.75], 4)
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "curve.png"
            summary = render_curve(snapshot, "unit test", output)
            self.assertTrue(output.exists())
            self.assertGreater(output.stat().st_size, 0)
        self.assertEqual(summary["subdomains"], 2)
        # The plotter writes the tree id, which is unique across the ranks.
        self.assertEqual(summary["subtrees"], 4)
        self.assertEqual(summary["patches"], 4)
        self.assertEqual(summary["cells"], 16)

    def test_render_curve_counts_labels_that_start_at_one_and_skip_values(self):
        # Three trees numbered 1, 2 and 4: The largest id exceeds the number of trees.
        snapshot = make_grid_snapshot(4, np.zeros(16), [1] * 6 + [2] * 6 + [4] * 4)
        with tempfile.TemporaryDirectory() as directory:
            summary = render_curve(snapshot, "unit test", Path(directory) / "curve.png")
        self.assertEqual(summary["subtrees"], 3)
        self.assertEqual(summary["subdomains"], 1)


if __name__ == "__main__":
    unittest.main()
