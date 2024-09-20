#include "{{CLASSNAME}}.h"
#include "mghype/mghype.h"
#include "tarch/la/LUDecomposition.h"

tarch::logging::Log  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log( "{{NAMESPACE | join("::")}}::{{CLASSNAME}}" );

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}():
  Solver("{{CLASSNAME}}", {{SOLVER_TOLERANCE}} ),
  _state( State::ProjectOnly )
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

  // may not be static, as it depends upon h
  tarch::la::Matrix< CellUnknowns, CellUnknowns, double > result = ::mghype::composeMatrixFromHWeightedLinearCombination( matrices, scaleFactors, cellSize );

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

  // may not be static, as it depends upon h
  tarch::la::Matrix< CellUnknowns, CellUnknowns, double > result = ::mghype::composeMatrixFromHWeightedLinearCombination( matrices, scaleFactors, cellSize );

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

  // may not be static, as it depends upon h
  tarch::la::Matrix< FaceUnknownsProjection * TwoTimesD, CellUnknowns, double > result = ::mghype::composeMatrixFromHWeightedLinearCombination( matrices, scaleFactors, cellSize );
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

  // may not be static, as it depends upon h
  tarch::la::Matrix< CellUnknowns, FaceUnknownsSolution * TwoTimesD, double > result = ::mghype::composeMatrixFromHWeightedLinearCombination( matrices, scaleFactors, cellSize );
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

  // In general, this may not be static, due to H-dependence. However, for the time being 
  // we keep it static for a couple of three reasons:

  // 1. The only non trivial matrix that we insert here (at time of writing) does not need scaling
  // 2. The BLAS implementation on COSMA complains whenever we enter this function in an OMP region
  // 3. We don't refine h any further once we reach the solver region

  static tarch::la::Matrix< CellUnknowns, CellUnknowns, double > result = ::mghype::composeMatrixFromHWeightedLinearCombination( matrices, scaleFactors, cellSize );

  static tarch::la::Matrix< CellUnknowns, CellUnknowns, double > invertedResult = tarch::la::invert( result );

  return invertedResult;
}
{% endif %}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::beginMeshSweep() {
  if (_state==State::UpdateAndProjectImmediately) {
    clearGlobalResidual();
  }
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::endMeshSweep() {
  switch (_state) {
    case State::UpdateAndProjectImmediately:
      {
        logInfo( "endMeshSweep()", toString() );
      }
      break;
    case State::ProjectOnly:
      {
        _state = State::UpdateAndProjectImmediately;
      }
      break;
      /*
    case State::RiemannSolverAndUpdate:
      {
        logInfo( "endMeshSweep()", toString() );
        _state = State::TriggerVolumetricKernelsAndProject;
      }
      break;
*/
    case State::Suspend:
      {
        _state = State::UpdateAndProjectImmediately;
      }
      break;
  }
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::updateCell() const {
  return _state == State::UpdateAndProjectImmediately;
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::updateFace() const {
  return _state == State::UpdateAndProjectImmediately;
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::projectOntoFaces() const {
  return _state == State::UpdateAndProjectImmediately
      or _state == State::ProjectOnly;
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::suspend(bool projectOntoFaces) {
  if (projectOntoFaces) {
    _state = State::ProjectOnly;
  }
  else {
    _state = State::Suspend;
  }
}
