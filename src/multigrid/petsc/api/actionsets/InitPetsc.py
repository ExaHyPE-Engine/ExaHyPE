# This file is part of the Peano's PETSc extension. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2


class SendDofsToVertices(ActionSet):
  """!

  use this class to store some indices on each vertex inside touchCellFirstTime!
  
  """

  templateTouchCellFirstTime="""
  //lambda to insert a value into first uninitialised position.
  //these vectors attached to cells will have all their entries 
  //initialised to -1 otherwise

  auto insertValueIntoVertex = [&](const int& vertex, const int& dofIndex){
    //here we will insert dofIndex into the first empty position in the 
    //vector held by fineGridVertices{{SOLVER_NAME}}(vertex)
    int indexWithinVector = 2*TwoPowerD; //deliberately set it to be too large

    //we have TwoPowerD entries in this vector
    for (int i=0; i<TwoPowerD; i++){
      if ( fineGridVertices{{SOLVER_NAME}}(vertex).getDofIndices(i) == -1 )
        indexWithinVector = std::min(i, indexWithinVector);
    }
    if(indexWithinVector < 0 or indexWithinVector > TwoPowerD)
      throw std::runtime_error("error in inserting to vector!");
    fineGridVertices{{SOLVER_NAME}}(vertex).setDofIndices(indexWithinVector,dofIndex);
  };

  if (not marker.willBeRefined()){
    //get global index for the index held by this cell
    std::pair<int,int> localIndex( _spacetreeId, fineGridCell{{SOLVER_NAME}}.getNumber() );
    int globalIndex =  repositories::getGlobalCellIndex( localIndex );

    //vertex 0 takes the globalIndex + 0;
    //vertex 1 takes the globalIndex + n; etc
    static_assert(Dimensions==2, "hardcoding 2d for time being");

    //worry about boundary when it comes to assembly time
    insertValueIntoVertex(0, globalIndex);
    insertValueIntoVertex(1, globalIndex + {{POLYNOMIAL_DEGREE}});
    insertValueIntoVertex(2, globalIndex + {{DEGPLUS1POWERD}} - ({{POLYNOMIAL_DEGREE}} + 1));
    insertValueIntoVertex(3, globalIndex + {{DEGPLUS1POWERD}} - 1);

  }

  """
  
  
  def __init__(self,
               solver):
    """!
    
    
    """
    super( SendDofsToVertices, self ).__init__()
    self.d = {}
    self.d["SOLVER_INSTANCE"]        = solver.instance_name()
    self.d["SOLVER_NAME"]            = solver.typename()
    self.d["POLYNOMIAL_DEGREE"]      = solver.polynomial_degree
    self.d["DEGPLUS1POWERD"]         = (solver.polynomial_degree+1) ** solver.dimensions

  def get_body_of_operation(self,operation_name):
    """!

    """
    result = ""
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


  def get_attributes(self):
    """!
    
Define the local map

Every action set has excactly one attribute and that's an instance of
petsc::LocalToGlobalMap. Before we run through a local mesh partition,
the corresponding observer object will create a copy of the action set
for this traversal. In the corresponding constructor, we initialise
our thread-local copy of the map with the correct tree number.
    
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


  def get_includes(self):
    """!
   
Consult petsc.Project for details
    
"""    
    return """
#include "repositories/SolverRepository.h"
"""
