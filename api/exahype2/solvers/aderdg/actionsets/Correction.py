# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractADERDGActionSet import AbstractADERDGActionSet

import jinja2
import peano4.solversteps


class Correction(AbstractADERDGActionSet):
    _Template_SolveRiemannProblem = """
  if ({{FACE_GUARD}}) {
    const auto faceNumber = marker.getSelectedFaceNumber();
    const auto direction = faceNumber % DIMENSIONS;
    const auto isBoundary =
      fineGridFace{{SOLVER_NAME}}FaceLabel.getBoundary()
      and not {{FULL_QUALIFIED_SOLVER_NAME}}::PeriodicBC[direction];
    const auto interiorFacePosition = faceNumber < DIMENSIONS ? 1 : 0;
    const auto timeStamp = isBoundary
      ? fineGridFace{{SOLVER_NAME}}FaceLabel.getNewTimeStamp(interiorFacePosition)
{% if HANGING_FACE %}
      : std::max(
          coarseGridFaces{{SOLVER_NAME}}FaceLabel(faceNumber).getNewTimeStamp(
            interiorFacePosition
          ),
          fineGridFace{{SOLVER_NAME}}FaceLabel.getNewTimeStamp(
            1 - interiorFacePosition
          )
        );
{% else %}
      : std::max(
          fineGridFace{{SOLVER_NAME}}FaceLabel.getNewTimeStamp(0),
          fineGridFace{{SOLVER_NAME}}FaceLabel.getNewTimeStamp(1)
        );
{% endif %}

    {{COMPUTE_TIME_STEP_SIZE}}

    constexpr std::size_t FluxElementsPerFace =
      facedata::{{SOLVER_NAME}}QFluxEstimates::Cardinality / 2;
    constexpr std::size_t StateElementsPerFace =
      facedata::{{SOLVER_NAME}}QEstimates::Cardinality / 2;
    constexpr std::size_t NumberOfUnknowns = {{NUMBER_OF_UNKNOWNS}};
    constexpr std::size_t NumberOfValues =
      NumberOfUnknowns + {{NUMBER_OF_AUXILIARY_VARIABLES}};
    constexpr int NumberOfDofsPerAxis = {{ORDER}} + 1;
    auto* const leftFluxEstimates =
      fineGridFace{{SOLVER_NAME}}QFluxEstimates.data();
    auto* const rightFluxEstimates = leftFluxEstimates + FluxElementsPerFace;
    auto* const leftStateEstimates = fineGridFace{{SOLVER_NAME}}QEstimates.data();
    auto* const rightStateEstimates = leftStateEstimates + StateElementsPerFace;
    const auto& faceCentre = marker.getX();

    if (isBoundary) {
      const auto isNegativeBoundary = faceNumber < DIMENSIONS;
      const auto* const insideStateEstimates =
        isNegativeBoundary ? rightStateEstimates : leftStateEstimates;
      auto* const outsideStateEstimates =
        isNegativeBoundary ? leftStateEstimates : rightStateEstimates;
      auto* const outsideFluxEstimates =
        isNegativeBoundary ? leftFluxEstimates : rightFluxEstimates;

{% if USE_FLUX and USE_DIFFUSIVE_FLUX %}
      const auto inverseNormalCellSize = 1.0 / marker.getH()(direction);
{% endif %}

      peano4::dForEachExclusive(
        NumberOfDofsPerAxis,
        direction,
        0,
        [&](const tarch::la::Vector<DIMENSIONS, int>& degreeOfFreedom) {
          auto nodePosition = faceCentre - 0.5 * marker.getH();
          nodePosition(direction) = faceCentre(direction);

          std::size_t serialisedDegreeOfFreedom = 0;
          std::size_t stride = 1;
          for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
            if (dimension != direction) {
              nodePosition(dimension) +=
                kernels::{{SOLVER_NAME}}::Quadrature<{{SOLUTION_STORAGE_PRECISION}}>::nodes[
                  degreeOfFreedom(dimension)
                ]
                * marker.getH()(dimension);
              serialisedDegreeOfFreedom +=
                static_cast<std::size_t>(degreeOfFreedom(dimension)) * stride;
              stride *= static_cast<std::size_t>(NumberOfDofsPerAxis);
            }
          }

          const auto dataOffset = serialisedDegreeOfFreedom * NumberOfValues;
          const auto fluxOffset = serialisedDegreeOfFreedom * NumberOfUnknowns;

{% if STATELESS_BOUNDARY_CONDITIONS %}
          {{FULL_QUALIFIED_SOLVER_NAME}}::boundaryConditions(
{% else %}
          repositories::{{SOLVER_INSTANCE}}.boundaryConditions(
{% endif %}
            insideStateEstimates + dataOffset,
            outsideStateEstimates + dataOffset,
            nodePosition,
            marker.getH(),
            timeStamp,
            direction
          );

{% if USE_FLUX %}
{% if USE_DIFFUSIVE_FLUX %}
          {{CORRECTOR_COMPUTATION_PRECISION}} normalStateGradient[NumberOfUnknowns]{};
          const {{CORRECTOR_COMPUTATION_PRECISION}} zeroStateGradient[NumberOfUnknowns]{};
          const {{CORRECTOR_COMPUTATION_PRECISION}}* stateGradients[DIMENSIONS]{
            zeroStateGradient,
            zeroStateGradient,
#if DIMENSIONS == 3
            zeroStateGradient
#endif
          };
          stateGradients[direction] = normalStateGradient;
          for (std::size_t unknown = 0; unknown < NumberOfUnknowns; unknown++) {
            normalStateGradient[unknown] =
              (rightStateEstimates[dataOffset + unknown]
               - leftStateEstimates[dataOffset + unknown])
              * inverseNormalCellSize;
          }
{% endif %}
{% if STATELESS_FLUX %}
          {{FULL_QUALIFIED_SOLVER_NAME}}::flux(
{% else %}
          repositories::{{SOLVER_INSTANCE}}.flux(
{% endif %}
            outsideStateEstimates + dataOffset,
{% if USE_DIFFUSIVE_FLUX %}
            stateGradients,
{% endif %}
            nodePosition,
            marker.getH(),
            timeStamp,
            timeStepSize,
            direction,
            outsideFluxEstimates + fluxOffset
          );
{% else %}
          std::fill_n(
            outsideFluxEstimates + fluxOffset,
            NumberOfUnknowns,
            0.0
          );
{% endif %}
        }
      );
    }

    {{RIEMANN_SOLVER_CALL}}

{% if not HANGING_FACE %}
    fineGridFace{{SOLVER_NAME}}FaceLabel.setUpdated(0, true);
    fineGridFace{{SOLVER_NAME}}FaceLabel.setUpdated(1, true);
{% endif %}
  }
"""

    _Template_SynchroniseHangingFaceLabels = """
  if ({{HANGING_FACE_GUARD}}) {
    const auto faceNumber = marker.getSelectedFaceNumber();
    fineGridFace{{SOLVER_NAME}}FaceLabel.setNewTimeStamp(
      0,
      std::max(
        coarseGridFaces{{SOLVER_NAME}}FaceLabel(faceNumber).getNewTimeStamp(0),
        fineGridFace{{SOLVER_NAME}}FaceLabel.getNewTimeStamp(0)
      )
    );
    fineGridFace{{SOLVER_NAME}}FaceLabel.setNewTimeStamp(
      1,
      std::max(
        coarseGridFaces{{SOLVER_NAME}}FaceLabel(faceNumber).getNewTimeStamp(1),
        fineGridFace{{SOLVER_NAME}}FaceLabel.getNewTimeStamp(1)
      )
    );
    fineGridFace{{SOLVER_NAME}}FaceLabel.setOldTimeStamp(
      0,
      std::max(
        coarseGridFaces{{SOLVER_NAME}}FaceLabel(faceNumber).getOldTimeStamp(0),
        fineGridFace{{SOLVER_NAME}}FaceLabel.getOldTimeStamp(0)
      )
    );
    fineGridFace{{SOLVER_NAME}}FaceLabel.setOldTimeStamp(
      1,
      std::max(
        coarseGridFaces{{SOLVER_NAME}}FaceLabel(faceNumber).getOldTimeStamp(1),
        fineGridFace{{SOLVER_NAME}}FaceLabel.getOldTimeStamp(1)
      )
    );
  }
"""

    _Template_TouchCellFirstTime = """
  if ({{CELL_GUARD}}) {
    const auto timeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();
    // The predictor and corrector advance the same time step.
    const auto timeStepSize =
      fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStepSize();

    auto* const committedState = fineGridCell{{UNKNOWN_IDENTIFIER}}.data();
    constexpr std::size_t FluxElementsPerFace =
      facedata::{{SOLVER_NAME}}QFluxEstimates::Cardinality / 2;
    constexpr int NodesPerCell = {{NUMBER_OF_DOF*NUMBER_OF_DOF*NUMBER_OF_DOF_3D}};
    constexpr int SpaceBasisSize = NodesPerCell * {{NUMBER_OF_DATA}};

    // Each direction's two faces accumulate into a direction buffer, and the
    // buffers combine through a commutative pairwise sum. A fixed face order
    // would tie the committed state's rounding to the axis order, so two
    // mirror-image cells would drift apart in the last bits and knife-edge
    // admissibility verdicts would flip one-sided.
    {{SOLUTION_STORAGE_PRECISION}} directionUpdates[DIMENSIONS][SpaceBasisSize];
    // Hanging-face Riemann data resides on the side that sent it.
    for (int direction = 0; direction < DIMENSIONS; direction++) {
      auto* const updatedState = directionUpdates[direction];
      std::fill_n(
        updatedState,
        SpaceBasisSize,
        static_cast<{{SOLUTION_STORAGE_PRECISION}}>(0)
      );
      {
        auto* const faceFluxEstimates =
          fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(direction).data()
          + (fineGridFaces{{SOLVER_NAME}}FaceLabel(direction).getAboveHanging()[0]
               ? 0
               : FluxElementsPerFace);
        const auto faceOrientation = 0;
        {{FACE_INTEGRAL_CALL}}
      }
      {
        auto* const faceFluxEstimates =
          fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(
            direction + DIMENSIONS
          ).data()
          + (fineGridFaces{{SOLVER_NAME}}FaceLabel(
               direction + DIMENSIONS
             ).getAboveHanging()[1]
               ? FluxElementsPerFace
               : 0);
        const auto faceOrientation = 1;
        {{FACE_INTEGRAL_CALL}}
      }
    }
    for (int node = 0; node < NodesPerCell; node++) {
      for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}}; unknown++) {
        const auto entry = node * {{NUMBER_OF_DATA}} + unknown;
#if DIMENSIONS == 2
        committedState[entry] +=
          directionUpdates[0][entry] + directionUpdates[1][entry];
#else
        committedState[entry] +=
          directionUpdates[0][entry] + directionUpdates[1][entry]
          + directionUpdates[2][entry];
#endif
      }
    }

    // A postprocessing snippet addresses the updated patch as QNew.
    [[maybe_unused]] auto* const QNew = committedState;
    {{POSTPROCESS_UPDATED_PATCH}}

    {{COMPUTE_NEW_TIME_STEP_SIZE}}

    fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStamp(timeStamp + timeStepSize);
    fineGridCell{{SOLVER_NAME}}CellLabel.setHasUpdated(true);

    repositories::{{SOLVER_INSTANCE}}
      .update(
        timeStepSize,
        timeStamp + timeStepSize,
        tarch::la::min(marker.getH()),
        tarch::la::max(marker.getH()),
        false
      );
  }
"""

    def __init__(self, solver, cell_guard, face_guard, hanging_face_guard):
        super().__init__(solver)
        self._cell_guard = cell_guard
        self._face_guard = face_guard
        self._hanging_face_guard = hanging_face_guard

    def _add_action_set_entries_to_dictionary(self, dictionary):
        dictionary["CELL_GUARD"] = self._cell_guard
        # The correction is provisional while a limiter runs, so the step it would
        # propose rests on candidates the flagging may still reject. The flagging
        # and the recompute report the wave speeds of the cells they accept.
        if self._solver.subcell_limiter_is_enabled:
            dictionary["COMPUTE_NEW_TIME_STEP_SIZE"] = ""

    def get_body_of_operation(self, operation_name):
        result = ""
        dictionary = {}
        self._solver._init_dictionary_with_default_parameters(dictionary)
        self._solver.add_entries_to_text_replacement_dictionary(dictionary)
        self._add_action_set_entries_to_dictionary(dictionary)
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME
        ):
            dictionary["FACE_GUARD"] = self._face_guard
            dictionary["HANGING_FACE"] = False
            result = jinja2.Template(self._Template_SolveRiemannProblem).render(
                **dictionary
            )
        elif (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_HANGING_FACE_FIRST_TIME
        ):
            dictionary["FACE_GUARD"] = (
                "(" + self._hanging_face_guard + ")"
                " and not marker.isInteriorFaceWithinPatch()"
            )
            dictionary["HANGING_FACE_GUARD"] = self._hanging_face_guard
            dictionary["HANGING_FACE"] = True
            result = jinja2.Template(self._Template_SolveRiemannProblem).render(
                **dictionary
            )
            result += jinja2.Template(
                self._Template_SynchroniseHangingFaceLabels
            ).render(**dictionary)
        elif (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME
        ):
            result = jinja2.Template(self._Template_TouchCellFirstTime).render(
                **dictionary
            )
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        return super().get_includes()
