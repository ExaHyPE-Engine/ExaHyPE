import os
import sys

# Ensure the loadbalancing API is importable when running pytest from repo root
sys.path.insert(0, os.path.abspath("src/toolbox/loadbalancing/api"))

from Input.RegularGridGenerator import RegularGridGenerator
import Strategies.SplitTrees as split_strategy


def test_bottom_up_propagation_promotes_parent_if_all_children_same_subtree_2d():
    """
    Stage II ("Bottom-Up Propagation") invariant test for
    [`Strategies.SplitTrees._propagate_bottom_up`](src/toolbox/loadbalancing/api/Strategies/SplitTrees.py).

    If all existing children of a parent belong to the same subtree, then after
    Stage II the parent has to be assigned to that subtree as well.
    """
    # Arrange: 2D tree with 2 levels: L0 = 3x3, L1 = 9x9 (children)
    tree = RegularGridGenerator(name="pytest-tree", max_level=2, dimension=2)

    parent_level = 0
    child_level = 1

    # Choose a parent at level 0
    parent_a, parent_b = 1, 1
    assert tree._tree_arrays[parent_level][parent_a][parent_b] == (1, 0)

    # Make *all* of its 3x3 children belong to subtree 1
    for ca in range(3 * parent_a, 3 * parent_a + 3):
        for cb in range(3 * parent_b, 3 * parent_b + 3):
            tree._tree_arrays[child_level][ca][cb] = (1, 1)

    # Create a second parent whose children are mixed across subtrees -> should NOT promote
    mixed_parent_a, mixed_parent_b = 0, 0
    assert tree._tree_arrays[parent_level][mixed_parent_a][mixed_parent_b] == (1, 0)

    tree._tree_arrays[child_level][0][0] = (1, 1)
    tree._tree_arrays[child_level][0][1] = (1, 1)
    tree._tree_arrays[child_level][0][2] = (1, 0)  # keep mixed

    # Act: run Stage II only
    split_strategy._propagate_bottom_up(tree)

    # Assert: uniform-children parent is promoted
    assert tree._tree_arrays[parent_level][parent_a][parent_b] == (1, 1)

    # Assert: mixed-children parent remains unchanged
    assert tree._tree_arrays[parent_level][mixed_parent_a][mixed_parent_b] == (1, 0)