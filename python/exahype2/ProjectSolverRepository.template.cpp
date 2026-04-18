// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SolverRepository.h"

#include "Constants.h"
#include "DataRepository.h"

#include "tarch/multicore/Core.h"
#include "tarch/accelerator/Device.h"

#include "peano4/parallel/SpacetreeSet.h"

#include "exahype2/exahype2.h"
#include "exahype2/LoadBalancingConfiguration.h"
#include "exahype2/RefinementControlService.h"

{% for item in TRACERS -%}
#include "vertexdata/{{ item }}Set.h"
#include "globaldata/{{ item }}.h"
{% endfor %}

{% for INCLUDE in ADDITIONAL_INCLUDES -%}
#include "{{INCLUDE}}"
{% endfor %}

#include <algorithm>

{% for item in NAMESPACE -%}
namespace {{ item }} {
{% endfor %}

int statisticsExchangeTag = tarch::mpi::Rank::reserveFreeTag("SolverRepository - statistics exchange tag");

tarch::logging::Log _log("{{NAMESPACE | join("::")}}::SolverRepository");

{% if LOAD_BALANCER_CONFIGURATION!="" %}
toolbox::loadbalancing::LoadBalancer loadBalancer({{LOAD_BALANCER_CONFIGURATION}});
{% else %}
toolbox::loadbalancing::LoadBalancer loadBalancer(new ::exahype2::LoadBalancingConfiguration);
{% endif %}

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

double getMinMeshSize() {
  double result = std::numeric_limits<double>::max();
  {% for item in SOLVERS -%}
  result = std::min( result, {{ item[1] }}.getMinMeshSize() );
  {# Comment to ensure that jinja2 adds a new line#}
  {%- endfor %}
  return result;
}

double getMaxMeshSize() {
  double result = 0.0;
  {% for item in SOLVERS -%}
  result = std::max( result, {{ item[1] }}.getMaxMeshSize() );
  {# Comment to ensure that jinja2 adds a new line#}
  {%- endfor %}
  return result;
}

double getMaxAdmissibleMeshSize() {
  double result = 0.0;
  {% for item in SOLVERS -%}
  result = std::max( result, {{ item[1] }}.getMaxAdmissibleMeshSize() );
  {# Comment to ensure that jinja2 adds a new line#}
  {%- endfor %}
  return result;
}

double getMinAdmissibleMeshSize() {
  double result = std::numeric_limits<double>::max();
  {% for item in SOLVERS -%}
  result = std::min( result, {{ item[1] }}.getMinAdmissibleMeshSize() );
  {# Comment to ensure that jinja2 adds a new line#}
  {%- endfor %}
  return result;
}

std::size_t getMaxNumberOfPatches() {
  const auto ratio = DomainSize[0] / std::min(getMinAdmissibleMeshSize(), DomainSize[0]);
  const std::size_t depth = static_cast<std::size_t>(std::round(std::log(ratio) / std::log(3.0))) + 1;
  const std::size_t patches = std::max(std::size_t{9}, static_cast<size_t>(std::pow(3, depth * DIMENSIONS)));
  return patches;
}

std::size_t getMaxNumberOfFaces() {
  const auto ratio = DomainSize[0] / std::min(getMinAdmissibleMeshSize(), DomainSize[0]);
  const std::size_t depth = static_cast<std::size_t>(std::round(std::log(ratio) / std::log(3.0))) + 1;
  const std::size_t facesPerDimension = std::max(std::size_t{12}, static_cast<size_t>(std::pow(3, depth * DIMENSIONS) + std::pow(3, depth)));
  return facesPerDimension * DIMENSIONS;
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

  loadBalancer.update();
  DataRepository::updateMemoryUsage();

  {% for item in TRACERS -%}
  logDebug("finishTimeStep()", vertexdata::{{ item }}Set::printParticleStateStatistics());
  {%- endfor %}
}

void finishGridConstructionStep() {
  {% for item in SOLVERS -%}
  {{ item[1] }}.finishGridConstructionStep();
  {# Comment to ensure that jinja2 adds a new line#}
  {%- endfor %}

  ::exahype2::RefinementControlService::getInstance().finishStep();
  loadBalancer.update();
}

void finishGridInitialisationStep() {
  {% for item in SOLVERS -%}
  {{ item[1] }}.finishGridInitialisationStep();
  {# Comment to ensure that jinja2 adds a new line#}
  {%- endfor %}

  loadBalancer.update();
}

void finishPlottingStep() {
  {% for item in SOLVERS -%}
  {{ item[1] }}.finishPlottingStep();
  {# Comment to ensure that jinja2 adds a new line#}
  {%- endfor %}
}

void logTimeStep() {
  {% for item in SOLVERS -%}
  {{ item[1] }}.logTimeStep();
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

  loadBalancer.update();
  ::exahype2::RefinementControlService::getInstance().finishStep();
}

void initLogFilters() {
  {% if LOG_FILTER_FILE!="" %}
  if (not tarch::logging::LogFilterFileReader::parsePlainTextFile("{{LOG_FILTER_FILE}}")) {
    // Manual selection, as log filters are not there yet
    if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
      logWarning("main()", "Filter file {{LOG_FILTER_FILE}} not found or file has been corrupted. Use default logging configuration." );
    }
    ::exahype2::setDefaultLogStatements();
  }
  {% else %}
  ::exahype2::setDefaultLogStatements();
  {% endif %}
}

{% for item in NAMESPACE -%}
}
{%- endfor %}
