//
// Generated through Peano's Python API
// www.peano-framework.org
//
#pragma once

#include "toolbox/loadbalancing/strategies/NoLoadBalancing.h"
{% if LOAD_BALANCER != "" %}
#include "{{LOAD_BALANCER | replace('::', '/') }}.h"
{% endif %}

{% for SOLVER in SOLVER_INSTANCES %}
#include "{{SOLVER[0]}}.h"
{% endfor %}
{% for SOLVER in AUX_SOLVER_INSTANCES %}
#include "{{SOLVER[0]}}.h"
{% endfor %}

{% for N in NAMESPACE %}
namespace {{N}} {
{% endfor %}
  namespace repositories {

    static int plotCounter = -1;

    {% for SOLVER in SOLVER_INSTANCES %}
    extern {{NAMESPACE | join("::")}}::{{SOLVER[0]}}  {{SOLVER[1]}};
    {% endfor %}
    // @todo remove these aux solvers
    {% for SOLVER in AUX_SOLVER_INSTANCES %}
    extern {{NAMESPACE | join("::")}}::{{SOLVER[0]}}  {{SOLVER[1]}};
    {% endfor %}

    {% if LOAD_BALANCER != "" %}
    extern {{LOAD_BALANCER}}  loadBalancer;
    {% else %}
    extern toolbox::loadbalancing::strategies::NoLoadBalancing  loadBalancer;
    {% endif %}
    
    bool terminationCriterionHolds();
    void beginMeshSweep();
    void endMeshSweep();

    /**
    * At the moment, these do nothing but call 
    * loadBalancer.finishStep()
    */
    void endCreateGrid();

    int getPlotCounter();
    void incrementPlotCounter();

{% for N in NAMESPACE %}
}
{% endfor %}

}
