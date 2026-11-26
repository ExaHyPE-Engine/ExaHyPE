// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "PDE.h"

#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

namespace examples::exahype2::aderdg::grmhd {
  INLINE_METHOD GPU_CALLABLE_METHOD void evaluateKerrSchildMetric(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    double&                                      lapse,
    double&                                      rootDeterminant,
    Vector3&                                     shift,
    Matrix3x3&                                   gCov
  );

  INLINE_METHOD GPU_CALLABLE_METHOD void setMichelAccretionPrimitives(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    Unknowns&                                    V
  );

  INLINE_METHOD GPU_CALLABLE_METHOD void setMichelAccretionState(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    double                                       t,
    Unknowns&                                    Q
  );

  INLINE_METHOD GPU_CALLABLE_METHOD void setAlfvenWaveState(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    double                                       t,
    Unknowns&                                    Q
  );
} // namespace examples::exahype2::aderdg::grmhd

#include "InitialData.cpph"
