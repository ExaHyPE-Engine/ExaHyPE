// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "../ParticleSetIterators.h"


namespace swift2 {
  namespace kernels {
    namespace ompoffloading {
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


#include "../ompiterators/ParticleSetIterators.cpph"
