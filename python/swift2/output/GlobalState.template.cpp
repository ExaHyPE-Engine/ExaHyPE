#include <algorithm>

#include "GlobalState.h"
#include "peano4/grid/grid.h"
#include "peano4/parallel/SpacetreeSet.h"



{% for item in NAMESPACE -%}
  namespace {{ item }} {
{%- endfor %}

int statisticsExchangeTag = tarch::mpi::Rank::reserveFreeTag("SolverRepository - statistics exchange tag");

tarch::logging::Log _log( "SolverRepository" );

{% if LOAD_BALANCER!="" -%}
{{LOAD_BALANCER}}              loadBalancer({{LOAD_BALANCER_ARGUMENTS}});
{% else -%}
toolbox::loadbalancing::NoLoadBalancing  loadBalancer;
{% endif -%}



{% for item in PARTICLE_SPECIES -%}
{{ item[0] }} {{ item[1] }};
{%- endfor %}


double getMinTimeStamp() {
  double result = std::numeric_limits<double>::max();
  {% for item in PARTICLE_SPECIES -%}
    result = std::min( result, {{ item[1] }}.getMinTimeStamp(ofCurrentlyRunningGridSweep) );
  {%- endfor %}
  // @todo change
  return 1.0;
  return result;
}


double getMaxTimeStamp() {
  double result = 0.0;
  {% for item in PARTICLE_SPECIES -%}
    result = std::max( result, {{ item[1] }}.getMaxTimeStamp(ofCurrentlyRunningGridSweep) );
  {%- endfor %}
  return result;
}


double getMinMeshSize() {
  double result = std::numeric_limits<double>::max();
  {% for item in PARTICLE_SPECIES -%}
    result = std::min( result, {{ item[1] }}.getMinMeshSize() );
  {%- endfor %}
  return result;
}


double getMaxMeshSize() {
  double result = 0.0;
  {% for item in PARTICLE_SPECIES -%}
    result = std::max( result, {{ item[1] }}.getMaxMeshSize() );
  {%- endfor %}
  return result;
}


double getMinTimeStepSize() {
  double result = std::numeric_limits<double>::max();
  {% for item in PARTICLE_SPECIES -%}
    result = std::min( result, {{ item[1] }}.getMinTimeStepSize() );
  {%- endfor %}
  return result;
}


double getMaxTimeStepSize() {
  double result = 0.0;
  {% for item in PARTICLE_SPECIES -%}
  result = std::max( result, {{ item[1] }}.getMaxTimeStepSize() );
  {%- endfor %}
  return result;
}


void startGridConstructionStep() {
  {% for item in PARTICLE_SPECIES -%}
  {{ item[1] }}.startGridConstructionStep();
  {%- endfor %}
}


void startInitialisationStep() {
  {% for item in PARTICLE_SPECIES -%}
  {{ item[1] }}.startGridInitialisationStep();
  {%- endfor %}
}


void startTimeStep() {
  {% for item in PARTICLE_SPECIES -%}
  {{ item[1] }}.startTimeStep(minTimeStamp,maxTimeStamp,minTimeStepSize,maxTimeStepSize);
  {%- endfor %}
}


void startPlotStep() {
  {% for item in PARTICLE_SPECIES -%}
  {{ item[1] }}.startPlottingStep(minTimeStamp,maxTimeStamp,minTimeStepSize,maxTimeStepSize);
  {%- endfor %}
}


void finishTimeStep() {
  {% for item in PARTICLE_SPECIES -%}
  {{ item[1] }}.finishTimeStep();
  {%- endfor %}

  loadBalancer.finishStep();

  #ifdef UseSmartMPI
  smartmpi::tick();
  #endif
}


void finishGridConstructionStep() {
  {% for item in PARTICLE_SPECIES -%}
  {{ item[1] }}.finishGridConstructionStep();
  vertexdata::{{ item }}::finishedTraversal();
  {%- endfor %}

  loadBalancer.finishStep();
}


void finishInitialisationStep() {
  {% for item in PARTICLE_SPECIES -%}
  {{ item[1] }}.finishGridInitialisationStep();
  vertexdata::{{ item }}::finishedTraversal();
  {%- endfor %}

  loadBalancer.finishStep();
}


void finishPlotStep() {
  {% for item in PARTICLE_SPECIES -%}
  {{ item[1] }}.finishPlottingStep();

  vertexdata::{{ item }}::finishedTraversal();
  {%- endfor %}
}

{% for item in NAMESPACE -%}
  }

{%- endfor %}

