#include "ElasticWaveSolver.h"

tarch::logging::Log applications::exahype2::elasticwave::ElasticWaveSolver::_log(
  "applications::exahype2::elasticwave::ElasticWaveSolver"
);

double applications::exahype2::elasticwave::ElasticWaveSolver::maxEigenvalue(
  const double* __restrict__ Q,
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  double                                       t,
  double                                       dt,
  int                                          normal
) {
  return maxEigenvalue(Q, x, h, t, dt, normal, Offloadable::Yes);
}

void applications::exahype2::elasticwave::ElasticWaveSolver::flux(
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
