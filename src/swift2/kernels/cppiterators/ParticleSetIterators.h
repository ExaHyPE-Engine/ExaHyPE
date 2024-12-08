// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "../ParticleSetIterators.h"


namespace swift2 {
  namespace kernels {
    namespace coalesced {
      /**
       * Version of forAllParticles assuming chunks of coalesced memory
       *
       * Please consult the cousin routine in swift2/kernels/ParticleSetIterators.h for generic docu.
       *
       * The routine accepts a container of pointers called assignedParticles.
       * This is really just a sequence of pointers to particles on the heap
       * and we can, for the generic baseline implementation in the namespace
       * above, not make any assumptions on the order of the particles.
       *
       * For this particular variant, we however know that there is some
       * structure in the particles, i.e. that assignedParticles actually hosts
       * numberOfCoalescedAssignedParticles coalesced particles.
       *
       * Instead of looping over the container, we can therefore take the
       * address of the very first particle in the sequence, and then use a
       * simple for loop with pointer arithmetics to traverse the whole
       * sequence.
       */
      template <typename ParticleContainer>
      void forAllParticles(
        const peano4::datamanagement::VertexMarker&                         marker,
        ParticleContainer&                                                  assignedParticles,
        int                                                                 numberOfCoalescedAssignedParticles,
        ParticleUnaryOperatorOnVertex<typename ParticleContainer::DoFType > auto f,
        UpdateParticleAssignedToVertexPredicate<typename ParticleContainer::DoFType> predicate
      );


      /**
       * Version of forAllParticles assuming chunks of coalesced memory
       *
       * Please consult the cousin routine in swift2/kernels/ParticleSetIterators.h for generic docu.
       *
       * The routine accepts a container of pointers called assignedParticles.
       * This is really just a sequence of pointers to particles on the heap
       * and we can, for the generic baseline implementation in the namespace
       * above, not make any assumptions on the order of the particles.
       *
       * @image html coalesced-memory.png
       *
       * For this particular variant, we however know that there is some
       * structure in the pointers. We know that it consists of chunks. In
       * the example above, the first four particles on the heap are stored
       * continuously. The next three are also stored en bloc, albeit maybe
       * somewhere completely differently on the heap.
       *
       * We exploit this knowledge by looping over the chunks identified by
       * numberOfCoalescedLocalParticlesPerVertex in the outer loop. For
       * each chunk, we can then use a plain for loop with pointer
       * arithmetics similar to the techniques in forAllParticles().
       * This innermost loop should, in theory, be well-suited to vectorise.
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
       * Simple vectorised version of pair-wise comparison
       *
       * Please consult the cousin routine in swift2/kernels/ParticleSetIterators.h for generic docu.
       *
       * We know in this case that activeParticles is clustered into chunks
       * described by numberOfCoalescedActiveParticlesPerVertex. The same
       * reasoning holds for localParticles. So this routine is very similar
       * to forAllParticles(), but it hosts two different particle sets
       * (which might even point to different particle types) and the set of
       * active particles is likely much bigger than the local particles.
       *
       * Our implementation does not exploit our specialist knowledge about
       * the local particles. It loops over them one by one. Per local
       * particles it has to loop over all active particles. Here, we
       * employ exactly the techniques described for forAllParticles(),
       * i.e. we run over the active particle set chunk-wisely. Per chunk,
       * we exploit the fact that particles are stored continuously.
       */
      template <typename LocalParticleContainer, typename ActiveParticleContainer>
      void forAllParticlePairs(
        const peano4::datamanagement::CellMarker&  marker,
        LocalParticleContainer&                    localParticles,
        ActiveParticleContainer&                   activeParticles,
        const std::vector<int>&                    numberOfCoalescedLocalParticlesPerVertex,
        const std::vector<int>&                    numberOfCoalescedActiveParticlesPerVertex,
        ParticleBinaryOperator<PCParticle<LocalParticleContainer>, PCParticle<ActiveParticleContainer>> auto f,
        UpdateParticleAssignedToCellPredicate<PCParticle<LocalParticleContainer>>  localParticlePredicate,
        UpdateParticlePairWithinCellPredicate<PCParticle<LocalParticleContainer>>  particlePairPredicate
      );

      namespace prefixcheck {
        template <typename ParticleContainer>
        void forAllParticles(
          const peano4::datamanagement::VertexMarker&                         marker,
          ParticleContainer&                                                  assignedParticles,
          int                                                                 numberOfCoalescedAssignedParticles,
          ParticleUnaryOperatorOnVertex<typename ParticleContainer::DoFType > auto f,
          UpdateParticleAssignedToVertexPredicate<typename ParticleContainer::DoFType> predicate
        );

        template <typename ParticleContainer>
        void forAllParticles(
          const peano4::datamanagement::CellMarker&                  marker,
          ParticleContainer&                                         localParticles,
          const std::vector<int>&                                    numberOfCoalescedLocalParticlesPerVertex,
          ParticleUnaryOperatorOnCell<PCParticle<ParticleContainer>> auto f,
          UpdateParticleAssignedToCellPredicate<PCParticle<ParticleContainer>>  predicate
        );

        template <typename LocalParticleContainer, typename ActiveParticleContainer>
        void forAllParticlePairs(
          const peano4::datamanagement::CellMarker&  marker,
          LocalParticleContainer&                    localParticles,
          ActiveParticleContainer&                   activeParticles,
          const std::vector<int>&                    numberOfCoalescedLocalParticlesPerVertex,
          const std::vector<int>&                    numberOfCoalescedActiveParticlesPerVertex,
          ParticleBinaryOperator<PCParticle<LocalParticleContainer>, PCParticle<ActiveParticleContainer>> auto f,
          UpdateParticleAssignedToCellPredicate<PCParticle<LocalParticleContainer>>  localParticlePredicate,
          UpdateParticlePairWithinCellPredicate<PCParticle<LocalParticleContainer>>  particlePairPredicate
        );
      }
    }
  } // namespace kernels
} // namespace swift2


#include "ParticleSetIterators.cpph"
