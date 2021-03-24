# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.visualisation.Filter import Filter


class ExtractFineGridFilter( Filter ):
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
     
     I therefore introduce a Tolerance of around 10 percent.
     If in doubt, I keep stuff.
    
    """
    result = False
    
    if dimensions==3:
      Tolerance = 0.1 * max(a.size[0],b.size[0],a.size[1],b.size[1],a.size[2],b.size[2])
      result = a.offset[0] + a.size[0] > b.offset[0] + Tolerance and \
               a.offset[1] + a.size[1] > b.offset[1] + Tolerance  and \
               a.offset[2] + a.size[2] > b.offset[2] + Tolerance  and \
               a.offset[0] + Tolerance < b.offset[0] + b.size[0] and \
               a.offset[1] + Tolerance < b.offset[1] + b.size[1] and \
               a.offset[2] + Tolerance < b.offset[2] + b.size[2]
    else:
      Tolerance = 0.1 * max(a.size[0],b.size[0],a.size[1],b.size[1])
      result = a.offset[0] + a.size[0] > b.offset[0] + Tolerance and \
               a.offset[1] + a.size[1] > b.offset[1] + Tolerance and \
               a.offset[0] + Tolerance < b.offset[0] + b.size[0] and \
               a.offset[1] + Tolerance < b.offset[1] + b.size[1]

    return result
  
  
  def render(self, cell_data, dof, dimensions, unknowns, description, mapping):
    """
    
      Overwrite this one for the particular filter. 
      
      List operations are very epensive in Python. I therefore implement a 
      two-pass stra
      
    """
    def sort_key(patch):
      return patch.size[0]
  
    if self.verbose:
      print( "Sort input data to optimise algorithmic efficiency" )
    cell_data.sort(reverse=True, key=sort_key)
    
    new_num_patches = 0
    new_cell_data   = [None] * len(cell_data)
    
    if self.verbose:
      print( "Build up auxiliary data structures over " + str(len(cell_data)) + " entries" )

    delta_for_print = 10    
    ratio_for_print = 0
    if len(cell_data)>10000:
      delta_for_print = 1

    for i in range(0, len(cell_data) ):
      insert = True
      
      j = i+1
      while j<len(cell_data) and insert:
        is_not_blocked_by_element_to_the_right = not self.__patches_overlap(cell_data[i],cell_data[j],dimensions) or cell_data[i].size[0] < cell_data[j].size[0]
        #if not is_not_blocked_by_element_to_the_right:
        #  print( "patch " + str(cell_data[i]) + " does not enter result as there's also cell data " + str(cell_data[j]) )
        insert = insert and is_not_blocked_by_element_to_the_right
        j = j+1

      if insert:        
        new_cell_data[new_num_patches] = cell_data[i]
        new_num_patches += 1

      if i>0.01*ratio_for_print*len(cell_data):
        if self.verbose:
          print( "... " + str(ratio_for_print) + "%" )
        ratio_for_print += delta_for_print

    if self.verbose:
      print( "Copy results into output (commit changes)" )
    result_cell_data = [None] * new_num_patches
    for i in range(0,new_num_patches):
      result_cell_data[i] = new_cell_data[i]

    if self.verbose:
      print( "extracted " + str( new_num_patches ) + " from the " + str( len(cell_data) ) + " patch(es)" )

    return result_cell_data, dof, dimensions, unknowns, description, mapping
  
