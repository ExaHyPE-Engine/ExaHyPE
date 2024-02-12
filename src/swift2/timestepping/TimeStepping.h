// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/VertexMarker.h"

#include "swift2/kernels/ParticleUpdatePredicates.h"


namespace swift2 {
  namespace timestepping {
    /**
     * Reset the marker of a particle from whatever it has been before to
     * NotMoved. This routine should be called in any task graph before a
     * particle's position is actually altered. The name is not brilliant,
     * as you should also call this routine prior to a change of the interaction
     * radius.
     */
    template <typename Particle>
    void reset_moved_particle_marker(Particle&  particle);

    template <typename Particle>
    void resetMovedParticleMarker(
      const peano4::datamanagement::VertexMarker&  marker,
      Particle&                                    particle
    ) {
      if (::swift2::kernels::localParticleCanBeUpdatedInVertexKernel(marker,particle)) {
        reset_moved_particle_marker(particle);
      }
    }

    /**
     * Reset moved marker
     *
     * This is an alternative to resetMovedParticleMarker. We do not check
     * any predicate at all.
     *
     * @todo Mladen I think we can skip the if here, but I'm not sure.
     */
    template <typename Particle>
    void resetMovedParticleMarkerWithMasking(
      const peano4::datamanagement::VertexMarker&  marker,
      Particle&                                    particle
    ) {
      reset_moved_particle_marker(particle);
    }
  }
}


#include "TimeStepping.cpph"


