# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet


import jinja2





class ParticleAMR(ActionSet):
  """

    AMR criterion based upon the particle density

    Most codes use this criterion in combination with a classic mesh-based criterion
    which ensures that the mesh is not coarser than a certain maximal mesh sizes.

  """
  def __init__(self,particle_set,particle_tree_analysis,min_particles_per_cell=1,max_particles_per_cell=65536):
    """
    
    particle_set: peano4.toolbox.particles.ParticleSet
       Instance of the particle set that is to be used as trigger. Each particle in the
       set has a cut-off radius and this one is used to guide the AMR.
     
    particle_tree_analysis: peano4.toolbox.particles.ParticleTreeAnalysis
       Tree analysis algorithm. Usually is instantiated over particle_set, but you can 
       also impose another analysis algorithm.
       
    min_particles_per_cell: int
       The tree is not refined if there are less than min_particles_per_cell particles
       within the cell. By default, this parameter is set to 1 and thus the code refines
       always if the is a particle in a cell which is way "smaller" than the cell. Set
       it to a higher number to ensure that there's (approximately) a certain particle 
       counter per cell.
       
    max_particles_per_cell: int
       Forces the code to refine even if the cut-off radius would veto a refinement. 
       In this case, the AMR criterion will refine, but the particles likely would not 
       drop down further. Has to be used with care therefore. A very high value effectively
       disables this criterion (default).
       
    """
    self.d = {
      "CELL_DATA_NAME":          particle_tree_analysis._cell_data_name,
      "MIN_PARTICLES_PER_CELL":  min_particles_per_cell,
      "MAX_PARTICLES_PER_CELL":  max_particles_per_cell,
      "PARTICLE_SET":            particle_set.name,
      "PARTICLE":                particle_set.particle_model.name,
    }
    pass


  def user_should_modify_template(self):
    return False


  __Template_TouchCellLastTime = jinja2.Template("""
  bool refine  = false;
  bool erase   = false;
  if (
    not coarseGridCell{{CELL_DATA_NAME}}.getParentOfRefinedCell()
    and
    fineGridCell{{CELL_DATA_NAME}}.getNumberOfParticles() > {{MAX_PARTICLES_PER_CELL}}
  ) {
   refine = true;
  }

  if (
    not refine
    and
    not coarseGridCell{{CELL_DATA_NAME}}.getParentOfRefinedCell()
    and
    fineGridCell{{CELL_DATA_NAME}}.getNumberOfParticles() > {{MIN_PARTICLES_PER_CELL}}
  ) {
    double h = std::numeric_limits<double>::max();

    for (int i=0; i<TwoPowerD; i++) {
      h = std::min( h, fineGridVertices{{PARTICLE_SET}}(i).getMinCutOffRadius() );
    }

    if (h < tarch::la::min(marker.h())/3.0 ) {
      refine = true;
    }
  }

  if (
    not refine
    and
    fineGridCell{{CELL_DATA_NAME}}.getParentOfRefinedCell()
    and
    fineGridCell{{CELL_DATA_NAME}}.getNumberOfParticles() <= {{MIN_PARTICLES_PER_CELL}}
  ) {
    erase = true;
  }

  if (refine) {
    peano4::grid::GridControlEvent newEntry;
    newEntry.setRefinementControl( peano4::grid::GridControlEvent::RefinementControl::Refine );
    newEntry.setOffset( marker.getOffset() );
    newEntry.setWidth( marker.h() );
    newEntry.setH( marker.h()/3.0 );
    _localGridControlEvents.push_back(newEntry);
  }
  else if (erase) {
    peano4::grid::GridControlEvent newEntry;
    newEntry.setRefinementControl( peano4::grid::GridControlEvent::RefinementControl::Erase );
    newEntry.setOffset( marker.getOffset() );
    newEntry.setWidth( marker.h() );
    newEntry.setH( marker.h() );
    _localGridControlEvents.push_back(newEntry);
  }
""")


  __Template_EndTraversal = jinja2.Template("""
  static tarch::multicore::BooleanSemaphore semaphore;
  tarch::multicore::Lock lock(semaphore);

  _gridControlEvents.insert( _gridControlEvents.end(), _localGridControlEvents.begin(), _localGridControlEvents.end() );
""")


  def get_body_of_operation(self,operation_name):
    result = "\n"
    if operation_name==ActionSet.OPERATION_BEGIN_TRAVERSAL:
      result = """
  _gridControlEvents.clear();
"""
    if operation_name==ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
      result = self.__Template_TouchCellLastTime.render(**self.d)
    if operation_name==ActionSet.OPERATION_END_TRAVERSAL:
      result = self.__Template_EndTraversal.render(**self.d)
    return result


  def get_body_of_getGridControlEvents(self):
    return "return _gridControlEvents;"


  def get_attributes(self):
    return """
  std::vector< peano4::grid::GridControlEvent >          _localGridControlEvents;
  static std::vector< peano4::grid::GridControlEvent >   _gridControlEvents;
"""

  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")

  def get_static_initialisations(self,full_qualified_classname):
    return """
std::vector< peano4::grid::GridControlEvent >  """ + full_qualified_classname + """::_gridControlEvents;
"""


  def get_includes(self):
    result = jinja2.Template( """
#include "tarch/multicore/Lock.h"
#include "vertexdata/{{PARTICLE_SET}}.h"
#include "globaldata/{{PARTICLE}}.h"
""" )
    return result.render(**self.d)
