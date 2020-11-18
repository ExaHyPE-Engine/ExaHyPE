#include "AbstractAcoustics.h"


tarch::logging::Log   examples::exahype2::acoustics::AbstractAcoustics::AbstractAcoustics::_log( "examples::exahype2::acoustics::AbstractAcoustics::AbstractAcoustics" );


double examples::exahype2::acoustics::AbstractAcoustics::getMinMeshSize() const {
  return _minH;
}


double examples::exahype2::acoustics::AbstractAcoustics::getMaxMeshSize() const {
  return _maxH;
}


double examples::exahype2::acoustics::AbstractAcoustics::getMinTimeStamp() const {
  return _timeStamp;
}


double examples::exahype2::acoustics::AbstractAcoustics::getMaxTimeStamp() const {
  return _timeStamp;
}


double examples::exahype2::acoustics::AbstractAcoustics::getMinTimeStepSize() const {
  return 0.0001;
}


double examples::exahype2::acoustics::AbstractAcoustics::getMaxTimeStepSize() const {
  return 0.0001;
}


examples::exahype2::acoustics::AbstractAcoustics::SolverState examples::exahype2::acoustics::AbstractAcoustics::getSolverState() const {
  return _solverState;
}







void examples::exahype2::acoustics::AbstractAcoustics::adjustSolution(
  double * __restrict__ Q, // Q[2+2],
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t
) {
  if (tarch::la::equals(t,0.0) ) {
    
   Q[0] = (volumeCentre(0)-0.5)*(volumeCentre(0)-0.5) + (volumeCentre(1)-0.5)*(volumeCentre(1)-0.5) < 0.1*0.1 ? 0.2 : 0.1;
   Q[1] = 0.1;
   Q[2] = 0.1; // first aux variable
   Q[3] = 0.1; // first aux variable
  
  }
}





void examples::exahype2::acoustics::AbstractAcoustics::boundaryConditions(
  double * __restrict__ Qinside, // Qinside[2+2]
  double * __restrict__ Qoutside, // Qoutside[2+2]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {
  Qoutside[0] = Qinside[0];
Qoutside[1] = Qinside[1];
Qoutside[2] = Qinside[2];
Qoutside[3] = Qinside[3];

}





examples::exahype2::acoustics::AbstractAcoustics::AbstractAcoustics():
  _NumberOfFiniteVolumesPerAxisPerPatch( 11 ),
  _timeStamp(0.0),
  _solverState(SolverState::GridConstruction),
  _maxH(0.1),
  _minH(0.1)
  {
}


void examples::exahype2::acoustics::AbstractAcoustics::startGridConstructionStep() {
}


void examples::exahype2::acoustics::AbstractAcoustics::finishGridConstructionStep() {
}


void examples::exahype2::acoustics::AbstractAcoustics::startGridInitialisationStep() {
  _solverState = SolverState::GridInitialisation;
}


void examples::exahype2::acoustics::AbstractAcoustics::finishGridInitialisationStep() {
}



void examples::exahype2::acoustics::AbstractAcoustics::startTimeStep(
  double globalMinTimeStamp,
  double globalMaxTimeStamp,
  double globalMinTimeStepSize,
  double globalMaxTimeStepSize
) {
  _solverState = SolverState::TimeStep;
}


void examples::exahype2::acoustics::AbstractAcoustics::finishTimeStep() {
  _timeStamp += 0.0001;
}


void examples::exahype2::acoustics::AbstractAcoustics::startPlottingStep(
  double globalMinTimeStamp,
  double globalMaxTimeStamp,
  double globalMinTimeStepSize,
  double globalMaxTimeStepSize
) {
  _solverState = SolverState::Plotting;
}


void examples::exahype2::acoustics::AbstractAcoustics::finishPlottingStep() {
}


std::string examples::exahype2::acoustics::AbstractAcoustics::toString(SolverState state) {
  switch (state) {
    case SolverState::GridConstruction:
      return "grid-construction";
    case SolverState::GridInitialisation:
      return "grid-initialisation";
    case SolverState::TimeStep:
      return "time-step";
    case SolverState::Plotting:
      return "plotting";
  }
  return "<undef>";
}
