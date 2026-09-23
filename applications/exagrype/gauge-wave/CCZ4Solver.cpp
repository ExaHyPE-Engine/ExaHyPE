// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CCZ4Solver.h"

namespace {
  /**
   * Writes the exact gauge wave for the line element -H dt^2 + H dx^2 + dy^2 + dz^2.
   * The metric derivative D_kij stores half the derivative of the conformal metric.
   */
  void gaugeWave(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const double                                 t,
    const int                                    numberOfUnknowns,
    double* const NOALIAS                        Q
  ) {
    using Shortcuts = applications::exahype2::exagrype::CCZ4Solver::Shortcuts;
    assertion(Q != nullptr);
    assertion(numberOfUnknowns >= Shortcuts::grad_phi + 3, numberOfUnknowns);
    assertion(std::isfinite(t) and std::isfinite(x[0]), t, x);

    constexpr double NumberOfPeaks = 2.0;
    constexpr double Amplitude     = 0.1;

    const double phase      = NumberOfPeaks * tarch::la::PI * (x[0] - t);
    const double h          = 1.0 - Amplitude * std::sin(phase);
    const double dxH        = -NumberOfPeaks * tarch::la::PI * Amplitude * std::cos(phase);
    const double phi        = std::pow(1.0 / h, 1.0 / 6.0);
    const double phiSquared = phi * phi;
    const double lapse      = std::sqrt(h);
    const double dxPhi      = -phi * dxH / (6.0 * h);
    const double curvatureX = 0.5 * dxH / lapse;
    const double traceK     = curvatureX / h;

    std::fill_n(Q, numberOfUnknowns, 0.0);

    Q[Shortcuts::gamma_tilde + 0]      = phiSquared * h;
    Q[Shortcuts::gamma_tilde + 3]      = phiSquared;
    Q[Shortcuts::gamma_tilde + 5]      = phiSquared;
    Q[Shortcuts::A_tilde + 0]          = phiSquared * (curvatureX - 1.0 / 3.0 * traceK * h);
    Q[Shortcuts::A_tilde + 3]          = phiSquared * (-1.0 / 3.0 * traceK);
    Q[Shortcuts::A_tilde + 5]          = phiSquared * (-1.0 / 3.0 * traceK);
    Q[Shortcuts::gamma_hat + 0]        = 2.0 * dxH / (3.0 * phiSquared * h * h);
    Q[Shortcuts::alpha]                = lapse;
    Q[Shortcuts::grad_alpha + 0]       = 0.5 * dxH / lapse;
    Q[Shortcuts::grad_gamma_tilde + 0] = phiSquared * dxH / 3.0;
    Q[Shortcuts::grad_gamma_tilde + 3] = phi * dxPhi;
    Q[Shortcuts::grad_gamma_tilde + 5] = phi * dxPhi;
    Q[Shortcuts::K]                    = traceK;
    Q[Shortcuts::phi]                  = phi;
    Q[Shortcuts::grad_phi + 0]         = dxPhi;
  }
} // namespace

void applications::exahype2::exagrype::CCZ4Solver::initialCondition(
  double* const NOALIAS                        Q,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h
) {
  (void)h;

  gaugeWave(x, 0.0, NumberOfUnknowns + NumberOfAuxiliaryVariables, Q);
  for (int i = 0; i < NumberOfUnknowns; i++) {
    assertion(std::isfinite(Q[i]), x, i);
  }
}

void applications::exahype2::exagrype::CCZ4Solver::boundaryConditions(
  const double* const NOALIAS                                   Qinside,
  double* const NOALIAS                                         Qoutside,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>&                  h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const int                                    normal
) {
  (void)h;

  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    assertion(std::isfinite(Qinside[i]), x, t, normal, i);
    Qoutside[i] = Qinside[i];
  }
}

::exahype2::RefinementCommand applications::exahype2::exagrype::CCZ4Solver::refinementCriterion(
  const double* const NOALIAS                  Q,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h,
  const double                                 t
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
  const double* const                          Q,
  double* const                                solution
) {
  (void)dt;
  (void)Q;

  gaugeWave(x, t, NumberOfUnknowns, solution);
}
