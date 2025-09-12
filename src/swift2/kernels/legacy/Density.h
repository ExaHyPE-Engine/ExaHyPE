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
      bool prepareDensityUpdateParticlePredicate(
        const peano4::datamanagement::VertexMarker& marker,
        const Particle&                             localParticle
      ) {
        return ::swift2::kernels::localParticleCanBeUpdatedInVertexKernel(
          marker,
          localParticle
        );
      }

      template <typename Particle>
      void prepareDensity(
        const peano4::datamanagement::VertexMarker& marker,
        Particle&                                   localParticle
      ) {
        assertion(prepareDensityUpdateParticlePredicate(marker, localParticle));
        hydro_init_particle(&localParticle);
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
      bool endDensityCalculationUpdateParticlePredicate(
        const peano4::datamanagement::VertexMarker& marker,
        const Particle&                             localParticle
      ) {
        return ::swift2::kernels::localParticleCanBeUpdatedInVertexKernel(
          marker,
          localParticle
        );
      }

      template <typename Particle>
      void endDensityCalculation(
        const peano4::datamanagement::VertexMarker& marker,
        Particle&                                   localParticle
      ) {
        assertion(
          endDensityCalculationUpdateParticlePredicate(marker, localParticle)
        );
        hydro_end_density(&localParticle);
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
      template <typename ParticleA, typename ParticleB = ParticleA>
      void density_kernel(
        ParticleA&       localParticle,
        const ParticleB& activeParticle
      );


      /**
       * Predicate to decide if to update a particle according to density kernel
       *
       * This is an implementation of the concept
       * UpdateParticleAssignedToCellPredicate() for densityKernel(), i.e.
       * should be used in combination with the latter.
       *
       * The function delegates to
       * ::swift2::kernels::localParticleCanBeUpdatedInCellKernel() and combines
       * it with localParticle.getSmoothingLengthConverged(): We only consider
       * updating a particles if its smoothing length computation has not yet
       * converged.
       */
      template <typename Particle>
      bool densityKernelUpdateParticlePredicate(
        const peano4::datamanagement::CellMarker& marker,
        const bool&     localParticleIsContainedInCell,
        const Particle& localParticle
      );

      /**
       * Predicate if two particles contribute towards the density checks
       *
       * This predicate assumes that
       * ::swift2::kernels::localParticleCanBeUpdatedInCellKernel() holds. It
       * then evaluates if activeParticle contributes towards localParticle. Is
       * used by the densityKernel() to find out if the actual density
       * calculation is to be invoked, but can also be used as explicit check in
       * combination with densityKernelWithoutChecks().
       *
       * ## Optimisation
       *
       * - The comparison of radii has to be done by comaring the squares, as
       *   we otherwise would need a square root.
       *
       * ::swift2::kernels::localParticleCanBeUpdatedInCellKernel<globaldata::HydroPart>
       */
      template <typename ParticleA, typename ParticleB = ParticleA>
      bool densityKernelEvaluatePairInteractionPredicate(
        const peano4::datamanagement::CellMarker& marker,
        const ParticleA&                          localParticle,
        const ParticleB&                          activeParticle
      );

      /**
       * Invoke density kernel without any outer additional admissibility checks
       *
       * This routine forwards the function call to density_kernel(). Please
       * consult this routine for additional information.
       *
       * The kernel assumes that the predicates all
       * hold, i.e. we do not check them anymore. As
       * ::swift2::kernels::localParticleCanBeUpdatedInCellKernel(),
       * we don't have to check anything and directly forward
       * the call to the Swift 1 "legacy" kernel. It makes no sense to use this
       * version in the generic pair-wise interactions, but it makes sense to
       * combine it with coalesced memory accesses and a dedicated outer
       * predicate.
       *
       * That is, a vanilla density calculation equals
       *
       * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
       * ::swift2::kernels::forAllParticlePairs( cellMarker,
       * workItemParticleSet[workItem], workItemParticleSet[workItem],
       * createCoalescedMemoryIndices(), createCoalescedMemoryIndices(),
       * ::swift2::kernels::legacy::densityKernel<globaldata::HydroPart>,
       * ::swift2::kernels::alwaysUpdateInCellKernel<globaldata::HydroPart>,
       * ::swift2::kernels::localParticleCanBeUpdatedInCellKernelFromAnyOtherParticle<globaldata::HydroPart>);
       * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
       *
       * whereas we can use an optimised version such as
       *
       * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
       * ::swift2::kernels::forAllParticlePairs( cellMarker,
       * workItemParticleSet[workItem], workItemParticleSet[workItem],
       * createCoalescedMemoryIndices(), createCoalescedMemoryIndices(),
       * ::swift2::kernels::legacy::densityKernelWithoutChecks<globaldata::HydroPart>,
       * ::swift2::kernels::localParticleCanBeUpdatedInCellKernel<globaldata::HydroPart>,
       * ::swift2::kernels::legacy::densityKernelPairEvaluationPredicate<globaldata::HydroPart>);
       * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
       */
      template <typename ParticleA, typename ParticleB = ParticleA>
      void densityKernel(
        const peano4::datamanagement::CellMarker& marker,
        ParticleA&                                localParticle,
        const ParticleB&                          activeParticle
      );
    } // namespace legacy
  }   // namespace kernels
} // namespace swift2


#include "Density.cpph"
