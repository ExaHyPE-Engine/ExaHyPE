# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractLimiterActionSet import AbstractLimiterActionSet

import jinja2
import peano4.solversteps


class Predict(AbstractLimiterActionSet):
    """Prepares each cell's limiter status for the next candidate solution."""

    _Template_TouchCellFirstTime = """
  if ({{GUARD}}) {
    // The previous status decides whether a cell keeps its evolved subcell data:
    // a cell already on the subcell scheme carries it forward, and re-projecting
    // the polynomial onto it every step would diffuse the subgrid profile.
    fineGridCell{{SOLVER_NAME}}CellLabel.setPreviousLimiterStatus(
      fineGridCell{{SOLVER_NAME}}CellLabel.getLimiterStatus()
    );
    fineGridCell{{SOLVER_NAME}}CellLabel.setLimiterStatus(
      fineGridCell{{SOLVER_NAME}}CellLabel.getWasTroubledInPreviousStep()
        ? ::exahype2::dg::LimiterStatus::Limited
        : ::exahype2::dg::LimiterStatus::Regular
    );
  }
"""

    def __init__(self, solver, guard):
        super().__init__(solver)
        self._guard = guard

    def _add_action_set_entries_to_dictionary(self, dictionary):
        dictionary["GUARD"] = self._guard

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
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        return (
            super(Predict, self).get_includes()
            + """
#include "exahype2/dg/LimiterStatus.h"
"""
        )
