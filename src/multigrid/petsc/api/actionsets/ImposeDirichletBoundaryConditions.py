# This file is part of the Peano multigrid project. For conditions of 
# distribution and use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2



class ImposeDirichletBoundaryConditions(ActionSet):
  """!

  @todo This version is not yet implemented, and maybe we don't need it ever!
  
  
  Add those contributions to matrix which impose the boundary conditions
  
  Dirichlet (and really all boundary) faces do not hold any artificial or real
  degrees of freedom in our code. So the core assembly ignores these faces and 
  we end up with a linear equation system which is not coupled to the PDE's
  boundary conditions. This additional action sets runs over the mesh and adds
  the missing coupling.
  
  For every cell that we visit (and that is not outside the computational 
  domain), we check its 2d faces, i.e. the left and right face along each
  axis. If this face is an interior, nothing is to be done. We only have to do
  something for boundary faces. In this action set, we assume that each face 
  is a Dirichlet face. 
  
  If a face is a Dirichlet face, we loop over the nodes on the face. For 
  this, we emply Peano's exclusive d-dimensional for (dfore), which is a 
  d-dimensional for loop skipping one dimension. Per dof, we compute the 
  dof's coordinate x and ask the user's solver what the value there should be.
  This gives us the right-hand side for the equation system row that 
  imposes the condition.
  
  Next, we use compute the projection of the solution to the boundary. This
  might be a different projection than used for the faces, even though the
  code is basically the same. For some solvers, we project solely the 
  derivatives along the normal onto the face, for others, we 
  
  
  @todo We need the projection matrix for the solution!
  @todo We have to add the corresponding matrix entries below, as well as
    the right-hand side

  """
  TemplateTouchCellFirstTime = """
  if (fineGridCell{{SOLVER_NAME}}PETScData.getType() == celldata::{{SOLVER_NAME}}PETScData::Type::Interior) {
    std::pair<int,int> localCellIndex = std::make_pair(_spacetreeId, fineGridCell{{SOLVER_NAME}}PETScData.getUnknownBaseNumber());
    int globalCellIndex = repositories::{{SOLVER_INSTANCE}}.getLocalToGlobalMap().getGlobalIndex(localCellIndex, ::petsc::LocalToGlobalMap::Type::Cell);
    
    assertion( globalCellIndex>=0 );
    
    for (int d=0; d<Dimensions; d++) {
      // =================================
      // Look at left face along axis d
      // =================================
      if ( fineGridFaces{{SOLVER_NAME}}PETScData(d).getType()==facedata::{{SOLVER_NAME}}PETScData::Type::Boundary ) {
        assertion( repositories::{{SOLVER_INSTANCE}}.CellUnknowns==1 );      

        int row = 0;
        
        // Loop over all projections of the solution onto the face. See docu on
        // dfore in Peano's utilities
        dfore(faceNode, repositories::{{SOLVER_INSTANCE}}.Order+1, d, 0) {
          tarch::la::Vector<Dimensions,double> x = marker.getOffset(); // left bottom vertex of cell
          for (int dd=0; dd<Dimensions; dd++) {
            if (dd!=d) {
              x(dd) += repositories::{{SOLVER_INSTANCE}}.QuadraturePointsInUnitInterval[ faceNode(dd) ] * marker.h()(dd);
            }
          }
          
          // Look up the value: rhs is not relevant here, but I want to reuse the existing signatures 
          double value, rhs, exact;
          repositories::{{SOLVER_INSTANCE}}.initNode(
            x,
            marker.h(),
            value,
            rhs,
            exact
          );
          
          // @todo We have to discuss this
          int rhsRow = 0;
          repositories::{{SOLVER_INSTANCE}}.getLinearEquationSystem().insert( rhsRow, value );
          for (int col=0; col<repositories::{{SOLVER_INSTANCE}}.DoFsPerCell; col++) {
            //repositories::{{SOLVER_INSTANCE}}.getLinearEquationSystem().insert(
            //  rhsRow, 
            //  globalCellIndex+col,
            //  projectionCellToFaceMatrix(d * dofsPerFace + row,col)
            //);
          }
          
          row++;
        }
      }
      
      // =================================
      // Look at right face along axis d
      // =================================
      if ( fineGridFaces{{SOLVER_NAME}}PETScData(d+Dimensions).getType()==facedata::{{SOLVER_NAME}}PETScData::Type::Boundary ) {
       // @todo Later
      } 
    }
  }
  """
  
  def __init__(self,
               solver,
               ):
    """!
    
    Yet to be written
        
    """
    
    super( ImposeDirichletBoundaryConditions, self ).__init__()
    self.d = {}
    self.d["SOLVER_INSTANCE"]        = solver.instance_name()
    self.d["SOLVER_NAME"]            = solver.typename()


  def get_body_of_operation(self,operation_name):
    """!

Provide C++ code snippet for peano4.solversteps.ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME  
Provide C++ code snippet for peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME  
  
Only touchVertexFirstTime is an event where this action set actually
does something: It inserts the template TemplateTouchVertexFirstTime and 
replaces it with entries from the dictionary. The latter is befilled
in init().

We actually do something during touchVertexFirstTime and touchCellFirstTime. We insert the 
appropriate template into each.
    
    """
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = jinja2.Template(self.TemplateTouchCellFirstTime).render(**self.d)
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
#include "tarch/la/Matrix.h"
#include "peano4/utils/Loop.h"
"""

  def get_attributes(self):
    """!
    
    
    """
    return f"""
  int _spacetreeId;    
"""
      
  def get_constructor_body(self):
    """!
    
Define body of constructor

@see get_attributes()
    
    """
    return f"""
    _spacetreeId = treeNumber;
"""
