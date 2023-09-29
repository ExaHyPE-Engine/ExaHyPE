# This file is part of the Peano's PETSc extension. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2


class EnumerateDofs(ActionSet):
  """!

  Enumerate the vertices, cells and face entries
  
  """
  
  
  TemplateEnumerateVertex = """
  auto isOnBoundary = [&]( const tarch::la::Vector< Dimensions, double > & x ) -> bool{
    bool isOnBoundary = false;
    for (int d=0; d<Dimensions; d++) {
      isOnBoundary |= tarch::la::smallerEquals( x(d), DomainOffset(d) );
      isOnBoundary |= tarch::la::greaterEquals( x(d), DomainOffset(d)+DomainSize(d) );
    }
    return isOnBoundary;
  };

  fineGridVertex{{SOLVER_NAME}}PETScData.setUnknownBaseNumber( ::petsc::LocalToGlobalMap::NoDegreeOfFreedom );
  
  if ( not marker.willBeRefined() ) { 
    switch ( isOnBoundary(marker.x()) ){
      case true:
        fineGridVertex{{SOLVER_NAME}}PETScData.setType( vertexdata::{{SOLVER_NAME}}PETScData::Type::Boundary );
        break;
      case false:
        fineGridVertex{{SOLVER_NAME}}PETScData.setType( vertexdata::{{SOLVER_NAME}}PETScData::Type::Interior );
        {% if VERTEX_CARDINALITY>0 %}
        fineGridVertex{{SOLVER_NAME}}PETScData.setUnknownBaseNumber( _localVertexMap.reserveIndex( {{VERTEX_CARDINALITY}} ) );
        {% endif %}
        break;
    }
  }
  else {
    fineGridVertex{{SOLVER_NAME}}PETScData.setType( vertexdata::{{SOLVER_NAME}}PETScData::Type::Coarse );
  }
"""

  
  TemplateEnumerateFace = """
  auto isOnBoundary = [&]( const tarch::la::Vector< Dimensions, double >&  x) -> bool {
      bool isOnBoundary = false;
      for (int d=0; d<Dimensions; d++) {
        isOnBoundary |= tarch::la::smallerEquals( x(d), DomainOffset(d) );
        isOnBoundary |= tarch::la::greaterEquals( x(d), DomainOffset(d)+DomainSize(d) );
      }
      return isOnBoundary;
  };

  fineGridFace{{SOLVER_NAME}}PETScData.setUnknownBaseNumber( ::petsc::LocalToGlobalMap::NoDegreeOfFreedom );
  
  if ( not marker.willBeRefined()  ) { 
    switch ( isOnBoundary(marker.x()) ){
      case true:
        fineGridFace{{SOLVER_NAME}}PETScData.setType( facedata::{{SOLVER_NAME}}PETScData::Type::Boundary );
        break;
      case false:
        fineGridFace{{SOLVER_NAME}}PETScData.setType( facedata::{{SOLVER_NAME}}PETScData::Type::Interior );
        {% if FACE_CARDINALITY>0 %}
        fineGridFace{{SOLVER_NAME}}PETScData.setUnknownBaseNumber( _localFaceMap.reserveIndex( {{FACE_CARDINALITY}} ) );
        {% endif %}
        break;
    }
  } 
  else {
    fineGridFace{{SOLVER_NAME}}PETScData.setType( facedata::{{SOLVER_NAME}}PETScData::Type::Coarse );
  }
"""

  
  TemplateEnumerateCell = """
  fineGridCell{{SOLVER_NAME}}PETScData.setUnknownBaseNumber( ::petsc::LocalToGlobalMap::NoDegreeOfFreedom );
  
  if ( not marker.willBeRefined()  ) { 
    fineGridCell{{SOLVER_NAME}}PETScData.setType( celldata::{{SOLVER_NAME}}PETScData::Type::Interior );
    {% if CELL_CARDINALITY>0 %}
    fineGridCell{{SOLVER_NAME}}PETScData.setUnknownBaseNumber( _localCellMap.reserveIndex( {{CELL_CARDINALITY}} ) );
    {% endif %}
  } 
  else {
    fineGridCell{{SOLVER_NAME}}PETScData.setType( celldata::{{SOLVER_NAME}}PETScData::Type::Coarse );
  }
"""

  
  TemplateEndTraversal = """
  repositories::{{SOLVER_INSTANCE}}.getLocalToGlobalMap().merge( _localVertexMap );
  repositories::{{SOLVER_INSTANCE}}.getLocalToGlobalMap().merge( _localFaceMap );
  repositories::{{SOLVER_INSTANCE}}.getLocalToGlobalMap().merge( _localCellMap );
"""


  def __init__(self,
               solver):
    """!A
    
Enumerate vertex-associated degrees of freedom
    
The initialisation requires a solver object, as we have to know what C++
object this solver will produce, and we need to know how many unknowns
we have per vertex.

solver: petsc.solvers.CollocatedLowOrderDiscretisation or similar solver where
  degrees of freedom are assigned exclusively to the vertices.
    
    """
    super( EnumerateDofs, self ).__init__()
    self.d = {}
    self.d["SOLVER_INSTANCE"]        = solver.instance_name()
    self.d["SOLVER_NAME"]            = solver.typename()
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
      result = jinja2.Template(self.TemplateEnumerateVertex).render(**self.d)
      pass 
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = jinja2.Template(self.TemplateEnumerateCell).render(**self.d)
      pass 
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME:
      result = jinja2.Template(self.TemplateEnumerateFace).render(**self.d)
      pass 
    if operation_name==peano4.solversteps.ActionSet.OPERATION_END_TRAVERSAL:
      result = jinja2.Template(self.TemplateEndTraversal).render(**self.d)
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
  ::petsc::LocalToGlobalMap  _localCellMap;
  ::petsc::LocalToGlobalMap  _localFaceMap;
  ::petsc::LocalToGlobalMap  _localVertexMap;
"""
      
  def get_constructor_body(self):
    """!
    
Define body of constructor

Consult the superclass' description of the function for results. I 
basically initialise the _localMap with the correct tree number.

@see get_attributes()
    
    """
    return """
  _localCellMap.setTreeNumber(   treeNumber );
  _localFaceMap.setTreeNumber(   treeNumber );
  _localVertexMap.setTreeNumber( treeNumber );
"""


  def get_includes(self):
    """!
   
Consult multigrid.petsc.Project for details
    
"""    
    return """
#include "repositories/SolverRepository.h"
"""
  
