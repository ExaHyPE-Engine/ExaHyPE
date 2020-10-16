# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org



class Filter(object):
  """
  
    An abstract filter class. Implement render to filter out 
    patches from the data that are not to be displayed.
    
  """
  def __init__(self,d):
    self._d = d
    pass
  
  
  def render(self,cell_data, num_patches, dof, unknowns):
    """
    
      Overwrite this one for the particular filter. 
      
    """
    return cell_data, num_patches, dof, unknowns

