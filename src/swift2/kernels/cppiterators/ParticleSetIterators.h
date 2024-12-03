// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "../ParticleSetIterators.h"


namespace swift2 {
  namespace kernels {
    namespace coalesced {
      template <typename ParticleContainer>
      void forAllParticles(
        const peano4::datamanagement::VertexMarker&                         marker,
        ParticleContainer&                                                  assignedParticles,
        int                                                                 numberOfCoalescedAssignedParticles,
        ParticleUnaryOperatorOnVertex<typename ParticleContainer::DoFType > auto f
      );


      template <typename ParticleContainer>
      void forAllLocalParticles(
        const peano4::datamanagement::CellMarker&                  marker,
        ParticleContainer&                                         localParticles,
        const std::vector<int>&                                    numberOfCoalescedLocalParticlesPerVertex,
        ParticleUnaryOperatorOnCell<PCParticle<ParticleContainer>> auto f
      );


      /**
       * Simple vectorised version of pair-wise comparison
       *
       * We know in this case that activeParticles is clustered into chunks
       * described by numberOfCoalescedActiveParticlesPerVertex. A similar
       * reasoning btw holds for localParticles, but we ignore this.
       */
      template <typename LocalParticleContainer, typename ActiveParticleContainer>
      void forAllParticlePairs(
        const peano4::datamanagement::CellMarker&  marker,
        LocalParticleContainer&                    localParticles,
        ActiveParticleContainer&                   activeParticles,
        const std::vector<int>&                    numberOfCoalescedLocalParticlesPerVertex,
        const std::vector<int>&                    numberOfCoalescedActiveParticlesPerVertex,
        ParticleBinaryOperator<PCParticle<LocalParticleContainer>, PCParticle<ActiveParticleContainer>> auto f
      );
    }
  } // namespace kernels
} // namespace swift2


#include "ParticleSetIterators.cpph"
