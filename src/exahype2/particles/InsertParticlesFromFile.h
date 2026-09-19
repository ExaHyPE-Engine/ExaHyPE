// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/dataview/CellMarker.h"
#include "peano4/particles/ParticleFactory.h"
#include "peano4/particles/particles.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/la/Vector.h"

namespace exahype2::particles {
  template <typename Particle, bool StoreInitialPosition, typename FileReader, typename Vertices>
  INLINE_METHOD void insertParticlesFromFile(
    const peano4::dataview::CellMarker& marker,
    FileReader&                         fileReader,
    Vertices&                           fineGridVertices,
    int                                 treeId,
    int&                                particleNumberOnThisTree,
    const int*                          initialPositionIndex
  );
} // namespace exahype2::particles

#include "exahype2/particles/InsertParticlesFromFile.cpph"
