// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FVSolver.h"

double tests::exahype2::fortran::FVSolver::maxEigenvalue(
  const double* const NOALIAS                  Q,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h,
  const double                                 t,
  const double                                 dt,
  const int                                    normal
) {
  static constexpr double g = 9.81;
  const double            u = Q[0] > 1e-5 ? Q[1 + normal] / Q[0] : 0.0;
  const double            c = Q[0] > 1e-5 ? std::sqrt(g * Q[0]) : 0.0;
  return std::max(std::abs(u - c), std::abs(u + c));
}

extern "C" {
double rpn2_augmented_wrapper(
  const double* NOALIAS ql,
  const double* NOALIAS qr,
  double* NOALIAS       amdq,
  double* NOALIAS       apdq,
  const int*            normal
);
}

double tests::exahype2::fortran::FVSolver::solveRiemannProblem(
  const double* const NOALIAS                  QL,
  const double* const NOALIAS                  QR,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h,
  const double                                 t,
  const double                                 dt,
  const int                                    normal,
  double* const NOALIAS                        FL,
  double* const NOALIAS                        FR
) {
  // The Fortran solver returns the Clawpack fluctuations: the left-going
  // fluctuation A^-dQ into FL and the right-going fluctuation A^+dQ into FR (for
  // h, hu, hv). The Godunov kernel updates the left cell by -dt/h * FL and the
  // right cell by +dt/h * FR, whereas wave propagation requires the right cell
  // to receive -dt/h * A^+dQ, so the right-going fluctuation is negated.
  const double smax = rpn2_augmented_wrapper(QL, QR, FL, FR, &normal);

  FR[0] = -FR[0];
  FR[1] = -FR[1];
  FR[2] = -FR[2];
  FL[3] = 0.0;
  FR[3] = 0.0;

  return smax;
}
