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
  {{SOLVER[1]}}.getLocalToGlobalMap().computeLocalToGlobalMap();
  {% endfor %}
}


int {{NAMESPACE | join("::")}}::repositories::getTotalNumberOfDegreesOfFreedom() {
  int result = 0;
  {% for SOLVER in SOLVER_INSTANCES %}
  result += {{SOLVER[1]}}.getLocalToGlobalMap().getTotalNumberOfIndices();
  {% endfor %}
  return result;
}

{% if SOLVER_INSTANCES[0][0]=="HDG" %}

void {{NAMESPACE | join("::")}}::repositories::incrementFaceDofs(int cardinality){
  _faceDofs += cardinality;
}

void {{NAMESPACE | join("::")}}::repositories::incrementBoundaryFaces(int cardinality){
  _boundaryFaces += cardinality;
}

int {{NAMESPACE | join("::")}}::repositories::getTotalFaceDofs() {
  return _faceDofs;
}

int {{NAMESPACE | join("::")}}::repositories::getTotalBoundaryFaces() {
  return _boundaryFaces;
}

{% endif %}
