#include "SolverRepository.h"



{% for N in NAMESPACE %}
namespace {{N}} {
{% endfor %}
  namespace repositories {
    {% for SOLVER in SOLVER_INSTANCES %}
    {{SOLVER[0]}}  {{SOLVER[1]}};
    {% endfor %}
    // ::petsc::PetscData instanceOfPetsc;
    PetscData instanceOfPetsc;

  }
{% for N in NAMESPACE %}
}
{% endfor %}


void {{NAMESPACE | join("::")}}::repositories::computeLocalToGlobalMapsForAllSolvers() {
  {% for SOLVER in SOLVER_INSTANCES %}
  {% if SOLVER_INSTANCES[0][0]=="HDG" %}
    {{SOLVER[1]}}.getFaceLocalToGlobalMap().computeLocalToGlobalMap();
    {{SOLVER[1]}}.getCellLocalToGlobalMap().computeLocalToGlobalMap();
  {% else %}
    {{SOLVER[1]}}.getLocalToGlobalMap().computeLocalToGlobalMap();
  {% endif %}
  {% endfor %}
}


int {{NAMESPACE | join("::")}}::repositories::getTotalNumberOfDegreesOfFreedom() {
  int result = 0;
  {% for SOLVER in SOLVER_INSTANCES %}
  {% if SOLVER_INSTANCES[0][0]=="HDG" %}
  result += {{SOLVER[1]}}.getFaceLocalToGlobalMap().getTotalNumberOfIndices();
  result += {{SOLVER[1]}}.getCellLocalToGlobalMap().getTotalNumberOfIndices();
  {% else %}
  result += {{SOLVER[1]}}.getLocalToGlobalMap().getTotalNumberOfIndices();
  {% endif %}
  {% endfor %}
  return result;
}

{% if SOLVER_INSTANCES[0][0]=="HDG" %}

int {{NAMESPACE | join("::")}}::repositories::getTotalFaceDofs() {
  int result = 0;
  {% for SOLVER in SOLVER_INSTANCES %}
  result += {{SOLVER[1]}}.getFaceLocalToGlobalMap().getTotalNumberOfIndices();
  {% endfor %}
  return result;
}

int {{NAMESPACE | join("::")}}::repositories::getTotalCellDofs() {
  int result = 0;
  {% for SOLVER in SOLVER_INSTANCES %}
  result += {{SOLVER[1]}}.getCellLocalToGlobalMap().getTotalNumberOfIndices();
  {% endfor %}
  return result;
}


{% endif %}
