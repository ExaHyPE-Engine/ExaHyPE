#include "{{CLASSNAME}}.h"
#include "mghype/mghype.h"
#include "tarch/multicore/Lock.h"

tarch::logging::Log  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log( "{{NAMESPACE | join("::")}}::{{CLASSNAME}}" );

static_assert( {{NAMESPACE | join("::")}}::{{CLASSNAME}}::CoarsestLevel >= 0, "Coarsest level must be at least 0" );

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}():
  Solver("{{CLASSNAME}}", {{SOLVER_TOLERANCE}} ),
  _state(State::Solve), _activeLevel(-1), _smoothingState(SmoothingState::PreSmoothing),
  _preSmoothingStepsCounter(0), _postSmoothingStepsCounter(0), _vCycleCounter(0)
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
tarch::la::Matrix< {{VERTEX_CARDINALITY}}*TwoPowerD, {{VERTEX_CARDINALITY}}*TwoPowerD, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getRhsMatrix(
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


{{NAMESPACE | join("::")}}::vertexdata::{{SOLVER_NAME}}::Type {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getVertexType(
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

{{NAMESPACE | join("::")}}::celldata::{{SOLVER_NAME}}::Type {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getCellType(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h
) const {
  return celldata::{{SOLVER_NAME}}::Type::Interior;
}

/**
 * If we suspend, we want to make sure that the program phase
 * is in its initial state for the next time we revisit.
 */
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::suspend() {
  _state = State::Suspend;

  _smoothingState            = SmoothingState::PreSmoothing;
  _preSmoothingStepsCounter  = 0;
  _postSmoothingStepsCounter = 0;
  _activeLevel               = getFinestLevel();
  _vCycleCounter             = 0;
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::beginMeshSweep() {
  if ( 
    _activeLevel == getFinestLevel()
    and
    (
      // we won't be updating the vertex solution at this step, instead we just calculate the residual
      // and restrict it. Hence we don't want to clear out the residual now, since the abstract solver
      // will think that the max residual is now 0, and wrongly terminate early.
      _smoothingState == SmoothingState::PreSmoothing and _preSmoothingStepsCounter < PreSmoothingSteps
      or
      _smoothingState == SmoothingState::PostSmoothing
    )
  )
    clearGlobalResidualAndSolutionUpdate();

  if (
    _state == State::Solve
    and
    tarch::mpi::Rank::getInstance().isGlobalMaster()
  )
    logInfo("beginMeshSweep()", "running phase " << toString(_smoothingState) << " at level " << _activeLevel << ". Completed " << getVCycleCounter() << " V-Cycles");
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::endMeshSweep() {
  if (_state == State::Solve) {
    synchroniseGlobalResidualAndSolutionUpdate();
    if ( tarch::mpi::Rank::getInstance().isGlobalMaster() )
      logInfo( "endMeshSweep()", toString() );
  }

  // control flow
  _state = State::Solve;

  switch( _smoothingState ){
    case SmoothingState::PreSmoothing:
    // Always increment the counter, and change phase if we pass
    // the number of steps we specified
    {
      if (
        ++_preSmoothingStepsCounter > PreSmoothingSteps
       ) {
        if ( _activeLevel == CoarsestLevel )
          // On the coarsest level. Move to PostSmoothing
          _smoothingState = SmoothingState::PostSmoothing;
        else
          // Not yet on coarsest level. Restrict.
          _smoothingState = SmoothingState::RestrictToNextLevel;

        // reset the counter
        _preSmoothingStepsCounter = 0;
      }

    } break;

    case SmoothingState::RestrictToNextLevel:
    /*
    Finished restriction. Time to move up one level
    */
    {
      // Always decrement activeLevel. If we are on coarsest level
      // then we move straight on to post-smoothing
      if ( --_activeLevel == CoarsestLevel )
        _smoothingState = SmoothingState::PostSmoothing;

      else
        _smoothingState = SmoothingState::PreSmoothing;
    } break;

    case SmoothingState::PostSmoothing:
    {
      // Always increment the counter, and change phase if we
      // pass the number of steps we specified
      if (
        ++_postSmoothingStepsCounter >= PostSmoothingSteps
      ) {
        if (
          _activeLevel == getFinestLevel()
        ) {
          _smoothingState = SmoothingState::PreSmoothing;

          // Finished a full v cycle
          incrementVCycleCounter();
        }

        /*
        Time to prolongate. First increment the active level, then when we touch a fine
        vertex, we have the coarse vertices in scope so we can grab contributions we need.
        */
        else {
          _activeLevel++;
          _smoothingState = SmoothingState::Prolongation;
        }

        // reset the counter for next time
        _postSmoothingStepsCounter = 0;
      }

    } break;

    case SmoothingState::Prolongation:
    {
      // already increased active level. Time to do post-smoothing
      _smoothingState = SmoothingState::PostSmoothing;
    } break;

    default:
      assertion(false);
  }
}

/*
We want perform Jacobi on the active level if we're in PreSmoothing or PostSmoothing.
*/
bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::updateResidual(int level) const {
  return 
    (_state == State::Solve)
    and
    (
      _smoothingState == SmoothingState::PreSmoothing
      or
      _smoothingState == SmoothingState::PostSmoothing
    )
    and
    (level == getActiveLevel());
}

/**
 * In this scenario, we can mirror the behaviour of updateResidual
 * 
 */
bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::updateSolution(int level) const {
  return 
    (_state == State::Solve)
    and
    (
      // make sure we don't update the solution on the final presmoothing step - we want to calculate
      // the residual that we restrict, but not use it!
      _smoothingState == SmoothingState::PreSmoothing and _preSmoothingStepsCounter < PreSmoothingSteps
      or
      _smoothingState == SmoothingState::PostSmoothing
    )
    and
    (level == getActiveLevel());
}

/**
 * We can again mirror updateResidual
 * 
 */
bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::reportSolutionUpdates(int level) const {

  return
    updateResidual(level)
    and
    level == getFinestLevel();
}


/**
* In the restriction phase, we do something on one above the active level and something
* on the active level. So this function returns true if we are in Restriction mode.
*
* The action set must query the level to get the specific behaviour requested.
*/
bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::restrictToNextLevel() const {
  return
    (_state == State::Solve)
    and
    (_smoothingState == SmoothingState::RestrictToNextLevel);
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::prolongate() const {
  return
    (_state == State::Solve)
    and
    (_smoothingState == SmoothingState::Prolongation)
    and
    (_activeLevel >= 1 + CoarsestLevel); // we only want prolongations from real vertices!
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
  _finestLevel = std::max( _finestLevel, level );
}

int {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getFinestLevel() const {
  return _finestLevel;
}


/**
 * finest level must be at least one larger than the coarsest level
 */
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::validateRefinementLevel() {
  if ( getFinestLevel() <= CoarsestLevel ) {
    logError("validateRefinementLevel()", "CoarsestLevel must be at least one smaller than finest level. CoarsestLevel=" << CoarsestLevel << " finestLevel=" << getFinestLevel());
    throw std::runtime_error("Invalid refinement level");
  }
  logInfo("validateRefinementLevel()", "CoarsestLevel=" << CoarsestLevel << " finestLevel =" << getFinestLevel());
}



std::string {{NAMESPACE | join("::")}}::{{CLASSNAME}}::toString(SmoothingState state) const {
  std::ostringstream output;
  switch (state){
    case SmoothingState::PreSmoothing:
    {
      output << "PreSmoothing" << _preSmoothingStepsCounter;
    } break;
    case SmoothingState::RestrictToNextLevel:
    {
      output << "RestrictToNextLevel";
    } break;
    case SmoothingState::PostSmoothing:
    {
      output << "PostSmoothing" << _postSmoothingStepsCounter;
    } break;
    case SmoothingState::Prolongation:
    {
      output << "Prolongation";
    } break;
    default:
    {} break;
  }

  return output.str();
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::State {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getState() const {
  return _state;
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::SmoothingState {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getSmoothingState() const {
  return _smoothingState;
}
