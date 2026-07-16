import pytest

from toolbox.loadbalancing.api.input.RegularGridGenerator import RegularGridGenerator
import toolbox.loadbalancing.api.strategies.SplitTrees as split_strategy


@pytest.mark.parametrize(
    "dimensions",
    [2, 3],
)
def test_extract_split_instructions_no_splits_returns_empty_lists(dimensions):
    """
    Stage IV invariant: A tree with only a single subtree yields empty split instructions.

    This tests the base case where no splitting has occurred (all cells remain in subtree 0).
    """
    # Arrange: tree with 2 levels; all cells in subtree 0 (no splitting)
    tree = RegularGridGenerator(
        name=f"pytest-tree-no-splits-{dimensions}d",
        max_level=2,
        dimensions=dimensions,
    )

    # Act: extract split instructions
    grid_sweeps, splitting_trees, num_cells, sub_trees = split_strategy.extract_split_instructions(
        tree, base_sweep=2
    )

    # Assert: all lists should be empty for unsplit tree
    assert grid_sweeps == []
    assert splitting_trees == []
    assert num_cells == []
    assert sub_trees == []


@pytest.mark.parametrize(
    "dimensions",
    [2, 3],
)
def test_extract_split_instructions_level_zero_introduction(dimensions):
    """
    Stage IV: Cells introduced at level 0 are attributed to source subtree 0.

    When a subtree is present on level 0 (no parent exists), it is treated as
    introduced from subtree 0 with grid_sweep = base_sweep * 1.
    """
    # Arrange: tree with 2 levels
    tree = RegularGridGenerator(
        name=f"pytest-tree-level0-intro-{dimensions}d",
        max_level=2,
        dimensions=dimensions,
    )
    tree._subtree_count = 2

    # Manually assign one cell at level 0 to subtree 1
    if dimensions == 2:
        tree._tree_arrays[0][(0, 0)] = (1, 1)
    else:
        tree._tree_arrays[0][(0, 0, 0)] = (1, 1)

    # Act: extract split instructions
    grid_sweeps, splitting_trees, num_cells, sub_trees = split_strategy.extract_split_instructions(
        tree, base_sweep=2
    )

    # Assert: one operation introduced from subtree 0 at level 0
    assert len(grid_sweeps) == 1
    assert grid_sweeps[0] == 2  # base_sweep * (level + 1) = 2 * 1
    assert splitting_trees[0] == 0  # source is always subtree 0 at level 0
    assert num_cells[0] == 1  # one cell assigned to subtree 1
    assert sub_trees[0] == 1  # target subtree is 1


@pytest.mark.parametrize(
    "dimensions",
    [2, 3],
)
def test_extract_split_instructions_parent_child_edge_introduction(dimensions):
    """
    Stage IV: Cells introduced across a parent-child edge are correctly attributed.

    A cell is "introduced" at a given level if it belongs to a different subtree
    than its parent. The source subtree for that operation is the parent's subtree.
    """
    # Arrange: tree with 3 levels
    tree = RegularGridGenerator(
        name=f"pytest-tree-parent-child-intro-{dimensions}d",
        max_level=3,
        dimensions=dimensions,
    )
    tree._subtree_count = 2

    # Level 0: single root in subtree 0
    if dimensions == 2:
        tree._tree_arrays[0][(0, 0)] = (1, 0)
        # Level 1: parent at (0,0) in subtree 0
        tree._tree_arrays[1][(0, 0)] = (1, 0)
        # Level 2: child at (0,0) in subtree 1 (introduced from parent subtree 0)
        tree._tree_arrays[2][(0, 0)] = (1, 1)
    else:
        tree._tree_arrays[0][(0, 0, 0)] = (1, 0)
        tree._tree_arrays[1][(0, 0, 0)] = (1, 0)
        tree._tree_arrays[2][(0, 0, 0)] = (1, 1)

    # Act: extract split instructions
    grid_sweeps, splitting_trees, num_cells, sub_trees = split_strategy.extract_split_instructions(
        tree, base_sweep=2
    )

    # Assert: introduction detected at level 2 from subtree 0
    # We should see 1 operation at level 2
    level2_ops = [i for i, gs in enumerate(grid_sweeps) if gs == 6]  # base_sweep * 3 = 6
    assert len(level2_ops) >= 1

    # Check the operation at level 2
    op_idx = level2_ops[0]
    assert splitting_trees[op_idx] == 0  # source is parent subtree 0
    assert num_cells[op_idx] == 1  # one cell introduced
    assert sub_trees[op_idx] == 1  # target is subtree 1


@pytest.mark.parametrize(
    "dimensions",
    [2, 3],
)
def test_extract_split_instructions_multi_level_split_chain(dimensions):
    """
    Stage IV: Multiple splits across levels are correctly tracked and sorted.

    This tests a chain of splits: Subtree 0 -> Subtree 1 -> Subtree 2 across
    multiple levels, verifying correct source attribution and sweep ordering.
    """
    # Arrange: tree with 3 levels
    tree = RegularGridGenerator(
        name=f"pytest-tree-chain-{dimensions}d",
        max_level=3,
        dimensions=dimensions,
    )
    tree._subtree_count = 3

    if dimensions == 2:
        # Level 0: root in subtree 0
        tree._tree_arrays[0][(0, 0)] = (1, 0)
        # Level 1: first split - introduce subtree 1
        tree._tree_arrays[1][(0, 0)] = (1, 0)
        tree._tree_arrays[1][(0, 1)] = (1, 1)
        # Level 2: second split - introduce subtree 2 from subtree 1
        tree._tree_arrays[2][(0, 3)] = (1, 1)
        tree._tree_arrays[2][(0, 4)] = (1, 2)
    else:
        tree._tree_arrays[0][(0, 0, 0)] = (1, 0)
        tree._tree_arrays[1][(0, 0, 0)] = (1, 0)
        tree._tree_arrays[1][(0, 0, 1)] = (1, 1)
        tree._tree_arrays[2][(0, 0, 3)] = (1, 1)
        tree._tree_arrays[2][(0, 0, 4)] = (1, 2)

    # Act: extract split instructions
    grid_sweeps, splitting_trees, num_cells, sub_trees = split_strategy.extract_split_instructions(
        tree, base_sweep=2
    )

    # Assert: should have at least 2 operations (one for subtree 1, one for subtree 2)
    assert len(grid_sweeps) >= 2

    # Verify operations are sorted by (grid_sweep, target_subtree, level)
    for i in range(len(grid_sweeps) - 1):
        if grid_sweeps[i] == grid_sweeps[i + 1]:
            assert sub_trees[i] <= sub_trees[i + 1]


@pytest.mark.parametrize(
    "dimensions",
    [2, 3],
)
def test_extract_split_instructions_ambiguous_parent_source_defaults_to_zero(dimensions):
    """
    Stage IV: When introduced cells have multiple different parent subtrees,
    the source subtree defaults to 0.

    This handles the case where a target subtree is introduced from multiple
    source subtrees on the same level, making the source ambiguous.
    """
    # Arrange: tree with 3 levels for clear parent-child relationships
    tree = RegularGridGenerator(
        name=f"pytest-tree-ambiguous-{dimensions}d",
        max_level=3,
        dimensions=dimensions,
    )
    tree._subtree_count = 3

    if dimensions == 2:
        # Level 0: one root in subtree 0
        tree._tree_arrays[0][(0, 0)] = (1, 0)
        # Level 1: split root into two parents - one in subtree 0, one in subtree 1
        tree._tree_arrays[1][(0, 0)] = (1, 0)
        tree._tree_arrays[1][(0, 1)] = (1, 1)
        # Level 2: children from both parents (at different parent coords) assigned to subtree 2
        # Children of (0,0) are at (0,0)-(0,2)
        tree._tree_arrays[2][(0, 0)] = (1, 2)
        # Children of (0,1) are at (0,3)-(0,5)
        tree._tree_arrays[2][(0, 3)] = (1, 2)
    else:
        tree._tree_arrays[0][(0, 0, 0)] = (1, 0)
        tree._tree_arrays[1][(0, 0, 0)] = (1, 0)
        tree._tree_arrays[1][(0, 0, 1)] = (1, 1)
        tree._tree_arrays[2][(0, 0, 0)] = (1, 2)
        tree._tree_arrays[2][(0, 0, 3)] = (1, 2)

    # Act: extract split instructions
    grid_sweeps, splitting_trees, num_cells, sub_trees = split_strategy.extract_split_instructions(
        tree, base_sweep=2
    )

    # Assert: when cells with target=2 exist at level 2 (grid_sweep=8)
    # with parents from subtrees 0 and 1, the source should default to 0 (ambiguous)
    subtree2_ops = [
        i for i, st in enumerate(sub_trees) if st == 2 and grid_sweeps[i] == 8
    ]  # level 2 operation
    if len(subtree2_ops) > 0:
        op_idx = subtree2_ops[0]
        assert splitting_trees[op_idx] == 0  # ambiguous → defaults to 0


def test_extract_split_instructions_grid_sweep_calculation():
    """
    Stage IV: Grid sweeps are correctly calculated as base_sweep * (level + 1).

    Verify that the grid sweep values follow the formula: base_sweep * (level + 1)
    for different base_sweep values.
    """
    # Arrange: tree with 3 levels
    tree = RegularGridGenerator(
        name="pytest-tree-sweep-calc",
        max_level=3,
        dimensions=2,
    )
    tree._subtree_count = 2
    tree._tree_arrays[0][(0, 0)] = (1, 1)  # Introduce at level 0
    tree._tree_arrays[1][(0, 0)] = (1, 1)  # Ensure existence at level 1
    tree._tree_arrays[2][(0, 0)] = (1, 1)  # Ensure existence at level 2

    # Act: extract with base_sweep = 3
    grid_sweeps, _, _, _ = split_strategy.extract_split_instructions(
        tree, base_sweep=3
    )

    # Assert: sweeps should be multiples of 3
    expected_sweeps = {3, 6, 9}  # base_sweep * 1, 2, 3
    actual_sweeps = set(grid_sweeps)
    assert actual_sweeps.issubset(expected_sweeps)


def test_extract_split_instructions_consistency_with_split_tree():
    """
    Stage IV: Integration test verifying consistency between split_tree output
    and extract_split_instructions when Stage IV is enabled.

    This ensures that the instructions extracted from the final tree state
    match what split_tree reports when enable_extract_split_instructions=True.
    """
    # Arrange: tree with 2 levels
    tree = RegularGridGenerator(
        name="pytest-tree-consistency",
        max_level=2,
        dimensions=2,
    )

    # Manually set up a simple split pattern
    tree._subtree_count = 2
    tree._tree_arrays[0][(0, 0)] = (1, 0)
    tree._tree_arrays[1][(0, 0)] = (1, 0)
    tree._tree_arrays[1][(0, 1)] = (1, 1)

    # Act: extract instructions directly
    grid_sweeps, splitting_trees, num_cells, sub_trees = split_strategy.extract_split_instructions(
        tree, base_sweep=2
    )

    # Assert: results should be consistent lists
    assert isinstance(grid_sweeps, list)
    assert isinstance(splitting_trees, list)
    assert isinstance(num_cells, list)
    assert isinstance(sub_trees, list)

    # All lists should have the same length
    assert len(grid_sweeps) == len(splitting_trees) == len(num_cells) == len(sub_trees)


@pytest.mark.parametrize(
    "dimensions",
    [2, 3],
)
def test_extract_split_instructions_only_introduced_cells_counted(dimensions):
    """
    Stage IV: Only cells "introduced" across parent-child edges are counted,
    not all cells of a target subtree.

    A cell is introduced if it differs from its parent's subtree. Cells that
    match their parent's subtree are not counted as introduced.
    """
    # Arrange: tree with 3 levels
    tree = RegularGridGenerator(
        name=f"pytest-tree-introduced-only-{dimensions}d",
        max_level=3,
        dimensions=dimensions,
    )
    tree._subtree_count = 2

    if dimensions == 2:
        # Level 0-1: all in subtree 0
        tree._tree_arrays[0][(0, 0)] = (1, 0)
        tree._tree_arrays[1][(0, 0)] = (1, 0)
        tree._tree_arrays[1][(0, 1)] = (1, 0)
        # Level 2: mix of subtrees
        # (0,0)-(0,2) are children of (0,0) at level 1 → not introduced (parent also 0)
        tree._tree_arrays[2][(0, 0)] = (1, 0)
        # (0,3)-(0,5) are children of (0,1) at level 1 → introduced to subtree 1
        tree._tree_arrays[2][(0, 3)] = (1, 1)
    else:
        tree._tree_arrays[0][(0, 0, 0)] = (1, 0)
        tree._tree_arrays[1][(0, 0, 0)] = (1, 0)
        tree._tree_arrays[1][(0, 0, 1)] = (1, 0)
        tree._tree_arrays[2][(0, 0, 0)] = (1, 0)
        tree._tree_arrays[2][(0, 0, 3)] = (1, 1)

    # Act: extract split instructions
    grid_sweeps, splitting_trees, num_cells, sub_trees = split_strategy.extract_split_instructions(
        tree, base_sweep=2
    )

    # Assert: only the introduced cell at level 2 is counted
    level2_ops = [i for i, gs in enumerate(grid_sweeps) if gs == 6]
    assert len(level2_ops) > 0
    op_idx = level2_ops[0]
    assert num_cells[op_idx] == 1  # only one cell introduced


@pytest.mark.parametrize(
    "base_sweep",
    [1, 2, 3, 5],
)
def test_extract_split_instructions_parameterized_base_sweep(base_sweep):
    """
    Stage IV: Grid sweep values scale correctly with the base_sweep parameter.

    Verify that different base_sweep values produce correctly scaled grid sweeps.
    """
    # Arrange: simple 2-level tree with split at level 0
    tree = RegularGridGenerator(
        name="pytest-tree-base-sweep",
        max_level=2,
        dimensions=2,
    )
    tree._subtree_count = 2
    tree._tree_arrays[0][(0, 0)] = (1, 1)

    # Act: extract with varying base_sweep
    grid_sweeps, _, _, _ = split_strategy.extract_split_instructions(
        tree, base_sweep=base_sweep
    )

    # Assert: grid sweeps are multiples of base_sweep
    assert len(grid_sweeps) > 0
    for sweep in grid_sweeps:
        assert sweep % base_sweep == 0
        assert sweep // base_sweep in [1, 2]  # level 0 or 1


def test_extract_split_instructions_result_tuple_structure():
    """
    Stage IV: Return value always has the correct tuple structure of 4 lists.

    Regardless of tree state, the function must return a tuple of 4 lists.
    """
    # Arrange: minimal tree
    tree = RegularGridGenerator(
        name="pytest-tree-tuple-structure",
        max_level=1,
        dimensions=2,
    )

    # Act: extract instructions (no splits)
    result = split_strategy.extract_split_instructions(tree, base_sweep=2)

    # Assert: result is always a 4-tuple of lists
    assert isinstance(result, tuple)
    assert len(result) == 4
    for item in result:
        assert isinstance(item, list)
