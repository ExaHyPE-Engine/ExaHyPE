# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractADERDGActionSet import AbstractADERDGActionSet
from ...RefinementCriterionTemplate import RefinementCriterionTemplate

import jinja2
import peano4.solversteps


class RefinementCriterion(AbstractADERDGActionSet):
    """
    The action set that evaluates the refinement criterion.

    AMR is a multistep process in ExaHyPE. Most of the relevant documentation
    lives in the class exahype2::RefinementControl.

    There are different phases in ExaHyPE: grid generation (either with
    refinement or without), initialisation, plotting and time stepping. Which
    time stepping evaluates AMR depends on the solver, but a few points hold
    for all schemes:

    - The grid creation does not work on solver data, so we do not evaluate
      the criterion here.
    - InitGrid has volumetric data and can evaluate the criterion. The guard
      that a solver passes in enables that.
    - The plotting does not alter the solution. It thus makes no sense to
      evaluate the criterion here.

    The criterion is evaluated once per cell: the solver's refinementCriterion()
    receives the whole (Order+1)^DIMENSIONS node block in AoS layout together
    with the cell centre and size, so it can compute cell-wide quantities such
    as a multiresolution detail. During grid construction there is no solution
    yet, so Q is nullptr.
    """

    def __init__(
        self,
        solver,
        guard,
        build_up_new_refinement_instructions,
        implement_previous_refinement_instructions,
        called_by_grid_construction,
    ):
        """
        :: Attributes

        _implement_previous_refinement_instructions: Boolean
          Controls whether instructions are picked up from the RefinementControl
          database. Consult exahype2::RefinementControl for a description of the
          control flow.

        :: Arguments

        guard: C++ expression which evaluates to true or false
          A per-cell decision whether to study a cell.

        build_up_new_refinement_instructions: Boolean
          See remarks on multistep realisation of AMR in C++ class
          exahype2::RefinementControl.

        implement_previous_refinement_instructions: Boolean
          See remarks on multistep realisation of AMR in C++ class
          exahype2::RefinementControl.

        called_by_grid_construction: Boolean
          Selects the grid construction variant of the criterion, which has no
          solution at hand and therefore evaluates it with a nullptr and t=0.
        """
        super(RefinementCriterion, self).__init__(solver)
        self._guard = guard
        self._build_up_new_refinement_instructions = (
            build_up_new_refinement_instructions
        )
        self._implement_previous_refinement_instructions = (
            implement_previous_refinement_instructions
        )
        self._called_by_grid_construction = called_by_grid_construction

    def get_body_of_getGridControlEvents(self):
        if self._implement_previous_refinement_instructions:
            return """
    return ::exahype2::RefinementControlService::getInstance().getGridControlEvents();
"""
        else:
            return """
    return std::vector<::peano4::grid::GridControlEvent>();
"""

    def get_body_of_operation(self, operation_name):
        result = ""
        if (
            self._build_up_new_refinement_instructions
            and operation_name == peano4.solversteps.ActionSet.OPERATION_BEGIN_TRAVERSAL
        ):
            result = """
  _localRefinementControl.clear();
"""

        if (
            self._build_up_new_refinement_instructions
            and operation_name == peano4.solversteps.ActionSet.OPERATION_END_TRAVERSAL
        ):
            result = """
  ::exahype2::RefinementControlService::getInstance().merge(_localRefinementControl);
"""

        if (
            self._build_up_new_refinement_instructions
            and operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_LAST_TIME
        ):
            d = {}
            d["CELL_LABEL_NAME"] = self._solver._cell_label.name
            # When touchCellLastTime runs, the prediction has already overwritten the
            # current solution buffer with the space-time predictor's provisional state
            # for the next step, whose surface corrections are still missing. The
            # corrected solution of this time step lives in the previous-solution
            # buffer, so the criterion must read that one.
            d["CELL_ACCESSOR"] = (
                "fineGridCell" + self._solver.previous_unknown_identifier()
            )
            d["PREDICATE"] = (
                "not marker.willBeRefined() and not marker.hasBeenRefined() and "
                + self._guard
            )
            d["GRID_CONSTRUCTION"] = self._called_by_grid_construction
            self._solver._init_dictionary_with_default_parameters(d)
            self._solver.add_entries_to_text_replacement_dictionary(d)
            result = jinja2.Template(RefinementCriterionTemplate).render(**d)

        return result

    def get_attributes(self):
        return f"""
    ::exahype2::RefinementControl _localRefinementControl{{{self._solver._solver_number}}};
"""

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        return (
            super(RefinementCriterion, self).get_includes()
            + """
#include "exahype2/RefinementControlService.h"
#include "runner/RuntimeParameters.h"
#include "Constants.h"
"""
        )
