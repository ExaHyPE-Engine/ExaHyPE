# This file is part of the Peano load balancing toolbox. For conditions of 
# distribution and use, please see the copyright notice at www.peano-framework.org
import collections.abc
from typing import TYPE_CHECKING, overload

if TYPE_CHECKING:
    from toolbox.loadbalancing.api.Tree import Tree


def assign_round_Robin_rank_assignment(number_of_subtrees,
                                       max_ranks,
                                       ):
    result = []
    for i in range(0,len(number_of_subtrees)):
        result.append( i%max_ranks )
    return result
    
    
def _format_initialisation_lists(sweep_numbers,
                                 parent_trees,
                                 subpartition_sizes,
                                 destination_ranks,
                                 ):
    """!
    @param sweep_numbers: [Integer] or Integer
      Has to be either a sequence of integers or a single one. If it is a 
      single one, this means that all splits happen in the same sweep. If 
      it is a sequence, its length has to match the length of 
      subpartition_sizes.
      
    @param parent_trees: [Integer] or 0
      Is a sequence describing which trees fork at which point. You can also 
      pass in a straight 0 meaning that it is always only tree 0 which splits.
      
    @param subpartition_sizes: [Integer]
      Size of the individual splits.
      
    @param destination_ranks: [Integer]
      The size of the destination trees has to match subpartition_sizes.
    """
    assert not isinstance(sweep_numbers, collections.abc.Sequence) or len(subpartition_sizes)==len(sweep_numbers) 
    assert not isinstance(parent_trees, collections.abc.Sequence) or parent_trees==0
    assert len(destination_ranks)==len(subpartition_sizes)
    
    sweep_number_string       = "{"
    parent_tree_string        = "{"
    subpartition_sizes_string = "{"
    destination_string        = "{"
    
    for i in range(0,len(subpartition_sizes)):
        if i>0:
            sweep_number_string       += ", "
            parent_tree_string        += ", "
            subpartition_sizes_string += ", "
            destination_string        += ", "
          
        if isinstance(sweep_numbers, collections.abc.Sequence):
            sweep_number_string += str( sweep_numbers[i] )
        else:
            sweep_number_string += str( sweep_numbers )

        if isinstance(parent_trees, collections.abc.Sequence):
            parent_tree_string += str( parent_trees[i] )
        else:
            parent_tree_string += str( parent_trees )
            
        subpartition_sizes_string += str( subpartition_sizes[i] )
        
        destination_string += str(destination_ranks[i])
     
    sweep_number_string       += "}"
    parent_tree_string        += "}"
    subpartition_sizes_string += "}"
    destination_string        += "}"

    return (sweep_number_string + ", "+
            parent_tree_string + ", " + 
            subpartition_sizes_string + ", " + 
            destination_string
           )


def _tree_start_level(peano_tree, subtree_id):
    for level in range(peano_tree._max_level):
        size = 3 ** (level + 1)
        if peano_tree._dimension == 2:
            for a in range(size):
                for b in range(size):
                    weight, current_subtree = peano_tree._tree_arrays[level][a][b]
                    if weight == 1 and current_subtree == subtree_id:
                        return level
        else:
            for a in range(size):
                for b in range(size):
                    for c in range(size):
                        weight, current_subtree = peano_tree._tree_arrays[level][a][b][c]
                        if weight == 1 and current_subtree == subtree_id:
                            return level

    raise ValueError(f"subtree {subtree_id} not found in tree '{peano_tree._name}'")


def _infer_hardcoded_mode(peano_tree, subtrees, destination_ranks):
    start_levels_per_rank = {}

    for subtree_id, destination_rank in zip(subtrees, destination_ranks):
        start_level = _tree_start_level(peano_tree, subtree_id)
        start_levels_per_rank.setdefault(destination_rank, set()).add(start_level)

    for start_levels in start_levels_per_rank.values():
        if len(start_levels) > 1:
            return "peano4::SplitInstruction::Mode::BottomUp"

    return "peano4::SplitInstruction::Mode::AggressiveTopDown"
    
    
@overload
def create_constructor_initialisation_string(
    peano_tree: "Tree",
    destination_ranks: list[int],
    output: bool = False,
) -> tuple[str, str]:
    ...


@overload
def create_constructor_initialisation_string(
    peano_tree,
    destination_ranks,
    output,
    *legacy_args,
) -> str:
    ...


def create_constructor_initialisation_string(peano_tree,
                                             destination_ranks,
                                             output=False,
                                             *legacy_args,
                                            ) -> str | tuple[str, str]:
    """!
        Construct the constructor arguments for an instance of Hardcoded, together with
        the inferred split mode.

        The mode is chosen as follows (see image below):
        - `AggressiveTopDown`: all subtrees assigned to rank k start on the same level l.
            The split (iteration) number then corresponds to level l.
        - `BottomUp`: subtrees assigned to rank k start on different levels.
            The split number then corresponds to the coarsest level at which all subtrees
            belonging to rank k are present.

        @image html src/toolbox/loadbalancing/api/output/create_constructor_initialisation_string.png

        @todo Dmitry: the function is overloaded to preserve backwards compatibility with the
            legacy 4-argument call in `benchmarks/mghype/poisson/benchmarks.py`.
            It may make sense to migrate that caller to the tree-based API and remove the overload.

        @return `(hardcoded_config, hardcoded_mode)` when called with a Tree instance;
            a plain format string when called with the legacy 4-argument signature.
    """
    # Backward compatibility path:
    # create_constructor_initialisation_string(sweep_numbers, parent_trees, subpartition_sizes, destination_ranks)
    if not hasattr(peano_tree, "_tree_arrays"):
        if isinstance(output, bool):
            raise ValueError(
                "Legacy call requires four positional arguments: "
                "(sweep_numbers, parent_trees, subpartition_sizes, destination_ranks)"
            )
        if len(legacy_args) != 1:
            raise ValueError(
                "Legacy call requires exactly four positional arguments"
            )

        sweep_numbers = peano_tree
        parent_trees = destination_ranks
        subpartition_sizes = output
        destination_ranks = legacy_args[0]
        return _format_initialisation_lists(
            sweep_numbers,
            parent_trees,
            subpartition_sizes,
            destination_ranks,
        )

    if legacy_args:
        raise ValueError("Tree-based call does not accept additional positional arguments")

    from toolbox.loadbalancing.api.strategies.SplitTrees import extract_split_instructions

    sweep_numbers, parent_trees, subpartition_sizes, subtrees = extract_split_instructions(peano_tree)

    if output:
        print("Extracted hardcoded parameters:")
        print("  grid_sweeps     =", sweep_numbers)
        print("  splitting_trees =", parent_trees)
        print("  number_of_cells =", subpartition_sizes)
        print("  subtrees       =", subtrees)

    if len(destination_ranks) != len(subtrees):
        raise ValueError(
            "destination_ranks must have the same length as the extracted split instructions"
        )

    hardcoded_mode = _infer_hardcoded_mode(peano_tree, subtrees, destination_ranks)
    hardcoded_config = "{" + _format_initialisation_lists(
        sweep_numbers,
        parent_trees,
        subpartition_sizes,
        destination_ranks,
    ) + ", " + hardcoded_mode + "}"
    return hardcoded_config, hardcoded_mode

