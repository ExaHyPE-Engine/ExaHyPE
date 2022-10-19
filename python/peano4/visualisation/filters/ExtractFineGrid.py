# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Filter import Filter


class ExtractFineGrid( Filter ):
  """
    
    Extract the fine grid information
      
    This is a very expensive filter if it is applied on large patch 
    sets, as it has to compare each patch with each other patch.
    If you want to use it in an economically sensible way, I 
    recommend that you first apply other, cheapter filters to bring
    the patch count down.
   
    Even though the overlap operation is idempotent - if a patch
    is not a fine grid patch within a small local dataset written
    by one tree, then it can't be part of the fine grid in the 
    overall data set - I found that it is usually faster to 
    concatenate all data and then to apply the filter.
      
  """
  def __init__(self, run_on_individual_pieces_of_data=False, verbose=False):
    Filter.__init__(self, run_on_individual_pieces_of_data, True, verbose)
    pass
  
  
  def __patches_overlap(self,a,b,dimensions):
    """
    
     When we extract the fine grid patches, we rely on a 
     patch overlap. With floating point precision which is 
     often smaller then IEEE single (for a lot of vis tools
     or formats, respectively), I try to be rather careful
     with throwing away patches - otherwise, you quickly end
     up with vis where small pieces are missing. 
     
     I therefore introduce an expansion factor. I throw away
     patches if they overlap. So to be more pessimistic, the 
     expansion has to be set negative. 
     
     a: peano4.visualisation.Patch
     
     b: peano4.visualisation.Patch
    
    """
    result = False
    
    if dimensions==3:
      Expansion = -0.1 * min(a.size[0],b.size[0],a.size[1],b.size[1],a.size[2],b.size[2])
      result = a.offset[0] + a.size[0] + Expansion > b.offset[0] and \
               a.offset[1] + a.size[1] + Expansion > b.offset[1] and \
               a.offset[2] + a.size[2] + Expansion > b.offset[2] and \
               a.offset[0] < b.offset[0] + b.size[0] + Expansion and \
               a.offset[1] < b.offset[1] + b.size[1] + Expansion and \
               a.offset[2] < b.offset[2] + b.size[2] + Expansion
    else:
      Expansion = -0.1 * min(a.size[0],b.size[0],a.size[1],b.size[1])
      result = a.offset[0] + a.size[0] + Expansion > b.offset[0] and \
               a.offset[1] + a.size[1] + Expansion > b.offset[1] and \
               a.offset[0] < b.offset[0] + b.size[0] + Expansion and \
               a.offset[1] < b.offset[1] + b.size[1] + Expansion

    return result
  
  
  def render(self, cell_data, dof, dimensions, unknowns, is_data_associated_to_cell, description, mapping):
    """
    
      Overwrite this one for the particular filter. 
 
      We create an empty result array. Then we run over the input sequence
      of cell_data. Per element, we check if a cell_data to the right within
      this input sequence overlaps. 
      
      List operations are very epensive in Python. I therefore implement a 
      two-pass strategy, where I first sort the patches according to their
      size. Actually, I sort them according to their inverted size, i.e. 
      big entries come first.
      
    """
    def sort_key(patch):
      return patch.size[0]
  
    if self.verbose:
      print( "Sort input data to optimise algorithmic efficiency. Small patches come first" )
    cell_data.sort(reverse=False, key=sort_key)
    
    new_cell_data   = [None] * len(cell_data)
    
    if self.verbose:
      print( "Build up auxiliary data structures over " + str(len(cell_data)) + " entries" )

    new_cell_data[0] = cell_data[0]
    current_index_in_cell_data     = 1
    current_index_in_new_cell_data = 0
    
    # I memorise the min h and add a shortcut for the finest cell. This turned out
    # to be absolutely essential for almost all data
    min_size = cell_data[current_index_in_cell_data].size[0]
    
    ratio_of_last_progress_print = 10

    while current_index_in_cell_data<len(cell_data):
      assert cell_data[current_index_in_cell_data-1].size[0] <= cell_data[current_index_in_cell_data].size[0]

      if cell_data[current_index_in_cell_data].size[0]==min_size:
        current_index_in_new_cell_data += 1
        new_cell_data[current_index_in_new_cell_data] = cell_data[current_index_in_cell_data]
        current_index_in_cell_data += 1
      else:
        overlaps = False
        check_index = 0
        while not overlaps and check_index < current_index_in_new_cell_data:
          overlaps = overlaps or self.__patches_overlap(new_cell_data[check_index],cell_data[current_index_in_cell_data],dimensions)
          check_index += 1
        if overlaps:
          current_index_in_cell_data    += 1
        else:
          current_index_in_new_cell_data += 1
          new_cell_data[current_index_in_new_cell_data] = cell_data[current_index_in_cell_data]
          current_index_in_cell_data += 1

      if ( current_index_in_cell_data>ratio_of_last_progress_print/100*len(cell_data) ):
        print( "... {}%".format(ratio_of_last_progress_print) )
        ratio_of_last_progress_print += 10

    current_index_in_new_cell_data += 1
    if self.verbose:
      print( "extracted " + str( current_index_in_new_cell_data ) + " from the " + str( len(cell_data) ) + " patch(es)" )
    new_cell_data = new_cell_data[0:current_index_in_new_cell_data]

    return new_cell_data, dof, dimensions, unknowns, is_data_associated_to_cell, description, mapping
  
