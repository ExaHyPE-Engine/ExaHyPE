// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace applications::exahype2::exaskred::meteotsunami {
  namespace coriolisForce {
    double constant(double param);
    double betaPlane(double y, double f0, double beta);
    double sphere(double latDeg, double omega);
  } // namespace coriolisForce

  namespace bottomFriction {
    double manningFriction(double h, double hu, double hv, double gravity, double param);
  } // namespace bottomFriction
} // namespace applications::exahype2::exaskred::meteotsunami
