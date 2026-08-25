# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractLimiterActionSet import AbstractLimiterActionSet
from ..Labels import (
    cell_dmp_extrema_to_face_snippet,
    hanging_face_dmp_extrema_reduce_snippet,
    reset_face_dmp_extrema_snippet,
)

import jinja2
import peano4.solversteps


class Predict(AbstractLimiterActionSet):
    """Prepares each cell's limiter status and its face sides' extrema for the
    next candidate solution."""

    _Template_TouchFaceFirstTime = """
{% if USES_DMP %}
  if ({{FACE_GUARD}}) {
{{RESET_FACE_DMP_EXTREMA}}
  }
{% endif %}
"""

    _Template_TouchHangingFaceLastTime = """
{% if USES_DMP %}
  if ({{FACE_GUARD}}) {
{{HANGING_FACE_DMP_EXTREMA_REDUCE}}
  }
{% endif %}
"""

    _Template_TouchCellFirstTime = """
  if ({{CELL_GUARD}}) {
    // The previous status decides whether a cell keeps its evolved subcell data.
    fineGridCell{{SOLVER_NAME}}CellLabel.setPreviousLimiterStatus(
      fineGridCell{{SOLVER_NAME}}CellLabel.getLimiterStatus()
    );
    fineGridCell{{SOLVER_NAME}}CellLabel.setLimiterStatus(
      fineGridCell{{SOLVER_NAME}}CellLabel.getWasTroubledInPreviousStep()
        ? ::exahype2::dg::LimiterStatus::Limited
        : ::exahype2::dg::LimiterStatus::Regular
    );
{% if USES_DMP %}
    // The predictor has copied the accepted solution into its backup. The cell
    // writes the backup's extrema, sampled like a candidate, onto its face
    // sides. The flagging of every neighbour reads them after the exchange.
    constexpr int NumberOfObservables     = {{NUMBER_OF_DMP_OBSERVABLES}};
    constexpr int NumberOfDofsPerAxis     = {{ORDER}} + 1;
    constexpr int NumberOfSubcellsPerAxis = 2 * {{ORDER}} + 1;
    constexpr int DataPerNode             = {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
#if DIMENSIONS == 2
    constexpr int NumberOfNodes = NumberOfDofsPerAxis * NumberOfDofsPerAxis;
#else
    constexpr int NumberOfNodes = NumberOfDofsPerAxis * NumberOfDofsPerAxis * NumberOfDofsPerAxis;
#endif
    using Shortcuts [[maybe_unused]] = {{FULL_QUALIFIED_SOLVER_NAME}}::Shortcuts;
    const auto mapStateToObservables = [](
      const {{SOLUTION_STORAGE_PRECISION}}* const Q,
      {{SOLUTION_STORAGE_PRECISION}}* const observables
    ) {
      {{DMP_OBSERVABLES}}
    };
    static constexpr {{SOLUTION_STORAGE_PRECISION}} DgToLobattoMatrix[] = {{DMP_DG_TO_LOBATTO_MATRIX}};
    static constexpr {{SOLUTION_STORAGE_PRECISION}} DgToSubcellMatrix[] = {{DMP_DG_TO_SUBCELL_MATRIX}};
    const auto* const acceptedSolution = fineGridCell{{UNKNOWN_IDENTIFIER}}_old.data();
    {{SOLUTION_STORAGE_PRECISION}} acceptedMinimum[NumberOfObservables];
    {{SOLUTION_STORAGE_PRECISION}} acceptedMaximum[NumberOfObservables];
{{ACCEPTED_EXTREMA_REDUCTION}}
{{CELL_DMP_EXTREMA_TO_FACES}}
{% endif %}
  }
"""

    def __init__(self, solver, cell_guard, face_guard):
        super().__init__(solver)
        self._cell_guard = cell_guard
        self._face_guard = face_guard

    def _add_action_set_entries_to_dictionary(self, dictionary):
        dictionary["CELL_GUARD"] = self._cell_guard
        dictionary["FACE_GUARD"] = self._face_guard
        dictionary["USES_DMP"] = self._solver._subcell_limiter_uses_dmp
        if not dictionary["USES_DMP"]:
            return
        number_of_observables = self._solver._subcell_limiter_number_of_dmp_observables
        dictionary["NUMBER_OF_DMP_OBSERVABLES"] = number_of_observables
        dictionary["DMP_OBSERVABLES"] = self._rendered_dmp_observables(dictionary)
        self._add_dmp_sampling_matrices_to_dictionary(dictionary)
        dictionary["ACCEPTED_EXTREMA_REDUCTION"] = self._dmp_extrema_reduction_snippet(
            "acceptedSolution", "acceptedMinimum", "acceptedMaximum", dictionary
        )
        dictionary["RESET_FACE_DMP_EXTREMA"] = reset_face_dmp_extrema_snippet(
            self._solver._name, number_of_observables
        )
        dictionary["CELL_DMP_EXTREMA_TO_FACES"] = cell_dmp_extrema_to_face_snippet(
            self._solver._name, number_of_observables, "acceptedMinimum", "acceptedMaximum"
        )
        dictionary["HANGING_FACE_DMP_EXTREMA_REDUCE"] = (
            hanging_face_dmp_extrema_reduce_snippet(self._solver._name, number_of_observables)
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
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_HANGING_FACE_LAST_TIME
        ):
            result = jinja2.Template(self._Template_TouchHangingFaceLastTime).render(
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
        return (
            super(Predict, self).get_includes()
            + """
#include "exahype2/dg/Limiter.h"
#include "exahype2/dg/LimiterStatus.h"
"""
        )
