// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"

TARGET_DECLARE
namespace tarch::la {
  constexpr double PI                        = 3.1415926535897932384626433832795028841972;
  constexpr double E                         = 2.7182818284590452353602874713526624977572;
  constexpr double NUMERICAL_ZERO_DIFFERENCE = 1.0e-8;

  INLINE_METHOD GPU_CALLABLE_METHOD double max(double a, double b, double c);

  INLINE_METHOD GPU_CALLABLE_METHOD double min(double a, double b, double c);

  INLINE_METHOD GPU_CALLABLE_METHOD double pow(double base, double exponent);
} // namespace tarch::la
TARGET_DECLARE_END

#include "tarch/la/Scalar.cpph"
