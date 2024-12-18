from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2

from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2


class ProjectIntoCellAndUpdateCellSolution(ActionSet):
  """!
  This class is an identical copy of the other one written for the pure DG.
  
  But we change how the data is accessed, since we use patches here instead of standard
  DastGen arrays in the cell/faces for everything

  """
  templateTouchCellFirstTime="""
  if ( 
    fineGridCell{{SOLVER_NAME}}.getType() != celldata::{{SOLVER_NAME}}::Type::Coarse 
    and
    repositories::{{SOLVER_INSTANCE}}.projectOntoCells()
  ) {
    logTraceInWith2Arguments( "ProjectOntoCells", fineGridCell{{SOLVER_NAME}}.getResidual(), fineGridCell{{SOLVER_NAME}}.getSolution() );

    // we intend that the internal residual vector should be reset and updated in the 
    // cell update procedure

    // This vector faceSol will contain: soln on face 0 | soln on face 1 | soln on face 2 | .....
    tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.FaceUnknownsSolution*TwoTimesD, double > faceSol;
    for (int f=0; f<TwoTimesD; f++)
    for (int s=0; s<repositories::{{SOLVER_INSTANCE}}.FaceUnknownsSolution; s++)
      faceSol( f*repositories::{{SOLVER_INSTANCE}}.FaceUnknownsSolution + s ) = fineGridFaces{{SOLVER_NAME}}Solution(f).get(s);

    // next, we need to project solution on face onto the cell...
    auto projection = repositories::{{SOLVER_INSTANCE}}.getCellFromFaceMatrix(marker.x(), marker.h()) * faceSol;
    // loop over each face, finish updating residual as we go
    for (int f=0; f<repositories::{{SOLVER_INSTANCE}}.CellUnknowns; f++)
    {
      // project the face solution onto the cell; add to r
      fineGridCell{{SOLVER_NAME}}Residual.set(f,
        fineGridCell{{SOLVER_NAME}}Residual.get(f)- projection(f));
    }

    // multiply by "smoother" / preconditioner
    fineGridCell{{SOLVER_NAME}}Residual.set(
      repositories::{{SOLVER_INSTANCE}}.getInvertedApproxSystemMatrix(marker.x(),marker.h()) 
      * fineGridCell{{SOLVER_NAME}}Residual.get()
    );

    for (int i=0; i<fineGridCell{{SOLVER_NAME}}Solution.Cardinality; i++) {
      double res = fineGridCell{{SOLVER_NAME}}Residual.get( i );
      double du  = repositories::{{SOLVER_INSTANCE}}.OmegaCell * res;
      fineGridCell{{SOLVER_NAME}}Solution.set( i,
        fineGridCell{{SOLVER_NAME}}Solution.get( i ) + du
      );
      repositories::{{SOLVER_INSTANCE}}.updateGlobalResidual(res, marker.h());
      repositories::{{SOLVER_INSTANCE}}.updateGlobalSolutionUpdates(du, marker.h()(0));
      repositories::{{SOLVER_INSTANCE}}.updateMinMaxMeshSize( marker.h() );
    }

    logTraceOutWith2Arguments( "ProjectOntoCells", fineGridCell{{SOLVER_NAME}}.getResidual(), fineGridCell{{SOLVER_NAME}}.getSolution() );
  }

  """
  def __init__(self,
               solver,
               descend_invocation_order=0,
               parallel=True):
    super( ProjectIntoCellAndUpdateCellSolution, self ).__init__(
      descend_invocation_order,
      parallel
    )
    self.d = {}
    self.d["SOLVER_INSTANCE"]    = solver.instance_name()
    self.d["SOLVER_NAME"]        = solver.typename()

  def get_body_of_operation(self,operation_name):
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
    return __name__.replace(".py", "").replace(".", "_") + "_ProjectIntoCellAndUpdateCellSolution"
    
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


