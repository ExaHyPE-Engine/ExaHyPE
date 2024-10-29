"""!
The action sets in this file are intended for use with new MG solvers.

We have a mix of behaviours to add in - namely determining max refinement
level during our first descent, as well as standard stuff eg determining
which facets are boundary / interior etc.

For the CG, we need to determine:
  - the number of refinement levels available
  - the level of the finest level

We need to add some bookkeeping to the abstract solver to keep track of
these, because the solver level matters.

We need to associate an int "level" with each vertex, so we give each Vertex
this attribute (Level) and set it to -1 initially. Then during
touchVertexFirstTime, we set the "level" of the fineGridVertex to be one more
than the level of the coarse vertex

Slight problem is that we don't have the coarse vertex in scope at this stage.
So we need to maintain this hierarchy for the cells, as well as setting the 
level of the vertices to be one more than the coarse cells.
"""


from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2

class InitDofsCollocatedMG(ActionSet):
  templateTouchCellFirstTime="""
  
  // set the level...
  fineGridCell{{SOLVER_NAME}}.setLevel(
    coarseGridCell{{SOLVER_NAME}}.getLevel() + 1
  );
  
  celldata::{{SOLVER_NAME}}::Type dofType = repositories::{{SOLVER_INSTANCE}}.getCellDoFType(marker.x(),marker.h());
  switch(dofType)
  {
    case celldata::{{SOLVER_NAME}}::Type::Outside:
    {
      assertion(false);
    }
    break;

    case celldata::{{SOLVER_NAME}}::Type::Interior:
    {
      fineGridCell{{SOLVER_NAME}}.setType( celldata::{{SOLVER_NAME}}::Type::Interior );
      repositories::{{SOLVER_INSTANCE}}.updateMinMaxMeshSize( marker.h() );
    }
    break;
  }

  """

  templateTouchVertexFirstTime="""

  // set its level...
  fineGridVertex{{SOLVER_NAME}}.setLevel(
    coarseGridCell{{SOLVER_NAME}}.getLevel() + 1
  );

  tarch::la::Vector<{{VERTEX_CARDINALITY}}, double> zeroes { 0 };
  fineGridVertex{{SOLVER_NAME}}.setResidual( zeroes );
  fineGridVertex{{SOLVER_NAME}}.setDelta( zeroes );
  fineGridVertex{{SOLVER_NAME}}.setOldU( zeroes );
  fineGridVertex{{SOLVER_NAME}}.setDiag( zeroes );

  vertexdata::{{SOLVER_NAME}}::Type dofType = repositories::{{SOLVER_INSTANCE}}.getVertexDoFType(marker.x(),marker.h());

  switch(dofType)
  {
    case vertexdata::{{SOLVER_NAME}}::Type::Boundary:
    {
      fineGridVertex{{SOLVER_NAME}}.setType( vertexdata::{{SOLVER_NAME}}::Type::Boundary );
      if ( !marker.willBeRefined() ) {
        // we have boundary vertex
        // init its solution too
        tarch::la::Vector<{{VERTEX_CARDINALITY}}, double> u;
        tarch::la::Vector<{{VERTEX_CARDINALITY}}, double> rhs;

        // send these into init
        repositories::{{SOLVER_INSTANCE}}.initVertex( marker.x(), marker.h(), u, rhs );

        // store 
        fineGridVertex{{SOLVER_NAME}}.setU( u );
        fineGridVertex{{SOLVER_NAME}}.setRhs( rhs );
        
        // if we are on the finest level, we need to alert the abstract solver...
        repositories::{{SOLVER_INSTANCE}}.setActiveLevel( fineGridVertex{{SOLVER_NAME}}.getLevel() );
        repositories::{{SOLVER_INSTANCE}}.setFinestLevel( fineGridVertex{{SOLVER_NAME}}.getLevel() );
      }
    }
    break;

    case vertexdata::{{SOLVER_NAME}}::Type::Interior:
    {
      fineGridVertex{{SOLVER_NAME}}.setType( vertexdata::{{SOLVER_NAME}}::Type::Interior );

      if ( !marker.willBeRefined() ){
        // init its solution since we are on the finest level
        tarch::la::Vector<{{VERTEX_CARDINALITY}}, double> u;
        tarch::la::Vector<{{VERTEX_CARDINALITY}}, double> rhs;

        // send these into init
        repositories::{{SOLVER_INSTANCE}}.initVertex( marker.x(), marker.h(), u, rhs );

        // store 
        fineGridVertex{{SOLVER_NAME}}.setU( u );
        fineGridVertex{{SOLVER_NAME}}.setRhs( rhs );

        // if we are on the finest level, we need to alert the abstract solver...
        repositories::{{SOLVER_INSTANCE}}.setActiveLevel( fineGridVertex{{SOLVER_NAME}}.getLevel() );
        repositories::{{SOLVER_INSTANCE}}.setFinestLevel( fineGridVertex{{SOLVER_NAME}}.getLevel() );
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


  """
  def __init__(self,
               solver):
    super( InitDofsCollocatedMG, self ).__init__()
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
