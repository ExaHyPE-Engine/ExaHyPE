#ifndef PEANO_EQUATION_OF_STATE_HYDRO_H
#define PEANO_EQUATION_OF_STATE_HYDRO_H

#include "cmath"

/*
 * Returns pressure using the standard EoS.
 *
 */

#define hydro_gamma ((double)(1.66666667))

__attribute__((always_inline)) inline static double
gas_pressure_from_internal_energy(double density, double u) {

  return density * u * (hydro_gamma - 1.0);
}

/*
 * This function returns the sound speed given the internal energy.
 * This assumes the standard EoS holds.
 *
 */

__attribute__((always_inline)) inline static double
gas_soundspeed_from_internal_energy(double u) {

  return std::sqrt(u * hydro_gamma * (hydro_gamma - 1.0));
}

#endif // PEANO_EQUATION_OF_STATE_HYDRO_H
