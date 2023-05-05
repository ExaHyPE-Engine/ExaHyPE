# This file is part of the Peano's PETSc extension. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2

class AssemblePetscMatrix(ActionSet):
  """!
  Trigger assembly of PETSc matrix, using values from mesh
  """
  TemplateInitCell = """
  //init a vector to collect global indices, and recall their position in the 
  //enumeration so we can apply the stencil correctly
  std::vector<int> enumeratedIndices(TwoPowerD, -1);

  for (int i=0; i<TwoPowerD ; i++){
    if (fineGridVerticesPoisson(i).getType() != vertexdata::Poisson::Type::Dirichlet
        and fineGridVerticesPoisson(i).getNumber() < repositories::getTotalNumberOfDegreesOfFreedom()
    ) {
      std::pair<int,int> localIndex = std::make_pair(_spacetreeId, fineGridVerticesPoisson(i).getNumber());
      int globalIndex = repositories::{{SOLVER_INSTANCE}}.getLocalToGlobalMap().getGlobalIndex(localIndex);
      enumeratedIndices[i] = globalIndex;
    }

    //get a global index
  }

  //next, loop twice over TwoPowerD, so we can correctly fill the petsc matrix.
  //calling our increment function 16 times is temporary; this can be done with 
  //one call to PETSc. While in development, we do it the dumb way.
  for (int i=0; i<TwoPowerD; i++){
    for (int j=0; j<TwoPowerD; j++){
    if (enumeratedIndices[i] != -1 and enumeratedIndices[j] != -1){
      if (i == j){
        repositories::instanceOfPetsc.incrementDoubleAtIndexij(enumeratedIndices[i], enumeratedIndices[j], 2.0/3.0); //2/3 /h
      }

      // if they add up to 3, then they must be opposites - i know this is a bodge
      else if (i + j == 3){
        repositories::instanceOfPetsc.incrementDoubleAtIndexij(enumeratedIndices[i], enumeratedIndices[j], -1.0/3.0);
      }

      //final two cases
      else{
        repositories::instanceOfPetsc.incrementDoubleAtIndexij(enumeratedIndices[i], enumeratedIndices[j], -1.0/6.0);    
      }
    }

    }
  }
  
  """

  TemplateInitVertex="""
  //here we send the value, rhs from the mesh to petsc

  if (fineGridVertex{{SOLVER_NAME}}.getType() != vertexdata::Poisson::Type::Dirichlet
      and fineGridVertex{{SOLVER_NAME}}.getNumber() < repositories::getTotalNumberOfDegreesOfFreedom()
  ){
    //first, get global index
    std::pair<int,int> localIndex = std::make_pair(_spacetreeId, fineGridVertex{{SOLVER_NAME}}.getNumber());
    int globalIndex = repositories::{{SOLVER_INSTANCE}}.getLocalToGlobalMap().getGlobalIndex(localIndex);

    //get the val, rhs that was present in the mesh
    double val = fineGridVertex{{SOLVER_NAME}}.getValue(0);
    double rhs = fineGridVertex{{SOLVER_NAME}}.getRhs(0);

    //send these to petsc
    repositories::instanceOfPetsc.insertValueAndRHS(val, rhs, globalIndex);
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
    super( AssemblePetscMatrix, self ).__init__()
    self.d = {}
    self.d["SOLVER_INSTANCE"] = solver.instance_name()
    self.d["SOLVER_NAME"]     = solver.typename()
    self.d["CARDINALITY"]     = solver.number_of_unknowns


  def get_body_of_operation(self,operation_name):
    """!

Provide C++ code snippet for peano4.solversteps.ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME  
Provide C++ code snippet for peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME  
  
Only touchVertexFirstTime is an event where this action set actually
does something: It inserts the template TemplateInitVertex and 
replaces it with entries from the dictionary. The latter is befilled
in init().

We actually do something during touchVertexFirstTime and touchCellFirstTime. We insert the 
appropriate template into each.
    
    """
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = jinja2.Template(self.TemplateInitCell).render(**self.d)
      pass 
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

@see get_attributes()
    
    """
    return """
    _spacetreeId = treeNumber;
"""
