# This file is part of the Peano load balancing toolbox. For conditions of 
# distribution and use, please see the copyright notice at www.peano-framework.org
import collections.abc
from typing import TYPE_CHECKING, overload

if TYPE_CHECKING:
    from toolbox.loadbalancing.api.Tree import Tree


def assign_trees_to_ranks_round_Robin(number_of_subtrees: int,
                                      max_ranks: int,
                                      ):
    """!
    
    Take the trees and assign them to the ranks in a round-robin fashion
    
    This is likely not the best option in many cases. In many cases, it 
    might be better to assign the trees in chunks to preserve some 
    locality.
    
    Note that we implicitly assume that the first tree remains on rank 0,
    so we really only assign number_of_subtrees-1 entries.    
    
    """
    result = []
    for i in range(1,number_of_subtrees):
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
    # @todo DMitry I comment this out, as I got errors otherwise
    #assert not isinstance(parent_trees, collections.abc.Sequence) or parent_trees==0
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


def _infer_hardcoded_mode(subtrees):
    """!
    Infer split mode(s) from extracted split operations.

    The mode decision is made per subtree independently:
    - If a subtree appears in split instructions on exactly one level,
      `AggressiveTopDown` is sufficient.
    - If it appears on multiple levels, all instructions for that subtree use
      `BottomUp`.

    The result is either a single mode token (all splits same mode) or a C++
    initializer-list expression with one mode per split operation.
    """
    if not subtrees:
        return "peano4::SplitInstruction::Mode::AggressiveTopDown"

    occurrences_per_subtree = {}
    for subtree_id in subtrees:
        occurrences_per_subtree[subtree_id] = occurrences_per_subtree.get(subtree_id, 0) + 1

    mode_per_subtree = {}
    for subtree_id, occurrences in occurrences_per_subtree.items():
        if occurrences > 1:
            mode_per_subtree[subtree_id] = "peano4::SplitInstruction::Mode::BottomUp"
        else:
            mode_per_subtree[subtree_id] = "peano4::SplitInstruction::Mode::AggressiveTopDown"

    per_split_modes = [mode_per_subtree[subtree_id] for subtree_id in subtrees]
    unique_modes = set(per_split_modes)

    if len(unique_modes) == 1:
        return per_split_modes[0]

    return "{" + ", ".join(per_split_modes) + "}"
    
    
@overload
def create_constructor_initialisation_string(
    peano_tree: "Tree",
    destination_ranks: list[int],
    verbose: bool = False,
) -> tuple[str, str]:
    """!
    Docu is missing                
    """
    ...


@overload
def create_constructor_initialisation_string(
    peano_tree,
    destination_ranks,
    verbose,
    *legacy_args,
) -> str:
    ...


def create_constructor_initialisation_string(peano_tree,
                                             destination_ranks,
                                             verbose=False,
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
            
        @todo Dmitry: There's a bug here. If you run
        
        python3 tutorial_offline_load_balancing_sbh.py --trees 8 --cell-size 1.8 --load-balancing-metric offline-regular-bottom-up
        
        then you get 

Extracted hardcoded parameters:
  grid_sweeps       = [4, 4, 4, 4, 4, 4, 4, 6, 6, 6, 6, 6, 6, 6]
  splitting_trees   = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
  number_of_cells   = [90, 90, 90, 90, 90, 90, 91, 31, 31, 30, 30, 30, 30, 3]
  subtrees          = [1, 2, 3, 4, 5, 6, 7, 1, 2, 3, 4, 5, 6, 7]
  destination ranks = [0, 0, 0, 0, 0, 0, 0]
  modi              = peano4::SplitInstruction::Mode::BottomUp

        The bottom-up is correct. However it all has to be on Level 6, as the rot of node 1 
        for example is not all on the same level.


    """
    # Backward compatibility path:
    # create_constructor_initialisation_string(sweep_numbers, parent_trees, subpartition_sizes, destination_ranks)
    if not hasattr(peano_tree, "_tree_arrays"):
        if isinstance(verbose, bool):
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
        subpartition_sizes = verbose
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
    hardcoded_mode = _infer_hardcoded_mode(subtrees)

    if verbose:
        print("Extracted hardcoded parameters:")
        print("  grid_sweeps       =", sweep_numbers)
        print("  splitting_trees   =", parent_trees)
        print("  number_of_cells   =", subpartition_sizes)
        print("  subtrees          =", subtrees)
        print("  destination ranks =", destination_ranks)
        print("  modi              =", hardcoded_mode)

    if len(destination_ranks) != len(subtrees):
        raise ValueError(
            "destination_ranks ({}) must have the same length as the extracted split instructions ({})".format(len(destination_ranks),len(subtrees))
        )

    hardcoded_config = "{" + _format_initialisation_lists(
        sweep_numbers,
        parent_trees,
        subpartition_sizes,
        destination_ranks,
    ) + ", " + hardcoded_mode + "}"
    return hardcoded_config, hardcoded_mode

