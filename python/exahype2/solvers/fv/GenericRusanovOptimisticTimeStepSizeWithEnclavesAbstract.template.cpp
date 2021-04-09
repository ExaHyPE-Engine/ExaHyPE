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
  _previousAdmissibleTimeStepSize = 0.0;
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

    if ( tarch::la::equals(_timeStepSize,0.0) ) {
      _timeStepSize                       = TimeStapSizeDamping * nextAdmissibleTimeStepSize;
      _predictedTimeStepSize              = 0.0;
      _previousAdmissibleTimeStepSize     = 0.0;
      _admissibleTimeStepSize             = std::numeric_limits<double>::max();
      logInfo(
        "finishTimeStep()",
        "end of secondary sweep: initial time step size is set to " << _timeStepSize << ". Do not yet issue tasks optimistically"
      );
      _mergeEnclaveTaskOutcomeInSecondaryTraversal    = true;
      _spawnEnclaveTaskInPrimaryTraversal      = true;
      _dropOptimisticTaskInPrimaryTraversal           = false;
      _mergeOptimisticTaskOutcomeInSecondaryTraversal = false;
      _spawnOptimisticTaskInSecondaryTraversal        = false;
    }
    /**
     *  ok, we can continue
     */
    else if (
      _predictedTimeStepSize > nextAdmissibleTimeStepSize
      or
      _previousAdmissibleTimeStepSize > nextAdmissibleTimeStepSize
    ) {
      logInfo(
        "finishTimeStep()",
        "end of secondary sweep: guess has been too optimistic (predicted "<< _predictedTimeStepSize << " after previous one of " << _previousAdmissibleTimeStepSize <<
        "), admissible time step size is " << nextAdmissibleTimeStepSize << ". Have to throw away optimistic predictions"
      );
      _timeStamp                          += _timeStepSize;
      _timeStepSize                        = nextAdmissibleTimeStepSize;
      _predictedTimeStepSize               = 0.0;
      _previousAdmissibleTimeStepSize      = 0.0;
      _mergeEnclaveTaskOutcomeInSecondaryTraversal    = true;
      _spawnEnclaveTaskInPrimaryTraversal             = true;
      _dropOptimisticTaskInPrimaryTraversal           = true;
      _mergeOptimisticTaskOutcomeInSecondaryTraversal = false;
      _spawnOptimisticTaskInSecondaryTraversal        = false;
    }
    else if ( tarch::la::equals(_predictedTimeStepSize, 0.0) ) {
      assertion( _predictedTimeStepSize <= nextAdmissibleTimeStepSize );

      _timeStamp                          += _timeStepSize;
      _timeStepSize                        = nextAdmissibleTimeStepSize;
      _predictedTimeStepSize               = 0.0;
      _previousAdmissibleTimeStepSize      = 0.0;
      logInfo(
        "finishTimeStep()",
        "end of secondary sweep: no prediction of a time step size had been available. No time steps spawned optimistically. Continue with time step size "<< _timeStepSize
      );
      _mergeEnclaveTaskOutcomeInSecondaryTraversal    = true;
      _spawnEnclaveTaskInPrimaryTraversal      = true;
      _dropOptimisticTaskInPrimaryTraversal           = false;
      _mergeOptimisticTaskOutcomeInSecondaryTraversal = false;
      _spawnOptimisticTaskInSecondaryTraversal        = true;
    }
    else {
      assertion( _predictedTimeStepSize <= nextAdmissibleTimeStepSize );

      _timeStamp                          += _timeStepSize;
      _timeStepSize                        = _predictedTimeStepSize;
      _previousAdmissibleTimeStepSize      = _predictedTimeStepSize;
      _predictedTimeStepSize               = nextAdmissibleTimeStepSize;
      logInfo(
        "finishTimeStep()",
        "end of secondary sweep: predicted time step size "<< _predictedTimeStepSize << " is admissible (max admissible time step size would be " <<
        nextAdmissibleTimeStepSize << ", so continue with this time step size"
      );
      _mergeEnclaveTaskOutcomeInSecondaryTraversal    = false;
      _spawnEnclaveTaskInPrimaryTraversal             = false;
      _dropOptimisticTaskInPrimaryTraversal           = false;
      _mergeOptimisticTaskOutcomeInSecondaryTraversal = true;
      _spawnOptimisticTaskInSecondaryTraversal        = true;
    }
  }
  // end of primary time step:
  //
  // In the secondary step, we might issue optimistic computations. So this is
  // the place where we predict.
  else {
    if ( tarch::la::equals(_timeStepSize, 0.0)  ) {
      logInfo(
        "finishTimeStep()",
        "end of primary sweep: do not yet have a prediction what a valid time step size might look like. Did not spawn optimistic tasks"
      );
      _mergeOptimisticTaskOutcomeInSecondaryTraversal = false;
    }
    else {
      if ( tarch::la::equals(_previousAdmissibleTimeStepSize, 0.0)  ) {
        _predictedTimeStepSize = TimeStapSizeDamping * _timeStepSize;
      }
      else {
        // nextAdmissibleTimeStepSize is stored now within _predictedTimeStepSize as of last
        // termination of secondary sweep
        double growthOfAdmissibleTimeStepSize = std::min(1.0, _predictedTimeStepSize / _previousAdmissibleTimeStepSize);

        double backupOfPredictedTimeStepSize = _predictedTimeStepSize;

        if ( tarch::la::equals(growthOfAdmissibleTimeStepSize,1.0) ) {
          _predictedTimeStepSize = 0.5 * (_timeStepSize + _predictedTimeStepSize);
          logInfo(
            "finishTimeStep()",
            "end of primary sweep: pick creeping average new predicted time step size for optimistic tasking (predicted dt=" <<
            _predictedTimeStepSize << ", current dt=" <<
            _timeStepSize << ", admissible dt=" << backupOfPredictedTimeStepSize <<
            ")"
          );
        }
        else {
          _predictedTimeStepSize = 0.5 * growthOfAdmissibleTimeStepSize * growthOfAdmissibleTimeStepSize * (_timeStepSize + _predictedTimeStepSize);
          logInfo(
            "finishTimeStep()",
            "end of primary sweep: pick biased new predicted time step size for optimistic tasking (predicted dt=" <<
            _predictedTimeStepSize << ", current dt=" <<
            _timeStepSize << ", admissible dt=" << backupOfPredictedTimeStepSize <<
            ", extrapolated growth=" << growthOfAdmissibleTimeStepSize << ")"
          );
        }
      }
      _previousAdmissibleTimeStepSize          = _predictedTimeStepSize;
      _spawnOptimisticTaskInSecondaryTraversal = true;
    }
  }

  // @todo Debug
  logInfo( "finishTimeStep()",
    "mergeEnclaveTaskOutcomeInSecondaryTraversal=" << _mergeEnclaveTaskOutcomeInSecondaryTraversal <<
    ",spawnEnclaveTaskInPrimaryTraversal=" << _spawnEnclaveTaskInPrimaryTraversal <<
    ",dropOptimisticTaskInPrimaryTraversal=" << _dropOptimisticTaskInPrimaryTraversal <<
    ",mergeOptimisticTaskOutcomeInSecondaryTraversal=" << _mergeOptimisticTaskOutcomeInSecondaryTraversal <<
    ",spawnOptimisticTaskInSecondaryTraversal=" << _spawnOptimisticTaskInSecondaryTraversal
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
