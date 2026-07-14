#pragma once

#include "ParticleDomainBoundary.h"
#include "ParticleForces.h"
#include "ParticleHelper.h"
#include "ParticleIntegration.h"
#include "ParticleInterpolation.h"

#include "peano4/datamanagement/CellMarker.h"
#include "tarch/la/Vector.h"

namespace exahype2 {
  namespace fv {
    namespace particles {
      /**
       * Updates the state of a particle:
       * - Resets acceleration
       * - Interpolates fields from the grid
       * - Applies forces (e.g. gravity)
       * - Computes new position using integration scheme
       * - Applies domain boundary handling (e.g. reflection)
       *
       * @tparam Indices Indices used to extract Q entries
       * @tparam Particle The particle type
       * @param particle Reference to the particle to be updated
       * @param marker The cell marker for position context
       * @param Q Pointer to the cell-based solver data
       */
      template <int... Indices, typename Particle>
      void updateParticleState(
        Particle&                                 particle,
        const peano4::datamanagement::CellMarker& marker,
        const double* __restrict__ Q
      );
    } // namespace particles
  } // namespace fv
} // namespace exahype2

#include "ParticleUpdate.cpph"
