#include "{{CLASSNAME}}.h"
constexpr double pi = 3.14159265358979323846264338328;

double initRhs(
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
  const tarch::la::Vector< {{VERTEX_CARDINALITY}}, double >&  value,
  const tarch::la::Vector< {{VERTEX_CARDINALITY}}, double >&  rhs
  {% endif %}
) {
  value = 0.0;
  rhs = initRhs(x);
}
