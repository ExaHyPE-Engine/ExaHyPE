// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SolverRepository.h"

#include "Constants.h"
#include "exahype2/NonNegativeReduction.h"
#include "exahype2/RefinementControlService.h"
#include "peano4/topology/TopologyManager.h"

#include "tarch/Assertions.h"
#include "tarch/instrumentation/Debug.h"
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
  /**
   * Records a committed event set the refinement plan rejects as realised, so
   * no mesh-refinement sweep runs for it. Every rank holds the same committed
   * set and evaluates the same plan, so every rank drops the same set.
   */
  void realisePlanRejectedEvents() {
    auto& service = ::exahype2::RefinementControlService::getInstance();
    auto& manager = ::peano4::topology::TopologyManager::getInstance();
    if (service.hasGridControlEvents() and not manager.controlEventsChangePlan(service.getGridControlEvents())) {
      service.realiseCommittedEventsWithoutSweep();
    }
  }

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
 * Returns whether any solver is in its first grid sweep of a time step.
 *
 * startTimeStep() enters this state, and finishTimeStep() leaves it in place.
 */
bool {{ NAMESPACE | join("::") }}::isFirstGridSweepOfTimeStep() {
  bool result = false;
{% for solver in SOLVERS %}
  result = result or {{ solver[1] }}.isFirstGridSweepOfTimeStep();
{% endfor %}
  return result;
}

/**
 * Returns whether any solver is in its last grid sweep of a time step.
 *
 * finishTimeStep() leaves the state in place, so a caller reads it after the sweep.
 */
bool {{ NAMESPACE | join("::") }}::isLastGridSweepOfTimeStep() {
  bool result = false;
{% for solver in SOLVERS %}
  result = result or {{ solver[1] }}.isLastGridSweepOfTimeStep();
{% endfor %}
  return result;
}

/**
 * Returns the earliest time stamp of any solver.
 *
 * Under local time stepping every cell has reached this time, and a cell can carry a later one.
 */
double {{ NAMESPACE | join("::") }}::getMinTimeStamp() {
  double result = std::numeric_limits<double>::max();
{% for solver in SOLVERS %}
  result = std::min(result, {{ solver[1] }}.getMinTimeStamp());
{% endfor %}
  return result;
}

/**
 * Returns the latest time stamp of any solver.
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
 * Advances every solver to its next sweep state.
 *
 * The state stays fixed until the next call of this function or a suspension of the solver.
 */
void {{ NAMESPACE | join("::") }}::startTimeStep() {
  assertion(::exahype2::RefinementControlService::getInstance().committedEventsAgreeAcrossRanks());
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
    // Every rank holds the identical merged event set before the mesh-refinement sweep
    // starts, so the commit gathers collectively on the last grid sweep of a time step.
    ::exahype2::RefinementControlService::getInstance().finishRefinementCriterionStep();
    realisePlanRejectedEvents();
  }
{% for particleName in TRACERS %}
  ::{{ NAMESPACE[:-1] | join("::") }}::vertexdata::{{ particleName }}Set::reduceNumberOfParticles();
  logDebug(::{{ NAMESPACE[:-1] | join("::") }}::vertexdata::{{ particleName }}Set::printParticleStateStatistics());
{% endfor %}
}

void {{ NAMESPACE | join("::") }}::startDynamicMeshRefinementStep() {
  assertion(::exahype2::RefinementControlService::getInstance().committedEventsAgreeAcrossRanks());
{% for solver in SOLVERS %}
  {{ solver[1] }}.startDynamicMeshRefinementStep();
{% endfor %}
}

void {{ NAMESPACE | join("::") }}::finishDynamicMeshRefinementStep() {
  {%- for solver in SOLVERS %}
  {{ solver[1] }}.finishDynamicMeshRefinementStep();
  {%- endfor %}

  ::exahype2::RefinementControlService::getInstance().finishDynamicMeshRefinementStep();
  realisePlanRejectedEvents();
}

void {{ NAMESPACE | join("::") }}::finishGridConstructionStep() {
  {%- for solver in SOLVERS %}
  {{ solver[1] }}.finishGridConstructionStep();
  {%- endfor %}

  ::exahype2::RefinementControlService::getInstance().finishStep();
  realisePlanRejectedEvents();
}

void {{ NAMESPACE | join("::") }}::finishGridInitialisationStep() {
{% for solver in SOLVERS %}
  {{ solver[1] }}.finishGridInitialisationStep();
{% endfor %}
  // The initialisation sweep evaluates the refinement criterion on the initial condition.
  // The commit here lets the mesh-refinement sweeps set the initial condition on the finer
  // mesh before time stepping starts.
  ::exahype2::RefinementControlService::getInstance().finishRefinementCriterionStep();
  realisePlanRejectedEvents();
}

void {{ NAMESPACE | join("::") }}::finishPlottingStep() {
{% for solver in SOLVERS %}
  {{ solver[1] }}.finishPlottingStep();
{% endfor %}
}

void {{ NAMESPACE | join("::") }}::startSimulation() {
  // A refinement request names its cell by level and lattice position, so the service needs
  // the anchor and the extent of the cell lattice. Coarsening needs the count of the solvers
  // that evaluate a refinement criterion, because every one of them has to consent.
  ::exahype2::RefinementControlService::getInstance().setDomain(DomainOffset, DomainSize);
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
  ::exahype2::RefinementControlService::getInstance().shutdown();
}
