// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/particles/CoordinateOwnership.h"
#include "peano4/dataview/CellMarker.h"
#include "peano4/particles/ParticleFactory.h"
#include "peano4/particles/particles.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/la/Vector.h"

namespace exahype2::particles {
  template <typename Particle, std::size_t NumberOfParticles, typename Vertices, typename Initialisation>
  INLINE_METHOD void insertParticlesByCoordinates(
    const peano4::dataview::CellMarker&                                         marker,
    const std::array<tarch::la::Vector<DIMENSIONS, double>, NumberOfParticles>& coordinates,
    const tarch::la::Vector<DIMENSIONS, double>&                                domainOffset,
    Vertices&                                                                   fineGridVertices,
    int                                                                         treeId,
    Initialisation&&                                                            initialise
  );
} // namespace exahype2::particles

#include "exahype2/particles/InsertParticlesByCoordinates.cpph"
