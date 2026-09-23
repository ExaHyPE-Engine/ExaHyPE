// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ParticleFactory.h"

/**
 * Draws a thread-local coordinate perturbation from [-0.5, 0.5).
 */
double peano4::particles::drawParticlePositionNoise() {
  thread_local std::mt19937                           randomEngine(std::random_device{}());
  thread_local std::uniform_real_distribution<double> distribution(0.0, 1.0);
  return distribution(randomEngine) - 0.5;
}
