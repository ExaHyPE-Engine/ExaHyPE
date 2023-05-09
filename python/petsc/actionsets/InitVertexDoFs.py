# This file is part of the Peano's PETSc extension. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2


class InitVertexDoFs(ActionSet):
  """!

Initialise degrees of freedom associated with the mesh vertices
    
This simple action set plugs into the situations where we touch a vertex
for the first time throughout a mesh traversal. Of these events, it ignores
all those for refined vertices, i.e. it solely handles the unrefined 
vertices in the spacetree. If focuses on the fine grid.

The injected code snippet is simple and basically asks if a vertex will be 
refined. If not, it invokes the initialisation routine of the solver. For
this, the action set has to know the typename and the instance name of the 
solver. Besides the actual behaviour, the action set also has to include
the solver in the first place. This happens in get_includes().
  
  """
  
  
  # TemplateInitVertex = """
  # if ( not marker.willBeRefined() ) { 
  #   logTraceIn( "touchCellFirstTime(...)---InitialCondition" );
    
  #   repositories::{{SOLVER_INSTANCE}}.init(
  #     marker.x(),
  #     marker.h(),
  #     {% if CARDINALITY==1 %}
  #     *(fineGridVertex{{SOLVER_NAME}}.getValue().data()),
  #     *(fineGridVertex{{SOLVER_NAME}}.getRhs().data())
  #     {% else %}
  #     fineGridVertex{{SOLVER_NAME}}.getValue(),
  #     fineGridVertex{{SOLVER_NAME}}.getRhs()
  #     {% endif %}
  #   );
    
  #   logTraceOut( "touchCellFirstTime(...)---InitialCondition" );
  # } 
#  """

#TODO - change calls to setValue and setRhs to reflect higher dimensions - we'll have to consider
# more than the 0th index
  TemplateInitVertex = """
  auto isOnBoundary = [&]( const tarch::la::Vector< Dimensions, double > & vertex ) -> bool {
    //for now, hardcode 2 dimensions
    double x = vertex[0];
    double y = vertex[1];

    if ( x < 1E-10 or x > 1 - 1E-10) return true;
    if ( y < 1E-10 or y > 1 - 1E-10) return true;
    return false;
  };

  switch ( isOnBoundary(marker.x()) ){
    case true:
      fineGridVertex{{SOLVER_NAME}}.setType( vertexdata::Poisson::Type::Dirichlet );
      break;
    case false:
      fineGridVertex{{SOLVER_NAME}}.setType( vertexdata::Poisson::Type::DoF );
      break;
  }

  if ( not marker.willBeRefined() ) { 
    logTraceIn( "touchCellFirstTime(...)---InitialCondition" );

    //init two doubles; pass these by reference into init()
    double value, rhs;
    
    repositories::{{SOLVER_INSTANCE}}.init(
      marker.x(),
      marker.h(),
      value,
      rhs
    );

    if (fineGridVertex{{SOLVER_NAME}}.getType() != vertexdata::Poisson::Type::Dirichlet){
      //place these values into the grid
      fineGridVertex{{SOLVER_NAME}}.setValue(0, value);
      fineGridVertex{{SOLVER_NAME}}.setRhs(0, rhs);

    }
    
    logTraceOut( "touchCellFirstTime(...)---InitialCondition" );
  } 
"""
  
  def __init__(self,
               solver):
    """!
    
Initialise vertex-associated degrees of freedom
    
The initialisation requires a solver object, as we have to know what C++
object this solver will produce.

solver: petsc.solvers.CollocatedLowOrderDiscretisation or similar solver where
  degrees of freedom are assigned exclusively to the vertices.
    
    """
    super( InitVertexDoFs, self ).__init__()
    self.d = {}
    self.d["SOLVER_INSTANCE"] = solver.instance_name()
    self.d["SOLVER_NAME"]     = solver.typename()
    self.d["CARDINALITY"]     = solver.number_of_unknowns


  def get_body_of_operation(self,operation_name):
    """!

Provide C++ code snippet for peano4.solversteps.ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME  
  
Only touchVertexFirstTime is an event where this action set actually
does something: It inserts the template TemplateInitVertex and 
replaces it with entries from the dictionary. The latter is befilled
in init().
    
    """
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
      result = jinja2.Template(self.TemplateInitVertex).render(**self.d)
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
   
Consult petsc.Project for details
    
"""    
    return """
#include "repositories/SolverRepository.h"
"""
