# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import yaml
from Tree import Tree


from .SFC import Peano_to_Cartesian_2d
from .SFC import Peano_to_Cartesian_3d


def split_tree(
    peano_tree: Tree,
    number_of_subtrees: int,
    traversal: str = "bfs_finest_first",
    enable_propagate_bottom_up: bool = False,
    enable_optimize_hierarchy: bool = False,
    require_all_children_for_hierarchy_optimization: bool = True,
    enable_extract_split_instructions: bool = False,
) -> tuple[list[int], list[int], list[int], list[int]]:
    """
    Split a tree using a staged load-balancing workflow.

    Stages:
    1. distribute_leaves_bottom_up() or distribute_leaves_top_down(): Redistribute leaf nodes across subtrees.
    2. _propagate_bottom_up(): Propagate subtree ownership to internal nodes.
    3. _optimize_hierarchy() (optional): Further reduce split complexity by
       promoting complete child sets up the hierarchy.
    4. extract_split_instructions() (optional): Derive hardcoded split
       instructions from the transformed tree.

    Args:
        peano_tree: Tree to split (modified in place).
        number_of_subtrees: Total number of subtrees to create.
        traversal: Stage I traversal strategy. Supported values:
            - "bfs": level-order leaf collection
            - "bfs_finest_first": level-order leaf collection within each level,
              but levels processed from finest to coarsest
            - "dfs": depth-first leaf collection
        enable_propagate_bottom_up: If True, enable Stage II bottom-up subtree
            propagation after leaf redistribution.
        enable_optimize_hierarchy: If True, enable Stage III hierarchical
            optimisation to minimise split operations through hierarchy lifting.
        require_all_children_for_hierarchy_optimization: Stage III guard for
            parent promotion. If True (default), a parent is only considered if
            all 3^d children exist. If False, Stage III may also promote mixed
            parents with partial child sets.
        enable_extract_split_instructions: If True, run Stage IV and return the
            derived `(gridSweeps, splittingTrees, numberOfCells, subTrees)`.

    Returns:
        A tuple `(gridSweeps, splittingTrees, numberOfCells, subTrees)`.
        If Stage IV is disabled, returns `([], [], [], [])`.

    Notes:
        - All stages after Stage I are optional.
        - The tree is modified in place by the enabled transformation stages.
        - If number_of_subtrees <= 1, no transformation stage runs.
        - Stage III requires Stage II to have been applied first for correctness.
    """
    if number_of_subtrees <= 1:
        print(f"No splitting requested (number_of_subtrees = {number_of_subtrees}).")
        if enable_extract_split_instructions:
            return extract_split_instructions(peano_tree)
        return ([], [], [], [])

    print(
        f"Splitting tree '{peano_tree._name}' into "
        f"{number_of_subtrees} subtrees (staged workflow)"
    )

    traversal_mode = traversal.lower()
    if traversal_mode == "bfs":
        has_leaves_to_split, moved_nodes = distribute_leaves_bottom_up(
            peano_tree, number_of_subtrees, finest_level_first=False
        )
    elif traversal_mode == "bfs_finest_first":
        has_leaves_to_split, moved_nodes = distribute_leaves_bottom_up(
            peano_tree, number_of_subtrees, finest_level_first=True
        )
    elif traversal_mode == "dfs":
        has_leaves_to_split, moved_nodes = distribute_leaves_top_down(peano_tree, number_of_subtrees)
    else:
        raise ValueError(
            f"Unsupported traversal '{traversal}'. Use 'bfs', 'bfs_finest_first' or 'dfs'."
        )

    if not has_leaves_to_split:
        if enable_extract_split_instructions:
            return extract_split_instructions(peano_tree)
        return ([], [], [], [])

    if enable_propagate_bottom_up:
        _propagate_bottom_up(peano_tree)

    if enable_optimize_hierarchy:
        _optimize_hierarchy(
            peano_tree,
            require_all_children=require_all_children_for_hierarchy_optimization,
        )


    # Update tree metadata after all stages
    peano_tree._subtree_count = number_of_subtrees
    peano_tree.update_split_pattern()

    print(f"Splitting complete. Subtree count: {peano_tree._subtree_count}")
    print(f"Leaf distribution: {moved_nodes}")
    print(f"Split pattern: {peano_tree._split_pattern}")

    if enable_extract_split_instructions:
        return extract_split_instructions(peano_tree)

    return ([], [], [], [])


def distribute_leaves_bottom_up(
    peano_tree: Tree,
    number_of_subtrees: int,
    finest_level_first: bool = False,
) -> tuple[bool, list[int]]:
    """
    Stage I: BFS-style leaf redistribution over Peano SFC order.

    This strategy runs through the mesh level by level. It first assigns all
    leaves on the finest level to partitions, then the leaves on the next
    coarser level, and so forth. Within each level, cells are visited in
    Peano space-filling-curve order, so the redistribution follows the same
    ordering as the SFC traversal used elsewhere in the load-balancing API.

    @image html PeanoSFC.png

    The goal is to keep the finest-mesh cells contiguous while respecting the
    hierarchical tree structure. In practice, that makes the routine well
    suited for solvers that spend most of their work on the finest cells, such
    as limiter-based or multigrid-style workflows.

    The lexicographic order that used to be documented here now lives in
    `Strategies/Lexicographic.py`; this function intentionally uses Peano SFC
    order instead.
    
    Leaf definition:
    - Any existing cell on the finest level, or
    - Any existing coarser-level cell with no children on the next level.

    Args:
        peano_tree: Tree to update (modified in place).
        number_of_subtrees: Total number of subtrees to create.
        finest_level_first: If `False` (default), process levels from coarsest
            to finest (`0..max_level-1`). If `True`, process levels from finest
            to coarsest while keeping BFS order inside each level.

    Returns:
        A tuple `(has_leaves_to_split, moved_nodes_per_subtree)`:
        - `has_leaves_to_split`: `False` if no eligible leaves were found.
        - `moved_nodes_per_subtree`: Per-subtree counts of reassigned leaves.

    Notes:
        - Only leaves in subtree 0 are redistributed.
        - Node existence flags (`weight`) are preserved.
        - This stage does not update non-leaf assignments.
    """
    # Collect all leaf nodes (at all levels) that belong to subtree 0.
    # The level sweep can be either coarse->fine (legacy) or fine->coarse.
    leaf_nodes = []

    level_range = range(peano_tree._max_level)
    if finest_level_first:
        level_range = range(peano_tree._max_level - 1, -1, -1)

    for level in level_range:
        size = 3**(level + 1)

        if peano_tree._dimension == 2:
            for peano_index in range(size * size):
                a, b = Peano_to_Cartesian_2d(peano_index, size)
                current_node = peano_tree._tree_arrays[level][a][b]
                if current_node[0] == 1 and current_node[1] == 0:  # Node exists and belongs to subtree 0
                    # Check if this is a leaf node
                    is_leaf = False

                    if level == peano_tree._max_level - 1:
                        # Nodes at finest level are always leaves
                        is_leaf = True
                    else:
                        # Check if node has no children (not refined further)
                        child_a, child_b = 3*a, 3*b
                        if (child_a < 3**(level+2) and child_b < 3**(level+2) and 
                            peano_tree._tree_arrays[level+1][child_a][child_b][0] == 0):
                            is_leaf = True

                    if is_leaf:
                        leaf_nodes.append((level, a, b))

        elif peano_tree._dimension == 3:
            for peano_index in range(size * size * size):
                a, b, c = Peano_to_Cartesian_3d(peano_index, size)
                current_node = peano_tree._tree_arrays[level][a][b][c]
                if current_node[0] == 1 and current_node[1] == 0:  # Node exists and belongs to subtree 0
                    # Check if this is a leaf node
                    is_leaf = False

                    if level == peano_tree._max_level - 1:
                        # Nodes at finest level are always leaves
                        is_leaf = True
                    else:
                        # Check if node has no children (not refined further)
                        child_a, child_b, child_c = 3*a, 3*b, 3*c
                        if (child_a < 3**(level+2) and child_b < 3**(level+2) and child_c < 3**(level+2) and
                            peano_tree._tree_arrays[level+1][child_a][child_b][child_c][0] == 0):
                            is_leaf = True

                    if is_leaf:
                        leaf_nodes.append((level, a, b, c))

    total_leaves = len(leaf_nodes)

    if total_leaves == 0:
        print("No leaf nodes found in subtree 0 to split.")
        return (False, [])

    print(f"Found {total_leaves} leaf nodes across all levels to distribute")

    # Calculate how many leaves each subtree should get for balanced distribution
    leaves_per_subtree = total_leaves // (number_of_subtrees)
    remaining_leaves = total_leaves % (number_of_subtrees)

    print(f"Target distribution: {leaves_per_subtree} leaves per subtree, {remaining_leaves} extra leaves")

    # Create distribution plan
    distribution = []
    for i in range(number_of_subtrees):
        target_count = leaves_per_subtree + (1 if i < remaining_leaves else 0)
        distribution.append(target_count)

    print(f"Distribution plan: {distribution}")

    # Apply the redistribution to leaf nodes
    current_subtree = 0
    current_count = 0
    moved_nodes = [0] * (number_of_subtrees)  # Track actual moves

    for leaf_coords in leaf_nodes:
        if peano_tree._dimension == 2:
            level, a, b = leaf_coords
            # Assign leaf to current subtree
            peano_tree._tree_arrays[level][a][b] = (1, current_subtree)
            moved_nodes[current_subtree] += 1
            current_count += 1
            
        elif peano_tree._dimension == 3:
            level, a, b, c = leaf_coords
            # Assign leaf to current subtree
            peano_tree._tree_arrays[level][a][b][c] = (1, current_subtree)
            moved_nodes[current_subtree] += 1
            current_count += 1

        # Check if current subtree is full, move to next
        if current_count >= distribution[current_subtree] and current_subtree < number_of_subtrees - 1:
            current_subtree += 1
            current_count = 0

    return (True, moved_nodes)


def distribute_leaves_top_down(peano_tree: Tree, number_of_subtrees: int) -> tuple[bool, list[int]]:
    """
    Stage I: DFS-style leaf redistribution over the Peano SFC.

    Collect leaf cells currently assigned to subtree 0 and redistribute them
    across `number_of_subtrees` subtrees as evenly as possible.
    The redistribution logic is identical to `distribute_leaves_bottom_up()`, but leaf
    discovery order follows DFS instead of level-order scanning. Both component-root
    discovery and per-parent child traversal use Peano SFC order.

    Args:
        peano_tree: Tree to update (modified in place).
        number_of_subtrees: Total number of subtrees to create.

    Returns:
        A tuple `(has_leaves_to_split, moved_nodes_per_subtree)`:
        - `has_leaves_to_split`: `False` if no eligible leaves were found.
        - `moved_nodes_per_subtree`: Per-subtree counts of reassigned leaves.
        
        Both return values can be ignored if you want to.
    """
    leaf_nodes = []
    visited = set()
    max_level = peano_tree._max_level

    def _node_exists(level: int, a: int, b: int, c: int | None = None) -> bool:
        if peano_tree._dimension == 2:
            return peano_tree._tree_arrays[level][a][b][0] == 1
        return peano_tree._tree_arrays[level][a][b][c][0] == 1

    def _node_subtree(level: int, a: int, b: int, c: int | None = None) -> int:
        if peano_tree._dimension == 2:
            return peano_tree._tree_arrays[level][a][b][1]
        return peano_tree._tree_arrays[level][a][b][c][1]

    def _children_2d(level: int, a: int, b: int) -> tuple[bool, list[tuple[int, int]]]:
        if level == max_level - 1:
            return (False, [])

        children = []
        has_existing_children = False
        for peano_index in range(9):
            local_a, local_b = Peano_to_Cartesian_2d(peano_index, 3)
            child_a = 3 * a + local_a
            child_b = 3 * b + local_b
            if _node_exists(level + 1, child_a, child_b):
                has_existing_children = True
                if _node_subtree(level + 1, child_a, child_b) == 0:
                    children.append((child_a, child_b))

        return (has_existing_children, children)

    def _children_3d(level: int, a: int, b: int, c: int) -> tuple[bool, list[tuple[int, int, int]]]:
        if level == max_level - 1:
            return (False, [])

        children = []
        has_existing_children = False
        for peano_index in range(27):
            local_a, local_b, local_c = Peano_to_Cartesian_3d(peano_index, 3)
            child_a = 3 * a + local_a
            child_b = 3 * b + local_b
            child_c = 3 * c + local_c
            if _node_exists(level + 1, child_a, child_b, child_c):
                has_existing_children = True
                if _node_subtree(level + 1, child_a, child_b, child_c) == 0:
                    children.append((child_a, child_b, child_c))

        return (has_existing_children, children)

    def _dfs_2d(level: int, a: int, b: int) -> None:
        key = (level, a, b)
        if key in visited:
            return
        visited.add(key)

        has_existing_children, children = _children_2d(level, a, b)
        if not has_existing_children:
            leaf_nodes.append((level, a, b))
            return

        for child_a, child_b in children:
            _dfs_2d(level + 1, child_a, child_b)

    def _dfs_3d(level: int, a: int, b: int, c: int) -> None:
        key = (level, a, b, c)
        if key in visited:
            return
        visited.add(key)

        has_existing_children, children = _children_3d(level, a, b, c)
        if not has_existing_children:
            leaf_nodes.append((level, a, b, c))
            return

        for child_a, child_b, child_c in children:
            _dfs_3d(level + 1, child_a, child_b, child_c)

    # Start DFS from all subtree-0 nodes that are roots of subtree-0 components.
    # A component root is a level-0 node or a node whose parent is not in subtree 0.
    # Within each level, roots are discovered in Peano SFC order.
    for level in range(max_level):
        size = 3 ** (level + 1)

        if peano_tree._dimension == 2:
            for peano_index in range(size * size):
                a, b = Peano_to_Cartesian_2d(peano_index, size)
                if not _node_exists(level, a, b):
                    continue
                if _node_subtree(level, a, b) != 0:
                    continue

                is_component_root = level == 0
                if not is_component_root:
                    pa, pb = a // 3, b // 3
                    parent_exists = _node_exists(level - 1, pa, pb)
                    parent_subtree0 = parent_exists and _node_subtree(level - 1, pa, pb) == 0
                    is_component_root = not parent_subtree0

                if is_component_root:
                    _dfs_2d(level, a, b)

        elif peano_tree._dimension == 3:
            for peano_index in range(size * size * size):
                a, b, c = Peano_to_Cartesian_3d(peano_index, size)
                if not _node_exists(level, a, b, c):
                    continue
                if _node_subtree(level, a, b, c) != 0:
                    continue

                is_component_root = level == 0
                if not is_component_root:
                    pa, pb, pc = a // 3, b // 3, c // 3
                    parent_exists = _node_exists(level - 1, pa, pb, pc)
                    parent_subtree0 = (
                        parent_exists and _node_subtree(level - 1, pa, pb, pc) == 0
                    )
                    is_component_root = not parent_subtree0

                if is_component_root:
                    _dfs_3d(level, a, b, c)

    total_leaves = len(leaf_nodes)

    if total_leaves == 0:
        print("No leaf nodes found in subtree 0 to split.")
        return (False, [])

    print(f"Found {total_leaves} leaf nodes across all levels to distribute (DFS order)")

    leaves_per_subtree = total_leaves // (number_of_subtrees)
    remaining_leaves = total_leaves % (number_of_subtrees)

    print(f"Target distribution: {leaves_per_subtree} leaves per subtree, {remaining_leaves} extra leaves")

    distribution = []
    for i in range(number_of_subtrees):
        target_count = leaves_per_subtree + (1 if i < remaining_leaves else 0)
        distribution.append(target_count)

    print(f"Distribution plan: {distribution}")

    current_subtree = 0
    current_count = 0
    moved_nodes = [0] * (number_of_subtrees)

    for leaf_coords in leaf_nodes:
        if peano_tree._dimension == 2:
            level, a, b = leaf_coords
            peano_tree._tree_arrays[level][a][b] = (1, current_subtree)
            moved_nodes[current_subtree] += 1
            current_count += 1

        elif peano_tree._dimension == 3:
            level, a, b, c = leaf_coords
            peano_tree._tree_arrays[level][a][b][c] = (1, current_subtree)
            moved_nodes[current_subtree] += 1
            current_count += 1

        if current_count >= distribution[current_subtree] and current_subtree < number_of_subtrees - 1:
            current_subtree += 1
            current_count = 0

    return (True, moved_nodes)


def _propagate_bottom_up(peano_tree: Tree) -> None:
    """
    Stage II of the staged load-balancing workflow.

    Propagate subtree assignments from leaves to internal nodes in a bottom-up pass.
    For each existing non-leaf node, if all existing children belong to the same
    subtree, assign the parent to that subtree.

    Args:
        peano_tree: Tree to update (modified in place).

    Returns:
        None.

    Notes:
        - Operates from finest internal level to root.
        - Preserves node existence flags (`weight`).
        - Parents with mixed-child subtree assignments remain unchanged.
        - Intended to run after Stage I (`distribute_leaves_bottom_up()` or `distribute_leaves_top_down()`).
    """
    print("Updating non-leaf node assignments to maintain tree topology...")
    
    # Work from finest to coarsest level (bottom-up)
    for level in range(peano_tree._max_level - 2, -1, -1):  # Skip finest level (only leaves)
        size = 3**(level + 1)
        
        if peano_tree._dimension == 2:
            for a in range(size):
                for b in range(size):
                    current_node = peano_tree._tree_arrays[level][a][b]
                    if current_node[0] == 1:  # Node exists
                        # Check children assignments
                        child_subtrees = set()
                        child_size = 3**(level + 2)
                        
                        # Collect subtree IDs of all existing children
                        for child_a in range(3*a, min(3*(a+1), child_size)):
                            for child_b in range(3*b, min(3*(b+1), child_size)):
                                child_node = peano_tree._tree_arrays[level+1][child_a][child_b]
                                if child_node[0] == 1:  # Child exists
                                    child_subtrees.add(child_node[1])
                        
                        # If all children belong to the same subtree, assign parent to that subtree
                        if len(child_subtrees) == 1:
                            target_subtree = list(child_subtrees)[0]
                            peano_tree._tree_arrays[level][a][b] = (1, target_subtree)
                            
        elif peano_tree._dimension == 3:
            for a in range(size):
                for b in range(size):
                    for c in range(size):
                        current_node = peano_tree._tree_arrays[level][a][b][c]
                        if current_node[0] == 1:  # Node exists
                            # Check children assignments
                            child_subtrees = set()
                            child_size = 3**(level + 2)
                            
                            # Collect subtree IDs of all existing children
                            for child_a in range(3*a, min(3*(a+1), child_size)):
                                for child_b in range(3*b, min(3*(b+1), child_size)):
                                    for child_c in range(3*c, min(3*(c+1), child_size)):
                                        child_node = peano_tree._tree_arrays[level+1][child_a][child_b][child_c]
                                        if child_node[0] == 1:  # Child exists
                                            child_subtrees.add(child_node[1])
                            
                            # If all children belong to the same subtree, assign parent to that subtree
                            if len(child_subtrees) == 1:
                                target_subtree = list(child_subtrees)[0]
                                peano_tree._tree_arrays[level][a][b][c] = (1, target_subtree)
    
    print("Non-leaf node assignment update completed.")

def _optimize_hierarchy(peano_tree: Tree, require_all_children: bool = True) -> None:
    """
    Stage III of the staged load-balancing workflow.

        Minimise the total number of split operations whilst preserving load balance
        established by Stage II. The algorithm lifts split decisions up the tree hierarchy
        by promoting a parent on level L-1 to the dominant subtree among its children
        on level L.

        Hierarchy lifting is applicable when:
        - All children of a parent exist (count equals 3^d, where d is the spatial
            dimensions count: d=2 -> 9 children, d=3 -> 27 children)
        - Children are mixed across at least two subtrees
        - A dominant child subtree M exists (largest child count)
        - The dominant subtree differs from the current parent subtree and yields a
            reduction of lower-level split pressure

        When these conditions are met, the parent is reassigned to the dominant subtree.
        Remaining child subtrees stay on level L, but their source parent subtree is now
        the updated one (which effectively moves part of splitting upwards).

    The algorithm operates bottom-up from finest to coarsest level(s), iteratively
    lifting applicable promotions until no further reductions are possible.

    Args:
        peano_tree: Tree to update (modified in place).
        Required precondition: Stage II (bottom-up propagation) must have been
        applied first to ensure non-leaf subtree consistency.
        require_all_children: If True (default), only promote parents whose
        full 3^d child set exists. If False, also allow promotions for mixed
        partial child sets.

    Returns:
        None.

    Notes:
        - This function is intended to run after `_propagate_bottom_up()`.
        - Preserves node existence flags (`weight`).
        - Modifies only subtree assignments to reduce split operation count.
        - Multi-pass iteration allows cascading promotions up the tree.
        - The "all children in one subtree" case is already resolved by Stage II;
          Stage III primarily targets mixed-child parents.
    """
    print("Stage III: Optimising hierarchy to minimise split operations...")

    dimensions = peano_tree._dimension
    max_level = peano_tree._max_level
    children_per_parent = 3 ** dimensions

    def _dominant_subtree(child_subtrees: list[int], parent_subtree: int) -> int | None:
        """
        Pick a promotion target from mixed child subtree assignments.

        Returns the dominant subtree id when promotion is beneficial, otherwise None.
        """
        if require_all_children and len(child_subtrees) != children_per_parent:
            return None

        counts: dict[int, int] = {}
        for subtree in child_subtrees:
            counts[subtree] = counts.get(subtree, 0) + 1

        if len(counts) <= 1:
            # Stage II already covers this case.
            return None

        # Deterministic tie-break: larger count first, then larger subtree id.
        ranked = sorted(counts.items(), key=lambda item: (item[1], item[0]), reverse=True)
        dominant_subtree, dominant_count = ranked[0]

        if dominant_subtree == parent_subtree:
            return None

        # Do not move ownership "backward" to an earlier subtree.
        if dominant_subtree < parent_subtree:
            return None

        parent_count = counts.get(parent_subtree, 0)
        if dominant_count <= parent_count:
            return None

        return dominant_subtree

    # Iterate until no more optimisations can be made
    made_changes = True
    pass_count = 0

    while made_changes:
        made_changes = False
        pass_count += 1

        # Work from finest internal level to root (bottom-up)
        for level in range(max_level - 2, -1, -1):
            size = 3 ** (level + 1)

            if dimensions == 2:
                for a in range(size):
                    for b in range(size):
                        parent_node = peano_tree._tree_arrays[level][a][b]
                        if parent_node[0] != 1:  # Parent doesn't exist
                            continue

                        # Collect all children's subtree assignments
                        child_subtrees = []
                        child_size = 3 ** (level + 2)

                        for child_a in range(3 * a, min(3 * (a + 1), child_size)):
                            for child_b in range(3 * b, min(3 * (b + 1), child_size)):
                                child_node = peano_tree._tree_arrays[level + 1][child_a][child_b]
                                if child_node[0] == 1:  # Child exists
                                    child_subtrees.append(child_node[1])

                        target_subtree = _dominant_subtree(
                            child_subtrees=child_subtrees,
                            parent_subtree=parent_node[1],
                        )

                        if target_subtree is not None:
                            peano_tree._tree_arrays[level][a][b] = (1, target_subtree)
                            made_changes = True

            elif dimensions == 3:
                for a in range(size):
                    for b in range(size):
                        for c in range(size):
                            parent_node = peano_tree._tree_arrays[level][a][b][c]
                            if parent_node[0] != 1:  # Parent doesn't exist
                                continue

                            # Collect all children's subtree assignments
                            child_subtrees = []
                            child_size = 3 ** (level + 2)

                            for child_a in range(3 * a, min(3 * (a + 1), child_size)):
                                for child_b in range(3 * b, min(3 * (b + 1), child_size)):
                                    for child_c in range(3 * c, min(3 * (c + 1), child_size)):
                                        child_node = peano_tree._tree_arrays[level + 1][
                                            child_a][child_b][child_c]
                                        if child_node[0] == 1:  # Child exists
                                            child_subtrees.append(child_node[1])

                            target_subtree = _dominant_subtree(
                                child_subtrees=child_subtrees,
                                parent_subtree=parent_node[1],
                            )

                            if target_subtree is not None:
                                peano_tree._tree_arrays[level][a][b][c] = (
                                    1, target_subtree)
                                made_changes = True

    print(
        f"Hierarchy optimisation completed in {pass_count} pass(es). "
        f"Total split operations reduced."
    )

def generate_tree_yaml(peano_tree: Tree, output_yaml_file: str) -> None:
    """
    Generate a YAML file describing the complete tree structure with spatial layout.
    
    ## YAML Structure
    
    The output contains:
    - **Tree Metadata**: Name, dimensions, domain boundaries, leaf counts
    - **Split Summary**: Distribution statistics across subtrees  
    - **Level Data**: Complete spatial arrays for each refinement level
    
    ## Spatial Array Format
    
    Each level contains a spatial representation:
    - **2D**: Row-indexed arrays with comma-separated (weight,subtree_id) tuples
    - **3D**: Plane-row indexed arrays with nested structure
    - **Indexing**: 0-based indexing for both rows and planes
    
    ## Data Format Example
    
    ```yaml
    tree_structure:
      - level: 1
        size: 3
        data:
          0: "(1,0), (1,0), (1,1)"
          1: "(1,1), (1,1), (1,2)"
          2: "(1,2), (1,2), (1,2)"
    ```
    
    ## Integration with Peano
    
    The YAML format is designed for:
    - Direct import into Peano's hardcoded load balancer
    - Visualization tools and analysis scripts
    - Checkpoint/restart mechanisms for offline decomposition
    
    ## Technical Details
    
    - Preserves exact spatial relationships from tree arrays
    - Maintains type consistency (all values converted to int)
    - Uses safe YAML dumping to prevent formatting issues
    
    Args:
        peano_tree: Tree object to generate YAML from
        output_yaml_file: Path to the output YAML file
        
    Returns:
        None

    Raises:
        IOError: If file cannot be written due to permissions or disk space
        
    Note:
        Large trees may generate substantial YAML files. Consider file system
        constraints when processing high-resolution adaptive meshes.
    """
    # Convert tree arrays to readable format
    tree_levels = []
    
    for level in range(peano_tree._max_level):
        size = 3**(level + 1)
        
        # Create dictionary with row numbers as keys
        level_data = {}
        
        if peano_tree._dimension == 2:
            # Convert 2D array to numbered rows (0-indexed)
            for a in range(size):
                row_data = []
                for b in range(size):
                    weight, subtree_id = peano_tree._tree_arrays[level][a][b]
                    row_data.append(f"({int(weight)},{int(subtree_id)})")
                level_data[a] = ", ".join(row_data)  # Keep 0-indexed rows
            
        elif peano_tree._dimension == 3:
            # Convert 3D array to numbered planes and rows (0-indexed)
            for a in range(size):
                plane_data = {}
                for b in range(size):
                    row_data = []
                    for c in range(size):
                        weight, subtree_id = peano_tree._tree_arrays[level][a][b][c]
                        row_data.append(f"({int(weight)},{int(subtree_id)})")
                    plane_data[b] = ", ".join(row_data)  # Changed to 0-indexed rows
                level_data[a] = plane_data  # Changed to 0-indexed planes
        
        tree_levels.append({
            'level': level + 1,
            'size': size,
            'data': level_data
        })
    
    # Create the complete YAML structure
    split_data = {
        'title': f"Complete Tree Structure from {peano_tree._name}",
        'description': "Spatial representation of tree with (weight, subtree_id) tuples",
        'version': "2.0",
        'tree_info': {
            'name': peano_tree._name,
            'dimension': peano_tree._dimension,
            'max_level': peano_tree._max_level,
            'domain_offset': list(peano_tree._domain_offset),
            'domain_size': list(peano_tree._domain_size),
            'total_leaf_count': int(peano_tree._leaf_count),
            'subtree_count': peano_tree._subtree_count
        },
        'split_summary': {
            'subtree_distribution': [int(count) for count in peano_tree._split_pattern]
        },
        'tree_structure': tree_levels
    }
    
    try:
        with open(output_yaml_file, 'w') as file:
            yaml.dump(split_data, file, default_flow_style=False, sort_keys=False)
        print(f"Complete tree structure written to {output_yaml_file}")
    except IOError as e:
        print(f"Error writing to YAML file: {e}")

def reset_tree_to_single_subtree(peano_tree: Tree) -> Tree:
    """
    Reset all tree nodes to belong to subtree 0, effectively erasing all splitting.
    
    ## Use Cases
    
    - **Algorithm Testing**: Reset to known state between experiments
    - **Re-splitting**: Clear previous decomposition before applying new strategy
    - **Debugging**: Return to simple single-subtree configuration
    
    ## Implementation Details
    
    - Preserves all node weights (existence flags)
    - Only modifies subtree assignments (sets all to 0)
    - Updates tree metadata (subtree_count, split_pattern) consistently
    
    Args:
        peano_tree: Tree object to reset
        
    Returns:
        Modified Tree object with all nodes assigned to subtree 0
        
    Note:
        This operation is irreversible - previous subtree assignments are lost.
        Consider generating YAML backup before reset if preservation is needed.
    """
    print(f"Resetting tree '{peano_tree._name}' - moving all nodes to subtree 0")
    
    # Reset all nodes in all levels to subtree 0
    for level in range(peano_tree._max_level):
        size = 3**(level + 1)
        
        if peano_tree._dimension == 2:
            for a in range(size):
                for b in range(size):
                    current_node = peano_tree._tree_arrays[level][a][b]
                    if current_node[0] == 1:  # Node exists (weight = 1)
                        # Reset to subtree 0 while preserving weight
                        peano_tree._tree_arrays[level][a][b] = (1, 0)
                        
        elif peano_tree._dimension == 3:
            for a in range(size):
                for b in range(size):
                    for c in range(size):
                        current_node = peano_tree._tree_arrays[level][a][b][c]
                        if current_node[0] == 1:  # Node exists (weight = 1)
                            # Reset to subtree 0 while preserving weight
                            peano_tree._tree_arrays[level][a][b][c] = (1, 0)
    
    # Update tree metadata after reset
    peano_tree._subtree_count = 1
    peano_tree.update_split_pattern()
    
    print(f"Reset completed. All {int(peano_tree._leaf_count)} nodes now belong to subtree 0")
    print(f"New split pattern: {peano_tree._split_pattern}")
    
    return peano_tree

def extract_split_instructions(
    peano_tree: Tree, base_sweep: int = 2
) -> tuple[list[int], list[int], list[int], list[int]]:
    """
    Stage IV of the staged load-balancing workflow.

    Derive constructor lists for Peano's hardcoded load balancer from a split tree.

    The function scans the tree arrays and reconstructs a sequence of split operations
    to replay the observed decomposition with
    [`toolbox::loadbalancing::strategies::Hardcoded`](src/toolbox/loadbalancing/strategies/Hardcoded.h).

    An operation is emitted whenever, on some level, a cell with `subtree_id > 0` exists
    whose parent exists and has a different subtree id (i.e. the subtree gets introduced
    across a parent–child edge). Level 0 has no parent; any cells with `subtree_id > 0`
    on level 0 are treated as introduced from subtree 0.

    Mapping:
    - `gridSweeps[level] = base_sweep * (level + 1)` (levels are 0-based in the API arrays)
    - `splittingTrees` is the unique parent subtree id for all introduced cells on that level,
      or 0 if ambiguous
    - `numberOfCells` is the count of introduced cells on that level
    - `subTrees` are subtree ids (not MPI ranks); map them to destination ranks separately

    Args:
        peano_tree: Input tree with subtree assignments.
        base_sweep: Base sweep index used to generate `gridSweeps` (default: 2).

    Returns:
        Tuple `(gridSweeps, splittingTrees, numberOfCells, subTrees)` suitable for
        building a `Hardcoded` configuration.

    Note:
        This helper is intended for *offline replay* of an already-computed split pattern.
        It does not modify the tree.
    """
    if peano_tree._subtree_count <= 1:
        print("No splits to extract - tree has only one subtree")
        return ([], [], [], [])

    dim = peano_tree._dimension
    max_level = peano_tree._max_level

    def count_cells_on_level(subtree_id: int, level: int) -> int:
        """Count existing cells (weight==1) on a given level that belong to subtree_id."""
        size = 3 ** (level + 1)
        count = 0
        if dim == 2:
            for a in range(size):
                for b in range(size):
                    w, s = peano_tree._tree_arrays[level][a][b]
                    if w == 1 and s == subtree_id:
                        count += 1
        else:
            for a in range(size):
                for b in range(size):
                    for c in range(size):
                        w, s = peano_tree._tree_arrays[level][a][b][c]
                        if w == 1 and s == subtree_id:
                            count += 1
        return count

    def parent_subtrees_for_introduced_cells(subtree_id: int, level: int) -> tuple[int, set[int]]:
        """
        Return (count, parent_ids) for cells of subtree_id at given level that are
        'introduced' wrt their parent: parent exists and parent_subtree != subtree_id.
        """
        if level <= 0:
            return (0, set())

        size = 3 ** (level + 1)
        count = 0
        parent_ids: set[int] = set()

        if dim == 2:
            for a in range(size):
                for b in range(size):
                    w, s = peano_tree._tree_arrays[level][a][b]
                    if w != 1 or s != subtree_id:
                        continue
                    pa, pb = a // 3, b // 3
                    pw, ps = peano_tree._tree_arrays[level - 1][pa][pb]
                    if pw == 1 and ps != subtree_id:
                        count += 1
                        parent_ids.add(ps)
        else:
            for a in range(size):
                for b in range(size):
                    for c in range(size):
                        w, s = peano_tree._tree_arrays[level][a][b][c]
                        if w != 1 or s != subtree_id:
                            continue
                        pa, pb, pc = a // 3, b // 3, c // 3
                        pw, ps = peano_tree._tree_arrays[level - 1][pa][pb][pc]
                        if pw == 1 and ps != subtree_id:
                            count += 1
                            parent_ids.add(ps)

        return (count, parent_ids)

    split_operations = []

    # For each subtree, check all levels for new "introductions" across parent-child edges.
    for target_subtree in range(1, peano_tree._subtree_count):
        # Level 0: no parent edge exists, but a subtree can already be present there.
        moved_level0 = count_cells_on_level(target_subtree, level=0)
        if moved_level0 > 0:
            split_operations.append(
                {
                    "grid_sweep": base_sweep * (0 + 1),  # == base_sweep
                    "source_subtree": 0,
                    "target_subtree": target_subtree,
                    "num_cells": moved_level0,
                    "level": 0,
                }
            )

        # Levels >= 1: introduced across parent-child edges
        for level in range(1, max_level):
            moved_cells, parent_ids = parent_subtrees_for_introduced_cells(target_subtree, level)
            if moved_cells == 0:
                continue

            source_subtree = next(iter(parent_ids)) if len(parent_ids) == 1 else 0
            grid_sweep = base_sweep * (level + 1)

            split_operations.append(
                {
                    "grid_sweep": grid_sweep,
                    "source_subtree": source_subtree,
                    "target_subtree": target_subtree,
                    "num_cells": moved_cells,
                    "level": level,
                }
            )

    split_operations.sort(key=lambda op: (op["grid_sweep"], op["target_subtree"], op["level"]))

    gridSweeps = [op["grid_sweep"] for op in split_operations]
    splittingTrees = [op["source_subtree"] for op in split_operations]
    numberOfCells = [op["num_cells"] for op in split_operations]
    subTrees = [op["target_subtree"] for op in split_operations]

    return (gridSweeps, splittingTrees, numberOfCells, subTrees)
