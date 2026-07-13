# This file is part of the ExaHyPE2 project. For conditions of decomposition and
# use, please see the copyright notice at www.peano-framework.org

from ..Tree import Tree
from .Utils import no_split_result


def equidistant_splitting(peano_tree: Tree,
                          number_of_subtrees: int,
                          verbose=False,
                          ):
    
    if number_of_subtrees <= 1:
        return no_split_result(
            f"No splitting requested (number_of_subtrees = {number_of_subtrees})."
        )

    total_leaves = peano_tree.number_of_leaves
    
    if total_leaves == 0:
        print("No leaf nodes found in subtree 0 to split.")
        return (False, [])

    # Calculate how many leaves each subtree should get for balanced decomposition
    leaves_per_subtree = total_leaves // (number_of_subtrees)
    remaining_leaves = total_leaves % (number_of_subtrees)

    # Create decomposition plan
    decomposition = []
    for i in range(number_of_subtrees):
        target_count = leaves_per_subtree + (1 if i < remaining_leaves else 0)
        decomposition.append(target_count)

    if verbose:
        print(
          f"Splitting tree '{peano_tree._name}' into "
          f"{number_of_subtrees} subtrees (staged workflow)"
        )
        print(f"Found {total_leaves} leaf nodes across all levels for decomposition")
        print(f"Target decomposition: {leaves_per_subtree} leaves per subtree, {remaining_leaves} extra leaves that have to be distributed among other trees")
        print(f"Decomposition plan: {decomposition}")
    
    return decomposition


def quantised_splitting(peano_tree: Tree,
                        number_of_subtrees: int,
                        dimension: int,
                        verbose=False,
                       ):
    """!
    
    Quantised splitting
    
    This splitting starts from a perfect splitting as we find it in
    equidistant_splitting(), but then it picks partitions of the type
    @f$ 1 \\cdot 3^{kd},  2 \\cdot 3^{kd}, ...,  (3^d-1) \\cdot 3^{kd}, 1 \\cdot 3^{(k+1)d}, 2 \\cdot 3^{(k+1)d}, ... @f$
    
    ## Implementation
    
    - Determine optimal number of splittings
    - Introduce a residual counter. As the number of leaves typically does not 
      exactly fit to number_of_subtrees, this is the remainder of the modulo
      division.
    - Identify smallest chunk from sequence above that literally just fits.
    - Add the cells that should have been added to this chunk (due to the 
      optimal decomposition) but cannot be added (due to the quantisation from
      the sequence above) to the residual.
    - Next step, increase the optimum threshold by the residual.
      
    """
    
    if number_of_subtrees <= 1:
        return no_split_result(
            f"No splitting requested (number_of_subtrees = {number_of_subtrees})."
        )

    total_leaves = peano_tree.number_of_leaves
    
    if total_leaves == 0:
        print("No leaf nodes found in subtree 0 to split.")
        return (False, [])

    # Calculate how many leaves each subtree should get for balanced decomposition
    optimal_leaves_per_subtree = total_leaves // (number_of_subtrees)
    residual_leaves            = total_leaves % optimal_leaves_per_subtree
    
    # Create decomposition plan
    decomposition = []
    for i in range(number_of_subtrees):
        current_chunk     = 0
        current_step_size = 3**dimension
        while (current_chunk<optimal_leaves_per_subtree+residual_leaves):
            if current_chunk == current_step_size * 3**dimension:
                current_step_size = current_step_size * 3**dimension
            current_chunk = current_chunk + current_step_size
        current_chunk = current_chunk - current_step_size
        residual_leaves = residual_leaves + optimal_leaves_per_subtree - current_chunk
        assert residual_leaves>=0, "residual leaves = {}".format(residual_leaves)
        decomposition.append(current_chunk)
        print(f"append {current_chunk} and maintain residual {residual_leaves} with optimum {optimal_leaves_per_subtree}. New target {optimal_leaves_per_subtree+residual_leaves}")

    decomposition[-1] = decomposition[-1] + residual_leaves
    
    assert sum(decomposition)==total_leaves, "{} vs {}".format(sum(decomposition),total_leaves)

    if verbose:
        print(
          f"Splitting tree '{peano_tree._name}' into "
          f"{number_of_subtrees} subtrees (staged workflow)"
        )
        print(f"Found {total_leaves} leaf nodes across all levels for decomposition")
        print(f"Target decomposition: {optimal_leaves_per_subtree} leaves per subtree")
        print(f"Decomposition plan: {decomposition}")
    
    return decomposition
