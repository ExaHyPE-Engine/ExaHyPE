# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .action_set import ActionSet
from .solver_action_set import SolverActionSet


class RefinementCriterion(SolverActionSet):
    """Collects the refine and erase requests of every cell the guard admits.

    ``called_by_grid_construction`` evaluates the criterion without a solution, and
    ``skip_after_topology_change`` collects no request while a realised topology change is
    still fresh, so the committed event set is the same for every decomposition.
    """

    _TEMPLATE_TOUCH_CELL_LAST_TIME = """
  ::exahype2::RefinementCommand refinementCriterion = ::exahype2::getDefaultRefinementCommand();

  const double maxAdmissiblePatchSize =
    ::runner::RuntimeParameters::getInstance().getMaxAdmissiblePatchSize(DomainSize);
  const double minAdmissiblePatchSize =
    ::runner::RuntimeParameters::getInstance().getMinAdmissiblePatchSize(DomainSize);
{% if GRID_CONSTRUCTION %}
  if (not marker.willBeRefined() and tarch::la::greater(tarch::la::max(marker.getH()), maxAdmissiblePatchSize)) {
    refinementCriterion = ::exahype2::RefinementCommand::Refine;
  } else if (
    not marker.willBeRefined() and tarch::la::greaterEquals(tarch::la::max(marker.getH()), minAdmissiblePatchSize)
    and not fineGridCell{{CELL_LABEL_NAME}}.getAMRCriterionEvaluatedThroughoutGridConstruction()
  ) {
    refinementCriterion =
      {{FULL_QUALIFIED_SOLVER_NAME}}::refinementCriterion(nullptr, marker.getX(), marker.getH(), 0.0);
    fineGridCell{{CELL_LABEL_NAME}}.setAMRCriterionEvaluatedThroughoutGridConstruction(true);
  } else {
    refinementCriterion = ::exahype2::RefinementCommand::Keep;
  }
{%- else %}
  if ({{PREDICATE}} and tarch::la::greater(tarch::la::max(marker.getH()), maxAdmissiblePatchSize)) {
    refinementCriterion = ::exahype2::RefinementCommand::Refine;
  } else if (
    {{PREDICATE}} and fineGridCell{{CELL_LABEL_NAME}}.getHasUpdated()
    and (
      tarch::la::greater(tarch::la::max(marker.getH()), minAdmissiblePatchSize)
      or peano4::sfc::ActiveCurve::SubdivisionFactor * tarch::la::max(marker.getH()) <= maxAdmissiblePatchSize
    )
  ) {
    // A cell that can neither refine nor erase keeps its grid, so the criterion is not
    // evaluated there. The cell label carries the time of the solution the criterion
    // reads, because the solver's running minimum changes while other tasks update it.
    refinementCriterion = {{FULL_QUALIFIED_SOLVER_NAME}}::refinementCriterion(
      {{CELL_ACCESSOR}}.data(),
      marker.getX(),
      marker.getH(),
      fineGridCell{{CELL_LABEL_NAME}}.getTimeStamp()
    );
    if (
      refinementCriterion == ::exahype2::RefinementCommand::Refine
      and tarch::la::smallerEquals(tarch::la::max(marker.getH()), minAdmissiblePatchSize)
    ) {
      logDebug("The cell is at the minimum admissible patch size, so the refine command becomes Keep");
      refinementCriterion = ::exahype2::RefinementCommand::Keep;
    } else if (
      refinementCriterion == ::exahype2::RefinementCommand::Erase
      and peano4::sfc::ActiveCurve::SubdivisionFactor * tarch::la::max(marker.getH()) > maxAdmissiblePatchSize
    ) {
      logDebug("The coarser cell would exceed the maximum admissible patch size, so the erase command becomes Keep");
      refinementCriterion = ::exahype2::RefinementCommand::Keep;
    }
  } else {
    refinementCriterion = ::exahype2::RefinementCommand::Keep;
  }
{%- endif %}

  _localRefinementControl.addCommand(marker.getX(), marker.getH(), refinementCriterion);
"""

    def __init__(
        self,
        solver,
        guard,
        build_up_new_refinement_instructions,
        implement_previous_refinement_instructions,
        called_by_grid_construction,
        skip_after_topology_change,
        solution="current",
    ):
        super().__init__(solver)
        self._guard = guard
        self._build_up_new_refinement_instructions = (
            build_up_new_refinement_instructions
        )
        self._implement_previous_refinement_instructions = (
            implement_previous_refinement_instructions
        )
        self._called_by_grid_construction = called_by_grid_construction
        self._skip_after_topology_change = skip_after_topology_change
        self._solution = solution

    def grid_control_events_body(self):
        if self._implement_previous_refinement_instructions:
            return """
  return ::exahype2::RefinementControlService::getInstance().getGridControlEvents();
"""
        return """
  return std::vector<peano4::grid::GridControlEvent>();
"""

    def operation_body(self, operation_name):
        if not self._build_up_new_refinement_instructions:
            return ""
        if operation_name == ActionSet.OPERATION_BEGIN_TRAVERSAL:
            return """
  _localRefinementControl.clear();
"""
        if operation_name == ActionSet.OPERATION_END_TRAVERSAL:
            return """
  ::exahype2::RefinementControlService::getInstance().merge(_localRefinementControl);
"""
        if operation_name != ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
            return ""
        predicate = "not marker.willBeRefined() and not marker.hasBeenRefined() and "
        if self._skip_after_topology_change:
            predicate += (
                "not ::exahype2::RefinementControlService::getInstance()"
                ".hasRealisedTopologyChangesSinceLastTimeStep() and "
            )
        return self.render(
            self._TEMPLATE_TOUCH_CELL_LAST_TIME,
            CELL_LABEL_NAME=self._solver.name + "CellLabel",
            CELL_ACCESSOR=self._solver.layout.solution(self._solution),
            PREDICATE=predicate + self._guard,
            GRID_CONSTRUCTION=self._called_by_grid_construction,
        )

    def attributes(self):
        return """
  ::exahype2::RefinementControl _localRefinementControl{{{}}};
""".format(self._solver.solver_number)

    def includes(self):
        return super().includes() + [
            "exahype2/RefinementControlService.h",
            "peano4/sfc/ActiveCurve.h",
            "runner/RuntimeParameters.h",
            "tarch/instrumentation/Debug.h",
        ]
