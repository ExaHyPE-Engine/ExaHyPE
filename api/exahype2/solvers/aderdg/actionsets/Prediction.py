# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractADERDGActionSet import AbstractADERDGActionSet

import jinja2
import peano4.solversteps


class Prediction(AbstractADERDGActionSet):
    """
    ADER-DG prediction action set.

    The extrapolated solution from the space-time predictor has to be projected onto
    the faces before we can solve the Riemann problems. The projection therefore
    happens in one grid sweep and the corresponding Riemann solve in the next one.
    """

    _Template_TouchCellFirstTime = """
  if ({{GUARD}}) {
    const double timeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();
    {{COMPUTE_TIME_STEP_SIZE}}

#if DIMENSIONS == 2
    constexpr int SpaceBasisSize = ({{ORDER}} + 1) * ({{ORDER}} + 1);
#else
    constexpr int SpaceBasisSize = ({{ORDER}} + 1) * ({{ORDER}} + 1) * ({{ORDER}} + 1);
#endif

    constexpr int BasisElementsPerFace  = facedata::{{SOLVER_NAME}}QEstimates::Cardinality / 2;
    constexpr int FluxElementsPerFace   = facedata::{{SOLVER_NAME}}QFluxEstimates::Cardinality / 2;

    auto* QNew = fineGridCell{{UNKNOWN_IDENTIFIER}}.data();
    auto* QOld = fineGridCell{{UNKNOWN_IDENTIFIER}}_old.data();

    {{PREPROCESS_RECONSTRUCTED_PATCH}}

    {{CORRECTOR_COMPUTATION_PRECISION}}* lQhbnd[TWO_TIMES_D] = {
      fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(0).data() + BasisElementsPerFace,
      fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(1).data() + BasisElementsPerFace,
#if DIMENSIONS==3
      fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(2).data() + BasisElementsPerFace,
#endif
      fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(0 + DIMENSIONS).data(),
      fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(1 + DIMENSIONS).data(),
#if DIMENSIONS==3
      fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(2 + DIMENSIONS).data()
#endif
    };

    {{CORRECTOR_COMPUTATION_PRECISION}}* lFhbnd[TWO_TIMES_D] = {
      fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(0).data() + FluxElementsPerFace,
      fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(1).data() + FluxElementsPerFace,
#if DIMENSIONS==3
      fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(2).data() + FluxElementsPerFace,
#endif
      fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(0 + DIMENSIONS).data(),
      fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(1 + DIMENSIONS).data(),
#if DIMENSIONS==3
      fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(2 + DIMENSIONS).data()
#endif
    };

    std::copy_n(QNew, SpaceBasisSize * ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}), QOld);
    {{SPACE_TIME_PREDICTOR_CALL}}

    for (int d = 0; d < DIMENSIONS; d++) {
      // Time stamp information for the boundary handling.
      fineGridFaces{{SOLVER_NAME}}FaceLabel(d).setNewTimeStamp(1, timeStamp);
      fineGridFaces{{SOLVER_NAME}}FaceLabel(d + DIMENSIONS).setNewTimeStamp(0, timeStamp);

      fineGridFaces{{SOLVER_NAME}}FaceLabel(d).setUpdated(1, true);
      fineGridFaces{{SOLVER_NAME}}FaceLabel(d + DIMENSIONS).setUpdated(0, true);

      // The prediction derives this cell's face estimates, so its halves need
      // no mesh-transfer projection.
      fineGridFaces{{SOLVER_NAME}}FaceLabel(d).setRequiresDataTransfer(1, false);
      fineGridFaces{{SOLVER_NAME}}FaceLabel(d + DIMENSIONS).setRequiresDataTransfer(0, false);
    }

    fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStepSize(timeStepSize);
    repositories::{{SOLVER_INSTANCE}}.update(timeStepSize, timeStamp, tarch::la::min(marker.getH()), tarch::la::max(marker.getH()));
  }
"""

    def __init__(self, solver, guard):
        super(Prediction, self).__init__(solver)
        self.guard = guard
        self.multiple_precisions = len(solver._predictor_computation_precisions) > 1

    def _add_action_set_entries_to_dictionary(self, d):
        d["GUARD"] = self.guard

    def get_body_of_operation(self, operation_name):
        result = ""
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME
        ):
            d = {}
            self._solver._init_dictionary_with_default_parameters(d)
            self._solver.add_entries_to_text_replacement_dictionary(d)
            self._add_action_set_entries_to_dictionary(d)
            result = jinja2.Template(self._Template_TouchCellFirstTime).render(**d)
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        return super(Prediction, self).get_includes()
