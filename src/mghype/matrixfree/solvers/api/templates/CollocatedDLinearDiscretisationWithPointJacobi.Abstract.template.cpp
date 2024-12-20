#include "{{CLASSNAME}}.h"
#include "mghype/mghype.h"


tarch::logging::Log  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log( "{{NAMESPACE | join("::")}}::{{CLASSNAME}}" );

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}():
  Solver("{{CLASSNAME}}", {{SOLVER_TOLERANCE}} ),
  _state(State::Solve), _updateSolution(false)
{}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::~{{CLASSNAME}}() {}


{% if LOCAL_ASSEMBLY_MATRIX is defined %}
tarch::la::Matrix< {{VERTEX_CARDINALITY}}*TwoPowerD, {{VERTEX_CARDINALITY}}*TwoPowerD, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getLocalAssemblyMatrix(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
) {
  logTraceInWith2Arguments("getLocalAssemblyMatrix", cellCentre, cellSize);
  static std::vector< tarch::la::Matrix< {{VERTEX_CARDINALITY}}*TwoPowerD, {{VERTEX_CARDINALITY}}*TwoPowerD, double > > matrices = {
    {% for MATRIX in LOCAL_ASSEMBLY_MATRIX %}
    {
      {{MATRIX[0]| join(", ")}}
        {% for ROW in MATRIX[1:] %}
        ,{{ROW | join(", ")}}
      {% endfor %}
    },
    {% endfor %}
  };

  {% if LOCAL_ASSEMBLY_MATRIX_SCALING is not defined %}
  #error If matrices are predefined, scaling has to be defined, too
  {% endif %}

  static std::vector<int> scaleFactors = {
      {% for el in LOCAL_ASSEMBLY_MATRIX_SCALING %}
        {{el}},
      {% endfor %}
  };

  // may not be static, as it depends upon h
  tarch::la::Matrix< {{VERTEX_CARDINALITY}}*TwoPowerD, {{VERTEX_CARDINALITY}}*TwoPowerD, double > result = ::mghype::composeMatrixFromHWeightedLinearCombination( matrices, scaleFactors, cellSize );

  logTraceOutWith1Argument("getLocalAssemblyMatrix", result);
  return result;
}
{% endif %}


{% if MASS_MATRIX is defined  %}
tarch::la::Matrix< {{VERTEX_CARDINALITY}}*TwoPowerD, {{VERTEX_CARDINALITY}}*TwoPowerD, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getRhsMatrix(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
) {
  static std::vector< tarch::la::Matrix< {{VERTEX_CARDINALITY}}*TwoPowerD, {{VERTEX_CARDINALITY}}*TwoPowerD, double > > matrices = {
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
  tarch::la::Matrix< {{VERTEX_CARDINALITY}}*TwoPowerD, {{VERTEX_CARDINALITY}}*TwoPowerD, double > result = ::mghype::composeMatrixFromHWeightedLinearCombination( matrices, scaleFactors, cellSize );

  return result;
}
{% endif %}


{{NAMESPACE | join("::")}}::vertexdata::{{SOLVER_NAME}}::Type {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getVertexType(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h
) {
  auto isOnBoundary = [&]( const tarch::la::Vector< Dimensions, double > & x ) -> bool{
    bool isOnBoundary = false;
    for (int d=0; d<Dimensions; d++) {
      isOnBoundary |= tarch::la::smallerEquals( x(d), DomainOffset(d) );
      isOnBoundary |= tarch::la::greaterEquals( x(d), DomainOffset(d)+DomainSize(d) );
    }
    return isOnBoundary;
  };

  return isOnBoundary(x) ? vertexdata::{{SOLVER_NAME}}::Type::Boundary : vertexdata::{{SOLVER_NAME}}::Type::Interior;
}


{{NAMESPACE | join("::")}}::celldata::{{SOLVER_NAME}}::Type {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getCellType(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h
) {
  return celldata::{{SOLVER_NAME}}::Type::Interior;
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::suspend() {
  _state = State::Suspend;

  // we don't want to update the solution immediately when we
  // revisit this solver. We will have updated the solution itself 
  // during the DGCGCoupling, and we don't want to re-update the
  // solution with an out-of-date residual.
  _updateSolution = false;
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::isSuspended() const {
  return _state == State::Suspend;
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::beginMeshSweep() {
  clearGlobalResidualAndSolutionUpdate();
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::endMeshSweep() {
  if ( _state == State::Solve ) {
    synchroniseGlobalResidualAndSolutionUpdate();
    
    // if we were already in State::Solve, then we've done at 
    // least one sweep, and we can safely re-enable solution updates.
    _updateSolution = true;
  }

  if (
    tarch::mpi::Rank::getInstance().isGlobalMaster()
    and
    _state == State::Solve
  ) {
    logInfo( "endMeshSweep()", toString() );
  }

  _state = State::Solve;
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::updateSolution() const {
  return 
    _state == State::Solve
    and
    _updateSolution;
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::updateResidual() const {
  return _state == State::Solve;
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::State {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getState() const {
  return _state;
}

{{ABSTRACT_SOLVER_IMPLEMENTATION_EXTRAS}}
