# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractADERDGActionSet import AbstractADERDGActionSet

import jinja2
import peano4.solversteps


class Prediction(AbstractADERDGActionSet):
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

    auto* const updatedState = fineGridCell{{UNKNOWN_IDENTIFIER}}.data();
    auto* const previousState = fineGridCell{{UNKNOWN_IDENTIFIER}}_old.data();

    // User preprocessing kernels use the established QNew and QOld names.
    [[maybe_unused]] auto* const QNew = updatedState;
    [[maybe_unused]] auto* const QOld = previousState;

    {{PREPROCESS_RECONSTRUCTED_PATCH}}

    {{CORRECTOR_COMPUTATION_PRECISION}}* stateEstimates[peano4::FacesPerCell] = {
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

    {{CORRECTOR_COMPUTATION_PRECISION}}* fluxEstimates[peano4::FacesPerCell] = {
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

    // This copy decides which buffer a plotter reads: In fused ADER-DG the
    // correction writes the accepted state into Q earlier in this traversal, so
    // Q_old carries it. A limiting solver corrects in its own traversal and leaves
    // the accepted state in Q.
    std::copy_n(
      updatedState,
      SpaceBasisSize * ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}),
      previousState
    );
    {{SPACE_TIME_PREDICTOR_CALL}}

    for (int direction = 0; direction < DIMENSIONS; direction++) {
      fineGridFaces{{SOLVER_NAME}}FaceLabel(direction).setNewTimeStamp(1, timeStamp);
      fineGridFaces{{SOLVER_NAME}}FaceLabel(direction + DIMENSIONS).setNewTimeStamp(0, timeStamp);

      fineGridFaces{{SOLVER_NAME}}FaceLabel(direction).setUpdated(1, true);
      fineGridFaces{{SOLVER_NAME}}FaceLabel(direction + DIMENSIONS).setUpdated(0, true);

      // The prediction derives this cell's face estimates, so its halves need
      // no mesh-transfer projection.
      fineGridFaces{{SOLVER_NAME}}FaceLabel(direction).setRequiresDataTransfer(1, false);
      fineGridFaces{{SOLVER_NAME}}FaceLabel(direction + DIMENSIONS).setRequiresDataTransfer(0, false);
    }

    fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStepSize(timeStepSize);
    bool hasHangingFaces = false;
    for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
      const auto& faceLabel =
        fineGridFaces{{SOLVER_NAME}}FaceLabel(faceNumber);
      hasHangingFaces =
        hasHangingFaces
        or faceLabel.getIsHanging()
        or faceLabel.getAboveHanging()[0]
        or faceLabel.getAboveHanging()[1];
    }
    repositories::{{SOLVER_INSTANCE}}.update(
      timeStepSize,
      timeStamp,
      tarch::la::min(marker.getH()),
      tarch::la::max(marker.getH()),
      hasHangingFaces
    );
  }
"""

    def __init__(self, solver, guard):
        super(Prediction, self).__init__(solver)
        self.guard = guard
        self.multiple_precisions = len(solver._predictor_computation_precisions) > 1

    def _add_action_set_entries_to_dictionary(self, dictionary):
        dictionary["GUARD"] = self.guard

    def get_body_of_operation(self, operation_name):
        result = ""
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME
        ):
            dictionary = {}
            self._solver._init_dictionary_with_default_parameters(dictionary)
            self._solver.add_entries_to_text_replacement_dictionary(dictionary)
            self._add_action_set_entries_to_dictionary(dictionary)
            result = jinja2.Template(self._Template_TouchCellFirstTime).render(
                **dictionary
            )
        if operation_name == peano4.solversteps.ActionSet.OPERATION_END_TRAVERSAL:
            result = (
                "  repositories::"
                + self._solver.get_name_of_global_instance()
                + ".reduceLocalUpdates();\n"
            )
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        return super(Prediction, self).get_includes()
