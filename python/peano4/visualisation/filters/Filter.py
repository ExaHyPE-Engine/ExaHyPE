# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org



class Filter(object):
  """
  
    An abstract filter class. Implement render to filter out 
    patches from the data that are not to be displayed.
    
  """
  def __init__(self, run_on_individual_pieces_of_data, run_on_concatenated_data, verbose):
    """
    
    exploit_idempotent: boolean
      Exploit the fact that the filter is idempotent, i.e. that we
      can apply it to fragments of the dataset and then to the whole
      result again.
      
    """
    self.run_on_individual_pieces_of_data = run_on_individual_pieces_of_data
    self.run_on_concatenated_data         = run_on_concatenated_data
    self.verbose                          = verbose
    pass


  def __str__(self):
    return "Filter: " + self.__class__.__name__ + ", run on individual pieces of data=" + str(self.run_on_individual_pieces_of_data) + ", run on concatenated data=" + str(self.run_on_concatenated_data) + ", verbose=" + str(self.verbose)  

  
  def render(self, cell_data, dof, dimension, unknowns, description, mapping):
    """
    
      Overwrite this one for the particular filter. 
      
      cell_data: [Patch]
      
    """
    return cell_data, dof, dimension, unknowns, description, mapping

