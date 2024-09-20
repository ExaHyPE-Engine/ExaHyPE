#include "SolverRepository.h"

{% for N in NAMESPACE %}
namespace {{N}} {
{% endfor %}
  namespace repositories {
    {% for SOLVER in SOLVER_INSTANCES %}
    {{SOLVER[0]}}  {{SOLVER[1]}};
    {% endfor %}


    bool terminationCriterionHolds() {
    /*
    Return true if all solvers return true.
    */
      bool output = true;
      {% for SOLVER in SOLVER_INSTANCES %}
      output &= {{SOLVER[1]}}.terminationCriterionHolds();
      {% endfor %}
      return output;
    }


    void beginMeshSweep() {
      static tarch::logging::Log _log( "{{NAMESPACE | join("::")}}" );

      {% for SOLVER in SOLVER_INSTANCES %}
      {{SOLVER[1]}}.beginMeshSweep();
      {% endfor %}
    }

    void endMeshSweep() {
      static tarch::logging::Log _log( "{{NAMESPACE | join("::")}}" );

      {% for SOLVER in SOLVER_INSTANCES %}
      {{SOLVER[1]}}.endMeshSweep();
      {% endfor %}
    }
  }
{% for N in NAMESPACE %}
}
{% endfor %}

