# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import unittest

import numpy as np

from postprocessing.render_solution import (
    _global_clim,
    build_parser,
    clim_of_field,
    default_field,
    ordered_field_options,
    render_slice_figure,
)
from postprocessing.solution_io import MeshSnapshot

UNIT_QUAD_POINTS = [[0.0, 0.0], [1.0, 0.0], [1.0, 1.0], [0.0, 1.0]]


def make_snapshot(node_values, points=None, cells=None):
    points = UNIT_QUAD_POINTS if points is None else points
    cells = [[0, 1, 2, 3]] if cells is None else cells
    return MeshSnapshot(
        name="test",
        time=0.0,
        points=np.asarray(points, dtype=float),
        cells=np.asarray(cells),
        node_data={"q": np.asarray(node_values, dtype=float)},
        cell_data={},
        field_order=[("q", "node")],
    )


def make_cube_snapshot():
    """Returns 2 x 2 x 2 hexahedra with the node field q and the cell field CurveIndex."""
    axis = np.array([0.0, 0.5, 1.0])
    points = np.array([[x, y, z] for z in axis for y in axis for x in axis])
    cells = [
        [
            (z + dz) * 9 + (y + dy) * 3 + (x + dx)
            for dz in (0, 1)
            for dy in (0, 1)
            for dx in (0, 1)
        ]
        for z in (0, 1)
        for y in (0, 1)
        for x in (0, 1)
    ]
    return MeshSnapshot(
        name="test",
        time=0.0,
        points=points,
        cells=np.asarray(cells),
        node_data={"q": points[:, 0]},
        cell_data={"CurveIndex": np.arange(8.0)},
        field_order=[("q", "node"), ("CurveIndex", "cell")],
    )


class FixedSource:
    def __init__(self, snapshots):
        self._snapshots = snapshots
        self.count = len(snapshots)

    def snapshot(self, index):
        return self._snapshots[index]


class RenderSolutionTest(unittest.TestCase):
    def test_global_clim_spans_every_snapshot(self):
        source = FixedSource(
            [
                make_snapshot([[1.0], [1.0], [1.0], [1.0]]),
                make_snapshot([[5.0], [5.0], [5.0], [5.0]]),
                make_snapshot([[-2.0], [-2.0], [-2.0], [-2.0]]),
            ]
        )
        self.assertEqual(_global_clim(source, "q", None), (-2.0, 5.0))

    def test_global_clim_survives_a_refined_snapshot(self):
        refined_points = [
            [0.0, 0.0],
            [0.5, 0.0],
            [0.5, 0.5],
            [0.0, 0.5],
            [1.0, 0.0],
            [1.0, 0.5],
        ]
        source = FixedSource(
            [
                make_snapshot([[1.0], [1.0], [1.0], [1.0]]),
                make_snapshot(
                    [[0.0], [2.0], [4.0], [6.0], [8.0], [10.0]],
                    points=refined_points,
                    cells=[[0, 1, 2, 3], [1, 4, 5, 2]],
                ),
            ]
        )
        # A cell value is the mean of its nodal values. The second snapshot has the cell
        # means 3 and 6.
        self.assertEqual(_global_clim(source, "q", None), (1.0, 6.0))

    def test_global_clim_widens_a_constant_field(self):
        source = FixedSource([make_snapshot([[3.0], [3.0], [3.0], [3.0]])])
        low, high = _global_clim(source, "q", None)
        self.assertLess(low, high)
        self.assertAlmostEqual(0.5 * (low + high), 3.0)

    def test_global_clim_spans_an_outlier_snapshot(self):
        values = [0.0] * 49 + [1.0] * 50 + [1000.0]
        source = FixedSource(
            [make_snapshot([[value], [value], [value], [value]]) for value in values]
        )
        self.assertEqual(_global_clim(source, "q", None), (0.0, 1000.0))

    def test_the_colour_limits_are_open_until_a_run_bounds_them(self):
        arguments = build_parser().parse_args(["solutions"])
        self.assertIsNone(arguments.vmin)
        self.assertIsNone(arguments.vmax)

    def test_the_colour_limits_take_a_lower_and_an_upper_value(self):
        arguments = build_parser().parse_args(
            ["solutions", "--vmin", "0", "--vmax", "2"]
        )
        self.assertAlmostEqual(arguments.vmin, 0.0)
        self.assertAlmostEqual(arguments.vmax, 2.0)

    def test_the_colour_limits_of_one_field_leave_the_other_fields_free(self):
        self.assertEqual(clim_of_field("rho", "rho", (1.0, 2.0)), (1.0, 2.0))
        self.assertIsNone(clim_of_field("RefinementLevel", "rho", (1.0, 2.0)))
        self.assertIsNone(clim_of_field("CurveIndex", "rho", (1.0, 2.0)))
        self.assertIsNone(clim_of_field("rho", "rho", None))

    def test_a_slice_draws_a_cell_field_without_interpolation(self):
        snapshot = make_cube_snapshot()
        cell_figure = render_slice_figure(snapshot, "CurveIndex", "z", 0)
        node_figure = render_slice_figure(snapshot, "q", "z", 0)
        cell_image = cell_figure.axes[0].images[0]
        node_image = node_figure.axes[0].images[0]
        self.assertEqual(cell_image.get_interpolation(), "nearest")
        self.assertEqual(node_image.get_interpolation(), "bicubic")
        np.testing.assert_array_equal(cell_image.get_array(), [[0.0, 1.0], [2.0, 3.0]])

    def test_the_grid_fields_come_last_and_are_never_the_default(self):
        variables = ["CurveIndex", "E", "RefinementLevel", "Subdomain", "j", "rho"]
        self.assertEqual(default_field(variables), "E")
        self.assertEqual(
            ordered_field_options(variables),
            ["E", "j", "rho", "CurveIndex", "RefinementLevel", "Subdomain"],
        )


if __name__ == "__main__":
    unittest.main()
