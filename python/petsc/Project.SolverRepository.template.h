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
    int getTotalVertexDofs();

    /**
    * we order the matrix so that cell-cell coupling sits
    * in the top left, with face-face coupling in the bottom right
    */
    int getGlobalCellIndex(std::pair<int,int> localIndex);
    int getGlobalFaceIndex(std::pair<int,int> localIndex);
    int getGlobalVertexIndex(std::pair<int,int> localIndex);

    /*
    * store points, weight and coefficients for gaussian
    * quadratures. aligned left to right / bottom to top
    */
    const std::vector<std::vector<double>> quadPoints = {{QUADRATURE_POINTS}};
    const std::vector<double> quadWeights             = {{WEIGHTS}};
    const std::vector<std::vector<double>> polyCoeffs = {{POLY_COEFFS}};

    const int polynomialDegree = {{POLY_DEGREE}};

    /*
    we will enumerate our DoFs 0 -> (n+1)^d.
    for now we only have 1D basis functions, and we promote them to 2d by taking their product.
    we do this by taking the x position of the quadrature point, and pulling out an appropriate basis function
    and then      taking the y position of the quadrature point, and pulling out an appropriate basis function
    and multiplying them together.

    dof ordering, with polynomial degree n:


    .......................
    .......................
    .......................
    n+1 - n+2 - .... - 2n+1
    0   - 1   - .... -    n

    so this method will pick out (x,y) coords for each dof. 

    This is temporary - I haven't ironed out where the dofs should sit physically. we 
    should be able to get some correct local matrices.

    TODO upgrade to 3d
    */
    const std::vector<int> convertDofIndexToXYCoordinate(const int& dofIndex);

    /*
    method to evaluate polynomial at given point, given a set of coefficients
    */
    double evalPoly( const std::vector<double>& coeffs, const double& quadPoint );

    /*
    overload to:
      - convert index into a tuple that describes position in the cell
      - evaluate the 1d basis at each of the points
      - take the product

      ie we promote a 1d basis to an n-dimensional basis by evaluating 
      our basis polynomial at (x,y,z,...) and taking the product
      poly(x)*poly(y)*poly(z)*...
    */
    double evalPoly( const int& index, const std::vector<double>& quadPoint );

    {% endif %}

  }
{% for N in NAMESPACE %}
}
{% endfor %}

