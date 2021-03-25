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
  _previousAdmissibleTimeStepSize(0.0),
  _admissibleTimeStepSize(std::numeric_limits<double>::max()),
  _predictedTimeStepSize(-1.0) {
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
  if ( _solverState == SolverState::Secondary ) {
    double nextAdmissibleTimeStepSize      = 0;
    double localNextAdmissibleTimeStepSize = std::numeric_limits<double>::max();
    if ( tarch::la::greater(_admissibleTimeStepSize,0.0) ) {
      localNextAdmissibleTimeStepSize = std::min(localNextAdmissibleTimeStepSize, _admissibleTimeStepSize);
    }
    if ( tarch::la::greater(_admissibleTimeStepSizeAfterPrimaryGridSweep,0.0) ) {
      localNextAdmissibleTimeStepSize = std::min(localNextAdmissibleTimeStepSize, _admissibleTimeStepSizeAfterPrimaryGridSweep);
    }
    #ifdef Parallel
    MPI_Allreduce(&localNextAdmissibleTimeStepSize, &nextAdmissibleTimeStepSize, 1, MPI_DOUBLE, MPI_MIN, tarch::mpi::Rank::getInstance().getCommunicator() );
    #else
    nextAdmissibleTimeStepSize = localNextAdmissibleTimeStepSize;
    #endif

    if ( tarch::la::equals(_timeStepSize,0.0) ) {
      const double TimeStapSizeDamping = 0.98;
      _timeStepSize                   = TimeStapSizeDamping * nextAdmissibleTimeStepSize;
      _predictedTimeStepSize          = -1.0;
      _previousAdmissibleTimeStepSize = nextAdmissibleTimeStepSize;
    }
    else if ( _predictedTimeStepSize <= nextAdmissibleTimeStepSize ) {
      _timeStamp    += _timeStepSize;

      double growthOfAdmissibleTimeStepSize = std::min( 1.0, nextAdmissibleTimeStepSize / _previousAdmissibleTimeStepSize );
      double newPredictedTimeStepSize = 0.5 * (_predictedTimeStepSize + growthOfAdmissibleTimeStepSize * nextAdmissibleTimeStepSize);

      _predictedTimeStepSize = newPredictedTimeStepSize;
    }
    else {
      _timeStamp             += _timeStepSize;
      _timeStepSize           = nextAdmissibleTimeStepSize;
      _predictedTimeStepSize  = -1.0;
    }
  }
  else if ( _predictedTimeStepSize<=0.0 ) {
    double growthOfAdmissibleTimeStepSize = std::min( 1.0, _timeStepSize / _previousAdmissibleTimeStepSize );
    _predictedTimeStepSize = growthOfAdmissibleTimeStepSize * _timeStepSize;
  }
  else if ( _predictedTimeStepSize>0.0 ) {
    _timeStepSize  = _predictedTimeStepSize;
  }


  if ( _solverState == SolverState::Secondary and _predictedTimeStepSize>0.0 ) {
    logInfo(
      "finishTimeStep()",
      "continue with primary tree sweep at time stamp " << _timeStamp <<
      " and issue skeleton tasks with time step size " << _timeStepSize <<
      ". Some tasks have been optimistically brought forward with " << _predictedTimeStepSize
    );
  }
  else if ( _solverState == SolverState::Secondary ) {
    logInfo(
      "finishTimeStep()",
      "continue with primary tree sweep at time stamp " << _timeStamp <<
      " and issue both skeleton and enclave tasks with time step size " << _timeStepSize
    );
  }
  else if ( _predictedTimeStepSize>0.0 ) {
    logInfo(
      "finishTimeStep()",
      "continue with secondary tree sweep at time stamp " << _timeStamp <<
      ". Enclave tasks finish with time step size " << _timeStepSize <<
      " and new enclave tasks are triggered optimistically with " << _predictedTimeStepSize
    );
  }
  else {
    logInfo(
      "finishTimeStep()",
      "continue with secondary tree sweep at time stamp " << _timeStamp <<
      ". Enclave tasks finish with time step size " << _timeStepSize <<
      ". No new enclave tasks are triggered optimistically. Furthermore, discard pre-computed tasks"
    );
  }

  assertion( _timeStepSize < std::numeric_limits<double>::max() );
  assertion( _timeStepSize >=0.0 );
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
