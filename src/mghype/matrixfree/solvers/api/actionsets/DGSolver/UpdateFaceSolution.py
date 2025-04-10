from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2

class UpdateFaceSolution(ActionSet):
  """!
  This is for DG solver.
  
  @todo Docu missing
  
  """


  templatetouchFaceFirstTime="""
  if ( 
    fineGridFace{{SOLVER_NAME}}.getType() == facedata::{{SOLVER_NAME}}::Type::Interior 
    and
    repositories::{{SOLVER_INSTANCE}}.updateFace()
  ) {
    logTraceInWith2Arguments( "updateFaceSolution::Interior", fineGridFace{{SOLVER_NAME}}.getSolution(), marker.toString() );

    // project the u^\pm into a temporary vector
    tarch::la::Vector< 2*repositories::{{SOLVER_INSTANCE}}.FaceUnknownsSolution, double > projections;
    // alias for number of solutions
    const int nSols = repositories::{{SOLVER_INSTANCE}}.FaceUnknownsSolution;
    for (int i=0; i<repositories::{{SOLVER_INSTANCE}}.FaceUnknownsSolution; i++) {
      projections[i]       = fineGridFace{{SOLVER_NAME}}.getLProjection(i);
      projections[i+nSols] = fineGridFace{{SOLVER_NAME}}.getRProjection(i);
    }

    // multiply by riemann matrix
    auto faceSolutionUpdate = repositories::{{SOLVER_INSTANCE}}.applyRiemannMatrix( projections );

    for (int i=0; i<repositories::{{SOLVER_INSTANCE}}.FaceUnknownsSolution; i++)
      fineGridFace{{SOLVER_NAME}}.setSolution(
        i,
        (1-repositories::{{SOLVER_INSTANCE}}.OmegaFace)*fineGridFace{{SOLVER_NAME}}.getSolution(i)
         + repositories::{{SOLVER_INSTANCE}}.OmegaFace * faceSolutionUpdate(i)
      );

    fineGridFace{{SOLVER_NAME}}.setLProjection( 0.0 );
    fineGridFace{{SOLVER_NAME}}.setRProjection( 0.0 );
    logTraceOutWith1Argument( "updateFaceSolution::Interior", fineGridFace{{SOLVER_NAME}}.getSolution() );
  }

  else if (
    fineGridFace{{SOLVER_NAME}}.getType() == facedata::{{SOLVER_NAME}}::Type::Boundary
    and
    repositories::{{SOLVER_INSTANCE}}.updateFace()
  ) {
    logTraceInWith2Arguments( "updateFaceSolution::InteriorPenalty", fineGridFace{{SOLVER_NAME}}.getLProjection(), fineGridFace{{SOLVER_NAME}}.getRProjection() );
    // Essentially what we do here is fix the solution on the boundary to be 
    // the inner-side projection.

    // skip halfway along the projection vector if we are on face 0 or 1
    int startIndexProjection = 
      marker.getSelectedFaceNumber() < Dimensions ?
      repositories::{{SOLVER_INSTANCE}}.NodesPerFace * repositories::{{SOLVER_INSTANCE}}.ProjectionsPerFaceNode :
      0;

    if ( marker.getSelectedFaceNumber() < Dimensions )
      fineGridFace{{SOLVER_NAME}}.setSolution(
        repositories::{{SOLVER_INSTANCE}}.applyBoundaryConditionMatrix(
          fineGridFace{{SOLVER_NAME}}.getRProjection()
        )
      );

    else
      fineGridFace{{SOLVER_NAME}}.setSolution(
        repositories::{{SOLVER_INSTANCE}}.applyBoundaryConditionMatrix(
          fineGridFace{{SOLVER_NAME}}.getLProjection()

        ));

    logTraceOut( "updateFaceSolution::InteriorPenalty");
  }


"""


  def __init__(self,
               solver,
               descend_invocation_order=0,
               parallel=True):
    super( UpdateFaceSolution, self ).__init__(
      descend_invocation_order,
      parallel
    )
    self.d = {}
    self.d["SOLVER_INSTANCE"]    = solver.instance_name()
    self.d["SOLVER_NAME"]        = solver.typename()

  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME:
      result = jinja2.Template(self.templatetouchFaceFirstTime).render(**self.d)
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
    return __name__.replace(".py", "").replace(".", "_") + "_UpdateFaceSolution"
    
  def user_should_modify_template(self):
    """!
    
    The action set that Peano will generate that corresponds to this class
    should not be modified by users and can safely be overwritten every time
    we run the Python toolkit.
    
    """
    return False

  def get_includes(self):
    """!
   
    We need the solver repository in this action set, as we directly access
    the solver object. We also need access to Peano's d-dimensional loops.
         
    """    
    return """
#include "../repositories/SolverRepository.h"
#include "peano4/utils/Loop.h"
"""

