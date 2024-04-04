# This file is part of the Peano's PETSc extension. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2


class InitFaceDoFs(ActionSet):
  """!

Initialise degrees of freedom associated with the cells and Faces.

Much the same as the old code to init DoFs on vertices, except here
we also make the call to the localtoglobalmap. ie we both init the 
degrees of freedom and the global mapping for indices on faces and cells
  
  """
  
  TemplateInitFace="""
    @todo This is a left-over. Not yet maintained
    
    
  // lambda to determine the pair of vertices that determine 
  // whether face i is on the boundary or not
  // eg if vertices 0 and 1 are both dirichlet, then face 1 must
  // be on the boundary
  auto getCrucialIndices = [](int i) -> const std::pair<int,int>{
    static constexpr std::pair<int,int> v0 = {0,2};
    static constexpr std::pair<int,int> v1 = {0,1};
    static constexpr std::pair<int,int> v2 = {1,3};
    static constexpr std::pair<int,int> v3 = {2,3};
    if (i==0)      return v0;
    else if (i==1) return v1; 
    else if (i==2) return v2;
    else if (i==3) return v3;
    else throw std::runtime_error("invalid index passed!"); 
  };

  if (!marker.willBeRefined()){
    logTraceIn( "touchFaceFirstTime(...)---InitialCondition" );

    int faceNumber = marker.getSelectedFaceNumber();
    std::pair<int,int> crucialIndices = getCrucialIndices(faceNumber);
    if(
      fineGridVertices{{SOLVER_NAME}}(crucialIndices.first).getType()  == vertexdata::{{SOLVER_NAME}}::Type::Dirichlet
      and
      fineGridVertices{{SOLVER_NAME}}(crucialIndices.second).getType() == vertexdata::{{SOLVER_NAME}}::Type::Dirichlet
    ){
      fineGridFace{{SOLVER_NAME}}.setType(facedata::{{SOLVER_NAME}}::Type::Boundary);
    }
    else{
      fineGridFace{{SOLVER_NAME}}.setType(facedata::{{SOLVER_NAME}}::Type::Interior);
    }

    //init vectors for value and rhs. modify them in initFace()
    tarch::la::Vector<{{FACE_CARDINALITY}}, double>   value;
    tarch::la::Vector<{{FACE_CARDINALITY}}, double>   rhs;

    repositories::{{SOLVER_INSTANCE}}.initFace(
      marker.x(),
      marker.h(),
      value,
      rhs
    );

    //place these values into the mesh
    fineGridFace{{SOLVER_NAME}}.setValue( value );
    fineGridFace{{SOLVER_NAME}}.setRhs( rhs );

    logTraceOut( "touchFaceFirstTime(...)---InitialCondition" );

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
    super( InitFaceAndCellDoFs, self ).__init__()
    self.d = {}
    self.d["SOLVER_INSTANCE"]        = solver.instance_name()
    self.d["SOLVER_NAME"]            = solver.typename()
#    self.d["FACE_CARDINALITY"]       = solver.number_of_face_unknowns
#    self.d["FACE_DOFS"]              = solver.number_of_face_dofs
#    self.d["CELL_CARDINALITY"]       = solver.number_of_cell_unknowns
#    self.d["CELL_DOFS"]              = solver.number_of_cell_dofs

  def get_body_of_operation(self,operation_name):
    """!

    
    """
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME:
      result = jinja2.Template(self.TemplateInitFace).render(**self.d)
      pass 
    
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = jinja2.Template(self.TemplateInitCell).render(**self.d)
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
    """
    return """
"""

  def get_constructor_body(self):
    return """
"""

  def get_includes(self):
    """!
   
Consult petsc.Project for details
    
"""    
    return """
#include "repositories/SolverRepository.h"
"""
