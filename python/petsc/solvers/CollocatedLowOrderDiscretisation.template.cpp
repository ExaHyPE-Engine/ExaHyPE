#include "{{CLASSNAME}}.h"


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}() {
  // @todo add your stuff here
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::~{{CLASSNAME}}() {
  // @todo add your stuff here
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::init(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  {% if CARDINALITY==1 %}
  double&                                       value,
  double&                                       rhs
  {% else %}
  const tarch::la::Vector< {{CARDINALITY}}, double >&  value,
  const tarch::la::Vector< {{CARDINALITY}}, double >&  rhs
  {% endif %}
) {
  // @todo add your stuff here
}
