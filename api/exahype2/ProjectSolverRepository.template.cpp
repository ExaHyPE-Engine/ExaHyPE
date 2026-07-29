// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SolverRepository.h"

#include "Constants.h"
#include "exahype2/RefinementControlService.h"
#include "peano4/parallel/SpacetreeSet.h"

#include "tarch/instrumentation/Log.h"
#include "tarch/mpi/Rank.h"

{% for item in TRACERS -%}
#include "vertexdata/{{ item }}Set.h"
{% endfor %}

{% for INCLUDE in ADDITIONAL_INCLUDES -%}
#include "{{INCLUDE}}"
{% endfor %}

{%- for item in SOLVERS %}
{{ NAMESPACE[:-1] | join("::") }}::{{ item[0] }} {{ NAMESPACE | join("::") }}::{{ item[1] }};
{# Comment to ensure that jinja2 adds a new line#}
{%- endfor %}

namespace {
  ::runner::SolverStatistic makeValue(const char* label, double value) {
    return {label, ::runner::SolverStatistic::Kind::Value, value, value, 0};
  }

  ::runner::SolverStatistic makeRange(const char* label, double minValue, double maxValue) {
    return {label, ::runner::SolverStatistic::Kind::Value, minValue, maxValue, 0};
  }

  [[maybe_unused]] ::runner::SolverStatistic makeCount(const char* label, long value) {
    return {label, ::runner::SolverStatistic::Kind::Count, 0.0, 0.0, value};
  }

  template <typename Solver>
  auto appendMaxEigenvalue(std::vector<::runner::SolverStatistic>& result, const Solver& solver, int) -> decltype(solver.getMaxEigenvalue(), void()) {
    result.push_back(makeValue("λ_max", solver.getMaxEigenvalue()));
  }

  template <typename Solver>
  void appendMaxEigenvalue(std::vector<::runner::SolverStatistic>&, const Solver&, long) {}

  template <typename Solver>
  auto appendTroubledCells(std::vector<::runner::SolverStatistic>& result, const Solver& solver, int) -> decltype(solver.getNumberOfTroubledCells(), void()) {
    if (solver.getNumberOfTroubledCells() > 0) {
      result.push_back(makeCount("Troubled", solver.getNumberOfTroubledCells()));
    }
  }

  template <typename Solver>
  void appendTroubledCells(std::vector<::runner::SolverStatistic>&, const Solver&, long) {}

  template <typename Solver, typename AppendAdditionalStatistics>
  std::pair<std::string, std::vector<::runner::SolverStatistic>> makeSolverStatistics(const char* solverName, const Solver& solver, AppendAdditionalStatistics appendAdditionalStatistics) {
    std::vector<::runner::SolverStatistic> statistics;
    statistics.push_back(makeValue("dt", solver.getMinTimeStepSize()));
    statistics.push_back(makeRange("h", solver.getMinCellSize(), solver.getMaxCellSize()));
    appendAdditionalStatistics(statistics);
    appendMaxEigenvalue(statistics, solver, 0);
    appendTroubledCells(statistics, solver, 0);
    return {solverName, statistics};
  }

  {%- if PROJECT_TRACERS %}
  std::pair<std::string, std::vector<::runner::SolverStatistic>> makeParticleStatistics(const char* particleName, int numberOfParticles) {
    return {particleName, {makeCount("Particles", numberOfParticles)}};
  }
  {%- endif %}
} // namespace

bool {{ NAMESPACE | join("::") }}::mayPlot() {
  bool result = true;
  {%- for item in SOLVERS %}
  result = result and {{ item[1] }}.mayPlot();
  {%- endfor %}
  return result;
}

std::vector<std::pair<std::string, std::vector<::runner::SolverStatistic>>> {{ NAMESPACE | join("::") }}::getSolverStatistics() {
  std::vector<std::pair<std::string, std::vector<::runner::SolverStatistic>>> result;
  {%- for item in SOLVERS %}
  {% if item[2] | length > 0 -%}
  result.push_back(makeSolverStatistics("{{ item[0] }}", {{ item[1] }}, [](std::vector<::runner::SolverStatistic>& result) {
    {% for tracer in item[2] -%}
    result.push_back(makeCount("{{ tracer }}", vertexdata::{{ tracer }}Set::getTotalNumberOfParticles()));
    {# Comment to ensure that jinja2 adds a new line#}
    {%- endfor %}
  }));
  {% else -%}
  result.push_back(makeSolverStatistics("{{ item[0] }}", {{ item[1] }}, [](std::vector<::runner::SolverStatistic>&) {}));
  {%- endif %}
  {%- endfor %}
  const auto particleStatistics = getParticleStatistics();
  result.insert(result.end(), particleStatistics.begin(), particleStatistics.end());
  return result;
}

std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>> {{ NAMESPACE | join("::") }}::getConfigurationSummary() {
  std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>> result;
  {%- for item in SOLVERS %}
  {
    std::vector<std::pair<std::string, std::string>> entries;
    {% for statistic in item[3] -%}
    entries.emplace_back("{{ statistic[0] }}", "{{ statistic[1] }}");
    {# Comment to ensure that jinja2 adds a new line#}
    {%- endfor %}
    entries.emplace_back("Degrees of Freedom", std::to_string({{ item[1] }}.getNumberOfDoFs()));
    result.emplace_back("Solver \"{{ item[0] }}\"", entries);
  }
  {%- endfor %}
  return result;
}

std::vector<std::pair<std::string, std::vector<::runner::SolverStatistic>>> {{ NAMESPACE | join("::") }}::getParticleStatistics() {
  std::vector<std::pair<std::string, std::vector<::runner::SolverStatistic>>> result;
  {% for item in PROJECT_TRACERS -%}
  result.push_back(makeParticleStatistics("{{ item }}", vertexdata::{{ item }}Set::getTotalNumberOfParticles()));
  {%- endfor %}
  return result;
}

bool {{ NAMESPACE | join("::") }}::isFirstGridSweepOfTimeStep() {
  bool result = false;
  {%- for item in SOLVERS %}
  result = result or {{ item[1] }}.isFirstGridSweepOfTimeStep();
  {%- endfor %}
  return result;
}

bool {{ NAMESPACE | join("::") }}::isLastGridSweepOfTimeStep() {
  bool result = false;
  {%- for item in SOLVERS %}
  result = result or {{ item[1] }}.isLastGridSweepOfTimeStep();
  {%- endfor %}
  return result;
}

double {{ NAMESPACE | join("::") }}::getMinTimeStamp() {
  double result = std::numeric_limits<double>::max();
  {%- for item in SOLVERS %}
  result = std::min(result, {{ item[1] }}.getMinTimeStamp());
  {%- endfor %}
  return result;
}

double {{ NAMESPACE | join("::") }}::getMaxTimeStamp() {
  double result = 0.0;
  {%- for item in SOLVERS %}
  result = std::max(result, {{ item[1] }}.getMaxTimeStamp());
  {%- endfor %}
  return result;
}

double {{ NAMESPACE | join("::") }}::getMinTimeStepSize() {
  double result = std::numeric_limits<double>::max();
  {%- for item in SOLVERS %}
  result = std::min( result, {{ item[1] }}.getMinTimeStepSize() );
  {%- endfor %}
  return result;
}

double {{ NAMESPACE | join("::") }}::getMaxTimeStepSize() {
  double result = 0.0;
  {%- for item in SOLVERS %}
  result = std::max( result, {{ item[1] }}.getMaxTimeStepSize() );
  {%- endfor %}
  return result;
}

void {{ NAMESPACE | join("::") }}::suspendSolversForOneGridSweep() {
  {%- for item in SOLVERS %}
  {{ item[1] }}.suspendSolversForOneGridSweep();
  {%- endfor %}
}

int {{ NAMESPACE | join("::") }}::getNumberOfDoFs() {
  int numberOfDoFs = 0;
  {%- for item in SOLVERS %}
  numberOfDoFs += {{ item[1] }}.getNumberOfDoFs();
  {%- endfor %}
  return numberOfDoFs;
}

void {{ NAMESPACE | join("::") }}::startGridConstructionStep() {
  {%- for item in SOLVERS %}
  {{ item[1] }}.startGridConstructionStep();
  {%- endfor %}
}

void {{ NAMESPACE | join("::") }}::startGridInitialisationStep() {
  {%- for item in SOLVERS %}
  {{ item[1] }}.startGridInitialisationStep();
  {%- endfor %}
}

void {{ NAMESPACE | join("::") }}::startTimeStep() {
  {%- for item in SOLVERS %}
  {{ item[1] }}.startTimeStep();
  {%- endfor %}
}

void {{ NAMESPACE | join("::") }}::startPlottingStep() {
  {%- for item in SOLVERS %}
  {{ item[1] }}.startPlottingStep();
  {%- endfor %}
}

void {{ NAMESPACE | join("::") }}::finishTimeStep() {
  const bool commitDynamicMeshRefinementEvents = isLastGridSweepOfTimeStep();

  {%- for item in SOLVERS %}
  {{ item[1] }}.finishTimeStep();
  {%- endfor %}

  if (commitDynamicMeshRefinementEvents) {
    // Every rank must observe the identical merged event set before the DynamicAMR observer
    // starts, or the trigger/realise cycle acts on rank-dependent sets and the topology
    // change never completes. The commit therefore gathers collectively, once per completed
    // time step on its last grid sweep.
    ::exahype2::RefinementControlService::getInstance().finishRefinementCriterionStep();
  }

  {% for item in TRACERS -%}
  vertexdata::{{ item }}Set::reduceNumberOfParticles();
  logDebug(vertexdata::{{ item }}Set::printParticleStateStatistics());
  {%- endfor %}
}

void {{ NAMESPACE | join("::") }}::startDynamicMeshRefinementStep() {
  {%- for item in SOLVERS %}
  {{ item[1] }}.startDynamicMeshRefinementStep();
  {%- endfor %}
}

void {{ NAMESPACE | join("::") }}::finishDynamicMeshRefinementStep() {
  {%- for item in SOLVERS %}
  {{ item[1] }}.finishDynamicMeshRefinementStep();
  {%- endfor %}

  // A traversal may have to defer requested vertex changes while neighbouring transitions
  // are in flight; the service then keeps the events alive for another delivery.
  ::exahype2::RefinementControlService::getInstance().finishDynamicMeshRefinementStep(
    peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().numberOfDeferredGridControlChanges
  );
}

void {{ NAMESPACE | join("::") }}::finishGridConstructionStep() {
  {%- for item in SOLVERS %}
  {{ item[1] }}.finishGridConstructionStep();
  {%- endfor %}

  ::exahype2::RefinementControlService::getInstance().finishStep(
    peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().numberOfDeferredGridControlChanges
  );
}

void {{ NAMESPACE | join("::") }}::finishGridInitialisationStep() {
  {%- for item in SOLVERS %}
  {{ item[1] }}.finishGridInitialisationStep();
  {%- endfor %}

  // The initialisation sweep evaluates the refinement criterion on the exact initial
  // condition. Committing the events here means the dedicated mesh-refinement sweeps
  // can realise them and re-apply the initial condition on the finer mesh before time
  // stepping starts.
  ::exahype2::RefinementControlService::getInstance().finishRefinementCriterionStep();
}

void {{ NAMESPACE | join("::") }}::finishPlottingStep() {
  {%- for item in SOLVERS %}
  {{ item[1] }}.finishPlottingStep();
  {%- endfor %}
}

void {{ NAMESPACE | join("::") }}::startSimulation() {
  // The refinement control service promotes patch-local erase requests to parent events by
  // anchoring Peano's sibling groups at the domain offset. Coarsening requires the consent
  // of every solver that evaluates a refinement criterion, so the service needs their count.
  ::exahype2::RefinementControlService::getInstance().setDomainOffset(DomainOffset);
  ::exahype2::RefinementControlService::getInstance().setNumberOfSolvers({{ NUMBER_OF_AMR_PARTICIPANTS }});

  {%- for item in SOLVERS %}
  {{ item[1] }}.startSimulation();
  {%- endfor %}
}

void {{ NAMESPACE | join("::") }}::finishSimulation() {
  {%- for item in SOLVERS %}
  {{ item[1] }}.finishSimulation();
  {%- endfor %}

  ::exahype2::RefinementControlService::getInstance().finishStep();
}
