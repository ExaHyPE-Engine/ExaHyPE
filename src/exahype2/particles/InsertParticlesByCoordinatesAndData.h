// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/particles/CoordinateOwnership.h"
#include "peano4/dataview/CellMarker.h"
#include "peano4/particles/ParticleFactory.h"
#include "peano4/particles/particles.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/la/Vector.h"

namespace exahype2::particles {
  template <typename Particle, int UnknownsPerParticle, typename Vertices>
  INLINE_METHOD void insertParticlesByCoordinatesAndData(
    const peano4::dataview::CellMarker&          marker,
    const std::vector<double>&                   particleData,
    std::vector<bool>&                           particleAlreadyInserted,
    const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
    Vertices&                                    fineGridVertices,
    int                                          treeId,
    int&                                         particleNumberOnThisTree
  );
} // namespace exahype2::particles

#include "exahype2/particles/InsertParticlesByCoordinatesAndData.cpph"
