from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2



class InitDofsTest3(ActionSet):
  """!
  In touchVertexFirstTime, we set the type of vertex we see (either Interior, Boundary or Coarse),
  as well as sending some values to the solver implementation file to be placed into the mesh.

  In touchCellFirstTime, we just determine the type, as we don't store anything on cells in 
  this solver.
  """

  templateTouchVertexFirstTime = """
  static constexpr double eightPiSquared = 8 * tarch::la::PI * tarch::la::PI;

  double sinValue = 1;
  double sinRhs   = eightPiSquared;
  for (int i=0; i<Dimensions; i++){
    sinValue *= std::sin( 2*tarch::la::PI * marker.x()(i) );
    sinRhs   *= std::sin( 2*tarch::la::PI * marker.x()(i) );
  }

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

        // store 
        for (int dof=0; dof<{{VERTEX_CARDINALITY}}; dof++)
        {
          fineGridVertex{{SOLVER_NAME}}.setValue( dof, sinValue);
          fineGridVertex{{SOLVER_NAME}}.setRhs(   dof, sinRhs);
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

        // store 
        for (int dof=0; dof<{{VERTEX_CARDINALITY}}; dof++)
        {
          fineGridVertex{{SOLVER_NAME}}.setValue(dof, sinValue);
          fineGridVertex{{SOLVER_NAME}}.setRhs(  dof, sinRhs);
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
        // perform initialisation proc
        // this does not work for more than one unknown per vertex

        tarch::la::Vector<TwoPowerD,double> vu_values;
        tarch::la::Vector<TwoPowerD,double> vb_values;

        // collect
        for (int i=0; i<TwoPowerD; i++){
          vu_values[i] = fineGridVertices{{SOLVER_NAME}}(i).getValue(0);
          vb_values[i] = fineGridVertices{{SOLVER_NAME}}(i).getRhs(0);
        }

        // multiply v_random by A_cg
        vu_values = repositories::{{SOLVER_INSTANCE}}.getLocalAssemblyMatrix(marker.x(), marker.h()) * vu_values;
        vb_values = repositories::{{SOLVER_INSTANCE}}.getLocalAssemblyMatrix(marker.x(), marker.h()) * vb_values;

        // place this into rhs
        for (int i=0; i<TwoPowerD; i++){
          fineGridVertices{{SOLVER_NAME}}(i).setVu(
            fineGridVertices{{SOLVER_NAME}}(i).getVu() + vu_values(i)
          );

          fineGridVertices{{SOLVER_NAME}}(i).setVb(
            fineGridVertices{{SOLVER_NAME}}(i).getVb() + vb_values(i)
          );
        }

      }
    }
    break;

  }
  
  logTraceOutWith2Arguments("InitDofs::touchCellFirstTime", marker.toString(), fineGridCell{{SOLVER_NAME}}.toString());
   """
  
  templateTouchVertexLastTime="""
  
  """

  def __init__(self,
               solver):
    super( InitDofsTest3, self ).__init__()
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
