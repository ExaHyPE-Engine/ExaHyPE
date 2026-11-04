// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/accelerator/CompileTarget.h"

namespace applications::exahype2::exaskred::forcing {
  INLINE_METHOD GPU_CALLABLE_METHOD double getCoriolisParameterOnSphere(
    double latitudeInDegrees,
    double angularVelocity
  );

  INLINE_METHOD GPU_CALLABLE_METHOD double getManningFriction(
    double waterHeight,
    double momentumX,
    double momentumY,
    double gravity,
    double manningCoefficient,
    double minimumWaterHeight
  );
} // namespace applications::exahype2::exaskred::forcing

#include "Forcing.cpph"
