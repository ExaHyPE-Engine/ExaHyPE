# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from .AbstractRungeKuttaDGActionSet import AbstractRungeKuttaDGActionSet
from .ProjectLinearCombinationOfEstimatesOntoFaces import FaceProjections
from .ProjectLinearCombinationOfEstimatesOntoFaces import (
    compute_number_of_face_projection_quantities,
)

import jinja2
import peano4

from exahype2.solvers.ButcherTableau import ButcherTableau


class ComputeFinalLinearCombination(AbstractRungeKuttaDGActionSet):
    r"""!
    Forms the final Runge-Kutta state and refreshes its face projections.

    For @f$ \partial Q = F(Q) @f$, the final state is

    @f$ Q^{new} = Q^{old} + \sum_i a_i \Delta t F_i(Q). @f$

    The cell array holds @f$ Q^{old} @f$ on entry and is updated in place. The
    stage estimates cover the unknowns only, so the auxiliary variables stay
    unchanged.
    """

    _Template_Project = """
  if ({{PREDICATE}}) {
    {{COMPUTE_TIME_STEP_SIZE}}

#if DIMENSIONS == 2
    constexpr int NumberOfDofsPerCell = ({{ORDER}} + 1) * ({{ORDER}} + 1);
#else
    constexpr int NumberOfDofsPerCell =
      ({{ORDER}} + 1) * ({{ORDER}} + 1) * ({{ORDER}} + 1);
#endif
    constexpr int NumberOfValuesPerDof =
      {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
    auto* const updatedState = fineGridCell{{UNKNOWN_IDENTIFIER}}.data();
    const auto* const rightHandSideEstimates =
      fineGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.data();

    peano4::dForEach(
      {{ORDER}} + 1,
      [&](const tarch::la::Vector<DIMENSIONS, int>& degreeOfFreedom) {
        const auto serialisedDegreeOfFreedom =
          peano4::dLinearised(degreeOfFreedom, {{ORDER}} + 1);
        const auto solutionOffset =
          serialisedDegreeOfFreedom * NumberOfValuesPerDof;
        for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}}; unknown++) {
{% for weight_index in range(0, WEIGHTS | length) %}
          updatedState[solutionOffset + unknown] +=
            timeStepSize * {{WEIGHTS[weight_index]}}
            * rightHandSideEstimates[
              ({{weight_index}} * NumberOfDofsPerCell
               + serialisedDegreeOfFreedom)
                * {{NUMBER_OF_UNKNOWNS}}
              + unknown
            ];
{% endfor %}
        }
      }
    );

    const auto timeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();

    // The postprocessing snippet reads the updated state through newQ.
    [[maybe_unused]] auto* const newQ = updatedState;
    {{POSTPROCESS_UPDATED_CELL_AFTER_FINAL_LINEAR_COMBINATION}}

{% if COMPUTE_MAX_EIGENVALUE %}
    auto cellData = ::exahype2::CellData<double, double>(
      nullptr,
      marker.getX(),
      marker.getH(),
      timeStamp,
      timeStepSize,
      updatedState
    );

    const auto maxPhysicalEigenvalue =
      ::exahype2::dg::reduceMaxScaledEigenvalue_patchwise_functors(
        cellData,
        {{ORDER}},
        {{NUMBER_OF_UNKNOWNS}},
        {{NUMBER_OF_AUXILIARY_VARIABLES}},
        [](
          const double* const NOALIAS state,
          const tarch::la::Vector<DIMENSIONS, double>& position,
          const double evaluationTimeStamp,
          const double evaluationTimeStepSize,
          const int normal
        ) -> double {
          return repositories::{{SOLVER_INSTANCE}}.maxEigenvalue(
            state,
            position,
            evaluationTimeStamp,
            evaluationTimeStepSize,
            normal
          );
        },
        repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1d
      );

    const auto maxScaledEigenvalue = cellData.maxEigenvalue[0];
{% endif %}

    // The time-step snippet provides newTimeStepSize for the cell label.
    {{COMPUTE_NEW_TIME_STEP_SIZE}}

    fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStamp(timeStamp + timeStepSize);
    fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStepSize(newTimeStepSize);
    fineGridCell{{SOLVER_NAME}}CellLabel.setHasUpdated(true);
    repositories::{{SOLVER_INSTANCE}}.update(
      timeStepSize,
      timeStamp + timeStepSize,
      marker.getH()(0)
    );

{% if NUMBER_OF_FACE_PROJECTIONS != 1 %}
#error "The final projection supports one face projection."
{% endif %}

#if DIMENSIONS == 2
    auto* const leftFaceProjection =
      fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(0).data();
    auto* const rightFaceProjection =
      fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(2).data();
    auto* const bottomFaceProjection =
      fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(1).data();
    auto* const topFaceProjection =
      fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(3).data();
    ::exahype2::dg::projectVolumetricDataOntoFaces(
      updatedState,
      {{ORDER}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      repositories::{{SOLVER_INSTANCE}}.BasisFunctionValuesLeft1d,
      leftFaceProjection,
      rightFaceProjection,
      bottomFaceProjection,
      topFaceProjection
    );
#elif DIMENSIONS == 3
    auto* const leftFaceProjection =
      fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(0).data();
    auto* const rightFaceProjection =
      fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(3).data();
    auto* const bottomFaceProjection =
      fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(1).data();
    auto* const topFaceProjection =
      fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(4).data();
    auto* const frontFaceProjection =
      fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(2).data();
    auto* const backFaceProjection =
      fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(5).data();
    ::exahype2::dg::projectVolumetricDataOntoFaces(
      updatedState,
      {{ORDER}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      repositories::{{SOLVER_INSTANCE}}.BasisFunctionValuesLeft1d,
      leftFaceProjection,
      rightFaceProjection,
      bottomFaceProjection,
      topFaceProjection,
      frontFaceProjection,
      backFaceProjection
    );
#endif
  }
"""

    _Template_UpdateFace = """
  if (repositories::{{SOLVER_INSTANCE}}.isLastGridSweepOfTimeStep()) {
    for (int facePosition = 0; facePosition < 2; facePosition++) {
      if (fineGridFace{{SOLVER_NAME}}FaceLabel.getUpdated(facePosition)) {
        fineGridFace{{SOLVER_NAME}}FaceLabel.setOldTimeStamp(
          facePosition,
          fineGridFace{{SOLVER_NAME}}FaceLabel.getNewTimeStamp(facePosition)
        );
        fineGridFace{{SOLVER_NAME}}FaceLabel.setNewTimeStamp(
          facePosition,
          fineGridFace{{SOLVER_NAME}}FaceLabel.getUpdatedTimeStamp(facePosition)
        );

        ::exahype2::dg::copyOneSideOfFaceProjection(
          {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}},
          {{ORDER}},
          {{NUMBER_OF_FACE_PROJECTIONS}},
          marker.getSelectedFaceNumber() % DIMENSIONS,
          facePosition == 1,
          fineGridFace{{UNKNOWN_IDENTIFIER}}EstimateProjection.data(),
          fineGridFace{{UNKNOWN_IDENTIFIER}}OldSolutionProjection.data()
        );
      }
    }
  }
"""

    def __init__(self, solver, guard, face_projections: FaceProjections):
        """``guard`` selects the cells to update, and ``face_projections`` has
        to be the solution projection.
        """
        super().__init__(solver)
        self._guard = guard
        self._butcher_tableau = ButcherTableau(self._solver._rk_order)
        self._face_projections = face_projections
        assert (
            self._face_projections == FaceProjections.Solution
        ), "Only the solution face projection is supported."

    @property
    def guard(self):
        return self._guard

    @guard.setter
    def guard(self, guard):
        self._guard = guard

    def get_body_of_operation(self, operation_name):
        result = ""
        dictionary = {}
        self._solver._init_dictionary_with_default_parameters(dictionary)
        self._solver.add_entries_to_text_replacement_dictionary(dictionary)
        dictionary["PREDICATE"] = self._guard
        dictionary["WEIGHTS"] = self._butcher_tableau.final_estimate_weights()
        dictionary["NUMBER_OF_FACE_PROJECTIONS"] = (
            compute_number_of_face_projection_quantities(self._face_projections)
        )
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME
        ):
            result = jinja2.Template(self._Template_Project).render(**dictionary)
        elif (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_LAST_TIME
        ):
            result = jinja2.Template(self._Template_UpdateFace).render(**dictionary)
        elif operation_name == peano4.solversteps.ActionSet.OPERATION_END_TRAVERSAL:
            result = (
                "  repositories::"
                + self._solver.get_name_of_global_instance()
                + ".reduceLocalUpdates();\n"
            )
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        return (
            super().get_includes()
            + """
#include "peano4/Loop.h"
"""
        )
