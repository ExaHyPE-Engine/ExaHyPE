#include "{{CLASSNAME}}.h"
#include "mghype/mghype.h"
#include "tarch/multicore/Lock.h"

tarch::logging::Log  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log( "{{NAMESPACE | join("::")}}::{{CLASSNAME}}" );

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}():
  Solver("{{CLASSNAME}}", {{SOLVER_TOLERANCE}} ),
  _state(State::Solve), _activeLevel(-1), _smoothingState(SmoothingState::PreSmoothing),
  _preSmoothingStepsCounter(0), _postSmoothingStepsCounter(0)
{}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::~{{CLASSNAME}}() {}

{% if LOCAL_ASSEMBLY_MATRIX is defined %}
tarch::la::Matrix< {{VERTEX_CARDINALITY}}*TwoPowerD, {{VERTEX_CARDINALITY}}*TwoPowerD, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getLocalAssemblyMatrix(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
) const {
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
tarch::la::Matrix< {{VERTEX_CARDINALITY}}*TwoPowerD, {{VERTEX_CARDINALITY}}*TwoPowerD, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMassMatrix(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize,
  const int level
) const {
  // first define the standard mass matrix
  static std::vector< tarch::la::Matrix< {{VERTEX_CARDINALITY}}*TwoPowerD, {{VERTEX_CARDINALITY}}*TwoPowerD, double > > massMatrices = {
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

  static std::vector<int> massMatrixScaleFactors = {
      {% for el in MASS_MATRIX_SCALING %}
        {{el}},
      {% endfor %}
  };

  // may not be static, as it depends upon h
  tarch::la::Matrix< {{VERTEX_CARDINALITY}}*TwoPowerD, {{VERTEX_CARDINALITY}}*TwoPowerD, double > massMatrix = ::mghype::composeMatrixFromHWeightedLinearCombination( massMatrices, massMatrixScaleFactors, cellSize );


  // next the correction equation matrices

  static std::vector< tarch::la::Matrix< {{VERTEX_CARDINALITY}}*TwoPowerD, {{VERTEX_CARDINALITY}}*TwoPowerD, double > > correctionMatrices = {
    {% for MATRIX in CORRECTION_MATRIX %}
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

  static std::vector<int> correctionMatrixScaleFactors = {
      {% for el in CORRECTION_MATRIX_SCALING %}
        {{el}},
      {% endfor %}
  };

  // may not be static, as it depends upon h
  tarch::la::Matrix< {{VERTEX_CARDINALITY}}*TwoPowerD, {{VERTEX_CARDINALITY}}*TwoPowerD, double > correctionMatrix = ::mghype::composeMatrixFromHWeightedLinearCombination( correctionMatrices, correctionMatrixScaleFactors, cellSize );

  return (level==getFinestLevel()) ? massMatrix : correctionMatrix;
}
{% endif %}

{% if RESTRICTION_MATRIX is defined  %}
tarch::la::Matrix< {{VERTEX_CARDINALITY}}*TwoPowerD, {{VERTEX_CARDINALITY}}*FourPowerD, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getRestrictionMatrix(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
) const {
  const static tarch::la::Matrix< {{VERTEX_CARDINALITY}}*TwoPowerD, {{VERTEX_CARDINALITY}}*FourPowerD, double > result =
    {
      {{RESTRICTION_MATRIX[0]| join(", ")}}
        {% for ROW in RESTRICTION_MATRIX[1:] %}
        ,{{ROW | join(", ")}}
      {% endfor %}
    };

  return result;
}
{% endif %}

{% if PROLONGATION_MATRIX is defined  %}
tarch::la::Matrix< {{VERTEX_CARDINALITY}}*FourPowerD, {{VERTEX_CARDINALITY}}*TwoPowerD, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getProlongationMatrix(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
) const {
  const static tarch::la::Matrix< {{VERTEX_CARDINALITY}}*FourPowerD, {{VERTEX_CARDINALITY}}*TwoPowerD, double > result =
    {
      {{PROLONGATION_MATRIX[0]| join(", ")}}
        {% for ROW in PROLONGATION_MATRIX[1:] %}
        ,{{ROW | join(", ")}}
      {% endfor %}
    };

  return result;
}
{% endif %}


{{NAMESPACE | join("::")}}::vertexdata::{{SOLVER_NAME}}::Type {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getVertexDoFType(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h
) const {
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

{{NAMESPACE | join("::")}}::celldata::{{SOLVER_NAME}}::Type {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getCellDoFType(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h
) const {
  return celldata::{{SOLVER_NAME}}::Type::Interior;
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::suspend() {
  _state = State::Suspend;
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::beginMeshSweep() {
  if ( _activeLevel == getFinestLevel() )
    clearGlobalResidualAndSolutionUpdate();
  logInfo("beginMeshSweep()", "running phase " << toString(_smoothingState) << " at level " << _activeLevel);

}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::endMeshSweep() {
  if (_state == State::Solve) {
    logInfo( "endMeshSweep()", toString() );
  }

  // control flow
  _state = State::Solve;

  switch( _smoothingState ){
    case SmoothingState::PreSmoothing:
    // always increment the counter, and change phase if we pass
    // the number of steps we specified
    {
      if (
        ++_preSmoothingStepsCounter >= PreSmoothingSteps
       ) {
        // reset the counter and restrict to the
        // next level coarser
        _preSmoothingStepsCounter = 0;
        _smoothingState = SmoothingState::Restrict;
      }

    } break;

    case SmoothingState::Restrict:
    /*
    Finished restriction. Time to move up one level
    */
    {
      // decrement activeLevel, but only go back to PreSmoothing if
      // we aren't at the coarsest level
      if (  --_activeLevel == 0 )
        _smoothingState = SmoothingState::PostSmoothing;
      else
        _smoothingState = SmoothingState::PreSmoothing;
    } break;

    case SmoothingState::PostSmoothing:
    {
      if (
        ++_postSmoothingStepsCounter >= PostSmoothingSteps
      ) {
        _postSmoothingStepsCounter = 0;

        // if we're already on finest level, time to move to presmoothing again
        if ( _activeLevel == getFinestLevel() ) {
          _smoothingState = SmoothingState::PreSmoothing;
        }
        else _activeLevel++;
      }

    } break;

    default:
      assertion(false);
  }
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::update(int level) const {
  /*
  We want perform Jacobi on the active level if we're in PreSmoothing,
  Restriction or PostSmoothing. So allow all of them!
  */
  return 
    (_state == State::Solve)
    and
    (level == _activeLevel);
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::restrict() const {
  /*
  In the restriction phase, we do something on one above the active level and something
  on the active level. So this function returns true if we are in Restriction mode.
  */
  return
    (_state == State::Solve)
    and
    (_smoothingState == SmoothingState::Restrict);
    // and
    // (level==_activeLevel+1);
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::prolongate() const {
  return
    (_smoothingState == SmoothingState::PostSmoothing)
    and
    (_postSmoothingStepsCounter == 0)
    and
    (_activeLevel >= 1); // we only want prolongations from real vertices!
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::setActiveLevel(int level) {
  tarch::multicore::Lock lock( _semaphore );
  _activeLevel = level;
}

int {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getActiveLevel() const {
  return _activeLevel;
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::setFinestLevel(int level) {
  tarch::multicore::Lock lock( _semaphore );
  if (level>_finestLevel) {
    _finestLevel = level;
  }
}

int {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getFinestLevel() const {
  return _finestLevel;
}

std::string {{NAMESPACE | join("::")}}::{{CLASSNAME}}::toString(SmoothingState state) const {
  std::ostringstream output;
  switch (state){
    case SmoothingState::PreSmoothing:
    {
      output << "PreSmoothing" << _preSmoothingStepsCounter;
    } break;
    case SmoothingState::Restrict:
    {
      output << "Restrict";
    } break;
    case SmoothingState::PostSmoothing:
    {
      output << "PostSmoothing" << _postSmoothingStepsCounter;
    } break;
    default:
    {
      // output  "";
    } break;
  }

  return output.str();
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::State {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getState() const {
  return _state;
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::SmoothingState {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getSmoothingState() {
  return _smoothingState;
}
