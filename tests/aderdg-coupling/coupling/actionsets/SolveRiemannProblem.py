# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from exahype2.actionsets import ActionSet

from .AbstractLimiterActionSet import AbstractLimiterActionSet


class SolveRiemannProblem(AbstractLimiterActionSet):
    """Copies the face estimates of one solver onto the quadrature points of the other."""

    TemplateCopyFaceData = """
  if ( {{PREDICATE}} ) {
    constexpr int FluxElementsPerFace   = facedata::{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}FluxEstimates::Cardinality / 2;
    constexpr int BasisElementsPerFace  = facedata::{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}Estimates::Cardinality / 2;

    constexpr int FluxElementsPerFace2  = facedata::{{LIMITER_SOLVER_UNKNOWN_IDENTIFIER}}FluxEstimates::Cardinality / 2;
    constexpr int BasisElementsPerFace2 = facedata::{{LIMITER_SOLVER_UNKNOWN_IDENTIFIER}}Estimates::Cardinality / 2;

    constexpr int NumberOfVariables  = repositories::{{REGULAR_SOLVER_INSTANCE}}.NumberOfUnknowns;
    constexpr int NumberOfParameters = repositories::{{REGULAR_SOLVER_INSTANCE}}.NumberOfAuxiliaryVariables;

    if (fineGridCell{{SOLVER_NAME}}CellLabel.getTroubledMarker() == celldata::{{SOLVER_NAME}}CellLabel::TroubledMarker::REGULAR) {
      for (int d = 0; d < DIMENSIONS; d++) {
        if (fineGridFaces{{SOLVER_NAME}}FaceLabel(d).getTroubledMarker() == facedata::{{SOLVER_NAME}}FaceLabel::TroubledMarker::TROUBLED) {
          kernels::{{SOLVER_NAME}}::regularToLimiterFace(
            fineGridFaces{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}Estimates(d).data() + BasisElementsPerFace,
            fineGridFaces{{LIMITER_SOLVER_UNKNOWN_IDENTIFIER}}Estimates(d).data() + BasisElementsPerFace2,
            NumberOfVariables + NumberOfParameters
          );
          kernels::{{SOLVER_NAME}}::regularToLimiterFace(
            fineGridFaces{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}FluxEstimates(d).data() + FluxElementsPerFace,
            fineGridFaces{{LIMITER_SOLVER_UNKNOWN_IDENTIFIER}}FluxEstimates(d).data() + FluxElementsPerFace2,
            NumberOfVariables
          );
        }

        if (fineGridFaces{{SOLVER_NAME}}FaceLabel(d + DIMENSIONS).getTroubledMarker() == facedata::{{SOLVER_NAME}}FaceLabel::TroubledMarker::TROUBLED) {
          kernels::{{SOLVER_NAME}}::regularToLimiterFace(
            fineGridFaces{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}Estimates(d + DIMENSIONS).data(),
            fineGridFaces{{LIMITER_SOLVER_UNKNOWN_IDENTIFIER}}Estimates(d + DIMENSIONS).data(),
            NumberOfVariables + NumberOfParameters
          );
          kernels::{{SOLVER_NAME}}::regularToLimiterFace(
            fineGridFaces{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}FluxEstimates(d + DIMENSIONS).data(),
            fineGridFaces{{LIMITER_SOLVER_UNKNOWN_IDENTIFIER}}FluxEstimates(d + DIMENSIONS).data(),
            NumberOfVariables
          );
        }
      }
    }

    if (fineGridCell{{SOLVER_NAME}}CellLabel.getTroubledMarker() == celldata::{{SOLVER_NAME}}CellLabel::TroubledMarker::TROUBLED) {
      for (int d = 0; d < DIMENSIONS; d++) {
        kernels::{{SOLVER_NAME}}::limiterToRegularFace(
          fineGridFaces{{LIMITER_SOLVER_UNKNOWN_IDENTIFIER}}Estimates(d).data() + BasisElementsPerFace2,
          fineGridFaces{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}Estimates(d).data() + BasisElementsPerFace,
          NumberOfVariables + NumberOfParameters
        );
        kernels::{{SOLVER_NAME}}::limiterToRegularFace(
          fineGridFaces{{LIMITER_SOLVER_UNKNOWN_IDENTIFIER}}FluxEstimates(d).data() + FluxElementsPerFace2,
          fineGridFaces{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}FluxEstimates(d).data() + FluxElementsPerFace,
          NumberOfVariables
        );

        kernels::{{SOLVER_NAME}}::limiterToRegularFace(
          fineGridFaces{{LIMITER_SOLVER_UNKNOWN_IDENTIFIER}}Estimates(d + DIMENSIONS).data(),
          fineGridFaces{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}Estimates(d + DIMENSIONS).data(),
          NumberOfVariables + NumberOfParameters
        );
        kernels::{{SOLVER_NAME}}::limiterToRegularFace(
          fineGridFaces{{LIMITER_SOLVER_UNKNOWN_IDENTIFIER}}FluxEstimates(d + DIMENSIONS).data(),
          fineGridFaces{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}FluxEstimates(d + DIMENSIONS).data(),
          NumberOfVariables
        );
      }
    }
  }
  """

    def operation_body(self, operation_name):
        result = ""
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
            result = self.render(self.TemplateCopyFaceData)
        return result

    @property
    def name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def includes(self):
        return super().includes() + ["kernels/{}/Limiter.h".format(self._solver.name)]
