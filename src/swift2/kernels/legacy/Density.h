// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/VertexMarker.h"
#include "swift2/kernels/legacy/Swift.h"
#include "swift2/kernels/ParticleUpdatePredicates.h"


namespace swift2 {
  namespace kernels {
    namespace legacy {

      /**
       * @brief Prepares a particle for the density calculation.
       */
      template <typename Particle>
      void prepareDensity(
        const peano4::datamanagement::VertexMarker& marker,
        Particle&                                   localParticle
      ) {
        if (::swift2::kernels::
              localParticleCanBeUpdatedInVertexKernel(marker, localParticle)) {
          hydro_init_particle(&localParticle);
        }
      }

      /**
       * @todo No masking yet
       */
      template <typename Particle>
      void prepareDensityWithMasking(
        const peano4::datamanagement::VertexMarker& marker,
        Particle&                                   localParticle
      ) {
        prepareDensity(marker, localParticle);
      }


      /**
       * @brief Finishes the density calculation.
       *
       * Multiplies the density and number of neighbours by the appropiate
       * constants and add the self-contribution term. Additional quantities
       * such as velocity gradients will also get the final terms added to them
       * here.
       *
       * Also adds/multiplies the cosmological terms if need be. (Not
       * implemented yet)
       */
      template <typename Particle>
      void hydro_end_density(Particle* localParticle);

      template <typename Particle>
      void endDensityCalculation(
        const peano4::datamanagement::VertexMarker& marker,
        Particle&                                   localParticle
      ) {
        if (::swift2::kernels::
              localParticleCanBeUpdatedInVertexKernel(marker, localParticle)) {
          hydro_end_density(&localParticle);
        }
      }

      template <typename Particle>
      void endDensityCalculationWithMasking(
        const peano4::datamanagement::VertexMarker& marker,
        Particle&                                   localParticle
      ) {
        endDensityCalculation(marker, localParticle);
      }


      /**
       * @brief The actual density kernel, which interacts a local particle and
       * an active particle, and updates the local particle.
       *
       *
       * In this particle-particle interaction loop, we accumulate several
       * quantities. For the subsequent force computation, we need to
       * collect the density
       *
       * @f$ \rho_i = \rho(\mathbf{x}_i) = \sum_j m_j W(r_{ij}, h_i)@f$
       *
       * and its derivative w.r.t.  @f$ h  @f$ :
       *
       * @f$ \frac{\partial \rho_i}{\partial h}
       *      = \sum_j m_j  \frac{\partial W(r_{ij}, h_i)}{\partial h}
       *      @f$
       *
       * (see the @f$f_i@f$ in swift2::kernels::legacy::force_kernel to see
       * where and how it is used)
       * where @f$ m_j @f$ is the particle mass, @f$ r_{ij} @f$ is the distance
       * between particle @f$i @f$ and @f$ j @f$, @f$h_i @f$ is the smoothing
       * length of @f$i @f$ and @f$ W(r, h) @f$ is the SPH kernel function
       * we use.
       *
       * Let
       *
       * @f$ W = \frac{1}{h^\nu} f\left( \frac{r}{h}\right) \equiv
       *         \frac{1}{h^\nu} f( q )
       * @f$
       *
       * for @f$ \nu @f$ dimensions. Then
       *
       * @f$ \frac{\partial W(r_{ij}, h_i)}{\partial h}
       *   = - \nu \frac{1}{h ^ {\nu + 1}} f(r/h) +
       *      \frac{1}{h^\nu} \frac{\partial f(q)}{\partial h}
       *   =  - \nu \frac{1}{h ^ {\nu + 1}} f(q) +
       *      \frac{1}{h^\nu} \frac{\partial f(q)}{\partial q} \frac{\partial
       * q}{\partial h} =  - \frac{1}{h} \left( \nu W(q) + q \frac{\partial
       * W(q)}{\partial q} \right)
       * @f$
       *
       * which is a quantity we accumulate in the density interaction kernel
       * for the estimate of @f$  \frac{\partial \rho_i}{\partial h} @f$.
       *
       *
       * Similarly, for the computation of the smoothing length, we require to
       * accumulate the analgue number densities (and its derivative w.r.t. h):
       *
       * @f$ n_i = \sum_j W(r_{ij}, h_i)@f$
       *
       * @f$ \frac{\partial n_i}{\partial h}
       *      = \sum_j \frac{\partial W(r_{ij}, h_i)}{\partial h}
       *      @f$
       *
       *
       *
       *
       *
       */
      template <typename Particle>
      void density_kernel(
        Particle*       localParticle,
        const Particle* activeParticle
      );


      /**
       * Predicate to decide if to update a particle according to density kernel
       *
       * This is an implementation of the concept UpdateParticleAssignedToCellPredicate()
       * for densityKernel(), i.e. should be used in combination with the latter.
       *
       * The function delegates to ::swift2::kernels::localParticleCanBeUpdatedInCellKernel()
       * and combines it with localParticle.getSmoothingLengthConverged(): We only consider
       * updating a particles if its smoothing length computation has not yet converged.
       */
      template <typename Particle>
      bool densityKernelUpdateParticlePredicate(
        const peano4::datamanagement::CellMarker& marker, const bool& localParticleIsContainedInCell, const Particle& localParticle
      );

      /**
       * Predicate if two particles contribute towards the density checks
       *
       * This predicate assumes that ::swift2::kernels::localParticleCanBeUpdatedInCellKernel()
       * holds. It then evaluates if activeParticle contributes towards
       * localParticle. Is used by the densityKernel() to find out if the
       * actual density calculation is to be invoked, but can also be used as
       * explicit check in combination with densityKernelWithoutChecks().
       *
       * ## Optimisation
       *
       * - The comparison of radii has to be done by comaring the squares, as
       *   we otherwise would need a square root.
       *
       * ::swift2::kernels::localParticleCanBeUpdatedInCellKernel<globaldata::HydroPart>
       */
      template <typename Particle>
      bool densityKernelEvaluatePairInteractionPredicate(
        const peano4::datamanagement::CellMarker& marker, const Particle& localParticle, const Particle& activeParticle
      );

      /**
       * Density kernel
       *
       * This routine forwards the function call to density_kernel(). Please
       * consult this routine for additional information.
       *
       * ## Predicates and preconditions
       *
       * This kernel is to be used with
       *
       * - densityKernelUpdateParticlePredicate() and
       * - densityKernelEvaluatePairInteractionPredicate()
       */
      template <typename Particle>
      void densityKernel(
        const peano4::datamanagement::CellMarker& marker,
        Particle&                                 localParticle,
        const Particle&                           activeParticle
      );
    } // namespace legacy
  }   // namespace kernels
} // namespace swift2


#include "Density.cpph"
