from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2

class UpdateCell(ActionSet):
  """!
  This is for DG solver.
  """
  templateTCFT="""
  if ( fineGridCell{{SOLVER_NAME}}.getType() != celldata::{{SOLVER_NAME}}::Type::Coarse )
  {
    logTraceIn("UpdateCell");

    // write Mb into it, i.e. mass matrix * rhs
    tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.CellUnknowns, double > r =
      repositories::{{SOLVER_INSTANCE}}.getMassMatrix( marker.x(), marker.h() ) * fineGridCell{{SOLVER_NAME}}.getRhs();

    // This vector faceSol will contain: soln on face 0 | soln on face 1 | soln on face 2 | .....
    tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.FaceUnknownsSolution*TwoTimesD, double > faceSol;
    for (int f=0; f<TwoTimesD; f++)
    for (int s=0; s<repositories::{{SOLVER_INSTANCE}}.FaceUnknownsSolution; s++)
      faceSol( f*repositories::{{SOLVER_INSTANCE}}.FaceUnknownsSolution + s ) = fineGridFaces{{SOLVER_NAME}}(f).getSolution(s);
  

    // next, we need to project solution on face onto the cell...
    auto projection = repositories::{{SOLVER_INSTANCE}}.getCellFromFaceMatrix() * faceSol;
    // loop over each face
    for (int f=0; f<repositories::{{SOLVER_INSTANCE}}.CellUnknowns; f++)
    {
      // project the face solution onto the cell; add to r
      r += projection(f);
    }

    // fetch inverted assembly matrix from abstract solver class. left-multiply r.
    r = repositories::{{SOLVER_INSTANCE}}.getInvertedAssemblyMatrix() * r;

    // scale current entries in solution vector by (1-OmegaCell)
    for (int i=0; i<fineGridCell{{SOLVER_NAME}}.getSolution().size(); i++)
    {
      double sol = fineGridCell{{SOLVER_NAME}}.getSolution(i) * (1-repositories::{{SOLVER_INSTANCE}}.OmegaCell);

      // add on r values whilst we are here
      fineGridCell{{SOLVER_NAME}}.setSolution(i, sol + r(i) );
    }

    logTraceOut("UpdateCell");
  }
  """

  templateTFFT="""
  if ( fineGridFace{{SOLVER_NAME}}.getType() != facedata::{{SOLVER_NAME}}::Type::Coarse )
  {
    // project the u^\pm into a temporary vector
    tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.FaceUnknownsSolution, double > projection = 
      repositories::{{SOLVER_INSTANCE}}.getRiemannMatrix() * fineGridFace{{SOLVER_NAME}}.getProjection();

    for (int i=0; i<repositories::{{SOLVER_INSTANCE}}.FaceUnknownsSolution; i++)
      fineGridFace{{SOLVER_NAME}}.setSolution(
        i,
        (1-repositories::{{SOLVER_INSTANCE}}.OmegaFace)*fineGridFace{{SOLVER_NAME}}.getSolution(i)
         + repositories::{{SOLVER_INSTANCE}}.OmegaFace * projection(i)
      );
  }

"""
  def __init__(self,
               solver,
               descend_invocation_order=0,
               parallel=True):
    super( UpdateCell, self ).__init__(
      descend_invocation_order,
      parallel
    )
    self.d = {}
    self.d["SOLVER_INSTANCE"]    = solver.instance_name()
    self.d["SOLVER_NAME"]        = solver.typename()

  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME:
      result = jinja2.Template(self.templateTFFT).render(**self.d)
      pass 
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
