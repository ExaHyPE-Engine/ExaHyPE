#include "EulerFVSmooth.h"
#include "exahype2/RefinementControl.h"
#include <cmath>

tarch::logging::Log benchmarks::exahype2::euler::smooth_setup::EulerFVSmooth::_log( "examples::exahype2::test::euler_sod::benchmark::euler_sod" );

void benchmarks::exahype2::euler::smooth_setup::EulerFVSmooth::initialCondition(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  bool                                         gridIsConstructed
) {
  logTraceInWith3Arguments( "initialCondition(...)", volumeCentre, volumeH, gridIsConstructed );
  // Initial data
  constexpr double gamma = 1.4;
  constexpr double p = 1.0;
  Q[0] = 1.0 + 0.2 * std::cos((2.0 * volumeCentre[0]) * 3.1415);
  Q[1] = 1.0;
  Q[2] = 0.0;
  Q[3] = p / (gamma - 1) + 0.5 / Q[0] * (Q[1] * Q[1]);
  logTraceOut( "initialCondition(...)" );
}

void benchmarks::exahype2::euler::smooth_setup::EulerFVSmooth::boundaryConditions(
  const double * __restrict__                  Qinside, // Qinside[4+0]
  double * __restrict__                        Qoutside, // Qoutside[4+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {
  logTraceInWith4Arguments( "boundaryConditions(...)", faceCentre, volumeH, t, normal );
  Qoutside[0] = Qinside[0];
  Qoutside[1] = Qinside[1];
  Qoutside[2] = Qinside[2];
  Qoutside[3] = Qinside[3];
  logTraceOut( "boundaryConditions(...)" );
}

void ::benchmarks::exahype2::euler::smooth_setup::EulerFVSmooth::maxEigenvalue(
  const double * __restrict__ Q, // Q[4+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double* __restrict__                         maxEigenvalue
)  {
  logTraceInWith4Arguments( "maxEigenvalue(...)", faceCentre, volumeH, t, normal );
  const double irho = 1.0 / Q[0];
  const double gamma = 1.4;
  const double p = (gamma - 1) * (Q[3] - 0.5 * irho * (Q[1] * Q[1] + Q[2] * Q[2]));
  const double c = std::sqrt(gamma * p * irho);

  double result = 1.0;
  switch (normal)
  {
  case 0:
    result = std::max(std::abs(Q[1] + c), std::abs(Q[1] - c));
    break;
  case 1:
    result = std::max(std::abs(Q[2] + c), std::abs(Q[2] - c));
    break;
  }
  logTraceOut("maxEigenvalue(...)");
  maxEigenvalue[0] = result;
}

void ::benchmarks::exahype2::euler::smooth_setup::EulerFVSmooth::flux(
  const double * __restrict__ Q, // Q[4+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__ F // F[4]
)  {
  logTraceInWith4Arguments( "flux(...)", faceCentre, volumeH, t, normal );
  const double irho = 1.0 / Q[0];
  const double gamma = 1.4;
  const double p =
      (gamma - 1) * (Q[3] - 0.5 * irho * (Q[1] * Q[1] + Q[2] * Q[2]));

  switch (normal)
  {
  case 0:
    F[0] = Q[1];
    F[1] = irho * Q[1] * Q[1] + p;
    F[2] = irho * Q[1] * Q[2];
    F[3] = irho * Q[1] * (Q[3] + p);
    break;

  case 1:
    F[0] = Q[2];
    F[1] = irho * Q[2] * Q[1];
    F[2] = irho * Q[2] * Q[2] + p;
    F[3] = irho * Q[2] * (Q[3] + p);
    break;
  }
  logTraceOut( "flux(...)" );
}