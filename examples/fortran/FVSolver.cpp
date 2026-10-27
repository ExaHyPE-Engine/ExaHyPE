// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FVSolver.h"

#include "tarch/la/ScalarOperations.h"

void
examples::fortran::FVSolver::initialCondition(
  double* const NOALIAS                                         Q,
  const tarch::la::Vector<DIMENSIONS, double>&                  x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
  [[maybe_unused]] const RuntimeParameters&                     parameters
) {
  const double radius =
    std::sqrt(tarch::la::productRoundedOnce(x(0), x(0)) + tarch::la::productRoundedOnce(x(1), x(1)));
  Q[Shortcuts::h]      = radius < 0.5 ? 2.0 : 1.0;
  Q[Shortcuts::hu + 0] = 0.0;
  Q[Shortcuts::hu + 1] = 0.0;
  Q[Shortcuts::b]      = 0.0;
}

void
examples::fortran::FVSolver::boundaryConditions(
  [[maybe_unused]] const double* const NOALIAS                  Qinside,
  double* const NOALIAS                                         Qoutside,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const int                                    normal,
  [[maybe_unused]] const RuntimeParameters&                     parameters
) {
  Qoutside[Shortcuts::h]      = 1.0;
  Qoutside[Shortcuts::hu + 0] = 0.0;
  Qoutside[Shortcuts::hu + 1] = 0.0;
  Qoutside[Shortcuts::b]      = 0.0;
}

double
examples::fortran::FVSolver::maxEigenvalue(
  const double* const NOALIAS                  Q,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h,
  const double                                 t,
  const double                                 dt,
  const int                                    normal,
  [[maybe_unused]] const RuntimeParameters&    parameters
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
examples::fortran::FVSolver::solveRiemannProblem(
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
  double* const NOALIAS                        FR,
  [[maybe_unused]] const RuntimeParameters&    parameters
) {
  // The Fortran solver rounds a reflected problem differently. Each Clawpack fluctuation averages the
  // solutions of the problem and of the reflected problem. A reflected face then receives the
  // reflected fluctuations bitwise.
  double reflectedLeftState[4]    = {QR[0], QR[1], QR[2], QR[3]};
  double reflectedRightState[4]   = {QL[0], QL[1], QL[2], QL[3]};
  reflectedLeftState[1 + normal]  = -reflectedLeftState[1 + normal];
  reflectedRightState[1 + normal] = -reflectedRightState[1 + normal];

  double       leftGoing[3];
  double       rightGoing[3];
  double       reflectedLeftGoing[3];
  double       reflectedRightGoing[3];
  const double maxWaveSpeed = rpn2_augmented_wrapper(QL, QR, leftGoing, rightGoing, &normal);
  const double reflectedMaxWaveSpeed =
    rpn2_augmented_wrapper(reflectedLeftState, reflectedRightState, reflectedLeftGoing, reflectedRightGoing, &normal);

  // The Godunov kernel adds dt/h * FR to the right cell. The right cell needs -dt/h * A^+dQ. FR
  // therefore receives the negated right-going fluctuation.
  for (int unknown = 0; unknown < 3; unknown++) {
    const double reflection = unknown == 1 + normal ? -1.0 : 1.0;
    FL[unknown]             = 0.5 * (leftGoing[unknown] + reflection * reflectedRightGoing[unknown]);
    FR[unknown]             = -0.5 * (rightGoing[unknown] + reflection * reflectedLeftGoing[unknown]);
  }
  FL[3] = 0.0;
  FR[3] = 0.0;

  return std::max(maxWaveSpeed, reflectedMaxWaveSpeed);
}
