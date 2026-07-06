// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SolverRepository.h"

#include "exahype2/RefinementControlService.h"

#include "tarch/instrumentation/Debug.h"
#include "tarch/mpi/Rank.h"

{% for item in TRACERS -%}
#include "vertexdata/{{ item }}Set.h"
{% endfor %}

{% for INCLUDE in ADDITIONAL_INCLUDES -%}
#include "{{INCLUDE}}"
{% endfor %}

#include <algorithm>
#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

{% for item in NAMESPACE -%}
namespace {{ item }} {
{% endfor %}

int statisticsExchangeTag = tarch::mpi::Rank::reserveFreeTag("SolverRepository - statistics exchange tag");

{% for item in SOLVERS -%}
{{ item[0] }} {{ item[1] }};
{# Comment to ensure that jinja2 adds a new line#}
{%- endfor %}

bool mayPlot() {
  bool result = true;
  {% for item in SOLVERS -%}
  result &= {{ item[1] }}.mayPlot();
  {# Comment to ensure that jinja2 adds a new line#}
  {%- endfor %}
  return result;
}

std::string getSolverStateDescriptions() {
  std::ostringstream result;
  {% for item in SOLVERS -%}
  if (not result.str().empty()) result << ", ";
  result << "{{ item[1] }}:" << {{ item[1] }}.toString( {{ item[1] }}.getSolverState() );
  {# Comment to ensure that jinja2 adds a new line#}
  {%- endfor %}
  return result.str();
}

std::string formatSolverStatisticRange(double minValue, double maxValue) {
  std::ostringstream result;
  if (minValue == maxValue) {
    result << minValue;
  } else {
    result << "[" << minValue << ", " << maxValue << "]";
  }
  return result.str();
}

template <typename Value>
void appendSolverStatistic(std::ostringstream& result, const char* label, const Value& value) {
  if (not result.str().empty()) {
    result << ", ";
  }
  result << label << "=" << value;
}

template <typename Solver>
auto appendMaxEigenvalue(std::ostringstream& result, const Solver& solver, int) -> decltype(solver.getMaxEigenvalue(), void()) {
  appendSolverStatistic(result, "λ_{max}", solver.getMaxEigenvalue());
}

template <typename Solver>
void appendMaxEigenvalue(std::ostringstream&, const Solver&, long) {}

template <typename Solver>
auto appendTroubledCells(std::ostringstream& result, const Solver& solver, int) -> decltype(solver.getNumberOfTroubledCells(), void()) {
  if (solver.getNumberOfTroubledCells() > 0) {
    appendSolverStatistic(result, "Troubled", solver.getNumberOfTroubledCells());
  }
}

template <typename Solver>
void appendTroubledCells(std::ostringstream&, const Solver&, long) {}

void appendParticleCount(std::ostringstream& result, const char* particleName, int numberOfParticles) {
  appendSolverStatistic(result, particleName, numberOfParticles);
}

template <typename Solver, typename AppendAdditionalStatistics>
std::pair<std::string, std::string> makeSolverStatistics(const char* solverName, const Solver& solver, AppendAdditionalStatistics appendAdditionalStatistics) {
  std::ostringstream statistics;
  appendSolverStatistic(statistics, "dt", solver.getMinTimeStepSize());
  appendSolverStatistic(statistics, "h", formatSolverStatisticRange(solver.getMinCellSize(), solver.getMaxCellSize()));
  appendAdditionalStatistics(statistics);
  appendMaxEigenvalue(statistics, solver, 0);
  appendTroubledCells(statistics, solver, 0);
  return {solverName, statistics.str()};
}

std::pair<std::string, std::string> makeParticleStatistics(const char* particleName, int numberOfParticles) {
  std::ostringstream statistics;
  appendSolverStatistic(statistics, "Particles", numberOfParticles);
  return {particleName, statistics.str()};
}

std::vector<std::pair<std::string, std::string>> getSolverStatistics() {
  std::vector<std::pair<std::string, std::string>> result;
  {% for item in SOLVERS -%}
  {% if item[2] | length > 0 -%}
  result.push_back(makeSolverStatistics("{{ item[0] }}", {{ item[1] }}, [](std::ostringstream& result) {
    {% for tracer in item[2] -%}
    appendParticleCount(result, "{{ tracer }}", vertexdata::{{ tracer }}Set::getTotalNumberOfParticles());
    {# Comment to ensure that jinja2 adds a new line#}
    {%- endfor %}
  }));
  {% else -%}
  result.push_back(makeSolverStatistics("{{ item[0] }}", {{ item[1] }}, [](std::ostringstream&) {}));
  {%- endif %}
  {# Comment to ensure that jinja2 adds a new line#}
  {%- endfor %}
  const auto particleStatistics = getParticleStatistics();
  result.insert(result.end(), particleStatistics.begin(), particleStatistics.end());
  return result;
}

std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>> getConfigurationSummary() {
  std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>> result;
  {% for item in SOLVERS -%}
  {
    std::vector<std::pair<std::string, std::string>> entries;
    {% for statistic in item[3] -%}
    entries.emplace_back("{{ statistic[0] }}", "{{ statistic[1] }}");
    {# Comment to ensure that jinja2 adds a new line#}
    {%- endfor %}
    entries.emplace_back("Degrees of Freedom", std::to_string({{ item[1] }}.getNumberOfDoFs()));
    result.emplace_back("Solver \"{{ item[0] }}\"", entries);
  }
  {# Comment to ensure that jinja2 adds a new line#}
  {%- endfor %}
  return result;
}

std::vector<std::pair<std::string, std::string>> getParticleStatistics() {
  std::vector<std::pair<std::string, std::string>> result;
  {% for item in PROJECT_TRACERS -%}
  result.push_back(makeParticleStatistics("{{ item }}", vertexdata::{{ item }}Set::getTotalNumberOfParticles()));
  {# Comment to ensure that jinja2 adds a new line#}
  {%- endfor %}
  return result;
}

bool isFirstGridSweepOfTimeStep() {
  bool result = false;
  {% for item in SOLVERS -%}
  result |= {{ item[1] }}.isFirstGridSweepOfTimeStep();
  {# Comment to ensure that jinja2 adds a new line#}
  {%- endfor %}
  return result;
}

bool isLastGridSweepOfTimeStep() {
  bool result = false;
  {% for item in SOLVERS -%}
  result |= {{ item[1] }}.isLastGridSweepOfTimeStep();
  {# Comment to ensure that jinja2 adds a new line#}
  {%- endfor %}
  return result;
}

double getMinTimeStamp() {
  double result = std::numeric_limits<double>::max();
  {% for item in SOLVERS -%}
  result = std::min(result, {{ item[1] }}.getMinTimeStamp());
  {# Comment to ensure that jinja2 adds a new line#}
  {%- endfor %}
  return result;
}

double getMaxTimeStamp() {
  double result = 0.0;
  {% for item in SOLVERS -%}
  result = std::max(result, {{ item[1] }}.getMaxTimeStamp());
  {# Comment to ensure that jinja2 adds a new line#}
  {%- endfor %}
  return result;
}

double getMinTimeStepSize() {
  double result = std::numeric_limits<double>::max();
  {% for item in SOLVERS -%}
  result = std::min( result, {{ item[1] }}.getMinTimeStepSize() );
  {# Comment to ensure that jinja2 adds a new line#}
  {%- endfor %}
  return result;
}

double getMaxTimeStepSize() {
  double result = 0.0;
  {% for item in SOLVERS -%}
  result = std::max( result, {{ item[1] }}.getMaxTimeStepSize() );
  {# Comment to ensure that jinja2 adds a new line#}
  {%- endfor %}
  return result;
}

void suspendSolversForOneGridSweep() {
  {% for item in SOLVERS -%}
  {{ item[1] }}.suspendSolversForOneGridSweep();
  {# Comment to ensure that jinja2 adds a new line#}
  {%- endfor %}
}

int getNumberOfDoFs() {
  int numberOfDoFs = 0;
  {% for item in SOLVERS -%}
  numberOfDoFs += {{ item[1] }}.getNumberOfDoFs();
  {# Comment to ensure that jinja2 adds a new line#}
  {%- endfor %}
  return numberOfDoFs;
}

void startGridConstructionStep() {
  {% for item in SOLVERS -%}
  {{ item[1] }}.startGridConstructionStep();
  {# Comment to ensure that jinja2 adds a new line#}
  {%- endfor %}
}

void startGridInitialisationStep() {
  {% for item in SOLVERS -%}
  {{ item[1] }}.startGridInitialisationStep();
  {# Comment to ensure that jinja2 adds a new line#}
  {%- endfor %}
}

void startTimeStep() {
  {% for item in SOLVERS -%}
  {{ item[1] }}.startTimeStep();
  {# Comment to ensure that jinja2 adds a new line#}
  {%- endfor %}
}

void startPlottingStep() {
  {% for item in SOLVERS -%}
  {{ item[1] }}.startPlottingStep();
  {# Comment to ensure that jinja2 adds a new line#}
  {%- endfor %}
}

void finishTimeStep() {
  {% for item in SOLVERS -%}
  {{ item[1] }}.finishTimeStep();
  {# Comment to ensure that jinja2 adds a new line#}
  {%- endfor %}

  ::exahype2::RefinementControlService::getInstance().finishStep();

  {% for item in TRACERS -%}
  vertexdata::{{ item }}Set::reduceNumberOfParticles();
  logDebug(vertexdata::{{ item }}Set::printParticleStateStatistics());
  {%- endfor %}
}

void finishGridConstructionStep() {
  {% for item in SOLVERS -%}
  {{ item[1] }}.finishGridConstructionStep();
  {# Comment to ensure that jinja2 adds a new line#}
  {%- endfor %}

  ::exahype2::RefinementControlService::getInstance().finishStep();
}

void finishGridInitialisationStep() {
  {% for item in SOLVERS -%}
  {{ item[1] }}.finishGridInitialisationStep();
  {# Comment to ensure that jinja2 adds a new line#}
  {%- endfor %}
}

void finishPlottingStep() {
  {% for item in SOLVERS -%}
  {{ item[1] }}.finishPlottingStep();
  {# Comment to ensure that jinja2 adds a new line#}
  {%- endfor %}
}

void startSimulation() {
  {% for item in SOLVERS -%}
  {{ item[1] }}.startSimulation();
  {# Comment to ensure that jinja2 adds a new line#}
  {%- endfor %}
}

void finishSimulation() {
  {% for item in SOLVERS -%}
  {{ item[1] }}.finishSimulation();
  {# Comment to ensure that jinja2 adds a new line#}
  {%- endfor %}

  ::exahype2::RefinementControlService::getInstance().finishStep();
}

{% for item in NAMESPACE -%}
}
{%- endfor %}
