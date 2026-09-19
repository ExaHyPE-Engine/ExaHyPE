// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Constants.h"
#include "peano4/dataview/CellMarker.h"
#include "peano4/particles/internal.h"
#include "peano4/particles/particles.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/la/Scalar.h"
#include "tarch/la/VectorOperations.h"

namespace exahype2::particles {
  template <typename ActiveParticle, typename LocalParticle, typename ActiveVertices, typename LocalVertices>
  INLINE_METHOD int collectParticlesOfCell(
    const peano4::dataview::CellMarker& marker,
    const ActiveVertices&               fineGridVerticesActiveParticles,
    const LocalVertices&                fineGridVerticesLocalParticles,
    std::list<ActiveParticle*>&         activeParticles,
    std::list<LocalParticle*>&          localParticles,
    std::vector<bool>&                  localParticleIsContainedInCell
  );

  template <typename LocalVertices>
  INLINE_METHOD void updateContainedInAdjacentCellOfLocalParticles(
    const peano4::dataview::CellMarker& marker,
    const LocalVertices&                fineGridVerticesLocalParticles
  );

  template <typename ActiveParticle>
  INLINE_METHOD void releaseParticlesOfCell(int numberOfActiveParticles, std::list<ActiveParticle*>& activeParticles);
} // namespace exahype2::particles

#include "exahype2/particles/InteractionSets.cpph"
