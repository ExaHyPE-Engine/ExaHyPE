from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2


class ProjectIntoCellAndUpdateCellSolutionWithTasks(ActionSet):
  templateTouchCellFirstTime="""
  if ( 
    fineGridCell{{SOLVER_NAME}}.getType() != celldata::{{SOLVER_NAME}}::Type::Coarse 
    and
    repositories::{{SOLVER_INSTANCE}}.projectOntoCells()
  ) {
    logTraceInWith2Arguments( "ProjectOntoCells", fineGridCell{{SOLVER_NAME}}.getResidual(), fineGridCell{{SOLVER_NAME}}.getSolution() );

    /**
     * Collect the face data into a face vector and determine the projection 
     * into the cell.
     */
    tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.FaceUnknownsSolution*TwoTimesD, double > faceSol;
    for (int f=0; f<TwoTimesD; f++)
    for (int s=0; s<repositories::{{SOLVER_INSTANCE}}.FaceUnknownsSolution; s++)
      faceSol( f*repositories::{{SOLVER_INSTANCE}}.FaceUnknownsSolution + s ) = fineGridFaces{{SOLVER_NAME}}(f).getSolution(s);

    auto projection = repositories::{{SOLVER_INSTANCE}}.applyCellFromFaceMatrix(marker.x(), marker.h(), faceSol);

    
    /**
     * Take the cell residual and substract the projected values. Before we do 
     * this, we have to wait for the residual calculation to complete.
     */
    int taskNumber = fineGridCell{{TASK_MARKER}}.getNumber() * 2;
    tarch::multicore::waitForTasks({taskNumber+0,taskNumber+1});
    

    /**
     * Now that the volumetric kernel is used, we know that noone is accessing
     * the residual anymore, so we can safely substract the projected value
     * and compute the residual norm.
     */  
    for (int f=0; f<repositories::{{SOLVER_INSTANCE}}.CellUnknowns; f++) {
      fineGridCell{{SOLVER_NAME}}.setResidual(f,
        fineGridCell{{SOLVER_NAME}}.getResidual(f)- projection(f));
    }

    // update global residual before multiplying by the preconditioner
    for (int i=0; i<fineGridCell{{SOLVER_NAME}}.getSolution().size(); i++) {
      double res = fineGridCell{{SOLVER_NAME}}.getResidual( i );
      _solverStatistics.updateGlobalResidual(res, marker.h());
    }


    /**
     * Take inverted cell operator and then apply it. We already have the 
     * residual as a smart pointer, so we can apply the inverse in-situ.
     */
    tarch::la::ShallowMatrix< {{SOLVER_NAME}}::CellUnknowns, {{SOLVER_NAME}}::CellUnknowns, double > invertedApproxSystemMatrix
      = fineGridCell{{SOLVER_NAME}}.getInverseOfSystemMatrix();
    tarch::la::SmartPointerVector<{{SOLVER_NAME}}::CellUnknowns, double> residual( fineGridCell{{SOLVER_NAME}}.getResidualSmartPointer() );

    fineGridCell{{SOLVER_NAME}}.setResidual( 
      tarch::la::multiply( invertedApproxSystemMatrix, residual )
    );
    
    

    /**
     * Free memory. If we immediately run the next iteration sweep, it is
     * better to recycle the memory, i.e. to omit any free here. However, the
     * free is robust, i.e. it does not affect the semantics.
     */
    if ( not repositories::{{SOLVER_INSTANCE}}.updateResidual() ) {
      tarch::freeMemory(
        fineGridCell{{SOLVER_NAME}}.getInverseOfSystemMatrix(),
        tarch::MemoryLocation::Heap 
      );
      fineGridCell{{SOLVER_NAME}}.setInverseOfSystemMatrix( nullptr );
    }

    /**
     * Finally update the solution
     */
    for (int i=0; i<fineGridCell{{SOLVER_NAME}}.getSolution().size(); i++) {
      double res = fineGridCell{{SOLVER_NAME}}.getResidual( i );
      double du  = repositories::{{SOLVER_INSTANCE}}.OmegaCell * res;
      fineGridCell{{SOLVER_NAME}}.setSolution( i,
        fineGridCell{{SOLVER_NAME}}.getSolution( i ) + du
      );
      _solverStatistics.updateGlobalSolutionUpdates(du, marker.h()(0));
      _solverStatistics.updateMinMaxMeshSize( marker.h() );
    }

    logTraceOutWith2Arguments( "ProjectOntoCells", fineGridCell{{SOLVER_NAME}}.getResidual(), fineGridCell{{SOLVER_NAME}}.getSolution() );
  }

  """
  def __init__(self,
               solver,
               descend_invocation_order=0,
               parallel=True):
    super( ProjectIntoCellAndUpdateCellSolutionWithTasks, self ).__init__(
      descend_invocation_order,
      parallel
    )
    self.d = {}
    self.d["SOLVER_INSTANCE"]    = solver.instance_name()
    self.d["SOLVER_NAME"]        = solver.typename()
    self.d["TASK_MARKER"]        = solver._task_name + "MeshNumber"

  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = jinja2.Template(self.templateTouchCellFirstTime).render(**self.d)
      pass
    if operation_name==peano4.solversteps.ActionSet.OPERATION_BEGIN_TRAVERSAL:
      result = jinja2.Template("""
  _solverStatistics.clearGlobalPrecondResidualUpdate();
  _solverStatistics.clearGlobalResidualAndSolutionUpdate();
""").render(**self.d)      
    if operation_name==peano4.solversteps.ActionSet.OPERATION_END_TRAVERSAL:
      result = jinja2.Template("""
  repositories::{{SOLVER_INSTANCE}}.merge( _solverStatistics );
""").render(**self.d)
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
#include "../repositories/SolverRepository.h"
#include "peano4/utils/Loop.h"
#include "tarch/multicore/Task.h"
#include "tarch/multicore/multicore.h"
#include "tarch/la/ShallowMatrix.h"
"""

  def get_attributes(self):
        """!

        Return attributes as copied and pasted into the generated class.

        Please note that action sets are not persistent, i.e. there is one
        object creation per grid sweep per tree.

        """
        return """
  ::mghype::matrixfree::solvers::SolverStatistics _solverStatistics;        
"""        

