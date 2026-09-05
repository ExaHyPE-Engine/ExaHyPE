# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractFVActionSet import AbstractFVActionSet
from ...RefinementCriterionTemplate import RefinementCriterionTemplate

import jinja2
import peano4.solversteps


class RefinementCriterion(AbstractFVActionSet):
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
        The guard is a C++ expression that decides per cell whether the criterion
        runs. called_by_grid_construction selects the variant that evaluates the
        criterion with a nullptr solution and t=0, and skip_after_topology_change
        collects no request while a realised topology change is still fresh.
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
            if self._solver._patch.dim[0] != self._solver._patch.dim[1]:
                raise Exception("Error: The refinement criterion needs a square patch.")

            dictionary = {}
            dictionary["CELL_LABEL_NAME"] = self._solver._cell_label.name
            dictionary["CELL_ACCESSOR"] = (
                "fineGridCell" + self._solver.current_unknown_identifier()
            )
            # The first time step after a realised topology change still carries
            # refinement markers on a set of cells that depends on the decomposition.
            # Collecting no requests during that step keeps the committed event set
            # identical for every decomposition.
            dictionary["PREDICATE"] = (
                "not marker.willBeRefined() and not marker.hasBeenRefined() and "
            )
            if self._skip_after_topology_change:
                dictionary["PREDICATE"] += (
                    "not ::exahype2::RefinementControlService::getInstance()"
                    ".hasRealisedTopologyChangesSinceLastTimeStep() and "
                )
            dictionary["PREDICATE"] += self._guard
            dictionary["GRID_CONSTRUCTION"] = self._called_by_grid_construction
            self._solver._init_dictionary_with_default_parameters(dictionary)
            self._solver.add_entries_to_text_replacement_dictionary(dictionary)
            dictionary["CHECKPOINT_SUPPRESSED_SOLVER_STATES"] = ""
            result = jinja2.Template(RefinementCriterionTemplate).render(**dictionary)

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
#include "exahype2/fv/PatchUtils.h"
#include "tarch/instrumentation/Debug.h"
#include "Constants.h"
"""
        )
