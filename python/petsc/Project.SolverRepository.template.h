//
// Peano's PETSc extension
// Generated through Peano's Python API
// www.peano-framework.org
//
#pragma once


{% for SOLVER in SOLVER_INSTANCES %}

#include "{{SOLVER[0]}}.h"
#include "petsc/Data.h"

{% endfor %}



{% for N in NAMESPACE %}
namespace {{N}} {
{% endfor %}
  namespace repositories {

    {% for SOLVER in SOLVER_INSTANCES %}
    extern {{NAMESPACE | join("::")}}::{{SOLVER[0]}}  {{SOLVER[1]}};
    extern ::petsc::PetscData instanceOfPetsc;
    {% endfor %}

    void computeLocalToGlobalMapsForAllSolvers();
    int getTotalNumberOfDegreesOfFreedom();

  }
{% for N in NAMESPACE %}
}
{% endfor %}

