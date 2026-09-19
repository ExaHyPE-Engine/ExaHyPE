# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import unittest

import numpy as np

from postprocessing.check_symmetry import check_snapshot, field_kind, mirror_values
from postprocessing.solution_io import MeshSnapshot


def make_two_cell_snapshot(cell_data):
    # Two unit quads side by side along x; the x-mirror about (1, 0.5) maps one onto the other.
    points = [[0.0, 0.0], [1.0, 0.0], [2.0, 0.0], [0.0, 1.0], [1.0, 1.0], [2.0, 1.0]]
    cells = [[0, 1, 4, 3], [1, 2, 5, 4]]
    return MeshSnapshot(
        name="test",
        time=0.0,
        points=np.asarray(points, dtype=float),
        cells=np.asarray(cells),
        node_data={},
        cell_data={
            name: np.asarray(values, dtype=float) for name, values in cell_data.items()
        },
        field_order=[(name, "cell") for name in cell_data],
    )


CENTRE = np.asarray([1.0, 0.5])


class CheckSymmetryTest(unittest.TestCase):
    def check(self, snapshot, **kinds):
        return check_snapshot(snapshot, CENTRE, "x", 0.0, 0.0, ["Subdomain"], **kinds)

    def test_field_kind_defaults(self):
        self.assertEqual(field_kind("rho", 1, [], [], []), "scalar")
        self.assertEqual(field_kind("u", 2, [], [], []), "vector")
        self.assertEqual(field_kind("rhoU", 3, [], [], []), "vector")
        self.assertEqual(field_kind("stress", 9, [], [], []), "tensor")
        self.assertEqual(field_kind("packed", 4, [], [], []), "scalar")

    def test_field_kind_overrides(self):
        self.assertEqual(field_kind("packed", 4, ["packed"], [], []), "vector")
        self.assertEqual(field_kind("rhoU", 3, [], [], ["rhoU"]), "scalar")
        self.assertEqual(field_kind("t", 9, [], ["t"], []), "tensor")

    def test_packed_state_vector_is_not_mirrored_as_a_vector(self):
        # A packed [rho, rhoU_x, rhoU_y, rhoE] state, identical in both cells: Symmetric as
        # per-component scalars. Vector semantics would negate the density component.
        packed = [[1.0, 0.0, 0.0, 2.5], [1.0, 0.0, 0.0, 2.5]]
        self.assertIsNone(self.check(make_two_cell_snapshot({"packed": packed})))

    def test_three_component_field_defaults_to_vector(self):
        # The x component flips sign between mirror partners, so it is symmetric only under
        # vector semantics.
        velocity = [[1.0, 0.5, 0.0], [-1.0, 0.5, 0.0]]
        self.assertIsNone(self.check(make_two_cell_snapshot({"velocity": velocity})))
        self.assertIsNotNone(
            self.check(
                make_two_cell_snapshot({"velocity": velocity}),
                scalar_fields=["velocity"],
            )
        )

    def test_two_component_field_defaults_to_vector(self):
        # Raw 2D output stores geometric vectors with two components.
        velocity = [[1.0, 0.5], [-1.0, 0.5]]
        self.assertIsNone(self.check(make_two_cell_snapshot({"u": velocity})))
        self.assertIsNotNone(
            self.check(make_two_cell_snapshot({"u": velocity}), scalar_fields=["u"])
        )

    def test_tensor_field_transforms_on_both_indices(self):
        # An x reflection negates the xy and yx entries of a rank-2 tensor and keeps the
        # diagonal, so an antisymmetric off-diagonal pair is mirror-symmetric.
        left = [2.0, 1.0, 0.0, 1.0, 3.0, 0.0, 0.0, 0.0, 0.0]
        right = [2.0, -1.0, 0.0, -1.0, 3.0, 0.0, 0.0, 0.0, 0.0]
        self.assertIsNone(self.check(make_two_cell_snapshot({"stress": [left, right]})))

    def test_tiny_cells_displaced_by_half_a_cell_are_not_symmetric(self):
        # Two cells of width 1e-6 whose mirror partners sit half a cell width
        # off: The geometry test works from the cell's own size, so the
        # displacement is detected however small the cells are.
        width = 1.0e-6
        shift = 0.5 * width
        points = [
            [0.0, 0.0],
            [width, 0.0],
            [width, width],
            [0.0, width],
            [2.0 + shift, 0.0],
            [2.0 + width + shift, 0.0],
            [2.0 + width + shift, width],
            [2.0 + shift, width],
        ]
        cells = [[0, 1, 2, 3], [4, 5, 6, 7]]
        snapshot = MeshSnapshot(
            name="test",
            time=0.0,
            points=np.asarray(points, dtype=float),
            cells=np.asarray(cells),
            node_data={},
            cell_data={"q": np.asarray([[1.0], [1.0]], dtype=float)},
            field_order=[("q", "cell")],
        )
        centre = np.asarray([1.0 + 0.5 * width, 0.5 * width])
        result = check_snapshot(snapshot, centre, "x", 0.0, 0.0, ["Subdomain"])
        self.assertIsNotNone(result)

    def test_asymmetric_values_are_detected(self):
        self.assertIsNotNone(
            self.check(make_two_cell_snapshot({"rho": [[1.0], [2.0]]}))
        )

    def test_mirror_values_axis_swap_exchanges_vector_components(self):
        values = np.asarray([[1.0, 2.0, 3.0]])
        swapped = mirror_values(values, "xy", "vector")
        self.assertTrue(np.array_equal(swapped, np.asarray([[2.0, 1.0, 3.0]])))

    def test_mirror_values_axis_swap_exchanges_tensor_rows_and_columns(self):
        tensor = np.arange(9.0).reshape(1, 9)
        swapped = mirror_values(tensor, "xy", "tensor").reshape(3, 3)
        expected = np.arange(9.0).reshape(3, 3)[[1, 0, 2]][:, [1, 0, 2]]
        self.assertTrue(np.array_equal(swapped, expected))


if __name__ == "__main__":
    unittest.main()
