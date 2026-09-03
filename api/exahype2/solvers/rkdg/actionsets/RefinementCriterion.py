# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractRungeKuttaDGActionSet import AbstractRungeKuttaDGActionSet
from ...RefinementCriterionTemplate import RefinementCriterionTemplate

import jinja2
import peano4.solversteps


class RefinementCriterion(AbstractRungeKuttaDGActionSet):

    def __init__(
        self,
        solver,
        guard,
        build_up_new_refinement_instructions,
        implement_previous_refinement_instructions,
        called_by_grid_construction,
        skip_after_topology_change,
    ):
        """
        `guard` is a C++ expression that decides per cell whether the criterion
        runs. `implement_previous_refinement_instructions` decides whether the
        action set reads the instructions from the RefinementControl database.
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
        self._skip_after_topology_change = skip_after_topology_change

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
            d["CELL_ACCESSOR"] = "fineGridCell" + self._solver._current_time_step.name
            # The first time step after a realised topology change still carries
            # refinement markers on some cells, and which cells carry them depends
            # on how many sweeps the mesh update needed, which varies with the
            # decomposition. Collecting no requests during that step keeps the
            # committed event set identical for every decomposition.
            d["PREDICATE"] = (
                "not marker.willBeRefined() and not marker.hasBeenRefined() and "
            )
            if self._skip_after_topology_change:
                d["PREDICATE"] += (
                    "not ::exahype2::RefinementControlService::getInstance()"
                    ".hasRealisedTopologyChangesSinceLastTimeStep() and "
                )
            d["PREDICATE"] += self._guard
            d["GRID_CONSTRUCTION"] = self._called_by_grid_construction
            # A Runge-Kutta DG solver cannot yet offload its criterion to a free function.
            d["STATELESS_REFINEMENT_CRITERION"] = False
            self._solver._init_dictionary_with_default_parameters(d)
            self._solver.add_entries_to_text_replacement_dictionary(d)
            d["CHECKPOINT_SUPPRESSED_SOLVER_STATES"] = ""
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
#include "tarch/instrumentation/Debug.h"
#include "Constants.h"
"""
        )
