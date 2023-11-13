from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2


class Solve(ActionSet):
  templateTVFT = """ 
  if (fineGridVertex{{SOLVER_NAME}}.getType() != vertexdata::{{SOLVER_NAME}}::Type::Coarse)
  {
    logTraceInWith1Argument("TouchVertexFirstTime", marker.toString());

    // set residual to rhs
    // we can pass the whole vector around, but intent is clearer this way
    for (int dof=0; dof<{{VERTEX_CARDINALITY}}; dof++)
      fineGridVertex{{SOLVER_NAME}}.setResidual( dof, 
                   fineGridVertex{{SOLVER_NAME}}.getRhs( dof ) );
    
    // set diag to 0
    for (int dof=0; dof<{{VERTEX_CARDINALITY}}; dof++)
      fineGridVertex{{SOLVER_NAME}}.setFieldDiag(dof, 0.0);
    
    logTraceOutWith1Argument("TouchVertexFirstTime", marker.toString());
  }
  """

  templateTVLT = """
  if (fineGridVertex{{SOLVER_NAME}}.getType() != vertexdata::{{SOLVER_NAME}}::Type::Coarse)
  {
    logTraceInWith1Argument("TouchVertexLastTime", marker.toString());

    for (int dof=0; dof<{{VERTEX_CARDINALITY}}; dof++)
    {
      assertion( repositories::{{SOLVER_NAME}}.getFieldDiag(dof) > 0 );
      double val = fineGridVertex{{SOLVER_NAME}}.getValue(dof) 
                  + repositories::{{SOLVER_INSTANCE}}::Omega 
                    * ( 1.0 / fineGridVertex{{SOLVER_NAME}}.getFieldDiag(dof) )
                    * fineGridVertex{{SOLVER_NAME}}.getResidual(dof);

      repositories::{{SOLVER_NAME}}.setValue(dof, val);
    }
    logTraceOutWith1Argument("TouchVertexLastTime", marker.toString());
  }
  """

  templateTCFT = """ """
  
  def __init__(self,
               solver):
    super( Solve, self ).__init__()
    self.d = {}
    self.d["SOLVER_INSTANCE"]    = solver.instance_name()
    self.d["SOLVER_NAME"]        = solver.typename()
    self.d["VERTEX_CARDINALITY"] = solver._unknowns_per_vertex

  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
      result = jinja2.Template(self.templateTVFT).render(**self.d)
      pass 
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = jinja2.Template(self.templateTCFT).render(**self.d)
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
#include "repositories/SolverRepository.h"
#include "peano4/utils/Loop.h"
"""
