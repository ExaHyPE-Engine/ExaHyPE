from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2


class InitDofsCollocated(ActionSet):
  """!
  In touchVertexFirstTime, we set the type of vertex we see (either Interior, Boundary or Coarse),
  as well as sending some values to the solver implementation file to be placed into the mesh.

  In touchCellFirstTime, we just determine the type, as we don't store anything on cells in 
  this solver.
  """

  templateTouchVertexFirstTime = """
  // this is done inside getDofType. included here for sanity check
  auto isOnBoundary = [&]( const tarch::la::Vector< Dimensions, double > & x ) -> bool{
    bool isOnBoundary = false;
    for (int d=0; d<Dimensions; d++) {
      isOnBoundary |= tarch::la::smallerEquals( x(d), DomainOffset(d) );
      isOnBoundary |= tarch::la::greaterEquals( x(d), DomainOffset(d)+DomainSize(d) );
    }
    return isOnBoundary;
  };

  logTraceInWith3Arguments("InitDofs::touchVertexFirstTime", marker.toString(), isOnBoundary(marker.x()), fineGridVertex{{SOLVER_NAME}}.toString());

  vertexdata::{{SOLVER_NAME}}::Type dofType = repositories::{{SOLVER_INSTANCE}}.getVertexDoFType(marker.x(),marker.h());

  switch(dofType)
  {
    case vertexdata::{{SOLVER_NAME}}::Type::Boundary:
    {
      if ( marker.willBeRefined() )
      {
        // Coarse grid vertex. Mark it as such for later
        fineGridVertex{{SOLVER_NAME}}.setType( vertexdata::{{SOLVER_NAME}}::Type::Coarse );
      }
      else
      {
        // we have boundary vertex
        fineGridVertex{{SOLVER_NAME}}.setType( vertexdata::{{SOLVER_NAME}}::Type::Boundary );
        // init its value too
        tarch::la::Vector<{{VERTEX_CARDINALITY}}, double> value;
        tarch::la::Vector<{{VERTEX_CARDINALITY}}, double> rhs;

        // send these into init
        repositories::{{SOLVER_INSTANCE}}.initVertex( marker.x(), marker.h(), value, rhs );

        // store 
        for (int dof=0; dof<{{VERTEX_CARDINALITY}}; dof++)
        {
          fineGridVertex{{SOLVER_NAME}}.setValue(dof, value(dof));
          fineGridVertex{{SOLVER_NAME}}.setRhs(  dof,   rhs(dof));
        }
      }
    }
    break;

    case vertexdata::{{SOLVER_NAME}}::Type::Interior:
    {
      if (isOnBoundary(marker.x())) 
      {
        logWarning( "touchVertexFirstTime(...)", "vertex at " << marker.toString() << " labelled as interior even though it is located at global domain boundary" );
      }

      if ( marker.willBeRefined() )
      {
        fineGridVertex{{SOLVER_NAME}}.setType( vertexdata::{{SOLVER_NAME}}::Type::Coarse );
      }

      else
      {
        fineGridVertex{{SOLVER_NAME}}.setType( vertexdata::{{SOLVER_NAME}}::Type::Interior );

        // init its value too
        tarch::la::Vector<{{VERTEX_CARDINALITY}}, double> value;
        tarch::la::Vector<{{VERTEX_CARDINALITY}}, double> rhs;

        // send these into init
        repositories::{{SOLVER_INSTANCE}}.initVertex( marker.x(), marker.h(), value, rhs );

        // store 
        for (int dof=0; dof<{{VERTEX_CARDINALITY}}; dof++)
        {
          fineGridVertex{{SOLVER_NAME}}.setValue(dof, value(dof));
          fineGridVertex{{SOLVER_NAME}}.setRhs(  dof,   rhs(dof));
        }
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


  logTraceOutWith3Arguments("InitDofs::touchVertexFirstTime", marker.toString(), isOnBoundary(marker.x()), fineGridVertex{{SOLVER_NAME}}.toString());
   """

  templateTouchCellFirstTime = """
  logTraceInWith2Arguments("InitDofs::touchCellFirstTime", marker.toString(), fineGridCell{{SOLVER_NAME}}.toString());

  celldata::{{SOLVER_NAME}}::Type dofType = repositories::{{SOLVER_INSTANCE}}.getCellDoFType(marker.x(),marker.h());

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
      }
    }
    break;

  }
  
  logTraceOutWith2Arguments("InitDofs::touchCellFirstTime", marker.toString(), fineGridCell{{SOLVER_NAME}}.toString());
   """
  
  def __init__(self,
               solver):
    super( InitDofsCollocated, self ).__init__()
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

class InitDofsDG(ActionSet):
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
        tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.CellUnknowns, double > value;
        tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.CellUnknowns, double > rhs;
        // send to init
        repositories::{{SOLVER_INSTANCE}}.initCell(marker.x(), marker.h(), value, rhs);

        logTraceInWith5Arguments("touchCellFirstTime::Interior", fineGridCell{{SOLVER_NAME}}.getSolution(), fineGridCell{{SOLVER_NAME}}.getRhs(), marker.toString(), value, rhs);

        // adding junk data to the solution and rhs for development purposes
        for (int i=0; i<repositories::{{SOLVER_INSTANCE}}.CellUnknowns; i++)
        {
          fineGridCell{{SOLVER_NAME}}.setSolution(i,value(i));
          fineGridCell{{SOLVER_NAME}}.setRhs(i,rhs(i));
        }

        logTraceOutWith2Arguments("touchCellFirstTime::Interior", fineGridCell{{SOLVER_NAME}}.getSolution(), fineGridCell{{SOLVER_NAME}}.getRhs());
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

        // send to init
        repositories::{{SOLVER_INSTANCE}}.initFace(marker.x(), marker.h(), sol, projection);

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
    super( InitDofsDG, self ).__init__()
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
"""

