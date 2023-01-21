#pragma once

#include "cmath"
#include "swift2/adiabatic_index.h"

/*
 * Returns pressure using the standard EoS.
 *
 */

__attribute__((always_inline)) inline static double
gas_pressure_from_internal_energy(double density, double u) {

  return density * u * hydro_gamma_minus_one;
}

/*
 * This function returns the sound speed given the internal energy.
 * This assumes the standard EoS holds.
 *
 */

__attribute__((always_inline)) inline static double
gas_soundspeed_from_internal_energy(double u) {

  return std::sqrt(u * hydro_gamma * hydro_gamma_minus_one);
}

