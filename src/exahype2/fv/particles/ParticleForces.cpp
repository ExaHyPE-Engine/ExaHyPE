// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ParticleForces.h"

// Static member definitions with default values
int    exahype2::fv::particles::forces::_forceMethod      = 0;      // Default: no forces
double exahype2::fv::particles::forces::_mu               = 10.0;   // Default gravitational constant
double exahype2::fv::particles::forces::_gravityMagnitude = 9.81;   // Default Earth gravity
double exahype2::fv::particles::forces::_dynamicViscosity = 1.0e-3; // Default water viscosity at 20°C
double exahype2::fv::particles::forces::_fluidDensity     = 1.0;    // Default water density

// Force method setters
void exahype2::fv::particles::forces::setForceMethod(ForceType forces) { _forceMethod = static_cast<int>(forces); }

void exahype2::fv::particles::forces::setForceMethod(int forceMethod) { _forceMethod = forceMethod; }

void exahype2::fv::particles::forces::enableForce(ForceType force) { _forceMethod |= static_cast<int>(force); }

void exahype2::fv::particles::forces::disableForce(ForceType force) { _forceMethod &= ~static_cast<int>(force); }

bool exahype2::fv::particles::forces::isForceEnabled(ForceType force) {
  return (_forceMethod & static_cast<int>(force)) == static_cast<int>(force);
}

void exahype2::fv::particles::forces::disableAllForces() { _forceMethod = 0; }

// Parameter setters
void exahype2::fv::particles::forces::setGravitationalParameter(double mu) { _mu = mu; }

void exahype2::fv::particles::forces::setGravityMagnitude(double g) { _gravityMagnitude = g; }

void exahype2::fv::particles::forces::setDynamicViscosity(double viscosity) { _dynamicViscosity = viscosity; }

void exahype2::fv::particles::forces::setDefaultFluidDensity(double density) { _fluidDensity = density; }

void exahype2::fv::particles::forces::
  setAllParameters(int forceMethod, double mu, double g, double viscosity, double density) {
  _forceMethod      = forceMethod;
  _mu               = mu;
  _gravityMagnitude = g;
  _dynamicViscosity = viscosity;
  _fluidDensity     = density;
}
