// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FVSolver.h"

double
tests::exahype2::fortran::FVSolver::maxEigenvalue(
  const State&                              Q,
  const Coordinates&                        x,
  const Coordinates&                        h,
  const double                              t,
  const double                              dt,
  const int                                 normal,
  [[maybe_unused]] const RuntimeParameters& parameters
) {
  static constexpr double Gravity    = 9.81;
  const double            velocity   = Q.h > 1e-5 ? Q.hu(normal) / Q.h : 0.0;
  const double            soundSpeed = Q.h > 1e-5 ? std::sqrt(Gravity * Q.h) : 0.0;
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
  const State&                              QL,
  const State&                              QR,
  const Coordinates&                        x,
  const Coordinates&                        h,
  const double                              t,
  const double                              dt,
  const int                                 normal,
  [[maybe_unused]] const bool               isBoundaryFace,
  [[maybe_unused]] const int                faceIndex,
  Flux&                                     FL,
  Flux&                                     FR,
  [[maybe_unused]] const RuntimeParameters& parameters
) {
  // The Fortran solver rounds a reflected problem differently. Each Clawpack fluctuation averages the
  // solutions of the problem and of the reflected problem. A reflected face then receives the
  // reflected fluctuations bitwise.
  State reflectedLeftState       = QR;
  State reflectedRightState      = QL;
  reflectedLeftState.hu(normal)  = -reflectedLeftState.hu(normal);
  reflectedRightState.hu(normal) = -reflectedRightState.hu(normal);

  Flux         leftGoing;
  Flux         rightGoing;
  Flux         reflectedLeftGoing;
  Flux         reflectedRightGoing;
  const double maxWaveSpeed = rpn2_augmented_wrapper(
    reinterpret_cast<const double*>(&QL),
    reinterpret_cast<const double*>(&QR),
    reinterpret_cast<double*>(&leftGoing),
    reinterpret_cast<double*>(&rightGoing),
    &normal
  );
  const double reflectedMaxWaveSpeed = rpn2_augmented_wrapper(
    reinterpret_cast<const double*>(&reflectedLeftState),
    reinterpret_cast<const double*>(&reflectedRightState),
    reinterpret_cast<double*>(&reflectedLeftGoing),
    reinterpret_cast<double*>(&reflectedRightGoing),
    &normal
  );

  // The Godunov kernel adds dt/h * FR to the right cell. The right cell needs -dt/h * A^+dQ. FR
  // therefore receives the negated right-going fluctuation.
  FL.h = 0.5 * (leftGoing.h + reflectedRightGoing.h);
  FR.h = -0.5 * (rightGoing.h + reflectedLeftGoing.h);
  for (int axis = 0; axis < 2; axis++) {
    const double reflection = axis == normal ? -1.0 : 1.0;
    FL.hu(axis)             = 0.5 * (leftGoing.hu(axis) + reflection * reflectedRightGoing.hu(axis));
    FR.hu(axis)             = -0.5 * (rightGoing.hu(axis) + reflection * reflectedLeftGoing.hu(axis));
  }
  FL.b = 0.0;
  FR.b = 0.0;

  return std::max(maxWaveSpeed, reflectedMaxWaveSpeed);
}
