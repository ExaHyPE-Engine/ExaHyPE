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
       * @brief Predict additional particle fields forward in time when drifting
       *
       * Additional hydrodynamic quantites are drifted forward in time here. These
       * include thermal quantities (thermal energy or total energy or entropy, ...).
       */
      template <typename Particle>
      void hydro_predict_extra(Particle* localParticle);

      template <typename Particle>
      void hydroPredictExtra(const peano4::datamanagement::VertexMarker& marker, Particle& localParticle) {
        if (::swift2::kernels::localParticleCanBeUpdatedInVertexKernel(marker,localParticle)) {
          hydro_predict_extra(&localParticle);
        }
      }

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
      void endDensityComputation(const peano4::datamanagement::VertexMarker& marker, Particle& localParticle) {
        if (::swift2::kernels::localParticleCanBeUpdatedInVertexKernel(marker,localParticle)) {
          hydro_end_density(&localParticle);
        }
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
       * @brief Finishes the calculations from the density loop
       *
       * Multiplies the denstiy and number of neighbours by the appropiate
       * constants and add the self-contribution term. Additional quantities
       * such as the velocity gradients will also get the final terms added to
       * them here.
       * This is equivalent to the hydro_end_density function in SWIFT
       *
       * Ends the density calculation. Also does a single Newton-Raphson
       * iteration.
       *
       *
       * @param ParticleContainer Typicaly either
       *         std::list<Particle *>
       *   or
       *         std::unordered_set<Particle *>
       *
       * @todo Do we need this one?
       *
       */
      template <typename Particle>
      void end_density_calculation(Particle* particle);

      template <typename Particle>
      void endDensityCalculation(const peano4::datamanagement::VertexMarker& marker, Particle& localParticle) {
        if (::swift2::kernels::localParticleCanBeUpdatedInVertexKernel(marker, localParticle)) {
          end_density_calculation(&localParticle);
        }
      }

      /**
       * @brief Finishes the calculations from the density loop same as
       * endDensityCalculation(...), but doesn't modify the particles'
       * smoothing lengths nor search radii.
       *
       * WARNING: the only reason we still have this is because it was already implemented,
       * and we might use it in the future. However we currently DO NOT have a SPH
       * formulation which correctly works with constant smoothing lengths. We're solving
       * the wrong equations for that.
       */
      template <typename Particle>
      void end_density_calculation_constant_smoothing_length(Particle* particle);

      template <typename Particle>
      void endDensityCalculationConstantSmoothingLength(const peano4::datamanagement::VertexMarker& marker, Particle& localParticle, const Particle& activeParticle) {
        if (::swift2::kernels::localParticleCanBeUpdatedInVertexKernel(marker,localParticle)) {
          end_density_calculation_constant_smoothing_length(&localParticle);
        }
      }
    }
  }
}


#include "Density.cpph"

