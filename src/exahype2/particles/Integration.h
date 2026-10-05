// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/particles/Forces.h"
#include "exahype2/particles/ParticleSet.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

TARGET_DECLARE
namespace exahype2::particles {
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD State advancePartialStep(
    const State& current,
    double       timeStepSize,
    double       stepFraction
  );
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD State advancePartialStepWithBase(
    const State& base,
    const State& update,
    double       timeStepSize,
    double       factor
  );
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD State advanceSemiImplicitPartialStep(
    const State& current,
    double       timeStepSize,
    double       stepFraction
  );

  template <typename Interpolation>
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<DIMENSIONS, double> advance(
    Particle&                                    particle,
    const Forces&                                forces,
    double                                       timeStepSize,
    const Interpolation&                         interpolateAt,
    const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
    const tarch::la::Vector<DIMENSIONS, double>& domainWidth
  );

  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD bool isPeriodic(int periodicAxes, int axis);

  INLINE_METHOD GPU_CALLABLE_METHOD void reflectAtWalls(
    Particle&                                    particle,
    tarch::la::Vector<DIMENSIONS, double>&       position,
    double                                       dampingFactor,
    const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
    const tarch::la::Vector<DIMENSIONS, double>& domainWidth,
    int                                          periodicAxes
  );

  INLINE_METHOD GPU_CALLABLE_METHOD void wrapIntoDomain(
    tarch::la::Vector<DIMENSIONS, double>&       position,
    const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
    const tarch::la::Vector<DIMENSIONS, double>& domainWidth,
    int                                          periodicAxes
  );
} // namespace exahype2::particles
TARGET_DECLARE_END

#include "exahype2/particles/Integration.cpph"
