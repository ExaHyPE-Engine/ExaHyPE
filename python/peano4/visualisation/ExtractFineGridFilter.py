# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.visualisation.Filter import Filter


class ExtractFineGridFilter( Filter ):
  def __init__(self):
    Filter.__init__(self)
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
    #new_num_patches = 0
    #new_cell_data   = [None] * num_patches
    new_cell_data = []
    
    for i in range(0, len(cell_data) ):
      insert = True
      
      j = 0
      while j<len(cell_data) and insert:
        if j!=i:
          insert = insert and (not self.__patches_overlap(cell_data[i],cell_data[j],dimensions) or cell_data[i].size[0] < cell_data[j].size[0])
        j = j+1

      if insert:        
        #new_cell_data[new_num_patches] = cell_data[i]
        #new_num_patches += 1
        new_cell_data.append( cell_data[i] )

    print( "extracted " + str( len(new_cell_data) ) + " from the " + str( len(cell_data) ) + " patch(es)" )

    return new_cell_data, dof, unknowns, dimensions
  
