#include "{{CLASSNAME}}.h"
#include "repositories/SolverRepository.h" //bad template practice; quick fix
constexpr double pi = 3.14159265358979323846264338328;

double f(
  const tarch::la::Vector<Dimensions, double>&  x
){
  double f = -8 * pi * pi;
  for (int i=0; i<Dimensions; i++){
    f *= std::sin( 2 * pi * x[i] );
  }
  return f;
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}() {
  // @todo add your stuff here
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::~{{CLASSNAME}}() {
  // @todo add your stuff here
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::vertexInit(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  {% if VERTEX_CARDINALITY==1 %}
  double&                                       value,
  double&                                       rhs
  {% else %}
  tarch::la::Vector< {{VERTEX_CARDINALITY}}, double >&  value,
  tarch::la::Vector< {{VERTEX_CARDINALITY}}, double >&  rhs
  {% endif %}
) {
  // @todo add your stuff here
}

/*

todo: instead of harcoding 
  tarch::la::Vector<Dimensions, double> tempX = {x[0] - h[0]/2, x[1] - h[1]/2};
make it:
  tarch::la::Vector<Dimensions, double> tempX = {x[0] - h[0]/CARDINALITY, x[1] - h[1]/CARDINALITY};
and do the same for the nested for loop below

*/

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::cellInit(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  // for now, just use value, rhs. not sure what this will be in 
  // future.
  tarch::la::Vector< {{CELL_CARDINALITY}}, double >&  value,
  tarch::la::Vector< {{CELL_CARDINALITY}}, double >&  rhs  
) {
  double cellSize = 1;
  for (int i=0; i<Dimensions; i++)
    cellSize *= h[i];

  //set all entries to 0 by default. might not be necessary
  for (int i=0; i<{{CELL_CARDINALITY}}; i++)
  {
    value[i] = 0;
    rhs[i]   = 0;
  }
  
  const int nPlusOne = {{POLYNOMIAL_DEGREE}}+1; ///polynomial degree + 1
  //proper loop for rhs construction
  //we loop over point i in the mesh:
  for (int i=0; i<{{CELL_DOFS}}; i++)
  {
    //we take the integral over the whole cell
    double integral=0;
    int p=0; int w=0;
    for (; p<repositories::quadWeights.size(); p++, w++){
      const std::vector<double> localQuadPoint  = {{NAMESPACE | join("::")}}::repositories::quadPoints[p];
      const double              localQuadWeight = {{NAMESPACE | join("::")}}::repositories::quadWeights[w];
      const tarch::la::Vector<Dimensions,double> quadPointInCell = {{NAMESPACE | join("::")}}::repositories::convertQuadPointToRealLocation(
        p, x, h
      );

      integral += 0.25 * cellSize * localQuadWeight * repositories::evalPoly(i, localQuadPoint) * f(quadPointInCell);
    }
    /*
    this i runs over the number of dofs in this cell. ie we don't have the
    required multicplicity here. 
    we want to set all the u values with this integral. at time of writing
    we have no policy for setting phi_x and phi_y. so we multiply by
    number of dofs per cell to "push" this further.
    */
    rhs[i * {{CELL_DOF_UNKNOWNS}}] = integral;
  }

}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::faceInit(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  // for now, just use value, rhs. not sure what this will be in 
  // future.
  tarch::la::Vector< {{FACE_CARDINALITY}}, double >&  value,
  tarch::la::Vector< {{FACE_CARDINALITY}}, double >&  rhs  
) {}
