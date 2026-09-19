// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/dataview/CellMarker.h"
#include "peano4/particles/particles.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/la/Vector.h"

namespace exahype2 {
  template <typename Particle, typename Projection>
  INLINE_METHOD void moveParticlesThroughCell(
    const peano4::dataview::CellMarker&          marker,
    const std::list<Particle*>&                  particles,
    const tarch::la::Vector<DIMENSIONS, double>& domainWidth,
    const std::bitset<DIMENSIONS>&               periodicBoundaries,
    Projection&&                                 project
  );

  template <typename Particles>
  INLINE_METHOD void markParticlesAsNotMoved(Particles& particles);
} // namespace exahype2

#include "exahype2/Tracing.cpph"
