#include "SolverRepository.h"

{% for N in NAMESPACE %}
namespace {{N}} {
{% endfor %}
  namespace repositories {
    {% for SOLVER in SOLVER_INSTANCES %}
    {{SOLVER[0]}}  {{SOLVER[1]}};
    {% endfor %}


    bool allSolversTerminationCriterionHolds() {
    /*
    Return true if all solvers return true.
    */
      bool output = true;
      {% for SOLVER in SOLVER_INSTANCES %}
      output &= {{SOLVER[1]}}.terminationCriterionHolds();
      {% endfor %}
      return output;
    }


    void plotAllSolversToInfo() {
      static tarch::logging::Log _log( "{{NAMESPACE | join("::")}}" );

      {% for SOLVER in SOLVER_INSTANCES %}
      logInfo( "plotAllSolversToInfo()", {{SOLVER[1]}}.toString() );
      {% endfor %}
    }

  }
{% for N in NAMESPACE %}
}
{% endfor %}

