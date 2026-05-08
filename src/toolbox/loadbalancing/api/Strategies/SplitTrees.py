import yaml
from Tree import Tree

# to a separate file
def split_finest_level(peano_tree: Tree, number_of_splits: int) -> Tree: # might be goot for pipeline, peano tree in/out
    """
    [Deprecated]
    Split a tree into the specified number of splits on the finest level only.
    It distributes leaf nodes as evenly as possible across subtrees.
    
    Args:
        peano_tree: Tree object to split
        number_of_splits: Number of subtrees to create (total subtrees = original + number_of_splits)
        
    Returns:
        Modified Tree object with applied splits
    """
    if number_of_splits <= 0:
        print(f"No splitting needed (number_of_splits = {number_of_splits}).")
        return peano_tree
        
    print(f"Splitting tree '{peano_tree._name}' into {number_of_splits + 1} subtrees")
    
    # Get the finest level
    finest_level = peano_tree._max_level - 1
    
    # Count total leaf nodes at finest level belonging to subtree 0 (assuming all start there)
    total_nodes = 0
    size = 3**(finest_level + 1)
    
    if peano_tree._dimension == 2:
        for a in range(size):
            for b in range(size):
                current_node = peano_tree._tree_arrays[finest_level][a][b]
                if current_node[0] == 1 and current_node[1] == 0:  # Node exists and belongs to subtree 0
                    total_nodes += 1
    elif peano_tree._dimension == 3:
        for a in range(size):
            for b in range(size):
                for c in range(size):
                    current_node = peano_tree._tree_arrays[finest_level][a][b][c]
                    if current_node[0] == 1 and current_node[1] == 0:  # Node exists and belongs to subtree 0
                        total_nodes += 1
    
    if total_nodes == 0:
        print("No leaf nodes found at finest level in subtree 0 to split.")
        return peano_tree
    
    print(f"Found {total_nodes} leaf nodes at finest level to distribute")
    
    # Calculate how many nodes each subtree should get for balanced distribution
    nodes_per_subtree = total_nodes // (number_of_splits + 1)
    remaining_nodes = total_nodes % (number_of_splits + 1)
    
    print(f"Target distribution: {nodes_per_subtree} leaf nodes per subtree, {remaining_nodes} extra leaf nodes")
    
    # Create distribution plan
    distribution = []
    for i in range(number_of_splits + 1):
        target_count = nodes_per_subtree + (1 if i < remaining_nodes else 0)
        distribution.append(target_count)
    
    print(f"Distribution plan: {distribution}")
    
    # Apply the redistribution
    current_subtree = 0
    current_count = 0
    moved_nodes = [0] * (number_of_splits + 1)  # Track actual moves
    
    if peano_tree._dimension == 2:
        for a in range(size):
            for b in range(size):
                current_node = peano_tree._tree_arrays[finest_level][a][b]
                if current_node[0] == 1 and current_node[1] == 0:  # Node exists and belongs to subtree 0
                    # Assign to current subtree
                    peano_tree._tree_arrays[finest_level][a][b] = (1, current_subtree)
                    moved_nodes[current_subtree] += 1
                    current_count += 1
                    
                    # Check if current subtree is full, move to next
                    if current_count >= distribution[current_subtree] and current_subtree < number_of_splits:
                        current_subtree += 1
                        current_count = 0
                        
    elif peano_tree._dimension == 3:
        for a in range(size):
            for b in range(size):
                for c in range(size):
                    current_node = peano_tree._tree_arrays[finest_level][a][b][c]
                    if current_node[0] == 1 and current_node[1] == 0:  # Node exists and belongs to subtree 0
                        # Assign to current subtree
                        peano_tree._tree_arrays[finest_level][a][b][c] = (1, current_subtree)
                        moved_nodes[current_subtree] += 1
                        current_count += 1
                        
                        # Check if current subtree is full, move to next
                        if current_count >= distribution[current_subtree] and current_subtree < number_of_splits:
                            current_subtree += 1
                            current_count = 0
    
    # Update tree metadata after all splits
    peano_tree._subtree_count = number_of_splits + 1
    peano_tree.update_split_pattern()
    
    print(f"Splitting completed. New subtree count: {peano_tree._subtree_count}")
    print(f"Actual distribution: {moved_nodes}")
    print(f"New split pattern: {peano_tree._split_pattern}")
    
    return peano_tree

def split_tree(
    peano_tree: Tree,
    number_of_splits: int,
    traversal: str = "bfs",
    optimize_hierarchy: bool = False,
) -> Tree:
    """
    Split a tree using a staged load-balancing workflow.

    Stages:
    1. _distribute_leaves_bfs() or _distribute_leaves_dfs(): Redistribute leaf nodes across subtrees.
    2. _propagate_bottom_up(): Propagate subtree ownership to internal nodes.
    3. _optimize_hierarchy() (optional, TBD): Further reduce split complexity.

    Args:
        peano_tree: Tree to split (modified in place).
        number_of_splits: Number of additional subtrees to create.
            Total subtree count after splitting is number_of_splits + 1.
        traversal: Stage I traversal strategy. Supported values:
            - "bfs": level-order leaf collection (default)
            - "bfs_finest_first": level-order leaf collection within each level,
              but levels processed from finest to coarsest
            - "dfs": depth-first leaf collection
        optimize_hierarchy: If True, request Stage III hierarchical optimisation.
            Stage III is currently not implemented and will be skipped.

    Returns:
        The same tree instance with updated subtree assignments.

    Notes:
        - Stage III is optional and currently not implemented.
        - If number_of_splits <= 0, the tree is returned unchanged.
    """
    if number_of_splits <= 0:
        print(f"No splitting requested (number_of_splits = {number_of_splits}).")
        return peano_tree

    print(
        f"Splitting tree '{peano_tree._name}' into "
        f"{number_of_splits + 1} subtrees (staged workflow)"
    )

    traversal_mode = traversal.lower()
    if traversal_mode == "bfs":
        has_leaves_to_split, moved_nodes = _distribute_leaves_bfs(
            peano_tree, number_of_splits, finest_level_first=False
        )
    elif traversal_mode == "bfs_finest_first":
        has_leaves_to_split, moved_nodes = _distribute_leaves_bfs(
            peano_tree, number_of_splits, finest_level_first=True
        )
    elif traversal_mode == "dfs":
        has_leaves_to_split, moved_nodes = _distribute_leaves_dfs(peano_tree, number_of_splits)
    else:
        raise ValueError(
            f"Unsupported traversal '{traversal}'. Use 'bfs', 'bfs_finest_first' or 'dfs'."
        )

    if not has_leaves_to_split:
        return peano_tree

    _propagate_bottom_up(peano_tree)

    if optimize_hierarchy:
        print("Stage III requested, but _optimize_hierarchy() is not implemented yet. Skipping.")

    # Update tree metadata after all stages
    peano_tree._subtree_count = number_of_splits + 1
    peano_tree.update_split_pattern()

    print(f"Splitting complete. Subtree count: {peano_tree._subtree_count}")
    print(f"Leaf distribution: {moved_nodes}")
    print(f"Split pattern: {peano_tree._split_pattern}")

    return peano_tree


def _distribute_leaves_bfs(
    peano_tree: Tree,
    number_of_splits: int,
    finest_level_first: bool = False,
) -> tuple[bool, list[int]]:
    """
    Stage I BFS variant of the staged load-balancing workflow.

    Collect leaf cells currently assigned to subtree 0 and redistribute them
    across `number_of_splits + 1` subtrees as evenly as possible while
    preserving level-order traversal within each level.

    Leaf definition:
    - Any existing cell on the finest level, or
    - Any existing coarser-level cell with no children on the next level.

    Args:
        peano_tree: Tree to update (modified in place).
        number_of_splits: Number of additional subtrees to create.
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
            for a in range(size):
                for b in range(size):
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
            for a in range(size):
                for b in range(size):
                    for c in range(size):
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
    leaves_per_subtree = total_leaves // (number_of_splits + 1)
    remaining_leaves = total_leaves % (number_of_splits + 1)

    print(f"Target distribution: {leaves_per_subtree} leaves per subtree, {remaining_leaves} extra leaves")

    # Create distribution plan
    distribution = []
    for i in range(number_of_splits + 1):
        target_count = leaves_per_subtree + (1 if i < remaining_leaves else 0)
        distribution.append(target_count)

    print(f"Distribution plan: {distribution}")

    # Apply the redistribution to leaf nodes
    current_subtree = 0
    current_count = 0
    moved_nodes = [0] * (number_of_splits + 1)  # Track actual moves

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
        if current_count >= distribution[current_subtree] and current_subtree < number_of_splits:
            current_subtree += 1
            current_count = 0

    return (True, moved_nodes)


def _distribute_leaves_dfs(peano_tree: Tree, number_of_splits: int) -> tuple[bool, list[int]]:
    """
    Stage I variant using depth-first traversal order.

    Collect leaf cells currently assigned to subtree 0 and redistribute them
    across `number_of_splits + 1` subtrees as evenly as possible.
    The redistribution logic is identical to `_distribute_leaves_bfs()`, but leaf
    discovery order follows DFS instead of level-order scanning.

    Args:
        peano_tree: Tree to update (modified in place).
        number_of_splits: Number of additional subtrees to create.

    Returns:
        A tuple `(has_leaves_to_split, moved_nodes_per_subtree)`:
        - `has_leaves_to_split`: `False` if no eligible leaves were found.
        - `moved_nodes_per_subtree`: Per-subtree counts of reassigned leaves.
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

        child_size = 3 ** (level + 2)
        children = []
        has_existing_children = False
        for child_a in range(3 * a, min(3 * (a + 1), child_size)):
            for child_b in range(3 * b, min(3 * (b + 1), child_size)):
                if _node_exists(level + 1, child_a, child_b):
                    has_existing_children = True
                    if _node_subtree(level + 1, child_a, child_b) == 0:
                        children.append((child_a, child_b))

        return (has_existing_children, children)

    def _children_3d(level: int, a: int, b: int, c: int) -> tuple[bool, list[tuple[int, int, int]]]:
        if level == max_level - 1:
            return (False, [])

        child_size = 3 ** (level + 2)
        children = []
        has_existing_children = False
        for child_a in range(3 * a, min(3 * (a + 1), child_size)):
            for child_b in range(3 * b, min(3 * (b + 1), child_size)):
                for child_c in range(3 * c, min(3 * (c + 1), child_size)):
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
    for level in range(max_level):
        size = 3 ** (level + 1)

        if peano_tree._dimension == 2:
            for a in range(size):
                for b in range(size):
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
            for a in range(size):
                for b in range(size):
                    for c in range(size):
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

    leaves_per_subtree = total_leaves // (number_of_splits + 1)
    remaining_leaves = total_leaves % (number_of_splits + 1)

    print(f"Target distribution: {leaves_per_subtree} leaves per subtree, {remaining_leaves} extra leaves")

    distribution = []
    for i in range(number_of_splits + 1):
        target_count = leaves_per_subtree + (1 if i < remaining_leaves else 0)
        distribution.append(target_count)

    print(f"Distribution plan: {distribution}")

    current_subtree = 0
    current_count = 0
    moved_nodes = [0] * (number_of_splits + 1)

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

        if current_count >= distribution[current_subtree] and current_subtree < number_of_splits:
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
        - Intended to run after Stage I (`_distribute_leaves_bfs()` or `_distribute_leaves_dfs()`).
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

def extract_hardcoded_split_parameters(peano_tree: Tree, base_sweep: int = 2) -> tuple:
    """
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
