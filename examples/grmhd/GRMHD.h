// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/RefinementCommand.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

namespace examples::grmhd {
  template <class Shortcuts, typename Precision>
  INLINE_METHOD GPU_CALLABLE_METHOD double getMaxEigenvalue(const Precision* NOALIAS Q, int normal);

  template <class Shortcuts, typename Precision>
  INLINE_METHOD GPU_CALLABLE_METHOD void computeFlux(const Precision* NOALIAS Q, int normal, Precision* NOALIAS F);

  template <class Shortcuts, typename Precision>
  INLINE_METHOD GPU_CALLABLE_METHOD void computeNonconservativeProduct(
    const Precision* NOALIAS Q,
    const Precision* NOALIAS deltaQ,
    int                      normal,
    Precision* NOALIAS       BTimesDeltaQ
  );

  template <class Shortcuts, typename Precision, typename Nodes>
  INLINE_METHOD GPU_CALLABLE_METHOD ::exahype2::RefinementCommand getRefinementCommand(
    const Precision* NOALIAS Q,
    const Nodes&             nodes,
    int                      numberOfValues,
    double                   detailTolerance
  );

  template <class Shortcuts, typename Precision>
  INLINE_METHOD GPU_CALLABLE_METHOD void convertPrimitiveToConserved(Precision* Q, const double* V);

  template <class Shortcuts, typename Precision>
  INLINE_METHOD GPU_CALLABLE_METHOD double convertConservedToPrimitive(
    const Precision* Q,
    double*          V,
    double (&gContr)[3][3]
  );

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
    double  inverseEnthalpyFactor,
    double  density,
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
    double        inverseEnthalpyFactor,
    double        density,
    double        e,
    double        s2,
    double        b2,
    double        sb2,
    bool*         failed
  );

  INLINE_METHOD GPU_CALLABLE_METHOD void evaluateKerrSchildMetric(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    double*                                      lapse,
    double*                                      rootDeterminant,
    double*                                      shift,
    double (&gCov)[3][3]
  );
} // namespace examples::grmhd

#include "examples/grmhd/GRMHD.cpph"
