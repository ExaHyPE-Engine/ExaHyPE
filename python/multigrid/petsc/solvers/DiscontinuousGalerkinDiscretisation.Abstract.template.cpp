#include "{{CLASSNAME}}.h"



tarch::logging::Log  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log( "{{NAMESPACE | join("::")}}::{{CLASSNAME}}" );


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}():
  _localToGlobalMap( ::petsc::LocalToGlobalMap::RankGlobalTreeNumber )  {
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::~{{CLASSNAME}}() {
}


::petsc::LinearEquationSystem&  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getLinearEquationSystem() {
  return _linearEquationSystem;
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishAssembly() {
}


{% if CELL_CELL_MASS_MATRIX!=[] %}
tarch::la::Matrix< DoFsPerCell, DoFsPerCell, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMassMatrix(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
) {
  static tarch::la::Matrix< DoFsPerCell, DoFsPerCell, double > result = {
      {{CELL_CELL_MASS_MATRIX[0]| join(", ")}}
      {% for ROW in CELL_CELL_MASS_MATRIX[1:] %}
        ,{{ROW | join(", ")}}
      {% endfor %}
  };
  return tarch::la::volume(cellSize) * result;
}
{% endif %}


{% if CELL_CELL_SYSTEM_MATRIX!=[] %}
tarch::la::Matrix< DoFsPerCell, DoFsPerCell, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getSystemMatrix(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
) {
  static tarch::la::Matrix< DoFsPerCell, DoFsPerCell, double > result = {
      {{CELL_CELL_SYSTEM_MATRIX[0]| join(", ")}}
      {% for ROW in CELL_CELL_SYSTEM_MATRIX[1:] %}
        ,{{ROW | join(", ")}}
      {% endfor %}
  };
  return tarch::la::volume(cellSize) / cellSize(0) / cellSize(0) * result;
}
{% endif %}


{% if CELL_TO_FACE_MATRIX!=[] %}
tarch::la::Matrix< NodesPerFace*Unknowns*FacesPerCell, DoFsPerCell, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getProjectionOfCellDataOntoFace(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
) {
  static tarch::la::Matrix< NodesPerFace*Unknowns*FacesPerCell, DoFsPerCell, double > result = {
      {{CELL_TO_FACE_MATRIX[0]| join(", ")}}
      {% for ROW in CELL_TO_FACE_MATRIX[1:] %}
        ,{{ROW | join(", ")}}
      {% endfor %}
  };
  return result;
}
{% endif %}


{% if FACE_TO_CELL_MATRIX!=[] %}
tarch::la::Matrix< DoFsPerCell, NodesPerFace*Unknowns*FacesPerCell, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getProjectionOfRiemannSolutionOntoCell(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
) {
  static tarch::la::Matrix< DoFsPerCell, NodesPerFace*Unknowns*FacesPerCell, double > result = {
      {{FACE_TO_CELL_MATRIX[0]| join(", ")}}
      {% for ROW in FACE_TO_CELL_MATRIX[1:] %}
        ,{{ROW | join(", ")}}
      {% endfor %}
  };
  return cellSize(0) * result;
}
{% endif %}


{% if FACE_FACE_RIEMANN_PROBLEM_MATRIX!=[] %}
tarch::la::Matrix< NodesPerFace*Unknowns, 2*NodesPerFace*Unknowns, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getRiemannSolver(
  const tarch::la::Vector<Dimensions, double>&  faceCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
) {
  static tarch::la::Matrix< NodesPerFace*Unknowns, 2*NodesPerFace*Unknowns, double > result = {
      {{FACE_FACE_RIEMANN_PROBLEM_MATRIX[0]| join(", ")}}
      {% for ROW in FACE_FACE_RIEMANN_PROBLEM_MATRIX[1:] %}
        ,{{ROW | join(", ")}}
      {% endfor %}
  };
  return result;
}
{% endif %}

