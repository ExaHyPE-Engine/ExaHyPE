import yaml
from Tree import Tree

# to a separate file
def split_finest_level(peano_tree: Tree, number_of_splits: int) -> Tree: # might be goot for pipeline, peano tree in/out
    """
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
    
    Args:
        peano_tree: Tree object to split
        number_of_splits: Number of subtrees to create (total subtrees = original + number_of_splits)
        
    Returns:
        Modified Tree object with applied splits
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
    A non-leaf node should be assigned to the same subtree as its parent and children
    if they belong to the same subtree.
    
    Args:
        peano_tree: Tree object to update
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
    
    Args:
        peano_tree: Tree object to generate YAML from
        output_yaml_file: Path to the output YAML file
        
    Returns:
        None
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
    This returns the tree to its initial state where all nodes belong to a single subtree.
    
    Args:
        peano_tree: Tree object to reset
        
    Returns:
        Modified Tree object with all nodes assigned to subtree 0
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
