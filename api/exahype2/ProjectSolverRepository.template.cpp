// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SolverRepository.h"

#include "Constants.h"
#include "exahype2/NonNegativeReduction.h"
#include "exahype2/RefinementControlService.h"
#include "peano4/parallel/SpacetreeSet.h"

#include "tarch/instrumentation/Log.h"
#include "tarch/mpi/Rank.h"

{% for particleName in TRACERS %}
#include "vertexdata/{{ particleName }}Set.h"
{% endfor %}

{% for includePath in ADDITIONAL_INCLUDES %}
#include "{{ includePath }}"
{% endfor %}

{% for solver in SOLVERS %}
{{ NAMESPACE[:-1] | join("::") }}::{{ solver[0] }} {{ NAMESPACE | join("::") }}::{{ solver[1] }};
{% endfor %}

namespace {
  ::runner::SolverStatistic makeValue(const char* const label, const double value) {
    return {label, ::runner::SolverStatistic::Kind::Value, value, value, 0};
  }

  ::runner::SolverStatistic makeRange(const char* const label, const double minValue, const double maxValue) {
    return {label, ::runner::SolverStatistic::Kind::Value, minValue, maxValue, 0};
  }

  [[maybe_unused]] ::runner::SolverStatistic makeCount(const char* const label, const long value) {
    return {label, ::runner::SolverStatistic::Kind::Count, 0.0, 0.0, value};
  }

  template <typename Solver>
  auto appendMaxEigenvalue(
    std::vector<::runner::SolverStatistic>& result,
    const Solver& solver,
    [[maybe_unused]] const int overloadPriority
  ) -> decltype(solver.getMaxEigenvalue(), void()) {
    result.push_back(makeValue("λ_max", solver.getMaxEigenvalue()));
  }

  template <typename Solver>
  void appendMaxEigenvalue(
    [[maybe_unused]] std::vector<::runner::SolverStatistic>& result,
    [[maybe_unused]] const Solver& solver,
    [[maybe_unused]] const long overloadPriority
  ) {}

  template <typename Solver>
  auto appendTroubledCells(
    std::vector<::runner::SolverStatistic>& result,
    const Solver& solver,
    [[maybe_unused]] const int overloadPriority
  ) -> decltype(solver.getNumberOfTroubledCells(), void()) {
    if (solver.getNumberOfTroubledCells() > 0) {
      result.push_back(makeCount("Troubled", solver.getNumberOfTroubledCells()));
    }
  }

  template <typename Solver>
  void appendTroubledCells(
    [[maybe_unused]] std::vector<::runner::SolverStatistic>& result,
    [[maybe_unused]] const Solver& solver,
    [[maybe_unused]] const long overloadPriority
  ) {}

  template <typename Solver, typename AppendAdditionalStatistics>
  std::pair<std::string, std::vector<::runner::SolverStatistic>> makeSolverStatistics(
    const char* const solverName,
    const Solver& solver,
    const AppendAdditionalStatistics& appendAdditionalStatistics
  ) {
    std::vector<::runner::SolverStatistic> statistics;
    statistics.push_back(makeValue("dt", solver.getMinTimeStepSize()));
    statistics.push_back(makeRange("h", solver.getMinCellSize(), solver.getMaxCellSize()));
    appendAdditionalStatistics(statistics);
    appendMaxEigenvalue(statistics, solver, 0);
    appendTroubledCells(statistics, solver, 0);
    return {solverName, statistics};
  }

{% if PROJECT_TRACERS | length > 0 %}
  std::pair<std::string, std::vector<::runner::SolverStatistic>> makeParticleStatistics(
    const char* const particleName,
    const int numberOfParticles
  ) {
    return {particleName, {makeCount("Particles", numberOfParticles)}};
  }
{% endif %}
} // namespace

bool {{ NAMESPACE | join("::") }}::mayPlot() {
  bool result = true;
{% for solver in SOLVERS %}
  result = result and {{ solver[1] }}.mayPlot();
{% endfor %}
  return result;
}

std::vector<std::pair<std::string, std::vector<::runner::SolverStatistic>>>
{{ NAMESPACE | join("::") }}::getSolverStatistics() {
  std::vector<std::pair<std::string, std::vector<::runner::SolverStatistic>>> result;
{% for solver in SOLVERS %}
{% if solver[2] | length > 0 %}
  result.push_back(makeSolverStatistics(
    "{{ solver[0] }}",
    {{ solver[1] }},
    [](std::vector<::runner::SolverStatistic>& additionalStatistics) {
{% for particleName in solver[2] %}
      additionalStatistics.push_back(makeCount(
        "{{ particleName }}",
        ::{{ NAMESPACE[:-1] | join("::") }}::vertexdata::{{ particleName }}Set::getTotalNumberOfParticles()
      ));
{% endfor %}
    }
  ));
{% else %}
  result.push_back(makeSolverStatistics(
    "{{ solver[0] }}",
    {{ solver[1] }},
    []([[maybe_unused]] const std::vector<::runner::SolverStatistic>& additionalStatistics) {}
  ));
{% endif %}
{% endfor %}
  const auto particleStatistics = getParticleStatistics();
  result.insert(result.end(), particleStatistics.begin(), particleStatistics.end());
  return result;
}

std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>>
{{ NAMESPACE | join("::") }}::getConfigurationSummary() {
  std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>> result;
{% for solver in SOLVERS %}
  {
    std::vector<std::pair<std::string, std::string>> configurationEntries;
{% for statisticLabel, statisticValue in solver[3] %}
    configurationEntries.emplace_back("{{ statisticLabel }}", "{{ statisticValue }}");
{% endfor %}
    configurationEntries.emplace_back("Degrees of Freedom", std::to_string({{ solver[1] }}.getNumberOfDoFs()));
    result.emplace_back("Solver \"{{ solver[0] }}\"", std::move(configurationEntries));
  }
{% endfor %}
  return result;
}

std::vector<std::pair<std::string, std::vector<::runner::SolverStatistic>>>
{{ NAMESPACE | join("::") }}::getParticleStatistics() {
  std::vector<std::pair<std::string, std::vector<::runner::SolverStatistic>>> result;
{% for particleName in PROJECT_TRACERS %}
  result.push_back(makeParticleStatistics(
    "{{ particleName }}",
    ::{{ NAMESPACE[:-1] | join("::") }}::vertexdata::{{ particleName }}Set::getTotalNumberOfParticles()
  ));
{% endfor %}
  return result;
}

/**
 * Return whether any solver is in its first grid sweep of a time step.
 *
 * startTimeStep() enters this state. It remains observable after the mesh
 * traversal because finishTimeStep() does not advance the solver state.
 */
bool {{ NAMESPACE | join("::") }}::isFirstGridSweepOfTimeStep() {
  bool result = false;
{% for solver in SOLVERS %}
  result = result or {{ solver[1] }}.isFirstGridSweepOfTimeStep();
{% endfor %}
  return result;
}

/**
 * Return whether any solver is in its last grid sweep of a time step.
 *
 * The state remains observable after finishTimeStep(), which lets the caller
 * recognise a completed multi-sweep time step.
 */
bool {{ NAMESPACE | join("::") }}::isLastGridSweepOfTimeStep() {
  bool result = false;
{% for solver in SOLVERS %}
  result = result or {{ solver[1] }}.isLastGridSweepOfTimeStep();
{% endfor %}
  return result;
}

/**
 * Return the earliest time stamp exposed by any solver.
 *
 * With local time stepping this is the time reached by every cell; other cells
 * can already expose a later maximum time stamp.
 */
double {{ NAMESPACE | join("::") }}::getMinTimeStamp() {
  double result = std::numeric_limits<double>::max();
{% for solver in SOLVERS %}
  result = std::min(result, {{ solver[1] }}.getMinTimeStamp());
{% endfor %}
  return result;
}

/**
 * Return the latest time stamp exposed by any solver.
 *
 * It marks the leading edge when local time stepping spans multiple times.
 */
double {{ NAMESPACE | join("::") }}::getMaxTimeStamp() {
  double result = 0.0;
{% for solver in SOLVERS %}
  result = std::max(result, {{ solver[1] }}.getMaxTimeStamp());
{% endfor %}
  return result;
}

double {{ NAMESPACE | join("::") }}::getMinTimeStepSize() {
  double result = std::numeric_limits<double>::max();
{% for solver in SOLVERS %}
  result = ::exahype2::NonNegativeReduction::minimum(result, {{ solver[1] }}.getMinTimeStepSize());
{% endfor %}
  return result;
}

double {{ NAMESPACE | join("::") }}::getMaxTimeStepSize() {
  double result = 0.0;
{% for solver in SOLVERS %}
  result = ::exahype2::NonNegativeReduction::maximum(result, {{ solver[1] }}.getMaxTimeStepSize());
{% endfor %}
  return result;
}

void {{ NAMESPACE | join("::") }}::suspendSolversForOneGridSweep() {
{% for solver in SOLVERS %}
  {{ solver[1] }}.suspendSolversForOneGridSweep();
{% endfor %}
}

int {{ NAMESPACE | join("::") }}::getNumberOfDoFs() {
  int numberOfDoFs = 0;
{% for solver in SOLVERS %}
  numberOfDoFs += {{ solver[1] }}.getNumberOfDoFs();
{% endfor %}
  return numberOfDoFs;
}

void {{ NAMESPACE | join("::") }}::startGridConstructionStep() {
{% for solver in SOLVERS %}
  {{ solver[1] }}.startGridConstructionStep();
{% endfor %}
}

void {{ NAMESPACE | join("::") }}::startGridInitialisationStep() {
{% for solver in SOLVERS %}
  {{ solver[1] }}.startGridInitialisationStep();
{% endfor %}
}

/**
 * Advance every solver to its next sweep state.
 *
 * The state stays fixed until this function is called again or the solver is
 * suspended; finishTimeStep() does not alter it.
 */
void {{ NAMESPACE | join("::") }}::startTimeStep() {
{% for solver in SOLVERS %}
  {{ solver[1] }}.startTimeStep();
{% endfor %}
}

void {{ NAMESPACE | join("::") }}::startPlottingStep() {
{% for solver in SOLVERS %}
  {{ solver[1] }}.startPlottingStep();
{% endfor %}
}

void {{ NAMESPACE | join("::") }}::finishTimeStep() {
{% for solver in SOLVERS %}
  {{ solver[1] }}.finishTimeStep();
{% endfor %}
  const bool commitDynamicMeshRefinementEvents = isLastGridSweepOfTimeStep();
  if (commitDynamicMeshRefinementEvents) {
    // Every rank must observe the identical merged event set before the DynamicAMR observer
    // starts, or the trigger/realise cycle acts on rank-dependent sets and the topology
    // change never completes. The commit therefore gathers collectively, once per completed
    // time step on its last grid sweep.
    ::exahype2::RefinementControlService::getInstance().finishRefinementCriterionStep();
  }
{% for particleName in TRACERS %}
  ::{{ NAMESPACE[:-1] | join("::") }}::vertexdata::{{ particleName }}Set::reduceNumberOfParticles();
  logDebug(::{{ NAMESPACE[:-1] | join("::") }}::vertexdata::{{ particleName }}Set::printParticleStateStatistics());
{% endfor %}
}

void {{ NAMESPACE | join("::") }}::startDynamicMeshRefinementStep() {
{% for solver in SOLVERS %}
  {{ solver[1] }}.startDynamicMeshRefinementStep();
{% endfor %}
}

void {{ NAMESPACE | join("::") }}::finishDynamicMeshRefinementStep() {
{% for solver in SOLVERS %}
  {{ solver[1] }}.finishDynamicMeshRefinementStep();
{% endfor %}
  // Vertex changes deferred while neighbouring transitions are in flight need
  // their refinement events retained for delivery in the next traversal. The
  // same holds for refinements the traversal has just triggered: an event whose
  // target spans several levels can mark the next level's vertices only once
  // this round has been realised, so retiring the set here would realise an
  // incomplete, traversal-order-shaped subset of it.
  const auto gridStatistics = ::peano4::parallel::SpacetreeSet::getInstance().getGridStatistics();
  ::exahype2::RefinementControlService::getInstance().finishDynamicMeshRefinementStep(
    gridStatistics.numberOfDeferredGridControlChanges
      + gridStatistics.numberOfRefinementsTriggeredByGridControl
  );
}

void {{ NAMESPACE | join("::") }}::finishGridConstructionStep() {
{% for solver in SOLVERS %}
  {{ solver[1] }}.finishGridConstructionStep();
{% endfor %}
  ::exahype2::RefinementControlService::getInstance().finishStep(
    ::peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().numberOfDeferredGridControlChanges
  );
}

void {{ NAMESPACE | join("::") }}::finishGridInitialisationStep() {
{% for solver in SOLVERS %}
  {{ solver[1] }}.finishGridInitialisationStep();
{% endfor %}
  // The initialisation sweep evaluates the refinement criterion on the exact initial
  // condition. Committing the events here means the dedicated mesh-refinement sweeps
  // can realise them and re-apply the initial condition on the finer mesh before time
  // stepping starts.
  ::exahype2::RefinementControlService::getInstance().finishRefinementCriterionStep();
}

void {{ NAMESPACE | join("::") }}::finishPlottingStep() {
{% for solver in SOLVERS %}
  {{ solver[1] }}.finishPlottingStep();
{% endfor %}
}

void {{ NAMESPACE | join("::") }}::startSimulation() {
  // The domain offset anchors sibling groups when patch-local erase requests are
  // promoted to parent events. Coarsening requires consent from each solver that
  // evaluates a refinement criterion, so the service also needs the participant count.
  ::exahype2::RefinementControlService::getInstance().setDomainOffset(::{{ NAMESPACE[:-1] | join("::") }}::DomainOffset);
  ::exahype2::RefinementControlService::getInstance().setNumberOfSolvers({{ NUMBER_OF_AMR_PARTICIPANTS }});

{% for solver in SOLVERS %}
  {{ solver[1] }}.startSimulation();
{% endfor %}
}

void {{ NAMESPACE | join("::") }}::finishSimulation() {
{% for solver in SOLVERS %}
  {{ solver[1] }}.finishSimulation();
{% endfor %}
  ::exahype2::RefinementControlService::getInstance().finishStep();
}
