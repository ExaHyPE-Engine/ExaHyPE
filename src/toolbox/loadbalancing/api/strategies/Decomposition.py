# This file is part of the ExaHyPE2 project. For conditions of decomposition and
# use, please see the copyright notice at www.peano-framework.org

from Tree import Tree
from Utils import no_split_result


def equidistant_splitting(peano_tree: Tree,
                          number_of_subtrees: int,
                          verbose=False,
                          ):
    
    if number_of_subtrees <= 1:
        return no_split_result(
            f"No splitting requested (number_of_subtrees = {number_of_subtrees})."
        )

    print(
        f"Splitting tree '{peano_tree._name}' into "
        f"{number_of_subtrees} subtrees (staged workflow)"
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
        print(f"Found {total_leaves} leaf nodes across all levels for decomposition")
        print(f"Target decomposition: {leaves_per_subtree} leaves per subtree, {remaining_leaves} extra leaves that have to be distributed among other trees")
        print(f"Decomposition plan: {decomposition}")
    
    return decomposition
