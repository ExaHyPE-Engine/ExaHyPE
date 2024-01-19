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
       * Convert some quantities coming from the density loop over neighbours into
       * quantities ready to be used in the force loop over neighbours. Quantities
       * are typically read from the density sub-structure and written to the force
       * sub-structure. Examples of calculations done here include the calculation
       * of viscosity term constants, thermal conduction terms, hydro conversions,
       * etc.
       */
      template <typename Particle>
      void hydro_prepare_force(Particle* localParticle);

      template <typename Particle>
      void prepareHydroForce(const peano4::datamanagement::VertexMarker& marker, Particle& localParticle) {
        if (::swift2::kernels::localParticleCanBeUpdatedInVertexKernel(marker,localParticle)) {
          hydro_prepare_force(&localParticle);
        }
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
      void hydro_end_force(Particle* localParticle);

      template <typename Particle>
      void endHydroForceCalculation(const peano4::datamanagement::VertexMarker& marker, Particle& localParticle) {
        if (::swift2::kernels::localParticleCanBeUpdatedInVertexKernel(marker,localParticle)) {
          hydro_end_force(&localParticle);
        }
      }

      /**
       * @brief The actual force kernel, which interacts a local particle and an active
       * particle, and updates the local particle.
       *
       * ## Vectorisation
       *
       * This operation struggles to vectorise, as we have the statement
       *
       * @todo: boundary particle documentation needs to be removed along with
       * that sort of boundary particle treatment
       *
       * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
       *   if (tarch::la::smaller(r, iactR) and tarch::la::greater(r, 0.0) and not localParticle->getIsBoundaryParticle())
       * {
       * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
       *
       * in there. This if statement encapsulates quite a lot of operations. The
       * Intel compiler therefore cannot vectorise this routine anymore.
       *
       *
       * @see forceKernelVectorised()
       */
      template <typename Particle>
      void force_kernel(Particle* localParticle, const Particle* activeParticle);


      template <typename Particle>
      void forceKernel(const peano4::datamanagement::CellMarker& marker, Particle& localParticle, const Particle& activeParticle) {
        if (::swift2::kernels::localParticleCanBeUpdatedInCellKernelFromAnyOtherParticle(marker, localParticle, activeParticle)) {
          force_kernel(&localParticle, &activeParticle);
        }
      }

      /**
       * Predict hydro terms (SWIFT)
       * Relevant for inactive particles when using local-timestepping, but always
       * used.
       */
      template <typename Particle>
      void predict_hyrdro(Particle* particle);


      template <typename Particle>
      void predictHydro(const peano4::datamanagement::VertexMarker& marker, Particle& localParticle) {
        if (::swift2::kernels::localParticleCanBeUpdatedInVertexKernel(marker,localParticle)) {
          predict_hyrdro(&localParticle);
        }
      }

      /**
       * Vectorised alternative implementation of forceKernel()
       *
       * This routine is a vectorised variant of forceKernel() which you can use
       * if and only if you know that localParticle and activeParticle never ever
       * can point to the same object, i.e. if you know that there is no aliasing.
       * It is not clear if this operation is faster than forceKernel, but it
       * delivers more flops due to the excessive vectorisation.
       *
       * To make the function vectorisable, we had to apply a few tweaks compared
       * to the non-vector counterpart. Most of these tweaks are now
       * @ref swift_particle_new_solver "documented as part of Swift's generic solver documentation".
       *
       *
       * ## Specific differences to other vector kernel
       *
       * As we know a priori that this function is only invoked on distinct chunks
       * of particles, we know that we'll compare the same particle against each
       * other. Consequently, we may assume that the radius is never 0. This
       * additional check can be omitted.
       *
       *
       *
       * @todo This routine does not support boundary particles. But we wanted to
       *   remove this flag anyway.
       */
      template <typename Particle>
      void forceKernelVectorised(Particle* __restrict__ localParticle, const Particle* __restrict__ activeParticle);

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
      bool forceKernelDistanceCheck(Particle* __restrict__ localParticle, const Particle* __restrict__ activeParticle);

      // @todo Tobias: Clean up these variants as well such that they work within
      //               a generic setting. After that
      // - Provide vectorised versions of the unary particle updates
      // - Provide both a version which uses the distance check and which doesn't as generic version
      // - docu


      /**
       * Optimised alternative to computeHydroForce()
       *
       * Semantically, this operation does exactly the same as
       * computeHydroForce(). However, there are a few differences how the
       * function runs through the particles. The routine can be used if and only
       * if you use a @ref toolbox_particles_memorypool "memory pool" as it is
       * also discussed in the @ref page_swift_performance_optimisation "optimisation guidelines".
       *
       * The routine knows way more about the internal ordering of the particles
       * through the explicit arguments particlesAssociatedWithLocalVertices and
       * numberOfParticlesPerLocalVertex. It implicitly knows how the particles
       * are stored through the naming convention/the fact that you have to use
       * it with a memory pool. The Python class
       * peano4.toolbox.particles.UpdateParticle_MultiLevelInteraction_StackOfLists_ContiguousParticles.UpdateParticle_MultiLevelInteraction_StackOfLists_ContiguousParticles
       * provides a lot of information how we construct the passed lists over
       * particles.
       *
       * Further to the optimised loops, the function calls forceKernelVectorised()
       * instead of forceKernel() whenever it knows a priori that two particle
       * ranges to not overlap. See forceKernelVectorised() for further remarks
       * how we deliver vectorised kernels.
       *
       * See also @ref swift_particle_new_solver Create new solvers "Swift's discussion how to vectorise kernels"
       * for further details.
       *
       * ## Multiscale relationships
       *
       * Consult computeHydroForce() for a discussion of the handling of
       * multiscale relations. We can exploit the fact here that a set of
       * continuous active particles all should have the same cut-off
       * radius.
       */
      template <typename ParticleContainer>
      void computeHydroForce_vectoriseAndMapDistanceChecksOntoMasking(
        const peano4::datamanagement::CellMarker& marker,
        const ParticleContainer&                  localVertices,
        const std::vector<int>&                   numberOfParticlesPerVertex,
        const ParticleContainer&                  activeParticles,
        const std::vector<int>&                   numberOfActiveParticlesPerVertex
      );


      /**
       * Two-step, vectorised interaction kernel
       *
       * Alternative implementation of the hydro force which splits up the
       * underlying algorithm into two steps:
       *
       * 1. Identify all the particles which actually impose an interaction.
       *    This is a purely geometric distance check and its result is stored in
       *    a boolean array.
       * 2. For those interactions which are flagged as interactive, we actually
       *    evaluate the force.
       *
       * In this variant, we assume that the first step is the part
       * where the majority of the runtime is spent.
       * After step 1 has completed we have a list of markers. Now we still try
       * to vectorise over the markers holding a 1/true: We run trough the particle set
       * once again with a while loop and look ahead N elements (typically N=8). If a
       * reasonable amount of particles hold a 1 marker within this chunk, we process
       * all of them with the vectorised kernel, i.e. accept that some of them might
       * be masked out.
       */
      template <typename ParticleContainer>
      void computeHydroForce_vectoriseDistanceChecks(
        const peano4::datamanagement::CellMarker& marker,
        const ParticleContainer&                  localVertices,
        const std::vector<int>&                   numberOfParticlesPerVertex,
        const ParticleContainer&                  activeParticles,
        const std::vector<int>&                   numberOfActiveParticlesPerVertex
      );
    }
  }
}


#include "HydroForce.cpph"
