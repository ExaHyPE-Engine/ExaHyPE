# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from Tree import Tree
from .SFC import Peano_to_Cartesian_2d
from .SFC import Peano_to_Cartesian_3d
from mpmath import residual



def get_number_of_leaves(peano_tree: Tree):
    """!
    
    Find out how many leaves there are in the tree
    
    @todo Dmitry I think we can slim down this routine quite a lot
    
    """
    leaf_nodes = []

    level_range = range(peano_tree._max_level)
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
    return total_leaves


def equidistant_splitting(peano_tree: Tree,
                          number_of_subtrees: int,
                          verbose=False,
                          ):
    
    if number_of_subtrees <= 1:
        return _no_split_result(
            f"No splitting requested (number_of_subtrees = {number_of_subtrees})."
        )

    total_leaves = get_number_of_leaves(peano_tree)
    
    if total_leaves == 0:
        print("No leaf nodes found in subtree 0 to split.")
        return (False, [])

    # Calculate how many leaves each subtree should get for balanced distribution
    leaves_per_subtree = total_leaves // (number_of_subtrees)
    remaining_leaves = total_leaves % (number_of_subtrees)

    # Create distribution plan
    distribution = []
    for i in range(number_of_subtrees):
        target_count = leaves_per_subtree + (1 if i < remaining_leaves else 0)
        distribution.append(target_count)

    if verbose:
        print(
          f"Splitting tree '{peano_tree._name}' into "
          f"{number_of_subtrees} subtrees (staged workflow)"
        )
        print(f"Found {total_leaves} leaf nodes across all levels to distribute")
        print(f"Target distribution: {leaves_per_subtree} leaves per subtree, {remaining_leaves} extra leaves that have to be distributed among other trees")
        print(f"Distribution plan: {distribution}")
    
    return distribution


def quantised_splitting(peano_tree: Tree,
                        number_of_subtrees: int,
                        dimension: int,
                        verbose=False,
                       ):
    
    if number_of_subtrees <= 1:
        return _no_split_result(
            f"No splitting requested (number_of_subtrees = {number_of_subtrees})."
        )

    total_leaves = get_number_of_leaves(peano_tree)
    
    if total_leaves == 0:
        print("No leaf nodes found in subtree 0 to split.")
        return (False, [])

    # Calculate how many leaves each subtree should get for balanced distribution
    optimal_leaves_per_subtree = total_leaves // (number_of_subtrees)
    residual_leaves    = 0
    
    # Create distribution plan
    distribution = []
    for i in range(number_of_subtrees):
        current_chunk = 1
        while (current_chunk<optimal_leaves_per_subtree+residual_leaves):
            current_chunk = current_chunk * (3**dimension)
        if current_chunk>3**dimension and current_chunk>optimal_leaves_per_subtree+residual_leaves:
            current_chunk = current_chunk / (3**dimension)
        residual_leaves = residual_leaves + optimal_leaves_per_subtree - current_chunk
        assert residual_leaves>=0
        distribution.append(current_chunk)
        #print(f"append {current_chunk} and maintain residual {residual_leaves} with optimum {optimal_leaves_per_subtree}. New target {optimal_leaves_per_subtree+residual_leaves}")

    distribution[-1] = distribution[-1] + (total_leaves-sum(distribution))
    
    assert sum(distribution)==total_leaves, "{} vs {}".format(sum(distribution),total_leaves)

    if verbose:
        print(f"Found {total_leaves} leaf nodes across all levels to distribute")
        print(f"Target distribution: {optimal_leaves_per_subtree} leaves per subtree")
        print(f"Distribution plan: {distribution}")
    
    return distribution
