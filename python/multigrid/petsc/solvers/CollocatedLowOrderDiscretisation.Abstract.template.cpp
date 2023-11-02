#include "{{CLASSNAME}}.h"



tarch::logging::Log  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log( "{{NAMESPACE | join("::")}}::{{CLASSNAME}}" );


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}():
  _localToGlobalMap( ::petsc::LocalToGlobalMap::RankGlobalTreeNumber )  {

}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::~{{CLASSNAME}}() {

}


{% if CELL_LHS_MATRIX!=[] %}
tarch::la::Matrix< {{VERTEX_CARDINALITY}}*TwoPowerD, {{VERTEX_CARDINALITY}}*TwoPowerD, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getLhsMatrix(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
) {
  static tarch::la::Matrix< {{VERTEX_CARDINALITY}}*TwoPowerD, {{VERTEX_CARDINALITY}}*TwoPowerD, double > result = {
      {{CELL_LHS_MATRIX[0]| join(", ")}}
      {% for ROW in CELL_LHS_MATRIX[1:] %}
        ,{{ROW | join(", ")}}
      {% endfor %}
  };
  double scaling = 1.0;
  {% if CELL_LHS_MATRIX_SCALING >= 0 %}
  for (int i=0; i<{{CELL_LHS_MATRIX_SCALING}}; i++) {
    scaling *= cellSize(0);
  {% else %}
  for (int i=0; i<(-1)*{{CELL_LHS_MATRIX_SCALING}}; i++) {
    scaling /= cellSize(0);
  {% endif %}
  }
  return scaling * result;
}
{% endif %}


{% if CELL_RHS_MATRIX!=[] %}
tarch::la::Matrix< {{VERTEX_CARDINALITY}}*TwoPowerD, {{VERTEX_CARDINALITY}}*TwoPowerD, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getRhsMatrix(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
) {
  static tarch::la::Matrix< {{VERTEX_CARDINALITY}}*TwoPowerD, {{VERTEX_CARDINALITY}}*TwoPowerD, double > result = {
      {{CELL_RHS_MATRIX[0]| join(", ")}}
      {% for ROW in CELL_RHS_MATRIX[1:] %}
        ,{{ROW | join(", ")}}
      {% endfor %}
  };
  double scaling = 1.0;
  {% if CELL_RHS_MATRIX_SCALING >= 0 %}
  for (int i=0; i<{{CELL_RHS_MATRIX_SCALING}}; i++) {
    scaling *= cellSize(0);
  {% else %}
  for (int i=0; i<(-1)*{{CELL_RHS_MATRIX_SCALING}}; i++) {
    scaling /= cellSize(0);
  {% endif %}
  }
  return scaling * result;
}
{% endif %}


::petsc::LocalToGlobalMap&  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getLocalToGlobalMap() {
  return _localToGlobalMap;
}


::petsc::LinearEquationSystem&  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getLinearEquationSystem() {
  return _linearEquationSystem;
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishAssembly() {
}


