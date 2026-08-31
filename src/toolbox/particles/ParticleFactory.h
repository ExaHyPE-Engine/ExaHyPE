// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Globals.h"
#include "peano4/Loop.h"
#include "tarch/la/Vector.h"

namespace toolbox::particles {
  double drawParticlePositionNoise();

  template <class Particle>
  INLINE_METHOD void init(
    Particle&                                    particle,
    const tarch::la::Vector<DIMENSIONS, double>& position,
    const double                                 searchRadius
  );

  template <class Particle>
  INLINE_METHOD void initWithData(
    Particle&                                    particle,
    const tarch::la::Vector<DIMENSIONS, double>& position,
    const double                                 searchRadius,
    const std::vector<double>&                   initialData
  );

  template <class Particle>
  INLINE_METHOD std::vector<Particle*> createEquallySpacedParticles(
    const double                                 spacing,
    const tarch::la::Vector<DIMENSIONS, double>& voxelCentre,
    const tarch::la::Vector<DIMENSIONS, double>& voxelSize,
    const bool                                   addNoise,
    const double                                 noiseScale = 1.0
  );

  template <class Particle>
  INLINE_METHOD std::vector<Particle*> createParticlesAlignedWithGlobalCartesianMesh(
    const double                                 spacing,
    const tarch::la::Vector<DIMENSIONS, double>& voxelCentre,
    const tarch::la::Vector<DIMENSIONS, double>& voxelSize,
    const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
    const tarch::la::Vector<DIMENSIONS, double>& domainSize,
    const bool                                   addNoise,
    const double                                 noiseScale = 1.0
  );
} // namespace toolbox::particles

#include "toolbox/particles/ParticleFactory.cpph"
