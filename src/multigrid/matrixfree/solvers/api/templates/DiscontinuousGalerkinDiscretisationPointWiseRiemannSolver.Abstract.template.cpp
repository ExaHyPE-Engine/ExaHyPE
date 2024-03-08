#include "{{CLASSNAME}}.h"
#include <cmath> // for std::pow

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

template<int Rows, int Cols>
tarch::la::Matrix<Rows,Cols,double> scaleMatrix(
  const std::vector< tarch::la::Matrix<Rows,Cols,double> >& matrices,
  const std::vector<int>&                                   scaleFactors,
  const tarch::la::Vector<Dimensions, double>&  h
){
  tarch::la::Matrix<Rows,Cols,double> output(0);
  assert( matrices.size() == scaleFactors.size() );
  auto matrixIter = matrices.begin();
  auto scaleIter  = scaleFactors.begin();
  for (; matrixIter!=matrices.end(), scaleIter!=scaleFactors.end(); matrixIter++, scaleIter++)
    output = output + (*matrixIter) * std::pow(h(0), *scaleIter);
  return output;
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


{% if MASS_MATRIX is defined %}
tarch::la::Matrix< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMassMatrix(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
) {
  logTraceInWith2Arguments("getMassMatrix", cellCentre, cellSize);
  static std::vector< tarch::la::Matrix< CellUnknowns, CellUnknowns, double > > matrices = {
    {% for MATRIX in MASS_MATRIX %}
    // {# MASS_MATRIX is an array, possible of length 1 #}
    {
      {{MATRIX[0]| join(", ")}}
        {% for ROW in MATRIX[1:] %}
        ,{{ROW | join(", ")}}
      {% endfor %}
    },
    {% endfor %}
  };

  {% if MASS_MATRIX_SCALING is not defined %}
  #error If matrices are predefined, scaling has to be defined, too
  {% endif %}

  static std::vector<int> scaleFactors = {
      {% for el in MASS_MATRIX_SCALING %}
        {{el}},
      {% endfor %}
  };
  static tarch::la::Matrix< CellUnknowns, CellUnknowns, double > result = scaleMatrix( matrices, scaleFactors, cellSize );

  logTraceOutWith1Argument("getMassMatrix", result);
  return result;
}
{% endif %}


{% if ASSEMBLY_MATRIX is defined %}
tarch::la::Matrix< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getLocalAssemblyMatrix(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
) {
  logTraceInWith2Arguments("getLocalAssemblyMatrix", cellCentre, cellSize);
  static std::vector< tarch::la::Matrix< CellUnknowns, CellUnknowns, double > > matrices = {
    {% for MATRIX in ASSEMBLY_MATRIX %}
    {
      {{MATRIX[0]| join(", ")}}
        {% for ROW in MATRIX[1:] %}
        ,{{ROW | join(", ")}}
      {% endfor %}
    },
    {% endfor %}
  };

  {% if ASSEMBLY_MATRIX_SCALING is not defined %}
  #error If matrices are predefined, scaling has to be defined, too
  {% endif %}

  static std::vector<int> scaleFactors = {
      {% for el in ASSEMBLY_MATRIX_SCALING %}
        {{el}},
      {% endfor %}
  };

  static tarch::la::Matrix< CellUnknowns, CellUnknowns, double > result = scaleMatrix( matrices, scaleFactors, cellSize );

  logTraceOutWith1Argument("getLocalAssemblyMatrix", result);
  return result;
}
{% endif %}


{% if FACE_FROM_CELL_PROJECTION is defined %}
tarch::la::Matrix<{{NAMESPACE | join("::")}}::{{CLASSNAME}}::FaceUnknownsProjection * TwoTimesD, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, double> {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getFaceFromCellMatrix(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
)
{
  logTraceInWith2Arguments("getFaceFromCellMatrix", cellCentre, cellSize);
  static std::vector< tarch::la::Matrix< FaceUnknownsProjection * TwoTimesD, CellUnknowns, double > > matrices = {
    {% for MATRIX in FACE_FROM_CELL_PROJECTION %}
    {
      {{MATRIX[0]| join(", ")}}
        {% for ROW in MATRIX[1:] %}
        ,{{ROW | join(", ")}}
      {% endfor %}
    },
    {% endfor %}
  };

  {% if FACE_FROM_CELL_PROJECTION_SCALING is not defined %}
  #error If matrices are predefined, scaling has to be defined, too
  {% endif %}

  static std::vector<int> scaleFactors = {
      {% for el in FACE_FROM_CELL_PROJECTION_SCALING %}
        {{el}},
      {% endfor %}
  };

  static tarch::la::Matrix< FaceUnknownsProjection * TwoTimesD, CellUnknowns, double > result = scaleMatrix( matrices, scaleFactors, cellSize );
  logTraceOutWith1Argument("getFaceFromCellMatrix", result);
  return result;
} 
{% endif %}


{% if CELL_FROM_FACE_PROJECTION is defined %}
tarch::la::Matrix<{{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::FaceUnknownsSolution * TwoTimesD, double> {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getCellFromFaceMatrix(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
)
{
  logTraceInWith2Arguments("getCellFromFaceMatrix", cellCentre, cellSize);
  static std::vector< tarch::la::Matrix<CellUnknowns, FaceUnknownsSolution * TwoTimesD, double > > matrices = {
    {% for MATRIX in CELL_FROM_FACE_PROJECTION %}
    {
      {{MATRIX[0]| join(", ")}}
        {% for ROW in MATRIX[1:] %}
        ,{{ROW | join(", ")}}
      {% endfor %}
    },
    {% endfor %}
  };

  {% if CELL_FROM_FACE_PROJECTION_SCALING is not defined %}
  #error If matrices are predefined, scaling has to be defined, too
  {% endif %}

  static std::vector<int> scaleFactors = {
      {% for el in CELL_FROM_FACE_PROJECTION_SCALING %}
        {{el}},
      {% endfor %}
  };

  static tarch::la::Matrix< CellUnknowns, FaceUnknownsSolution * TwoTimesD, double > result = scaleMatrix( matrices, scaleFactors, cellSize );
  logTraceOutWith1Argument("getCellFromFaceMatrix", result);
  return result;
} 
{% endif %}


{% if RIEMANN_MATRIX is defined %}
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

{% if BOUNDARY_MATRIX is defined %}
tarch::la::Matrix<{{NAMESPACE | join("::")}}::{{CLASSNAME}}::FaceUnknownsSolution, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::FaceUnknownsSolution, double> {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getBoundaryConditionMatrix()
{
  /*
  Warning - the shape of this is a bodge. Fixing it to be identity.
  */
  static tarch::la::Matrix<  FaceUnknownsSolution, FaceUnknownsSolution, double > result = {
      {{BOUNDARY_MATRIX[0]| join(", ")}}
      {% for ROW in BOUNDARY_MATRIX[1:] %}
        ,{{ROW | join(", ")}}
      {% endfor %}
  };

  return result;
} 
{% endif %}


{% if APPROX_SYSTEM_MATRIX is defined %}
tarch::la::Matrix< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getInvertedApproxSystemMatrix(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
) {
  static std::vector< tarch::la::Matrix< CellUnknowns, CellUnknowns, double > > matrices = {
    {% for MATRIX in APPROX_SYSTEM_MATRIX %}
    {
      {{MATRIX[0]| join(", ")}}
        {% for ROW in MATRIX[1:] %}
        ,{{ROW | join(", ")}}
      {% endfor %}
    },
    {% endfor %}
  };

  {% if APPROX_SYSTEM_MATRIX_SCALING is not defined %}
  #error If matrices are predefined, scaling has to be defined, too
  {% endif %}

  static std::vector<int> scaleFactors = {
      {% for el in APPROX_SYSTEM_MATRIX_SCALING %}
        {{el}},
      {% endfor %}
  };

  static tarch::la::Matrix< CellUnknowns, CellUnknowns, double > result = scaleMatrix( matrices, scaleFactors, cellSize );

  return result;
}
{% endif %}


