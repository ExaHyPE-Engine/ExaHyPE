#include "SolverRepository.h"

#include <algorithm>



{% for item in NAMESPACE -%}
  namespace {{ item }} {

{%- endfor %}


::exahype2::RefinementControl  refinementControl;


{% if LOAD_BALANCER!="" -%}
{{LOAD_BALANCER}}              loadBalancer;
{% endif -%}

{% for item in SOLVERS -%}
{{ item[0] }} {{ item[1] }};
{%- endfor %}



double getMinTimeStamp() {
  double result = std::numeric_limits<double>::max();
  {% for item in SOLVERS -%}
  result = std::min( result, {{ item[1] }}.getMinTimeStamp() );
  {%- endfor %}
  return result;
}


double getMaxTimeStamp() {
  double result = 0.0;
  {% for item in SOLVERS -%}
  result = std::max( result, {{ item[1] }}.getMaxTimeStamp() );
  {%- endfor %}
  return result;
}


double getMinTimeStepSize() {
  double result = std::numeric_limits<double>::max();
  {% for item in SOLVERS -%}
  result = std::min( result, {{ item[1] }}.getMinTimeStepSize() );
  {%- endfor %}
  return result;
}


double getMaxTimeStepSize() {
  double result = 0.0;
  {% for item in SOLVERS -%}
  result = std::max( result, {{ item[1] }}.getMaxTimeStepSize() );
  {%- endfor %}
  return result;
}


void startGridConstructionStep() {
  {% for item in SOLVERS -%}
  {{ item[1] }}.startGridConstructionStep();
  {%- endfor %}
}


void startGridInitialisationStep() {
  {% for item in SOLVERS -%}
  {{ item[1] }}.startGridInitialisationStep();
  {%- endfor %}
}


void startTimeStep(
  double minTimeStamp,
  double maxTimeStamp,
  double minTimeStepSize,
  double maxTimeStepSize
) {
  {% for item in SOLVERS -%}
  {{ item[1] }}.startTimeStep(minTimeStamp,maxTimeStamp,minTimeStepSize,maxTimeStepSize);
  {%- endfor %}
}


void startPlottingStep(
  double minTimeStamp,
  double maxTimeStamp,
  double minTimeStepSize,
  double maxTimeStepSize
) {
  {% for item in SOLVERS -%}
  {{ item[1] }}.startPlottingStep(minTimeStamp,maxTimeStamp,minTimeStepSize,maxTimeStepSize);
  {%- endfor %}
}


void finishTimeStep() {
  {% for item in SOLVERS -%}
  {{ item[1] }}.finishTimeStep();
  {%- endfor %}

  refinementControl.finishStep();

  bool mayUpdateLoadBalancing = true;
  {% for item in SOLVERS -%}
    mayUpdateLoadBalancing &= {{ item[1] }}.mayUpdateLoadBalancing();
  {%- endfor %}

  if (mayUpdateLoadBalancing) {
    loadBalancer.finishStep();
  }
}


void finishGridConstructionStep() {
  {% for item in SOLVERS -%}
  {{ item[1] }}.finishGridConstructionStep();
  {%- endfor %}

  refinementControl.finishStep();
  loadBalancer.finishStep();
}


void finishGridInitialisationStep() {
  {% for item in SOLVERS -%}
  {{ item[1] }}.finishGridInitialisationStep();
  {%- endfor %}
}


void finishPlottingStep() {
  {% for item in SOLVERS -%}
  {{ item[1] }}.finishPlottingStep();
  {%- endfor %}
}


void finishSimulation() {
  refinementControl.finishStep();
}


{% for item in NAMESPACE -%}
  }

{%- endfor %}

