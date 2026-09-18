# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

RefinementCriterionTemplate = """
  ::exahype2::RefinementCommand refinementCriterion = ::exahype2::getDefaultRefinementCommand();
  const double                  maxAdmissiblePatchSize =
    ::runner::RuntimeParameters::getInstance().getMaxAdmissiblePatchSize(DomainSize);
  const double minAdmissiblePatchSize =
    ::runner::RuntimeParameters::getInstance().getMinAdmissiblePatchSize(DomainSize);

  {% if GRID_CONSTRUCTION %}
  if (not marker.willBeRefined() and tarch::la::greater(tarch::la::max(marker.getH()), maxAdmissiblePatchSize)) {
    refinementCriterion = ::exahype2::RefinementCommand::Refine;
  } else if (not marker.willBeRefined()
             and tarch::la::greaterEquals(tarch::la::max(marker.getH()), minAdmissiblePatchSize)
             and not fineGridCell{{CELL_LABEL_NAME}}.getAMRCriterionEvaluatedThroughoutGridConstruction()) {
    {% if STATELESS_REFINEMENT_CRITERION %}
    refinementCriterion =
      {{FULL_QUALIFIED_SOLVER_NAME}}::refinementCriterion(nullptr, marker.getX(), marker.getH(), 0.0);
    {% else %}
    refinementCriterion =
      repositories::{{SOLVER_INSTANCE}}.refinementCriterion(nullptr, marker.getX(), marker.getH(), 0.0);
    {% endif %}
    fineGridCell{{CELL_LABEL_NAME}}.setAMRCriterionEvaluatedThroughoutGridConstruction(true);
  } else {
    refinementCriterion = ::exahype2::RefinementCommand::Keep;
  }
  {% else %}
  if ({{PREDICATE}}
      and tarch::la::greater(tarch::la::max(marker.getH()), maxAdmissiblePatchSize)) {
    refinementCriterion = ::exahype2::RefinementCommand::Refine;
  } else if ({{PREDICATE}} and fineGridCell{{CELL_LABEL_NAME}}.getHasUpdated()
             and (tarch::la::greater(tarch::la::max(marker.getH()), minAdmissiblePatchSize)
                  or peano4::sfc::ActiveCurve::SubdivisionFactor * tarch::la::max(marker.getH())
                       <= maxAdmissiblePatchSize)) {
    // A cell that can neither refine nor erase keeps its grid, so the criterion is not
    // evaluated there. The cell label carries the time of the solution the criterion
    // reads, because the solver's running minimum changes while other tasks update it.
    {% if STATELESS_REFINEMENT_CRITERION %}
    refinementCriterion = {{FULL_QUALIFIED_SOLVER_NAME}}::refinementCriterion(
      {{CELL_ACCESSOR}}.data(),
      marker.getX(),
      marker.getH(),
      fineGridCell{{CELL_LABEL_NAME}}.getTimeStamp()
    );
    {% else %}
    refinementCriterion = repositories::{{SOLVER_INSTANCE}}.refinementCriterion(
      {{CELL_ACCESSOR}}.data(),
      marker.getX(),
      marker.getH(),
      fineGridCell{{CELL_LABEL_NAME}}.getTimeStamp()
    );
    {% endif %}
    if (refinementCriterion == ::exahype2::RefinementCommand::Refine
        and tarch::la::smallerEquals(tarch::la::max(marker.getH()), minAdmissiblePatchSize)) {
      logDebug("The cell is at the minimum admissible patch size, so the refine command becomes Keep");
      refinementCriterion = ::exahype2::RefinementCommand::Keep;
    } else if (refinementCriterion == ::exahype2::RefinementCommand::Erase
               and peano4::sfc::ActiveCurve::SubdivisionFactor * tarch::la::max(marker.getH())
                     > maxAdmissiblePatchSize) {
      logDebug("The coarser cell would exceed the maximum admissible patch size, so the erase command becomes Keep");
      refinementCriterion = ::exahype2::RefinementCommand::Keep;
    }
  } else {
    refinementCriterion = ::exahype2::RefinementCommand::Keep;
  }
  {% endif %}

  _localRefinementControl.addCommand(marker.getX(), marker.getH(), refinementCriterion);
"""
