// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include <list>


namespace swift2 {
  namespace kernels {
    /**
     *
     * I have to explain how things work with this marker
     */
    template <typename Particle>
    void genericInteraction(
      const std::list< Particle* >&  localParticles,
      const std::list< Particle* >&  activeParticles,
      std::function< void(Particle* localParticle, Particle* activeParticle) > interaction
    );
  }
}


#include "ParticleParticleInteraction.cpph"

