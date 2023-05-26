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

    /**
    * this name is a little outdated.
    * should this be vertexDofs or should its function be altered?
    */
    int getTotalNumberOfDegreesOfFreedom();
    
    /**
    * for now, use this to count the total number of faces
    */
    {% if SOLVER_INSTANCES[0][0]=="HDG" %}
    int getTotalFaceDofs();
    void incrementFaceDofs(int cardinality);
    void incrementBoundaryFaces(int cardinality);
    int getTotalBoundaryFaces();
    static int _faceDofs;
    static int _boundaryFaces;
    {% endif %}

  }
{% for N in NAMESPACE %}
}
{% endfor %}

