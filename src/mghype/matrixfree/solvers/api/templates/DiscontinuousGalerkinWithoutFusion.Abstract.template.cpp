#include "{{CLASSNAME}}.h"
#include "mghype/mghype.h"
#include "tarch/la/LUDecomposition.h"

tarch::logging::Log  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log( "{{NAMESPACE | join("::")}}::{{CLASSNAME}}" );

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}():
  Solver("{{CLASSNAME}}", {{SOLVER_TOLERANCE}} ),
  _action( Action::ProjectOntoFaces )
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

  static tarch::la::Matrix< CellUnknowns, CellUnknowns, double > result = ::mghype::composeMatrixFromHWeightedLinearCombination( matrices, scaleFactors, cellSize );

  static tarch::la::Matrix< CellUnknowns, CellUnknowns, double > invertedResult = tarch::la::invert( result );

  return invertedResult;
}
{% endif %}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::beginMeshSweep() {
  // we need to clear the residual at some point after reporting, which occurs during
  // ProjectIntoCellAndUpdateCellSolution action set. So we clear it just before then
  if ( _action == Action::ProjectIntoCellAndUpdateCellSolution )
    clearGlobalResidualAndSolutionUpdate();

  logInfo("beginMeshSweep", "running phase " << toString(_action));
}

/**
 * Cycle through each of the action sets
 */
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::endMeshSweep() {
  switch (_action) {
    case Action::ProjectOntoFaces:
    {
      _action = Action::ProjectIntoCellAndUpdateCellSolution;
    } break;

    case Action::ProjectIntoCellAndUpdateCellSolution:
    {
      _action = Action::UpdateResidual;
    } break;

    case Action::UpdateResidual:
    {
      _action = Action::UpdateFaceSolution;
    } break;

    case Action::UpdateFaceSolution:
    {
      _action = Action::ProjectOntoFaces;
    } break;

  }

  logInfo( "endMeshSweep()", toString() );
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::updateResidual() const {
  return _action == Action::UpdateResidual;
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::updateFace() const {
  return _action == Action::UpdateFaceSolution;
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::projectOntoFaces() const {
  return _action == Action::ProjectOntoFaces;
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::projectOntoCells() const {
  return _action == Action::ProjectIntoCellAndUpdateCellSolution;
}


std::string {{NAMESPACE | join("::")}}::{{CLASSNAME}}::toString(Action state) const {
  std::ostringstream output;
  switch (state){
    case Action::ProjectOntoFaces:
    {
      output << "ProjectOntoFaces";
    } break;
    case Action::ProjectIntoCellAndUpdateCellSolution:
    {
      output << "ProjectIntoCellAndUpdateCellSolution";
    } break;
    case Action::UpdateResidual:
    {
      output << "UpdateResidual";
    } break;
    case Action::UpdateFaceSolution:
    {
      output << "UpdateFaceSolution";
    } break;
    default:
    {} break;
  }

  return output.str();
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::Action {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getState() const { return _action; }
