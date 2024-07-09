// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

#include "ParticleIdentifier.h"

toolbox::particles::assignmentchecks::internal::ParticleIdentifier::
  ParticleIdentifier(
    const std::string&                           particleName__,
    const tarch::la::Vector<Dimensions, double>& particleX__,
    const int                                    particleID__
  ):
  particleName(particleName__),
  particleX(particleX__),
  particleID(particleID__) {}

toolbox::particles::assignmentchecks::internal::ParticleSearchIdentifier::
  ParticleSearchIdentifier(
    const std::string&                           particleName__,
    const tarch::la::Vector<Dimensions, double>& particleX__,
    const int                                    particleID__,
    const double                                 positionTolerance__
  ):
  ParticleIdentifier(particleName__, particleX__, particleID__),
  positionTolerance(positionTolerance__) {}


bool toolbox::particles::assignmentchecks::internal::ParticleIdentifier::
  numericalEquals(const ParticleIdentifier& rhs) const {

  return (particleName == rhs.particleName) and (particleID == rhs.particleID)
         and tarch::la::equals(particleX, rhs.particleX, 0.);
}


bool toolbox::particles::assignmentchecks::internal::ParticleIdentifier::
  numericalSmaller(const ParticleIdentifier& rhs) const {
  if (*this == rhs) {
    return false;
  }
  if (particleName < rhs.particleName) {
    return true;
  }
  if (particleName > rhs.particleName) {
    return false;
  }
  if (particleID < rhs.particleID)
    return true;
  if (particleID > rhs.particleID)
    return false;
  for (int d = 0; d < Dimensions; d++) {
    if (particleX(d) < rhs.particleX(d)) {
      return true;
    }
    if (particleX(d) > rhs.particleX(d)) {
      return false;
    }
  }
  // If everthing is equal so far, then one is not smaller than the oter.
  return false;
}

bool toolbox::particles::assignmentchecks::internal::ParticleIdentifier::
  numericalSmallerWithTolerance(
    const ParticleIdentifier& rhs,
    const double              tolerance
  ) const {

  if (particleName < rhs.particleName) {
    return true;
  }
  if (particleName > rhs.particleName) {
    return false;
  }
  if (particleID < rhs.particleID)
    return true;
  if (particleID > rhs.particleID)
    return false;
  for (int d = 0; d < Dimensions; d++) {
    if ((particleX(d) - rhs.particleX(d)) < -tolerance) {
      return true;
    }
  }
  // If everthing is equal so far, then one is not smaller than the oter.
  return false;
}


std::string toolbox::particles::assignmentchecks::internal::ParticleIdentifier::
  toString() const {
  return "(" + particleName + ",ID=" + std::to_string(particleID) + ","
         + ::toString(particleX) + ")";
}


bool toolbox::particles::assignmentchecks::internal::ParticleIdentifier::
  operator<(
    const toolbox::particles::assignmentchecks::internal::ParticleIdentifier&
      rhs
  ) const {
  return numericalSmaller(rhs);
}


bool toolbox::particles::assignmentchecks::internal::ParticleSearchIdentifier::
  operator<(
    const toolbox::particles::assignmentchecks::internal::ParticleIdentifier&
      rhs
  ) const {
  return numericalSmallerWithTolerance(rhs, positionTolerance);
}

bool toolbox::particles::assignmentchecks::internal::ParticleIdentifier::
  operator<(
    const toolbox::particles::assignmentchecks::internal::
      ParticleSearchIdentifier& rhs
  ) const {
  return numericalSmallerWithTolerance(rhs, rhs.positionTolerance);
}

bool operator==(
  const toolbox::particles::assignmentchecks::internal::ParticleIdentifier& lhs,
  const toolbox::particles::assignmentchecks::internal::ParticleIdentifier& rhs
) {
  return lhs.numericalEquals(rhs);
}

bool operator!=(
  const toolbox::particles::assignmentchecks::internal::ParticleIdentifier& lhs,
  const toolbox::particles::assignmentchecks::internal::ParticleIdentifier& rhs
) {
  return not lhs.numericalEquals(rhs);
}
