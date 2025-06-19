# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractADERDGActionSet import AbstractADERDGActionSet

import jinja2
import peano4.solversteps

class PredictionOnHangingCells(AbstractADERDGActionSet):
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
    constexpr int SpaceFaceSize         = {{ORDER}} + 1;
    constexpr int SpaceBasisSize        = ({{ORDER}} + 1) * ({{ORDER}} + 1);
#else
    constexpr int SpaceFaceSize         = ({{ORDER}} + 1) * ({{ORDER}} + 1);
    constexpr int SpaceBasisSize        = ({{ORDER}} + 1) * ({{ORDER}} + 1) * ({{ORDER}} + 1);
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
    {{SPACE_TIME_PREDICTOR_CALL}}

    /**
     * Copy the estimates into the face data containers. Note that Peano uses a different ordering of
     * the faces that the kernels, so these are reordered somewhat.
     * The kernels order by dimension, then side of the face whereas Peano orders by
     * side, then dimension.
     * E.g., in 2d the kernels have the order left, right, lower face, upper face
     * whereas Peano orders: left, lower, right, upper.
     *
     * 2d: 0,1,2,3 -> 0,2,1,3
     * 3d: 0,1,2,3,4,5 -> 0,3,1,4,2,5
     *
     * On hanging faces, this instead computes the Riemann solution and face integral, then
     * stores the solution of this into the face. The reason for this is that performing the
     * Riemann solution on the hanging cell and restricting these is much more accurate than
     * performing them on the coarse faces and projecting those to the fine faces, to the
     * degree that the later breaks many simulations.
     */
    for (int d = 0; d < Dimensions; d++) {
      int direction = d;

      // Left values
      if (fineGridFaces{{SOLVER_NAME}}FaceLabel(d).getIsHanging()) {
        {{CORRECTOR_COMPUTATION_PRECISION}}* FL = fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d).value;
        {{CORRECTOR_COMPUTATION_PRECISION}}* FR = fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d).value + FluxElementsPerFace;
        {{CORRECTOR_COMPUTATION_PRECISION}}* QL = fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d).value;
        {{CORRECTOR_COMPUTATION_PRECISION}}* QR = fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d).value + BasisElementsPerFace;
        const int orientation = 0;
        const int faceNumber  = d;
        const bool isBoundary = false;
        auto faceCentre = marker.x();
        faceCentre[d] -= 0.5 * marker.h()[d];
        {{RIEMANN_SOLVER_CALL}}
        {{CORRECTOR_COMPUTATION_PRECISION}}* FIn = FR;
        {{FACE_INTEGRAL_CALL}}
      }

      // Right values
      if (fineGridFaces{{SOLVER_NAME}}FaceLabel(d+Dimensions).getIsHanging()) {
        {{CORRECTOR_COMPUTATION_PRECISION}}* FL = fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d + Dimensions).value;
        {{CORRECTOR_COMPUTATION_PRECISION}}* FR = fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d + Dimensions).value + FluxElementsPerFace;
        {{CORRECTOR_COMPUTATION_PRECISION}}* QL = fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d + Dimensions).value;
        {{CORRECTOR_COMPUTATION_PRECISION}}* QR = fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d + Dimensions).value + BasisElementsPerFace;
        const int orientation = 1;
        const int faceNumber  = d + Dimensions;
        const bool isBoundary = false;
        auto faceCentre = marker.x();
        faceCentre[d] += 0.5 * marker.h()[d];
        {{RIEMANN_SOLVER_CALL}}
        {{CORRECTOR_COMPUTATION_PRECISION}}* FIn = FL;
        {{FACE_INTEGRAL_CALL}}
      }
    }

    fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStepSize(timeStepSize);
    repositories::{{SOLVER_INSTANCE}}.update(timeStepSize, timeStamp, tarch::la::min(marker.h()), tarch::la::max(marker.h()));
  }
"""

    def __init__(self, solver, guard):
        super(PredictionOnHangingCells, self).__init__(solver)
        self.guard = guard
        self.multiple_precisions = len(solver._predictor_computation_precisions) > 1

    def _add_action_set_entries_to_dictionary(self, d):
        d["GUARD"] = self.guard

    def get_body_of_operation(self, operation_name):
        result = ""
        if operation_name == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            d = {}
            self._solver._init_dictionary_with_default_parameters(d)
            self._solver.add_entries_to_text_replacement_dictionary(d)
            self._add_action_set_entries_to_dictionary(d)
            result = jinja2.Template(self._Template_TouchCellFirstTime).render(**d)
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        result = super(PredictionOnHangingCells, self).get_includes()
        return result
