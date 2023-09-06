# This file is part of the Peano's PETSc extension. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2


class EnumerateVertices(ActionSet):
  """!

Enumerate the vertices    
  
  """
  
  
  TemplateEnumerateVertex = """
  if ( 
    not marker.willBeRefined() 
    and
    fineGridVertex{{SOLVER_NAME}}.getType() != vertexdata::{{SOLVER_NAME}}::Type::Dirichlet
  ) { 
    logTraceIn( "touchCellFirstTime(...)---InitialCondition" );

    fineGridVertex{{SOLVER_NAME}}.setNumber( _localMap.reserveIndex( {{VERTEX_CARDINALITY}} ) );
    
    logTraceOut( "touchCellFirstTime(...)---InitialCondition" );
  } 
"""

  TemplateEndTraversal = """
  repositories::{{SOLVER_INSTANCE}}.getLocalToGlobalMap().merge( _localMap );
"""
  
  
  def __init__(self,
               solver):
    """!
    
Enumerate vertex-associated degrees of freedom
    
The initialisation requires a solver object, as we have to know what C++
object this solver will produce, and we need to know how many unknowns
we have per vertex.

solver: petsc.solvers.CollocatedLowOrderDiscretisation or similar solver where
  degrees of freedom are assigned exclusively to the vertices.
    
    """
    super( EnumerateVertices, self ).__init__()
    self.d = {}
    self.d["SOLVER_INSTANCE"]        = solver.instance_name()
    self.d["SOLVER_NAME"]            = solver.typename()
    self.d["VERTEX_CARDINALITY"]     = solver.number_of_vertex_unknowns


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
  ::petsc::LocalToGlobalMap  _localMap;
"""
      
  def get_constructor_body(self):
    """!
    
Define body of constructor

Consult the superclass' description of the function for results. I 
basically initialise the _localMap with the correct tree number.

@see get_attributes()
    
    """
    return """
  _localMap.setTreeNumber( treeNumber );
"""


  def get_includes(self):
    """!
   
Consult petsc.Project for details
    
"""    
    return """
#include "repositories/SolverRepository.h"
"""









class EnumerateVerticesGeneral(ActionSet):
  """!

Enumerate the vertices

Same as above, but we refer to it as  getVertexLocalToGlobalMap etc
  
  """
  
  
  TemplateEnumerateVertex = """
  if ( 
    not marker.willBeRefined() 
  ) { 
    logTraceIn( "touchCellFirstTime(...)---InitialCondition" );

    auto isCorner = [&]( const tarch::la::Vector< Dimensions, double > & vertex ) -> bool{
      bool xIsOnBoundary = ( vertex[0] < 1E-10 or  vertex[0] > 1 - 1E-10);
      bool yIsOnBoundary = ( vertex[1] < 1E-10 or  vertex[1] > 1 - 1E-10);
      return (xIsOnBoundary and yIsOnBoundary);
    };

    if (fineGridVertex{{SOLVER_NAME}}.getType() == vertexdata::{{SOLVER_NAME}}::Type::DoF)
      fineGridVertex{{SOLVER_NAME}}.setNumber( _localVertexMap.reserveIndex( TwoPowerD - 1 ) ); //we just want to count the number of vertices
    
    else if(fineGridVertex{{SOLVER_NAME}}.getType() == vertexdata::{{SOLVER_NAME}}::Type::Dirichlet)
    {
      if ( isCorner(marker.x()) )
      {
        fineGridVertex{{SOLVER_NAME}}.setNumber( _localVertexMap.reserveIndex( 1 ) );

      }
      else
      {
        fineGridVertex{{SOLVER_NAME}}.setNumber( _localVertexMap.reserveIndex( TwoPowerD / 2 ) );
      }

    }

    logTraceOut( "touchCellFirstTime(...)---InitialCondition" );
  } 
"""

  TemplateEndTraversal = """
  repositories::{{SOLVER_INSTANCE}}.getVertexLocalToGlobalMap().merge( _localVertexMap );
"""
  
  
  def __init__(self,
               solver):
    """!
    
Enumerate vertex-associated degrees of freedom
    
The initialisation requires a solver object, as we have to know what C++
object this solver will produce, and we need to know how many unknowns
we have per vertex.

solver: petsc.solvers.CollocatedLowOrderDiscretisation or similar solver where
  degrees of freedom are assigned exclusively to the vertices.
    
    """
    super( EnumerateVerticesGeneral, self ).__init__()
    self.d = {}
    self.d["SOLVER_INSTANCE"]        = solver.instance_name()
    self.d["SOLVER_NAME"]            = solver.typename()
    self.d["VERTEX_CARDINALITY"]     = solver.number_of_vertex_unknowns


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
  _localVertexMap.setTreeNumber( treeNumber );
"""


  def get_includes(self):
    """!
   
Consult petsc.Project for details
    
"""    
    return """
#include "repositories/SolverRepository.h"
"""

class EnumerateCellsAndFaces(ActionSet):
  """!

  
  """
  
  TemplateEnumerateCell = """
  if ( not marker.willBeRefined() ){
    logTraceIn( "touchCellFirstTime(...)---InitialCondition" );

    fineGridCell{{SOLVER_NAME}}.setNumber( _localCellMap.reserveIndex( {{CELL_CARDINALITY}} ) );

    logTraceOut( "touchCellFirstTime(...)---InitialCondition" );
  }

"""

  TemplateEnumerateFace = """
  if ( not marker.willBeRefined() ){
    logTraceIn( "touchFaceFirstTime(...)---InitialCondition" );

    fineGridFace{{SOLVER_NAME}}.setNumber( _localFaceMap.reserveIndex( {{FACE_CARDINALITY}} ) );

    logTraceOut( "touchFaceFirstTime(...)---InitialCondition" );
  }

"""

  TemplateEndTraversal = """
  repositories::{{SOLVER_INSTANCE}}.getCellLocalToGlobalMap().merge( _localCellMap );
  repositories::{{SOLVER_INSTANCE}}.getFaceLocalToGlobalMap().merge( _localFaceMap );
"""
  
  
  def __init__(self,
               solver):
    """!
    
Enumerate vertex-associated degrees of freedom
    
The initialisation requires a solver object, as we have to know what C++
object this solver will produce, and we need to know how many unknowns
we have per vertex.

solver: petsc.solvers.CollocatedLowOrderDiscretisation or similar solver where
  degrees of freedom are assigned exclusively to the vertices.
    
    """
    super( EnumerateCellsAndFaces, self ).__init__()
    self.d = {}
    self.d["SOLVER_INSTANCE"]        = solver.instance_name()
    self.d["SOLVER_NAME"]            = solver.typename()
    self.d["CELL_CARDINALITY"]       = solver.number_of_cell_unknowns
    self.d["FACE_CARDINALITY"]       = solver.number_of_face_unknowns


  def get_body_of_operation(self,operation_name):
    """!

Provide C++ code snippet for peano4.solversteps.ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME  
  
Only touchVertexFirstTime is an event where this action set actually
does something: It inserts the template TemplateInitVertex and 
replaces it with entries from the dictionary. The latter is befilled
in init().
    
    """
    result = ""
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
"""
      
  def get_constructor_body(self):
    """!
    
Define body of constructor

Consult the superclass' description of the function for results. I 
basically initialise the _localMap with the correct tree number.

@see get_attributes()
    
    """
    return """
  _localCellMap.setTreeNumber( treeNumber );
  _localFaceMap.setTreeNumber( treeNumber );
"""


  def get_includes(self):
    """!
   
Consult petsc.Project for details
    
"""    
    return """
#include "repositories/SolverRepository.h"
"""


