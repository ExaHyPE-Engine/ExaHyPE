# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from Filter import Filter


class ExtractFineGridFilter( Filter ):
  def __init__(self,dimensions):
    Filter.__init__(self,dimensions)
    pass
  
  
  def __patches_overlap(self,a,b):
    if self._dimensions==3:
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
  
  
  def render(self,cell_data, num_patches, dof, unknowns):
    """
    
      Overwrite this one for the particular filter. 
      
    """
    new_num_patches = 0
    new_cell_data   = []
    
    for i in range(0,num_patches):
      overlaps_with_patch_in_new_cell_data = False
      for j in new_cell_data:
        overlaps_with_patch_in_new_cell_data = overlaps_with_patch_in_new_cell_data or self.__patches_overlap(cell_data[i],j)
        
      if not overlaps_with_patch_in_new_cell_data:
        new_entry = cell_data[i]
        for j in range(i+1,num_patches):
          if self.__patches_overlap(cell_data[j],new_entry) and cell_data[j].size[0] < new_entry.size[0]:
            new_entry = cell_data[j]
        new_num_patches += 1
        new_cell_data.append(new_entry)

    print( "extracted " + str(new_num_patches) + " from the " + str(num_patches) + " patch(es)" )

    return new_cell_data, new_num_patches, dof, unknowns
  
