#pragma once

#include "cmath"

#include "adiabatic_index.h"


/**
 * @file equation_of_state.h
 * @brief contains functions and definitions related to the equation of state
 * of ideal gases.
 * This is a legacy file - kept as closely to the SWIFT original files as possible.
 */

namespace swift2 {
  namespace kernels {
  namespace legacy {
    namespace eos {

      // @TODO we need to make this modular. This is only valid for ideal gases. It needs to be exchangeable.

      /**
       * @brief Returns the internal energy given density and entropy
       *
       * Computes \f$u = \frac{A\rho^{\gamma-1} }{\gamma - 1}\f$.
       *
       * @param density The density \f$\rho\f$.
       * @param entropy The entropy \f$A\f$.
       */
      __attribute__((always_inline, const)) inline static double gas_internal_energy_from_entropy(
        double density, double entropy
      ) {

        return entropy * adiabaticIndex::pow_gamma_minus_one(density) * hydro_one_over_gamma_minus_one;
      }

      /**
       * @brief Returns the pressure given density and entropy
       *
       * Computes \f$P = A\rho^\gamma\f$.
       *
       * @param density The density \f$\rho\f$.
       * @param entropy The entropy \f$A\f$.
       */
      __attribute__((always_inline, const)) inline static double gas_pressure_from_entropy(
        double density, double entropy
      ) {

        return entropy * adiabaticIndex::pow_gamma(density);
      }

      /**
       * @brief Returns the entropy given density and pressure.
       *
       * Computes \f$A = \frac{P}{\rho^-\gamma}\f$.
       *
       * @param density The density \f$\rho\f$.
       * @param pressure The pressure \f$P\f$.
       * @return The entropy \f$A\f$.
       */
      __attribute__((always_inline, const)) inline static double gas_entropy_from_pressure(
        double density, double pressure
      ) {

        return pressure * adiabaticIndex::pow_minus_gamma(density);
      }

      /**
       * @brief Returns the sound speed given density and entropy
       *
       * Computes \f$c = \sqrt{\gamma A \rho^{\gamma-1}}\f$.
       *
       * @param density The density \f$\rho\f$.
       * @param entropy The entropy \f$A\f$.
       */
      __attribute__((always_inline, const)) inline static double gas_soundspeed_from_entropy(
        double density, double entropy
      ) {

        return std::sqrt(hydro_gamma * adiabaticIndex::pow_gamma_minus_one(density) * entropy);
      }

      /**
       * @brief Returns the entropy given density and internal energy
       *
       * Computes \f$A = \frac{(\gamma - 1)u}{\rho^{\gamma-1}}\f$.
       *
       * @param density The density \f$\rho\f$
       * @param u The internal energy \f$u\f$
       */
      __attribute__((always_inline, const)) inline static double gas_entropy_from_internal_energy(
        double density, double u
      ) {

        return hydro_gamma_minus_one * u * adiabaticIndex::pow_minus_gamma_minus_one(density);
      }

      /**
       * @brief Returns the pressure given density and internal energy
       *
       * Computes \f$P = (\gamma - 1)u\rho\f$.
       *
       * @param density The density \f$\rho\f$
       * @param u The internal energy \f$u\f$
       */
      __attribute__((always_inline, const)) inline static double gas_pressure_from_internal_energy(
        double density, double u
      ) {

        return hydro_gamma_minus_one * u * density;
      }

      /**
       * @brief Returns the internal energy given density and pressure.
       *
       * Computes \f$u = \frac{1}{\gamma - 1}\frac{P}{\rho}\f$.
       *
       * @param density The density \f$\rho\f$.
       * @param pressure The pressure \f$P\f$.
       * @return The internal energy \f$u\f$.
       */
      __attribute__((always_inline, const)) inline static double gas_internal_energy_from_pressure(
        double density, double pressure
      ) {

        return hydro_one_over_gamma_minus_one * pressure / density;
      }

      /**
       * @brief Returns the sound speed given density and internal energy
       *
       * Computes \f$c = \sqrt{\gamma (\gamma - 1) u }\f$.
       *
       * @param u The internal energy \f$u\f$
       */
      __attribute__((always_inline, const)) inline static double gas_soundspeed_from_internal_energy(double u) {

        return std::sqrt(u * hydro_gamma * hydro_gamma_minus_one);
      }

      /**
       * @brief Returns the sound speed given density and pressure
       *
       * Computes \f$c = \sqrt{\frac{\gamma P}{\rho} }\f$.
       *
       * @param density The density \f$\rho\f$
       * @param P The pressure \f$P\f$
       */
      __attribute__((always_inline, const)) inline static double gas_soundspeed_from_pressure(
        double density, double P
      ) {

        return std::sqrt(hydro_gamma * P / density);
      }

    } // namespace eos
  }   // namespace legacy
  }
} // namespace swift2
