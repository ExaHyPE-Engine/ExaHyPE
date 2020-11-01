# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet


import jinja2


   


class ParticleAMR(ActionSet):
  """
  
    AMR criterion based upon the particle density
        
  """
  def __init__(self,particle_set,particle_tree_analysis,min_particles_per_cell=1,max_particles_per_cell=65536):
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
