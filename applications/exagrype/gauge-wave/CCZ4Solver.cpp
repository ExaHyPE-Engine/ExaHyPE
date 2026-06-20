// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CCZ4Solver.h"

#include "InitialCondition.h"

#include <cmath>

void applications::exahype2::exagrype::CCZ4Solver::initialCondition(
  double* __restrict__ Q,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h
) {
  (void)h;
  ::applications::exahype2::exagrype::initialCondition(x, 0.0, NumberOfUnknowns + NumberOfAuxiliaryVariables, Q);
  for (int i = 0; i < NumberOfUnknowns; i++) {
    assertion(std::isfinite(Q[i]), x, i);
  }
}

void applications::exahype2::exagrype::CCZ4Solver::boundaryConditions(
  const double* __restrict__ Qinside,
  double* __restrict__ Qoutside,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h,
  double                                       t,
  int                                          normal
) {
  (void)h;
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    assertion(Qinside[i] == Qinside[i], x, t, normal, i);
    Qoutside[i] = Qinside[i];
  }
}

::exahype2::RefinementCommand applications::exahype2::exagrype::CCZ4Solver::refinementCriterion(
  const double* __restrict__ Q,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h,
  double                                       t
) {
  (void)Q;
  (void)x;
  (void)h;
  (void)t;
  return ::exahype2::RefinementCommand::Keep;
}

void applications::exahype2::exagrype::CCZ4Solver::analyticalSolution(
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const double                                 t,
  const double                                 dt,
  const double* __restrict__ Q,
  double* __restrict__ solution
) {
  (void)dt;
  (void)Q;
  ::applications::exahype2::exagrype::initialCondition(x, t, NumberOfUnknowns, solution);
}
