# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Filter import Filter


class ShrinkPatches( Filter ):
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
  def __init__(self, scaling, verbose=False):
    super(ShrinkPatches,self).__init__(run_on_individual_pieces_of_data=True, 
                                       run_on_concatenated_data=False, 
                                       verbose=verbose)
    self.scaling = scaling
    pass
  
  
  def render(self, cell_data, dof, dimensions, unknowns, is_data_associated_to_cell, description, mapping):
    for i in cell_data:
      if dimensions==3:
        i.offset = ( i.offset[0] + i.size[0]*(1.0-self.scaling)/2.0, 
                     i.offset[1] + i.size[1]*(1.0-self.scaling)/2.0, 
                     i.offset[2] + i.size[2]*(1.0-self.scaling)/2.0)
        i.size = ( i.size[0]*self.scaling, i.size[1]*self.scaling, i.size[2]*self.scaling )
      else:
        i.offset = ( i.offset[0] + i.size[0]*(1.0-self.scaling)/2.0, 
                     i.offset[1] + i.size[1]*(1.0-self.scaling)/2.0)
        i.size = ( i.size[0]*self.scaling, i.size[1]*self.scaling )        
        
    return cell_data, dof, dimensions, unknowns, is_data_associated_to_cell, description, mapping
  
