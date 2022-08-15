#ifndef PEANO_EQUATION_OF_STATE_HYDRO_H
#define PEANO_EQUATION_OF_STATE_HYDRO_H

#include "Constants.h"
#include "cmath"

/*
 * Returns pressure using the standard EoS.
 *
 */

__attribute__((always_inline)) inline static double gas_pressure_from_internal_energy(
  double density, double u){

  return density * u * ( examples::particles::sph::GAMMA - 1.0 );

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

__attribute__((always_inline)) inline static double gas_internal_energy_from_pressure(
  double density, double pressure){

  return  1.0 / ( examples::particles::sph::GAMMA - 1.0 ) * pressure / density;
}



/*
 * @brief Returns the sound speed given the internal energy.
 * This assumes the standard EoS holds.
 *
 */

__attribute__((always_inline)) inline static double gas_soundspeed_from_internal_energy(
  double u){

  return std::sqrt( u * examples::particles::sph::GAMMA*(examples::particles::sph::GAMMA - 1.0) );

}








#endif // PEANO_EQUATION_OF_STATE_HYDRO_H

