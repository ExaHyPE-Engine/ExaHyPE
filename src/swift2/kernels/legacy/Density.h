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
       * @brief Prepares a particle for the density calculation.
       *
       * Zeroes all the relevant arrays in preparation for the sums taking place in
       * the various density loop over neighbours. Typically, all fields of the
       * density sub-structure of a particle get zeroed in here.
       */
      template <typename Particle>
      void hydro_prepare_density(Particle* localParticle);

      template <typename Particle>
      void prepareDensity(const peano4::datamanagement::VertexMarker& marker, Particle& localParticle) {
        if (::swift2::kernels::localParticleCanBeUpdatedInVertexKernel(marker,localParticle)) {
          hydro_prepare_density(&localParticle);
        }
      }

      /**
       * @todo No masking yet
       */
      template <typename Particle>
      void prepareDensityWithMasking(const peano4::datamanagement::VertexMarker& marker, Particle& localParticle) {
        prepareDensity(marker, localParticle);
      }


      /**
       * @brief Finishes the density calculation.
       *
       * Multiplies the density and number of neighbours by the appropiate constants
       * and add the self-contribution term.
       * Additional quantities such as velocity gradients will also get the final
       * terms added to them here.
       *
       * Also adds/multiplies the cosmological terms if need be. (Not implemented yet)
       */
      template <typename Particle>
      void hydro_end_density(Particle* localParticle);

      template <typename Particle>
      void endDensityCalculation(const peano4::datamanagement::VertexMarker& marker, Particle& localParticle) {
        if (::swift2::kernels::localParticleCanBeUpdatedInVertexKernel(marker,localParticle)) {
          hydro_end_density(&localParticle);
        }
      }

      template <typename Particle>
      void endDensityCalculationWithMasking(const peano4::datamanagement::VertexMarker& marker, Particle& localParticle) {
        endDensityCalculation(marker, localParticle);
      }


      /**
       * @brief The actual density kernel, which interacts a local particle and an active
       * particle, and updates the local particle.
       */
      template <typename Particle>
      void density_kernel(Particle* localParticle, const Particle* activeParticle);

      template <typename Particle>
      void densityKernel(const peano4::datamanagement::CellMarker& marker, Particle& localParticle, const Particle& activeParticle) {
        if (::swift2::kernels::localParticleCanBeUpdatedInCellKernelFromAnyOtherParticle(marker, localParticle, activeParticle)) {
          density_kernel(&localParticle, &activeParticle);
        }
      }

      /**
       * @todo No masking yet
       */
      template <typename Particle>
      void densityKernelWithMasking(const peano4::datamanagement::CellMarker& marker, Particle& localParticle, const Particle& activeParticle) {
        densityKernel( marker, localParticle, activeParticle );
      }

    }
  }
}


#include "Density.cpph"

