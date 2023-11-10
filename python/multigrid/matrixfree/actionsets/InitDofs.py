from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2


class InitDofs(ActionSet):
  # need to set the type of the vertex and give it a value
  templateTVFT = """
  // this is done inside getDofType. included here for sanity check
  auto isOnBoundary = [&]( const tarch::la::Vector< Dimensions, double > & x ) -> bool{
    bool isOnBoundary = false;
    for (int d=0; d<Dimensions; d++) {
      isOnBoundary |= tarch::la::smallerEquals( x(d), DomainOffset(d) );
      isOnBoundary |= tarch::la::greaterEquals( x(d), DomainOffset(d)+DomainSize(d) );
    }
    return isOnBoundary;
  };

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

  logTraceInWith3Arguments("InitDofs::TVFT", marker.toString(), isOnBoundary(marker.x()), fineGridVertex{{SOLVER_NAME}}.toString());

  logTraceOut("InitDofs::TVFT");
   """
  templateTCFT = """ """
  
  def __init__(self,
               solver):
    super( InitDofs, self ).__init__()
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
