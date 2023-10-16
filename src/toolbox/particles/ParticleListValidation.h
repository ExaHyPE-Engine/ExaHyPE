// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <list>
#include <vector>

#include "peano4/datamanagement/VertexEnumerator.h"

namespace toolbox {
  namespace particles {
    /**
     * Check if all particles are either gathered or host the empt set
     */
    template <typename T>
    void ensureAllParticleListsAreGatheredOrEmpty(
      peano4::datamanagement::VertexEnumerator<T> vertices
    );



    /**
     * Validate encoding of particle lists
     *
     * The flag numberOfParticlesPerVertexAdded encodes how many particles
     * each vertex contributes to a list. If these particles are continuous
     * in memory (per vertex), then we can iterate over activeParticles and
     * validate that the resulting particles match the particles "reconstructed"
     * via pointer arithmetic. If this is not the case, we dump an assertion.
     */
    template <typename T>
    void ensureThatParticleListsEncodeContinuousMemoryChunks(
      const std::list<T*>       activeParticles,
      const std::vector<int>&   numberOfParticlesPerVertexAdded
    );
  }
}


#include "../../toolbox/particles/ParticleListValidation.cpph"
