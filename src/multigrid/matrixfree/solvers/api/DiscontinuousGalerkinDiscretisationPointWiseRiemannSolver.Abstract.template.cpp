#include "{{CLASSNAME}}.h"

#include <cmath>     // for std::pow
#include <algorithm> // for std::transform

tarch::logging::Log  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log( "{{NAMESPACE | join("::")}}::{{CLASSNAME}}" );

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}() {}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::~{{CLASSNAME}}() {}

bool isOnBoundary(const tarch::la::Vector<Dimensions, double>& x)
{
  using namespace {{NAMESPACE | join("::")}};
  bool isOnBoundary = false;
  for (int d=0; d<Dimensions; d++) {
    isOnBoundary |= tarch::la::smallerEquals( x(d), DomainOffset(d) );
    isOnBoundary |= tarch::la::greaterEquals( x(d), DomainOffset(d)+DomainSize(d) );
  }
  return isOnBoundary;
}

/*
Pass in matrix of doubles and matrix of ints, as well as cell width.

Each element inputMatrix[i,j] <- inputMatrix[i,j] * h^( scaleFactors[i,j] )
*/
template<int Rows, int Cols>
tarch::la::Matrix<Rows,Cols,double> scaleMatrix(
  const tarch::la::Matrix<Rows,Cols,double>&    inputMatrix,
  const tarch::la::Matrix<Rows,Cols,int>&       scaleFactors,
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h
) {
  tarch::la::Matrix<Rows,Cols,double> matrix;
  std::transform( inputMatrix.data(), inputMatrix.data() + inputMatrix.size(),
                  scaleFactors.data(),
                  matrix.data(),      // write results of the binary op back to "matrix"
                  [&](double m, int s){ return m * std::pow(h(0),s); }
  );
  return matrix;
}

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
  return isOnBoundary(x) ?
    facedata::{{SOLVER_NAME}}::Type::Boundary:
    facedata::{{SOLVER_NAME}}::Type::Interior;
}

{% if MASS_MATRIX!=[] %}
tarch::la::Matrix< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMassMatrix(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
) {
  static tarch::la::Matrix< CellUnknowns, CellUnknowns, double > matrix = {
      {{MASS_MATRIX[0]| join(", ")}}
      {% for ROW in MASS_MATRIX[1:] %}
        ,{{ROW | join(", ")}}
      {% endfor %}
  };
{% if MASS_MATRIX_SCALING!=[] %}
  static tarch::la::Matrix< CellUnknowns, CellUnknowns, int > scaleFactors = {
      {{MASS_MATRIX_SCALING[0]| join(", ")}}
      {% for ROW in MASS_MATRIX_SCALING[1:] %}
        ,{{ROW | join(", ")}}
      {% endfor %}
  };
{% endif %}
  static tarch::la::Matrix< CellUnknowns, CellUnknowns, double > result = scaleMatrix( matrix, scaleFactors, cellCentre, cellSize );

  return result;
}
{% endif %}

{% if ASSEMBLY_MATRIX!=[] %}
tarch::la::Matrix< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getLocalAssemblyMatrix(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
) {
  tarch::la::Matrix< CellUnknowns, CellUnknowns, double > matrix = {
    {{ASSEMBLY_MATRIX[0]| join(", ")}}
    {% for ROW in ASSEMBLY_MATRIX[1:] %}
      ,{{ROW | join(", ")}}
    {% endfor %}
  };
{% if ASSEMBLY_MATRIX_SCALING!=[] %}
  static tarch::la::Matrix< CellUnknowns, CellUnknowns, int > scaleFactors = {
      {{ASSEMBLY_MATRIX_SCALING[0]| join(", ")}}
      {% for ROW in ASSEMBLY_MATRIX_SCALING[1:] %}
        ,{{ROW | join(", ")}}
      {% endfor %}
  };
{% endif %}
  static tarch::la::Matrix< CellUnknowns, CellUnknowns, double > result = scaleMatrix( matrix, scaleFactors, cellCentre, cellSize );

  return result;
}
{% endif %}

{% if FACE_FROM_CELL_PROJECTION!=[] %}
tarch::la::Matrix<{{NAMESPACE | join("::")}}::{{CLASSNAME}}::FaceUnknownsProjection * TwoTimesD, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, double> {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getFaceFromCellMatrix(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
)
{
  static tarch::la::Matrix< FaceUnknownsProjection * TwoTimesD, CellUnknowns, double > matrix = {
      {{FACE_FROM_CELL_PROJECTION[0]| join(", ")}}
      {% for ROW in FACE_FROM_CELL_PROJECTION[1:] %}
        ,{{ROW | join(", ")}}
      {% endfor %}
  };
{% if FACE_FROM_CELL_PROJECTION_SCALING!=[] %}
  static tarch::la::Matrix< FaceUnknownsProjection * TwoTimesD, CellUnknowns, int > scaleFactors = {
      {{FACE_FROM_CELL_PROJECTION_SCALING[0]| join(", ")}}
      {% for ROW in FACE_FROM_CELL_PROJECTION_SCALING[1:] %}
        ,{{ROW | join(", ")}}
      {% endfor %}
  };
{% endif %}
  static tarch::la::Matrix< FaceUnknownsProjection * TwoTimesD, CellUnknowns, double > result = scaleMatrix( matrix, scaleFactors, cellCentre, cellSize );
  return result;
} 
{% endif %}

{% if CELL_FROM_FACE_PROJECTION!=[] %}
tarch::la::Matrix<{{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::FaceUnknownsSolution * TwoTimesD, double> {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getCellFromFaceMatrix(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
)
{
  static tarch::la::Matrix<  CellUnknowns, FaceUnknownsSolution * TwoTimesD, double > matrix = {
      {{CELL_FROM_FACE_PROJECTION[0]| join(", ")}}
      {% for ROW in CELL_FROM_FACE_PROJECTION[1:] %}
        ,{{ROW | join(", ")}}
      {% endfor %}
  };
{% if CELL_FROM_FACE_PROJECTION_SCALING!=[] %}
  static tarch::la::Matrix<  CellUnknowns, FaceUnknownsSolution * TwoTimesD, int > scaleFactors = {
      {{CELL_FROM_FACE_PROJECTION_SCALING[0]| join(", ")}}
      {% for ROW in CELL_FROM_FACE_PROJECTION_SCALING[1:] %}
        ,{{ROW | join(", ")}}
      {% endfor %}
  };
{% endif %}
  static tarch::la::Matrix< CellUnknowns, FaceUnknownsSolution * TwoTimesD, double > result = scaleMatrix( matrix, scaleFactors, cellCentre, cellSize );
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

{% if BOUNDARY_MATRIX!=[] %}
tarch::la::Matrix<{{NAMESPACE | join("::")}}::{{CLASSNAME}}::FaceUnknownsSolution, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::FaceUnknownsProjection, double> {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getBoundaryConditionMatrix()
{
  static tarch::la::Matrix<  FaceUnknownsSolution, FaceUnknownsProjection, double > result = {
      {{BOUNDARY_MATRIX[0]| join(", ")}}
      {% for ROW in BOUNDARY_MATRIX[1:] %}
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


