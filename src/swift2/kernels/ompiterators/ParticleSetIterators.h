// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "../ParticleSetIterators.h"


namespace swift2 {
  namespace kernels {
    namespace ompoffloading {
      /**
       * Please consult cousing routine in swift2/kernels/ParticleSetIterators.h for generic docu.
       */
      template <typename ParticleContainer>
      void forAllParticles(
        const peano4::datamanagement::VertexMarker&                         marker,
        ParticleContainer&                                                  assignedParticles,
        int                                                                 numberOfCoalescedAssignedParticles,
        ParticleUnaryOperatorOnVertex<typename ParticleContainer::DoFType > auto f,
        UpdateParticleAssignedToVertexPredicate<typename ParticleContainer::DoFType>   predicate
      );


      /**
       * Please consult cousing routine in swift2/kernels/ParticleSetIterators.h for generic docu.
       */
      template <typename ParticleContainer>
      void forAllParticles(
        const peano4::datamanagement::CellMarker&                  marker,
        ParticleContainer&                                         localParticles,
        const std::vector<int>&                                    numberOfCoalescedLocalParticlesPerVertex,
        ParticleUnaryOperatorOnCell<PCParticle<ParticleContainer>> auto f,
        UpdateParticleAssignedToCellPredicate<PCParticle<ParticleContainer>>  predicate
      );


      /**
       * Please consult cousing routine in swift2/kernels/ParticleSetIterators.h for generic docu.
       */
      template <typename LocalParticleContainer, typename ActiveParticleContainer>
      void forAllParticlePairs(
        const swift2::CellMetaData&                cellMetaData,
        LocalParticleContainer&                    localParticles,
        ActiveParticleContainer&                   activeParticles,
        ParticleBinaryOperator<PCParticle<LocalParticleContainer>, PCParticle<ActiveParticleContainer>> auto f,
        UpdateParticleAssignedToCellPredicate<PCParticle<LocalParticleContainer>>  localParticlePredicate,
        UpdateParticlePairWithinCellPredicate<PCParticle<LocalParticleContainer>>  particlePairPredicate
      );
    }
  } // namespace kernels
} // namespace swift2


#include "../ompiterators/ParticleSetIterators.cpph"
