#include "SolverRepository.h"



{% for N in NAMESPACE %}
namespace {{N}} {
{% endfor %}
  namespace repositories {
    {% for SOLVER in SOLVER_INSTANCES %}
    {{SOLVER[0]}}  {{SOLVER[1]}};
    {% endfor %}
  }
{% for N in NAMESPACE %}
}
{% endfor %}


void {{NAMESPACE | join("::")}}::repositories::computeLocalToGlobalMapsForAllSolvers() {
  {% for SOLVER in SOLVER_INSTANCES %}
  {{SOLVER[1]}}.getLocalToGlobalMap().computeLocalToGlobalMap();
  {% endfor %}
}


void {{NAMESPACE | join("::")}}::repositories::initMatricesAndVectors() {
  {% for SOLVER in SOLVER_INSTANCES %}
  {{SOLVER[1]}}.getLinearEquationSystem().init( {{SOLVER[1]}}.getLocalToGlobalMap().getTotalNumberOfIndices() );
  {% endfor %}
}


void {{NAMESPACE | join("::")}}::repositories::solve() {
  {% for SOLVER in SOLVER_INSTANCES %}
  {{SOLVER[1]}}.getLinearEquationSystem().solve({{PRECONDITIONER_TYPE}}, {{SOLVER_TYPE}});
  {% endfor %}
}

