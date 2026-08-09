import itertools
import pytest

from toolbox.loadbalancing.api.input.RegularGridGenerator import RegularGridGenerator
import toolbox.loadbalancing.api.strategies.SplitTrees as split_strategy


@pytest.mark.parametrize(
    "dimensions,target_subtree,parent_coords,mixed_parent_coords",
    [
        (2, 1, (1, 1), (0, 0)),
        (3, 2, (1, 1, 1), (0, 0, 0)),
    ],
)
def test_bottom_up_propagation_promotes_parent_if_all_children_same_subtree(
    dimensions, target_subtree, parent_coords, mixed_parent_coords
):
    """
    Stage II ("Bottom-Up Propagation") invariant test in 2D and 3D.

    If all existing children of a parent belong to the same subtree, then after
    Stage II the parent has to be assigned to that subtree as well.
    """
    # Arrange: tree with 2 levels: L0 = 3^d, L1 = 9^d (children)
    tree = RegularGridGenerator(name=f"pytest-tree-{dimensions}d", max_level=2, dimensions=dimensions)

    parent_level = 0
    child_level = 1

    # Parent that should be promoted
    assert tree._tree_arrays[parent_level][parent_coords] == (1, 0)

    # Make all children of parent_coords belong to target_subtree
    ranges = [range(3 * coord, 3 * coord + 3) for coord in parent_coords]
    for child_coords in itertools.product(*ranges):
        tree._tree_arrays[child_level][child_coords] = (1, target_subtree)

    # Parent that should remain unchanged due to mixed children
    assert tree._tree_arrays[parent_level][mixed_parent_coords] == (1, 0)

    # Introduce mixed assignments under mixed_parent_coords:
    # some children in target_subtree, at least one child still in subtree 0.
    mixed_child_base = tuple(3 * coord for coord in mixed_parent_coords)
    first_child = mixed_child_base
    second_child = mixed_child_base[:-1] + (mixed_child_base[-1] + 1,)
    third_child = mixed_child_base[:-1] + (mixed_child_base[-1] + 2,)

    tree._tree_arrays[child_level][first_child] = (1, target_subtree)
    tree._tree_arrays[child_level][second_child] = (1, target_subtree)
    tree._tree_arrays[child_level][third_child] = (1, 0)

    # Act: run Stage II only
    split_strategy.propagate_bottom_up(tree)

    # Assert: uniform-children parent is promoted
    assert tree._tree_arrays[parent_level][parent_coords] == (1, target_subtree)

    # Assert: mixed-children parent remains unchanged
    assert tree._tree_arrays[parent_level][mixed_parent_coords] == (1, 0)