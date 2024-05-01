#pragma once

#include "toolbox/particles/MultiscaleTransitions.h"
#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/VertexMarker.h"

namespace swift2 {
  namespace kernels {
    template <typename Particle>
    using UpdateParticlePairWithinCellPredicate = std::function<bool(const peano4::datamanagement::CellMarker& marker, const Particle& localParticle, const Particle& activeParticle)>;

    template <typename Particle>
    using UpdateParticleAssignedToCellPredicate = std::function<bool(const peano4::datamanagement::CellMarker& marker, const Particle& localParticle)>;

    template <typename Particle>
    using UpdateParticleAssignedToVertexPredicate = std::function<bool(const peano4::datamanagement::VertexMarker& marker, const Particle& localParticle)>;


    /**
     * Degenerated predicate which always allows for an update.
     */
    template <typename Particle>
    bool alwaysUpdateInVertexKernel(const peano4::datamanagement::VertexMarker&  marker, const Particle& localParticle) {
      return true;
    }


    template <typename Particle>
    bool alwaysUpdateInCellKernel(const peano4::datamanagement::CellMarker&  marker, const Particle& localParticle) {
      return true;
    }


    template <typename Particle>
    bool alwaysUpdateParticlePairs(const peano4::datamanagement::CellMarker&  marker, const Particle& localParticle, const Particle& activeParticle) {
      return true;
    }


    /**
     * @brief is this localParticle a local particle (in the ParallelState sense)?
     */
    template <typename Particle>
    bool particleIsLocal(const peano4::datamanagement::VertexMarker& marker, const Particle& localParticle) {
      return localParticle.getParallelState() == Particle::ParallelState::Local;
    }


    /**
     * @brief Can we move (drift) this particle?
     *
     * This is a more restrictive version compared to
     * localParticleCanBeUpdatedInVertexKernel(), as it allows the underlying
     * kernel to move a particle, too. Hence, the particle has to be local, and
     * we have to check if it has not been moved yet. It is important that we
     * distinguish this more restrictive version from its counterpart, as not
     * each and every mesh traversal might reset the moved marker.
     *
     * @param localParticle: Particle to check for
     * @param marker: Identifier for this vertex
     */
    template <typename Particle>
    bool localParticleCanBeUpdatedAndMovedInVertexKernel(const peano4::datamanagement::VertexMarker& marker, const Particle& localParticle) {
      return (
        (localParticle.getMoveState() == Particle::MoveState::NotMovedYet)
        & particleIsLocal(marker, localParticle)
      );
    }

    /**
     * @brief Can we do work on this particle during a cell kernel sweep stage?
     *
     * A particle is to be updated if and only if
     *
     * - it is located within the cell of interest;
     * - it has not yet been updated by a cell;
     * - the particle interaction triggering the update is not a
     *   self-interaction.
     *
     * The second point is important. A particle might be located right at
     * the face in-between two cells. In this case, it is not clear to which
     * cell is actually belong to. So we are fine if either cell updates it,
     * but it should be only one cel at a time.
     *
     * ## Further remarks
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
    bool localParticleCanBeUpdatedInCellKernelFromAnyOtherParticle(
      const peano4::datamanagement::CellMarker& marker, const Particle& localParticle, const Particle& activeParticle
    ) {
      return &localParticle != &activeParticle
         and not localParticle.getCellHasUpdatedParticle()
         and marker.isContained(
           localParticle.getX(),
           toolbox::particles::internal::relativeGrabOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE
         )
         and localParticle.getParallelState() == Particle::ParallelState::Local;
    }


    template <typename Particle>
    bool localParticleCanBeUpdatedInCellKernel(
      const peano4::datamanagement::CellMarker& marker, const Particle& localParticle
    ) {
      return not localParticle.getCellHasUpdatedParticle()
         and marker.isContained(
           localParticle.getX(),
           toolbox::particles::internal::relativeGrabOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE
         )
         and localParticle.getParallelState() == Particle::ParallelState::Local;
    }


    /**
     * @brief Can we do work on this particle during a vertex kernel sweep stage?
     *
     * This predicate filters out all halo (virtual) particle. It implicitly
     * assumes that the particle-vertex association is correct. Therefore, we
     * really only have to mask out virtual particles. The predicate breaks
     * down if the association is not correct, which means it does not work
     * if particles move.
     *
     * @param localParticle: Particle to check for
     * @param marker: Identifier for this vertex
     */
    template <typename Particle>
    bool localParticleCanBeUpdatedInVertexKernel(const peano4::datamanagement::VertexMarker& marker, const Particle& localParticle) {
      return particleIsLocal(marker, localParticle);
    }
  } // namespace kernels
} // namespace swift2
