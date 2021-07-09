#include "AbstractSWE.h"
#include "Constants.h"

#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/multicore/Lock.h"

tarch::logging::Log                  examples::exahype2::swe::AbstractSWE::AbstractSWE::_log( "examples::exahype2::swe::AbstractSWE::AbstractSWE" );
tarch::multicore::BooleanSemaphore   examples::exahype2::swe::AbstractSWE::AbstractSWE::_sempahore;

std::bitset<Dimensions> examples::exahype2::swe::AbstractSWE::AbstractSWE::PeriodicBC = examples::exahype2::swe::PeriodicBC;


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
  return _timeStepSize;
}


double examples::exahype2::swe::AbstractSWE::getMaxTimeStepSize() const {
  return _timeStepSize;
}


examples::exahype2::swe::AbstractSWE::SolverState examples::exahype2::swe::AbstractSWE::getSolverState() const {
  return _solverState;
}



void examples::exahype2::swe::AbstractSWE::setMaximumEigenvalue( double value ) {
  if ( value>tarch::la::NUMERICAL_ZERO_DIFFERENCE ) {
    double newAdmissibleTimeStep = 0.9 * getMinMeshSize() / 40 / value;
    tarch::multicore::Lock lock(_sempahore);
    _admissibleTimeStepSize = std::min(_admissibleTimeStepSize, newAdmissibleTimeStep);
  }
}







void examples::exahype2::swe::AbstractSWE::initialCondition(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  bool                                         gridIsConstructred
) {
  
    static TopologyParser parser("topology/etopo10min120W60W60S0S.asc", "topology/dtopo_usgs100227.tt3");
    float z;
    float d;

    z = parser.sampletopology(volumeCentre(0), volumeCentre(1));
    d = parser.sampledisplacement(volumeCentre(0), volumeCentre(1), 1.0);
  
  //if(z < -200){z = -200;}
  
   if(z < 0){
      Q[0] = -z + d;
     //Q[0] = volumeCentre(0) > 0.3 && volumeCentre(0) < 0.4 ? 20-z : -z;
     //Q[0] = (volumeCentre(0)-0.5)*(volumeCentre(0)-0.5) + (volumeCentre(1)-0.5)*(volumeCentre(1)-0.5) < 0.1*0.1 ? 10-z : -z;
   }else{
     Q[0] = 0.0;
   }
   Q[1] = 0.0;
   Q[2] = 0.0;
   Q[3] = z; // bathymetry
  
}




void examples::exahype2::swe::AbstractSWE::boundaryConditions(
  const double * __restrict__                  Qinside,   // Qinside[3+1]
  double * __restrict__                        Qoutside,  // Qoutside[3+1]
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
  _NumberOfFiniteVolumesPerAxisPerPatch( 40 ),
  _timeStamp(0.0),
  _timeStepSize(0.0),
  _admissibleTimeStepSize(std::numeric_limits<double>::max()),
  _solverState(SolverState::GridConstruction),
  _maxH(0.1111111111111111),
  _minH(0.037037037037037035)
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
  _timeStamp += getMaxTimeStepSize();
  
  double newTimeStepSize = _admissibleTimeStepSize;
  #ifdef Parallel
  MPI_Allreduce(&newTimeStepSize, &_timeStepSize, 1, MPI_DOUBLE, MPI_MIN, tarch::mpi::Rank::getInstance().getCommunicator() );
  #else
  _timeStepSize = newTimeStepSize;
  #endif
  
  _admissibleTimeStepSize = std::numeric_limits<double>::max();
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



void examples::exahype2::swe::AbstractSWE::sourceTerm(
  const double * __restrict__                  Q,
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  double * __restrict__                        S
) {
  
  std::fill_n(S,3,0.0);
  
}

