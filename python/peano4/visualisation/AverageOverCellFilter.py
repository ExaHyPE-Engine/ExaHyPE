# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.visualisation.Filter import Filter
from peano4.visualisation.Patch  import Patch


class AverageOverCellFilter( Filter ):
  """
    
    Run over all cells. Each cell holds a patch with dof x dof (2d) or 
    dof x dof x dof (3d) cells. Each cell hosts unknown unknowns. This
    filter reduces each patch to a 1x1 or 1x1x1 outcome, i.e. a point 
    cloud.
          
  """
  def __init__(self, verbose=False):
    Filter.__init__(self, True, False, verbose)
    pass

 
  def render(self, cell_data, dof, dimension, unknowns, description, mapping):
    """
    
      Overwrite this one for the particular filter. 
      
    """
    result_cell_data = []

    delta_for_print = 10    
    ratio_for_print = 0
    if len(cell_data)>10000:
      delta_for_print = 1

    for patch in cell_data:
      average_values = [0] * unknowns

      entries_per_cell = dof**dimension

      for i in range(0,entries_per_cell):
        for j in range(0,unknowns):
          average_values[j] = average_values[j] + patch.values[i*unknowns+j]
      for j in range(0,unknowns):
        average_values[j] = average_values[j]/unknowns

      new_patch = Patch(patch.offset, patch.size, average_values, patch.subdomain_number)
      result_cell_data.append( new_patch )

    if self.verbose and len(result_cell_data)==0:
      print( "snapshot has been empty")
    elif self.verbose:
      print( "replaced " + str(len(result_cell_data)) + " patches with one voxel with average values")

    mapping = []
    if dimension==2:
      mapping = [0.0, 0.0,
                 1.0, 0.0,
                 0.0, 1.0,
                 1.0, 1.0]
    else:
      mapping = [0.0, 0.0, 0.0,
                 1.0, 0.0, 0.0,
                 0.0, 1.0, 0.0,
                 1.0, 1.0, 0.0,
                 0.0, 0.0, 1.0,
                 1.0, 0.0, 1.0,
                 0.0, 1.0, 1.0,
                 1.0, 1.0, 1.0
                ]
    return result_cell_data, 1, dimension, unknowns, description, mapping

