# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2
import peano4

from .AbstractRungeKuttaDGActionSet import AbstractRungeKuttaDGActionSet


class UpdateSolution(AbstractRungeKuttaDGActionSet):
    """Updates one Runge-Kutta stage per grid sweep."""

    _FACE = r"""
  const int insideSide = marker.getSelectedFaceNumber() < DIMENSIONS ? 1 : 0;
  const double timeStamp = fineGridFace{{SOLVER_NAME}}FaceLabel.getUpdatedTimeStamp(insideSide);
  const double timeStepSize = repositories::{{SOLVER_INSTANCE}}.getPreviousStageTimeStepSize();
  assertion(std::isfinite(timeStamp) and timeStamp >= 0.0, timeStamp);
  assertion(std::isfinite(timeStepSize) and timeStepSize >= 0.0, timeStepSize);

  constexpr int StateElementsPerFace =
    facedata::{{UNKNOWN_IDENTIFIER}}EstimateProjection::Cardinality / 2;
  constexpr int FluxElementsPerFace =
    facedata::{{UNKNOWN_IDENTIFIER}}RiemannSolution::Cardinality / 2;
  const double* leftStateEstimates =
    fineGridFace{{UNKNOWN_IDENTIFIER}}EstimateProjection.data();
  const double* rightStateEstimates = leftStateEstimates + StateElementsPerFace;
  double* leftFluxEstimates =
    fineGridFace{{UNKNOWN_IDENTIFIER}}RiemannSolution.data();
  double* rightFluxEstimates = leftFluxEstimates + FluxElementsPerFace;
  const int direction = marker.getSelectedFaceNumber() % DIMENSIONS;
  const auto& faceCentre = marker.getX();

  if (
    not repositories::{{SOLVER_INSTANCE}}.PeriodicBoundaries[direction]
    and not marker.hasBeenRefined()
    and fineGridFace{{SOLVER_NAME}}FaceLabel.getBoundary()
  ) {
    ::exahype2::dg::applyBoundaryConditions<
      {{FULL_QUALIFIED_SOLVER_NAME}}, {{DG_ORDER}}+1,
      {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}}
    >(
      fineGridFace{{UNKNOWN_IDENTIFIER}}EstimateProjection.data(),
      kernels::{{SOLVER_NAME}}::Quadrature<double>::QuadraturePoints1D,
      faceCentre, marker.getH(), timeStamp,
      marker.getSelectedFaceNumber()
    );
    const int outsideSide = marker.getSelectedFaceNumber() < DIMENSIONS ? 0 : 1;
    ::exahype2::dg::evaluateFluxOnFace<
      {{FULL_QUALIFIED_SOLVER_NAME}}, {{DG_ORDER}}+1,
      {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {% if FLUX_IMPLEMENTATION!="<none>" %}true{% else %}false{% endif %},
      {{ "true" if USE_DIFFUSIVE_FLUX else "false" }}
    >(
      leftFluxEstimates + outsideSide * FluxElementsPerFace,
      leftStateEstimates, rightStateEstimates, outsideSide,
      kernels::{{SOLVER_NAME}}::Quadrature<double>::QuadraturePoints1D,
      faceCentre, marker.getH(), timeStamp, timeStepSize, direction
    );
    if (marker.getSelectedFaceNumber()<DIMENSIONS) {
      fineGridFace{{SOLVER_NAME}}FaceLabel.setUpdatedTimeStamp(0, fineGridFace{{SOLVER_NAME}}FaceLabel.getUpdatedTimeStamp(1));
      fineGridFace{{SOLVER_NAME}}FaceLabel.setNewTimeStamp(0, fineGridFace{{SOLVER_NAME}}FaceLabel.getNewTimeStamp(1));
      fineGridFace{{SOLVER_NAME}}FaceLabel.setOldTimeStamp(0, fineGridFace{{SOLVER_NAME}}FaceLabel.getOldTimeStamp(1));
    } else {
      fineGridFace{{SOLVER_NAME}}FaceLabel.setUpdatedTimeStamp(1, fineGridFace{{SOLVER_NAME}}FaceLabel.getUpdatedTimeStamp(0));
      fineGridFace{{SOLVER_NAME}}FaceLabel.setNewTimeStamp(1, fineGridFace{{SOLVER_NAME}}FaceLabel.getNewTimeStamp(0));
      fineGridFace{{SOLVER_NAME}}FaceLabel.setOldTimeStamp(1, fineGridFace{{SOLVER_NAME}}FaceLabel.getOldTimeStamp(0));
    }
  }

  {{RIEMANN_SOLVER_CALL}}
"""

    _CELL = r"""
const int stage = repositories::{{SOLVER_INSTANCE}}.getRkStage();
if (stage>=0) {
  const int previousStage = (stage + {{RK_STEPS}} - 1) % {{RK_STEPS}};
  double stageTimeStepSize = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStepSize();

  double* previousRhs = fineGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.data()
    + previousStage * {{NUMBER_OF_DOFS_PER_CELL}} * {{NUMBER_OF_UNKNOWNS}};

  constexpr int FluxElementsPerFace = facedata::{{UNKNOWN_IDENTIFIER}}RiemannSolution::Cardinality / 2;
  constexpr int NodesPerCell = {{NUMBER_OF_DOFS_PER_CELL}};
  constexpr int SpaceBasisSize = NodesPerCell * {{NUMBER_OF_UNKNOWNS}};
  // Pairwise face sums preserve reflection symmetry when rounded.
  double directionUpdates[DIMENSIONS][SpaceBasisSize];
  for (int direction = 0; direction < DIMENSIONS; direction++) {
    auto* const updatedState = directionUpdates[direction];
    std::fill_n(updatedState, SpaceBasisSize, 0.0);
    {
      auto* const faceFluxEstimates =
        fineGridFaces{{UNKNOWN_IDENTIFIER}}RiemannSolution(direction).data()
        + FluxElementsPerFace;
      const auto faceOrientation = 0;
      {{FACE_INTEGRAL_CALL}}
    }
    {
      auto* const faceFluxEstimates =
        fineGridFaces{{UNKNOWN_IDENTIFIER}}RiemannSolution(
          direction + DIMENSIONS
        ).data();
      const auto faceOrientation = 1;
      {{FACE_INTEGRAL_CALL}}
    }
  }
  for (int entry = 0; entry < SpaceBasisSize; entry++) {
#if DIMENSIONS == 2
    previousRhs[entry] += directionUpdates[0][entry] + directionUpdates[1][entry];
#else
    previousRhs[entry] +=
      directionUpdates[0][entry] + directionUpdates[1][entry] + directionUpdates[2][entry];
#endif
  }
  [[maybe_unused]] const double* oldQ = fineGridCell{{UNKNOWN_IDENTIFIER}}.data();
  [[maybe_unused]] double* const dQdt = previousRhs;
  {{POSTPROCESS_UPDATED_CELL_AFTER_RUNGE_KUTTA_STEP}}

  if (repositories::{{SOLVER_INSTANCE}}.isFirstGridSweepOfTimeStep()) {
    const double previousTimeStepSize = stageTimeStepSize;
    ::exahype2::dg::rk::linearCombinationOfEstimates<
      {{RK_STEPS}}, {{DG_ORDER}}+1, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}}
    >(
      {{RK_STEPS}},
      fineGridCell{{UNKNOWN_IDENTIFIER}}.data(),
      fineGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.data(),
      fineGridCell{{UNKNOWN_IDENTIFIER}}.data(),
      kernels::{{SOLVER_NAME}}::ButcherTableau<double>::FinalEstimateWeights
    );

    const double timeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();
    double* newQ = fineGridCell{{UNKNOWN_IDENTIFIER}}.data();

    {{POSTPROCESS_UPDATED_CELL_AFTER_FINAL_LINEAR_COMBINATION}}

{% if COMPUTE_MAX_EIGENVALUE %}
    const auto eigenvalues = ::exahype2::dg::reduceMaxEigenvalues<
      {{FULL_QUALIFIED_SOLVER_NAME}}, {{DG_ORDER}}+1,
      {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}}
    >(
      newQ, kernels::{{SOLVER_NAME}}::Quadrature<double>::QuadraturePoints1D,
      marker.getX(), marker.getH(), timeStamp + previousTimeStepSize, previousTimeStepSize
    );
    const double maxPhysicalEigenvalue = eigenvalues.maxPhysicalEigenvalue;
    const double maxScaledEigenvalue   = eigenvalues.maxScaledEigenvalue;
{% endif %}

    {{COMPUTE_NEW_TIME_STEP_SIZE}}
    fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStamp(timeStamp+previousTimeStepSize);
    fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStepSize(newTimeStepSize);
    fineGridCell{{SOLVER_NAME}}CellLabel.setHasUpdated(true);
    repositories::{{SOLVER_INSTANCE}}.update(
      newTimeStepSize, timeStamp+previousTimeStepSize, marker.getH()(0)
    );
    stageTimeStepSize = newTimeStepSize;
  }

  ::exahype2::dg::rk::linearCombinationOfEstimates<
    {{RK_STEPS}}, {{DG_ORDER}}+1, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}}
  >(
    stage, fineGridCell{{UNKNOWN_IDENTIFIER}}.data(),
    fineGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.data(),
    fineGridCell{{UNKNOWN_IDENTIFIER}}LinearCombination.data(),
    kernels::{{SOLVER_NAME}}::ButcherTableau<double>::ButcherMatrix
  );

  const double stageTimeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp()
    + kernels::{{SOLVER_NAME}}::ButcherTableau<double>::RelativeTimeStepSizes[stage] * stageTimeStepSize;
  double* stageRhs = fineGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.data()
    + stage * {{NUMBER_OF_DOFS_PER_CELL}} * {{NUMBER_OF_UNKNOWNS}};
  ::exahype2::dg::rk::volumeIntegral<
    {{FULL_QUALIFIED_SOLVER_NAME}}, {{DG_ORDER}}+1,
    {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}},
    {{"true" if IS_LINEAR else "false"}},
    {% if FLUX_IMPLEMENTATION!="<none>" %}true{% else %}false{% endif %},
    {{ "true" if USE_DIFFUSIVE_FLUX else "false" }},
    {% if SOURCE_TERM_IMPLEMENTATION!="<none>" %}true{% else %}false{% endif %},
    {% if NCP_IMPLEMENTATION!="<none>" %}true{% else %}false{% endif %}
  >(
    fineGridCell{{UNKNOWN_IDENTIFIER}}LinearCombination.data(), stageRhs,
    kernels::{{SOLVER_NAME}}::Quadrature<double>::QuadraturePoints1D,
    kernels::{{SOLVER_NAME}}::DGMatrices<double>::MassMatrixDiagonal1D,
    kernels::{{SOLVER_NAME}}::DGMatrices<double>::StiffnessOperator1D,
    kernels::{{SOLVER_NAME}}::DGMatrices<double>::DerivativeOperator1D,
    marker.getX(), marker.getH(), stageTimeStamp, stageTimeStepSize
  );
  {{PROJECT_STAGE_ONTO_FACES}}
}

if (repositories::{{SOLVER_INSTANCE}}.getSolverState()=={{SOLVER_NAME}}::SolverState::TimeStepAfterGridInitialisation) {
  // Grid initialisation supplies no completed stage to combine.
  {{COMPUTE_TIME_STEP_SIZE}}
  ::exahype2::dg::rk::linearCombinationOfEstimates<
    {{RK_STEPS}}, {{DG_ORDER}}+1, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}}
  >(
    0, fineGridCell{{UNKNOWN_IDENTIFIER}}.data(),
    fineGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.data(),
    fineGridCell{{UNKNOWN_IDENTIFIER}}LinearCombination.data(),
    kernels::{{SOLVER_NAME}}::ButcherTableau<double>::ButcherMatrix
  );

  const double stageTimeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp()
    + kernels::{{SOLVER_NAME}}::ButcherTableau<double>::RelativeTimeStepSizes[0] * timeStepSize;

  double* stageRhs = fineGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.data();
  ::exahype2::dg::rk::volumeIntegral<
    {{FULL_QUALIFIED_SOLVER_NAME}}, {{DG_ORDER}}+1,
    {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}},
    {{"true" if IS_LINEAR else "false"}},
    {% if FLUX_IMPLEMENTATION!="<none>" %}true{% else %}false{% endif %},
    {{ "true" if USE_DIFFUSIVE_FLUX else "false" }},
    {% if SOURCE_TERM_IMPLEMENTATION!="<none>" %}true{% else %}false{% endif %},
    {% if NCP_IMPLEMENTATION!="<none>" %}true{% else %}false{% endif %}
  >(
    fineGridCell{{UNKNOWN_IDENTIFIER}}LinearCombination.data(), stageRhs,
    kernels::{{SOLVER_NAME}}::Quadrature<double>::QuadraturePoints1D,
    kernels::{{SOLVER_NAME}}::DGMatrices<double>::MassMatrixDiagonal1D,
    kernels::{{SOLVER_NAME}}::DGMatrices<double>::StiffnessOperator1D,
    kernels::{{SOLVER_NAME}}::DGMatrices<double>::DerivativeOperator1D,
    marker.getX(), marker.getH(), stageTimeStamp, timeStepSize
  );

  {% if COMPUTE_MAX_EIGENVALUE %}
  const auto eigenvalues = ::exahype2::dg::reduceMaxEigenvalues<
    {{FULL_QUALIFIED_SOLVER_NAME}}, {{DG_ORDER}}+1,
    {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}}
  >(
    fineGridCell{{UNKNOWN_IDENTIFIER}}.data(),
    kernels::{{SOLVER_NAME}}::Quadrature<double>::QuadraturePoints1D,
    marker.getX(),
    marker.getH(),
    stageTimeStamp,
    timeStepSize
  );
  repositories::{{SOLVER_INSTANCE}}.setMaxEigenvalue(eigenvalues.maxPhysicalEigenvalue);
  repositories::{{SOLVER_INSTANCE}}.setMaxScaledEigenvalue(eigenvalues.maxScaledEigenvalue);
  {% endif %}

  fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStepSize(timeStepSize);
  repositories::{{SOLVER_INSTANCE}}.update(
    timeStepSize,
    fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp(),
    marker.getH()(0)
  );
  {{PROJECT_STAGE_ONTO_FACES}}
}
"""

    _PROJECT = r"""
  double* faceState[peano4::FacesPerCell];
  constexpr int StateElementsPerFace =
    facedata::{{UNKNOWN_IDENTIFIER}}EstimateProjection::Cardinality / 2;
  for (int face = 0; face < peano4::FacesPerCell; face++) {
    faceState[face] = fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(face).data()
      + (face < DIMENSIONS ? StateElementsPerFace : 0);
  }
  ::exahype2::dg::extrapolateStateToFaces<
    {{DG_ORDER}}+1, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}}
  >(
    faceState,
    fineGridCell{{UNKNOWN_IDENTIFIER}}LinearCombination.data(),
    kernels::{{SOLVER_NAME}}::DGMatrices<double>::BasisFunctionValuesLeft1D,
    kernels::{{SOLVER_NAME}}::DGMatrices<double>::BasisFunctionValuesRight1D
  );

  double* projectedFaceFlux[peano4::FacesPerCell];
  constexpr int ProjectedFluxElementsPerFace =
    facedata::{{UNKNOWN_IDENTIFIER}}RiemannSolution::Cardinality / 2;
  for (int face = 0; face < peano4::FacesPerCell; face++) {
    projectedFaceFlux[face] = fineGridFaces{{UNKNOWN_IDENTIFIER}}RiemannSolution(face).data()
      + (face < DIMENSIONS ? ProjectedFluxElementsPerFace : 0);
  }
  ::exahype2::dg::extrapolateFluxToFaces<
    {{FULL_QUALIFIED_SOLVER_NAME}}, {{DG_ORDER}}+1,
    {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}},
    {% if FLUX_IMPLEMENTATION!="<none>" %}true{% else %}false{% endif %},
    {{ "true" if USE_DIFFUSIVE_FLUX else "false" }}
  >(
    projectedFaceFlux,
    fineGridCell{{UNKNOWN_IDENTIFIER}}LinearCombination.data(),
    kernels::{{SOLVER_NAME}}::DGMatrices<double>::BasisFunctionValuesLeft1D,
    kernels::{{SOLVER_NAME}}::DGMatrices<double>::BasisFunctionValuesRight1D,
    kernels::{{SOLVER_NAME}}::Quadrature<double>::QuadraturePoints1D,
    kernels::{{SOLVER_NAME}}::DGMatrices<double>::DerivativeOperator1D,
    marker.getX(), marker.getH(), stageTimeStamp,
    fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStepSize()
  );
  for (int d=0; d<DIMENSIONS; d++) {
    fineGridFaces{{SOLVER_NAME}}FaceLabel(d).setUpdated(1,true);
    fineGridFaces{{SOLVER_NAME}}FaceLabel(d+DIMENSIONS).setUpdated(0,true);
    fineGridFaces{{SOLVER_NAME}}FaceLabel(d).setUpdatedTimeStamp(1,stageTimeStamp);
    fineGridFaces{{SOLVER_NAME}}FaceLabel(d+DIMENSIONS).setUpdatedTimeStamp(0,stageTimeStamp);
    fineGridFaces{{SOLVER_NAME}}FaceLabel(d).setRequiresDataTransfer(1, false);
    fineGridFaces{{SOLVER_NAME}}FaceLabel(d+DIMENSIONS).setRequiresDataTransfer(0, false);
  }
"""

    _RIEMANN_SOLVER_CALL = r"""
{% if RIEMANN_SOLVER_IMPLEMENTATION!="<none>" %}
  {{FULL_QUALIFIED_SOLVER_NAME}}::riemannSolver(
    leftFluxEstimates,
    rightFluxEstimates,
    leftStateEstimates,
    rightStateEstimates,
    timeStamp,
    timeStepSize,
    faceCentre,
    marker.getH(),
    direction,
    {{FACE_IS_BOUNDARY}},
    {{FACE_NUMBER}}
  );
{% elif IS_LINEAR %}
  ::exahype2::dg::solveLinearRiemannProblem<
      {{FULL_QUALIFIED_SOLVER_NAME}}, {{DG_ORDER}}+1,
      {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {% if FLUX_IMPLEMENTATION!="<none>" %}true{% else %}false{% endif %},
      {% if NCP_IMPLEMENTATION!="<none>" %}true{% else %}false{% endif %},
      false
    >(
      leftFluxEstimates,
      rightFluxEstimates,
      leftStateEstimates,
      rightStateEstimates,
      kernels::{{SOLVER_NAME}}::Quadrature<double>::QuadratureWeightsOnFace,
      faceCentre,
      marker.getH(),
      timeStamp,
      timeStepSize,
      direction
    );
{% else %}
  ::exahype2::dg::solveRiemannProblem<
      {{FULL_QUALIFIED_SOLVER_NAME}}, {{DG_ORDER}}+1,
      {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {% if NCP_IMPLEMENTATION!="<none>" %}true{% else %}false{% endif %}
    >(
      leftFluxEstimates,
      rightFluxEstimates,
      leftStateEstimates,
      rightStateEstimates,
      kernels::{{SOLVER_NAME}}::Quadrature<double>::QuadratureWeightsOnFace,
      faceCentre,
      marker.getH(),
      timeStamp,
      timeStepSize,
      direction
    );
{% endif %}
"""
    _FACE_INTEGRAL_CALL = """
    ::exahype2::dg::addFaceIntegral<
      {{DG_ORDER}}+1,
      {{NUMBER_OF_UNKNOWNS}},
      {{"true" if IS_LINEAR else "false"}}>(
      updatedState,
      faceFluxEstimates,
      kernels::{{SOLVER_NAME}}::Quadrature<double>::QuadratureWeightsOnFace,
      kernels::{{SOLVER_NAME}}::Quadrature<double>::InverseQuadratureWeightsInCell,
      kernels::{{SOLVER_NAME}}::DGMatrices<double>::BasisFunctionValuesLeft1D,
      kernels::{{SOLVER_NAME}}::DGMatrices<double>::BasisFunctionValuesRight1D,
      direction,
      faceOrientation,
      1.0 / marker.getH()[direction],
      stageTimeStepSize
    );
"""

    _FACE_LAST = r"""
if (repositories::{{SOLVER_INSTANCE}}.isFirstGridSweepOfTimeStep()) {
  for (int leftRight=0; leftRight<2; leftRight++) {
    if (fineGridFace{{SOLVER_NAME}}FaceLabel.getUpdated(leftRight)) {
      fineGridFace{{SOLVER_NAME}}FaceLabel.setOldTimeStamp(leftRight, fineGridFace{{SOLVER_NAME}}FaceLabel.getNewTimeStamp(leftRight));
      fineGridFace{{SOLVER_NAME}}FaceLabel.setNewTimeStamp(leftRight, fineGridFace{{SOLVER_NAME}}FaceLabel.getUpdatedTimeStamp(leftRight));
    }
  }
}
"""

    def __init__(self, solver, cell_guard, riemann_solve_guard, face_guard):
        super().__init__(solver)
        self._cell_guard = cell_guard
        self._riemann_solve_guard = riemann_solve_guard
        self._face_guard = face_guard

    def _dictionary(self):
        d = {}
        self._solver._init_dictionary_with_default_parameters(d)
        self._solver.add_entries_to_text_replacement_dictionary(d)
        d["FACE_IS_BOUNDARY"] = (
            f"fineGridFace{self._solver._name}FaceLabel.getBoundary()"
        )
        d["FACE_NUMBER"] = "marker.getSelectedFaceNumber()"
        d["RIEMANN_SOLVER_CALL"] = jinja2.Template(self._RIEMANN_SOLVER_CALL).render(
            **d
        )
        d["FACE_INTEGRAL_CALL"] = jinja2.Template(self._FACE_INTEGRAL_CALL).render(**d)
        d["PROJECT_STAGE_ONTO_FACES"] = jinja2.Template(self._PROJECT).render(**d)
        return d

    def get_body_of_operation(self, operation_name):
        d = self._dictionary()
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME
        ):
            return (
                "if ("
                + self._riemann_solve_guard
                + ") {\n"
                + jinja2.Template(self._FACE).render(**d)
                + "\n}"
            )
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME
        ):
            return (
                "if ("
                + self._cell_guard
                + ") {\n"
                + jinja2.Template(self._CELL).render(**d)
                + "\n}"
            )
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_LAST_TIME
        ):
            return (
                "if ("
                + self._face_guard
                + ") {\n"
                + jinja2.Template(self._FACE_LAST).render(**d)
                + "\n}"
            )
        if operation_name == peano4.solversteps.ActionSet.OPERATION_END_TRAVERSAL:
            return (
                "  repositories::"
                + self._solver.get_name_of_global_instance()
                + ".reduceLocalUpdates();\n"
            )
        return ""

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        return (
            super().get_includes()
            + f'\n#include "kernels/{self._solver._name}/ButcherTableau.h"\n'
        )
