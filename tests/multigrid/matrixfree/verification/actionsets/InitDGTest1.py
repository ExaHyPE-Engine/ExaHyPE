from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2

class InitDofsDGTest1(ActionSet):
  """!
  Redoing this class for discontinuous galerkin solver.
  
  This time, we store values only in the cells and just 
  determine the type of vertex we see.
  """
  templateTouchVertexFirstTime = """

  //logTraceInWith3Arguments("InitDofsDG::touchVertexFirstTime", marker.toString(), isOnBoundary(marker.x()), fineGridVertex{{SOLVER_NAME}}.toString());

  vertexdata::{{SOLVER_NAME}}::Type dofType = repositories::{{SOLVER_INSTANCE}}.getVertexDoFType(marker.x(),marker.h());

  switch(dofType)
  {

    case vertexdata::{{SOLVER_NAME}}::Type::Interior:
    {
      if ( marker.willBeRefined() )
      {
        fineGridVertex{{SOLVER_NAME}}.setType( vertexdata::{{SOLVER_NAME}}::Type::Coarse );
      }

      else
      {
        fineGridVertex{{SOLVER_NAME}}.setType( vertexdata::{{SOLVER_NAME}}::Type::Interior );
      }
    }
    break;

    case vertexdata::{{SOLVER_NAME}}::Type::Coarse:
      assertionMsg(false, "should not be returned by user" );
      break;

    case vertexdata::{{SOLVER_NAME}}::Type::Undefined:
      assertionMsg(false, "should not be returned by user" );
      break;
  }


  //logTraceOutWith2Arguments("InitDofsDG::touchVertexFirstTime", marker.toString(), fineGridVertex{{SOLVER_NAME}}.toString());
   """

  templateTouchCellFirstTime = """
  logTraceInWith2Arguments("InitDofs::touchCellFirstTime", marker.toString(), fineGridCell{{SOLVER_NAME}}.toString());

  celldata::{{SOLVER_NAME}}::Type dofType = repositories::{{SOLVER_INSTANCE}}.getCellDoFType(marker.x(),marker.h());
  
  /*
  extremely hacky static random number generator
  */
  static std::mt19937 rng(0); // seed with 0 for reproducibility
  static std::uniform_real_distribution<> dis(0.0, 1.0);

  switch(dofType)
  {
    case celldata::{{SOLVER_NAME}}::Type::Outside:
    {
      if ( marker.willBeRefined() ) // make it coarse
      {
        fineGridCell{{SOLVER_NAME}}.setType( celldata::{{SOLVER_NAME}}::Type::Coarse );
      }
      else
      {
        fineGridCell{{SOLVER_NAME}}.setType( celldata::{{SOLVER_NAME}}::Type::Outside );
      }
    }
    break;

    case celldata::{{SOLVER_NAME}}::Type::Interior:
    {
      if ( marker.willBeRefined() )
      {
        fineGridCell{{SOLVER_NAME}}.setType( celldata::{{SOLVER_NAME}}::Type::Coarse );
      }
      else 
      {
        fineGridCell{{SOLVER_NAME}}.setType( celldata::{{SOLVER_NAME}}::Type::Interior );

        // create vectors to send to implementation
        tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.CellUnknowns, double > solution;
        tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.CellUnknowns, double > rhs;
        tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.CellUnknowns, double > residual;

        // first - fill the solution vector with randoms
        for (int i=0; i<repositories::{{SOLVER_INSTANCE}}.CellUnknowns; i++) {
          solution[i] = dis(rng);

          // and residual to 0...
          residual[i] = 0;
        }

        // set rhs...
        rhs =  repositories::{{SOLVER_INSTANCE}}.getLocalAssemblyMatrix(marker.x(), marker.h()) * solution;

        // send to mesh...
        fineGridCell{{SOLVER_NAME}}.setSolution( solution );
        fineGridCell{{SOLVER_NAME}}.setRhs( rhs );
        fineGridCell{{SOLVER_NAME}}.setResidual( residual );


      }
    }
    break;

  }
  
  logTraceOutWith2Arguments("InitDofs::touchCellFirstTime", marker.toString(), fineGridCell{{SOLVER_NAME}}.toString());
   """

  templatetouchFaceFirstTime = """
  logTraceInWith2Arguments("InitDofs::touchFaceFirstTime", marker.toString(), fineGridFace{{SOLVER_NAME}}.toString());

  facedata::{{SOLVER_NAME}}::Type dofType = repositories::{{SOLVER_INSTANCE}}.getFaceDoFType(marker.x(),marker.h());

  switch(dofType)
  {
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
    } break;

    case facedata::{{SOLVER_NAME}}::Type::Interior:
    {
      if ( marker.willBeRefined() )
      {
        fineGridFace{{SOLVER_NAME}}.setType( facedata::{{SOLVER_NAME}}::Type::Coarse );
      }
      else 
      {
        fineGridFace{{SOLVER_NAME}}.setType( facedata::{{SOLVER_NAME}}::Type::Interior );

        // vectors to be passed into face
        tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.FaceUnknownsSolution,   double > sol;
        tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.FaceUnknownsProjection, double > projection;

        // set all of these values to 0
        for (int i=0; i<repositories::{{SOLVER_INSTANCE}}.FaceUnknownsSolution; i++)
          sol[i] = 0;

        for (int i=0; i<repositories::{{SOLVER_INSTANCE}}.FaceUnknownsProjection; i++)
          projection[i] = 0;
        

        // put into mesh
        fineGridFace{{SOLVER_NAME}}.setSolution(sol);
        fineGridFace{{SOLVER_NAME}}.setProjection(projection);

      }
    } break;

    case facedata::{{SOLVER_NAME}}::Type::Boundary:
    {
      if ( marker.willBeRefined() )
      {
        fineGridFace{{SOLVER_NAME}}.setType( facedata::{{SOLVER_NAME}}::Type::Coarse );
      }
      else 
      {
        fineGridFace{{SOLVER_NAME}}.setType( facedata::{{SOLVER_NAME}}::Type::Boundary );

        // vectors to be passed into face
        tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.FaceUnknownsSolution,   double > sol;
        tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.FaceUnknownsProjection, double > projection;

        // send to init
        repositories::{{SOLVER_INSTANCE}}.initFace(marker.x(), marker.h(), sol, projection);

        // put into mesh
        fineGridFace{{SOLVER_NAME}}.setSolution(sol);
        fineGridFace{{SOLVER_NAME}}.setProjection(projection);
      }
    } break;
    

  }
  
  logTraceOutWith2Arguments("InitDofs::touchFaceFirstTime", marker.toString(), fineGridFace{{SOLVER_NAME}}.toString());
   """
  
  def __init__(self,
               solver):
    super( InitDofsDGTest1, self ).__init__()
    self.d = {}
    self.d["SOLVER_INSTANCE"]    = solver.instance_name()
    self.d["SOLVER_NAME"]        = solver.typename()
    # self.d["VERTEX_CARDINALITY"] = solver._unknowns_per_vertex

  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
      result = jinja2.Template(self.templateTouchVertexFirstTime).render(**self.d)
      pass 
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
#include "repositories/SolverRepository.h"
#include "peano4/utils/Loop.h"
#include<numeric> // for std::iota
#include <random>
"""

