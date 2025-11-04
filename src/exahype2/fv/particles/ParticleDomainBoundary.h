// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "ParticleHelper.h"
#include "tarch/la/Vector.h"

namespace exahype2 {
  namespace fv {
    namespace particles {
      namespace boundary {

        /**
         * Reflects a particle off domain boundaries if it leaves the domain.
         *
         * The position is modified in-place to bring it back within bounds.
         * The particle's velocity is reflected and optionally damped to model energy loss.
         *
         * @tparam Particle The particle type (must provide getX, setX, getVelocity, setVelocity)
         * @param particle The particle being checked and reflected
         * @param newPosition The proposed new position of the particle
         * @param damp Damping factor applied to reflected velocity (e.g., 0.8 for 20% loss)
         */
        template <typename Particle>
        void reflectIfOutOfDomain(
          Particle&                              particle,
          tarch::la::Vector<DIMENSIONS, double>& newPosition,
          double                                 dampingFactor
        );

      } // namespace boundary
    } // namespace particles
  } // namespace fv
} // namespace exahype2

#include "ParticleDomainBoundary.cpph"
