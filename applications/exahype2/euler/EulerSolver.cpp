#include "EulerSolver.h"

tarch::logging::Log applications::exahype2::euler::EulerSolver::_log("applications::exahype2::euler::EulerSolver");

double applications::exahype2::euler::EulerSolver::maxEigenvalue(
  const double* __restrict__ Q,
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  double                                       t,
  double                                       dt,
  int                                          normal
) {
  return maxEigenvalue(Q, x, h, t, dt, normal, Offloadable::Yes);
}

void applications::exahype2::euler::EulerSolver::flux(
  const double* __restrict__ Q,
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double* __restrict__ F
) {
  flux(Q, x, h, t, dt, normal, F, Offloadable::Yes);
}
