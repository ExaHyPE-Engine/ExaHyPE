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
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::beginMeshSweep() {
  if ( 
    _activeLevel == getFinestLevel()
    and
    /*
    we need to make sure we've updated the solution. But that doesn't
    happen until we've done at least one sweep at the finest level. So
    as a first try, let's check that we've done at least one sweep at
    the finest level (of either pre or post smoothing)

    We also want to make sure we clear out before restricting, or we
    risk double-accumulating...
    */
    (
      _postSmoothingStepsCounter >= 1
      or
      _preSmoothingStepsCounter  >= 1
      or
      _smoothingState == SmoothingState::RestrictToNextLevel
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

/**
 * @todo full accounting of the decisions in here.
 * 
 * We run this function once at the end of a sweep. This is where the control flow is modified.
 * 
 * Of note is the asymmetry in modifying the active level. When we restrict we want the finer level to be in scope
 * so that we can restrict its data to the coarser level. Hence we move up one level after(!) restriction is 
 * finished.
 * 
 * When we prolongate we need the coarser level to be in scope so we can prolongate data from them, and we need
 * the next level finer to be in scope as well. Hence we move down one level before(!) prolongation begins.
 * 
 * The control flow is modified in the switch statement in the body of this function. We ask the solver what state
 * it was in during last sweep, and then make decisions based on that.
 * 
 * If we just finished presmoothing:
 *  - We never do presmoothing on the coarsest level
 *  - We never modify the active level during presmoothing
 *  - We increment the counter, then check if we have exceeded the number of pre-determined steps we wanted.
 *    - If not exceeded, do nothing. Solver remains in presmoothing on the same level
 *    - If so, it's time to restrict. Set the solver to restrction on this level and move on.
 * 
 * If we just finished restriction:
 *  - Decrement the active level (remember coarser level is smaller number). We don't Decrement beforehand since we need the fine vertices (that we restrict FROM)
 *      and the coarse vertices (that we restrict TO) to both be in scope at once. Now we've done that, we can move up.
 *  - If that puts us onto the coarsest level, just move on to postsmoothing.
 * 
 * If we just finished postsmoothing:
 *  - We increment the counter, then check if we have exceeded the number of pre-determined steps we wanted.
 *  - if not exceeded, do nothing. Solver remains in postsmoothing on the same level
 *  - If we have exceeded, and we are not on the finest level, we now need to prolongate.
 *      We first increment the active level, so that when we prolongate on the next sweep, we have everything we need in scope, i.e. the
 *        vertices on the coarser level (that we just performed postsmoothing on) that we want to prolongate from and the vertices on the finer level 
 *        that we want to prolongate into
 *  - If we have exceeded, and are already on the finest level, then now we move onto presmoothing again and increment the number
 *     of completed v-cycles.
 * 
 * If we have just finished prolongation:
 *  - Just switched to postsmoothing. The active level was correctly set after we finished post-smoothing.
 *   
 * 
 */
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
        ++_preSmoothingStepsCounter >= PreSmoothingSteps
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
        /*
        If we are on the finest level, we want to do one more cycle  
        */        
        if (
          _activeLevel == getFinestLevel()
          and
          _postSmoothingStepsCounter == PostSmoothingSteps
        )  {
          // DO NOTHING!! GIVE ME ANOTHER SWEEP
        }

        /*
        This is the actual behaviour we want!

        We ask for another sweep at the finest level (see the if statement above)
        so that the reported numbers match exactly the vesion that was written before (with
        updating solution at the end of the sweep, rather than the start of the following
        sweep). Once we are satisifed with the development, we should delete the above.
        */
        else if (
          _activeLevel == getFinestLevel()
          and
          _postSmoothingStepsCounter > PostSmoothingSteps
        ) {
          _smoothingState = SmoothingState::PreSmoothing;

          // continue with updating solution; set the counter to 1
          // so that we don't skip updating the solution from here onwards
          // (we skip solution updates on the first presmoothing/postsmoothing)
          // sweep to avoid 0 divison by diag
          _preSmoothingStepsCounter  = 1;
          
          // reset this for next time
          _postSmoothingStepsCounter = 0;

          incrementVCycleCounter();
        }

        /*
        Time to prolongate. First increment the active level, then when we touch a fine
        vertex, we have the coarse vertices in scope so we can grab contributions we need.
        */
        else {
          _activeLevel++;
          _smoothingState = SmoothingState::Prolongation;
          // reset this for next time
          _postSmoothingStepsCounter = 0;
        }
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
 * We want to update the solution if we are in preSmoothing or postSmoothihg, but
 * not on the first sweep of either, otherwise we will accidentally do division
 * by zero with the diag when we touch the cell. 
 * 
 * We do not update the solution during restriction anymore, since during the final
 * sweep we want to update the residual, but we don't want to use it to update the
 * solution. That will be handled when we reach the finest level again at the end
 * of the v-cycle
 * 
 * Ditto for prolongation - the final postSmoothing sweep needs to have its updates
 * applied. Note that we increment the active level before performing prolongation - 
 * see the documentation for endMeshSweep(). Hence we check that the refinement level
 * of the vertex is one less than the activeLevel.
 * 
 */
bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::updateSolution(int level) const {
  bool output = false;

  // are we in presmoothing?
  output |= (
    _smoothingState == SmoothingState::PreSmoothing
    and
    _preSmoothingStepsCounter >= 1
    and
    level == getActiveLevel()
  );

  // are we in postsmoothing?
  output |= (
    _smoothingState == SmoothingState::PostSmoothing
    and
    _postSmoothingStepsCounter >= 1
    and
    level == getActiveLevel()
  );

  // are we in prolongation?
  output |= (
    _smoothingState == SmoothingState::Prolongation
    and
    level == getActiveLevel() - 1
  );

  // Must return false if we are supposed to be dormant (when being used as correction smoother)
  output &= (_state == State::Solve);

  return output;
}

/**
 * Since we update the vertex solution a sweep later, we must do our reporting a sweep later as
 * well. That means we report when:
 * 
 *  - The active level matches the finest level
 *  - Or we are restricting right now, and we need to report solution updates from the
 *      finest level.
 * 
 * We don't need to report when we are prolongating when the active level is the finest.
 * This is because during prolongation, we finalise updates from the level above, then
 * update the solution one level lower. Besides, the residuals on the finest level will
 * be zero until we've moved back to postSmoothing.
 * 
 */
bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::reportSolutionUpdates(int level) const {

  return
    _state == State::Solve
    and
    level == getFinestLevel()
    and
    level == getActiveLevel()
    and
    (
      _smoothingState == SmoothingState::PreSmoothing
      or
      _smoothingState == SmoothingState::PostSmoothing
      or
      _smoothingState == SmoothingState::RestrictToNextLevel
    );
}


/**
 * We need to consolidate the residual and diag across parallel boundaries in a variety of cases
 * 
 * First if we are in pre or post smoothing, and we are on the active level
 * 
 * Ditto for restriction, so vertices can agree on a solution before restriction
 * 
 * Same for prolongation, if we are one above the active level, so we can properly
 * update solution before prolongation.
 * 
 * This logic is somewhat messy, but the point gets across
 * 
 * This function does not get used anywhere at present. We wrote this to ensure the code can be
 * parallelised, but unfortunately sharing information in the Peano mesh across multiple levels
 * made this impossible. The intention of this function is to be used in the vertex merge method.
 * When two vertices on a parallel boundary are sharing data with each other, they should do
 * naive += with their values for residual and diagonal values, but only if this function
 * returns true. 
 * 
 * This part works, i.e. vertices across parallel boundary correctly agree on a residual by
 * accumulating contributions from different trees correctly.
 * 
 */
bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::consolidateResidualAndDiag(int level) const {
  return
    _state == State::Solve
    and
    (
      (_smoothingState == SmoothingState::PreSmoothing) and (level == getActiveLevel())
      or
      (_smoothingState == SmoothingState::PostSmoothing) and (level == getActiveLevel())
      or
      (_smoothingState == SmoothingState::RestrictToNextLevel) and (level == getActiveLevel())
      or
      (_smoothingState == SmoothingState::Prolongation) and (level == getActiveLevel() - 1)

    );
}


/*
In the restriction phase, we do something on one above the active level and something
on the active level. So this function returns true if we are in Restriction mode.

The action set must query the level to get the specific behaviour requested.
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
