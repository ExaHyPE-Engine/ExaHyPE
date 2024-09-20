#include "{{CLASSNAME}}.h"


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}() {
  // @todo add your stuff here
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::~{{CLASSNAME}}() {
  // @todo add your stuff here
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::initVertex(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  tarch::la::Vector< {{VERTEX_CARDINALITY}}, double >&  value,
  tarch::la::Vector< {{VERTEX_CARDINALITY}}, double >&  rhs
) {
  double tempRhs = 2 * tarch::la::PI * tarch::la::PI;
  for (int i = 0; i < Dimensions; i++){

    tempRhs *= std::sin( tarch::la::PI * x[i] );
  }
  // put this into the rhs vector
  for (int i=0; i<rhs.size(); i++)
  {
    rhs[i] = tempRhs;
  }

  // similarly, set all of the entries of "value" to 0
  for (int i=0; i<value.size(); i++)
  {
    value[i] = 0.0;
  }
}


{% if CELL_RHS_MATRIX==[] %}
tarch::la::Matrix< {{VERTEX_CARDINALITY}}*TwoPowerD, {{VERTEX_CARDINALITY}}*TwoPowerD, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getRhsMatrix(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
) {
  // @todo add your stuff here
}
{% endif %}


{% if CELL_LHS_MATRIX==[] %}
tarch::la::Matrix< {{VERTEX_CARDINALITY}}*TwoPowerD, {{VERTEX_CARDINALITY}}*TwoPowerD, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getLhsMatrix(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
) {
  // @todo add your stuff here
}
{% endif %}
