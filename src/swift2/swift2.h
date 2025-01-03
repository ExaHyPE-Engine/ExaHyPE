// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "peano4/datamanagement/CellMarker.h"
#include "toolbox/particles/MultiscaleTransitions.h"

namespace swift2 {
  /**
   * Set the particle markers of all particles within a cell
   *
   * Each particle owns a has-updated marker (a boolean). This marker is unset
   * by UpdateParticleMarker. Cell-wise operations such as the force or density
   * calculation should not update a particle, if its marker is already set. We
   * work with a week ownership model, i.e. particles that sit on the face
   * in-between two cells are updated by the first adjacent cell that becomes
   * active. This is better than a complex rule to make the association unique
   * running risk that we miss out on particles.
   *
   * An example of a predicate that checks this flag is localParticleCanBeUpdatedInCellKernelFromAnyOtherParticle().
   * Despite its name, it also looks if the particle has already been updated
   * before. We have two different places in teh code where we use this
   * routine:
   *
   * In the sequential/domain-decomposition code, we use touchCellLastTime() to
   * set all the markers. This happens in swift2.actionsets.UpdateParticleMarker.
   * In the task-based variant, we have to embed the routine into the actual
   * task  which updates a particle. This is done by
   * swift2.api.actionsets.TaskGraphKernelWrappers.construct_touch_cell_first_time_call().
   */
  void markAllParticlesAsUpdatedWithinCell(
    auto&                                      particleContainer,
    const peano4::datamanagement::CellMarker&  marker
  ) {
    for (auto* particle: particleContainer ) {
      if ( marker.isContained(
        particle->getX(),
        toolbox::particles::internal::relativeGrabOwnershipSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE
      )) {
        particle->setCellHasUpdatedParticle(true);
      }
    }
  }
}
