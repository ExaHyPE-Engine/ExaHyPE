# This file is part of the Peano's PETSc extension. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2
 

class EnumerateDoFs(ActionSet):
  """!

  Enumerate the vertices, cells and face entries
  
  This is the first step in a typical PETSc algorithm, as discussed on @ref documentation_multigrid_petsc.
  It has to ensure that each mesh entity hosts the appropriate number of 
  degrees of freedom. It hence strongly depends on the solver chosen, as 
  different solvers place different amounts of data and auxiliary quantities on
  the mesh entities. The information on data cardinality is queried from the 
  solver objects. The added logic contributed by this action set is if a mesh
  entity really holds unknowns or not. That is, this action set injects the 
  domain layout and boundary data knowledge into the mesh. This guiding 
  principles of this injection are discussed on 
  @ref documentation_multigrid_boundary_conditions "Peano's generic boundary condition pages".
  
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
  fineGridVertex{{SOLVER_NAME}}PETScData.setType( vertexdata::{{SOLVER_NAME}}PETScData::Type::Undefined );
  
  vertexdata::{{SOLVER_NAME}}PETScData::Type dofType = repositories::{{SOLVER_INSTANCE}}.getVertexDoFType(marker.x(),marker.h());
  
  switch (dofType) {
    case vertexdata::{{SOLVER_NAME}}PETScData::Type::Boundary:
      {
        fineGridVertex{{SOLVER_NAME}}PETScData.setType( vertexdata::{{SOLVER_NAME}}PETScData::Type::Boundary );
        {% if BOUNDARY_VERTICES_HOLD_DATA %}
        if ( marker.willBeRefined() ) { 
          fineGridVertex{{SOLVER_NAME}}PETScData.setType( vertexdata::{{SOLVER_NAME}}PETScData::Type::Coarse );
        }
        else {
          fineGridVertex{{SOLVER_NAME}}PETScData.setUnknownBaseNumber( _localVertexMap.reserveIndex( {{VERTEX_CARDINALITY}} ) );
        }
        {% endif %}
      }
      break;
    case vertexdata::{{SOLVER_NAME}}PETScData::Type::Interior:
      {
        if (isOnBoundary(marker.x())) {
          logWarning( "touchVertexFirstTime(...)", "vertex at " << marker.toString() << " labelled as interior even though it is located at global domain boundary" );
        }
        if ( marker.willBeRefined() ) { 
          fineGridVertex{{SOLVER_NAME}}PETScData.setType( vertexdata::{{SOLVER_NAME}}PETScData::Type::Coarse );
        }
        else {
          fineGridVertex{{SOLVER_NAME}}PETScData.setType( vertexdata::{{SOLVER_NAME}}PETScData::Type::Interior );
          fineGridVertex{{SOLVER_NAME}}PETScData.setUnknownBaseNumber( _localVertexMap.reserveIndex( {{VERTEX_CARDINALITY}} ) );
        }
      }
      break;
    case vertexdata::{{SOLVER_NAME}}PETScData::Type::Coarse:
      assertionMsg(false, "should not be returned by user" );
      break;
    case vertexdata::{{SOLVER_NAME}}PETScData::Type::Outside:
      {
        fineGridVertex{{SOLVER_NAME}}PETScData.setType( vertexdata::{{SOLVER_NAME}}PETScData::Type::Outside );
      }
      break;
    case vertexdata::{{SOLVER_NAME}}PETScData::Type::Undefined:
      assertionEquals( {{VERTEX_CARDINALITY}}, 0 );
      break;
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
  fineGridFace{{SOLVER_NAME}}PETScData.setType( facedata::{{SOLVER_NAME}}PETScData::Type::Undefined );
  
  facedata::{{SOLVER_NAME}}PETScData::Type dofType = repositories::{{SOLVER_INSTANCE}}.getFaceDoFType(marker.x(),marker.h());
  
  switch (dofType) {
    case facedata::{{SOLVER_NAME}}PETScData::Type::Boundary:
      {
        fineGridFace{{SOLVER_NAME}}PETScData.setType( facedata::{{SOLVER_NAME}}PETScData::Type::Boundary );
        {% if BOUNDARY_FACES_HOLD_DATA %}
        if ( marker.willBeRefined() ) { 
          fineGridFace{{SOLVER_NAME}}PETScData.setType( facedata::{{SOLVER_NAME}}PETScData::Type::Coarse );
        }
        else {
          fineGridFace{{SOLVER_NAME}}PETScData.setUnknownBaseNumber( _localFaceMap.reserveIndex( {{FACE_CARDINALITY}} ) );
        }
        {% endif %}
      }
      break;
    case facedata::{{SOLVER_NAME}}PETScData::Type::Interior:
      {
        if (isOnBoundary(marker.x())) {
          logWarning( "touchFaceFirstTime(...)", "face at " << marker.toString() << " labelled as interior even though it is located at global domain boundary" );
        }
        if ( marker.willBeRefined() ) { 
          fineGridFace{{SOLVER_NAME}}PETScData.setType( facedata::{{SOLVER_NAME}}PETScData::Type::Coarse );
        }
        else {
          fineGridFace{{SOLVER_NAME}}PETScData.setType( facedata::{{SOLVER_NAME}}PETScData::Type::Interior );
          fineGridFace{{SOLVER_NAME}}PETScData.setUnknownBaseNumber( _localFaceMap.reserveIndex( {{FACE_CARDINALITY}} ) );
        }
      }
      break;
    case facedata::{{SOLVER_NAME}}PETScData::Type::Coarse:
      assertionMsg(false, "should not be returned by user" );
      break;
    case facedata::{{SOLVER_NAME}}PETScData::Type::Outside:
      {
        fineGridFace{{SOLVER_NAME}}PETScData.setType( facedata::{{SOLVER_NAME}}PETScData::Type::Outside );
      }
      break;
    case facedata::{{SOLVER_NAME}}PETScData::Type::Undefined:
      assertionEquals( {{FACE_CARDINALITY}}, 0 );
      break;
  }
"""

  
  TemplateEnumerateCell = """
  fineGridCell{{SOLVER_NAME}}PETScData.setUnknownBaseNumber( ::petsc::LocalToGlobalMap::NoDegreeOfFreedom );
  fineGridCell{{SOLVER_NAME}}PETScData.setType( celldata::{{SOLVER_NAME}}PETScData::Type::Undefined );
  
  celldata::{{SOLVER_NAME}}PETScData::Type dofType = repositories::{{SOLVER_INSTANCE}}.getCellDoFType(marker.x(),marker.h());
  
  switch (dofType) {
    case celldata::{{SOLVER_NAME}}PETScData::Type::Interior:
      {
        if ( marker.willBeRefined()  ) { 
          fineGridCell{{SOLVER_NAME}}PETScData.setType( celldata::{{SOLVER_NAME}}PETScData::Type::Coarse );
        }
        else {
          fineGridCell{{SOLVER_NAME}}PETScData.setType( celldata::{{SOLVER_NAME}}PETScData::Type::Interior );
          fineGridCell{{SOLVER_NAME}}PETScData.setUnknownBaseNumber( _localCellMap.reserveIndex( {{CELL_CARDINALITY}} ) );
        }
      }
      break;
    case celldata::{{SOLVER_NAME}}PETScData::Type::Coarse:
      assertionMsg(false, "should not be returned by user" );
      break;
    case celldata::{{SOLVER_NAME}}PETScData::Type::Outside:
      {
        fineGridCell{{SOLVER_NAME}}PETScData.setType( celldata::{{SOLVER_NAME}}PETScData::Type::Outside );
      }
      break;
    case celldata::{{SOLVER_NAME}}PETScData::Type::Undefined:
      assertionEquals( {{CELL_CARDINALITY}}, 0 );
      break;
  }
"""

  
  TemplateEndTraversal = """
  logInfo( 
    "endTraversal()", 
    "vertex dofs=" << _localVertexMap.getTotalNumberOfIndices() <<
    ", face dofs=" << _localFaceMap.getTotalNumberOfIndices() <<
    ", cell dofs=" << _localCellMap.getTotalNumberOfIndices()
  );
    
  repositories::{{SOLVER_INSTANCE}}.getLocalToGlobalMap().merge( _localVertexMap, ::petsc::LocalToGlobalMap::Type::Vertex );
  repositories::{{SOLVER_INSTANCE}}.getLocalToGlobalMap().merge( _localFaceMap,   ::petsc::LocalToGlobalMap::Type::Face );
  repositories::{{SOLVER_INSTANCE}}.getLocalToGlobalMap().merge( _localCellMap,   ::petsc::LocalToGlobalMap::Type::Cell );
"""


  def __init__(self,
               solver,
               boundary_vertices_hold_data,
               boundary_faces_hold_data,
               ):
    """!A
    
Enumerate vertex-associated degrees of freedom
    
The initialisation requires a solver object, as we have to know what C++
object this solver will produce, and we need to know how many unknowns
we have per vertex.

solver: petsc.solvers.CollocatedLowOrderDiscretisation or similar solver where
  degrees of freedom are assigned exclusively to the vertices.
    
    """
    super( EnumerateDoFs, self ).__init__()
    self.d = {}
    self.d["SOLVER_INSTANCE"]        = solver.instance_name()
    self.d["SOLVER_NAME"]            = solver.typename()
    self.d["VERTEX_CARDINALITY"] = solver.number_of_matrix_entries_per_vertex
    self.d["FACE_CARDINALITY"]   = solver.number_of_matrix_entries_per_face
    self.d["CELL_CARDINALITY"]   = solver.number_of_matrix_entries_per_cell
    self.d["BOUNDARY_VERTICES_HOLD_DATA"] = boundary_vertices_hold_data
    self.d["BOUNDARY_FACES_HOLD_DATA"]    = boundary_faces_hold_data


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
  
