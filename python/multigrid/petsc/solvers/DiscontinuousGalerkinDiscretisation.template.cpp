#include "{{CLASSNAME}}.h"


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}() {
  // @todo add your stuff here
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::~{{CLASSNAME}}() {
  // @todo add your stuff here
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::initNode(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  {% if CELL_UNKNOWNS==1 %}
  double&                                       value,
  double&                                       rhs
  {% else %}
  tarch::la::Vector< {{CELL_UNKNOWNS}}, double >&  value,
  tarch::la::Vector< {{CELL_UNKNOWNS}}, double >&  rhs
  {% endif %}
) {
  // @todo insert your code here
}


{% if CELL_CELL_LHS_MATRIX==[] %}
tarch::la::Matrix< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::DoFsPerCell, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::DoFsPerCell, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getLhsMatrix(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
) {
  // @todo insert your code here
}
{% endif %}


{% if CELL_CELL_RHS_MATRIX==[] %}
tarch::la::Matrix< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::DoFsPerCell, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::DoFsPerCell, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getRhsMatrix(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
) {
  // @todo insert your code here
}
{% endif %}


{% if CELL_TO_FACE_MATRIX==[] %}
tarch::la::Matrix< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::NodesPerFace*{{NAMESPACE | join("::")}}::{{CLASSNAME}}::FaceUnknowns*{{NAMESPACE | join("::")}}::{{CLASSNAME}}::FacesPerCell, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::DoFsPerCell, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getProjectionOfCellDataOntoFace(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
) {
  // @todo insert your code here
}
{% endif %}


{% if FACE_TO_CELL_MATRIX==[] %}
tarch::la::Matrix< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::DoFsPerCell, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::NodesPerFace*{{NAMESPACE | join("::")}}::{{CLASSNAME}}::FaceUnknowns*{{NAMESPACE | join("::")}}::{{CLASSNAME}}::FacesPerCell, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getProjectionOfRiemannSolutionOntoCell(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
) {
  // @todo insert your code here
}
{% endif %}


{% if FACE_FACE_RIEMANN_PROBLEM_MATRIX==[] %}
tarch::la::Matrix< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::NodesPerFace*{{NAMESPACE | join("::")}}::{{CLASSNAME}}::FaceUnknowns, 2*{{NAMESPACE | join("::")}}::{{CLASSNAME}}::NodesPerFace*{{NAMESPACE | join("::")}}::{{CLASSNAME}}::Unknowns, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getRiemannSolver(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
) {
  // @todo insert your code here
}
{% endif %}
