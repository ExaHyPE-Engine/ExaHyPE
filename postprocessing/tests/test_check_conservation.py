# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import unittest

import numpy as np

from postprocessing.check_conservation import (
    count_limited_cells,
    galerkin_cells,
    integrate,
    quadrature_weights,
    unit_rule,
)
from postprocessing.solution_io import MeshSnapshot


def make_galerkin_snapshot(kind, nodes, cells, field):
    # Every Galerkin cell (lower corner, width) plots its nodes x nodes points as quads of its own.
    unit_nodes, _ = unit_rule(kind, nodes)
    points, quads = [], []
    for lower, width in cells:
        first = len(points)
        for y in unit_nodes:
            for x in unit_nodes:
                points.append([lower[0] + x * width[0], lower[1] + y * width[1]])
        for j in range(nodes - 1):
            for i in range(nodes - 1):
                corner = first + j * nodes + i
                quads.append([corner, corner + 1, corner + nodes + 1, corner + nodes])
    points = np.asarray(points)
    return MeshSnapshot(
        name="test",
        time=0.0,
        points=points,
        cells=np.asarray(quads),
        node_data={"q": field(points[:, 0], points[:, 1])},
        cell_data={},
        field_order=[("q", "node")],
    )


CELLS = [((0.0, 0.0), (1.0, 0.5)), ((1.0, 0.0), (0.5, 0.5)), ((1.5, 0.0), (0.5, 0.5))]


class CheckConservationTest(unittest.TestCase):
    def test_cell_data_integrates_as_volume_averages(self):
        points = [[0.0, 0.0], [1.0, 0.0], [3.0, 0.0], [0.0, 2.0], [1.0, 2.0], [3.0, 2.0]]
        snapshot = MeshSnapshot(
            name="test",
            time=0.0,
            points=np.asarray(points),
            cells=np.asarray([[0, 1, 4, 3], [1, 2, 5, 4]]),
            node_data={},
            cell_data={"rho": np.asarray([1.5, 0.25]), "j": np.asarray([[1.0, 2.0], [0.0, 1.0]])},
            field_order=[("rho", "cell"), ("j", "cell")],
        )
        integrals = integrate(snapshot, ["rho", "j"])
        np.testing.assert_allclose(integrals["rho"], [1.5 * 2.0 + 0.25 * 4.0])
        np.testing.assert_allclose(integrals["j"], [2.0, 8.0])

    def test_every_galerkin_cell_is_one_component(self):
        snapshot = make_galerkin_snapshot("legendre", 4, CELLS, lambda x, y: x)
        self.assertEqual(sorted(len(cell) for cell in galerkin_cells(snapshot)), [16, 16, 16])

    def test_gauss_legendre_nodes_integrate_their_polynomials_exactly(self):
        # Four Gauss-Legendre nodes integrate degree seven per axis exactly.
        snapshot = make_galerkin_snapshot("legendre", 4, CELLS, lambda x, y: x**7 + x * y**6)
        exact = 2.0**8 / 8.0 * 0.5 + 2.0**2 / 2.0 * 0.5**7 / 7.0
        np.testing.assert_allclose(integrate(snapshot, ["q"])["q"], [exact], rtol=1e-12)

    def test_gauss_lobatto_nodes_integrate_their_polynomials_exactly(self):
        # Four Gauss-Lobatto nodes integrate degree five per axis exactly.
        snapshot = make_galerkin_snapshot("lobatto", 4, CELLS, lambda x, y: x**5 * y)
        exact = 2.0**6 / 6.0 * 0.5**2 / 2.0
        np.testing.assert_allclose(integrate(snapshot, ["q"])["q"], [exact], rtol=1e-12)

    def test_the_weights_sum_to_the_domain_area(self):
        snapshot = make_galerkin_snapshot("legendre", 3, CELLS, lambda x, y: x)
        self.assertAlmostEqual(quadrature_weights(snapshot).sum(), 1.0, places=12)

    def test_an_equidistant_layout_is_rejected(self):
        snapshot = make_galerkin_snapshot("legendre", 4, CELLS, lambda x, y: x)
        snapshot.points[:16] = np.asarray(
            [[x, y] for y in np.linspace(0.0, 0.5, 4) for x in np.linspace(0.0, 1.0, 4)]
        )
        with self.assertRaises(ValueError):
            quadrature_weights(snapshot)

    def test_limited_galerkin_cells_are_counted_once(self):
        snapshot = make_galerkin_snapshot("legendre", 3, CELLS, lambda x, y: x)
        status = np.zeros(snapshot.num_cells)
        status[[0, 1, 2, 3]] = 2.0
        status[[8]] = 3.0
        snapshot.cell_data["LimiterStatus"] = status
        self.assertEqual(count_limited_cells(snapshot), 2)


if __name__ == "__main__":
    unittest.main()
