# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet


import jinja2





class ParticleAMR(ActionSet):
  """

    AMR criterion based upon the particle density

    Most codes use this criterion in combination with a classic mesh-based criterion
    which ensures that the mesh is not coarser than a certain maximal mesh sizes.
    
    
    :: Algorithm
    
    The algorithm relies heavily on the ParticleTreeAnalysis action set which 
    annotates each cell in the tree with the sum of the vertices associated to 
    the 2^d adjacent vertices. If this sum exceeds certain thresholds and, depending
    on the setup, some cut-off radii of hosted particles are small enough, the code
    refines the tree. It is then the job of subsequent runs with 
    UpdateParticleGridAssociation to drop the particles into the newly created 
    cells.
    
    
    As we work with cell data to guide the adaptivity yet store the particles 
    within vertices, it is clear that refinement pattern tends to smear out. We
    get rather regularly refined areas around the particles. For most applications,
    that's totally fine, as it also ensures that 
    
  """
  def __init__(self,particle_set,particle_tree_analysis,min_particles_per_cell=1,max_particles_per_cell=65536):
    """

    :: Arguments
    

    
    particle_set: peano4.toolbox.particles.ParticleSet
       Instance of the particle set that is to be used as trigger. Each particle in the
       set has a cut-off radius and this one is used to guide the AMR.
     
    particle_tree_analysis: peano4.toolbox.particles.ParticleTreeAnalysis
       Tree analysis algorithm. Usually is instantiated over particle_set, but you can 
       also impose another analysis algorithm.
       
    min_particles_per_cell: Integer (>=0)
       The tree is not refined if there are less than min_particles_per_cell particles
       within the cell. By default, this parameter is set to 1 and thus the code refines
       always if there are more than two particles held in a cell. You can set the value
       to 0. In this case, we refine whenever there's a particle in a cell and its cut-off
       would allow us to refine further. Set
       it to a higher number to ensure that there's (approximately) a certain particle 
       counter per cell.
       
    max_particles_per_cell: Integer
       Forces the code to refine if there are more than max_particles_per_cell particles
       held. Has to be bigger or equal to min_particles_per_cell. 
       
       Whenever the AMR criterion refines, it can happen that particles do not 
       drop down further, as their cut-off radius is too small.
       
       You can also set this argument to zero as well. In this case, the action sets 
       refines all the way down until the mesh size just fits the cut-off radius. It 
       even refines a cell if there's only one particle in it.
       
    """
    if min_particles_per_cell<0:
      raise Exception( "invalid min number of particles pre cell (has to be positive): {}".format(min_particles_per_cell) )

    if max_particles_per_cell<min_particles_per_cell or max_particles_per_cell==0:
      raise Exception( "max number of particles ({}) has to be bigger or equal to min number ({}) and it has to be positive".format(max_particles_per_cell,min_particles_per_cell) )
  
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
  constexpr double RefinementTolerance = 0.1;
  
  bool refine  = false;
  bool erase   = false;
  if (
    not marker.willBeRefined()
    and
    fineGridCell{{CELL_DATA_NAME}}.getNumberOfParticles() > {{MAX_PARTICLES_PER_CELL}}
  ) {
    refine = true;
  }

  if (
    not refine
    and
    not marker.willBeRefined()
    and
    fineGridCell{{CELL_DATA_NAME}}.getNumberOfParticles() > {{MIN_PARTICLES_PER_CELL}}
  ) {
    double h = std::numeric_limits<double>::max();

    for (int i=0; i<TwoPowerD; i++) {
      h = std::min( h, fineGridVertices{{PARTICLE_SET}}(i).getMinCutOffRadius() );
    }

    if (h < tarch::la::min(marker.h())/3.0/(1.0+RefinementTolerance) ) {
      refine = true;
    }
  }

  if (
    // I only study the finest refine cells within the tree and
    // then I try to find those which shouldn't be refined
    marker.hasBeenRefined() and marker.willBeRefined()
    and
    not fineGridCell{{CELL_DATA_NAME}}.getParentOfRefinedCell()
    and    
    fineGridCell{{CELL_DATA_NAME}}.getNumberOfParticles() <= {{MIN_PARTICLES_PER_CELL}}
  ) {
    assertion( fineGridCell{{CELL_DATA_NAME}}.getNumberOfParticles() <= {{MIN_PARTICLES_PER_CELL}} );
    erase = true;
  }

  if (refine) {
    peano4::grid::GridControlEvent newEntry;
    newEntry.setRefinementControl( peano4::grid::GridControlEvent::RefinementControl::Refine );
    newEntry.setOffset( marker.getOffset() );
    newEntry.setWidth( marker.h() );
    newEntry.setH( marker.h()/3.0 );
    _localGridControlEvents.push_back(newEntry);
    logDebug( "touchCellLastTime(...)", "add " << newEntry.toString() );
  }
  else if (erase) {
    peano4::grid::GridControlEvent newEntry;
    newEntry.setRefinementControl( peano4::grid::GridControlEvent::RefinementControl::Erase );
    newEntry.setOffset( marker.getOffset() - marker.h()*RefinementTolerance );
    newEntry.setWidth( marker.h() * (2.0 + 2.0*RefinementTolerance) );
    newEntry.setH( marker.h() * (1.0+RefinementTolerance) );
    _localGridControlEvents.push_back(newEntry);
    logDebug( "touchCellLastTime(...)", "add " << newEntry.toString() );
  }
""")


  __Template_EndTraversal = jinja2.Template("""
  static tarch::multicore::BooleanSemaphore semaphore;
  tarch::multicore::Lock lock(semaphore);

  // insert and then merge immediately to reduce the 
  // event count
  _gridControlEvents.insert( _gridControlEvents.end(), _localGridControlEvents.begin(), _localGridControlEvents.end() );
  _gridControlEvents = peano4::grid::merge( _gridControlEvents );
""")


  def get_body_of_operation(self,operation_name):
    result = "\n"
    if operation_name==ActionSet.OPERATION_BEGIN_TRAVERSAL:
      result = """
  if (_haveHandedOutGridControlEvents) {
    _gridControlEvents.clear();
    _haveHandedOutGridControlEvents = false;
  }
"""
    if operation_name==ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
      result = self.__Template_TouchCellLastTime.render(**self.d)
    if operation_name==ActionSet.OPERATION_END_TRAVERSAL:
      result = self.__Template_EndTraversal.render(**self.d)
    return result


  def get_body_of_getGridControlEvents(self):
    return """
  _haveHandedOutGridControlEvents = true;
  return _gridControlEvents;
"""


  def get_attributes(self):
    return """
  std::vector< peano4::grid::GridControlEvent >          _localGridControlEvents;
  static std::vector< peano4::grid::GridControlEvent >   _gridControlEvents;
  static bool                                            _haveHandedOutGridControlEvents;
"""


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


  def get_static_initialisations(self,full_qualified_classname):
    return """
std::vector< peano4::grid::GridControlEvent >  """ + full_qualified_classname + """::_gridControlEvents;
bool """ + full_qualified_classname + """::_haveHandedOutGridControlEvents = false;
"""


  def get_includes(self):
    result = jinja2.Template( """
#include "tarch/multicore/Lock.h"
#include "vertexdata/{{PARTICLE_SET}}.h"
#include "globaldata/{{PARTICLE}}.h"
#include "peano4/grid/grid.h"
""" )
    return result.render(**self.d)
