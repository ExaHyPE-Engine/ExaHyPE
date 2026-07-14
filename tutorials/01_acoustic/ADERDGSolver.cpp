// This file is part of the ExaHyPE 2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
//
// Worked ADER-DG implementation for the 2D linear acoustic equations, driven by
// a point source near the bottom-left corner of the domain, which radiates an
// expanding circular wave. The conserved variables are addressed through the
// generated Shortcuts enum:
//   Shortcuts::p        pressure       (1 component)
//   Shortcuts::v        velocity       (2 components: +0 is x, +1 is y)
#include "ADERDGSolver.h"

namespace {
  constexpr double Rho = 1.0;          // density
  constexpr double C   = 1.0;          // sound speed
  constexpr double K0  = Rho * C * C;  // bulk modulus
} // namespace

void tutorials::exahype2::acoustic::ADERDGSolver::initialCondition(
  double* const NOALIAS                        Q,
  const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize
) {
  // The medium is at rest; every wave in this run is created by the point source.
  Q[Shortcuts::p]     = 0.0;
  Q[Shortcuts::v + 0] = 0.0;
  Q[Shortcuts::v + 1] = 0.0;
}

void tutorials::exahype2::acoustic::ADERDGSolver::boundaryConditions(
  const double* const NOALIAS                  Qinside,
  double* const NOALIAS                        Qoutside,
  const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize,
  const double                                 time,
  const int                                    normal
) {
  // Non-periodic walls: a quiescent (zero) exterior state, off which the waves
  // reflect when they reach the edge of the box.
  Qoutside[Shortcuts::p]     = 0.0;
  Qoutside[Shortcuts::v + 0] = 0.0;
  Qoutside[Shortcuts::v + 1] = 0.0;
}

double tutorials::exahype2::acoustic::ADERDGSolver::maxEigenvalue(
  const double* const NOALIAS                  Q,
  const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize,
  const double                                 time,
  const double                                 timeStepSize,
  const int                                    normal
) {
  return C;
}

void tutorials::exahype2::acoustic::ADERDGSolver::flux(
  const double* const NOALIAS                  Q,
  const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize,
  const double                                 time,
  const double                                 timeStepSize,
  const int                                    normal,
  double* const NOALIAS                        F
) {
  // The +normal offset turns the face normal into the matching velocity
  // component: normal 0 picks v_x, normal 1 picks v_y.
  F[Shortcuts::p]          = K0 * Q[Shortcuts::v + normal];
  F[Shortcuts::v + 0]      = 0.0;
  F[Shortcuts::v + 1]      = 0.0;
  F[Shortcuts::v + normal] = Q[Shortcuts::p] / Rho;
}

void tutorials::exahype2::acoustic::ADERDGSolver::pointSource(
  const double* const NOALIAS                  Q,
  const double* const                          x,
  const double                                 t,
  const double                                 dt,
  double* const NOALIAS                        forceVector,
  const int                                    n
) {
  // The point source forces the n-th source location (here there is only one).
  // We inject a Gaussian pulse in time into the pressure equation.
  forceVector[Shortcuts::p]     = 0.0;
  forceVector[Shortcuts::v + 0] = 0.0;
  forceVector[Shortcuts::v + 1] = 0.0;

  constexpr double t0 = 0.25;   // time at which the pulse peaks
  constexpr double w  = 0.08;   // pulse width
  constexpr double M0 = 1.0;    // pulse amplitude
  forceVector[Shortcuts::p] = M0 * std::exp(-(t - t0) * (t - t0) / (2.0 * w * w));
}

void tutorials::exahype2::acoustic::ADERDGSolver::initPointSourceLocations(
  double location[NumberOfPointSources][DIMENSIONS]
) {
  // A single point source near the bottom-left corner of the [-1, 1]^2 box.
  location[0][0] = -0.7;
  location[0][1] = -0.7;
}

