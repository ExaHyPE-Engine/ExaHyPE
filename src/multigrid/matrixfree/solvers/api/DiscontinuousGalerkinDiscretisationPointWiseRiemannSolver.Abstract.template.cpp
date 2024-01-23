#include "{{CLASSNAME}}.h"

tarch::logging::Log  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log( "{{NAMESPACE | join("::")}}::{{CLASSNAME}}" );

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}() {}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::~{{CLASSNAME}}() {}

/*
  Add in methods to fetch matrices, determine position in domain of faces, cells, vertices
*/


{{NAMESPACE | join("::")}}::vertexdata::{{SOLVER_NAME}}::Type {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getVertexDoFType(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h
) {
  return vertexdata::{{SOLVER_NAME}}::Type::Interior;
}

{{NAMESPACE | join("::")}}::celldata::{{SOLVER_NAME}}::Type {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getCellDoFType(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h
) {
  return celldata::{{SOLVER_NAME}}::Type::Interior;
}

{{NAMESPACE | join("::")}}::facedata::{{SOLVER_NAME}}::Type {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getFaceDoFType(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h
) {
  return facedata::{{SOLVER_NAME}}::Type::Interior;
}

/*
Warning: this will become a variable matrix one day.
  Will need to become a class member
*/
{% if MASS_MATRIX!=[] %}
tarch::la::Matrix< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMassMatrix(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
) {
  static tarch::la::Matrix< CellUnknowns, CellUnknowns, double > result = {
      {{MASS_MATRIX[0]| join(", ")}}
      {% for ROW in MASS_MATRIX[1:] %}
        ,{{ROW | join(", ")}}
      {% endfor %}
  };

  // @todo add in scaling

  return result;
}
{% endif %}

/*
Warning: this will become a variable matrix one day.
  Will need to become a class member
*/
{% if ASSEMBLY_MATRIX!=[] %}
tarch::la::Matrix< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getLocalAssemblyMatrix(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
) {
  tarch::la::Matrix< CellUnknowns, CellUnknowns, double > result = {
    {{ASSEMBLY_MATRIX[0]| join(", ")}}
    {% for ROW in ASSEMBLY_MATRIX[1:] %}
      ,{{ROW | join(", ")}}
    {% endfor %}
  };

  // @todo add in scaling

  return result;
}
{% endif %}

{% if FACE_FROM_CELL_PROJECTION!=[] %}
tarch::la::Matrix<{{NAMESPACE | join("::")}}::{{CLASSNAME}}::FaceUnknownsProjection * TwoTimesD, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, double> {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getFaceFromCellMatrix()
{
  static tarch::la::Matrix< FaceUnknownsProjection * TwoTimesD, CellUnknowns, double > result = {
      {{FACE_FROM_CELL_PROJECTION[0]| join(", ")}}
      {% for ROW in FACE_FROM_CELL_PROJECTION[1:] %}
        ,{{ROW | join(", ")}}
      {% endfor %}
  };

  return result;
} 
{% endif %}

{% if CELL_FROM_FACE_PROJECTION!=[] %}
tarch::la::Matrix<{{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::FaceUnknownsSolution * TwoTimesD, double> {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getCellFromFaceMatrix()
{
  static tarch::la::Matrix<  CellUnknowns, FaceUnknownsSolution * TwoTimesD, double > result = {
      {{CELL_FROM_FACE_PROJECTION[0]| join(", ")}}
      {% for ROW in CELL_FROM_FACE_PROJECTION[1:] %}
        ,{{ROW | join(", ")}}
      {% endfor %}
  };

  return result;
} 

{% endif %}

{% if RIEMANN_MATRIX!=[] %}
tarch::la::Matrix<{{NAMESPACE | join("::")}}::{{CLASSNAME}}::FaceUnknownsSolution, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::FaceUnknownsProjection, double> {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getRiemannMatrix()
{
  static tarch::la::Matrix<  FaceUnknownsSolution, FaceUnknownsProjection, double > result = {
      {{RIEMANN_MATRIX[0]| join(", ")}}
      {% for ROW in RIEMANN_MATRIX[1:] %}
        ,{{ROW | join(", ")}}
      {% endfor %}
  };

  return result;
} 

{% endif %}

{% if APPROX_SYSTEM_MATRIX!=[] %}
tarch::la::Matrix< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getInvertedApproxSystemMatrix(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
) {

  static tarch::la::Matrix<  CellUnknowns, CellUnknowns, double > result = {
      {{APPROX_SYSTEM_MATRIX[0]| join(", ")}}
      {% for ROW in APPROX_SYSTEM_MATRIX[1:] %}
        ,{{ROW | join(", ")}}
      {% endfor %}
  };

  // merge the inversion routine in here

  return result;
}
{% endif %}


