import os
import sys
import pytest

# Ensure the loadbalancing API is importable when running pytest from repo root
sys.path.insert(0, os.path.abspath("src/toolbox/loadbalancing/api"))

import Strategies.SplitTrees as split_strategy
from tree_builders import build_tree3_adaptive_2d, build_tree3_all_rows_2d


# ---------------------------------------------------------------------------
# Stage I integration test
# ---------------------------------------------------------------------------

@pytest.mark.integration
def test_tree3_stage1_bfs_finest_first_leaf_distribution():
    """
    Integration test for Stage I with traversal="bfs_finest_first".

    Exercises the tree described by tree3_stage1_fine_first_bfs.dot:
    - 23 leaves total (21 at the finest level + 2 childless level-1 leaves).
    - BFS finest-first processes level 2 leaves before level 1 leaves.
    - With 4 subtrees, the balanced distribution is [6, 6, 6, 5].

    Expected leaf assignments after Stage I
    (first-row positions at each level):
        Level 3:  (0,0)–(0,5)  → subtree 0   [L3A1*, L3A2*]
                  (0,9)–(0,14) → subtree 1   [L3B1*, L3B2*]
                  (0,18)–(0,23)→ subtree 2   [L3C1*, L3C2*]
                  (0,24)–(0,26)→ subtree 3   [L3C3*]
        Level 2:  (0,2)        → subtree 3   [L2A3, childless leaf]
                  (0,5)        → subtree 3   [L2B3, childless leaf]
    Non-leaf nodes (all levels) remain subtree 0.
    """
    tree = build_tree3_adaptive_2d()

    split_strategy.split_tree(
        peano_tree=tree,
        number_of_subtrees=4,
        traversal="bfs_finest_first",
        enable_propagate_bottom_up=False,
        enable_optimize_hierarchy=False,
        enable_extract_split_instructions=False,
    )

    # --- Level 0: single root unchanged (non-leaf, no redistribution) ---
    assert tree._tree_arrays[0][0][0] == (1, 0), "Root must stay subtree 0 (non-leaf)"

    # --- Level 1: L1A, L1B, L1C unchanged (non-leaf, no redistribution) ---
    assert tree._tree_arrays[1][0][0] == (1, 0), "L1A must stay subtree 0 (non-leaf)"
    assert tree._tree_arrays[1][0][1] == (1, 0), "L1B must stay subtree 0 (non-leaf)"
    assert tree._tree_arrays[1][0][2] == (1, 0), "L1C must stay subtree 0 (non-leaf)"

    # --- Level 2: only the childless leaves (L2A3, L2B3) are redistributed ---
    # Non-leaves (L2A1, L2A2, L2B1, L2B2, L2C1, L2C2, L2C3) stay subtree 0
    for col in (0, 1, 3, 4, 6, 7, 8):
        assert tree._tree_arrays[2][0][col] == (1, 0), (
            f"Non-leaf L2 node at (0,{col}) must not be redistributed by Stage I"
        )
    # Childless leaves → subtree 3 (last bucket in [6,6,6,5] distribution)
    assert tree._tree_arrays[2][0][2] == (1, 3), "L2A3 (childless leaf) must be subtree 3"
    assert tree._tree_arrays[2][0][5] == (1, 3), "L2B3 (childless leaf) must be subtree 3"

    # --- Level 3: 21 finest-level leaves in BFS order ---
    # Subtree 0: first 6 (L3A1A/B/C + L3A2A/B/C)
    for col in range(0, 6):
        assert tree._tree_arrays[3][0][col] == (1, 0), f"L3A leaf (0,{col}) must be subtree 0"
    # Subtree 1: next 6 (L3B1A/B/C + L3B2A/B/C)
    for col in range(9, 15):
        assert tree._tree_arrays[3][0][col] == (1, 1), f"L3B leaf (0,{col}) must be subtree 1"
    # Subtree 2: next 6 (L3C1A/B/C + L3C2A/B/C)
    for col in range(18, 24):
        assert tree._tree_arrays[3][0][col] == (1, 2), f"L3C12 leaf (0,{col}) must be subtree 2"
    # Subtree 3: last 3 at finest level (L3C3A/B/C); the final 2 count comes from level 1
    for col in range(24, 27):
        assert tree._tree_arrays[3][0][col] == (1, 3), f"L3C3 leaf (0,{col}) must be subtree 3"

    # Gap cells (children of childless L2 nodes) must remain weight=0
    for col in list(range(6, 9)) + list(range(15, 18)):
        assert tree._tree_arrays[3][0][col] == (0, 0), (
            f"Level-2 cell (0,{col}) under a childless L2 node must stay non-existent"
        )

    # Verify the balanced leaf-count summary [6, 6, 6, 5]
    tree.update_split_pattern()
    assert list(tree._split_pattern) == [6.0, 6.0, 6.0, 5.0], (
        f"Expected [6,6,6,5] leaf distribution, got {list(tree._split_pattern)}"
    )


# ---------------------------------------------------------------------------
# Stage I + II integration test
# ---------------------------------------------------------------------------

@pytest.mark.integration
def test_tree3_stage2_bottom_up_propagation():
    """
    Integration test for Stage I + Stage II with traversal="bfs_finest_first".

    Exercises the tree described by tree3_stage2_fine_first_bfs.dot.
    After Stage II (bottom-up propagation), parents whose *existing* children
    all share the same subtree id are promoted to that subtree.

    Changes introduced by Stage II on top of Stage I:
        Level 1:
          L2B1=(0,3): all children → subtree 1  →  promoted from 0 to 1
          L2B2=(0,4): all children → subtree 1  →  promoted from 0 to 1
          L2C1=(0,6): all children → subtree 2  →  promoted from 0 to 2
          L2C2=(0,7): all children → subtree 2  →  promoted from 0 to 2
          L2C3=(0,8): all children → subtree 3  →  promoted from 0 to 3
          L2A1=(0,0): all children → subtree 0  →  stays 0 (no change)
          L2A2=(0,1): all children → subtree 0  →  stays 0 (no change)
          L2A3=(0,2): no existing children       →  stays subtree 3 (Stage I result)
          L2B3=(0,5): no existing children       →  stays subtree 3 (Stage I result)

                Level 1:
          L1A=(0,0): children {0,3} (L2A1=0, L2A2=0, L2A3=3) → mixed → stays 0
          L1B=(0,1): children {1,3} (L2B1=1, L2B2=1, L2B3=3) → mixed → stays 0
          L1C=(0,2): children {2,3} (L2C1=2, L2C2=2, L2C3=3) → mixed → stays 0

                Level 0:
                    Root=(0,0): children {0} (L1A=0, L1B=0, L1C=0) at this stage → stays 0
    """
    tree = build_tree3_adaptive_2d()

    split_strategy.split_tree(
        peano_tree=tree,
        number_of_subtrees=4,
        traversal="bfs_finest_first",
        enable_propagate_bottom_up=True,
        enable_optimize_hierarchy=False,
        enable_extract_split_instructions=False,
    )

    # --- Level 0: root stays subtree 0 (only one existing child-subtree) ---
    assert tree._tree_arrays[0][0][0] == (1, 0), "Root must stay 0"

    # --- Level 1: all L1 nodes stay subtree 0 (mixed L2 children) ---
    assert tree._tree_arrays[1][0][0] == (1, 0), "L1A must stay 0 (mixed children {0,3})"
    assert tree._tree_arrays[1][0][1] == (1, 0), "L1B must stay 0 (mixed children {1,3})"
    assert tree._tree_arrays[1][0][2] == (1, 0), "L1C must stay 0 (mixed children {2,3})"

    # --- Level 2: promoted and unchanged nodes ---
    assert tree._tree_arrays[2][0][0] == (1, 0), "L2A1 stays 0 (uniform subtree 0 children)"
    assert tree._tree_arrays[2][0][1] == (1, 0), "L2A2 stays 0 (uniform subtree 0 children)"
    assert tree._tree_arrays[2][0][2] == (1, 3), "L2A3 stays 3 (childless, set by Stage I)"
    assert tree._tree_arrays[2][0][3] == (1, 1), "L2B1 promoted to 1 (all children subtree 1)"
    assert tree._tree_arrays[2][0][4] == (1, 1), "L2B2 promoted to 1 (all children subtree 1)"
    assert tree._tree_arrays[2][0][5] == (1, 3), "L2B3 stays 3 (childless, set by Stage I)"
    assert tree._tree_arrays[2][0][6] == (1, 2), "L2C1 promoted to 2 (all children subtree 2)"
    assert tree._tree_arrays[2][0][7] == (1, 2), "L2C2 promoted to 2 (all children subtree 2)"
    assert tree._tree_arrays[2][0][8] == (1, 3), "L2C3 promoted to 3 (all children subtree 3)"

    # --- Level 3: unchanged from Stage I (propagation only moves up) ---
    for col in range(0, 6):
        assert tree._tree_arrays[3][0][col] == (1, 0), f"L3A leaf (0,{col}) must stay subtree 0"
    for col in range(9, 15):
        assert tree._tree_arrays[3][0][col] == (1, 1), f"L3B leaf (0,{col}) must stay subtree 1"
    for col in range(18, 24):
        assert tree._tree_arrays[3][0][col] == (1, 2), f"L3C12 leaf (0,{col}) must stay subtree 2"
    for col in range(24, 27):
        assert tree._tree_arrays[3][0][col] == (1, 3), f"L3C3 leaf (0,{col}) must stay subtree 3"


# ---------------------------------------------------------------------------
# Stage I + II + III integration test
# ---------------------------------------------------------------------------

@pytest.mark.integration
def test_tree3_stage3_no_root_promotion_with_single_root_first_row_embedding():
    """
    Integration test for Stage I + Stage II + Stage III (full pipeline).

    Exercises the tree described by tree3_stage3_fine_first_bfs.dot using
    a single-root Level 0 representation of the tree3 structure.

        Even with a single root at Level 0, this first-row-only 2D embedding does
        not satisfy Stage III's promotion precondition at the root:
        the root has only 3 existing Level-1 children, while Stage III in 2D
        requires all 3^2 = 9 children to exist.

        Therefore Stage III should not change the root assignment.
    """
    # Baseline: Stage I + II (Stage III disabled)
    tree_no_stage3 = build_tree3_adaptive_2d()
    split_strategy.split_tree(
        peano_tree=tree_no_stage3,
        number_of_subtrees=4,
        traversal="bfs_finest_first",
        enable_propagate_bottom_up=True,
        enable_optimize_hierarchy=False,
        enable_extract_split_instructions=False,
    )

    # Full pipeline: Stage I + II + III
    tree_with_stage3 = build_tree3_adaptive_2d()
    split_strategy.split_tree(
        peano_tree=tree_with_stage3,
        number_of_subtrees=4,
        traversal="bfs_finest_first",
        enable_propagate_bottom_up=True,
        enable_optimize_hierarchy=True,
        enable_extract_split_instructions=False,
    )

    # Stage II baseline: root remains 0.
    assert tree_no_stage3._tree_arrays[0][0][0] == (1, 0), "Root must be 0 after Stage II only"

    # Stage III should not promote root, because only 3/9 children exist.
    assert tree_with_stage3._tree_arrays[0][0][0] == (1, 0), (
        "Root must stay 0: Stage III requires all 9 children in 2D"
    )

    # L1 nodes remain as Stage II leaves from the root's perspective.
    assert tree_with_stage3._tree_arrays[1][0][0] == (1, 0), "L1A stays 0"
    assert tree_with_stage3._tree_arrays[1][0][1] == (1, 0), "L1B stays 0"
    assert tree_with_stage3._tree_arrays[1][0][2] == (1, 0), "L1C stays 0"


# ---------------------------------------------------------------------------
# All-rows representation variant (single-root Level 0)
# ---------------------------------------------------------------------------

@pytest.mark.integration
def test_tree3_all_rows_stage1_leaf_distribution():
    """
    Stage I test for the all-rows representation variant.

    In this all-rows recursive expansion variant, the represented tree is much
    denser than the first-row embedding. For 4 subtrees, Stage I
    deterministically distributes 585 leaves as [147, 146, 146, 146].
    """
    tree = build_tree3_all_rows_2d()

    split_strategy.split_tree(
        peano_tree=tree,
        number_of_subtrees=4,
        traversal="bfs_finest_first",
        enable_propagate_bottom_up=False,
        enable_optimize_hierarchy=False,
        enable_extract_split_instructions=False,
    )

    # Root and level-1 non-leaf nodes remain untouched in Stage I
    assert tree._tree_arrays[0][0][0] == (1, 0)
    for a in range(3):
        for b in range(3):
            assert tree._tree_arrays[1][a][b] == (1, 0)

    # Childless L2 stripes (A3 and B3 columns) should fall into the last subtree bucket
    # because finest-level leaves are assigned first with bfs_finest_first traversal.
    for a in range(9):
        for b in (2, 5):
            assert tree._tree_arrays[2][a][b] == (1, 3)

    tree.update_split_pattern()
    assert list(tree._split_pattern) == [147.0, 146.0, 146.0, 146.0]


@pytest.mark.integration
def test_tree3_all_rows_stage2_bottom_up_propagation():
    """
    Stage II test for the all-rows representation variant.

    Stage II keeps Root and Level-1 nodes in subtree 0 for this dense all-rows
    representation, but propagates mixed ownership into Level 2.

    Deterministic Level-2 pattern after Stage II:
    - columns 2 and 5 remain uniformly subtree 3 (childless bands)
    - all other Level-2 columns contain mixed ownership {0,1,2,3}
    """
    tree = build_tree3_all_rows_2d()

    split_strategy.split_tree(
        peano_tree=tree,
        number_of_subtrees=4,
        traversal="bfs_finest_first",
        enable_propagate_bottom_up=True,
        enable_optimize_hierarchy=False,
        enable_extract_split_instructions=False,
    )

    # Root stays 0 after Stage II; all L1 nodes remain 0 due to mixed L2 children.
    assert tree._tree_arrays[0][0][0] == (1, 0)
    for a in range(3):
        for b in range(3):
            assert tree._tree_arrays[1][a][b] == (1, 0)

    for b in range(9):
        subtree_set = {
            tree._tree_arrays[2][a][b][1]
            for a in range(9)
            if tree._tree_arrays[2][a][b][0] == 1
        }
        if b in (2, 5):
            assert subtree_set == {3}
        else:
            assert subtree_set == {0, 1, 2, 3}


@pytest.mark.integration
def test_tree3_all_rows_stage3_promotes_level1_not_root():
    """
    Stage III test for the all-rows representation variant.

    With all-row expansion, Stage III updates Level-1 ownership while keeping
    the root unchanged.

    Deterministic Level-1 outcome after Stage III:
    - row 0: subtree 0 across all three columns
    - row 1: subtree 1 across all three columns
    - row 2: subtree 3 across all three columns

    Root remains subtree 0.
    """
    tree = build_tree3_all_rows_2d()

    split_strategy.split_tree(
        peano_tree=tree,
        number_of_subtrees=4,
        traversal="bfs_finest_first",
        enable_propagate_bottom_up=True,
        enable_optimize_hierarchy=True,
        enable_extract_split_instructions=False,
    )

    # Root stays 0
    assert tree._tree_arrays[0][0][0] == (1, 0)

    # Level-1 promotions after Stage III
    for b in range(3):
        assert tree._tree_arrays[1][0][b] == (1, 0)
        assert tree._tree_arrays[1][1][b] == (1, 1)
        assert tree._tree_arrays[1][2][b] == (1, 3)

    # Stage III changes only non-leaf ownership; leaf split remains unchanged.
    tree.update_split_pattern()
    assert list(tree._split_pattern) == [147.0, 146.0, 146.0, 146.0]
