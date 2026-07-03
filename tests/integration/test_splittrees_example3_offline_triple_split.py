import os
import sys

import pytest

# Ensure the loadbalancing API is importable when running pytest from repo root
sys.path.insert(0, os.path.abspath("src/toolbox/loadbalancing/api"))

import strategies.SplitTrees as split_strategy
from tree_builders import build_visualise_tree_split_example3_2d


# ---------------------------------------------------------------------------
# Stage I integration test
# ---------------------------------------------------------------------------


@pytest.mark.integration
def test_example3_stage1_bfs_finest_first_leaf_distribution():
    """
    Integration test for the example shown in visualise_tree_split_example3.png.

    The final tree contains 10 leaves arranged as a single refined branch:
    - 4 deepest leaves on the left-most branch
    - 3 middle-level leaves on the same branch
    - 3 top-level leaves directly under the root

    With 3 subtrees and bfs_finest_first traversal, Stage I should distribute
    the leaves as [4, 3, 3].
    """
    tree = build_visualise_tree_split_example3_2d()

    split_strategy.split_tree(
        peano_tree=tree,
        number_of_subtrees=3,
        traversal="bfs_finest_first",
        enable_propagate_bottom_up=False,
        enable_optimize_hierarchy=False,
        enable_extract_split_instructions=False,
    )

    # Root and internal nodes remain in subtree 0 during Stage I.
    assert tree._tree_arrays[0][0][0] == (1, 0), "Root must stay subtree 0"
    assert tree._tree_arrays[1][0][0] == (1, 0), "Level-1 internal node must stay subtree 0"
    assert tree._tree_arrays[2][0][0] == (1, 0), "Level-2 internal node must stay subtree 0"

    # Finest-level leaves of the refined branch go to subtree 0.
    for coords in ((0, 0), (0, 2), (2, 0), (2, 2)):
        assert tree._tree_arrays[3][coords[0]][coords[1]] == (1, 0), (
            f"Deep leaf {coords} must be subtree 0"
        )

    # Middle-level leaves go to subtree 1.
    for coords in ((0, 2), (2, 0), (2, 2)):
        assert tree._tree_arrays[2][coords[0]][coords[1]] == (1, 1), (
            f"Middle leaf {coords} must be subtree 1"
        )

    # Top-level leaves go to subtree 2.
    for coords in ((0, 2), (2, 0), (2, 2)):
        assert tree._tree_arrays[1][coords[0]][coords[1]] == (1, 2), (
            f"Top-level leaf {coords} must be subtree 2"
        )

    tree.update_split_pattern()
    assert list(tree._split_pattern) == [4.0, 3.0, 3.0], (
        f"Expected [4,3,3] leaf distribution, got {list(tree._split_pattern)}"
    )


# ---------------------------------------------------------------------------
# Stage I + II + III integration test
# ---------------------------------------------------------------------------


@pytest.mark.integration
def test_example3_stage3_promotes_root_and_level1_node():
    """
    Integration test for the full staged pipeline on example 3.

    Stage II should compact the deepest branch onto Level 2.

    For this sparse example, Stage III keeps the upper mixed parents unchanged
    (their child set is incomplete), while preserving the same leaf split.
    """
    tree_stage2 = build_visualise_tree_split_example3_2d()
    split_strategy.split_tree(
        peano_tree=tree_stage2,
        number_of_subtrees=3,
        traversal="bfs_finest_first",
        enable_propagate_bottom_up=True,
        enable_optimize_hierarchy=False,
        enable_extract_split_instructions=False,
    )

    tree_stage3 = build_visualise_tree_split_example3_2d()
    split_strategy.split_tree(
        peano_tree=tree_stage3,
        number_of_subtrees=3,
        traversal="bfs_finest_first",
        enable_propagate_bottom_up=True,
        enable_optimize_hierarchy=True,
        enable_extract_split_instructions=False,
    )

    # Stage II only: the deepest refined node is compacted, but the upper
    # topology is still unresolved.
    assert tree_stage2._tree_arrays[0][0][0] == (1, 0), "Root must stay 0 after Stage II only"
    assert tree_stage2._tree_arrays[1][0][0] == (1, 0), "Level-1 internal node must stay 0 after Stage II only"
    assert tree_stage2._tree_arrays[2][0][0] == (1, 0), "Level-2 internal node must stay 0 after Stage II only"

    # Stage III: for this sparse case, upper mixed parents stay unchanged.
    assert tree_stage3._tree_arrays[0][0][0] == (1, 0), "Root must remain subtree 0"
    assert tree_stage3._tree_arrays[1][0][0] == (1, 0), "Level-1 internal node must remain subtree 0"
    assert tree_stage3._tree_arrays[2][0][0] == (1, 0), "Level-2 internal node stays subtree 0"

    # The leaf mapping remains the same 4-3-3 split.
    for coords in ((0, 0), (0, 2), (2, 0), (2, 2)):
        assert tree_stage3._tree_arrays[3][coords[0]][coords[1]] == (1, 0)
    for coords in ((0, 2), (2, 0), (2, 2)):
        assert tree_stage3._tree_arrays[2][coords[0]][coords[1]] == (1, 1)
    for coords in ((0, 2), (2, 0), (2, 2)):
        assert tree_stage3._tree_arrays[1][coords[0]][coords[1]] == (1, 2)

    tree_stage3.update_split_pattern()
    assert list(tree_stage3._split_pattern) == [4.0, 3.0, 3.0]


@pytest.mark.integration
def test_example3_stage3_can_promote_sparse_mixed_parents_when_enabled():
    """
    With relaxed Stage III gating, sparse mixed parents in example 3 can be
    promoted without requiring a complete child set.
    """
    tree_stage3_relaxed = build_visualise_tree_split_example3_2d()
    split_strategy.split_tree(
        peano_tree=tree_stage3_relaxed,
        number_of_subtrees=3,
        traversal="bfs_finest_first",
        enable_propagate_bottom_up=True,
        enable_optimize_hierarchy=True,
        require_all_children_for_hierarchy_optimization=False,
        enable_extract_split_instructions=False,
    )

    # Relaxed Stage III promotes sparse mixed parents along the branch.
    assert tree_stage3_relaxed._tree_arrays[0][0][0] == (1, 2), "Root should be promoted to subtree 2"
    assert tree_stage3_relaxed._tree_arrays[1][0][0] == (1, 1), "Level-1 internal node should be promoted to subtree 1"
    assert tree_stage3_relaxed._tree_arrays[2][0][0] == (1, 0), "Level-2 internal node remains subtree 0"

    # Leaf split remains unchanged.
    tree_stage3_relaxed.update_split_pattern()
    assert list(tree_stage3_relaxed._split_pattern) == [4.0, 3.0, 3.0]
