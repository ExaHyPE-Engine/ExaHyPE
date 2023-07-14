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

    const int polynomialDegree = {{POLY_DEGREE}};

    /*
    method to evaluate polynomial at given point, given a set of coefficients
    */
    double evalPoly( const std::vector<double>& coeffs, const double& quadPoint );

    /*
    we have n+1 different polynomials. use this overload to specify which one
    you mean.
    */
    inline double evalPoly( const int& index, const double& quadPoint ){
      if (index < 0
          or
          index > polynomialDegree + 1) throw std::runtime_error("bad index used!");
      return evalPoly( polyCoeffs[index], quadPoint );
    }

    /*
    method to eval 2 polynomials and return them
    */
    double evalPoly( const int& indexX, const int& indexY, const double& quadPoint );

    {% endif %}

  }
{% for N in NAMESPACE %}
}
{% endfor %}

