# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2


class InitDofs(ActionSet):
  """!

  This documentation is not written yet
  
  """


  templateTouchCellFirstTime = """
  logTraceInWith2Arguments("touchCellFirstTime", marker.toString(), fineGridCell{{SOLVER_NAME}}.toString());

  celldata::{{SOLVER_NAME}}::Type dofType = repositories::{{SOLVER_INSTANCE}}.getCellType(marker.x(),marker.h());

  switch(dofType) {
    case celldata::{{SOLVER_NAME}}::Type::Outside:
      {
        if ( marker.willBeRefined() ) {
          fineGridCell{{SOLVER_NAME}}.setType( celldata::{{SOLVER_NAME}}::Type::Coarse );
        }
        else {
          fineGridCell{{SOLVER_NAME}}.setType( celldata::{{SOLVER_NAME}}::Type::Outside );
        }
      }
      break;
    case celldata::{{SOLVER_NAME}}::Type::Interior:
      {
        if ( marker.willBeRefined() ) {
          fineGridCell{{SOLVER_NAME}}.setType( celldata::{{SOLVER_NAME}}::Type::Coarse );
        }
        else {
          logTraceInWith1Argument("touchCellFirstTime::Interior", marker.toString());
          fineGridCell{{SOLVER_NAME}}.setType( celldata::{{SOLVER_NAME}}::Type::Interior );
         
          auto bottomLeftCornerOfCell = marker.x() - 0.5 * marker.h();
          int  counter = 0;

          dfor( k, repositories::{{SOLVER_INSTANCE}}.PolyDegree + 1 ) {
            auto dofPosition = bottomLeftCornerOfCell 
                             + tarch::la::multiplyComponents(marker.h(),repositories::{{SOLVER_INSTANCE}}.getIntegrationPoint(k));

            tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.UnknownsPerCellNode, double > value;
            tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.UnknownsPerCellNode, double > rhs;
            repositories::{{SOLVER_INSTANCE}}.initCellDoF(dofPosition, marker.h(), value, rhs, k);

            for (int i=0; i<repositories::{{SOLVER_INSTANCE}}.UnknownsPerCellNode; i++) {
              fineGridCell{{SOLVER_NAME}}.setSolution(counter,value(i));
              fineGridCell{{SOLVER_NAME}}.setRhs(counter,rhs(i));
              counter++;
            }
          }

          logTraceOutWith2Arguments("touchCellFirstTime::Interior", fineGridCell{{SOLVER_NAME}}.getSolution(), fineGridCell{{SOLVER_NAME}}.getRhs());
        }
      }
      break;
    case celldata::{{SOLVER_NAME}}::Type::Coarse:
    case celldata::{{SOLVER_NAME}}::Type::Undefined:
      assertionMsg(false, "should not be called");
      break;
  }
  
  logTraceOutWith2Arguments("touchCellFirstTime", marker.toString(), fineGridCell{{SOLVER_NAME}}.toString());
   """

  templatetouchFaceFirstTime = """
  logTraceInWith2Arguments("touchFaceFirstTime", marker.toString(), fineGridFace{{SOLVER_NAME}}.toString());

  facedata::{{SOLVER_NAME}}::Type dofType = repositories::{{SOLVER_INSTANCE}}.getFaceType(marker.x(),marker.h());

  switch(dofType) {
    case facedata::{{SOLVER_NAME}}::Type::Outside:
      {
        if ( marker.willBeRefined() ) // make it coarse
        {
          fineGridFace{{SOLVER_NAME}}.setType( facedata::{{SOLVER_NAME}}::Type::Coarse );
        }
        else
        {
          fineGridFace{{SOLVER_NAME}}.setType( facedata::{{SOLVER_NAME}}::Type::Outside );
        }
      } 
      break;
    case facedata::{{SOLVER_NAME}}::Type::Interior:
      {
        if ( marker.willBeRefined() ) {
          fineGridFace{{SOLVER_NAME}}.setType( facedata::{{SOLVER_NAME}}::Type::Coarse );
        }
        else {
          fineGridFace{{SOLVER_NAME}}.setType( facedata::{{SOLVER_NAME}}::Type::Interior );
        }
      } 
      break;
    case facedata::{{SOLVER_NAME}}::Type::Boundary:
      {
        if ( marker.willBeRefined() ) {
          fineGridFace{{SOLVER_NAME}}.setType( facedata::{{SOLVER_NAME}}::Type::Coarse );
        }
        else {
          fineGridFace{{SOLVER_NAME}}.setType( facedata::{{SOLVER_NAME}}::Type::Boundary );
        }
      } 
      break;
    case facedata::{{SOLVER_NAME}}::Type::Coarse:
    case facedata::{{SOLVER_NAME}}::Type::Undefined:
      assertionMsg(false, "should not be called");
      break;
  }

//  fineGridFace{{SOLVER_NAME}}.setLProjection( 0.0 );
//  fineGridFace{{SOLVER_NAME}}.setRProjection( 0.0 );
//  fineGridFace{{SOLVER_NAME}}.setSolution( 0.0 );
  
  logTraceOutWith2Arguments("touchFaceFirstTime", marker.toString(), fineGridFace{{SOLVER_NAME}}.toString());
   """
  
  def __init__(self,
               solver):
    """!
    
    Configure the initialisation action set
    
    """               
    super( InitDofs, self ).__init__()
    self.d = {}
    self.d["SOLVER_INSTANCE"]    = solver.instance_name()
    self.d["SOLVER_NAME"]        = solver.typename()
    self.d["UNKNOWNS_PER_NODE"]  = solver._unknowns_per_cell_node

  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = jinja2.Template(self.templateTouchCellFirstTime).render(**self.d)
      pass
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME:
      result = jinja2.Template(self.templatetouchFaceFirstTime).render(**self.d)
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

