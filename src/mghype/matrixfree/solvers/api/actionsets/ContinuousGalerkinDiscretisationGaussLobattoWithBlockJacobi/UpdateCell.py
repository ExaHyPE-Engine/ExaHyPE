# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2


class UpdateCell(ActionSet):
  """!

  Action set implementing the cell update of the continuous Galerkin solver
  
  """
  templateTouchCellFirstTime="""
  if ( 
    fineGridCell{{SOLVER_NAME}}.getType() != celldata::{{SOLVER_NAME}}::Type::Coarse 
//    and
    // only one condition here - ensures that we don't do this when restricting the residual
//    repositories::{{SOLVER_INSTANCE}}.updateResidual()
  ) {
    logTraceInWith4Arguments("updateResidual", 
      fineGridCell{{SOLVER_NAME}}.getSolution(),
      fineGridCell{{SOLVER_NAME}}.getRhs(),
      marker.toString(),
      repositories::{{SOLVER_INSTANCE}}.getRhsMatrix( marker.x(), marker.h() )
    );

    // Compute cell residual. Will be incomplete for boundary nodes
    tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.CellUnknowns, double > r =
      repositories::{{SOLVER_INSTANCE}}.getRhsMatrix( marker.x(), marker.h() ) * fineGridCell{{SOLVER_NAME}}.getRhs();
    r = r - repositories::{{SOLVER_INSTANCE}}.getLocalAssemblyMatrix(marker.x(), marker.h()) * fineGridCell{{SOLVER_NAME}}.getSolution();
          
    fineGridCell{{SOLVER_NAME}}.setResidual( r );
    
    tarch::la::Matrix<repositories::{{SOLVER_INSTANCE}}.InnerCellUnknowns, repositories::{{SOLVER_INSTANCE}}.InnerCellUnknowns, double>  
      innerMatrix = repositories::{{SOLVER_INSTANCE}}.NarrowToInteriorMatrix
                  * repositories::{{SOLVER_INSTANCE}}.getLocalAssemblyMatrix(marker.x(), marker.h())
                  * tarch::la::transpose(repositories::{{SOLVER_INSTANCE}}.NarrowToInteriorMatrix);

//    tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.CellUnknowns, double > du =
//      repositories::{{SOLVER_INSTANCE}}.NarrowToInteriorMatrix * r;

    
    fineGridCell{{SOLVER_NAME}}.setSolution(
      fineGridCell{{SOLVER_NAME}}.getSolution()
      + 
      repositories::{{SOLVER_INSTANCE}}.OmegaCell
        * tarch::la::transpose(repositories::{{SOLVER_INSTANCE}}.NarrowToInteriorMatrix)
        * tarch::la::invert(innerMatrix)
        * repositories::{{SOLVER_INSTANCE}}.NarrowToInteriorMatrix
        * r
    );
    

    logTraceOut("updateResidual");
  }
  """
  
  def __init__(self,
               solver):
    super( UpdateCell, self ).__init__()
    self.d = {}
    self.d["SOLVER_INSTANCE"]    = solver.instance_name()
    self.d["SOLVER_NAME"]        = solver.typename()

  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = jinja2.Template(self.templateTouchCellFirstTime).render(**self.d)
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
#include "tarch/la/LUDecomposition.h"
"""
