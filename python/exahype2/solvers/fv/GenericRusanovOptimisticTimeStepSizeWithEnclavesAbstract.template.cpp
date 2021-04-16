{% include "AbstractSolverAdaptiveTimeStepSize.template.cpp" %}
#include <algorithm>


#ifdef Parallel
#include <mpi.h>
#endif


#include "tarch/mpi/Rank.h"


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}():
  _NumberOfFiniteVolumesPerAxisPerPatch( {{NUMBER_OF_VOLUMES_PER_AXIS}} ),
  _timeStamp(0.0),
  _timeStepSize(0.0),
  _solverState(SolverState::GridConstruction),
  _maxH({{MAX_H}}),
  _minH({{MIN_H}}),
  _admissibleTimeStepSize(std::numeric_limits<double>::max()),
  _predictedTimeStepSize(0.0),
  _mergeEnclaveTaskOutcomeInSecondaryTraversal(true),
  _spawnEnclaveTaskInPrimaryTraversal(true),
  _dropOptimisticTaskInPrimaryTraversal(false),
  _mergeOptimisticTaskOutcomeInSecondaryTraversal(false),
  _spawnOptimisticTaskInSecondaryTraversal(false) {
  _previousAdmissibleTimeStepSize[0] = 0.0;
  _previousAdmissibleTimeStepSize[1] = 0.0;
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startGridConstructionStep() {
  assertion( _solverState == SolverState::GridConstruction );
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishGridConstructionStep() {
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startGridInitialisationStep() {
  assertion( _solverState == SolverState::GridConstruction );
  _solverState = SolverState::GridInitialisation;
  logDebug( "startGridInitialisationStep(...)", "new state is " << toString(_solverState) );
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishGridInitialisationStep() {
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startTimeStep(
  double globalMinTimeStamp,
  double globalMaxTimeStamp,
  double globalMinTimeStepSize,
  double globalMaxTimeStepSize
) {
  if (
    _solverState == SolverState::GridInitialisation
  ) {
    _solverState = SolverState::PrimaryAfterGridInitialisation;
  }
  else if (
    _solverState == SolverState::PrimaryAfterGridInitialisation
     or
    _solverState == SolverState::Primary
  ) {
    _solverState = SolverState::Secondary;
  }
  else {
    _solverState = SolverState::Primary;
  }

  logDebug( "startTimeStep(...)", "new state is " << toString(_solverState) );
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishTimeStep() {
  const double TimeStapSizeDamping    = 0.98;

  if ( _solverState == SolverState::Secondary ) {
    double localNextAdmissibleTimeStepSize = _admissibleTimeStepSize;
    double nextAdmissibleTimeStepSize      = 0;
    #ifdef Parallel
    MPI_Allreduce(&localNextAdmissibleTimeStepSize, &nextAdmissibleTimeStepSize, 1, MPI_DOUBLE, MPI_MIN, tarch::mpi::Rank::getInstance().getCommunicator() );
    #else
    nextAdmissibleTimeStepSize = localNextAdmissibleTimeStepSize;
    #endif
    _admissibleTimeStepSize             = std::numeric_limits<double>::max();

    _previousAdmissibleTimeStepSize[0] = _previousAdmissibleTimeStepSize[1];
    _previousAdmissibleTimeStepSize[1] = nextAdmissibleTimeStepSize;

    logDebug(
      "finishTimeStep()",
      "end of secondary sweep: admissible time step size=" << nextAdmissibleTimeStepSize
    );

    if ( tarch::la::equals(_timeStepSize,0.0) ) {
      _timeStamp                           = 0.0;
      _timeStepSize                        = TimeStapSizeDamping * nextAdmissibleTimeStepSize;
      _predictedTimeStepSize               = 0.0;

      _mergeEnclaveTaskOutcomeInSecondaryTraversal    = true;
      _spawnEnclaveTaskInPrimaryTraversal             = true;
      _dropOptimisticTaskInPrimaryTraversal           = false;
      _mergeOptimisticTaskOutcomeInSecondaryTraversal = false;
      _spawnOptimisticTaskInSecondaryTraversal        = false;

      logDebug(
        "finishTimeStep()",
        "initial time step size set to " << _timeStepSize
      );
    }
    else if (
      _predictedTimeStepSize >= nextAdmissibleTimeStepSize
      and
      _spawnOptimisticTaskInSecondaryTraversal
    ) {
      _timeStamp                          += _timeStepSize;
      _timeStepSize                        = nextAdmissibleTimeStepSize;
      _predictedTimeStepSize               = 0.0;

      _mergeEnclaveTaskOutcomeInSecondaryTraversal    = true;
      _spawnEnclaveTaskInPrimaryTraversal             = true;
      _dropOptimisticTaskInPrimaryTraversal           = true;
      _mergeOptimisticTaskOutcomeInSecondaryTraversal = false;
      _spawnOptimisticTaskInSecondaryTraversal        = false;

      logInfo(
        "finishTimeStep()",
        "guess has been too optimistic. Throw away optimistic predictions, reset time step size to " << _timeStepSize << ", and erase prediction"
      );
    }
    else if (
      _predictedTimeStepSize>0.0
      and
      not _spawnOptimisticTaskInSecondaryTraversal
    ) {
      _timeStamp                          += _timeStepSize;
      _timeStepSize                        = nextAdmissibleTimeStepSize;
      _predictedTimeStepSize               = nextAdmissibleTimeStepSize;

      _mergeEnclaveTaskOutcomeInSecondaryTraversal    = true;
      _spawnEnclaveTaskInPrimaryTraversal             = true;
      _dropOptimisticTaskInPrimaryTraversal           = false;
      _mergeOptimisticTaskOutcomeInSecondaryTraversal = false;
      _spawnOptimisticTaskInSecondaryTraversal        = true;

      logDebug(
        "finishTimeStep()",
        "start to spawn optimistic time steps after next primary traversal"
      );
    }
    else if (
      _predictedTimeStepSize>0.0
      and
      _spawnOptimisticTaskInSecondaryTraversal
    ) {
      _timeStamp                          += _timeStepSize;
      _timeStepSize                        = _predictedTimeStepSize;
      _predictedTimeStepSize               = nextAdmissibleTimeStepSize;

      _mergeEnclaveTaskOutcomeInSecondaryTraversal    = false;
      _spawnEnclaveTaskInPrimaryTraversal             = false;
      _dropOptimisticTaskInPrimaryTraversal           = false;
      _mergeOptimisticTaskOutcomeInSecondaryTraversal = true;
      _spawnOptimisticTaskInSecondaryTraversal        = true;

      logDebug(
        "finishTimeStep()",
        "optimistic time stepping reasonably conservative"
      );
    }
    else if ( tarch::la::equals(_predictedTimeStepSize,0.0) ) {
      _timeStamp                          += _timeStepSize;
      _timeStepSize                        = nextAdmissibleTimeStepSize;
      _predictedTimeStepSize               = nextAdmissibleTimeStepSize;

      _mergeEnclaveTaskOutcomeInSecondaryTraversal    = true;
      _spawnEnclaveTaskInPrimaryTraversal             = true;
      _dropOptimisticTaskInPrimaryTraversal           = false;
      _mergeOptimisticTaskOutcomeInSecondaryTraversal = false;
      _spawnOptimisticTaskInSecondaryTraversal        = false;

      logDebug(
        "finishTimeStep()",
        "have no valid predicted time step size yet (previous guess had been too optimistic)"
      );
    }
    else {
      assertion( _predictedTimeStepSize <= nextAdmissibleTimeStepSize );
      assertion( _predictedTimeStepSize > 0.0 );

      _timeStamp                          += _timeStepSize;
      _timeStepSize                        = _predictedTimeStepSize;
      _predictedTimeStepSize               = nextAdmissibleTimeStepSize;

      _mergeEnclaveTaskOutcomeInSecondaryTraversal    = false;
      _spawnEnclaveTaskInPrimaryTraversal             = false;
      _dropOptimisticTaskInPrimaryTraversal           = false;
      _mergeOptimisticTaskOutcomeInSecondaryTraversal = false;
      _spawnOptimisticTaskInSecondaryTraversal        = true;

      logDebug(
        "finishTimeStep()",
        "predicted time step size "<< _predictedTimeStepSize << " is admissible (max admissible time step size would be " <<
        nextAdmissibleTimeStepSize << "), so roll over predicted time step size"
      );
    }
  }
  // end of primary time step:
  //
  // In the secondary step, we predict. But we do not change any state
  // (even though that would be possible - but I want to keep things
  // simple and straight)!
  else {
    if ( tarch::la::equals(_timeStepSize, 0.0)  ) {
      logInfo(
        "finishTimeStep()",
        "end of initial primary sweep"
      );
    }
    else if ( tarch::la::equals(_previousAdmissibleTimeStepSize[0], 0.0)  ) {
      _predictedTimeStepSize = TimeStapSizeDamping * _timeStepSize;
      logInfo(
        "finishTimeStep()",
        "finish primary sweep and predict time step size of " << _predictedTimeStepSize
        << " due to artificial damping"
      );
    }
    else {
      // nextAdmissibleTimeStepSize is stored now within _predictedTimeStepSize as of last
      // termination of secondary sweep
      double growthOfAdmissibleTimeStepSize = std::min(1.0, _previousAdmissibleTimeStepSize[1] / _previousAdmissibleTimeStepSize[0]);

      if ( tarch::la::equals(growthOfAdmissibleTimeStepSize,1.0) ) {
        _predictedTimeStepSize = 0.5 * (_timeStepSize + _predictedTimeStepSize);
        logDebug(
            "finishTimeStep()",
            "end of primary sweep: pick creeping average as new predicted time step size for optimistic tasking (predicted dt=" <<
            _predictedTimeStepSize << ", current dt=" <<
            _timeStepSize << 
            ")"
        );
      }
      else {
        const double biasedFutureTimeStepSize = growthOfAdmissibleTimeStepSize * _predictedTimeStepSize;
        _predictedTimeStepSize = std::min( biasedFutureTimeStepSize, 0.5 * (_timeStepSize + biasedFutureTimeStepSize) );
        logDebug(
            "finishTimeStep()",
            "end of primary sweep: pick min of biased time step size and biased average (new predicted dt=" <<
            _predictedTimeStepSize << ", current dt=" <<
            _timeStepSize << ", biased dt=" << biasedFutureTimeStepSize <<
            ", extrapolated growth=" << growthOfAdmissibleTimeStepSize << 
            ", admissible dt[0]=" << _previousAdmissibleTimeStepSize[0] << ", admissible dt[1]=" << _previousAdmissibleTimeStepSize[1] << ")"
        );
      }
    }
  }

  logDebug( "finishTimeStep()",
    "spawnEnclaveTaskInPrimaryTraversal=" << _spawnEnclaveTaskInPrimaryTraversal <<
    ",mergeEnclaveTaskOutcomeInSecondaryTraversal=" << _mergeEnclaveTaskOutcomeInSecondaryTraversal <<
    ",spawnOptimisticTaskInSecondaryTraversal=" << _spawnOptimisticTaskInSecondaryTraversal <<
    ",mergeOptimisticTaskOutcomeInSecondaryTraversal=" << _mergeOptimisticTaskOutcomeInSecondaryTraversal <<
    ",dropOptimisticTaskInPrimaryTraversal=" << _dropOptimisticTaskInPrimaryTraversal
  );

  assertion( _timeStepSize < std::numeric_limits<double>::max() );
  assertion( _timeStepSize >=0.0 );
}


double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getPredictedTimeStepSize() const {
  return _predictedTimeStepSize;
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startPlottingStep(
  double globalMinTimeStamp,
  double globalMaxTimeStamp,
  double globalMinTimeStepSize,
  double globalMaxTimeStepSize
) {
  if ( _solverState == SolverState::GridInitialisation ) {
    _solverState = SolverState::PlottingInitialCondition;
  }
  else {
    _solverState = SolverState::Plotting;
  }
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishPlottingStep() {
}


std::string {{NAMESPACE | join("::")}}::{{CLASSNAME}}::toString(SolverState state) {
  switch (state) {
    case SolverState::GridConstruction:
      return "grid-construction";
    case SolverState::GridInitialisation:
      return "grid-initialisation";
    case SolverState::Primary:
      return "primary";
    case SolverState::Secondary:
      return "secondary";
    case SolverState::PlottingInitialCondition:
      return "plotting-initial-condition";
    case SolverState::PrimaryAfterGridInitialisation:
      return "primary-after-grid-initialisation";
    case SolverState::Plotting:
      return "plotting";
  }
  return "<undef>";
}


{% if EIGENVALUES_IMPLEMENTATION!="<user-defined>" %}
double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::maxEigenvalue(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {
  {{EIGENVALUES_IMPLEMENTATION}}
}
{% endif %}


{% if FLUX_IMPLEMENTATION!="<none>" %}
{% if FLUX_IMPLEMENTATION!="<user-defined>" %}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::flux(
 const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
 const tarch::la::Vector<Dimensions,double>&  faceCentre,
 const tarch::la::Vector<Dimensions,double>&  volumeH,
 double                                       t,
 int                                          normal,
 double * __restrict__ F // F[{{NUMBER_OF_UNKNOWNS}}]
) {
  {{FLUX_IMPLEMENTATION}}
}
{% endif %}
{% endif %}


{% if NCP_IMPLEMENTATION!="<none>" %}
{% if NCP_IMPLEMENTATION!="<user-defined>" %}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::nonconservativeProduct(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const double * __restrict__             deltaQ, // [{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal,
  double * __restrict__ BgradQ // BgradQ[{{NUMBER_OF_UNKNOWNS}}]
) {
  {{NCP_IMPLEMENTATION}}
}
{% endif %}
{% endif %}




{% if SOURCE_TERM_IMPLEMENTATION!="<user-defined>" %}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::sourceTerm(
  const double * __restrict__                  Q,
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  double * __restrict__                        S
) {
  {% if SOURCE_TERM_IMPLEMENTATION!="<empty>" %}
  {{SOURCE_TERM_IMPLEMENTATION}}
  {% else %}
  std::fill_n(S,{{NUMBER_OF_UNKNOWNS}},0.0);
  {% endif %}
}
{% endif %}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::mergeEnclaveTaskOutcomeInSecondaryTraversal() const {
  return _mergeEnclaveTaskOutcomeInSecondaryTraversal;
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::spawnEnclaveTaskInPrimaryTraversal() const {
  return _spawnEnclaveTaskInPrimaryTraversal;
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::dropOptimisticTaskInPrimaryTraversal() const {
  assertion( not _dropOptimisticTaskInPrimaryTraversal or _spawnEnclaveTaskInPrimaryTraversal );
  return _dropOptimisticTaskInPrimaryTraversal;
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::mergeOptimisticTaskOutcomeInSecondaryTraversal() const {
  assertion( not _mergeOptimisticTaskOutcomeInSecondaryTraversal or _spawnOptimisticTaskInSecondaryTraversal);
  return _mergeOptimisticTaskOutcomeInSecondaryTraversal;
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::spawnOptimisticTaskInSecondaryTraversal() const {
  return _spawnOptimisticTaskInSecondaryTraversal;
}
