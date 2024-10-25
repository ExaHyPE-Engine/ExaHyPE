from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2

class Restriction(ActionSet):
  """
  This class comes last in the pecking order so that we restrict the residual
  after we've finished computing it in the solver itself...
  """
  templateTouchVertexFirstTime="""
  if (
    repositories::{{SOLVER_INSTANCE}}.restrict()
    and
    fineGridVertex{{SOLVER_NAME}}.getLevel() == repositories::{{SOLVER_INSTANCE}}.getActiveLevel() - 1
  ) {
    for (int i=0; i<{{VERTEX_CARDINALITY}}; i++) {
      // set oldU to 0
      fineGridVertex{{SOLVER_NAME}}.setOldU( i, 0.0 );
      fineGridVertex{{SOLVER_NAME}}.setU(    i, 0.0 );
      fineGridVertex{{SOLVER_NAME}}.setRhs(  i, 0.0 );   
    }
  }
  """

  templateTouchVertexLastTime="""
  // Perform some actual restriction.
  if (
    repositories::{{SOLVER_INSTANCE}}.restrict()
    and
    fineGridVertex{{SOLVER_NAME}}.getLevel() == repositories::{{SOLVER_INSTANCE}}.getActiveLevel()
  ) {
    mghype::matrixfree::solvers::cgmultigrid::restrict<{{SOLVER_NAME}}>(
      repositories::{{SOLVER_INSTANCE}}.getRestrictionMatrix(marker.x(), marker.h()),
      coarseGridVertices{{SOLVER_NAME}},
      fineGridVertex{{SOLVER_NAME}},
      marker
    );
  }

  """
  def __init__(self,
               solver,
               descend_invocation_order=0,
               parallel=False):
    super( Restriction, self ).__init__(
      descend_invocation_order,
      parallel
    )
    self.d = {}
    self.d["SOLVER_INSTANCE"]    = solver.instance_name()
    self.d["SOLVER_NAME"]        = solver.typename()
    self.d["VERTEX_CARDINALITY"] = solver._unknowns_per_vertex

  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
      result = jinja2.Template(self.templateTouchVertexFirstTime).render(**self.d)
      pass 
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_VERTEX_LAST_TIME:
      result = jinja2.Template(self.templateTouchVertexLastTime).render(**self.d)
      pass
    return result

  def user_should_modify_template(self):
    """!
    
    The action set that Peano will generate that corresponds to this class
    should not be modified by users and can safely be overwritten every time
    we run the Python toolkit.
    
    """
    return False

  def get_includes(self):
    """!
   
    We need the solver repository in this action set, as we directly access
    the solver object. We also need access to Peano's d-dimensional loops.
         
    """    
    return """
#include "repositories/SolverRepository.h"
#include "peano4/utils/Loop.h"
#include "mghype/matrixfree/solvers/CGMultigrid.h"
"""

  def get_action_set_name(self):
    """!
    
    Configure name of generated C++ action set
    
    This action set will end up in the directory observers with a name that
    reflects how the observer (initialisation) is mapped onto this action 
    set. The name pattern is ObserverName2ActionSetIdentifier where this
    routine co-determines the ActionSetIdentifier. We make is reflect the
    Python class name.
     
    """
    return __name__.replace(".py", "").replace(".", "_") + "_Restrict"

