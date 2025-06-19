# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractADERDGActionSet import AbstractADERDGActionSet

import jinja2
import peano4.solversteps

class Prediction(AbstractADERDGActionSet):
    """
    The extrapolated solution from the space-time predictor has to be projected onto
    the faces, so we can then solve the Riemann problems. So the projection
    happens in one grid sweep, the corresponding Riemann solve in the next one.
    """

    _Template_TouchCellFirstTime = """
  bool isHangingCell = false;
  for (int d = 0; d < 2 * Dimensions; d++) {
    if (fineGridFaces{{SOLVER_NAME}}FaceLabel(d).getIsHanging()) {
      isHangingCell = true;
      break;
    }
  }

  if ({{GUARD}}) {
    const double timeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();
    {{COMPUTE_TIME_STEP_SIZE}}

#if Dimensions == 2
    constexpr int SpaceFaceSize  = {{ORDER}} + 1;
    constexpr int SpaceBasisSize = ({{ORDER}} + 1) * ({{ORDER}} + 1);
#else
    constexpr int SpaceFaceSize  = ({{ORDER}} + 1) * ({{ORDER}} + 1);
    constexpr int SpaceBasisSize = ({{ORDER}} + 1) * ({{ORDER}} + 1) * ({{ORDER}} + 1);
#endif

    constexpr int SpaceTimeBasisSize    = SpaceBasisSize * ({{ORDER}} + 1);
    constexpr int BasisElementsPerFace  = kernels::{{SOLVER_NAME}}::getBndFaceSize();
    constexpr int FluxElementsPerFace   = kernels::{{SOLVER_NAME}}::getBndFluxSize();

    auto* QNew = fineGridCell{{UNKNOWN_IDENTIFIER}}.value;
    auto* QOld = fineGridCell{{UNKNOWN_IDENTIFIER}}_old.value;

    {{PREPROCESS_RECONSTRUCTED_PATCH}}

    {{CORRECTOR_COMPUTATION_PRECISION}}* lQhbnd[TwoTimesD] = {
      fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(0).value + BasisElementsPerFace,
      fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(1).value + BasisElementsPerFace,
#if Dimensions==3
      fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(2).value + BasisElementsPerFace,
#endif
      fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(0 + Dimensions).value,
      fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(1 + Dimensions).value,
#if Dimensions==3
      fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(2 + Dimensions).value
#endif
    };

    {{CORRECTOR_COMPUTATION_PRECISION}}* lFhbnd[TwoTimesD] = {
      fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(0).value + FluxElementsPerFace,
      fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(1).value + FluxElementsPerFace,
#if Dimensions==3
      fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(2).value + FluxElementsPerFace,
#endif
      fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(0 + Dimensions).value,
      fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(1 + Dimensions).value,
#if Dimensions==3
      fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(2 + Dimensions).value
#endif
    };

    std::copy_n(QNew, SpaceBasisSize * ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}), QOld);
    if ({{ENCLAVE_GUARD}}) { // is skeleton cell
      {{SPACE_TIME_PREDICTOR_CALL}}
    }
{% if USE_ENCLAVE_TASKING %}
    else { // is enclave cell
      {{CORRECTOR_COMPUTATION_PRECISION}}* faces[2][TwoTimesD];
      for (int d = 0; d < TwoTimesD; ++d) {
        faces[0][d] = lQhbnd[d];
        faces[1][d] = lFhbnd[d];
      }

{% if CAN_FUSE %}
      ::exahype2::EnclaveBookkeeping::getInstance().bookmarkTask(new tasks::Prediction{{ENCLAVE_TASK_TYPE}}(
        marker,
        timeStamp,
        timeStepSize,
        QOld,
        faces,
        QNew
      ));
{% else %}
      tarch::multicore::spawnTask(new tasks::Prediction{{ENCLAVE_TASK_TYPE}}(
        marker,
        timeStamp,
        timeStepSize,
        QOld,
        faces,
        QNew
      ));
{% endif %}
    }
{% endif %}

    for (int d = 0; d < Dimensions; d++) {
      // Timestamp information for boundary handling
      fineGridFaces{{SOLVER_NAME}}FaceLabel(d).setNewTimeStamp(1, timeStamp);
      fineGridFaces{{SOLVER_NAME}}FaceLabel(d + Dimensions).setNewTimeStamp(0, timeStamp);
    }

    fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStepSize(timeStepSize);
    repositories::{{SOLVER_INSTANCE}}.update(timeStepSize, timeStamp, tarch::la::min(marker.h()), tarch::la::max(marker.h()));
  }
"""

    def __init__(self, solver, guard):
        super(Prediction, self).__init__(solver)
        self.guard = guard
        self.multiple_precisions = len(solver._predictor_computation_precisions) > 1

    def _add_action_set_entries_to_dictionary(self, d):
        d["GUARD"] = self.guard
        d["ENCLAVE_GUARD"] = "marker.willBeSkeletonCell()" if self._solver._use_enclave_tasking else "true"

    def get_body_of_operation(self, operation_name):
        result = ""
        if operation_name == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            d = {}
            self._solver._init_dictionary_with_default_parameters(d)
            self._solver.add_entries_to_text_replacement_dictionary(d)
            self._add_action_set_entries_to_dictionary(d)
            result = jinja2.Template(self._Template_TouchCellFirstTime).render(**d)
        return result

    def get_body_of_unprepareTraversal(self):
        result = ""
        if self._solver._use_enclave_tasking and self._solver._can_fuse:
            result += "::exahype2::EnclaveBookkeeping::getInstance().fusePendingTasks();"
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        result = super(Prediction, self).get_includes()
        if self._solver._use_enclave_tasking:
            result += """
#include "tasks/Prediction""" + self._solver._name + """EnclaveTask.h"
#include "exahype2/EnclaveBookkeeping.h"
"""
        return result
