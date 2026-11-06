// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/Assertions.h"
#include "tarch/accelerator/CompileTarget.h"

namespace tests::exahype2::aderdg::grmhd {
  using Shortcuts = tests::exahype2::aderdg::ADERDGSolverVariableShortcuts;

  INLINE_METHOD GPU_CALLABLE_METHOD double dotProduct(const double* first, const double* second);

  INLINE_METHOD GPU_CALLABLE_METHOD void crossProduct(
    const double* first,
    const double* second,
    double        scale,
    double*       result
  );

  INLINE_METHOD GPU_CALLABLE_METHOD void multiplyMatrixVector(
    const double (&matrix)[3][3],
    const double* vector,
    double*       result
  );

  INLINE_METHOD GPU_CALLABLE_METHOD double invertMatrix3x3(const double (&matrix)[3][3], double (&inverse)[3][3]);

  INLINE_METHOD GPU_CALLABLE_METHOD void evaluateRecoveryFunction(
    double  squaredVelocity,
    double* residual,
    double* residualDerivative,
    double  gam,
    double  d,
    double  e,
    double  s2,
    double  b2,
    double  sb2,
    double* w
  );

  INLINE_METHOD GPU_CALLABLE_METHOD void findSquaredVelocity(
    double*       v2,
    const double* lowerStart,
    const double* upperStart,
    const double* tolerance,
    double*       w,
    double        gam,
    double        d,
    double        e,
    double        s2,
    double        b2,
    double        sb2,
    bool*         failed
  );

  INLINE_METHOD GPU_CALLABLE_METHOD void convertPrimitiveToConserved(double* Q, const double* V);

  INLINE_METHOD GPU_CALLABLE_METHOD double convertConservedToPrimitive(
    const double* Q,
    double*       V,
    double (&gContr)[3][3]
  );
} // namespace tests::exahype2::aderdg::grmhd

#include "PDE.cpph"
