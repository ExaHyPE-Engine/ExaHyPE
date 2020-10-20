# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.visualisation.Filter import Filter


class ExtractFineGridFilter( Filter ):
  def __init__(self, exploit_idempotent=True):
    """
    
      Even though the overlap operation is idempotent - if a patch
      is not a fine grid patch within a small local dataset written
      by one tree, then it can't be part of the fine grid in the 
      overall data set - I found that it is usually faster to 
      concatenate all data and then to apply the filter.
      
    """
    Filter.__init__(self, exploit_idempotent)
    pass
  
  
  def __patches_overlap(self,a,b,dimensions):
    if dimensions==3:
      return a.offset[0] + a.size[0] >= b.offset[0] and \
             a.offset[1] + a.size[1] >= b.offset[1] and \
             a.offset[2] + a.size[2] >= b.offset[2] and \
             a.offset[0] <= b.offset[0] + b.size[0] and \
             a.offset[1] <= b.offset[1] + b.size[1] and \
             a.offset[2] <= b.offset[2] + b.size[2]
    else:
      return a.offset[0] + a.size[0] >= b.offset[0] and \
             a.offset[1] + a.size[1] >= b.offset[1] and \
             a.offset[0] <= b.offset[0] + b.size[0] and \
             a.offset[1] <= b.offset[1] + b.size[1] 
  
  
  def render(self,cell_data, dof, unknowns, dimensions):
    """
    
      Overwrite this one for the particular filter. 
      
      List operations are very epensive in Python. I therefore implement a 
      two-pass stra
      
    """
    new_num_patches = 0
    new_cell_data   = [None] * len(cell_data)
    
    print( "Build up auxiliary data structures over " + str(len(cell_data)) + " entries" )
    
    ratio_for_print = 10
    
    for i in range(0, len(cell_data) ):
      insert = True
      
      j = 0
      while j<len(cell_data) and insert:
        if j!=i:
          insert = insert and (not self.__patches_overlap(cell_data[i],cell_data[j],dimensions) or cell_data[i].size[0] < cell_data[j].size[0])
        j = j+1

      if insert:        
        new_cell_data[new_num_patches] = cell_data[i]
        new_num_patches += 1

      if i>0.01*ratio_for_print*len(cell_data):
        print( "... " + str(ratio_for_print) + "%" )
        ratio_for_print += 10

    print( "Copy results into output (commit changes)" )
    result_cell_data = [None] * new_num_patches
    for i in range(0,new_num_patches):
      result_cell_data[i] = new_cell_data[i]

    print( "extracted " + str( len(new_cell_data) ) + " from the " + str( len(cell_data) ) + " patch(es)" )

    return result_cell_data, dof, unknowns, dimensions
  
