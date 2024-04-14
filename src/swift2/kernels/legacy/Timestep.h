#pragma once

namespace swift2 {
  namespace kernels {
    namespace legacy {

      /**
       * @brief Computes the SPH time step size. May depend on SPH flavour.
       *
       * @TODO: add cosmological factors.
       */
      template <typename Particle>
      double hydro_compute_timestep(Particle* particle);


      /**
       * @brief computes the next time step size for all particles associated
       * to this vertex, and writes it into the particle species.
       *
       * @TODO: this needs to be made threadsafe.
       * @TODO: this needs to make use of the other forAll particleSetIterators.
       *  But for that, we need a new particleSetIterator that accepts a return
       *  value and a function to reduce the return value.
       */
      template <typename ParticleContainer>
      void computeNextTimestep(
        const peano4::datamanagement::VertexMarker& marker,
        ParticleContainer& assignedParticles
      );

    }
  }
}

#include "Timestep.cpph"
