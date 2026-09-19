# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import unittest

import numpy as np

from postprocessing.compare_solution import compare_snapshots
from postprocessing.solution_io import MeshSnapshot


def make_snapshot(points, cells, cell_data=None, node_data=None):
    cell_data = cell_data or {}
    node_data = node_data or {}
    field_order = [(name, "cell") for name in cell_data] + [
        (name, "node") for name in node_data
    ]
    return MeshSnapshot(
        name="test",
        time=0.0,
        points=np.asarray(points, dtype=float),
        cells=np.asarray(cells),
        node_data={
            name: np.asarray(values, dtype=float) for name, values in node_data.items()
        },
        cell_data={
            name: np.asarray(values, dtype=float) for name, values in cell_data.items()
        },
        field_order=field_order,
    )


UNIT_QUAD_POINTS = [[0.0, 0.0], [1.0, 0.0], [1.0, 1.0], [0.0, 1.0]]


class CompareSolutionTest(unittest.TestCase):
    def compare(self, left, right, field=None, unordered=False, skip_fields=None):
        return compare_snapshots(
            left,
            right,
            0,
            0.0,
            0.0,
            field,
            unordered,
            ["Subdomain"] if skip_fields is None else skip_fields,
        )

    def test_identical_snapshots_match(self):
        left = make_snapshot(UNIT_QUAD_POINTS, [[0, 1, 2, 3]], cell_data={"q": [[1.5]]})
        right = make_snapshot(
            UNIT_QUAD_POINTS, [[0, 1, 2, 3]], cell_data={"q": [[1.5]]}
        )
        self.assertIsNone(self.compare(left, right))

    def test_skipped_field_missing_on_one_side_matches(self):
        left = make_snapshot(
            UNIT_QUAD_POINTS,
            [[0, 1, 2, 3]],
            cell_data={"q": [[1.5]], "Subdomain": [[0.0]]},
        )
        right = make_snapshot(
            UNIT_QUAD_POINTS, [[0, 1, 2, 3]], cell_data={"q": [[1.5]]}
        )
        self.assertIsNone(self.compare(left, right))

    def test_extra_unselected_field_matches_under_field_filter(self):
        left = make_snapshot(
            UNIT_QUAD_POINTS, [[0, 1, 2, 3]], cell_data={"q": [[1.5]], "extra": [[7.0]]}
        )
        right = make_snapshot(
            UNIT_QUAD_POINTS, [[0, 1, 2, 3]], cell_data={"q": [[1.5]]}
        )
        self.assertIsNone(self.compare(left, right, field="q"))

    def test_extra_selected_field_is_a_mismatch(self):
        left = make_snapshot(
            UNIT_QUAD_POINTS, [[0, 1, 2, 3]], cell_data={"q": [[1.5]], "extra": [[7.0]]}
        )
        right = make_snapshot(
            UNIT_QUAD_POINTS, [[0, 1, 2, 3]], cell_data={"q": [[1.5]]}
        )
        self.assertIn("fields differ", self.compare(left, right))

    def test_field_filter_reports_a_field_missing_on_either_side(self):
        left = make_snapshot(UNIT_QUAD_POINTS, [[0, 1, 2, 3]], cell_data={"q": [[1.5]]})
        right = make_snapshot(
            UNIT_QUAD_POINTS, [[0, 1, 2, 3]], cell_data={"q": [[1.5]]}
        )
        self.assertIn("not present", self.compare(left, right, field="missing"))

    def test_rotated_connectivity_matches_unordered(self):
        node_values = [1.0, 2.0, 3.0, 4.0]
        left = make_snapshot(
            UNIT_QUAD_POINTS, [[0, 1, 2, 3]], node_data={"q": node_values}
        )
        right = make_snapshot(
            UNIT_QUAD_POINTS, [[1, 2, 3, 0]], node_data={"q": node_values}
        )
        self.assertIsNone(self.compare(left, right, unordered=True))

    def test_reflected_connectivity_matches_unordered(self):
        node_values = [1.0, 2.0, 3.0, 4.0]
        left = make_snapshot(
            UNIT_QUAD_POINTS, [[0, 1, 2, 3]], node_data={"q": node_values}
        )
        right = make_snapshot(
            UNIT_QUAD_POINTS, [[0, 3, 2, 1]], node_data={"q": node_values}
        )
        self.assertIsNone(self.compare(left, right, unordered=True))

    def test_different_values_still_fail_unordered(self):
        left = make_snapshot(
            UNIT_QUAD_POINTS, [[0, 1, 2, 3]], node_data={"q": [1.0, 2.0, 3.0, 4.0]}
        )
        right = make_snapshot(
            UNIT_QUAD_POINTS, [[1, 2, 3, 0]], node_data={"q": [1.0, 2.0, 3.0, 5.0]}
        )
        self.assertIsNotNone(self.compare(left, right, unordered=True))

    def test_different_geometry_still_fails_unordered(self):
        moved_points = [[0.0, 0.0], [1.0, 0.0], [1.0, 2.0], [0.0, 1.0]]
        left = make_snapshot(UNIT_QUAD_POINTS, [[0, 1, 2, 3]], cell_data={"q": [[1.5]]})
        right = make_snapshot(moved_points, [[0, 1, 2, 3]], cell_data={"q": [[1.5]]})
        self.assertIsNotNone(self.compare(left, right, unordered=True))

    def test_rotated_connectivity_matches_unordered_at_tiny_scale(self):
        # Cells far smaller than any fixed rounding quantum: The canonical vertex order and
        # the location matching must work from the cell size, not from an absolute scale.
        width = 1.0e-10
        points = [[0.0, 0.0], [width, 0.0], [width, width], [0.0, width]]
        node_values = [1.0, 2.0, 3.0, 4.0]
        left = make_snapshot(points, [[0, 1, 2, 3]], node_data={"q": node_values})
        right = make_snapshot(points, [[1, 2, 3, 0]], node_data={"q": node_values})
        self.assertIsNone(self.compare(left, right, unordered=True))

    def test_rotated_connectivity_matches_unordered_for_anisotropic_cell(self):
        # A 1 x 0.1 cell: The canonical vertex order quantises each axis by the
        # cell's own extent along that axis; a single per-cell quantum would
        # collapse the thin axis and falsely reject rotated connectivity.
        points = [[0.0, 0.0], [1.0, 0.0], [1.0, 0.1], [0.0, 0.1]]
        node_values = [1.0, 2.0, 3.0, 4.0]
        left = make_snapshot(points, [[0, 1, 2, 3]], node_data={"q": node_values})
        right = make_snapshot(points, [[1, 2, 3, 0]], node_data={"q": node_values})
        self.assertIsNone(self.compare(left, right, unordered=True))

    def test_reordered_cells_match_unordered(self):
        two_cell_points = [
            [0.0, 0.0],
            [1.0, 0.0],
            [2.0, 0.0],
            [0.0, 1.0],
            [1.0, 1.0],
            [2.0, 1.0],
        ]
        left = make_snapshot(
            two_cell_points,
            [[0, 1, 4, 3], [1, 2, 5, 4]],
            cell_data={"q": [[1.0], [2.0]]},
        )
        right = make_snapshot(
            two_cell_points,
            [[1, 2, 5, 4], [0, 1, 4, 3]],
            cell_data={"q": [[2.0], [1.0]]},
        )
        self.assertIsNone(self.compare(left, right, unordered=True))

    def test_rounding_noise_on_a_coincident_coordinate_matches_unordered(self):
        # Two vertices of a quad carry the same coordinate analytically, but different writers
        # store it with one-ulp differences. The tie in the canonical sort must not break on
        # that noise, or the same cell compares against a permuted vertex list.
        base = 0.888888888888888888
        noisy = np.nextafter(base, 0.0)
        left = make_snapshot(
            [[0.0, base], [0.01, base], [0.01, base + 0.01], [0.0, base + 0.01]],
            [[0, 1, 2, 3]],
            cell_data={"q": [[1.0]]},
        )
        right = make_snapshot(
            [[0.0, noisy], [0.01, base], [0.01, base + 0.01], [0.0, base + 0.01]],
            [[1, 2, 3, 0]],
            cell_data={"q": [[1.0]]},
        )
        self.assertIsNone(
            compare_snapshots(left, right, 0, 0.0, 1.0e-12, None, True, ["Subdomain"])
        )


if __name__ == "__main__":
    unittest.main()
