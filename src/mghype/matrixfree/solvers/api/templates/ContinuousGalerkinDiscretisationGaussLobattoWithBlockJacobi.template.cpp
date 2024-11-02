#include "{{CLASSNAME}}.h"


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}() {
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::~{{CLASSNAME}}() {
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::initCell(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  tarch::la::Vector< UnknownsPerCellNode, double >&  value,
  tarch::la::Vector< UnknownsPerCellNode, double >&  rhs
) {
{{CUSTOM_INIT_CELL}}
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::setBoundaryConditions(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  tarch::la::Vector< UnknownsPerCellNode, double >&  solution
) {
{{CUSTOM_BOUNDARY_CONDITIONS}}
}

