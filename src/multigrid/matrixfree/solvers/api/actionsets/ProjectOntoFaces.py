from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2

class ProjectOntoFaces(ActionSet):
  """!
  This is for DG solver.
  """

  templateTCFT="""
  /*
  all we need to do here is project updated solution u^c onto the faces
  */

  // what to do about boundary faces?
  if ( fineGridCell{{SOLVER_NAME}}.getType() != celldata::{{SOLVER_NAME}}::Type::Coarse )
  {
    // apply cell to face projection, store it in faceProjections
    // no tricky indexing needed here, since the whole of the cell
    // solution vector is in scope here.
    tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.FaceUnknownsProjection * TwoTimesD, double > faceProjections = 
      repositories::{{SOLVER_INSTANCE}}. getFaceFromCellMatrix(marker.x(), marker.h()) * fineGridCell{{SOLVER_NAME}}.getSolution();

    // write these values into the faces themselves
    for (int f=0; f<TwoTimesD; f++)
    for (int p=0; p<repositories::{{SOLVER_INSTANCE}}.FaceUnknownsProjection; p++)
      fineGridFaces{{SOLVER_NAME}}(f).setProjection(
        p,
        faceProjections( f*repositories::{{SOLVER_INSTANCE}}.FaceUnknownsProjection + p )
      );

  }
"""

  templateTFFT="""
  /*
  All we do here is set the value of the solution to be the same as 
  the interior-side projection
  */
  if ( fineGridFace{{SOLVER_NAME}}.getType() == facedata::{{SOLVER_NAME}}::Type::Boundary )
  {
    // project the u^\pm into a temporary vector
    tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.FaceUnknownsSolution, double > projection = 
      repositories::{{SOLVER_INSTANCE}}.getBoundaryConditionMatrix() * fineGridFace{{SOLVER_NAME}}.getProjection();

    for (int i=0; i<repositories::{{SOLVER_INSTANCE}}.FaceUnknownsSolution; i++)
      fineGridFace{{SOLVER_NAME}}.setSolution(
        i, projection(i)
      );
  }
"""

  def __init__(self,
               solver,
               descend_invocation_order=0,
               parallel=True):
    super( ProjectOntoFaces, self ).__init__(
      descend_invocation_order,
      parallel
    )
    self.d = {}
    self.d["SOLVER_INSTANCE"]    = solver.instance_name()
    self.d["SOLVER_NAME"]        = solver.typename()

  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = jinja2.Template(self.templateTCFT).render(**self.d)
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
   
    We need the solver repository in this action set, as we directly access
    the solver object. We also need access to Peano's d-dimensional loops.
         
    """    
    return """
#include "repositories/SolverRepository.h"
#include "peano4/utils/Loop.h"
"""
