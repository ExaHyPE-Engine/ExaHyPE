//
// A sample implementation file for this problem:
//
#include "EulerSolver.h"

// c chord length, t maximum relative thickness.
double airfoilSymmetric(double x, double c = 100, double t = 0.12) {
  return 5.0 * t * c
         * (0.2969 * sqrt(x / c) + ((((-0.1015) * (x / c) + 0.2843) * (x / c) - 0.3516) * (x / c) - 0.1260) * (x / c));
}

void tutorials::exahype2::euler::EulerSolver::initialCondition(
  [[maybe_unused]] double* const NOALIAS                        Q, // Q[4+0]
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h
) {
  const double y = airfoilSymmetric(x[0]);

  Q[0] = (y > x[1] && -y < x[1]) ? 1000.0 : 1.0;
  Q[1] = 1.0;
  Q[2] = 0.0;
  Q[3] = 2.5;
}

void tutorials::exahype2::euler::EulerSolver::boundaryConditions(
  [[maybe_unused]] const double* const NOALIAS                  Qinside,  // Qinside[4+0]
  [[maybe_unused]] double* const NOALIAS                        Qoutside, // Qoutside[4+0]
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const int                                    normal
) {
  if (normal == 0 && x[0] == DomainOffset[0]) {
    Qoutside[0] = 1.0;
    Qoutside[1] = 1.0;
    Qoutside[2] = 0.0;
    Qoutside[3] = 2.5;
  } else {
    Qoutside[0] = Qinside[0];
    Qoutside[1] = Qinside[1];
    Qoutside[2] = Qinside[2];
    Qoutside[3] = Qinside[3];
  }
}

double tutorials::exahype2::euler::EulerSolver::maxEigenvalue(
  [[maybe_unused]] const double* const NOALIAS                  Q, // Q[4+0]
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const double                                 dt,
  [[maybe_unused]] const int                                    normal
) {
  const double     irho  = 1.0 / Q[0];
  constexpr double gamma = 1.4;
  const double     p     = (gamma - 1) * (Q[3] - 0.5 * irho * (Q[1] * Q[1] + Q[2] * Q[2]));

  const double c = std::sqrt(gamma * p * irho);
  const double u = Q[normal + 1] * irho;

  return std::max(std::abs(u - c), std::abs(u + c));
}

void tutorials::exahype2::euler::EulerSolver::flux(
  [[maybe_unused]] const double* const NOALIAS                  Q, // Q[4+0]
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const double                                 dt,
  [[maybe_unused]] const int                                    normal,
  [[maybe_unused]] double* const NOALIAS                        F // F[4]
) {
  const double     irho  = 1.0 / Q[0];
  constexpr double gamma = 1.4;
  const double     p     = (gamma - 1) * (Q[3] - 0.5 * irho * (Q[1] * Q[1] + Q[2] * Q[2]));

  F[0] = Q[normal + 1];
  F[1] = Q[normal + 1] * Q[1] * irho;
  F[2] = Q[normal + 1] * Q[2] * irho;
  F[3] = Q[normal + 1] * irho * (Q[3] + p);

  F[normal + 1] += p;
}
