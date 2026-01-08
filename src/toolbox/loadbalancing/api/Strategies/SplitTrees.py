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

def split_leaves(peano_tree: Tree, number_of_splits: int) -> Tree:
    """
    Split a tree into the specified number of splits by distributing ALL leaf nodes.
    This includes leaves at the finest level AND leaves at coarser levels (nodes that weren't refined further).
    
    ## Algorithm Overview
    
    The splitting algorithm operates in two stages:
    1. **Leaf Distribution**: Identify and redistribute all leaf nodes across subtrees
    2. **Topology Preservation**: Update non-leaf nodes to maintain tree hierarchy constraints
    
    ## Leaf Node Identification
    
    A node is considered a leaf if:
    - It exists at the finest level (always a leaf), OR
    - It exists at a coarser level AND has no children in the next level
    
    ## Distribution Strategy
    
    - Uses round-robin assignment to ensure balanced distribution
    - Handles remainder nodes by assigning extras to first subtrees
    - Preserves spatial ordering through sequential array traversal
    
    ## Technical Constraints
    
    - Maintains Peano's tree topology constraint during Stage II
    - Only redistributes nodes originally in subtree 0
    - Preserves node weights (existence flags) throughout
    
    Args:
        peano_tree: Tree object to split
        number_of_splits: Number of subtrees to create (total subtrees = original + number_of_splits)
        
    Returns:
        Modified Tree object with applied splits
        
    Note:
        This function modifies the input tree in-place and calls _update_non_leaf_assignments()
        to ensure tree topology constraints are satisfied.
    """
    if number_of_splits <= 0:
        print(f"No splitting needed (number_of_splits = {number_of_splits}).")
        return peano_tree
        
    print(f"Splitting tree '{peano_tree._name}' into {number_of_splits + 1} subtrees using ALL leaves")
    
    # Collect all leaf nodes (at all levels) that belong to subtree 0
    leaf_nodes = []
    
    for level in range(peano_tree._max_level):
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
        return peano_tree
    
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
    
    # Update non-leaf nodes to follow the tree topology constraint:
    # If the parent and at least one child of a node are both associated to the same subtree,
    # then this non-leaf node must be associated to that subtree as well
    _update_non_leaf_assignments(peano_tree)
    
    # Update tree metadata after all splits
    peano_tree._subtree_count = number_of_splits + 1
    peano_tree.update_split_pattern()
    
    print(f"Leaf splitting completed. New subtree count: {peano_tree._subtree_count}")
    print(f"Actual distribution: {moved_nodes}")
    print(f"New split pattern: {peano_tree._split_pattern}")
    
    return peano_tree


def _update_non_leaf_assignments(peano_tree: Tree) -> None:
    """
    Update non-leaf node assignments to follow tree topology constraints.

    ## Algorithm Details
    
    This function implements Peano's fundamental tree topology constraint:
    "If all children of a node belong to the same subtree, the parent should also belong to that subtree."
    
    ## Processing Order
    
    - Processes levels from finest to coarsest (bottom-up)
    - Skips the finest level (contains only leaves)
    - Uses 3:1 refinement ratio for child-parent relationships
    
    ## Parent-Child Mapping
    
    For a parent node at coordinates (a,b) on level L:
    - Children are located at (3*a to 3*(a+1)-1, 3*b to 3*(b+1)-1) on level L+1
    - For 3D: extends to (3*c to 3*(c+1)-1) in third dimension
    
    ## Assignment Logic
    
    - Collects subtree IDs of all existing children
    - If all children belong to same subtree → assign parent to that subtree
    - If children span multiple subtrees → parent assignment unchanged
    
    ## Constraints Preserved
    
    - Only modifies nodes where ALL children have identical subtree assignments
    - Respects existing node weights (never creates or destroys nodes)
    - Maintains spatial relationships within tree hierarchy
    
    Args:
        peano_tree: Tree object to update
        
    Note:
        This function is called automatically by split_leaves() and should generally
        not be called directly unless implementing custom splitting strategies.
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
    Extract hardcoded split parameters from a tree for Peano's Hardcoded load balancer.
    
    This function analyzes the split pattern in a tree and returns tuple parameters
    required for the Hardcoded load balancer constructor:
    - gridSweeps: Grid sweep numbers when to perform each split (even numbers)
    - splittingTrees: Source subtree IDs for each split
    - numberOfCells: How many cells to move in each split
    - subTrees: Target subtree IDs for each split (needs mapping to destinationRanks)
    
    Args:
        peano_tree: Tree object with applied splits
        base_sweep: Base grid sweep number for split timing (default: 2)
        
    Interpretation (non-leaf-centric):
    - We do NOT count leaf cells.
    - For each subtree > 0 we infer the split level as the earliest level where
      any cell with this subtree id first appears.
    - The numberOfCells for that split equals the number of cells with this
      subtree id on that earliest level (we treat them as newly moved cells in
      that grid sweep, ignoring any deeper descendants that may be discovered
      in later sweeps).
    - Children discovered in subsequent traversals inherit the same subtree id
      until a further split introduces a new subtree.
    
    Source subtree inference:
    - For all cells of the target subtree at its earliest appearance level,
      we collect the parent subtree ids (previous level, indices // 3).
    - If exactly one unique parent subtree id exists, that id is used as
      splittingTrees entry.
    - Otherwise we fall back to 0.
    - If the earliest level is 0 (root level) we set source subtree to 0.
    
    Grid sweep numbering:
    - Uses base_sweep * (earliest_appearance_level + 1).
      (So, for max_level=3, the finest level has index 2, and thus grid sweep = base_sweep * 3.)

    Returns:
        (gridSweeps, splittingTrees, numberOfCells, subTrees) with subTrees
        holding the actual target subtree ids (not renumbered 1..N).
    """
    if peano_tree._subtree_count <= 1:
        print("No splits to extract - tree has only one subtree")
        return ([], [], [], [])
    
    def earliest_level(subtree_id: int) -> int | None:
        for level in range(peano_tree._max_level):
            size = 3**(level + 1)
            if peano_tree._dimension == 2:
                for a in range(size):
                    for b in range(size):
                        w, s = peano_tree._tree_arrays[level][a][b]
                        if w == 1 and s == subtree_id:
                            return level
            else:
                for a in range(size):
                    for b in range(size):
                        for c in range(size):
                            w, s = peano_tree._tree_arrays[level][a][b][c]
                            if w == 1 and s == subtree_id:
                                return level
        return None
    
    def count_cells_on_level(subtree_id: int, level: int) -> int:
        size = 3**(level + 1)
        count = 0
        if peano_tree._dimension == 2:
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
    
    def infer_source_subtree(subtree_id: int, level: int) -> int:
        if level == 0:
            return 0
        parent_ids = set()
        size = 3**(level + 1)
        if peano_tree._dimension == 2:
            for a in range(size):
                for b in range(size):
                    w, s = peano_tree._tree_arrays[level][a][b]
                    if w == 1 and s == subtree_id:
                        pa, pb = a // 3, b // 3
                        pw, ps = peano_tree._tree_arrays[level - 1][pa][pb]
                        if pw == 1:
                            parent_ids.add(ps)
        else:
            for a in range(size):
                for b in range(size):
                    for c in range(size):
                        w, s = peano_tree._tree_arrays[level][a][b][c]
                        if w == 1 and s == subtree_id:
                            pa, pb, pc = a // 3, b // 3, c // 3
                            pw, ps = peano_tree._tree_arrays[level - 1][pa][pb][pc]
                            if pw == 1:
                                parent_ids.add(ps)
        return parent_ids.pop() if len(parent_ids) == 1 else 0
    
    split_operations = []
    
    for target_subtree in range(1, peano_tree._subtree_count):
        level = earliest_level(target_subtree)
        if level is None:
            continue
        moved_cells = count_cells_on_level(target_subtree, level)
        if moved_cells == 0:
            continue
        source_subtree = infer_source_subtree(target_subtree, level)

        # Convention: schedule the split in the sweep corresponding to the level
        # where the target subtree first appears.
        # level is 0-based (0..max_level-1), while the "level number" is (level+1).
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
    
    gridSweeps      = [op["grid_sweep"] for op in split_operations]
    splittingTrees  = [op["source_subtree"] for op in split_operations]
    numberOfCells   = [op["num_cells"] for op in split_operations]
    subTrees        = [op["target_subtree"] for op in split_operations]
    
    return (gridSweeps, splittingTrees, numberOfCells, subTrees)
