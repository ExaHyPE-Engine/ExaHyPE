from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2

class UpdateSolution(ActionSet):
  """!
  We use this action set to apply the residual and solution updates to the vertices

  Logically, this should be the final step in a sweep whilst we are solving, but due
  to the fact that we need to perform synchronisation between vertices on parallel
  boundaries before continuing to update, restrict or prolongate, we move the
  residual/solution updates into its own action set. We push this action set into
  the following sweep, since vertex synchronisation happens after the traversal has
  begun, but before any action sets are invoked.


  The idea is then to perform the following:

  During Solve stage:
    - updateSolution (but not on the first stage)
    - ResetAndUpdateResidual
    - New sweep

  To facilitate this, we add in a new bool function to the abstract solver class (and
  rename the existing one). Previously we would update if we are on the active level
  and in any of the solver states. We should now change this.

  Solving gets its own sweep, and we lump in Restriction and Prolongation together,
  which should be able to run in parallel.

  We can then have two bool functions - updateResidual() and updateSolution().

  updateResidual() should return true if we are on the active level and in the
  smoothing state either PreSmoothing or PostSmoothing.

  updateSolution() should return true if:
    - We are on the active level, the smoothing state is either PreSmoothing or PostSmoothing
      and the relevant counter is at least 1 (prevents division by 0 error with diag == 0)
    - We are in phase Restriction, and we are on the active level. Note that moving to the
      restriction phase does not(!) move the activeLevel.  We then finish updating
      the solution to be restricted during touchVertexFirstTime. Since we are setting quantities 
      on the level above (ie coarser), we then do restriction during touchVertexLastTime
    - We are in the phase prolongation, and we are one above the active level. Prolongation
      grabs data from the level above during touchVertexFirstTime.
    
  Since we need to synchronise the solution/rhs data after restriction and prolongation, we
  separate these out into separate sweeps.
  """

  templateTouchVertexFirstTime="""
  if (
    fineGridVertex{{SOLVER_NAME}}.getType() == vertexdata::{{SOLVER_NAME}}::Type::Interior
    and
    repositories::{{SOLVER_INSTANCE}}.updateSolution( fineGridVertex{{SOLVER_NAME}}.getLevel() )
  ) {
    logTraceInWith3Arguments("TouchVertexLastTime", fineGridVertex{{SOLVER_NAME}}.toString(), marker.x(), marker.h());

    for (int unknown=0; unknown<{{VERTEX_CARDINALITY}}; unknown++)
    {
      logTraceInWith3Arguments("updateValue", fineGridVertex{{SOLVER_NAME}}.getU(unknown), fineGridVertex{{SOLVER_NAME}}.getDiag(unknown), fineGridVertex{{SOLVER_NAME}}.getResidual(unknown));
      assertion( fineGridVertex{{SOLVER_NAME}}.getDiag(unknown) > 0 );
      
      double r   = fineGridVertex{{SOLVER_NAME}}.getResidual(unknown);
      double du  = repositories::{{SOLVER_INSTANCE}}.Omega 
                 * 1.0 / fineGridVertex{{SOLVER_NAME}}.getDiag(unknown) * r;

      
      fineGridVertex{{SOLVER_NAME}}.setU(unknown, fineGridVertex{{SOLVER_NAME}}.getU(unknown) + du);
      logTraceOutWith1Argument("updateValue", fineGridVertex{{SOLVER_NAME}}.getU(unknown));
    }
    logTraceOutWith1Argument("TouchVertexLastTime", fineGridVertex{{SOLVER_NAME}}.toString());
  }
  // only care about solution on finest level...
  if (
    fineGridVertex{{SOLVER_NAME}}.getType() == vertexdata::{{SOLVER_NAME}}::Type::Interior
    and
    repositories::{{SOLVER_INSTANCE}}.reportSolutionUpdates( fineGridVertex{{SOLVER_NAME}}.getLevel() )
  ) {
    for (int unknown=0; unknown<{{VERTEX_CARDINALITY}}; unknown++) {
      double r   = fineGridVertex{{SOLVER_NAME}}.getResidual(unknown);
      double du  = repositories::{{SOLVER_INSTANCE}}.Omega 
                 * 1.0 / fineGridVertex{{SOLVER_NAME}}.getDiag(unknown) * r;

      repositories::{{SOLVER_INSTANCE}}.updateGlobalResidual(r, marker.h());
      repositories::{{SOLVER_INSTANCE}}.updateGlobalSolutionUpdates(du, marker.h()(0));
    }
  }
  """

  def __init__(self,
               solver,
               descend_invocation_order=0,
               parallel=False):
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
    return __name__.replace(".py", "").replace(".", "_")
