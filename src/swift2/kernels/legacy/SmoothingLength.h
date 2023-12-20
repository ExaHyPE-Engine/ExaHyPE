// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/VertexMarker.h"

#include "swift2/kernels/ParticleUpdatePredicates.h"


namespace swift2 {
  namespace kernels {
    namespace legacy {
      /**
       * Derive smoothing length from density
       *
       * This routine is to be called directly after hydro_end_density. It updates
       * the smoothing length according to the updated density quantities and then
       * decides if the smoothing length computation has terminated with something
       * reasonable. If not, the routine triggers an iteration rerun on this
       * species. The rerun will mean that we recalculate the density (as it
       * depends on the smoothing length) and then check again if we have
       * converged.
       *
       */
      template <typename Particle>
      void hydro_update_smoothing_length_and_rerun_if_required(
        Particle* localParticle
      );

      template <typename Particle>
      void updateSmoothingLengthAndRerunIfRequired(const peano4::datamanagement::VertexMarker& marker, Particle& localParticle) {
        if (::swift2::kernels::localParticleCanBeWorkedOnInVertexKernel(marker,localParticle)) {
          hydro_update_smoothing_length_and_rerun_if_required(&localParticle);
        }
      }
    }
  }
}


#include "SmoothingLength.cpph"
