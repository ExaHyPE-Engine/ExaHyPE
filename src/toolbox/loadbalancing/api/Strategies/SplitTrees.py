import yaml
from Tree import Tree

# to a separate file
def split_finest_level(peano_tree: Tree, number_of_splits: int) -> Tree: # might be goot for pipeline, peano tree in/out
    """
    Split a tree into the specified number of splits.
    Currently supports splitting on the finest level only.
    It distributes nodes as evenly as possible across subtrees.
    
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
    
    # Count total nodes at finest level belonging to subtree 0 (assuming all start there)
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
        print("No nodes found at finest level in subtree 0 to split.")
        return peano_tree
    
    print(f"Found {total_nodes} nodes at finest level to distribute")
    
    # Calculate how many nodes each subtree should get for balanced distribution
    nodes_per_subtree = total_nodes // (number_of_splits + 1)
    remaining_nodes = total_nodes % (number_of_splits + 1)
    
    print(f"Target distribution: {nodes_per_subtree} nodes per subtree, {remaining_nodes} extra nodes")
    
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

# def generate_split_yaml(peano_tree: Tree, output_yaml_file: str) -> None:
#     """
#     Generate a YAML file describing the split pattern of the tree.
#     (dump a "yaml" (or whatever) file (for Peano, different format))
    
#     Args:
#         peano_tree: Tree object to generate YAML from
#         output_yaml_file: Path to the output YAML file
        
#     Returns:
#         None
#     """
#     split_data = {
#         'name': peano_tree._name,
#         'dimension': peano_tree._dimension,
#         'max_level': peano_tree._max_level,
#         'subtree_count': peano_tree._subtree_count,
#         'split_pattern': peano_tree._split_pattern
#     }
    
#     try:
#         with open(output_yaml_file, 'w') as file:
#             yaml.dump(split_data, file, default_flow_style=False)
#         print(f"Split pattern written to {output_yaml_file}")
#     except IOError as e:
#         print(f"Error writing to YAML file: {e}")

# def _apply_finest_level_split(peano_tree, level, source_subtree, target_subtree, nodes_to_move):
#     """
#     Apply a single split operation at the finest level only.
    
#     Args:
#         peano_tree: Tree object
#         level: Level to apply split (should be finest level)
#         source_subtree: Source subtree ID
#         target_subtree: Target subtree ID  
#         nodes_to_move: Number of nodes to move
        
#     Returns:
#         Number of nodes actually moved
#     """
#     moved_count = 0
#     size = 3**(level + 1)
    
#     if peano_tree._dimension == 2:
#         # 2D case
#         for a in range(size):
#             for b in range(size):
#                 if moved_count >= nodes_to_move:
#                     break
                    
#                 # Check if this node belongs to the source subtree
#                 current_node = peano_tree._tree_arrays[level][a][b]
#                 if current_node[0] == 1 and current_node[1] == source_subtree:
#                     # Move this node to the target subtree
#                     peano_tree._tree_arrays[level][a][b] = (1, target_subtree)
#                     moved_count += 1
            
#             if moved_count >= nodes_to_move:
#                 break
                
#     elif peano_tree._dimension == 3:
#         # 3D case
#         for a in range(size):
#             for b in range(size):
#                 for c in range(size):
#                     if moved_count >= nodes_to_move:
#                         break
                        
#                     # Check if this node belongs to the source subtree
#                     current_node = peano_tree._tree_arrays[level][a][b][c]
#                     if current_node[0] == 1 and current_node[1] == source_subtree:
#                         # Move this node to the target subtree
#                         peano_tree._tree_arrays[level][a][b][c] = (1, target_subtree)
#                         moved_count += 1
                
#                 if moved_count >= nodes_to_move:
#                     break
            
#             if moved_count >= nodes_to_move:
#                 break
    
#     return moved_count

# def _count_unique_subtrees(peano_tree):
#     """Count the number of unique subtrees in the tree"""
#     subtree_ids = set()
    
#     for level in range(peano_tree._max_level):
#         size = 3**(level + 1)
        
#         if peano_tree._dimension == 2:
#             for a in range(size):
#                 for b in range(size):
#                     current_node = peano_tree._tree_arrays[level][a][b]
#                     if current_node[0] == 1:
#                         subtree_ids.add(current_node[1])
        
#         elif peano_tree._dimension == 3:
#             for a in range(size):
#                 for b in range(size):
#                     for c in range(size):
#                         current_node = peano_tree._tree_arrays[level][a][b][c]
#                         if current_node[0] == 1:
#                             subtree_ids.add(current_node[1])
    
#     return len(subtree_ids)
