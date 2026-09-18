// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Loop.h"
#include "tarch/la/Vector.h"

namespace peano4::particles {
  double drawParticlePositionNoise();

  template <class Particle>
  INLINE_METHOD void init(
    Particle&                                    particle,
    const tarch::la::Vector<DIMENSIONS, double>& position,
    double                                       searchRadius
  );

  template <class Particle>
  INLINE_METHOD void initWithData(
    Particle&                                    particle,
    const tarch::la::Vector<DIMENSIONS, double>& position,
    double                                       searchRadius,
    const std::vector<double>&                   initialData
  );

  template <class Particle>
  INLINE_METHOD std::vector<Particle*> createEquallySpacedParticles(
    double                                       spacing,
    const tarch::la::Vector<DIMENSIONS, double>& voxelCentre,
    const tarch::la::Vector<DIMENSIONS, double>& voxelSize,
    bool                                         addNoise,
    double                                       noiseScale = 1.0
  );

  template <class Particle>
  INLINE_METHOD std::vector<Particle*> createParticlesAlignedWithGlobalCartesianMesh(
    double                                       spacing,
    const tarch::la::Vector<DIMENSIONS, double>& voxelCentre,
    const tarch::la::Vector<DIMENSIONS, double>& voxelSize,
    const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
    const tarch::la::Vector<DIMENSIONS, double>& domainSize,
    bool                                         addNoise,
    double                                       noiseScale = 1.0
  );
} // namespace peano4::particles

#include "peano4/particles/ParticleFactory.cpph"
