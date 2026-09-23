// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ParticleForces.h"

int    exahype2::fv::particles::forces::globalForceMethod            = 0;
double exahype2::fv::particles::forces::globalGravitationalParameter = 10.0;
double exahype2::fv::particles::forces::globalGravityMagnitude       = 9.81;
double exahype2::fv::particles::forces::globalDynamicViscosity       = 1.0e-3;

void exahype2::fv::particles::forces::setForceMethod(const ForceType forces) {
  exahype2::fv::particles::forces::globalForceMethod = static_cast<int>(forces);
}

void exahype2::fv::particles::forces::setForceMethod(const int forceMethod) {
  exahype2::fv::particles::forces::globalForceMethod = forceMethod;
}

void exahype2::fv::particles::forces::enableForce(const ForceType force) {
  exahype2::fv::particles::forces::globalForceMethod |= static_cast<int>(force);
}

void exahype2::fv::particles::forces::disableForce(const ForceType force) {
  exahype2::fv::particles::forces::globalForceMethod &= ~static_cast<int>(force);
}

void exahype2::fv::particles::forces::disableAllForces() { exahype2::fv::particles::forces::globalForceMethod = 0; }

void exahype2::fv::particles::forces::setGravitationalParameter(const double gravitationalParameter) {
  exahype2::fv::particles::forces::globalGravitationalParameter = gravitationalParameter;
}

void exahype2::fv::particles::forces::setGravityMagnitude(const double gravityMagnitude) {
  exahype2::fv::particles::forces::globalGravityMagnitude = gravityMagnitude;
}

void exahype2::fv::particles::forces::setDynamicViscosity(const double dynamicViscosity) {
  exahype2::fv::particles::forces::globalDynamicViscosity = dynamicViscosity;
}

void exahype2::fv::particles::forces::setAllParameters(
  const int    forceMethod,
  const double gravitationalParameter,
  const double gravityMagnitude,
  const double dynamicViscosity
) {
  exahype2::fv::particles::forces::globalForceMethod            = forceMethod;
  exahype2::fv::particles::forces::globalGravitationalParameter = gravitationalParameter;
  exahype2::fv::particles::forces::globalGravityMagnitude       = gravityMagnitude;
  exahype2::fv::particles::forces::globalDynamicViscosity       = dynamicViscosity;
}
