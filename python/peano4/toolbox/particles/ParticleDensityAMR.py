# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet


import jinja2


   


class ParticleDensityAMR(ActionSet):
  """
  
    AMR criterion based upon the particle density
    
    This criterion is given a certain number of particles per cell. 
    Whenever a cell hosts more than this particular number, the 
    criterion refines the cell. The AMR criterion needs the particle
    tree analysis to work properly.
    
  """
  def __init__(self,particle_tree_analysis,max_particles_per_cell=10):
    pass
