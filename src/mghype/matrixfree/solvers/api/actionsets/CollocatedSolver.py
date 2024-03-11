from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2


class CollocatedSolver(ActionSet):
  templateTVFT = """ 
  if (fineGridVertex{{SOLVER_NAME}}.getType() != vertexdata::{{SOLVER_NAME}}::Type::Coarse)
  {
    logTraceInWith2Arguments("TouchVertexFirstTime", marker.toString(), fineGridVertex{{SOLVER_NAME}}.toString());

    for (int dof=0; dof<{{VERTEX_CARDINALITY}}; dof++)
    {
      // set residual to 0
      // we can pass the whole vector around, but intent is clearer this way
      fineGridVertex{{SOLVER_NAME}}.setResidual( dof, 0);

      // set diag to 0
      fineGridVertex{{SOLVER_NAME}}.setFieldDiag(dof, 0.0);
    }
    
    logTraceOutWith2Arguments("TouchVertexFirstTime", marker.toString(), fineGridVertex{{SOLVER_NAME}}.toString());
  }
  """

  templateTVLT = """
  if (fineGridVertex{{SOLVER_NAME}}.getType() != vertexdata::{{SOLVER_NAME}}::Type::Coarse)
  {
    logTraceInWith2Arguments("TouchVertexLastTime", marker.toString(), fineGridVertex{{SOLVER_NAME}}.toString());

    for (int dof=0; dof<{{VERTEX_CARDINALITY}}; dof++)
    {
      logTraceInWith3Arguments("updateValue", fineGridVertex{{SOLVER_NAME}}.getValue(dof), fineGridVertex{{SOLVER_NAME}}.getFieldDiag(dof), fineGridVertex{{SOLVER_NAME}}.getResidual(dof));
      assertion( fineGridVertex{{SOLVER_NAME}}.getFieldDiag(dof) > 0 );
      
      double r   = tarch::la::abs(fineGridVertex{{SOLVER_NAME}}.getResidual(dof));
      double val = fineGridVertex{{SOLVER_NAME}}.getValue(dof) 
                  + repositories::{{SOLVER_INSTANCE}}.Omega 
                    * ( 1.0 / fineGridVertex{{SOLVER_NAME}}.getFieldDiag(dof) )
                    * r;

      repositories::{{SOLVER_INSTANCE}}.updateGlobalResidual(
        tarch::la::abs(r),
        tarch::la::volume(marker.h()) * r * r,
        r * r,
        marker.h()(0)
      );

      fineGridVertex{{SOLVER_NAME}}.setValue(dof, val);
      logTraceOutWith1Argument("updateValue", fineGridVertex{{SOLVER_NAME}}.getValue(dof));
    }
    logTraceOutWith2Arguments("TouchVertexLastTime", marker.toString(), fineGridVertex{{SOLVER_NAME}}.toString());
  }
  """

  templateTCFT = """ 
  if ( fineGridCell{{SOLVER_NAME}}.getType() == celldata::{{SOLVER_NAME}}::Type::Interior )
  {
    logTraceInWith2Arguments("Solve::TCFT", marker.toString(), fineGridCell{{SOLVER_NAME}}.toString());
    for (int dof=0; dof<{{VERTEX_CARDINALITY}}; dof++)
    {
      // collect all of the vertex values into one vector
      tarch::la::Vector<TwoPowerD, double> vertexValues;
      tarch::la::Vector<TwoPowerD, double> rhsValues;
      tarch::la::Vector<TwoPowerD, double> residualValues;
      for (int i=0; i<TwoPowerD; i++)
      {
        vertexValues(i)   = fineGridVertices{{SOLVER_NAME}}(i).getValue(dof);
        rhsValues(i)      = fineGridVertices{{SOLVER_NAME}}(i).getRhs(dof);
        residualValues(i) = fineGridVertices{{SOLVER_NAME}}(i).getResidual(dof);
      }

      // apply local assembly matrix
      auto A       = repositories::{{SOLVER_INSTANCE}}.getLocalAssemblyMatrix(marker.x(), marker.h());
      vertexValues = A * vertexValues;

      // apply mass matrix
      // note - these should be constant throughout. no need to recalculate.
      auto M    = repositories::{{SOLVER_INSTANCE}}.getMassMatrix(marker.x(), marker.h());
      rhsValues = M * rhsValues;

      // residual = rhs - Ax
      residualValues += rhsValues;
      residualValues -= vertexValues;

      for (int i=0; i<TwoPowerD; i++)
      {
        logTraceInWith1Argument("puttingValuesBack", fineGridVertices{{SOLVER_NAME}}(i).toString());

        // increment fieldDiag
        // get current val
        double diagVal = fineGridVertices{{SOLVER_NAME}}(i).getFieldDiag(dof);
        // increment it
        diagVal += A(i,i);
        fineGridVertices{{SOLVER_NAME}}(i).setFieldDiag(dof, diagVal);

        // put new residual values back
        fineGridVertices{{SOLVER_NAME}}(i).setResidual(dof, residualValues(i));
        
        logTraceOutWith1Argument("puttingValuesBack", fineGridVertices{{SOLVER_NAME}}(i).toString());
      }

    }
    logTraceOutWith2Arguments("Solve::TCFT", marker.toString(), fineGridCell{{SOLVER_NAME}}.toString());
  }
  """
  
  def __init__(self,
               solver):
    super( CollocatedSolver, self ).__init__()
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
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_VERTEX_LAST_TIME:
      result = jinja2.Template(self.templateTVLT).render(**self.d)
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
