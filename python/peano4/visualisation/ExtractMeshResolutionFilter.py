# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.visualisation.Filter import Filter


class ExtractMeshResolutionFilter( Filter ):
  def __init__(self, min_h, max_h, run_on_individual_pieces_of_data=True):
    Filter.__init__(self, run_on_individual_pieces_of_data, not run_on_individual_pieces_of_data)
    self.min_h = min_h
    self.max_h = max_h
    if min_h > max_h:
      print( "Error: extract mesh resolution filter uses smaller max_h (" + str(max_h) + ") than min_h (" + str(min_h) + "). Permuted entries, but this should be fixed")
      self.min_h = max_h
      self.max_h = min_h
    pass
    
  def render(self,cell_data, dof, unknowns, dimensions):
    """
    
      Overwrite this one for the particular filter. 
      
    """
    def sort_key(patch):
      return patch.size[0]
  
    print( "Sort input data to optimise algorithmic efficiency" )
    cell_data.sort(key=sort_key)
    
    new_cell_data   = []

    ratio_for_print = 10

    i = 0;    
    while i<len(cell_data) and cell_data[i].size[0] < self.max_h:
      if cell_data[i].size[0] < self.max_h and cell_data[i].size[0] >= self.min_h:
        new_cell_data.append(cell_data[i])
      i += 1
      
      if i>0.01*ratio_for_print*len(cell_data):
        print( "... " + str(ratio_for_print) + "%" )
        ratio_for_print += 10

    print( "extracted " + str( len(new_cell_data) ) + " from the " + str( len(cell_data) ) + " patch(es)" )

    return new_cell_data, dof, unknowns, dimensions
  
