#include "{{CLASSNAME}}.h"


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}() {
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::~{{CLASSNAME}}() {
}


{%if CUSTOM_INIT_VERTEX=="" %}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::initVertex(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  tarch::la::Vector< {{VERTEX_CARDINALITY}}, double >&  value,
  tarch::la::Vector< {{VERTEX_CARDINALITY}}, double >&  rhs
) {
  // @todo insert your code here
}
{% endif %}


{%if CUSTOM_BOUNDARY_CONDITIONS=="" %}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::setBoundaryConditions(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  tarch::la::Vector< {{VERTEX_CARDINALITY}}, double >&  value
) {
  // @todo insert your code here
}
{% endif %}

