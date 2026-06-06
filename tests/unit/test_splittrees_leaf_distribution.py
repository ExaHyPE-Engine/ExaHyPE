import os
import sys
import itertools
import pytest

# Ensure the loadbalancing API is importable when running pytest from repo root
sys.path.insert(0, os.path.abspath("src/toolbox/loadbalancing/api"))

from input.RegularGridGenerator import RegularGridGenerator
import Strategies.SplitTrees as split_strategy


@pytest.mark.parametrize(
    "dimensions,number_of_subtrees",
    [
        (2, 1),
        (2, 3),
        (3, 1),
        (3, 3),
    ],
)
def test_leaf_distribution_balanced_across_subtrees(dimensions, number_of_subtrees):
    """
    Stage I ("Leaf Distribution") invariant test in 2D and 3D.

    Verify that leaves from subtree 0 are distributed as evenly as possible
    across `number_of_subtrees` subtrees.
    """
    # Arrange: tree with 2 levels; all cells initially in subtree 0
    tree = RegularGridGenerator(
        name=f"pytest-tree-stage-i-{dimensions}d", max_level=2, dimensions=dimensions
    )

    # Collect all leaf nodes before Stage I
    leaf_count_before = tree._leaf_count

    # Act: run Stage I only (BFS traversal, default)
    has_leaves, moved_nodes = split_strategy._distribute_leaves_bfs(tree, number_of_subtrees)

    # Assert: leaves were found and redistribution occurred
    assert has_leaves
    assert sum(moved_nodes) == leaf_count_before
    assert len(moved_nodes) == number_of_subtrees

    # Assert: distribution is balanced (within 1 leaf per subtree)
    max_leaves = max(moved_nodes)
    min_leaves = min(moved_nodes)
    assert max_leaves - min_leaves <= 1, (
        f"Leaf distribution unbalanced: max={max_leaves}, min={min_leaves}, "
        f"distribution={moved_nodes}"
    )

    # Assert: no subtree is empty if possible
    if leaf_count_before >= number_of_subtrees:
        assert all(count > 0 for count in moved_nodes), (
            f"Some subtrees are empty: {moved_nodes}"
        )


@pytest.mark.parametrize(
    "dimensions",
    [2, 3],
)
def test_leaf_distribution_does_not_modify_non_leaf_nodes(dimensions):
    """
    Stage I invariant: non-leaf nodes (internal nodes with children) must NOT
    be modified by Stage I leaf distribution.
    """
    # Arrange: tree with 3 levels; all cells initially in subtree 0
    tree = RegularGridGenerator(
        name=f"pytest-tree-nonleaf-check-{dimensions}d", max_level=3, dimensions=dimensions
    )

    # Capture non-leaf node assignments before Stage I
    non_leaf_assignments_before = {}
    for level in range(tree._max_level - 1):  # Skip finest level (only leaves)
        size = 3 ** (level + 1)
        if dimensions == 2:
            for a in range(size):
                for b in range(size):
                    node = tree._tree_arrays[level][a][b]
                    if node[0] == 1:  # Node exists
                        # Check if it has children
                        child_a, child_b = 3 * a, 3 * b
                        if (child_a < 3 ** (level + 2) and child_b < 3 ** (level + 2) and
                            tree._tree_arrays[level + 1][child_a][child_b][0] == 1):
                            # This is a non-leaf node
                            non_leaf_assignments_before[(level, a, b)] = node[1]

        elif dimensions == 3:
            for a in range(size):
                for b in range(size):
                    for c in range(size):
                        node = tree._tree_arrays[level][a][b][c]
                        if node[0] == 1:  # Node exists
                            # Check if it has children
                            child_a, child_b, child_c = 3 * a, 3 * b, 3 * c
                            if (child_a < 3 ** (level + 2) and child_b < 3 ** (level + 2) and
                                child_c < 3 ** (level + 2) and
                                tree._tree_arrays[level + 1][child_a][child_b][child_c][0] == 1):
                                # This is a non-leaf node
                                non_leaf_assignments_before[(level, a, b, c)] = node[1]

    # Act: run Stage I
    split_strategy._distribute_leaves_bfs(tree, number_of_subtrees=2)

    # Assert: all non-leaf nodes remain unchanged
    for level_coords, subtree_before in non_leaf_assignments_before.items():
        if dimensions == 2:
            level, a, b = level_coords
            node_after = tree._tree_arrays[level][a][b]
        else:
            level, a, b, c = level_coords
            node_after = tree._tree_arrays[level][a][b][c]

        assert node_after[1] == subtree_before, (
            f"Non-leaf node {level_coords} changed from subtree {subtree_before} "
            f"to subtree {node_after[1]} during Stage I"
        )


@pytest.mark.parametrize(
    "dimensions",
    [2, 3],
)
def test_leaf_distribution_only_redistributes_subtree_0(dimensions):
    """
    Stage I invariant: only leaves in subtree 0 are redistributed. Leaves already
    in other subtrees must NOT be modified.
    """
    # Arrange: tree with 2 levels
    tree = RegularGridGenerator(
        name=f"pytest-tree-subtree0-only-{dimensions}d", max_level=2, dimensions=dimensions
    )

    # Manually assign some leaves to subtree 1 (mark a corner block)
    finest_level = tree._max_level - 1
    size = 3 ** (finest_level + 1)

    # Mark leaves in top-right corner as subtree 1
    corner_leaves_subtree_1 = []
    if dimensions == 2:
        for a in range(size // 2, size):
            for b in range(size // 2, size):
                tree._tree_arrays[finest_level][a][b] = (1, 1)
                corner_leaves_subtree_1.append((a, b))

    elif dimensions == 3:
        for a in range(size // 2, size):
            for b in range(size // 2, size):
                for c in range(size // 2, size):
                    tree._tree_arrays[finest_level][a][b][c] = (1, 1)
                    corner_leaves_subtree_1.append((a, b, c))

    # Act: run Stage I
    split_strategy._distribute_leaves_bfs(tree, number_of_subtrees=1)

    # Assert: leaves in subtree 1 remain in subtree 1
    if dimensions == 2:
        for a, b in corner_leaves_subtree_1:
            node_after = tree._tree_arrays[finest_level][a][b]
            assert node_after[1] == 1, (
                f"Leaf ({a},{b}) in subtree 1 was incorrectly modified to subtree {node_after[1]}"
            )

    elif dimensions == 3:
        for a, b, c in corner_leaves_subtree_1:
            node_after = tree._tree_arrays[finest_level][a][b][c]
            assert node_after[1] == 1, (
                f"Leaf ({a},{b},{c}) in subtree 1 was incorrectly modified to subtree {node_after[1]}"
            )


@pytest.mark.parametrize(
    "dimensions",
    [2, 3],
)
def test_leaf_distribution_preserves_node_existence_weights(dimensions):
    """
    Stage I invariant: node existence flags (weights) must NOT be modified.
    Stage I only changes subtree assignments, not node existence.
    """
    # Arrange: tree with 2 levels
    tree = RegularGridGenerator(
        name=f"pytest-tree-weights-{dimensions}d", max_level=2, dimensions=dimensions
    )

    # Capture all node weights before Stage I
    weights_before = {}
    for level in range(tree._max_level):
        size = 3 ** (level + 1)
        if dimensions == 2:
            for a in range(size):
                for b in range(size):
                    weight, subtree = tree._tree_arrays[level][a][b]
                    weights_before[(level, a, b)] = weight

        elif dimensions == 3:
            for a in range(size):
                for b in range(size):
                    for c in range(size):
                        weight, subtree = tree._tree_arrays[level][a][b][c]
                        weights_before[(level, a, b, c)] = weight

    # Act: run Stage I
    split_strategy._distribute_leaves_bfs(tree, number_of_subtrees=2)

    # Assert: all weights remain unchanged
    for level_coords, weight_before in weights_before.items():
        if dimensions == 2:
            level, a, b = level_coords
            weight_after, _ = tree._tree_arrays[level][a][b]
        else:
            level, a, b, c = level_coords
            weight_after, _ = tree._tree_arrays[level][a][b][c]

        assert weight_after == weight_before, (
            f"Node {level_coords} weight changed from {weight_before} to {weight_after}"
        )


@pytest.mark.parametrize(
    "dimensions",
    [2, 3],
)
def test_leaf_distribution_returns_correct_moved_nodes_count(dimensions):
    """
    Stage I invariant: the returned `moved_nodes` list must accurately reflect
    how many leaves were assigned to each subtree.
    """
    # Arrange: tree with 2 levels
    tree = RegularGridGenerator(
        name=f"pytest-tree-move-count-{dimensions}d", max_level=2, dimensions=dimensions
    )

    number_of_subtrees = 2
    finest_level = tree._max_level - 1

    # Act: run Stage I
    has_leaves, moved_nodes = split_strategy._distribute_leaves_bfs(tree, number_of_subtrees)

    # Assert: moved_nodes list length is correct
    assert len(moved_nodes) == number_of_subtrees

    # Assert: count leaves actually assigned to each subtree and verify
    size = 3 ** (finest_level + 1)
    actual_counts = [0] * (number_of_subtrees)

    if dimensions == 2:
        for a in range(size):
            for b in range(size):
                weight, subtree = tree._tree_arrays[finest_level][a][b]
                if weight == 1:  # Leaf exists
                    if subtree < number_of_subtrees:
                        actual_counts[subtree] += 1

    elif dimensions == 3:
        for a in range(size):
            for b in range(size):
                for c in range(size):
                    weight, subtree = tree._tree_arrays[finest_level][a][b][c]
                    if weight == 1:  # Leaf exists
                        if subtree < number_of_subtrees:
                            actual_counts[subtree] += 1

    # Compare reported counts with actual counts
    assert actual_counts == moved_nodes, (
        f"Reported moved_nodes {moved_nodes} does not match actual counts {actual_counts}"
    )
