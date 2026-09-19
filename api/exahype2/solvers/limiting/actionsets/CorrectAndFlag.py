# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractLimiterActionSet import AbstractLimiterActionSet
from ..Labels import (
    cell_status_to_face_snippet,
    hanging_face_status_from_coarse_snippet,
    hanging_face_status_reduce_snippet,
    hanging_vertex_dmp_extrema_from_coarse_snippet,
    reset_refined_face_status_snippet,
    vertex_dmp_extrema_merge_snippet,
)
from ..ProjectionMatrices import create_subcell_projection_matrices

from exahype2.solvers.LagrangeBasis import render_tensor_1

import jinja2
import peano4.solversteps


class CorrectAndFlag(AbstractLimiterActionSet):
    """Tests candidate solutions and marks inadmissible cells as troubled."""

    _Template_TouchCellFirstTime = """
  if ({{CELL_GUARD}}) {
    [[maybe_unused]] constexpr int DataPerNode = {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
#if DIMENSIONS == 2
    constexpr int NumberOfNodes = ({{ORDER}} + 1) * ({{ORDER}} + 1);
#else
    constexpr int NumberOfNodes = ({{ORDER}} + 1) * ({{ORDER}} + 1) * ({{ORDER}} + 1);
#endif
    constexpr int NumberOfDofsPerAxis     = {{ORDER}} + 1;
    constexpr int NumberOfSubcellsPerAxis = 2 * {{ORDER}} + 1;
#if DIMENSIONS == 2
    constexpr int NumberOfSubcells = NumberOfSubcellsPerAxis * NumberOfSubcellsPerAxis;
#else
    constexpr int NumberOfSubcells = NumberOfSubcellsPerAxis * NumberOfSubcellsPerAxis * NumberOfSubcellsPerAxis;
#endif
    const auto* const candidate      = fineGridCell{{UNKNOWN_IDENTIFIER}}.data();
    using Shortcuts [[maybe_unused]] = {{FULL_QUALIFIED_SOLVER_NAME}}::Shortcuts;

    // The subcell scheme starts from the subcell averages of an accepted
    // candidate, so they pass the admissibility test and bound the time step
    // like the nodes.
    static constexpr {{SOLUTION_STORAGE_PRECISION}} DgToFvMatrix[] = {{DG_TO_FV_MATRIX}};
    {{SOLUTION_STORAGE_PRECISION}} candidateSubcells[NumberOfSubcells * DataPerNode];
    ::exahype2::dg::projectTensorProduct<DIMENSIONS, NumberOfDofsPerAxis, NumberOfSubcellsPerAxis, DataPerNode>(
      candidate, candidateSubcells, DgToFvMatrix
    );

    bool admissible = true;
    // A cell whose space-time predictor diverged holds no valid candidate, so it
    // is troubled regardless of the admissibility checks (Dumbser, Zanotti,
    // Loubere and Diot, J. Comput. Phys. 278 (2014) 47-75).
    if (fineGridCell{{SOLVER_NAME}}CellLabel.getPredictorDiverged()) {
      admissible = false;
    }
{% if USES_ADMISSIBILITY_CRITERION %}
    const auto isPhysicallyAdmissible = [&marker](
      [[maybe_unused]] const {{SOLUTION_STORAGE_PRECISION}}* const Q,
      [[maybe_unused]] const int numberOfStates
    ) -> bool {
      [[maybe_unused]] constexpr int NumberOfUnknowns           = {{NUMBER_OF_UNKNOWNS}};
      [[maybe_unused]] constexpr int NumberOfAuxiliaryVariables = {{NUMBER_OF_AUXILIARY_VARIABLES}};
      [[maybe_unused]] const auto    x                          = marker.getX();
      [[maybe_unused]] const auto    h                          = marker.getH();
      {{PHYSICAL_ADMISSIBILITY_CRITERION}}
    };
    if (not isPhysicallyAdmissible(candidate, NumberOfNodes)
        or not isPhysicallyAdmissible(candidateSubcells, NumberOfSubcells)) {
      admissible = false;
    }
{% endif %}
{% if USES_DMP %}
    constexpr int NumberOfObservables = {{NUMBER_OF_DMP_OBSERVABLES}};

    const auto mapStateToObservables = [](
      const {{SOLUTION_STORAGE_PRECISION}}* const Q,
      {{SOLUTION_STORAGE_PRECISION}}* const observables
    ) {
      {{DMP_OBSERVABLES}}
    };
    // A polynomial can attain extrema between its storage nodes. Lobatto samples
    // cover the cell boundary, while subcell averages cover the fallback grid.
    static constexpr {{SOLUTION_STORAGE_PRECISION}} DgToLobattoMatrix[] = {{DMP_DG_TO_LOBATTO_MATRIX}};
    static constexpr {{SOLUTION_STORAGE_PRECISION}} DgToSubcellMatrix[] = {{DMP_DG_TO_SUBCELL_MATRIX}};

    {{SOLUTION_STORAGE_PRECISION}} candidateMinimum[NumberOfObservables];
    {{SOLUTION_STORAGE_PRECISION}} candidateMaximum[NumberOfObservables];
{{CANDIDATE_EXTREMA_REDUCTION}}

    // The corrector has not touched the predictor backup, so it still holds the
    // accepted previous solution.
    const auto* const previousSolution = fineGridCell{{UNKNOWN_IDENTIFIER}}_old.data();
    {{SOLUTION_STORAGE_PRECISION}} referenceMinimum[NumberOfObservables];
    {{SOLUTION_STORAGE_PRECISION}} referenceMaximum[NumberOfObservables];
{{REFERENCE_EXTREMA_REDUCTION}}
    // The previous-step ranges of every cell adjacent to a node of this one widen
    // the reference, the neighbourhood V_i of Dumbser, Zanotti, Loubere and Diot,
    // arXiv 1406.7416, equation 23.
{{NEIGHBOUR_EXTREMA_MERGE}}

{% if DMP_RELAXATION_PARAMETER_IMPLEMENTATION %}
    const auto relaxationParameterForObservable = [](
      [[maybe_unused]] const int observable,
      [[maybe_unused]] const {{SOLUTION_STORAGE_PRECISION}} specifiedRelaxationParameter
    ) -> {{SOLUTION_STORAGE_PRECISION}} {
      {{DMP_RELAXATION_PARAMETER_IMPLEMENTATION}}
    };
{% endif %}
    for (int observableNumber = 0; observableNumber < NumberOfObservables; observableNumber++) {
      const auto observableInRange =
        ::exahype2::dg::satisfiesDiscreteMaximumPrinciple(
          candidateMinimum[observableNumber],
          candidateMaximum[observableNumber],
          referenceMinimum[observableNumber],
          referenceMaximum[observableNumber],
{% if DMP_RELAXATION_PARAMETER_IMPLEMENTATION %}
          relaxationParameterForObservable(
            observableNumber,
            static_cast<{{SOLUTION_STORAGE_PRECISION}}>({{DMP_RELAXATION_PARAMETER}})
          ),
{% else %}
          static_cast<{{SOLUTION_STORAGE_PRECISION}}>({{DMP_RELAXATION_PARAMETER}}),
{% endif %}
          static_cast<{{SOLUTION_STORAGE_PRECISION}}>({{DMP_DIFFERENCES_SCALING}})
        );
      if (not observableInRange) {
        admissible = false;
      }
    }
{% endif %}
    const auto remainsOnSubcellScheme = fineGridCell{{SOLVER_NAME}}CellLabel.getWasTroubledInPreviousStep();
    // A new rejection or its directly limited ring sets the bit again for the
    // following step.
    fineGridCell{{SOLVER_NAME}}CellLabel.setWasTroubledInPreviousStep(false);
{% if IS_ADAPTIVE %}
    // A cell that stays on the subcell scheme discards its candidate, so the
    // recompute reports its wave speed. Its previous status makes this verdict
    // independent of the order a traversal visits neighbours in.
    if (admissible and not remainsOnSubcellScheme
        and fineGridCell{{SOLVER_NAME}}CellLabel.getPreviousLimiterStatus() == ::exahype2::dg::LimiterStatus::Regular) {
      const double timeStamp      = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();
      const double timeStepSize   = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStepSize();
      const auto   maxEigenvalues = ::exahype2::dg::reduceMaxEigenvalues<
        {{FULL_QUALIFIED_SOLVER_NAME}}, {{DG_ORDER}} + 1, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}}
      >(
        candidate,
        kernels::{{SOLVER_NAME}}::Quadrature<{{SOLUTION_STORAGE_PRECISION}}>::QuadraturePoints1D,
        marker.getX(),
        marker.getH(),
        timeStamp,
        timeStepSize
      );
      repositories::{{SOLVER_INSTANCE}}.setMaxEigenvalue(maxEigenvalues.maxPhysicalEigenvalue);
      repositories::{{SOLVER_INSTANCE}}.setMaxScaledEigenvalue(maxEigenvalues.maxScaledEigenvalue);

      ::exahype2::dg::MaxEigenvalueStatistics subcellEigenvalues;
      for (int subcell = 0; subcell < NumberOfSubcells; subcell++) {
        tarch::la::Vector<DIMENSIONS, double> subcellSize;
        tarch::la::Vector<DIMENSIONS, double> subcellCentre;
        int                                   remainder = subcell;
        for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
          subcellSize(dimension)   = marker.getH()(dimension) / NumberOfSubcellsPerAxis;
          subcellCentre(dimension) = marker.getX()(dimension) - 0.5 * marker.getH()(dimension)
                                     + (remainder % NumberOfSubcellsPerAxis + 0.5) * subcellSize(dimension);
          remainder /= NumberOfSubcellsPerAxis;
        }
        double subcellState[DataPerNode];
        for (int entry = 0; entry < DataPerNode; entry++) {
          subcellState[entry] = static_cast<double>(candidateSubcells[subcell * DataPerNode + entry]);
        }
        subcellEigenvalues.accumulate<DIMENSIONS>(
          marker.getH(),
          [&](const int normal) {
            return {{FULL_QUALIFIED_SOLVER_NAME}}::maxEigenvalue(
              subcellState, subcellCentre, subcellSize, timeStamp, timeStepSize, normal
            );
          }
        );
      }
      repositories::{{SOLVER_INSTANCE}}.setMaxEigenvalue(subcellEigenvalues.maxPhysicalEigenvalue);
      repositories::{{SOLVER_INSTANCE}}.setMaxScaledEigenvalue(subcellEigenvalues.maxScaledEigenvalue);
    }
{% endif %}
    if (not admissible) {
      fineGridCell{{SOLVER_NAME}}CellLabel.setLimiterStatus(::exahype2::dg::LimiterStatus::Troubled);
      fineGridCell{{SOLVER_NAME}}CellLabel.setWasTroubledInPreviousStep(true);
    } else if (remainsOnSubcellScheme) {
      fineGridCell{{SOLVER_NAME}}CellLabel.setLimiterStatus(::exahype2::dg::LimiterStatus::Limited);
    }
    if (not admissible or remainsOnSubcellScheme) {
      repositories::{{SOLVER_INSTANCE}}.markCellAsTroubled();
    }
{{PROJECT_STATUS_TO_FACES}}
  }
"""

    _Template_TouchFaceFirstTime = """
  if ({{SWEEP_GUARD}}) {
{{RESET_REFINED_FACE_STATUS}}
  }
"""

    _Template_TouchHangingFaceFirstTime = """
  if ({{SWEEP_GUARD}}) {
{{HANGING_FACE_STATUS_FROM_COARSE}}
  }
"""

    _Template_TouchHangingFaceLastTime = """
  if ({{SWEEP_GUARD}}) {
{{HANGING_FACE_STATUS_REDUCE}}
  }
"""

    _Template_TouchHangingVertexFirstTime = """
{% if USES_DMP %}
  if ({{SWEEP_GUARD}}) {
{{HANGING_VERTEX_DMP_EXTREMA_FROM_COARSE}}
  }
{% endif %}
"""

    def __init__(self, solver, cell_guard, sweep_guard):
        super().__init__(solver)
        self._cell_guard = cell_guard
        self._sweep_guard = sweep_guard

    def _add_action_set_entries_to_dictionary(self, dictionary):
        dictionary["CELL_GUARD"] = self._cell_guard
        dictionary["SWEEP_GUARD"] = self._sweep_guard
        dictionary["RESET_REFINED_FACE_STATUS"] = reset_refined_face_status_snippet(
            self._solver._name
        )
        dictionary["HANGING_FACE_STATUS_FROM_COARSE"] = (
            hanging_face_status_from_coarse_snippet(self._solver._name)
        )
        dictionary["HANGING_FACE_STATUS_REDUCE"] = hanging_face_status_reduce_snippet(
            self._solver._name
        )
        dictionary["USES_DMP"] = self._solver._subcell_limiter_uses_dmp
        dictionary["USES_ADMISSIBILITY_CRITERION"] = (
            self._solver._subcell_limiter_uses_admissibility_criterion
        )
        dictionary["NUMBER_OF_DMP_OBSERVABLES"] = (
            self._solver._subcell_limiter_number_of_dmp_observables
        )
        dictionary["DMP_OBSERVABLES"] = self._rendered_dmp_observables(dictionary)
        dg_to_fv, _ = create_subcell_projection_matrices(self._solver._basis)
        dictionary["DG_TO_FV_MATRIX"] = render_tensor_1(
            [value for row in dg_to_fv for value in row]
        )
        if dictionary["USES_DMP"]:
            self._add_dmp_sampling_matrices_to_dictionary(dictionary)
            dictionary["CANDIDATE_EXTREMA_REDUCTION"] = (
                self._dmp_extrema_reduction_snippet(
                    "candidate", "candidateMinimum", "candidateMaximum", dictionary
                )
            )
            dictionary["REFERENCE_EXTREMA_REDUCTION"] = (
                self._dmp_extrema_reduction_snippet(
                    "previousSolution",
                    "referenceMinimum",
                    "referenceMaximum",
                    dictionary,
                )
            )
            number_of_observables = (
                self._solver._subcell_limiter_number_of_dmp_observables
            )
            dictionary["NEIGHBOUR_EXTREMA_MERGE"] = vertex_dmp_extrema_merge_snippet(
                self._solver._name,
                number_of_observables,
                "referenceMinimum",
                "referenceMaximum",
                dictionary["SOLUTION_STORAGE_PRECISION"],
            )
            dictionary["HANGING_VERTEX_DMP_EXTREMA_FROM_COARSE"] = (
                hanging_vertex_dmp_extrema_from_coarse_snippet(
                    self._solver._name, number_of_observables
                )
            )
        dictionary["DMP_RELAXATION_PARAMETER"] = (
            self._solver._subcell_limiter_dmp_relaxation_parameter
        )
        dictionary["DMP_RELAXATION_PARAMETER_IMPLEMENTATION"] = (
            self._dmp_relaxation_parameter_implementation(dictionary)
        )
        dictionary["DMP_DIFFERENCES_SCALING"] = (
            self._solver._subcell_limiter_dmp_differences_scaling
        )
        dictionary["PHYSICAL_ADMISSIBILITY_CRITERION"] = (
            self._physical_admissibility_criterion(dictionary)
        )
        dictionary["PROJECT_STATUS_TO_FACES"] = cell_status_to_face_snippet(
            self._solver._name
        )
        # Only an adaptive solver reduces an admissible cell's wave speed here.
        dictionary["IS_ADAPTIVE"] = bool(
            self._solver._compute_new_time_step_size.strip()
        )

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
            result = jinja2.Template(self._Template_TouchFaceFirstTime).render(
                **dictionary
            )
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_HANGING_FACE_FIRST_TIME
        ):
            result = jinja2.Template(self._Template_TouchHangingFaceFirstTime).render(
                **dictionary
            )
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_HANGING_FACE_LAST_TIME
        ):
            result = jinja2.Template(self._Template_TouchHangingFaceLastTime).render(
                **dictionary
            )
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_HANGING_VERTEX_FIRST_TIME
        ):
            result = jinja2.Template(self._Template_TouchHangingVertexFirstTime).render(
                **dictionary
            )
        if (
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
        includes = (
            super(CorrectAndFlag, self).get_includes()
            + """
#include "exahype2/dg/Limiter.h"
#include "exahype2/dg/LimiterStatus.h"
#include "exahype2/dg/SubcellProjection.h"
"""
        )
        if self._solver._compute_new_time_step_size.strip():
            includes += '#include "exahype2/dg/EigenvalueStatistics.h"\n'
        return includes
