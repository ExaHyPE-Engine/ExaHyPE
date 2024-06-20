//
// Generated through Peano's Python API
// www.peano-framework.org
//
#pragma once


{% for SOLVER in SOLVER_INSTANCES %}

#include "{{SOLVER[0]}}.h"

{% endfor %}

{% for N in NAMESPACE %}
namespace {{N}} {
{% endfor %}
  namespace repositories {

    {% for SOLVER in SOLVER_INSTANCES %}
    extern {{NAMESPACE | join("::")}}::{{SOLVER[0]}}  {{SOLVER[1]}};

    {% endfor %}
    
    bool terminationCriterionHolds();
    void beginMeshSweep();
    void endMeshSweep();

{% for N in NAMESPACE %}
}
{% endfor %}

}
