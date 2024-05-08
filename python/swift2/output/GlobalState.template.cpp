#include <algorithm>

#include "GlobalState.h"
#include "Constants.h"

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
toolbox::loadbalancing::strategies::NoLoadBalancing  loadBalancer;
{% endif -%}


bool rerunPreviousGridSweep() {
  bool result = false;
  {% for PARTICLE in PARTICLE_SPECIES -%}
  result |= vertexdata::{{PARTICLE}}Set::getNumberOfDropsIntoHorizontalTreeDecomposition()>0;

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

  {% for PARTICLE in PARTICLE_SPECIES -%}
  vertexdata::{{PARTICLE}}Set::clearReassignmentStatistics();
  {%- endfor %}
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
  logTraceIn( "startTimeStep()" );
  {% for PARTICLE in PARTICLE_SPECIES -%}
  globaldata::{{PARTICLE}}::getSpecies().startTimeStep();

  vertexdata::{{PARTICLE}}Set::clearReassignmentStatistics();
  vertexdata::{{PARTICLE}}Set::clearParticleStateStatistics();
  {%- endfor %}
  logTraceOut( "startTimeStep()" );
}


void startPlotStep() {
  logTraceIn( "PlotStep()" );
}


void finishTimeStep() {
  logTraceIn( "finishTimeStep()" );

  {% for PARTICLE in PARTICLE_SPECIES -%}
  globaldata::{{PARTICLE}}::getSpecies().finishTimeStep();
  vertexdata::{{PARTICLE}}Set::finishedTraversal(DomainOffset, DomainSize, PeriodicBC);
  vertexdata::{{PARTICLE}}Set::reduceParticleStateStatistics();
  logInfo("finishTimeStep()", vertexdata::{{PARTICLE}}Set::printParticleStateStatistics() );
  {%- endfor %}

  loadBalancer.finishStep();

  logInfo("finishTimeStep()", "min mesh size=" << ::peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getMinH()(0) );
  logInfo("finishTimeStep()",
    "#local cells=(" << ::peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalUnrefinedCells() <<
    "/" << ::peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalRefinedCells() <<
    ")\t(unrefined/refined)"
   );
  logInfo("finishTimeStep()",
    "#remote cells=(" << ::peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfRemoteUnrefinedCells() <<
    "/" << ::peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfRemoteRefinedCells() <<
    ")\t(unrefined/refined)"
   );

  #ifdef UseSmartMPI
  smartmpi::tick();
  #endif
  logTraceOut( "finishTimeStep()" );
}


void finishGridConstructionStep() {
  {% for PARTICLE in PARTICLE_SPECIES -%}
  vertexdata::{{PARTICLE}}Set::finishedTraversal(DomainOffset, DomainSize, PeriodicBC);
  {%- endfor %}

  loadBalancer.finishStep();
  logTraceOut( "GridConstructionStep()" );
}


void finishInitialisationStep() {
  {% for PARTICLE in PARTICLE_SPECIES -%}
  vertexdata::{{PARTICLE}}Set::finishedTraversal(DomainOffset, DomainSize, PeriodicBC);

  vertexdata::{{PARTICLE}}Set::reduceReassignmentStatistics();
  vertexdata::{{PARTICLE}}Set::reduceParticleStateStatistics();
  {%- endfor %}

  loadBalancer.finishStep();
  logTraceOut( "InitialisationStep()" );
}


void finishInitialConditionStep() {
  {% for PARTICLE in PARTICLE_SPECIES -%}
  vertexdata::{{PARTICLE}}Set::finishedTraversal(DomainOffset, DomainSize, PeriodicBC);

  vertexdata::{{PARTICLE}}Set::clearReassignmentStatistics();
  vertexdata::{{PARTICLE}}Set::clearParticleStateStatistics();
  {%- endfor %}

  loadBalancer.finishStep();
  logTraceOut( "InitialConditionStep()" );
}


void finishPlotStep() {
  {% for PARTICLE in PARTICLE_SPECIES -%}
  vertexdata::{{PARTICLE}}Set::finishedTraversal(DomainOffset, DomainSize, PeriodicBC);
  {%- endfor %}
  logTraceOut( "PlotStep()" );
}


void finishIntermediateStep() {
  {% for PARTICLE in PARTICLE_SPECIES -%}
  vertexdata::{{PARTICLE}}Set::finishedTraversal(DomainOffset, DomainSize, PeriodicBC);

  vertexdata::{{PARTICLE}}Set::reduceReassignmentStatistics();
  if (vertexdata::{{PARTICLE}}Set::registeredAnyResorting()) {
    logInfo( "finishIntermediateStep()", vertexdata::{{PARTICLE}}Set::printReassignmentStatistics() );
  }
  {%- endfor %}
  logTraceOut( "IntermediateStep()" );
}

{% for item in NAMESPACE -%}
  }

{%- endfor %}

