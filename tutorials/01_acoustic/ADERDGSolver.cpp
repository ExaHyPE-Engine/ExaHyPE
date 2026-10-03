// This file is part of the ExaHyPE 2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ADERDGSolver.h"

namespace {
  constexpr double Rho = 1.0;
  constexpr double C   = 1.0;         // Sound speed
  constexpr double K0  = Rho * C * C; // Bulk modulus
} // namespace

void
tutorials::exahype2::acoustic::ADERDGSolver::initialCondition(
  double* const NOALIAS                        Q,
  const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize
) {
  Q[Shortcuts::p]     = 0.0;
  Q[Shortcuts::v + 0] = 0.0;
  Q[Shortcuts::v + 1] = 0.0;
}

void
tutorials::exahype2::acoustic::ADERDGSolver::boundaryConditions(
  const double* const NOALIAS                  Qinside,
  double* const NOALIAS                        Qoutside,
  const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize,
  const double                                 time,
  const int                                    normal
) {
  Qoutside[Shortcuts::p]     = 0.0;
  Qoutside[Shortcuts::v + 0] = 0.0;
  Qoutside[Shortcuts::v + 1] = 0.0;
}

double
tutorials::exahype2::acoustic::ADERDGSolver::maxEigenvalue(
  const double* const NOALIAS                  Q,
  const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize,
  const double                                 time,
  const double                                 timeStepSize,
  const int                                    normal
) {
  return C;
}

void
tutorials::exahype2::acoustic::ADERDGSolver::flux(
  const double* const NOALIAS                  Q,
  const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize,
  const double                                 time,
  const double                                 timeStepSize,
  const int                                    normal,
  double* const NOALIAS                        F
) {
  F[Shortcuts::p]          = K0 * Q[Shortcuts::v + normal];
  F[Shortcuts::v + 0]      = 0.0;
  F[Shortcuts::v + 1]      = 0.0;
  F[Shortcuts::v + normal] = Q[Shortcuts::p] / Rho;
}

void
tutorials::exahype2::acoustic::ADERDGSolver::pointSource(
  const double* const NOALIAS Q,
  const double* const         x,
  const double                t,
  const double                dt,
  double* const NOALIAS       forceVector,
  const int                   n
) {
  forceVector[Shortcuts::p]     = 0.0;
  forceVector[Shortcuts::v + 0] = 0.0;
  forceVector[Shortcuts::v + 1] = 0.0;

  constexpr double PulsePeakTime  = 0.25;
  constexpr double PulseWidth     = 0.08;
  constexpr double PulseAmplitude = 1.0;
  forceVector[Shortcuts::p] =
    PulseAmplitude * std::exp(-(t - PulsePeakTime) * (t - PulsePeakTime) / (2.0 * PulseWidth * PulseWidth));
}

void
tutorials::exahype2::acoustic::ADERDGSolver::initPointSourceLocations(
  double location[NumberOfPointSources][DIMENSIONS]
) {
  location[0][0] = -0.7;
  location[0][1] = -0.7;
}
