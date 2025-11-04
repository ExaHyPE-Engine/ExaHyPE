// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ParticleIntegration.h"

// ================================================================================================
// Global Integration Settings
// ================================================================================================

/**
 * Default time step size used by integration methods.
 */
double exahype2::fv::particles::integration::_timeStepSize = 0.01;

/**
 * Default integration method:
 *   1 = Explicit Euler
 */
int exahype2::fv::particles::integration::_integrationMethod = 1;

/**
 * Set global parameters controlling particle integration behavior.
 *
 * @param timeStepSize       Time step size used in the integrator
 * @param integrationMethod  Method selector (see header for valid keys)
 */
void exahype2::fv::particles::integration::setGlobalIntegrationParameters(double timeStepSize, int integrationMethod) {
  _timeStepSize      = timeStepSize;
  _integrationMethod = integrationMethod;
}
