# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractRungeKuttaDGActionSet import AbstractRungeKuttaDGActionSet
from ...RefinementCriterionTemplate import RefinementCriterionTemplate

import jinja2
import peano4.solversteps


class RefinementCriterion(AbstractRungeKuttaDGActionSet):
    """
    The action set that evaluates the refinement criterion.

    AMR is a multistep process in ExaHyPE. The class exahype2::RefinementControl
    holds most of the relevant documentation.

    ExaHyPE runs through different phases: grid generation (with or without
    refinement), initialisation, plotting and time stepping. Which phase
    evaluates AMR depends on the solver, but some points hold for all schemes:

    - The grid creation does not use any actual solver data, so we should not
      evaluate the criterion here.
    - The initialisation has volumetric data and may evaluate the criterion.
      The updated flag that the initial condition sets lets the evaluation
      through.
    - The plotting does not alter the solution. It thus makes no sense to
      evaluate the criterion here.

    The criterion is evaluated once per cell: the solver's refinementCriterion()
    receives the whole node block of the current Runge-Kutta step in AoS layout
    together with the cell centre and size, so it can compute cell-wide
    quantities such as a multiresolution detail. During grid construction there
    is no solution yet, so Q is nullptr.
    """

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
        :: Attributes.

        _implement_previous_refinement_instructions: Boolean
          This name might be misleading. Consult exahype2::RefinementControl for
          a description of the control flow. This flag controls whether
          instructions are picked up from the RefinementControl database.

        :: Arguments

        guard: C++ expression which evaluates to true or false
          A per-cell decision whether we study a cell.

        build_up_new_refinement_instructions: Boolean
          See remarks on multistep realisation of AMR in C++ class
          exahype2::RefinementControl.

        implement_previous_refinement_instructions: Boolean
          See remarks on multistep realisation of AMR in C++ class
          exahype2::RefinementControl.
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
            d["PREDICATE"] = "not marker.willBeRefined() and not marker.hasBeenRefined() and "
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
