#include "{{CLASSNAME}}.h"

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
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::faceInit(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  // for now, just use value, rhs. not sure what this will be in 
  // future.
  tarch::la::Vector< {{FACE_CARDINALITY}}, double >&  value,
  tarch::la::Vector< {{FACE_CARDINALITY}}, double >&  rhs  
) {}
