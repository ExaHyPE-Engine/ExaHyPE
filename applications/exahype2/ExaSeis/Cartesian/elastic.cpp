#include "elastic.h"
#include "exahype2/RefinementControl.h"

tarch::logging::Log   applications::exahype2::elastic::elastic::_log( "applications::exahype2::elastic::elastic" );


/*
 * Enables the usage of shortcuts to access variables, e.g. use Q[s.v] instead of Q[0]
 */
applications::exahype2::elastic::VariableShortcuts s;



void ::applications::exahype2::elastic::elastic::initPointSourceLocations(double sourceLocation[NumberOfPointSources][Dimensions]){

  sourceLocation[0][0] = 0.0;
  sourceLocation[0][1] = 0.0;
#if Dimensions == 3
  //HHS1:
  // sourceLocation[0][2] = 0.693;
  //LOH1:
  sourceLocation[0][2] = 2.0;
#endif

}

void ::applications::exahype2::elastic::elastic::pointSource(
  const double* const Q, // Q[9+3]
  const double* const x, 
  const double t, 
  const double dt, 
  double* const forceVector, // Q[9
  int n) {

  for (int i = 0; i < s.NumberOfUnknowns; i++) {
    forceVector[i] = 0.0;
  }

  constexpr double t0 = 0.1;
  constexpr double M0 = 10.;//1000.0;
  double           f  = M0 * t / (t0 * t0) * std::exp(-t / t0);

  forceVector[s.sigma+3] = f; //sigma_xy
}

void ::applications::exahype2::elastic::elastic::pointSource(
  const float* const Q, // Q[9+3]
  const double* const x, 
  const double t, 
  const double dt, 
  float* const forceVector, // Q[9
  int n) {

  for (int i = 0; i < s.NumberOfUnknowns; i++) {
    forceVector[i] = 0.0;
  }

  constexpr float t0 = 0.1;
  constexpr float M0 = 10.;//1000.0;
  double           f  = M0 * t / (t0 * t0) * std::exp(-t / t0);

  forceVector[s.sigma+3] = f; //sigma_xy
}