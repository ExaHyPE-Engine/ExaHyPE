// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace swift2 {

  template <typename Particle>
  using ParticleUnaryOperator = std::function<void(Particle& localParticle)>;

  template <typename LocalParticle, typename ActiveParticle>
  using ParticleBinaryOperator = std::function<void(LocalParticle& localParticle, ActiveParticle& activeParticle)>;

  template <typename Particle>
  using ParticleUnaryOperatorWithPointerCallSemantics = std::function<void(Particle* localParticle)>;

  /**
   * Run over all particles and update them independent of each other
   *
   * This is a generic particle self-interaction kernel accepting a functor to
   * update one particle. We provide two flavours of the concept: One accepts a
   * particle reference and the other one a pointer.
   *
   * We distinguish two use cases for the particle self-interactions:
   *
   * 1. We want to execute a stand-alone computation over the particle, e.g.
   *    the kick step, which is does not require any particle-particle
   *    contribution.
   * 2. We want to execute computations after and before the particle-particle
   *    interactions kernels, e.g. to 'prepare' or 'end' a given calculation in
   *    SPH loops (e.g. density).
   *
   * In order to ensure the self-interaction kernels execute consistently
   * during the mesh traversals for these type of operations, the user should
   * bear in mind the difference between these two cases:
   *
   *  - The 'prepare' case should be mapped into touchVertexFirstTime(). Peano
   *    internally ensures that CellHasUpdatedParticle is false in this situation.
   *  - The 'end' case should be mapped into touchVertexLastTime. Peano
   *    internally ensures that CellHasUpdatedParticle is true in this situation.
   *
   *
   * @param ParticleContainer A subtype of std::list<Particle *>
   * @param UpdateFunction Has to be of type std::function<void(Particle* localParticle)>.
   */
  template <typename ParticleContainer>
  void forAllParticles(
    ParticleContainer& assignedParticles,
    ParticleUnaryOperator<typename ParticleContainer::DoFType >      f
  );


  template <typename ParticleContainer>
  void forAllParticles(
    ParticleContainer& assignedParticles,
    ParticleUnaryOperatorWithPointerCallSemantics<typename ParticleContainer::DoFType >      f
  );


  /**
   * Run over all local particle-active particle combinations
   *
   * This is basically a nested loop. The outer
   *
   * @param LocalParticleContainer  A subtype of std::list<Particle *>
   * @param ActiveParticleContainer A subtype of std::list<Particle *>
   */
  template <typename LocalParticleContainer, typename ActiveParticleContainer>
  void forAllParticlePairs(
    const peano4::datamanagement::CellMarker& marker,
    LocalParticleContainer&  localParticles,
    ActiveParticleContainer& activeParticles,
    ParticleBinaryOperator<typename LocalParticleContainer::value_type, typename ActiveParticleContainer::value_type>  f
  );
}


#include "ParticleSetIterators.cpph"
