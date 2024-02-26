#include "SolverRepository.h"

{% for N in NAMESPACE %}
namespace {{N}} {
{% endfor %}
  namespace repositories {
    {% for SOLVER in SOLVER_INSTANCES %}
    {{SOLVER[0]}}  {{SOLVER[1]}};
    {% endfor %}
    bool absValueWithinTolerance() {
    /*
    Return true if all solvers return true.
    */
      bool output = true;
      {% for SOLVER in SOLVER_INSTANCES %}
      output &= {{SOLVER[1]}}.absValueWithinTolerance();
      {% endfor %}
      return output;
    }
  }
{% for N in NAMESPACE %}
}
{% endfor %}

