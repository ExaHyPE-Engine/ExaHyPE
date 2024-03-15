# This file is part of the Peano multigrid project. For conditions of 
# distribution and use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2



class ImposeDirichletBoundaryConditionsWithInteriorPenaltyMethod(ActionSet):
  """!

  @todo Add comments which document from Slack
    
  """
  TemplateTouchFaceFirstTime = """
  if (fineGridFace{{SOLVER_NAME}}PETScData.getType() == facedata::{{SOLVER_NAME}}PETScData::Type::Boundary) {
    std::pair<int,int> localFaceIndex = std::make_pair(_spacetreeId, fineGridFace{{SOLVER_NAME}}PETScData.getUnknownBaseNumber());
    int globalFaceIndex               = repositories::{{SOLVER_INSTANCE}}.getLocalToGlobalMap().getGlobalIndex(localFaceIndex, ::petsc::LocalToGlobalMap::Type::Face);
    
    assertion( globalFaceIndex>=0 );

    logTraceInWith2Arguments("touchFaceFirstTime::Boundary", globalFaceIndex, marker.toString());
    
    /*
    we want to place -1 on the diagonal for each "interior-side" projection.
    That is, for face 0 and 2, we write to the "+" projection
             for face 1 and 3, we write to the "-" projection

    We also write -1 to all of the "exterior-side" projections. The actual
    value we use here is immaterial, so long as it is not zero.
    This makes the matrix full-rank. We use -1 to make this loop easier
    to write.

    we go up to 2*repositories::{{SOLVER_INSTANCE}}.NodesPerFace*repositories::{{SOLVER_INSTANCE}}.FaceUnknowns
    since this is total number of q^-, q^+, u^-, u^+.
    */
    for (int i=0; i<2*repositories::{{SOLVER_INSTANCE}}.NodesPerFace*repositories::{{SOLVER_INSTANCE}}.FaceUnknowns; i++)
    {
      repositories::{{SOLVER_INSTANCE}}.getLinearEquationSystem().insert(
        globalFaceIndex + i,
        globalFaceIndex + i,
        -1
      );
    }

    int faceNormalAxis = marker.getSelectedFaceNumber()%Dimensions;
    int row = 0;        
    dfore(faceNode, repositories::{{SOLVER_INSTANCE}}.Order+1, faceNormalAxis, 0) {

      tarch::la::Vector<Dimensions,double> x = marker.x(); // centre of face
      for (int dd=0; dd<Dimensions; dd++) {
        if (dd!=faceNormalAxis) {
          x(dd) -= 0.5 * marker.h()(dd);
          x(dd) += repositories::{{SOLVER_INSTANCE}}.QuadraturePointsInUnitInterval[ faceNode(dd) ] * marker.h()(dd);
        }
      }

      logTraceInWith5Arguments("touchFaceFirstTime::dfore", faceNode, faceNormalAxis, marker.x(), x,repositories::{{SOLVER_INSTANCE}}.FaceUnknowns);
          
      // Look up the value: rhs is not relevant here, but I want to reuse the existing signatures 
      double value, rhs, exact;
      repositories::{{SOLVER_INSTANCE}}.initNode(
        x,
        marker.h(),
        value,
        rhs,
        exact
      );
      
      for (int unknown = 0; unknown<repositories::{{SOLVER_INSTANCE}}.FaceUnknowns; unknown++) {
        // row corresponding to solution
        int globalMatrixRow = globalFaceIndex + row + 2*repositories::{{SOLVER_INSTANCE}}.NodesPerFace*repositories::{{SOLVER_INSTANCE}}.FaceUnknowns;
        
        logTraceInWith4Arguments("touchFaceFirstTime:unknownLoop", unknown, globalMatrixRow, row, marker.toString());

        // insert identity into row corresponding to solution at this node / for this unknown
        repositories::{{SOLVER_INSTANCE}}.getLinearEquationSystem().insert(
          globalMatrixRow,
          globalMatrixRow,
          1
        );
        
        static_assert(Dimensions==2, "the bodge on the line below is untested for Dimensions != 2. see comment in code");

        /*
        The faces are enumerated as follows:
          negative x axis
          negative y axis
          negative z axis
          positive x axis
          positive y axis
          positive z axis
        if we are on negative side of coordinate axis, then we wanna write to the positive projection, 
        since this is the one that is on interior side of cell.

        So, if the face number is >= than Dimensions, we wanna write to negative side projection.
        */
        bool writeToNegativeProjection = (marker.getSelectedFaceNumber() >= Dimensions);

        /*
        This part is to round off the equation. If q^- projection is on the inside of the cell,
        then we want the equation to read q^f - q^- = 0. Hence we write 1 onto the row, col 
        corresponding to q^f and then -1 in col for q^-. rhs will be zero to round this off.
        */

        // makes sure we only do this for q unknown
        if ( row < repositories::{{SOLVER_INSTANCE}}.FaceUnknowns )
        {
          repositories::{{SOLVER_INSTANCE}}.getLinearEquationSystem().insert(
            globalMatrixRow,
            globalFaceIndex + row, // q^-
            writeToNegativeProjection ? -1 : 0
          );

          repositories::{{SOLVER_INSTANCE}}.getLinearEquationSystem().insert(
            globalMatrixRow,
            globalFaceIndex + row + repositories::{{SOLVER_INSTANCE}}.NodesPerFace*repositories::{{SOLVER_INSTANCE}}.FaceUnknowns, // q^+
            writeToNegativeProjection ? 0 : -1
          );
        }

        /*
        Alternative behaviour for u unknown. Same as for q: If u^- projection is on the inside of the cell,
        then we want the equation to read u^f - u^- = 0
        */
        if ( row >= repositories::{{SOLVER_INSTANCE}}.FaceUnknowns  )
        {
          repositories::{{SOLVER_INSTANCE}}.getLinearEquationSystem().insert(
            globalMatrixRow,
            globalFaceIndex + row, // u^-
            writeToNegativeProjection ? -1 : 0
          );

          repositories::{{SOLVER_INSTANCE}}.getLinearEquationSystem().insert(
            globalMatrixRow,
            globalFaceIndex + row + repositories::{{SOLVER_INSTANCE}}.NodesPerFace*repositories::{{SOLVER_INSTANCE}}.FaceUnknowns, // u^+
            writeToNegativeProjection ? 0 : -1
          );
        }

        row++;

        logTraceOut("touchFaceFirstTime:unknownLoop");
      }
      logTraceOut("touchFaceFirstTime::dfore");

    }
    logTraceOut("touchFaceFirstTime::Boundary");
  }
  """
  
  def __init__(self,
               solver,
               ):
    """!
    
    Yet to be written
        
    """
    
    super( ImposeDirichletBoundaryConditionsWithInteriorPenaltyMethod, self ).__init__()
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
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME:
      result = jinja2.Template(self.TemplateTouchFaceFirstTime).render(**self.d)
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
