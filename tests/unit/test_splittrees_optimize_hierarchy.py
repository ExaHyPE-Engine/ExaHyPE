import os
import sys
import itertools
import pytest

# Ensure the loadbalancing API is importable when running pytest from repo root
sys.path.insert(0, os.path.abspath("src/toolbox/loadbalancing/api"))

from input.RegularGridGenerator import RegularGridGenerator
import Strategies.SplitTrees as split_strategy


def _child_coords(parent_coords):
    """Return all 3^d child coordinates for a parent coordinate tuple."""
    ranges = [range(3 * coord, 3 * coord + 3) for coord in parent_coords]
    return list(itertools.product(*ranges))


@pytest.mark.parametrize(
    "dimensions,parent_coords",
    [
        (2, (0, 0)),
        (3, (0, 0, 0)),
    ],
)
def test_optimize_hierarchy_promotes_parent_to_dominant_child_subtree(
    dimensions, parent_coords
):
    """
    Stage III invariant in 2D and 3D:
    If a parent has a full set of mixed children and a dominant subtree that
    differs from the parent subtree and improves split pressure, parent is promoted.
    """
    tree = RegularGridGenerator(
        name=f"pytest-tree-stage-iii-promote-{dimensions}d", max_level=2, dimensions=dimensions
    )

    parent_level = 0
    child_level = 1

    # Parent starts in subtree 0.
    assert tree._tree_arrays[parent_level][parent_coords] == (1, 0)

    children = _child_coords(parent_coords)
    total_children = len(children)
    dominant_count = total_children // 2 + 1
    parent_count = total_children - dominant_count - 1

    # Create mixed children where subtree 2 is dominant:
    # - dominant_count children in subtree 2
    # - parent_count children in subtree 0 (parent subtree)
    # - 1 child in subtree 1
    for coords in children[:dominant_count]:
        tree._tree_arrays[child_level][coords] = (1, 2)
    for coords in children[dominant_count:dominant_count + parent_count]:
        tree._tree_arrays[child_level][coords] = (1, 0)
    tree._tree_arrays[child_level][children[-1]] = (1, 1)

    # Act: Stage III only
    split_strategy._optimize_hierarchy(tree)

    # Parent should be promoted to dominant subtree 2.
    assert tree._tree_arrays[parent_level][parent_coords] == (1, 2)


@pytest.mark.parametrize(
    "dimensions,parent_coords",
    [
        (2, (1, 1)),
        (3, (1, 1, 1)),
    ],
)
def test_optimize_hierarchy_does_not_promote_when_move_would_be_backward(
    dimensions, parent_coords
):
    """
    Stage III safety invariant in 2D and 3D:
    Parent ownership is never moved backward to a smaller subtree id.
    """
    tree = RegularGridGenerator(
        name=f"pytest-tree-stage-iii-backward-{dimensions}d", max_level=2, dimensions=dimensions
    )

    parent_level = 0
    child_level = 1

    # Set parent to subtree 2.
    tree._tree_arrays[parent_level][parent_coords] = (1, 2)

    children = _child_coords(parent_coords)
    total_children = len(children)
    dominant_count = total_children // 2 + 1
    parent_count = total_children - dominant_count - 1

    # Mixed children with dominant subtree 0 (backward relative to parent subtree 2).
    for coords in children[:dominant_count]:
        tree._tree_arrays[child_level][coords] = (1, 0)
    for coords in children[dominant_count:dominant_count + parent_count]:
        tree._tree_arrays[child_level][coords] = (1, 2)
    tree._tree_arrays[child_level][children[-1]] = (1, 1)

    # Act: Stage III only
    split_strategy._optimize_hierarchy(tree)

    # Parent should remain unchanged due to backward-move guard.
    assert tree._tree_arrays[parent_level][parent_coords] == (1, 2)


@pytest.mark.parametrize(
    "dimensions,parent_coords",
    [
        (2, (2, 2)),
        (3, (2, 2, 2)),
    ],
)
def test_optimize_hierarchy_does_not_promote_when_children_are_incomplete(
    dimensions, parent_coords
):
    """
    Stage III safety invariant in 2D and 3D:
    No promotion occurs unless all 3^d children of a parent exist.
    """
    tree = RegularGridGenerator(
        name=f"pytest-tree-stage-iii-incomplete-{dimensions}d", max_level=2, dimensions=dimensions
    )

    parent_level = 0
    child_level = 1

    assert tree._tree_arrays[parent_level][parent_coords] == (1, 0)

    children = _child_coords(parent_coords)
    total_children = len(children)
    dominant_count = total_children // 2 + 1
    parent_count = total_children - dominant_count - 1

    # Build a would-be promotable mixed pattern...
    for coords in children[:dominant_count]:
        tree._tree_arrays[child_level][coords] = (1, 2)
    for coords in children[dominant_count:dominant_count + parent_count]:
        tree._tree_arrays[child_level][coords] = (1, 0)
    tree._tree_arrays[child_level][children[-1]] = (1, 1)

    # ...then remove one child (weight=0), invalidating full-child-set precondition.
    tree._tree_arrays[child_level][children[0]] = (0, 0)

    # Act: Stage III only
    split_strategy._optimize_hierarchy(tree)

    # Parent must remain unchanged.
    assert tree._tree_arrays[parent_level][parent_coords] == (1, 0)


@pytest.mark.parametrize("dimensions", [2, 3])
def test_optimize_hierarchy_preserves_node_existence_weights(dimensions):
    """
    Stage III invariant in 2D and 3D:
    Optimisation modifies only subtree ids, never node existence flags (weights).
    """
    tree = RegularGridGenerator(
        name=f"pytest-tree-stage-iii-weights-{dimensions}d", max_level=2, dimensions=dimensions
    )

    # Introduce mixed assignments to trigger potential promotions.
    parent_coords = (0, 0) if dimensions == 2 else (0, 0, 0)
    child_level = 1
    children = _child_coords(parent_coords)
    total_children = len(children)
    dominant_count = total_children // 2 + 1
    parent_count = total_children - dominant_count - 1
    for coords in children[:dominant_count]:
        tree._tree_arrays[child_level][coords] = (1, 2)
    for coords in children[dominant_count:dominant_count + parent_count]:
        tree._tree_arrays[child_level][coords] = (1, 0)
    tree._tree_arrays[child_level][children[-1]] = (1, 1)

    # Capture all weights before Stage III.
    weights_before = {}
    for level in range(tree._max_level):
        size = 3 ** (level + 1)
        if dimensions == 2:
            for a in range(size):
                for b in range(size):
                    weight, _ = tree._tree_arrays[level][a][b]
                    weights_before[(level, a, b)] = weight
        else:
            for a in range(size):
                for b in range(size):
                    for c in range(size):
                        weight, _ = tree._tree_arrays[level][a][b][c]
                        weights_before[(level, a, b, c)] = weight

    # Act: Stage III only
    split_strategy._optimize_hierarchy(tree)

    # Assert all weights unchanged.
    for coords, weight_before in weights_before.items():
        if dimensions == 2:
            level, a, b = coords
            weight_after, _ = tree._tree_arrays[level][a][b]
        else:
            level, a, b, c = coords
            weight_after, _ = tree._tree_arrays[level][a][b][c]

        assert weight_after == weight_before, (
            f"Node {coords} weight changed from {weight_before} to {weight_after}"
        )
