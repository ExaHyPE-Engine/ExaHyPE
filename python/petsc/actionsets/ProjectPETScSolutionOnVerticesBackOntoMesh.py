# This file is part of the Peano's PETSc extension. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2


class ProjectPETScSolutionOnVerticesBackOntoMesh(ActionSet):
  """!

Project solution vector from PETSc back onto vertices
    
This action set works if and only if your PETSc data is associated with
the vertices only. If runs over through the mesh. Each mesh vertex has 
a tree number and a (local) number. We take this tuple and ask the 
map what the global dof index is. With this index, we ask PETSc what the
corresponding entry in the solution vector reads like.
  
  """
  
  
  TemplateInitVertex = """
  if ( not marker.willBeRefined() ) { 
    logTraceIn( "touchCellFirstTime(...)---InitialCondition" );
    
    for (int i=0; i<{{CARDINALITY}}; i++) {
      std::pair<int,int> localIndex( _spacetreeId, fineGridVertex{{SOLVER_NAME}}.getNumber()+i );
      int globalIndex = repositories::{{SOLVER_INSTANCE}}.getLocalToGlobalMap().getGlobalIndex(localIndex);

      // ask our instance of Petsc the value stored in the solution vector here.
      double newValue = repositories::instanceOfPetsc.getX(globalIndex);
      fineGridVertex{{SOLVER_NAME}}.getValue().data()[i] = newValue;
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
    super( ProjectPETScSolutionOnVerticesBackOntoMesh, self ).__init__()
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

  def get_attributes(self):
    """!
    
    
    """
    return """
  int _spacetreeId;    
"""
      
  def get_constructor_body(self):
    """!
    
Define body of constructor

Consult the superclass' description of the function for results. I 
basically initialise the _localMap with the correct tree number.

@see get_attributes()
    
    """
    return """
  _spacetreeId = treeNumber;
"""
