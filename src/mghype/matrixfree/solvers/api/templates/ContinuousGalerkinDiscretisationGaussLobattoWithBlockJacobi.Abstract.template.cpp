#include "{{CLASSNAME}}.h"
#include "mghype/mghype.h"
#include "tarch/la/LUDecomposition.h"

tarch::logging::Log  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log( "{{NAMESPACE | join("::")}}::{{CLASSNAME}}" );


const tarch::la::Vector< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::PolyDegree+1, double >   {{NAMESPACE | join("::")}}::{{CLASSNAME}}::IntegrationPoints = { {{INTEGRATION_POINTS | join(", ")}} };


const tarch::la::Matrix<{{NAMESPACE | join("::")}}::{{CLASSNAME}}::InnerCellUnknowns, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, double>  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::NarrowToInteriorMatrix = {
  {{NARROW_TO_INTERIOR_MATRIX[0]| join(", ")}}
  {% for ROW in NARROW_TO_INTERIOR_MATRIX[1:] %}
    ,{{ROW | join(", ")}}
  {% endfor %}
};


tarch::la::Vector< Dimensions, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getIntegrationPoint(
  const tarch::la::Vector< Dimensions, int >& pos
) {
  tarch::la::Vector< Dimensions, double > result;
  for (int d=0; d<Dimensions; d++) {
    result(d) = IntegrationPoints( pos(d) );
  }
  return result;
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}():
  Solver("{{CLASSNAME}}", {{SOLVER_TOLERANCE}} ),
  _state( State::ProjectOntoFacesOnly )
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


{{NAMESPACE | join("::")}}::facedata::{{SOLVER_NAME}}::Type {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getFaceType(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h
) {
  return isOnBoundary(x) ?
    facedata::{{SOLVER_NAME}}::Type::Boundary:
    facedata::{{SOLVER_NAME}}::Type::Interior;
}


{{NAMESPACE | join("::")}}::celldata::{{SOLVER_NAME}}::Type {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getCellType(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h
) {
  return celldata::{{SOLVER_NAME}}::Type::Interior;
}


{% if RHS_MATRIX is defined %}
tarch::la::Matrix< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CellUnknowns, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getRhsMatrix(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
) {
  logTraceInWith2Arguments("getRhsMatrix", cellCentre, cellSize);
  static std::vector< tarch::la::Matrix< CellUnknowns, CellUnknowns, double > > matrices = {
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

  static std::vector<int> scaleFactors = {
      {% for el in RHS_MATRIX_SCALING %}
        {{el}},
      {% endfor %}
  };

  // may not be static, as it depends upon h
  tarch::la::Matrix< CellUnknowns, CellUnknowns, double > result = ::mghype::composeMatrixFromHWeightedLinearCombination( matrices, scaleFactors, cellSize );

  logTraceOutWith1Argument("getRhsMatrix", result);
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


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::beginMeshSweep() {
  if (_state==State::Solve
      or
      _state==State::UpdateAndRestrictResidual
  ) {
    clearGlobalResidualAndSolutionUpdate();
  }
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::endMeshSweep() {
  switch (_state) {
    case State::Solve:
      {
        logInfo( "endMeshSweep()", toString() );
      }
      break;
    
    /*
    This means we just prolongated from CG -> DG and projected
    the new solution back to the faces. So let's continue to solve
    as normal, remembering that the Riemann problem will be solved first.
    */
    case State::ProjectOntoFacesOnly:
      {
        _state = State::Solve;
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
        _state = State::ProjectOntoFacesOnly;
      }
      break;

    case State::UpdateAndRestrictResidual:
      {
        logInfo( "endMeshSweep()", toString() );
        _state = State::Suspend; 
      }
      break;
  }
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::updateCell() const {
  return _state == State::Solve
      or _state == State::ProjectOntoFacesOnly;
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::updateFace() const {
  return _state == State::Solve
      or _state == State::UpdateAndRestrictResidual;
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::projectOntoFaces() const {
  return _state == State::Solve
      or _state == State::ProjectOntoFacesOnly
      or _state == State::UpdateAndRestrictResidual;
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::projectOntoCells() const {
  return _state == State::Solve
      or _state == State::UpdateAndRestrictResidual;
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::isSuspended() const{
  return _state == State::Suspend;
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::suspend(bool projectOntoFaces, bool updateAndRestrictResidual) {
  if (projectOntoFaces) {
    _state = State::ProjectOntoFacesOnly;
  }
  else if (updateAndRestrictResidual) {
    _state = State::UpdateAndRestrictResidual;
  }
  else {
    _state = State::Suspend;
  }
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::State {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getState() const { 
  return _state; 
}


