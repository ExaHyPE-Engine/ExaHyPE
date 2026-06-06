# This file is part of the Peano load balancing toolbox. For conditions of 
# distribution and use, please see the copyright notice at www.peano-framework.org
import collections.abc


def assign_round_Robin_rank_assignment(number_of_subtrees,
                                       max_ranks,
                                       ):
    result = []
    for i in range(0,len(number_of_subtrees)):
        result.append( i%max_ranks )
    return result
    
    
def create_constructor_initialisation_string(sweep_numbers,
                                             parent_trees,
                                             subpartition_sizes,
                                             destination_ranks,
                                             ):
    """!
    
    Construct a string that can be used to feed into an instance of Hardcoded.
    
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

