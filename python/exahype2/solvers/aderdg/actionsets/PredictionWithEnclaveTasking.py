# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Prediction import Prediction


class PredictionWithEnclaveTasking(Prediction):
    _Template_TouchCellFirstTime_Preamble = """
  bool isHangingCell = false;
  for (int d = 0; d < 2 * Dimensions; d++) {
    if (fineGridFaces{{SOLVER_NAME}}FaceLabel(d).getIsHanging()) {
      isHangingCell = true;
      break;
    }
  }

  if ({{PREDICATE}}) {
    const double timeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();

    // Needs to declare and define timeStepSize
    {{COMPUTE_TIME_STEP_SIZE}}

    {{SOLUTION_STORAGE_PRECISION}}* luh = fineGridCell{{UNKNOWN_IDENTIFIER}}.value;
"""

    _Template_TouchCellFirstTime_Core = """
  if (marker.willBeSkeletonCell()) {
    tasks::{{SOLVER_NAME}}PredictionEnclaveTask::applyKernelToCell(
      marker,
      timeStamp,
      timeStepSize,
      luh,
      luh
    );

    fineGridCell{{SEMAPHORE_LABEL}}.setSemaphoreNumber(::exahype2::EnclaveBookkeeping::SkeletonTask);
    fineGridCell{{SOLVER_NAME}}CellLabel.setHasUpdated(true);

    {% if STORE_OLD_SOLUTION %}
    std::copy_n(luh, sizeLduh, fineGridCell{{UNKNOWN_IDENTIFIER}}_old.value);
    {% endif %}

    repositories::{{SOLVER_INSTANCE}}.update(timeStepSize, timeStamp, marker.h()(0));
  } else { // Is an enclave cell
    assertion(marker.willBeEnclaveCell());
    assertion(not marker.willBeRefined());

    auto newEnclaveTask = new tasks::{{SOLVER_NAME}}PredictionEnclaveTask(
      marker,
      timeStamp,
      timeStepSize,
      luh
    );

    fineGridCell{{SEMAPHORE_LABEL}}.setSemaphoreNumber(newEnclaveTask->getTaskId());

    tarch::multicore::spawnTask(newEnclaveTask);
  }
"""

    _Template_TouchCellFirstTime_Epilogue = """
    {{RESET_TROUBLED_MARKERS}}
  }
"""

    def __init__(self, solver, guard, on_hanging_cells=False):
        super(PredictionWithEnclaveTasking, self).__init__(solver, guard, on_hanging_cells)


    def get_body_of_operation(self, operation_name):
        return (
            super(PredictionWithEnclaveTasking, self).get_body_of_operation(operation_name)
        )


    def get_action_set_name(self):
        return (__name__ + ("_OnHangingCells" if self.onHangingCells else "")).replace(".py", "").replace(".", "_")


    def get_includes(self):
        return (
            super(PredictionWithEnclaveTasking, self).get_includes()
            + """
#include "tarch/multicore/Tasks.h"
#include "repositories/SolverRepository.h"
#include "tasks/"""
            + self._solver._name
            + """PredictionEnclaveTask.h"
"""
        )
