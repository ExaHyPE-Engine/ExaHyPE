# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import unittest

import numpy as np

from postprocessing.check_conservation import (
    build_parser,
    get_round_off_bound,
    integrate,
    integrate_cell_field,
    integrate_node_field,
)
from postprocessing.solution_io import MeshSnapshot


def make_cell_snapshot(values):
    # A unit quad and a quad of width two lie side by side along x.
    points = [[0.0, 0.0], [1.0, 0.0], [3.0, 0.0], [0.0, 1.0], [1.0, 1.0], [3.0, 1.0]]
    return MeshSnapshot(
        name="cells",
        time=0.0,
        points=np.asarray(points),
        cells=np.asarray([[0, 1, 4, 3], [1, 2, 5, 4]]),
        node_data={},
        cell_data={"rho": np.asarray(values, dtype=float)},
        field_order=[("rho", "cell")],
    )


def make_node_snapshot(nodes_per_axis, function, offset, width):
    # The points are the Gauss-Legendre nodes of one cell of the given offset and width, first
    # axis fastest.
    nodes, _ = np.polynomial.legendre.leggauss(nodes_per_axis)
    positions = offset + width * 0.5 * (nodes + 1.0)
    points = np.asarray([[x, y] for y in positions for x in positions])
    return MeshSnapshot(
        name="nodes",
        time=0.0,
        points=points,
        cells=np.zeros((0, 4), dtype=int),
        node_data={"rho": function(points[:, 0], points[:, 1])},
        cell_data={},
        field_order=[("rho", "node")],
    )


class CheckConservationTest(unittest.TestCase):
    def test_default_field_is_the_density(self):
        arguments = build_parser().parse_args(["solution", "--time-steps", "1"])
        self.assertEqual(arguments.fields, ["rho"])

    def test_bound_is_one_rounding_per_time_step_and_one_for_the_measurement(self):
        self.assertEqual(get_round_off_bound(3, "double"), 4 * np.finfo(np.float64).eps)
        self.assertEqual(
            get_round_off_bound(0, "float"), float(np.finfo(np.float32).eps)
        )

    def test_cell_field_integrates_over_the_cell_measures(self):
        integral, absolute = integrate_cell_field(
            make_cell_snapshot([2.0, -1.0]), np.asarray([2.0, -1.0])
        )
        self.assertEqual(integral, 0.0)
        self.assertEqual(absolute, 4.0)

    def test_node_field_integrates_polynomials_of_the_quadrature_order_exactly(self):
        snapshot = make_node_snapshot(3, lambda x, y: x**5 * y**4, 1.0, 2.0)
        integral, absolute = integrate_node_field(
            snapshot, snapshot.node_data["rho"], 3
        )
        expected = (3.0**6 - 1.0) / 6.0 * (3.0**5 - 1.0) / 5.0
        self.assertAlmostEqual(integral / expected, 1.0, places=14)
        self.assertAlmostEqual(absolute / expected, 1.0, places=14)

    def test_node_field_requires_the_number_of_nodes(self):
        snapshot = make_node_snapshot(2, lambda x, y: x + y, 0.0, 1.0)
        with self.assertRaises(ValueError):
            integrate(snapshot, "rho", None)

    def test_node_field_rejects_a_point_count_of_another_node_number(self):
        snapshot = make_node_snapshot(2, lambda x, y: x + y, 0.0, 1.0)
        with self.assertRaises(ValueError):
            integrate(snapshot, "rho", 3)


if __name__ == "__main__":
    unittest.main()
