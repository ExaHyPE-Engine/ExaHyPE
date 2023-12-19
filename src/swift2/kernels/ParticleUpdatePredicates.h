#pragma once

#include "toolbox/particles/MultiscaleTransitions.h"

namespace swift2 {
  namespace kernels {

    /**
     * @brief is this localParticle a local particle (in the ParallelState sense)?
     */
    template <typename Particle>
    bool localParticleIsLocal(const Particle& localParticle) {
      return (localParticle->getParallelState() == std::remove_pointer<Particle>::type::ParallelState::Local);
    }

    /**
     * Overload for convenience. `marker` may be required for function prototype
     * to be passed as argument to other function.
     */
    template <typename Particle, typename Marker>
    bool localParticleIsLocal(const Particle& localParticle, const Marker& marker) {
      return localParticleIsLocal(localParticle);
    }


    /**
     * @brief Can we move (drift) this particle?
     *
     * @param localParticle: particle to check for
     * @param marker: dummy parameter so this function can be passed on as a parameter
     */
    template <typename Particle>
    bool localParticleCanBeMoved(const Particle* localParticle) {
      return (
        (localParticle->getMoveState() == std::remove_pointer<Particle>::type::MoveState::NotMovedYet)
        & localParticleIsLocal(localParticle)
      );
    }

    /**
     * Overload for convenience. `marker` may be required for function prototype
     * to be passed as argument to other function.
     */
    template <typename Particle, typename Marker>
    bool localParticleCanBeMoved(const Particle* localParticle, const Marker& marker) {
      return localParticleCanBeMoved(localParticle);
    }


    /**
     * @brief Can we do work on this particle during a cell kernel sweep stage?
     *
     * I originally thought that this predicate should resemble
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      return (
        not localParticle->getCellHasUpdatedParticle()
        and
        marker.isContained(localParticle->getX())
        and
        not toolbox::particles::particleWillBeDroppedFurther(*localParticle, marker)
      );
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     * However, that's a poor idea, as it does not work along AMR boundaries for
     * particles which reside in a refined cell yet would be dropped into a hanging
     * vertex (which we don't). The file peano4.toolbox.particles.api.AbstractParticleGridAssociation
     * provides some examples on this.
     *
     *
     * @param localParticle: particle to check for
     * @param marker: the cell's CellMarker
     */
    template <typename Particle>
    bool localParticleCanBeWorkedOnCellKernel(
      const Particle* localParticle, const peano4::datamanagement::CellMarker& marker
    ) {

      return (not localParticle->getCellHasUpdatedParticle() and marker.isContained(localParticle->getX()));
    }


    /**
     * @brief Can we do work on this particle during a vertex kernel sweep stage?
     *
     * @param localParticle: particle to check for
     */
    template <typename Particle>
    bool localParticleCanBeWorkedOnVertexKernel(const Particle* localParticle) {
      return localParticleIsLocal(localParticle);
    }

    /**
     * Overload for convenience. `marker` may be required for function prototype
     * to be passed as argument to other function.
     */
    template <typename Particle>
    bool localParticleCanBeWorkedOnVertexKernel(
      const Particle* localParticle, const peano4::datamanagement::VertexMarker& marker
    ) {
      return localParticleCanBeWorkedOnVertexKernel(localParticle);
    }


    /**
     * @brief Dummy function that returns true, so it can be passed on as a parameter.
     *
     * @param localParticle: dummy particle parameter so this function can be passed on as a parameter
     * @param marker: dummy parameter so this function can be passed on as a parameter
     */
    template <typename Particle, typename Marker>
    bool localParticleNoRestrictions(const Particle* localParticle, const Marker& marker) {
      return true;
    }
  } // namespace kernels
} // namespace swift2
