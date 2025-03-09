#include "{{CLASSNAME}}.h"
#include "mghype/mghype.h"
#include "tarch/la/LUDecomposition.h"

tarch::logging::Log  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log( "{{NAMESPACE | join("::")}}::{{CLASSNAME}}" );

const tarch::la::Vector< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::PolyDegree+1, double >   {{NAMESPACE | join("::")}}::{{CLASSNAME}}::IntegrationPoints = { {{INTEGRATION_POINTS | join(", ")}} };


namespace {
  {% if RHS_MATRIX is defined %}
  std::vector< tarch::la::Matrix< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, double > > RhsMatrices = {
    {% for MATRIX in RHS_MATRIX %}
    // {# RHS_MATRIX is an array, possible of length 1 #}
    {
      {{MATRIX[0]| join(", ")}}
        {% for ROW in MATRIX[1:] %}
        ,{{ROW | join(", ")}}
      {% endfor %}
    },
    {% endfor %}
  };

  {% if RHS_MATRIX_SCALING is not defined %}
  #error If matrices are predefined, scaling has to be defined, too
  {% endif %}

  std::vector<int> RhsScaleFactors = {
      {% for el in RHS_MATRIX_SCALING %}
        {{el}},
      {% endfor %}
  };
  {% endif %}


  {% if ASSEMBLY_MATRIX is defined %}
  std::vector< tarch::la::Matrix< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, double > > AssemblyMatrices = {
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

  std::vector<int> AssemblyScaleFactors = {
        {% for el in ASSEMBLY_MATRIX_SCALING %}
          {{el}},
        {% endfor %}
    };
  {% endif %}


  {% if FACE_FROM_CELL_PROJECTION is defined %}
  std::vector< tarch::la::Matrix< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::FaceUnknownsProjection * TwoTimesD, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, double > > FaceFromCellProjectionMatrices = {
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

  std::vector<int> FaceFromCellProjectionScaleFactors = {
        {% for el in FACE_FROM_CELL_PROJECTION_SCALING %}
          {{el}},
        {% endfor %}
    };
  {% endif %}


  {% if CELL_FROM_FACE_PROJECTION is defined %}
  std::vector< tarch::la::Matrix<{{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::FaceUnknownsSolution * TwoTimesD, double > > CellFromFaceMatrices = {
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

  static std::vector<int> CellFromFaceScaleFactors = {
        {% for el in CELL_FROM_FACE_PROJECTION_SCALING %}
          {{el}},
        {% endfor %}
    };
  {% endif %}


  {% if APPROX_SYSTEM_MATRIX is defined %}
  std::vector< tarch::la::Matrix< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, double > > ApproxSystemMatrices = {
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

  std::vector<int> ApproxSystemMatricesScaleFactors = {
        {% for el in APPROX_SYSTEM_MATRIX_SCALING %}
          {{el}},
        {% endfor %}
    };
  {% endif %}
}


tarch::la::Vector< Dimensions, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getIntegrationPoint(
  const tarch::la::Vector< Dimensions, int >& pos
) {
  tarch::la::Vector< Dimensions, double > result;
  for (int d=0; d<Dimensions; d++) {
    assertion1( pos(d)>=0, pos );
    assertion1( pos(d)<PolyDegree+1, pos );
    result(d) = IntegrationPoints( pos(d) );
  }
  return result;
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}():
  Solver("{{CLASSNAME}}", {{SOLVER_TOLERANCE}} ),
  _state( State::ProjectOntoFaces )
{}

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


{{NAMESPACE | join("::")}}::celldata::{{SOLVER_NAME}}::Type {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getCellType(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h
) {
  return celldata::{{SOLVER_NAME}}::Type::Interior;
}

{{NAMESPACE | join("::")}}::facedata::{{SOLVER_NAME}}::Type {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getFaceType(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h
) {
  return isOnBoundary(x) ?
    facedata::{{SOLVER_NAME}}::Type::Boundary:
    facedata::{{SOLVER_NAME}}::Type::Interior;
}


{% if RHS_MATRIX is defined %}
tarch::la::Matrix< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getRhsMatrix(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
) {
  logTraceInWith2Arguments("getRhsMatrix", cellCentre, cellSize);

  // may not be static, as it depends upon h
  tarch::la::Matrix< CellUnknowns, CellUnknowns, double > result = ::mghype::composeMatrixFromHWeightedLinearCombination(
      RhsMatrices,
      RhsScaleFactors,
      cellSize
  );

  logTraceOutWith1Argument("getRhsMatrix", result);
  return result;
}


tarch::la::Vector< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::applyRhsMatrix(
  const tarch::la::Vector<Dimensions, double>&      cellCentre,
  const tarch::la::Vector<Dimensions, double>&      cellSize,
  const tarch::la::Vector< CellUnknowns, double >&  v
) {
  // getRhsMatrix(cellCentre, cellSize) * v;
  return ::mghype::applyMatrixFromHWeightedLinearCombination(
    RhsMatrices,
    RhsScaleFactors,
    cellSize,
    v
  );
}
{% endif %}


{% if ASSEMBLY_MATRIX is defined %}
tarch::la::Matrix< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getLocalAssemblyMatrix(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
) {
  logTraceInWith2Arguments("getLocalAssemblyMatrix", cellCentre, cellSize);

  // may not be static, as it depends upon h
  tarch::la::Matrix< CellUnknowns, CellUnknowns, double > result = ::mghype::composeMatrixFromHWeightedLinearCombination(
    AssemblyMatrices,
    AssemblyScaleFactors,
    cellSize
  );

  logTraceOutWith1Argument("getLocalAssemblyMatrix", result);
  return result;
}


tarch::la::Vector< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::applyLocalAssemblyMatrix(
  const tarch::la::Vector<Dimensions, double>&      cellCentre,
  const tarch::la::Vector<Dimensions, double>&      cellSize,
  const tarch::la::Vector< CellUnknowns, double >&  v
) {
  // getLocalAssemblyMatrix(cellCentre,cellSize) * v;
  return ::mghype::applyMatrixFromHWeightedLinearCombination(
    AssemblyMatrices,
    AssemblyScaleFactors,
    cellSize,
    v
  );
}
{% endif %}


{% if FACE_FROM_CELL_PROJECTION is defined %}
tarch::la::Matrix<{{NAMESPACE | join("::")}}::{{CLASSNAME}}::FaceUnknownsProjection * TwoTimesD, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, double> {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getFaceFromCellMatrix(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
)
{
  logTraceInWith2Arguments("getFaceFromCellMatrix", cellCentre, cellSize);
  tarch::la::Matrix< FaceUnknownsProjection * TwoTimesD, CellUnknowns, double > result = ::mghype::composeMatrixFromHWeightedLinearCombination(
    FaceFromCellProjectionMatrices,
    FaceFromCellProjectionScaleFactors,
    cellSize
  );
  logTraceOutWith1Argument("getFaceFromCellMatrix", result);
  return result;
} 


tarch::la::Vector< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::FaceUnknownsProjection * TwoTimesD, double> {{NAMESPACE | join("::")}}::{{CLASSNAME}}::applyFaceFromCellMatrix(
  const tarch::la::Vector<Dimensions, double>&     cellCentre,
  const tarch::la::Vector<Dimensions, double>&     cellSize,
  const tarch::la::Vector< CellUnknowns, double>&  v
) {
  return ::mghype::applyMatrixFromHWeightedLinearCombination(
    FaceFromCellProjectionMatrices,
    FaceFromCellProjectionScaleFactors,
    cellSize,
    v
  );
}
{% endif %}


{% if CELL_FROM_FACE_PROJECTION is defined %}
tarch::la::Matrix<{{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::FaceUnknownsSolution * TwoTimesD, double> {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getCellFromFaceMatrix(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
)
{
  logTraceInWith2Arguments("getCellFromFaceMatrix", cellCentre, cellSize);
  tarch::la::Matrix< CellUnknowns, FaceUnknownsSolution * TwoTimesD, double > result = ::mghype::composeMatrixFromHWeightedLinearCombination(
    CellFromFaceMatrices,
    CellFromFaceScaleFactors,
    cellSize
  );
  logTraceOutWith1Argument("getCellFromFaceMatrix", result);
  return result;
} 


tarch::la::Vector< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::applyCellFromFaceMatrix(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize,
  const tarch::la::Vector< FaceUnknownsSolution * TwoTimesD, double >& v
) {
  // getCellFromFaceMatrix(cellCentre,cellSize)*v;
  return ::mghype::applyMatrixFromHWeightedLinearCombination(
    CellFromFaceMatrices,
    CellFromFaceScaleFactors,
    cellSize,
    v
  );
}
{% endif %}


{% if RIEMANN_MATRIX is defined %}
tarch::la::Matrix<{{NAMESPACE | join("::")}}::{{CLASSNAME}}::FaceUnknownsSolution, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::FaceUnknownsProjection, double> {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getRiemannMatrix() {
  static tarch::la::Matrix<  FaceUnknownsSolution, FaceUnknownsProjection, double > result = {
      {{RIEMANN_MATRIX[0]| join(", ")}}
      {% for ROW in RIEMANN_MATRIX[1:] %}
        ,{{ROW | join(", ")}}
      {% endfor %}
  };

  return result;
} 


tarch::la::Vector< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::FaceUnknownsSolution, double> {{NAMESPACE | join("::")}}::{{CLASSNAME}}::applyRiemannMatrix(
  const tarch::la::Vector< FaceUnknownsProjection, double>& v
) {
  return getRiemannMatrix() * v;
}
{% endif %}


{% if BOUNDARY_MATRIX is defined %}
tarch::la::Matrix<{{NAMESPACE | join("::")}}::{{CLASSNAME}}::FaceUnknownsSolution, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::FaceUnknownsSolution, double> {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getBoundaryConditionMatrix() {
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


tarch::la::Vector< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::FaceUnknownsSolution, double> {{NAMESPACE | join("::")}}::{{CLASSNAME}}::applyBoundaryConditionMatrix(
  const tarch::la::Vector< FaceUnknownsSolution, double>& v
) {
  return getBoundaryConditionMatrix() * v;
}
{% endif %}


{% if APPROX_SYSTEM_MATRIX is defined %}
tarch::la::Matrix< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getInvertedApproxSystemMatrix(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
) {
  tarch::la::Matrix< CellUnknowns, CellUnknowns, double > result = ::mghype::composeMatrixFromHWeightedLinearCombination(
    ApproxSystemMatrices,
    ApproxSystemMatricesScaleFactors, cellSize );

  tarch::la::Matrix< CellUnknowns, CellUnknowns, double > invertedResult = tarch::la::invert( result );

  return invertedResult;
}


tarch::la::Vector< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::applyInvertedApproxSystemMatrix(
  const tarch::la::Vector<Dimensions, double>&     cellCentre,
  const tarch::la::Vector<Dimensions, double>&     cellSize,
  const tarch::la::Vector< CellUnknowns, double >& v
) {
  return getInvertedApproxSystemMatrix(cellCentre,cellSize) * v;
}
{% endif %}


// ===============================
// Solver-specific content will be appended here
// ===============================
