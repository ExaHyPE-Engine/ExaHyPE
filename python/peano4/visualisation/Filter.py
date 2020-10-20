# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org



class Filter(object):
  """
  
    An abstract filter class. Implement render to filter out 
    patches from the data that are not to be displayed.
    
    Filters have to be idempotent. Otherwise, all the internal
    optimisations do not work.
    
  """
  def __init__(self, exploit_idempotent):
    """
    
    exploit_idempotent: boolean
      Exploit the fact that the filter is idempotent, i.e. that we
      can apply it to fragments of the dataset and then to the whole
      result again.
      
    """
    self.exploit_idempotent = exploit_idempotent
    pass
  
  
  def render(self, cell_data, dof, unknowns, dimensions):
    """
    
      Overwrite this one for the particular filter. 
      
    """
    return cell_data, dof, unknowns, dimensions

