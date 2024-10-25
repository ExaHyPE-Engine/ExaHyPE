from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2


class InitDofsCollocated(ActionSet):
  """!
      
  Initialise degrees of freedom 
  
  In touchVertexFirstTime, we set the type of vertex we see (either Interior, Boundary or Coarse),
  as well as sending some values to the solver implementation file to be placed into the mesh.
  
  """

  templateTouchVertexFirstTime = """
  logTraceInWith2Arguments("InitDofs::touchVertexFirstTime", marker.toString(), fineGridVertex{{SOLVER_NAME}}.toString());

  vertexdata::{{SOLVER_NAME}}::Type dofType = repositories::{{SOLVER_INSTANCE}}.getVertexDoFType(marker.x(),marker.h());
  if (marker.willBeRefined() and dofType==vertexdata::{{SOLVER_NAME}}::Type::Interior) {
    dofType = vertexdata::{{SOLVER_NAME}}::Type::Coarse;
  }
  else {
    dofType = repositories::{{SOLVER_INSTANCE}}.getVertexDoFType(marker.x(),marker.h());
  }

  switch(dofType) {
    case vertexdata::{{SOLVER_NAME}}::Type::Boundary:
      {
        if (not marker.willBeRefined()) {
          tarch::la::Vector<{{VERTEX_CARDINALITY}}, double> value;
          tarch::la::Vector<{{VERTEX_CARDINALITY}}, double> rhs;
          
          repositories::{{SOLVER_INSTANCE}}.initVertex( marker.x(), marker.h(), value, rhs );

          for (int dof=0; dof<{{VERTEX_CARDINALITY}}; dof++) {
            fineGridVertex{{SOLVER_NAME}}.setValue(dof, value(dof));
            fineGridVertex{{SOLVER_NAME}}.setRhs(  dof,   rhs(dof));
          }
        }
      }
      break;
    case vertexdata::{{SOLVER_NAME}}::Type::Interior:
      {
        if ( marker.willBeRefined() ) {
          dofType = vertexdata::{{SOLVER_NAME}}::Type::Coarse;
        }
        else {
          tarch::la::Vector<{{VERTEX_CARDINALITY}}, double> value;
          tarch::la::Vector<{{VERTEX_CARDINALITY}}, double> rhs;

          repositories::{{SOLVER_INSTANCE}}.initVertex( marker.x(), marker.h(), value, rhs );

          for (int dof=0; dof<{{VERTEX_CARDINALITY}}; dof++) {
            fineGridVertex{{SOLVER_NAME}}.setValue(dof, value(dof));
            fineGridVertex{{SOLVER_NAME}}.setRhs(  dof,   rhs(dof));
          }
        }
      }
      break;
    case vertexdata::{{SOLVER_NAME}}::Type::Coarse:
    case vertexdata::{{SOLVER_NAME}}::Type::Outside:
      break;
    case vertexdata::{{SOLVER_NAME}}::Type::Undefined:
      assertionMsg(false, "should not be returned by user" );
      break;
  }
  fineGridVertex{{SOLVER_NAME}}.setType( dofType );

  logTraceOutWith2Arguments("InitDofs::touchVertexFirstTime", marker.toString(), fineGridVertex{{SOLVER_NAME}}.toString());
   """

  templateTouchCellFirstTime = """
  logTraceInWith2Arguments("InitDofs::touchCellFirstTime", marker.toString(), fineGridCell{{SOLVER_NAME}}.toString());

  celldata::{{SOLVER_NAME}}::Type dofType = repositories::{{SOLVER_INSTANCE}}.getCellDoFType(marker.x(),marker.h());;

  switch(dofType) {
    case celldata::{{SOLVER_NAME}}::Type::Coarse:
    case celldata::{{SOLVER_NAME}}::Type::Outside:
      break;
    case celldata::{{SOLVER_NAME}}::Type::Interior:
      {
        if ( marker.willBeRefined() ) {
          dofType = celldata::{{SOLVER_NAME}}::Type::Coarse;
        }
      }
      break;
    case celldata::{{SOLVER_NAME}}::Type::Undefined:
      assertionMsg( false, "should not be called" );
      break;
  }
  fineGridCell{{SOLVER_NAME}}.setType( dofType );
  
  logTraceOutWith2Arguments("InitDofs::touchCellFirstTime", marker.toString(), fineGridCell{{SOLVER_NAME}}.toString());
   """
  
  def __init__(self,
               solver):
    super( InitDofsCollocated, self ).__init__()
    self.d = {}
    self.d["SOLVER_INSTANCE"]    = solver.instance_name()
    self.d["SOLVER_NAME"]        = solver.typename()
    self.d["VERTEX_CARDINALITY"] = solver._unknowns_per_vertex_node

  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
      result = jinja2.Template(self.templateTouchVertexFirstTime).render(**self.d)
      pass 
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
"""

