// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "PDE.h"

#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

namespace tests::exahype2::aderdg::grmhd {
  INLINE_METHOD GPU_CALLABLE_METHOD void evaluateKerrSchildMetric(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    double*                                      lapse,
    double*                                      rootDeterminant,
    double*                                      shift,
    double (&gCov)[3][3]
  );

  INLINE_METHOD GPU_CALLABLE_METHOD void setMichelAccretionPrimitives(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    double*                                      V
  );

  INLINE_METHOD GPU_CALLABLE_METHOD void setMichelAccretionState(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    double                                       t,
    double*                                      Q
  );

  INLINE_METHOD GPU_CALLABLE_METHOD void setAlfvenWaveState(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    double                                       t,
    double*                                      Q
  );
} // namespace tests::exahype2::aderdg::grmhd

#include "InitialData.cpph"
