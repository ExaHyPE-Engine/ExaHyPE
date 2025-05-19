from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2

class UpdateSolution(ActionSet):
  """!
  This action set goes first, so that if we've done at least one sweep, we can update the
  solution based on the residual and diag.

  We wait until the next sweep so that vertices that are shared across multiple spacetrees
  have a chance to agree on a value for residual and diag, since not all contributions
  will be registered.
  """
  
  templateTouchVertexFirstTime="""
  if (
    fineGridVertex{{SOLVER_NAME}}.getType() == vertexdata::{{SOLVER_NAME}}::Type::Interior
    and
    repositories::{{SOLVER_INSTANCE}}.updateSolution()
  ) {
    for (int unknown=0; unknown<{{VERTEX_CARDINALITY}}; unknown++)
    {
      assertion( fineGridVertex{{SOLVER_NAME}}.getDiag(unknown) > 0 );
      
      double r   = fineGridVertex{{SOLVER_NAME}}.getResidual(unknown);
      double du  = repositories::{{SOLVER_INSTANCE}}.Omega 
                 * 1.0 / fineGridVertex{{SOLVER_NAME}}.getDiag(unknown) * r;

      repositories::{{SOLVER_INSTANCE}}.updateGlobalResidual(
        r, marker.h()
      );
      repositories::{{SOLVER_INSTANCE}}.updateGlobalSolutionUpdates(
        du, marker.h()(0)
      );

      fineGridVertex{{SOLVER_NAME}}.setU(unknown, fineGridVertex{{SOLVER_NAME}}.getU(unknown) + du );
    }
  }

  if ( fineGridVertex{{SOLVER_NAME}}.getType() == vertexdata::{{SOLVER_NAME}}::Type::Boundary ) {
    auto value = 0.0;
    for (int unknown=0; unknown<{{VERTEX_CARDINALITY}}; unknown++) {
      fineGridVertex{{SOLVER_NAME}}.setU(unknown, value);
    }
  }
  """

  def __init__(self,
               solver,
               descend_invocation_order=0,
               parallel=False
               ):
    super( UpdateSolution, self ).__init__(
      descend_invocation_order,
      parallel
    )
    self.d = {}
    self.d["SOLVER_INSTANCE"]    = solver.instance_name()
    self.d["SOLVER_NAME"]        = solver.typename()
    self.d["VERTEX_CARDINALITY"] = solver._unknowns_per_vertex_node

  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
      result = jinja2.Template(self.templateTouchVertexFirstTime).render(**self.d)
      pass 
    return result

  def get_action_set_name(self):
    """!
    
    Configure name of generated C++ action set
    
    This action set will end up in the directory observers with a name that
    reflects how the observer (initialisation) is mapped onto this action 
    set. The name pattern is ObserverName2ActionSetIdentifier where this
    routine co-determines the ActionSetIdentifier. We make is reflect the
    Python class name.
     
    """
    return __name__.replace(".py", "").replace(".", "_")
    
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
#include "../repositories/SolverRepository.h"
#include "peano4/utils/Loop.h"
"""
