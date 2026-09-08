// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace applications::exahype2::exaskred::forcing {
  double coriolisParameterOnSphere(double latitudeInDegrees, double angularVelocity);

  double manningFriction(
    double waterHeight,
    double momentumX,
    double momentumY,
    double gravity,
    double manningCoefficient,
    double minimumWaterHeight
  );
} // namespace applications::exahype2::exaskred::forcing
