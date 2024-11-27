#include "{{CLASSNAME}}.h"


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}() {
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::~{{CLASSNAME}}() {
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::initVertex(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  tarch::la::Vector< {{VERTEX_CARDINALITY}}, double >&  value,
  tarch::la::Vector< {{VERTEX_CARDINALITY}}, double >&  rhs
) {
{{CUSTOM_INIT_VERTEX}}
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::setBoundaryConditions(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  tarch::la::Vector< {{VERTEX_CARDINALITY}}, double >&  value
) {
{{CUSTOM_BOUNDARY_CONDITIONS}}  
}

