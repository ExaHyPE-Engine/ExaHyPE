# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

from .AbstractRungeKuttaDGActionSet import AbstractRungeKuttaDGActionSet

from exahype2.solvers.ButcherTableau                import ButcherTableau

import peano4
import exahype2
import jinja2


class SolveVolumeIntegral(AbstractRungeKuttaDGActionSet):
  """
  Solve volume integral

  Solve the volumetric part of the DG method. This includes the evaluation of the
  volume integral of the operator multiplied with a trial function, plus the
  subsequent multiplication with an inverted (lumped) mass matrix. So, eventually,
  we do an explicit Euler step subject to a relaxed time step size from the Butcher
  tableau. The input is already properly computed (see LinearCombinationOfEstimates()).

  The volume integral definition is an underdetermined action set, i.e., you still
  have to ensure that a number of macros are actually defined when you use it:

  - {{COMPUTE_TIME_STEP_SIZE}} has to specify a double timeStepSize. It may be const if
    you want. I don't alter it.

  In return, the following variables will be set by the action set:

  - timeStampOldSolution: double This is the time stamp of the original 
    solution, i.e. not the one we are currently evaluating due to the 
    Runge-Kutta scheme.
  - timeStamp: double This is the time stamp of the current Runge Kutta
    guess. It is timeStampOldSolution plus the weight of the Butcher scheme
    times the time step size.
  - QIn: double* A pointer to the actual linear combination of previous 
    guesses according to the Butcher tableau.
  - QOut: For Runge-Kutta, we ahve to store all guesses of the future solution,
    so we can eventually combine them into one final outcome. QOut points to 
    that segment/guess that's to be computed in this step.

  """

  TemplateCellKernel = """
  {% for PREDICATE_NO in range(0,PREDICATES|length) %}
  if ({{PREDICATES[PREDICATE_NO]}}) {
    const double timeStampOldSolution    = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();

    // Set the variable
    // double timeStepSize
    {{COMPUTE_TIME_STEP_SIZE}}

    const double timeStamp = timeStampOldSolution + {{BUTCHER_TABLEAU_RELATIVE_TIME_STEP_SIZES[PREDICATE_NO]}} * timeStepSize;

    assertion3( tarch::la::greaterEquals( timeStepSize, 0.0 ),  timeStamp, timeStepSize, timeStampOldSolution );
    assertion3( tarch::la::greaterEquals( timeStamp, 0.0 ),     timeStamp, timeStepSize, timeStampOldSolution );

    double* QIn  = fineGridCell{{UNKNOWN_IDENTIFIER}}LinearCombination.value;
    #if Dimensions==2
    double* QOut = fineGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.value + {{PREDICATE_NO}} * {{NUMBER_OF_DOFS_PER_CELL_2D}} * {{NUMBER_OF_UNKNOWNS}};
    #elif Dimensions==3
    double* QOut = fineGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.value + {{PREDICATE_NO}} * {{NUMBER_OF_DOFS_PER_CELL_3D}} * {{NUMBER_OF_UNKNOWNS}};
    #endif

    {% if SPAWN_VOLUME_KERNEL_AS_TASK %}
    if (marker.willBeSkeletonCell()) {
      ::exahype2::CellData cellData(
        QIn,
        marker.x(),
        marker.h(),
        timeStamp,
        timeStepSize,
        QOut
      );

      tasks::{{SOLVER_NAME}}_VolumetricSolverEnclaveTask::applyKernelToCell(
        marker,
        timeStamp,
        timeStepSize,
        QIn,
        QOut
      );
    } else {
      assertion(marker.willBeEnclaveCell());
      assertion(not marker.willBeRefined());
      auto newEnclaveTask = new tasks::{{SOLVER_NAME}}_VolumetricSolverEnclaveTask(
        marker,
        timeStamp,
        timeStepSize,
        {% if MAKE_COPY_OF_ENCLAVE_TASK_DATA %}
        QIn
        {% else %}
        fineGridCell{{UNKNOWN_IDENTIFIER}}LinearCombination.getSmartPointer(),
        QOut
        {% endif %}
      );
      
      int predecessorEnclaveTaskNumber = fineGridCell{{SEMAPHORE_LABEL}}.getSemaphoreNumber();
      
      tarch::multicore::spawnTask(
        newEnclaveTask, 
        predecessorEnclaveTaskNumber>=0 ? std::set<int>{predecessorEnclaveTaskNumber} : tarch::multicore::NoInDependencies, 
        newEnclaveTask->getTaskId()
      );
      
      ::exahype2::EnclaveTask::releaseTaskNumber(predecessorEnclaveTaskNumber);

      fineGridCell{{SEMAPHORE_LABEL}}.setSemaphoreNumber(newEnclaveTask->getTaskId());
    }
    {% else %}
    ::exahype2::CellData cellData(
      QIn,
      marker.x(),
      marker.h(),
      timeStamp,
      timeStepSize,
      QOut
    );

    {% if STATELESS_PDE_TERMS %}
    if (repositories::{{SOLVER_INSTANCE}}.cellCanUseStatelessPDETerms(
      marker.x(),
      marker.h(),
      timeStamp,
      timeStepSize
    )) {
      ::exahype2::dg::{{KERNEL_NAMESPACE}}::{{VOLUMETRIC_COMPUTE_KERNEL_CALL_STATELESS}}
    } else
    {% endif %}

    ::exahype2::dg::{{KERNEL_NAMESPACE}}::{{VOLUMETRIC_COMPUTE_KERNEL_CALL}}
    {% endif %}
  }
  {% endfor %}
"""


  def __init__(self,solver,spawn_volume_kernel_as_task = False):
    """
    guard_project: String (C++ code)
      Predicate which controls if the solution is actually projected.

    guard_safe_old_time_step: String (C++ code)
      Predicate which controls if the projection should be copied into
      the old solution and the time step should also be moved over.
    """
    super(SolveVolumeIntegral,self).__init__(solver)
    self.guards                      = []
    self._butcher_tableau            = ButcherTableau(self._solver._rk_order)
    self.spawn_volume_kernel_as_task = spawn_volume_kernel_as_task


  @property
  def guards(self):
    if self._guards==[]:
      raise Exception("Guards are not initialised")
    return self._guards


  @guards.setter
  def guards(self,new_guards):
    if new_guards!=[] and len(new_guards)!=self._solver.number_of_Runge_Kutta_steps():
      raise Exception( "Expect one guard per Runge Kutta step. Have {} steps but got guards {}".format(solver.number_of_Runge_Kutta_steps(),guards) )
    self._guards = new_guards


  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)
      d[ "PREDICATES" ]                               = self.guards
      d[ "BUTCHER_TABLEAU_RELATIVE_TIME_STEP_SIZES" ] = self._butcher_tableau.time_step_sizes()
      d[ "SPAWN_VOLUME_KERNEL_AS_TASK"]               = self.spawn_volume_kernel_as_task
      d[ "SEMAPHORE_LABEL" ]      = exahype2.grid.UpdateCellLabel.get_attribute_name(self._solver._name)
      result = jinja2.Template(self.TemplateCellKernel).render(**d)
      pass
    return result


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


  def get_includes(self):
    if self.spawn_volume_kernel_as_task:
      return super( SolveVolumeIntegral, self ).get_includes() + """
#include "tasks/""" + self._solver._name + """_VolumetricSolverEnclaveTask.h"
"""
    else:
      return super( SolveVolumeIntegral, self ).get_includes()
