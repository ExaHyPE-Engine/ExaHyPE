// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "peano4/datamanagement/CellMarker.h"
#include "toolbox/particles/MultiscaleTransitions.h"

namespace swift2 {
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
