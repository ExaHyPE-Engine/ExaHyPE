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
  State&                   Q,
  const Coordinates&       cellCentre,
  const Coordinates&       cellSize,
  const RuntimeParameters& parameters
) {
  Q = {};
}

void
tutorials::exahype2::acoustic::ADERDGSolver::boundaryConditions(
  const State&             Qinside,
  State&                   Qoutside,
  const Coordinates&       faceCentre,
  const Coordinates&       cellSize,
  const double             time,
  const int                normal,
  const RuntimeParameters& parameters
) {
  Qoutside = {};
}

double
tutorials::exahype2::acoustic::ADERDGSolver::maxEigenvalue(
  const State&             Q,
  const Coordinates&       cellCentre,
  const Coordinates&       cellSize,
  const double             time,
  const double             timeStepSize,
  const int                normal,
  const RuntimeParameters& parameters
) {
  return C;
}

void
tutorials::exahype2::acoustic::ADERDGSolver::flux(
  const State&             Q,
  const Coordinates&       cellCentre,
  const Coordinates&       cellSize,
  const double             time,
  const double             timeStepSize,
  const int                normal,
  Flux&                    F,
  const RuntimeParameters& parameters
) {
  F.p         = K0 * Q.v(normal);
  F.v         = 0.0;
  F.v(normal) = Q.p / Rho;
}

void
tutorials::exahype2::acoustic::ADERDGSolver::pointSource(
  const State&             Q,
  const Coordinates&       x,
  const double             t,
  const double             dt,
  Source&                  forceVector,
  const int                n,
  const RuntimeParameters& parameters
) {
  constexpr double PulsePeakTime  = 0.25;
  constexpr double PulseWidth     = 0.08;
  constexpr double PulseAmplitude = 1.0;
  forceVector                     = {};
  forceVector.p =
    PulseAmplitude * std::exp(-(t - PulsePeakTime) * (t - PulsePeakTime) / (2.0 * PulseWidth * PulseWidth));
}

void
tutorials::exahype2::acoustic::ADERDGSolver::initPointSourceLocations(
  PointSourceLocations&    location,
  const RuntimeParameters& parameters
) {
  location[0] = {-0.7, -0.7};
}
