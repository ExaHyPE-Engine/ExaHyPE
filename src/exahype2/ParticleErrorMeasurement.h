// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/ErrorMeasurement.h"
#include "peano4/Constants.h"
#include "tarch/CompilerSpecificSettings.h"

namespace exahype2 {
  template <typename Particle, typename Vertices>
  INLINE_METHOD void collectLocalParticles(
    const Vertices&                          fineGridVertices,
    std::map<std::pair<int, int>, Particle>& particles
  );

  template <int SolutionSize, typename Particle, typename ExactSolution>
  INLINE_METHOD void accumulateParticlePositionErrors(
    std::map<std::pair<int, int>, Particle>& particles,
    double                                   timeStamp,
    double                                   timeStepSize,
    ExactSolution&&                          exactSolution,
    ErrorMeasurement&                        measurement
  );
} // namespace exahype2

#include "exahype2/ParticleErrorMeasurement.cpph"
