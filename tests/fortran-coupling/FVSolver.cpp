// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FVSolver.h"

double
tests::exahype2::fortran::FVSolver::maxEigenvalue(
  const double* const NOALIAS                  Q,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h,
  const double                                 t,
  const double                                 dt,
  const int                                    normal
) {
  static constexpr double Gravity    = 9.81;
  const double            velocity   = Q[0] > 1e-5 ? Q[1 + normal] / Q[0] : 0.0;
  const double            soundSpeed = Q[0] > 1e-5 ? std::sqrt(Gravity * Q[0]) : 0.0;
  return std::max(std::abs(velocity - soundSpeed), std::abs(velocity + soundSpeed));
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

double
tests::exahype2::fortran::FVSolver::solveRiemannProblem(
  const double* const NOALIAS                  QL,
  const double* const NOALIAS                  QR,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h,
  const double                                 t,
  const double                                 dt,
  const int                                    normal,
  [[maybe_unused]] const bool                  isBoundaryFace,
  [[maybe_unused]] const int                   faceIndex,
  double* const NOALIAS                        FL,
  double* const NOALIAS                        FR
) {
  // The Fortran solver writes the Clawpack fluctuations A^-dQ into FL and A^+dQ into FR. The Godunov
  // kernel adds dt/h * FR to the right cell, but the right cell needs -dt/h * A^+dQ, so FR changes its
  // sign.
  const double smax = rpn2_augmented_wrapper(QL, QR, FL, FR, &normal);

  FR[0] = -FR[0];
  FR[1] = -FR[1];
  FR[2] = -FR[2];
  FL[3] = 0.0;
  FR[3] = 0.0;

  return smax;
}
