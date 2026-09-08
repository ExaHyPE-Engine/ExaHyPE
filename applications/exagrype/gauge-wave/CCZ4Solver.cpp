// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CCZ4Solver.h"

#include "InitialCondition.h"

void applications::exahype2::exagrype::CCZ4Solver::initialCondition(
  double* NOALIAS                              Q,
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
  const double* NOALIAS                        Qinside,
  double* NOALIAS                              Qoutside,
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
  const double* NOALIAS                        Q,
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
  const double* NOALIAS                        Q,
  double* NOALIAS                              solution
) {
  (void)dt;
  (void)Q;
  ::applications::exahype2::exagrype::initialCondition(x, t, NumberOfUnknowns, solution);
}
