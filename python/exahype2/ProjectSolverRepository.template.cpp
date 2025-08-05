// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SolverRepository.h"

#include "Constants.h"

#include "tarch/multicore/Core.h"
#include "tarch/accelerator/Device.h"

#include "peano4/grid/grid.h"
#include "peano4/parallel/SpacetreeSet.h"

#include "exahype2/LoadBalancingConfiguration.h"
#include "exahype2/RefinementControlService.h"
#include "exahype2/UserInterface.h"

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

::exahype2::PlotFilter         plotFilter;
peano4::grid::GridStatistics   gridStatisticsAfterGridConstruction;

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
  bool result = true;
  {% for item in SOLVERS -%}
  result &= {{ item[1] }}.isFirstGridSweepOfTimeStep();
  {# Comment to ensure that jinja2 adds a new line#}
  {%- endfor %}
  return result;
}

bool isLastGridSweepOfTimeStep() {
  bool result = true;
  {% for item in SOLVERS -%}
  result &= {{ item[1] }}.isLastGridSweepOfTimeStep();
  {# Comment to ensure that jinja2 adds a new line#}
  {%- endfor %}
  return result;
}

double getMinTimeStamp(bool ofCurrentlyRunningGridSweep) {
  double result = std::numeric_limits<double>::max();
  {% for item in SOLVERS -%}
  result = std::min( result, {{ item[1] }}.getMinTimeStamp(ofCurrentlyRunningGridSweep) );
  {# Comment to ensure that jinja2 adds a new line#}
  {%- endfor %}
  return result;
}

double getMaxTimeStamp(bool ofCurrentlyRunningGridSweep) {
  double result = 0.0;
  {% for item in SOLVERS -%}
  result = std::max( result, {{ item[1] }}.getMaxTimeStamp(ofCurrentlyRunningGridSweep) );
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
  double result = 0;
  {% for item in SOLVERS -%}
  result = std::max( result, {{ item[1] }}.getMaxMeshSize() );
  {# Comment to ensure that jinja2 adds a new line#}
  {%- endfor %}
  return result;
}

double getMaxAdmissibleMeshSize() {
  double result = std::numeric_limits<double>::max();
  {% for item in SOLVERS -%}
  result = std::min( result, {{ item[1] }}.MaxAdmissibleCellH );
  {# Comment to ensure that jinja2 adds a new line#}
  {%- endfor %}
  return result;
}

double getMinAdmissibleMeshSize() {
  double result = 0.0;
  {% for item in SOLVERS -%}
  result = std::max( result, {{ item[1] }}.MinAdmissibleCellH );
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

  {% for item in TRACERS -%}
  logInfo("finishTimeStep()", vertexdata::{{ item }}Set::printParticleStateStatistics());
  {%- endfor %}
}

void finishGridConstructionStep() {
  {% for item in SOLVERS -%}
  {{ item[1] }}.finishGridConstructionStep();
  {# Comment to ensure that jinja2 adds a new line#}
  {%- endfor %}

  ::exahype2::RefinementControlService::getInstance().finishStep();
  loadBalancer.update();

  gridStatisticsAfterGridConstruction = peano4::parallel::SpacetreeSet::getInstance().getGridStatistics();
  #ifdef WITH_MPI
  if ( tarch::mpi::Rank::getInstance().isGlobalMaster() ) {
    for (int rank=0; rank<tarch::mpi::Rank::getInstance().getNumberOfRanks(); rank++ ) {
      if (rank!=tarch::mpi::Rank::getGlobalMasterRank()) {
        peano4::grid::GridStatistics rankLocalStats;
        logDebug("finishGridConstructionStep()", "Receive grid statistics from rank " << rank << " via tag " << statisticsExchangeTag);
        peano4::grid::GridStatistics::receive(
          rankLocalStats,
          rank,
          statisticsExchangeTag,
          [&]() {
            tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
            tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
          },
          [&]() {
            tarch::mpi::Rank::getInstance().writeTimeOutWarning("{{NAMESPACE | join("::")}}", "finishGridConstructionStep()", rank, statisticsExchangeTag);
            tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut("{{NAMESPACE | join("::")}}", "finishGridConstructionStep()", rank, statisticsExchangeTag);
            tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
          },
          tarch::mpi::Rank::getInstance().getCommunicator()
        );
        gridStatisticsAfterGridConstruction = gridStatisticsAfterGridConstruction + rankLocalStats;
      }
    }
  } else {
    logDebug("finishGridConstructionStep()", "Send out local statistics " << gridStatisticsAfterGridConstruction.toString() << " to global master via tag " << statisticsExchangeTag);
    peano4::grid::GridStatistics::send(
      gridStatisticsAfterGridConstruction,
      tarch::mpi::Rank::getInstance().getGlobalMasterRank(),
      statisticsExchangeTag,
      [&]() {
        tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
        tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
      },
      [&]() {
        tarch::mpi::Rank::getInstance().writeTimeOutWarning( "", "receive()", tarch::mpi::Rank::getInstance().getGlobalMasterRank(), statisticsExchangeTag );
        tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "", "receive()", tarch::mpi::Rank::getInstance().getGlobalMasterRank(), statisticsExchangeTag );
        tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
      },
      tarch::mpi::Rank::getInstance().getCommunicator()
    );
  }
  #endif
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

  plotFilter.finishPlottingStep();
}

void printTimeStep(bool ofCurrentlyRunningGridSweep) {
  {% for item in SOLVERS -%}
  {{ item[1] }}.printTimeStep(ofCurrentlyRunningGridSweep);
  {# Comment to ensure that jinja2 adds a new line#}
  {%- endfor %}
}

void startSimulation() {
  {% for item in PLOT_FILTER -%}
  plotFilter.addFilter(
    {{item}}
  );
  {% endfor %}
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
