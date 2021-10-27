# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Filter import Filter

from peano4.visualisation.Patch  import Patch


def absolute( values ):
  return [ abs( values[0] ) ]


class Calculator( Filter ):
  """

  Calculate some value per voxel/sample point    
      
  """
  def __init__(self, run_on_individual_pieces_of_data=False, output_cardinality=1, functor=absolute, verbose=False):
    Filter.__init__(self, run_on_individual_pieces_of_data, True, verbose)
    self._output_cardinality = output_cardinality
    self._functor            = functor
    pass
    

  def render(self, cell_data, dof, dimensions, unknowns, description, mapping):
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
    if self.verbose:
      print( "Build up auxiliary data structures over " + str(len(cell_data)) + " entries" )

    delta_for_print = 10    
    ratio_for_print = 0
    if len(cell_data)>10000:
      delta_for_print = 1

    # Run over all patches
    total_number_of_cells_per_patch = dof * dof
    if dimensions==3:
      total_number_of_cells_per_patch *= dof
      
    result_cell_data = []
    
    for p in cell_data:
      new_cell_values = []
      for k in range(0,total_number_of_cells_per_patch):
        new_data = self._functor( p.values[k*unknowns:k*unknowns+unknowns] )
        if len(new_data)!=self._output_cardinality:
          raise Exception( "returned array size has to equal output cardinality")
        new_cell_values += new_data
      new_patch = Patch(p.offset, p.size, new_cell_values, p.subdomain_number)
      result_cell_data.append( new_patch )

    return result_cell_data, dof, dimensions, self._output_cardinality, description, mapping
  
