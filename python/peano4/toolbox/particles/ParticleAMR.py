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
    self.d = {
      "CELL_DATA_NAME":          particle_tree_analysis._cell_data_name,
      "MAX_PARTICLES_PER_CELL":  max_particles_per_cell
    }
    pass


  def user_should_modify_template(self):
    return False

  
  __Template_TouchCellLastTime = jinja2.Template("""
  if (
    not coarseGridCell{{CELL_DATA_NAME}}.getParentOfRefinedCell()
    and
    fineGridCell{{CELL_DATA_NAME}}.getNumberOfParticles() > {{MAX_PARTICLES_PER_CELL}}
  ) {
    peano4::grid::GridControlEvent newEntry;
    newEntry.setRefinementControl( peano4::grid::GridControlEvent::RefinementControl::Refine );
    newEntry.setOffset( marker.getOffset() );
    newEntry.setWidth( marker.h() );
    newEntry.setH( marker.h()/3.0 );
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
    return """
#include "tarch/multicore/Lock.h"
"""