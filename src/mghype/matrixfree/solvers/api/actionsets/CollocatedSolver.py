from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2


class CollocatedSolver(ActionSet):
  templateTouchVertexFirstTime = """ 
  if (
    fineGridVertex{{SOLVER_NAME}}.getType() == vertexdata::{{SOLVER_NAME}}::Type::Boundary
    and
    repositories::{{SOLVER_INSTANCE}}.update()
  ) {
    auto value = fineGridVertex{{SOLVER_NAME}}.getValue();
    repositories::{{SOLVER_INSTANCE}}.setBoundaryConditions(
      marker.x(),
      marker.h(),
      value
    );
    fineGridVertex{{SOLVER_NAME}}.setValue(value);
  }
  else if (fineGridVertex{{SOLVER_NAME}}.getType() == vertexdata::{{SOLVER_NAME}}::Type::Interior)
  {
    logTraceInWith2Arguments("TouchVertexFirstTime", marker.toString(), fineGridVertex{{SOLVER_NAME}}.toString());

    for (int unknown=0; unknown<{{VERTEX_CARDINALITY}}; unknown++) {
      // set residual to 0
      // we can pass the whole vector around, but intent is clearer this way
      fineGridVertex{{SOLVER_NAME}}.setResidual( unknown, 0);

      // set diag to 0
      fineGridVertex{{SOLVER_NAME}}.setDiag(unknown, 0.0);
    }
    
    logTraceOutWith2Arguments("TouchVertexFirstTime", marker.toString(), fineGridVertex{{SOLVER_NAME}}.toString());
  }
  """

  templateTouchVertexLastTime = """
  if (
    fineGridVertex{{SOLVER_NAME}}.getType() == vertexdata::{{SOLVER_NAME}}::Type::Interior
    and
    repositories::{{SOLVER_INSTANCE}}.update()
  ) {
    logTraceInWith2Arguments("TouchVertexLastTime", marker.toString(), fineGridVertex{{SOLVER_NAME}}.toString());

    for (int unknown=0; unknown<{{VERTEX_CARDINALITY}}; unknown++)
    {
      logTraceInWith3Arguments("updateValue", fineGridVertex{{SOLVER_NAME}}.getValue(unknown), fineGridVertex{{SOLVER_NAME}}.getDiag(unknown), fineGridVertex{{SOLVER_NAME}}.getResidual(unknown));
      assertion( fineGridVertex{{SOLVER_NAME}}.getDiag(unknown) > 0 );
      
      double r   = fineGridVertex{{SOLVER_NAME}}.getResidual(unknown);
      double val = fineGridVertex{{SOLVER_NAME}}.getValue(unknown) 
                 + repositories::{{SOLVER_INSTANCE}}.Omega 
                 * 1.0 / fineGridVertex{{SOLVER_NAME}}.getDiag(unknown) * r;

      repositories::{{SOLVER_INSTANCE}}.updateGlobalResidual(
        tarch::la::abs(r),
        tarch::la::volume(marker.h()) * r * r,
        r * r,
        marker.h()(0)
      );

      fineGridVertex{{SOLVER_NAME}}.setValue(unknown, val);
      logTraceOutWith1Argument("updateValue", fineGridVertex{{SOLVER_NAME}}.getValue(unknown));
    }
    logTraceOutWith2Arguments("TouchVertexLastTime", marker.toString(), fineGridVertex{{SOLVER_NAME}}.toString());
  }
  """

  templateTouchCellFirstTime = """ 
  if ( 
    fineGridCell{{SOLVER_NAME}}.getType() == celldata::{{SOLVER_NAME}}::Type::Interior 
    and
    repositories::{{SOLVER_INSTANCE}}.update()
  ) {
    logTraceInWith2Arguments("Solve::touchCellFirstTime", marker.toString(), fineGridCell{{SOLVER_NAME}}.toString());
    for (int unknown=0; unknown<{{VERTEX_CARDINALITY}}; unknown++)
    {
      // Mass and assembly matrix should be block diagonal, 
      // depending on which unknown we are currently dealing with.
      // We will collect the rhs & solution values from the 
      // vertices, and apply the correct matrix as we go.
      // Define the offset:
      int startIndex = TwoPowerD * unknown;
      
      auto A       = repositories::{{SOLVER_INSTANCE}}.getLocalAssemblyMatrix(marker.x(), marker.h());
      auto M       = repositories::{{SOLVER_INSTANCE}}.getMassMatrix(marker.x(), marker.h());

      // collect all of the vertex values into one vector - apply the appropriate
      // part of the matrix as we go.
      tarch::la::Vector<TwoPowerD, double> vertexValues;
      tarch::la::Vector<TwoPowerD, double> rhsValues;
      tarch::la::Vector<TwoPowerD, double> residualValues;
      for (int i=0; i<TwoPowerD; i++){
        double solColSum = 0;
        double rhsColSum = 0;
        for (int j=0; j<TwoPowerD; j++){
          solColSum += A(startIndex+i, startIndex+j) * fineGridVertices{{SOLVER_NAME}}(j).getValue(unknown);
          rhsColSum += M(startIndex+i, startIndex+j) * fineGridVertices{{SOLVER_NAME}}(j).getRhs(unknown);
        }
      
        vertexValues(i)   = solColSum;
        rhsValues(i)      = rhsColSum;
        residualValues(i) = fineGridVertices{{SOLVER_NAME}}(i).getResidual(unknown);
      }

      // residual = rhs - Ax
      residualValues += rhsValues;
      residualValues -= vertexValues;

      for (int i=0; i<TwoPowerD; i++)
      {
        logTraceInWith1Argument("puttingValuesBack", fineGridVertices{{SOLVER_NAME}}(i).toString());

        // increment fieldDiag
        // get current val
        double diagVal = fineGridVertices{{SOLVER_NAME}}(i).getDiag(unknown);
        // increment it
        diagVal += A(i,i);
        fineGridVertices{{SOLVER_NAME}}(i).setDiag(unknown, diagVal);

        // put new residual values back
        fineGridVertices{{SOLVER_NAME}}(i).setResidual(unknown, residualValues(i));
        
        logTraceOutWith1Argument("puttingValuesBack", fineGridVertices{{SOLVER_NAME}}(i).toString());
      }

    }
    logTraceOutWith2Arguments("Solve::touchCellFirstTime", marker.toString(), fineGridCell{{SOLVER_NAME}}.toString());
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
      result = jinja2.Template(self.templateTouchVertexFirstTime).render(**self.d)
      pass 
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = jinja2.Template(self.templateTouchCellFirstTime).render(**self.d)
      pass
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_VERTEX_LAST_TIME:
      result = jinja2.Template(self.templateTouchVertexLastTime).render(**self.d)
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
