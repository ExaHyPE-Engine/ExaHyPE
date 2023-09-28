//
// Peano's PETSc extension
// Generated through Peano's Python API
// www.peano-framework.org
//
#pragma once


{% for SOLVER in SOLVER_INSTANCES %}

#include "{{SOLVER[0]}}.h"

{% endfor %}

#include "PETScdata/PETScData.h"


{% for N in NAMESPACE %}
namespace {{N}} {
{% endfor %}
  namespace repositories {

    {% for SOLVER in SOLVER_INSTANCES %}
    extern {{NAMESPACE | join("::")}}::{{SOLVER[0]}}  {{SOLVER[1]}};

    {% endfor %}

    /**
     * here we insert an instance of our PetscData class. 
     * Not sure if this is the best place to put it, but then we 
     * can interact with it by referring to repositories::instanceOfPetsc
    */
    extern {{NAMESPACE | join("::")}}::PetscData instanceOfPetsc;
    
    void computeLocalToGlobalMapsForAllSolvers();

    int getTotalNumberOfDegreesOfFreedom();
    
    /**
    * these functions are needed in the main. so we 
    * declare them here and define them in the 
    * solver-specific snippets
    */
    int getGlobalMatrixRows();
    int getGlobalMatrixCols();
    
    /**
    * use this snippet to insert something project-specific. 
    * the Solver.py has an abstract method get_solver_repository_snippets
    * overload this with a jinja template.
    */
    {{HEADER_EXTRA}}
  }
{% for N in NAMESPACE %}
}
{% endfor %}
