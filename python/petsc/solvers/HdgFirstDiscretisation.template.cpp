#include "{{CLASSNAME}}.h"
constexpr double pi = 3.14159265358979323846264338328;

double poissonRHS(
  const tarch::la::Vector<Dimensions, double>&  x
){
  double rhs = Dimensions * pi * pi;
  for (int i = 0; i < Dimensions; i++){
    rhs *= std::sin( pi * x[i] );
  }
  return rhs;  
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

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::cellInit(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  // for now, just use value, rhs. not sure what this will be in 
  // future.
  tarch::la::Vector< {{CELL_CARDINALITY}}, double >&  value,
  tarch::la::Vector< {{CELL_CARDINALITY}}, double >&  rhs  
) {
  //begin at bottom left
  tarch::la::Vector<Dimensions, double> tempX = {x[0] - h[0]/2, x[1] - h[1]/2};

  //this only works for 2d
  //hardcode 2 since 2 vertices in each dimension
  for (int i=0; i<2; i++){
    for (int j=0; j<2; j++){
      rhs[ i*2 + j ] = poissonRHS( { tempX[0] + j*h[0], tempX[1] + i*h[1] } );
    }
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
