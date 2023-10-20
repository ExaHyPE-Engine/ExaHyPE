#include "ElasticWaveSolver.h"

void applications::exahype2::elasticwave::ElasticWaveSolver::initialCondition(
  double* __restrict__ Q,
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  const tarch::la::Vector<Dimensions, int>&    index,
  bool                                         gridIsConstructred
) {
  Q[0] = 0.0;
  Q[1] = 0.0;
  Q[2] = 0.0;
  Q[3] = 0.0;
  Q[4] = 0.0;
}

void applications::exahype2::elasticwave::ElasticWaveSolver::boundaryConditions(
  const double* __restrict__ Qinside,
  double* __restrict__ Qoutside,
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  double                                       t,
  int                                          normal
) {
  Qoutside[0] = 0.0;
  Qoutside[1] = 0.0;
  Qoutside[2] = 0.0;
  Qoutside[3] = 0.0;
  Qoutside[4] = 0.0;
}

void applications::exahype2::elasticwave::ElasticWaveSolver::algebraicSource(
  const tarch::la::Vector<Dimensions, double>& x, double t, const double* __restrict__ const Q, double* __restrict__ S
  /*void applications::exahype2::elasticwave::ElasticWaveSolver::sourceTerm(
    const double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& x,
    double                                       t,
    double                                       dt,
    double* __restrict__ S*/
) {
  S[0] = 0.0;
  S[1] = 0.0;
  S[2] = 0.0;
  S[3] = 0.0;
  S[4] = 0.0;

  const bool pointSource = tarch::la::norm2(x - tarch::la::Vector<Dimensions, double>({10, 10})) <= MaxAdmissibleCellH;
  if (pointSource) {
    constexpr double t0    = 0.1;
    constexpr double M0    = 1000.0;
    const double     force = M0 * t / (t0 * t0) * std::exp(-t / t0);
    S[4]                   = force;
  }
}

::exahype2::RefinementCommand applications::exahype2::elasticwave::ElasticWaveSolver::refinementCriterion(
  const double* __restrict__ Q,
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  double                                       t
) {
  auto result = ::exahype2::RefinementCommand::Keep;

  if (tarch::la::equals(t, 0.0)) {
    const bool pointSource = tarch::la::norm2(x - tarch::la::Vector<Dimensions, double>({10, 10})) <= MaxAdmissibleCellH;
    if (pointSource) {
      result = ::exahype2::RefinementCommand::Refine;
    }
  }

  if (tarch::la::greater(t, 0.0)) {
    const double stress = Q[4];
    if (stress >= 5) {
      result = ::exahype2::RefinementCommand::Refine;
    }
    if (stress < 5) {
      result = ::exahype2::RefinementCommand::Erase;
    }
  }

  return result;
}
