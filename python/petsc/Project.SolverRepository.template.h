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
    int getTotalCellDofs();

    /**
    * we order the matrix so that cell-cell coupling sits
    * in the top left, with face-face coupling in the bottom right
    */
    int getGlobalCellIndex(std::pair<int,int> localIndex);
    int getGlobalFaceIndex(std::pair<int,int> localIndex);

    /*
    * store points, weight and coefficients for gaussian
    * quadratures. aligned left to right / bottom to top
    */
    const inline std::vector<double> quadPoints              = {{QUADRATURE_POINTS}};
    const inline std::vector<double> quadWeights             = {{WEIGHTS}};
    const inline std::vector<std::vector<double>> polyCoeffs = {{POLY_COEFFS}};
    {% endif %}

  }
{% for N in NAMESPACE %}
}
{% endfor %}

