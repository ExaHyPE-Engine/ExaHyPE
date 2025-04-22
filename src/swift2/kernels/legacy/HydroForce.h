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
       * @brief Prepare a particle for the force calculation.
       *
       * Convert some quantities coming from the density loop over neighbours
       * into quantities ready to be used in the force loop over neighbours.
       * Quantities are typically read from the density sub-structure and
       * written to the force sub-structure. Examples of calculations done here
       * include the calculation of viscosity term constants, thermal conduction
       * terms, hydro conversions, etc.
       */
      template <typename Particle>
      void hydro_prepare_force(Particle& localParticle);

      template <typename Particle>
      bool prepareHydroForceUpdateParticlePredicate(
        const peano4::datamanagement::VertexMarker& marker,
        const Particle&                             localParticle
      ) {
        return ::swift2::kernels::localParticleCanBeUpdatedInVertexKernel(marker, localParticle);
      }

      template <typename Particle>
      void prepareHydroForce(
        const peano4::datamanagement::VertexMarker& marker,
        Particle&                                   localParticle
      ) {
        assertion(prepareHydroForceUpdateParticlePredicate(marker, localParticle));
        hydro_prepare_force(localParticle);
      }

      /**
       * @brief Reset acceleration fields of a particle
       *
       * Resets all hydro acceleration and time derivative fields in preparation
       * for the sums taking place in the various force interactions.
       */
      template <typename Particle>
      void hydro_reset_acceleration(Particle& localParticle);

      template <typename Particle>
      bool resetAccelerationUpdateParticlePredicate(
        const peano4::datamanagement::VertexMarker& marker,
        const Particle&                             localParticle
      ) {
        return ::swift2::kernels::localParticleCanBeUpdatedInVertexKernel(marker, localParticle);
      }

      template <typename Particle>
      void resetAcceleration(
        const peano4::datamanagement::VertexMarker& marker,
        Particle&                                   localParticle
      ) {
        assertion(resetAccelerationUpdateParticlePredicate(marker, localParticle));
        hydro_reset_acceleration(localParticle);
      }

      /**
       * @brief Finishes the force calculation.
       *
       * Multiplies the force and accelerations by the appropiate constants
       * and add the self-contribution term. In most cases, there is little
       * to do here.
       *
       * Cosmological terms are also added/multiplied here.
       *
       * @param localParticle The particle to act upon
       */
      template <typename Particle>
      void hydro_end_force(Particle& localParticle);

      template <typename Particle>
      bool endHydroForceCalculationUpdateParticlePredicate(
        const peano4::datamanagement::VertexMarker&  marker,
        const Particle&                              localParticle
      ) {
        return ::swift2::kernels::localParticleCanBeUpdatedInVertexKernel(marker, localParticle);
      }

      template <typename Particle>
      void endHydroForceCalculation(
        const peano4::datamanagement::VertexMarker& marker,
        Particle&                                   localParticle
      ) {
        assertion(endHydroForceCalculationUpdateParticlePredicate(marker, localParticle));
        hydro_end_force(localParticle);
      }

      /**
       * @brief The actual force kernel, which interacts a local particle and an
       * active particle, and updates the local particle.
       *
       *
       * In this kernel, we accumulate the sums necessary for the force interactions
       * between particles. In the "Minimal" SPH, by the end of it we're updating
       * the velocity and the internal energy via
       *
       * @f$ \frac{d\mathbf{v}_i}{dt} =
       *  - \sum_j m_j \left[
       *    \frac{f_i P_i}{\rho_i^2} \nabla_x W(\mathbf{x}_{ij}, h_i)
       *    + \frac{f_j P_j}{\rho_j^2} \nabla_x W(\mathbf{x}_{ij}, h_j)
       *    + \nu_{ij} \overline{\nabla_x W_{ij}}
       *  \right]
       * @f$
       *
       * and
       *
       * @f$ \frac{du_i}{dt} =
       *  \sum_j m_j \left[
       *    \frac{f_i P_i}{\rho_i^2} \mathbf{v}_{ij} \cdot \nabla_x W(\mathbf{x}_{ij}, h_i)
       *    + \frac{1}{2} \nu_{ij} \mathbf{v}_{ij} \cdot \overline{\nabla_x W_{ij}}
       *  \right]
       * @f$
       *
       * where
       *
       * @f$ \nu_{ij} = -\frac{1}{2} \frac{\alpha \mu_{ij} v_{sig,ij}}{\overline{\rho}_{ij}}@f$
       *
       * @f$ v_{sig,ij} = c_i + c_j - \beta \mu_{ij}@f$
       *
       * @f$ \mu_{ij} = \frac{\mathbf{v}_{ij} \cdot \mathbf{x}_{ij}}{|\mathbf{x}_{ij}|}@f$
       * if @f$ \mathbf{v}_{ij} \cdot \mathbf{x}_{ij} < 0@f$, or 0 otherwise
       *
       * @f$ f_i = \left( 1 + \frac{h_i}{3 \rho_i} \frac{\partial \rho_i}{\partial h_i} \right)^{-1} @f$
       *
       * @f$ c_i@f$: soundspeed of particle i
       *
       * @f$ P_i@f$: pressure of particle i
       *
       * @f$ \mathbf{x}_{ij} = \mathbf{x}_{i} - \mathbf{x}_{j} @f$
       *
       * @f$ \mathbf{v}_{ij} = \mathbf{v}_{i} - \mathbf{v}_{j} @f$
       *
       * @f$ \alpha, \beta @f$: user set viscosity coefficient. Usually 0 and 3, respectively.
       *
       * @f$ \overline{\rho}_{ij} = \frac{1}{2} (\rho_i + \rho_j) @f$
       *
       * @f$ \overline{\nabla_x W}_{ij} = \frac{1}{2} (\nabla_x W_i + \nabla_x W_j) @f$
       *
       * For a kernel
       *
       * @f$ W(\mathbf{x}_{ij}, h)
       *   = \frac{1}{h^d} f \left( \frac{\mathbf{x}_{ij}}{h} \right)
       *   = \frac{1}{h^d} f \left( q \right) @f$
       *
       * in @f$ d @f$ dimensions, we have
       *
       * @f$ \nabla_x W(\mathbf{x}_{ij}, h) =
       *   \frac{1}{h^{d+1}} \frac{\partial f(q)}{\partial q} \frac{\mathbf{x}_{ij}}{|\mathbf{x}_{ij}|} @f$
       *
       *
       * ## Vectorisation
       *
       * This operation struggles to vectorise, as we have the statement
       *

       * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
       *   if ((tarch::la::smaller(r, iactRi) or tarch::la::smaller(r, iactRj))
       * and tarch::la::greater(r, 0.0))
       * {
       * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
       *
       * in there. This if statement encapsulates quite a lot of operations. The
       * Intel compiler therefore cannot vectorise this routine anymore.
       *
       *
       * @see forceKernelVectorised()
       */
      template <typename ParticleA, typename ParticleB = ParticleA>
      void force_kernel(
        ParticleA&       localParticle,
        const ParticleB& activeParticle
      );


      template <typename Particle>
      bool forceKernelUpdateParticlePredicate(
        const peano4::datamanagement::CellMarker& marker, const bool& localParticleIsContainedInCell, const Particle& localParticle
      );

      template <typename Particle>
      bool forceKernelEvaluatePairInteractionPredicate(
        const peano4::datamanagement::CellMarker& marker, const Particle& localParticle, const Particle& activeParticle
      );

      /**
       * Generic force kernel implementation
       *
       * Is to be used in combination with
       *
       * - forceKernelUpdateParticlePredicate() and
       * - forceKernelEvaluatePairInteractionPredicate()
       */
      template <typename ParticleA, typename ParticleB = ParticleA>
      void forceKernel(
        const peano4::datamanagement::CellMarker& marker,
        ParticleA&                                 localParticle,
        const ParticleB&                           activeParticle
      );

      /**
       * Predict hydro terms (SWIFT)
       * Relevant for inactive particles when using local-timestepping, but
       * always used.
       */
      template <typename Particle> void predict_hyrdro(Particle* particle);


      template <typename Particle>
      bool predictHydroUpdateParticlePredicate(
        const peano4::datamanagement::VertexMarker&  marker,
        const Particle&                              localParticle
      ) {
        return ::swift2::kernels::localParticleCanBeUpdatedInVertexKernel(marker, localParticle);
      }


      template <typename Particle>
      void predictHydro(
        const peano4::datamanagement::VertexMarker& marker,
        Particle&                                   localParticle
      ) {
        assertion(predictHydroUpdateParticlePredicate(marker, localParticle));
        predict_hyrdro(&localParticle);
      }


      /**
       * Check distance between two particles
       *
       * First part of force kernel routine, where we check if two particles are
       * close enough to actually interact. This routine also returns true if
       * localParticle and activeParticle are the same. That is, it does not
       * check for equality. Therefore, the routine has to be used with
       * forceKernelVectorised() which in turn checks the distance interally but
       * masks out zero distances.
       */
      template <typename Particle>
      bool forceKernelDistanceCheck(
        Particle* __restrict__ localParticle,
        const Particle* __restrict__ activeParticle
      );
    } // namespace legacy
  }   // namespace kernels
} // namespace swift2


#include "HydroForce.cpph"
