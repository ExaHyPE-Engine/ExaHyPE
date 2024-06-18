# This file is part of the Peano's PETSc extension. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2


class InitVertexDoFs(ActionSet):
  """!

Initialise degrees of freedom associated with the mesh
    
This simple action set plugs into the situations where we touch a vertex
for the first time throughout a mesh traversal. Of these events, it ignores
all those for refined vertices, i.e. it solely handles the unrefined 
vertices in the spacetree. If focuses on the fine grid. The same is done
for the cells and the vertices.

The injected code snippet is simple and basically asks if a vertex will be 
refined. If not, it invokes the initialisation routine of the solver. For
this, the action set has to know the typename and the instance name of the 
solver. Besides the actual behaviour, the action set also has to include
the solver in the first place. This happens in get_includes().
  
  """
  TemplateInitVertex = """
  if ( fineGridVertex{{SOLVER_NAME}}PETScData.getType() != vertexdata::{{SOLVER_NAME}}PETScData::Type::Coarse ) {
    
    tarch::la::Vector<{{VERTEX_CARDINALITY}}, double> value;
    tarch::la::Vector<{{VERTEX_CARDINALITY}}, double> rhs;

    // use initVertex to get all the values we want to place
    // into each vertex dof
    repositories::{{SOLVER_INSTANCE}}.initVertex(
      marker.x(),
      marker.h(),
      value,
      rhs
    );

    for (int i=0; i<{{VERTEX_CARDINALITY}}; i++)
    {
      fineGridVertex{{SOLVER_NAME}}.setValue(i, value[i]);
      fineGridVertex{{SOLVER_NAME}}.setRhs(i, rhs[i]);
    }
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
    self.d["SOLVER_INSTANCE"]    = solver.instance_name()
    self.d["SOLVER_NAME"]        = solver.typename()
    self.d["VERTEX_CARDINALITY"] = solver.number_of_matrix_entries_per_vertex
    self.d["FACE_CARDINALITY"]   = solver.number_of_matrix_entries_per_face
    self.d["CELL_CARDINALITY"]   = solver.number_of_matrix_entries_per_cell


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
   
    Add some additional includes
    
    We need the solver repository here as we access hte solver object, and we
    also need access to all of Peano's d-dimensional for loops.
    
    """    
    return """
#include "repositories/SolverRepository.h"
#include "peano4/utils/Loop.h"
"""

