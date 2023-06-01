#include "{{CLASSNAME}}.h"

/**
* for now, include the setters we want here
* for now, skip p_exact calculation
*/

static double getRhs(double x, double y){
  static constexpr double c_u    = 1.0;
  static constexpr double c_p    = 1.0;
  static constexpr double sigma2 = 1.0 * 1.0; //simga**2
  
  const double rsq = (x-0.5)*(x-0.5) + (y-0.5)*(y-0.5);
  double result = (c_p + 1.0 / (sigma2*c_u) * (2 - rsq / sigma2) )
                  * exp( -0.5 * rsq / sigma2 );

  return result;
  
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
) {}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::faceInit(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  // for now, just use value, rhs. not sure what this will be in 
  // future.
  tarch::la::Vector< {{FACE_CARDINALITY}}, double >&  value,
  tarch::la::Vector< {{FACE_CARDINALITY}}, double >&  rhs  
) {}
