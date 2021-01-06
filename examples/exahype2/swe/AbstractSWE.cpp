#include "AbstractSWE.h"


tarch::logging::Log   examples::exahype2::swe::AbstractSWE::AbstractSWE::_log( "examples::exahype2::swe::AbstractSWE::AbstractSWE" );


double examples::exahype2::swe::AbstractSWE::getMinMeshSize() const {
  return _minH;
}


double examples::exahype2::swe::AbstractSWE::getMaxMeshSize() const {
  return _maxH;
}


double examples::exahype2::swe::AbstractSWE::getMinTimeStamp() const {
  return _timeStamp;
}


double examples::exahype2::swe::AbstractSWE::getMaxTimeStamp() const {
  return _timeStamp;
}


double examples::exahype2::swe::AbstractSWE::getMinTimeStepSize() const {
  return 0.0001;
}


double examples::exahype2::swe::AbstractSWE::getMaxTimeStepSize() const {
  return 0.0001;
}


examples::exahype2::swe::AbstractSWE::SolverState examples::exahype2::swe::AbstractSWE::getSolverState() const {
  return _solverState;
}




::exahype2::RefinementCommand examples::exahype2::swe::AbstractSWE::refinementCriterion(
  const double * __restrict__ Q, // Q[3+1],
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t
) {
  
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;

  if ( tarch::la::smallerEquals(_maxH,_NumberOfFiniteVolumesPerAxisPerPatch*tarch::la::max(volumeH)) ) {
    result = ::exahype2::RefinementCommand::Refine;
  }

  return result;
  
}




void examples::exahype2::swe::AbstractSWE::adjustSolution(
  double * __restrict__ Q,
  const double * __restrict__ Q, // Q[3+1],
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t
) {
  if (tarch::la::equals(t,0.0) ) {
    
   Q[0] = (volumeCentre(0)-0.5)*(volumeCentre(0)-0.5) + (volumeCentre(1)-0.5)*(volumeCentre(1)-0.5) < 0.1*0.1 ? 0.2 : 0.1;
   Q[1] = 0.0;
   Q[2] = 0.0;
   Q[3] = 1.0; // bathymetry
  
  }
}





void examples::exahype2::swe::AbstractSWE::boundaryConditions(
  const double * __restrict__ Qinside, // Qinside[3+1]
  const double * __restrict__ Qoutside, // Qoutside[3+1]
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





examples::exahype2::swe::AbstractSWE::AbstractSWE():
  _NumberOfFiniteVolumesPerAxisPerPatch( 11 ),
  _timeStamp(0.0),
  _solverState(SolverState::GridConstruction),
  _maxH(0.1),
  _minH(0.1)
  {
}


void examples::exahype2::swe::AbstractSWE::startGridConstructionStep() {
}


void examples::exahype2::swe::AbstractSWE::finishGridConstructionStep() {
}


void examples::exahype2::swe::AbstractSWE::startGridInitialisationStep() {
  _solverState = SolverState::GridInitialisation;
}


void examples::exahype2::swe::AbstractSWE::finishGridInitialisationStep() {
}



void examples::exahype2::swe::AbstractSWE::startTimeStep(
  double globalMinTimeStamp,
  double globalMaxTimeStamp,
  double globalMinTimeStepSize,
  double globalMaxTimeStepSize
) {
  _solverState = SolverState::TimeStep;
}


void examples::exahype2::swe::AbstractSWE::finishTimeStep() {
  _timeStamp += 0.0001;
}


void examples::exahype2::swe::AbstractSWE::startPlottingStep(
  double globalMinTimeStamp,
  double globalMaxTimeStamp,
  double globalMinTimeStepSize,
  double globalMaxTimeStepSize
) {
  _solverState = SolverState::Plotting;
}


void examples::exahype2::swe::AbstractSWE::finishPlottingStep() {
}


std::string examples::exahype2::swe::AbstractSWE::toString(SolverState state) {
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
