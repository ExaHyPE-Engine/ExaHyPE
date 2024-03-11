#include "{{CLASSNAME}}.h"


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}() {
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::~{{CLASSNAME}}() {
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::initVertex(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  tarch::la::Vector< VertexUnknowns, double >&  value,
  tarch::la::Vector< VertexUnknowns, double >&  rhs
) { return; }

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::initCell(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  tarch::la::Vector< CellUnknowns, double >&  value,
  tarch::la::Vector< CellUnknowns, double >&  rhs,
  tarch::la::Vector< CellUnknowns, double >&  exact
) {return;}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::initFace(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  tarch::la::Vector< FaceUnknownsSolution,   double >&  solution,
  tarch::la::Vector< FaceUnknownsProjection, double >&  projection
) {return;}

/**
* Returns true if we are outside the expected
* tolerance, and false if we can terminate.
*
*/
bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::terminationCriterionHolds()
{
  return true;
}
