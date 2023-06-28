#include <algorithm>

#include "GlobalState.h"

#include "peano4/grid/grid.h"
#include "peano4/parallel/SpacetreeSet.h"


#include "swift2/UserInterface.h"


{% for PARTICLE in PARTICLE_SPECIES -%}
#include "globaldata/{{ PARTICLE }}.h"
#include "vertexdata/{{ PARTICLE }}Set.h"
{% endfor %}


{% for item in NAMESPACE -%}
  namespace {{ item }} {
{%- endfor %}


tarch::logging::Log _log( "{{NAMESPACE | join("::")}}::GlobalState" );

{% if LOAD_BALANCER!="" -%}
{{LOAD_BALANCER}}              loadBalancer({{LOAD_BALANCER_ARGUMENTS}});
{% else -%}
toolbox::loadbalancing::NoLoadBalancing  loadBalancer;
{% endif -%}


bool rerunPreviousGridSweep() {
  bool result = false;
  {% for PARTICLE in PARTICLE_SPECIES -%}
  result |= globaldata::{{PARTICLE}}::getSpecies().rerunPreviousGridSweep();
  {%- endfor %}
  return result;
}


double getMinTimeStamp() {
  double result = std::numeric_limits<double>::max();
  {% for PARTICLE in PARTICLE_SPECIES -%}
  result = std::min( result, globaldata::{{PARTICLE}}::getSpecies().getMinTimeStamp() );
  {%- endfor %}
  return result;
}


double getMaxTimeStamp() {
  double result = 0.0;
  {% for PARTICLE in PARTICLE_SPECIES -%}
  result = std::max( result, globaldata::{{PARTICLE}}::getSpecies().getMaxTimeStamp() );
  {%- endfor %}
  return result;
}


double getMinSearchRadius() {
  double result = std::numeric_limits<double>::max();
  {% for PARTICLE in PARTICLE_SPECIES -%}
  result = std::min( result, globaldata::{{PARTICLE}}::getSpecies().getMinSearchRadius() );
  {%- endfor %}
  return result;
}


double getMaxSearchRadius() {
  double result = 0.0;
  {% for PARTICLE in PARTICLE_SPECIES -%}
  result = std::max( result, globaldata::{{PARTICLE}}::getSpecies().getMaxSearchRadius() );
  {%- endfor %}
  return result;
}


double getMinTimeStepSize() {
  double result = std::numeric_limits<double>::max();
  {% for PARTICLE in PARTICLE_SPECIES -%}
  result = std::min( result, globaldata::{{PARTICLE}}::getSpecies().getMinTimeStepSize() );
  {%- endfor %}
  return result;
}


double getMaxTimeStepSize() {
  double result = 0.0;
  {% for PARTICLE in PARTICLE_SPECIES -%}
  result = std::max( result, globaldata::{{PARTICLE}}::getSpecies().getMaxTimeStepSize() );
  {%- endfor %}
  return result;
}


void startGridConstructionStep() {
  logTraceIn( "GridConstructionStep()" );
}


void startIntermediateStep() {
  logTraceIn( "IntermediateStep()" );
}


void startInitialConditionStep() {
  if (not ::swift2::commandlinesettings::enableDynamicLoadBalancing() ) {
    loadBalancer.enable(false);
  }
  logTraceIn( "InitialConditionStep()" );
}


void startInitialisationStep() {
  if (not ::swift2::commandlinesettings::enableDynamicLoadBalancing() ) {
    loadBalancer.enable(false);
  }
  logTraceIn( "InitialisationStep()" );
}


void startTimeStep() {
  {% for PARTICLE in PARTICLE_SPECIES -%}
  globaldata::{{PARTICLE}}::getSpecies().startTimeStep();
  {%- endfor %}
  logTraceIn( "TimeStep()" );
}


void startPlotStep() {
  logTraceIn( "PlotStep()" );
}


void finishTimeStep() {
  {% for PARTICLE in PARTICLE_SPECIES -%}
  globaldata::{{PARTICLE}}::getSpecies().finishTimeStep();
  vertexdata::{{PARTICLE}}Set::finishedTraversal();
  vertexdata::{{PARTICLE}}Set::reduceAndPrintStatistics();
  {%- endfor %}

  loadBalancer.finishStep();

  logInfo("TimeStep()", "min mesh size=" << ::peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getMinH()(0) );
  logInfo("TimeStep()",
    "#local cells=(" << ::peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalUnrefinedCells() <<
    "/" << ::peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalRefinedCells() <<
    ")\t(unrefined/refined)"
   );
  logInfo("TimeStep()",
    "#remote cells=(" << ::peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfRemoteUnrefinedCells() <<
    "/" << ::peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfRemoteRefinedCells() <<
    ")\t(unrefined/refined)"
   );

  #ifdef UseSmartMPI
  smartmpi::tick();
  #endif
  logTraceOut( "TimeStep()" );
}


void finishGridConstructionStep() {
  {% for PARTICLE in PARTICLE_SPECIES -%}
  vertexdata::{{PARTICLE}}Set::finishedTraversal();
  {%- endfor %}

  loadBalancer.finishStep();
  logTraceOut( "GridConstructionStep()" );
}


void finishInitialisationStep() {
  {% for PARTICLE in PARTICLE_SPECIES -%}
  vertexdata::{{PARTICLE}}Set::finishedTraversal();
  vertexdata::{{PARTICLE}}Set::reduceAndPrintStatistics();
  {%- endfor %}

  loadBalancer.finishStep();
  logTraceOut( "InitialisationStep()" );
}


void finishInitialConditionStep() {
  {% for PARTICLE in PARTICLE_SPECIES -%}
  vertexdata::{{PARTICLE}}Set::finishedTraversal();
  vertexdata::{{PARTICLE}}Set::reduceAndPrintStatistics();
  {%- endfor %}

  loadBalancer.finishStep();
  logTraceOut( "InitialConditionStep()" );
}


void finishPlotStep() {
  {% for PARTICLE in PARTICLE_SPECIES -%}
  vertexdata::{{PARTICLE}}Set::finishedTraversal();
  {%- endfor %}
  logTraceOut( "PlotStep()" );
}


void finishIntermediateStep() {
  {% for PARTICLE in PARTICLE_SPECIES -%}
  vertexdata::{{PARTICLE}}Set::finishedTraversal();
  {%- endfor %}
  logTraceOut( "IntermediateStep()" );
}

{% for item in NAMESPACE -%}
  }

{%- endfor %}

