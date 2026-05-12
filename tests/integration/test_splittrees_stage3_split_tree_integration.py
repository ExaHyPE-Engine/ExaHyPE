import os
import sys
import itertools
import pytest

# Ensure the loadbalancing API is importable when running pytest from repo root
sys.path.insert(0, os.path.abspath("src/toolbox/loadbalancing/api"))

from Input.RegularGridGenerator import RegularGridGenerator
import Strategies.SplitTrees as split_strategy


def _build_stage3_mixed_parent_case_2d():
    """Build a 2D tree where Stage III can promote a mixed parent."""
    tree = RegularGridGenerator(name="pytest-stage3-integration-2d", max_level=2, dimension=2)

    parent_level = 0
    child_level = 1
    parent_coords = (0, 0)

    # Parent starts in subtree 0.
    tree._tree_arrays[parent_level][parent_coords] = (1, 0)

    children = list(
        itertools.product(
            range(3 * parent_coords[0], 3 * parent_coords[0] + 3),
            range(3 * parent_coords[1], 3 * parent_coords[1] + 3),
        )
    )

    # Mixed children with subtree 2 dominant.
    for coords in children[:5]:
        tree._tree_arrays[child_level][coords] = (1, 2)
    for coords in children[5:8]:
        tree._tree_arrays[child_level][coords] = (1, 0)
    tree._tree_arrays[child_level][children[8]] = (1, 1)

    return tree, parent_coords


@pytest.mark.integration
def test_split_tree_flags_enable_stage3_promotion(monkeypatch):
    """
    Integration-style test for Stage III through split_tree() flags.

    We run split_tree() twice on identical input:
    - with Stage II only: parent remains unchanged (mixed children)
    - with Stage II + Stage III: parent is promoted to dominant subtree
    """

    def _fake_stage_i(peano_tree, number_of_splits, finest_level_first=False):
        # Keep preconfigured tree untouched and report synthetic Stage I stats.
        return True, [0] * (number_of_splits + 1)

    # Force split_tree() to traverse Stage II/III over our handcrafted case.
    monkeypatch.setattr(split_strategy, "_distribute_leaves_bfs", _fake_stage_i)

    # Baseline: Stage II only -> mixed children should not promote parent.
    tree_stage2_only, parent_coords = _build_stage3_mixed_parent_case_2d()
    result_stage2_only = split_strategy.split_tree(
        peano_tree=tree_stage2_only,
        number_of_splits=2,
        traversal="bfs",
        enable_propagate_bottom_up=True,
        enable_optimize_hierarchy=False,
        enable_extract_split_instructions=False,
    )

    assert result_stage2_only == ([], [], [], [])
    assert tree_stage2_only._tree_arrays[0][parent_coords] == (1, 0)

    # Target: Stage II + Stage III -> parent promoted to dominant subtree 2.
    tree_stage2_plus_3, parent_coords = _build_stage3_mixed_parent_case_2d()
    result_stage2_plus_3 = split_strategy.split_tree(
        peano_tree=tree_stage2_plus_3,
        number_of_splits=2,
        traversal="bfs",
        enable_propagate_bottom_up=True,
        enable_optimize_hierarchy=True,
        enable_extract_split_instructions=False,
    )

    assert result_stage2_plus_3 == ([], [], [], [])
    assert tree_stage2_plus_3._tree_arrays[0][parent_coords] == (1, 2)
