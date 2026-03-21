# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractADERDGActionSet import AbstractADERDGActionSet

import jinja2
import peano4.solversteps


class Correction(AbstractADERDGActionSet):
    _Template_TouchFaceFirstTime = """
  if ({{FACE_GUARD}}) {
    const double timeStamp = std::max(
      fineGridFace{{SOLVER_NAME}}FaceLabel.getNewTimeStamp(0),
      fineGridFace{{SOLVER_NAME}}FaceLabel.getNewTimeStamp(1)
    );

    {{COMPUTE_TIME_STEP_SIZE}}

#if DIMENSIONS == 2
    constexpr int SpaceFaceSize = {{ORDER}} + 1;
#else
    constexpr int SpaceFaceSize = ({{ORDER}} + 1) * ({{ORDER}} + 1);
#endif

    constexpr int FluxElementsPerFace   = SpaceFaceSize * {{NUMBER_OF_UNKNOWNS}};
    constexpr int BasisElementsPerFace  = SpaceFaceSize * ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}});

    /*
     * 2d: 0,1,2,3 -> 0,2,1,3
     * 3d: 0,1,2,3,4,5 -> 0,3,1,4,2,5
     */
    {{CORRECTOR_COMPUTATION_PRECISION}}* FL = fineGridFace{{SOLVER_NAME}}QFluxEstimates.data();
    {{CORRECTOR_COMPUTATION_PRECISION}}* FR = fineGridFace{{SOLVER_NAME}}QFluxEstimates.data() + FluxElementsPerFace;
    {{CORRECTOR_COMPUTATION_PRECISION}}* QL = fineGridFace{{SOLVER_NAME}}QEstimates.data();
    {{CORRECTOR_COMPUTATION_PRECISION}}* QR = fineGridFace{{SOLVER_NAME}}QEstimates.data() + BasisElementsPerFace;
    const int direction     = marker.getSelectedFaceNumber() % DIMENSIONS;
    const int faceNumber    = marker.getSelectedFaceNumber();
    const bool isBoundary   = fineGridFace{{SOLVER_NAME}}FaceLabel.getBoundary();
    const auto& faceCentre  = marker.getX();

    {{RIEMANN_SOLVER_CALL}}

    fineGridFace{{SOLVER_NAME}}FaceLabel.setUpdated(0, true);
    fineGridFace{{SOLVER_NAME}}FaceLabel.setUpdated(1, true);
  }
"""

    _Template_TouchCellFirstTime = """
  if ({{CELL_GUARD}}) {
    const double timeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();
    // Reuse the same timeStepSize from the predictor
    const double timeStepSize = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStepSize();

    {{SOLUTION_STORAGE_PRECISION}}* QNew = fineGridCell{{UNKNOWN_IDENTIFIER}}.data();
    constexpr int FluxElementsPerFace = kernels::{{SOLVER_NAME}}::getBndFluxSize();

    for (int d = 0; d < DIMENSIONS; d++) {
      const int direction = d;

      // Negative face
      if (!fineGridFaces{{SOLVER_NAME}}FaceLabel(d).getIsHanging()) {
        // If above hanging face, the Riemann fluxes have been received from the other side and are therefore on that side.
        {{CORRECTOR_COMPUTATION_PRECISION}}* FIn = fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d).data() + (fineGridFaces{{SOLVER_NAME}}FaceLabel(d).getAboveHanging() ? 0 : FluxElementsPerFace);
        const int orientation = 0;
        {{FACE_INTEGRAL_CALL}}
      }

      // Positive face
      if (!fineGridFaces{{SOLVER_NAME}}FaceLabel(d + DIMENSIONS).getIsHanging()) {
        // If above hanging face, the Riemann fluxes have been received from the other side and are therefore on that side.
        {{CORRECTOR_COMPUTATION_PRECISION}}* FIn = fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d + DIMENSIONS).data() + (fineGridFaces{{SOLVER_NAME}}FaceLabel(d + DIMENSIONS).getAboveHanging() ? FluxElementsPerFace : 0);
        const int orientation = 1;
        {{FACE_INTEGRAL_CALL}}
      }
    } // for d

    {{POSTPROCESS_UPDATED_PATCH}}

    {{COMPUTE_NEW_TIME_STEP_SIZE}}

    fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStamp(timeStamp + timeStepSize);
    fineGridCell{{SOLVER_NAME}}CellLabel.setHasUpdated(true);

    repositories::{{SOLVER_INSTANCE}}.update(newTimeStepSize, timeStamp + timeStepSize, tarch::la::min(marker.getH()), tarch::la::max(marker.getH()));
  }
"""

    def __init__(self, solver, cell_guard, face_guard):
        super(Correction, self).__init__(solver)
        self.cell_guard = cell_guard
        self.face_guard = face_guard

    def _add_action_set_entries_to_dictionary(self, d):
        d["CELL_GUARD"] = self.cell_guard
        d["FACE_GUARD"] = self.face_guard

    def get_body_of_operation(self, operation_name):
        result = ""
        d = {}
        self._solver._init_dictionary_with_default_parameters(d)
        self._solver.add_entries_to_text_replacement_dictionary(d)
        self._add_action_set_entries_to_dictionary(d)
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME
        ):
            result = jinja2.Template(self._Template_TouchFaceFirstTime).render(**d)
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME
        ):
            result = jinja2.Template(self._Template_TouchCellFirstTime).render(**d)
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        result = super(Correction, self).get_includes()
        return result
