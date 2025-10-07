from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2


class UpdateResidualWithTasks(ActionSet):
  """!

  This is for DG solver
  
  @todo Documntation missing
  
  """
  templateTouchCellFirstTime="""
  if ( 
    fineGridCell{{SOLVER_NAME}}.getType() != celldata::{{SOLVER_NAME}}::Type::Coarse 
    and
    // only one condition here - ensures that we don't do this when restricting the residual
    repositories::{{SOLVER_INSTANCE}}.updateResidual()
  ) {
    logTraceInWith4Arguments("touchCellFirstTime(...)", 
      fineGridCell{{SOLVER_NAME}}.getSolution(),
      fineGridCell{{SOLVER_NAME}}.getRhs(),
      marker.toString(),
      repositories::{{SOLVER_INSTANCE}}.getRhsMatrix( marker.x(), marker.h() )
    );
    
    const int OperatorSize = {{SOLVER_NAME}}::CellUnknowns * {{SOLVER_NAME}}::CellUnknowns;

    /**
     * Lazy allocation of memory. In this particular realisation, it is never
     * released. In principle, that's a reasonable strategy, although we might
     * want to re-evaluate it later.
     */
    if (fineGridCell{{SOLVER_NAME}}.getInverseOfSystemMatrix()==nullptr) {
      fineGridCell{{SOLVER_NAME}}.setInverseOfSystemMatrix(
        tarch::allocateMemory<double>( OperatorSize, tarch::MemoryLocation::Heap )
      );
    }
        
    auto rhsPointer         = fineGridCell{{SOLVER_NAME}}.getRhsSmartPointer();
    auto solutionPointer    = fineGridCell{{SOLVER_NAME}}.getSolutionSmartPointer();
    auto residualPointer    = fineGridCell{{SOLVER_NAME}}.getResidualSmartPointer();
    double* inverseOperator = fineGridCell{{SOLVER_NAME}}.getInverseOfSystemMatrix();
    
    /**
     * Without the mutable keyword, the copied four arguments will stay const.
     * C++ then complains as any assignment to a const copy becomes invalid.
     * 
     * @todo There might be a lot of scope for optimisation
     * 
     * It remains an open question to which degree these implementations can be
     * optimised if we avoid copying forth and back into tarch vectors. We 
     * could set residualPointer to zero and then accumulate the outcome 
     * directly into this smart pointer.
     */
    auto updateResidualFunctor = [rhsPointer, solutionPointer, residualPointer, marker]() mutable -> void {
      logTraceInWith1Argument( "UpdateResidualWithTasks - updateResidualFunctor", marker.toString() );
      
      tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.CellUnknowns, double > r =
        repositories::{{SOLVER_INSTANCE}}.applyRhsMatrix( marker.x(), marker.h(), rhsPointer.toVector() );
      
      r = r - repositories::{{SOLVER_INSTANCE}}.applyLocalAssemblyMatrix(marker.x(), marker.h(), solutionPointer.toVector() );
      
      residualPointer = r;
      
      logInfo( "UpdateResidualWithTasks - updateResidualFunctor", marker.toString() );
      logTraceOutWith1Argument( "UpdateResidualWithTasks - updateResidualFunctor", marker.toString() );
    };


    /**
     * Issue the calculation of the inverse of the system matrix
     */
    auto invertMatrixFunctor = [inverseOperator, marker, OperatorSize]() mutable -> void {
      logTraceInWith1Argument( "UpdateResidualWithTasks - invertMatrixFunctor", marker.toString() );
      std::memcpy(
        inverseOperator,
        repositories::{{SOLVER_INSTANCE}}.getInvertedApproxSystemMatrix(
          marker.x(),
          marker.h()
        ).data(),
        sizeof(double)*OperatorSize
      );
      logInfo( "UpdateResidualWithTasks - invertMatrixFunctor", marker.toString() );
      logTraceOutWith1Argument( "UpdateResidualWithTasks - invertMatrixFunctor", marker.toString() );
    };


    static int residualTaskType     = peano4::parallel::getTaskType("{{SOLVER_INSTANCE}}::UpdateResidualTask");
    static int invertMatrixTaskType = peano4::parallel::getTaskType("{{SOLVER_INSTANCE}}::InvertTask");
    tarch::multicore::TaskWithCopyOfFunctor* updateResidualTask = new tarch::multicore::TaskWithCopyOfFunctor(
      residualTaskType,       
      tarch::multicore::Task::DefaultPriority,
      updateResidualFunctor
    );  
    tarch::multicore::TaskWithCopyOfFunctor* invertMatrixTask = new tarch::multicore::TaskWithCopyOfFunctor(
      invertMatrixTaskType,  
      tarch::multicore::Task::DefaultPriority,
      invertMatrixFunctor
    );  
    

    int taskNumber = fineGridCell{{TASK_MARKER}}.getNumber() * 2;
    tarch::multicore::spawnTask(
      updateResidualTask,
      tarch::multicore::NoInDependencies,
      taskNumber+0
    );
    tarch::multicore::spawnTask(
      invertMatrixTask,
      tarch::multicore::NoInDependencies,
      taskNumber+1
    ); 
    logInfo( "touchCellFirstTime(...)", "issue tasks " << (taskNumber+0) << " and " << (taskNumber+1) << " for " << marker.toString() );

    logTraceOut("touchCellFirstTime(...)");
  }
  """


  def __init__(self,
               solver,
               descend_invocation_order=0,
               parallel=True):
    super( UpdateResidualWithTasks, self ).__init__(
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

#include <cstring>
"""
