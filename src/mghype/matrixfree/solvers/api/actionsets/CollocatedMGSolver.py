from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2

"""!
Let's have two separate classes for solving and for restriction

We could use the other class completely and make a modification
to the update() function in the abstract solver, but we need to
pass in the level argument!
"""

class CollocatedMGSolver(ActionSet):
  templateTouchVertexFirstTime = """ 
  if (
    fineGridVertex{{SOLVER_NAME}}.getType() == vertexdata::{{SOLVER_NAME}}::Type::Boundary
    and
    repositories::{{SOLVER_INSTANCE}}.update( fineGridVertex{{SOLVER_NAME}}.getLevel() )
  ) {
    logTraceInWith1Argument("TouchVertexFirstTimeBoundary", fineGridVertex{{SOLVER_NAME}}.toString());
    auto value = fineGridVertex{{SOLVER_NAME}}.getU();
    repositories::{{SOLVER_INSTANCE}}.setBoundaryConditions(
      marker.x(),
      marker.h(),
      value
    );
    fineGridVertex{{SOLVER_NAME}}.setU(value);
    logTraceOutWith1Argument("TouchVertexFirstTimeBoundary", fineGridVertex{{SOLVER_NAME}}.toString());
  }
  else if (
    fineGridVertex{{SOLVER_NAME}}.getType() == vertexdata::{{SOLVER_NAME}}::Type::Interior
    and
    repositories::{{SOLVER_INSTANCE}}.update( fineGridVertex{{SOLVER_NAME}}.getLevel() )
  )
  {
    logTraceInWith3Arguments("TouchVertexFirstTimeInterior", fineGridVertex{{SOLVER_NAME}}.toString(), marker.x(), marker.h());

    for (int unknown=0; unknown<{{VERTEX_CARDINALITY}}; unknown++) {
      // set residual to 0
      // we can pass the whole vector around, but intent is clearer this way
      fineGridVertex{{SOLVER_NAME}}.setResidual( unknown, 0);

      // set diag to 0
      fineGridVertex{{SOLVER_NAME}}.setDiag(unknown, 0.0);
    }
    
    logTraceOutWith2Arguments("TouchVertexFirstTimeInterior", marker.toString(), fineGridVertex{{SOLVER_NAME}}.toString());
  }
  """

  templateTouchVertexLastTime = """
  if (
    fineGridVertex{{SOLVER_NAME}}.getType() == vertexdata::{{SOLVER_NAME}}::Type::Interior
    and
    repositories::{{SOLVER_INSTANCE}}.update( fineGridVertex{{SOLVER_NAME}}.getLevel() )
  ) {
    logTraceInWith3Arguments("TouchVertexLastTime", fineGridVertex{{SOLVER_NAME}}.toString(), marker.x(), marker.h());

    for (int unknown=0; unknown<{{VERTEX_CARDINALITY}}; unknown++)
    {
      logTraceInWith3Arguments("updateValue", fineGridVertex{{SOLVER_NAME}}.getU(unknown), fineGridVertex{{SOLVER_NAME}}.getDiag(unknown), fineGridVertex{{SOLVER_NAME}}.getResidual(unknown));
      assertion( fineGridVertex{{SOLVER_NAME}}.getDiag(unknown) > 0 );
      
      double r   = fineGridVertex{{SOLVER_NAME}}.getResidual(unknown);
      double val = fineGridVertex{{SOLVER_NAME}}.getU(unknown) 
                 + repositories::{{SOLVER_INSTANCE}}.Omega 
                 * 1.0 / fineGridVertex{{SOLVER_NAME}}.getDiag(unknown) * r;

      // only care about solution on finest level...
      if (
        fineGridVertex{{SOLVER_NAME}}.getLevel() == repositories::{{SOLVER_INSTANCE}}.getFinestLevel()
      )
        repositories::{{SOLVER_INSTANCE}}.updateGlobalResidual(
          tarch::la::abs(r),
          tarch::la::volume(marker.h()) * r * r,
          r * r,
          marker.h()(0)
        );

      fineGridVertex{{SOLVER_NAME}}.setU(unknown, val);
      logTraceOutWith1Argument("updateValue", fineGridVertex{{SOLVER_NAME}}.getU(unknown));
    }
    logTraceOutWith1Argument("TouchVertexLastTime", fineGridVertex{{SOLVER_NAME}}.toString());
  }
  """

  templateTouchCellFirstTime = """ 
  if ( 
    fineGridCell{{SOLVER_NAME}}.getType() == celldata::{{SOLVER_NAME}}::Type::Interior 
    and
    repositories::{{SOLVER_INSTANCE}}.update( fineGridCell{{SOLVER_NAME}}.getLevel() )
  ) {
    logTraceInWith3Arguments("Solve::touchCellFirstTime", fineGridCell{{SOLVER_NAME}}.toString(), marker.x(), marker.h());
    for (int unknown=0; unknown<{{VERTEX_CARDINALITY}}; unknown++)
    {
      // Mass and assembly matrix should be block diagonal, 
      // depending on which unknown we are currently dealing with.
      // We will collect the rhs & solution values from the 
      // vertices, and apply the correct matrix as we go.
      // Define the offset:
      int startIndex = TwoPowerD * unknown;
      
      auto A       = repositories::{{SOLVER_INSTANCE}}.getLocalAssemblyMatrix(marker.x(), marker.h());
      auto M       = repositories::{{SOLVER_INSTANCE}}.getMassMatrix(marker.x(), marker.h(), fineGridCell{{SOLVER_NAME}}.getLevel());

      // collect all of the vertex values into one vector - apply the appropriate
      // part of the matrix as we go.
      tarch::la::Vector<TwoPowerD, double> vertexValues;
      tarch::la::Vector<TwoPowerD, double> rhsValues;
      tarch::la::Vector<TwoPowerD, double> residualValues;
      for (int i=0; i<TwoPowerD; i++){
        double solColSum = 0;
        double rhsColSum = 0;
        for (int j=0; j<TwoPowerD; j++){
          solColSum += A(startIndex+i, startIndex+j) * fineGridVertices{{SOLVER_NAME}}(j).getU(unknown);
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
    logTraceOutWith1Argument("Solve::touchCellFirstTime", fineGridCell{{SOLVER_NAME}}.toString());
  }
  """
  def __init__(self,
               solver,
               descend_invocation_order=0,
               parallel=False):
    super( CollocatedMGSolver, self ).__init__(
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
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = jinja2.Template(self.templateTouchCellFirstTime).render(**self.d)
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
    return __name__.replace(".py", "").replace(".", "_") + "_Solve"


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

class Prolongation(ActionSet):
  """!
  This also should be called before the solver stuff
  """
  templateTouchVertexFirstTime="""
  if (
    repositories::{{SOLVER_INSTANCE}}.prolongate()
    and
    fineGridVertex{{SOLVER_NAME}}.getLevel() == repositories::{{SOLVER_INSTANCE}}.getActiveLevel() - 1
  ) {
    logTraceInWith1Argument("prolongate::setDelta", fineGridVertex{{SOLVER_NAME}}.toString());
    for (int i=0; i<{{VERTEX_CARDINALITY}}; i++) 
      fineGridVertex{{SOLVER_NAME}}.setDelta(
        i,
        fineGridVertex{{SOLVER_NAME}}.getU(i) - fineGridVertex{{SOLVER_NAME}}.getOldU(i)
      );
    logTraceOutWith1Argument("prolongate::setDelta", fineGridVertex{{SOLVER_NAME}}.toString());
  }

  else if (
    repositories::{{SOLVER_INSTANCE}}.prolongate()
    and
    fineGridVertex{{SOLVER_NAME}}.getLevel() == repositories::{{SOLVER_INSTANCE}}.getActiveLevel()
  ) {
    // do some real prolongation...
    mghype::matrixfree::solvers::cgmultigrid::prolongate<{{SOLVER_NAME}}>(
      repositories::{{SOLVER_INSTANCE}}.getProlongationMatrix(marker.x(), marker.h()),
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
    super( Prolongation, self ).__init__(
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
    # if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_VERTEX_LAST_TIME:
    #   result = jinja2.Template(self.templateTouchVertexLastTime).render(**self.d)
    #   pass
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
    return __name__.replace(".py", "").replace(".", "_") + "_Prolongation"



