#pragma once

namespace swift2 {
  namespace kernelPlaceholders {
    /**
     * This file contains the functions that we actually want to use from the
     * original swift codebase, and maintains their original name. The idea is
     * to eventually use the SWIFT implementation of these functions. But for
     * now, we keep a (modified) version of them here.
     *
     * A second point to keep these functions separate is the fact that they
     * do different things for each SPH flavour. So they are intended to be
     * replaceable anyway.
     *
     * There are some notable exceptions which are missing from this collection
     * of functions. Firstly, we keep the particle-particle interactions separately.
     * We are tinkering with them in Peano and optimizing them in a different way.
     * They are defined in ParticleParticleInteraction.h. Secondly, anything related
     * to the computation of the smoothing lengths is defined in
     * SmoothingLengthComputation.h. The smoothing length computation is intimately
     * tied to the neighbour search algorithms, and cannot be separated trivially
     * from the underlying codebase and infrastructure. As such, we can't simply re-use
     * what original SWIFT does, but need to provide our own algorithms.
     * In particular, we differentiate between the search radius and the interaction radius/
     * compact support radius of particles, which original swift doesn't. We provide
     * smoothing length computation algorithms for both a constant search radius as well
     * as a varying search radius.
     */

    /**
     * @brief Prepares a particle for the density calculation.
     *
     * Zeroes all the relevant arrays in preparation for the sums taking place in
     * the various density loop over neighbours. Typically, all fields of the
     * density sub-structure of a particle get zeroed in here.
     */
    template <typename Particle>
    void hydro_init_part(Particle& localParticle);

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
    void hydro_end_density(Particle& localParticle);

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
    void hydro_prepare_force(Particle& localParticle);

    /**
     * @brief Finishes the force calculation.
     *
     * Multiplies the force and accelerations by the appropiate constants
     * and add the self-contribution term. In most cases, there is little
     * to do here.
     *
     * Cosmological terms are also added/multiplied here.
     *
     * @param p The particle to act upon
     * @param cosmo The current cosmological model.
     */
    template <typename Particle>
    void hydro_end_force(Particle& localParticle);


    /**
     * @brief Predict additional particle fields forward in time when drifting
     *
     * Additional hydrodynamic quantites are drifted forward in time here. These
     * include thermal quantities (thermal energy or total energy or entropy, ...).
     */
    template <typename Particle>
    void hydro_predict_extra(Particle& localParticle);

    /**
     * @brief Kick the additional variables
     *
     * Additional hydrodynamic quantites are kicked forward in time here. These
     * include thermal quantities (thermal energy or total energy or entropy, ...).
     */
    template <typename Particle>
    void hydro_kick_extra(Particle& localParticle);

    /**
     * @brief Sets the values to be predicted in the drifts to their values at a
     * kick time.
     */
    template <typename Particle>
    void hydro_reset_predicted_values(Particle& localParticle);


  } // namespace kernelPlaceholders
} // namespace swift2

#include "SwiftKernelPlaceholders.cpph"
