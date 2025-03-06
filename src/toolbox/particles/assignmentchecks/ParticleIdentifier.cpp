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

// if (this->particleID == -1 or rhs.particleID == -1 ) {
std::cout << "In numericalEquals this=" << this->toString() << " rhs=" << rhs.toString()
<< " res=" <<
  ((particleName == rhs.particleName) and (particleID == rhs.particleID) and tarch::la::equals(particleX, rhs.particleX, 0.))
  << std::endl;
// }
  return (particleName == rhs.particleName) and (particleID == rhs.particleID)
         and tarch::la::equals(particleX, rhs.particleX, 0.);
}


bool toolbox::particles::assignmentchecks::internal::ParticleIdentifier::
  numericalSmaller(const ParticleIdentifier& rhs) const {

// if (particleID == -1 or rhs.particleID == -1 ) {
std::cout << "In numericalSmaller this=" << this->toString() << " rhs=" << rhs.toString();
  std::cout
    <<" a=" << (*this == rhs)
    <<" b=" << (particleName < rhs.particleName)
    << " c=" << (particleName > rhs.particleName)
    << " d=" << (particleID < rhs.particleID)
    << " e=" << (particleID > rhs.particleID)
    << " f0=" << (particleX(0) < rhs.particleX(0))
    << " f1=" << (particleX(1) < rhs.particleX(1))
    << " g0=" << (particleX(0) > rhs.particleX(0))
    << " g1=" << (particleX(1) > rhs.particleX(1))
      << std::endl;
// }

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
  // If everthing is equal so far, then one is not smaller than the other.
  return false;
}


bool toolbox::particles::assignmentchecks::internal::ParticleIdentifier::
  numericalSmallerWithTolerance(
    const ParticleIdentifier& rhs,
    const double              tolerance
  ) const {

// if (particleID == -1 or rhs.particleID == -1 ) {
std::cout << "In NumericalSmallerWIthTolerance this=" << this->toString() << " rhs=" << rhs.toString();
  std::cout << " a=" << (particleName < rhs.particleName)
    << " b=" << (particleName > rhs.particleName)
    << " c=" << (particleID < rhs.particleID)
    << " d=" << (particleID > rhs.particleID)
    << " e0=" << (particleX(0) - rhs.particleX(0) < -tolerance)
    << " e1=" << (particleX(1) - rhs.particleX(1) < tolerance)
    << " f0=" << (particleX(0) - rhs.particleX(0) > -tolerance)
    << " f1=" << (particleX(1) - rhs.particleX(1) > tolerance)
     << std::endl;
// }

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
    if ((particleX(d) - rhs.particleX(d)) < -tolerance * 1e-6) {
      return true;
    }
    if ((particleX(d) - rhs.particleX(d)) > tolerance * 1e-6) {
      return false;
    }
  }
  // If everthing is equal so far, then one is not smaller than the other.
  return false;
}


std::string toolbox::particles::assignmentchecks::internal::ParticleIdentifier::
  toString() const {
  return "(" + particleName + ",ID=" + std::to_string(particleID) + ","
         + ::toString(particleX) + ")";
}


// This takes ParticleIdentifier as argument
bool toolbox::particles::assignmentchecks::internal::ParticleIdentifier::
  operator<(
    const toolbox::particles::assignmentchecks::internal::ParticleIdentifier&
      rhs
  ) const {
// if (particleID == -1 or rhs.particleID == -1 ) {
std::cout << "In operator< I/I this=" << this->toString() << " rhs=" << rhs.toString() << std::endl;
// }
  // return numericalSmaller(rhs);
  return numericalSmaller(rhs);
}


bool toolbox::particles::assignmentchecks::internal::ParticleSearchIdentifier::
  operator<(
    const toolbox::particles::assignmentchecks::internal::ParticleIdentifier& rhs
  ) const {
// if (particleID == -1 or rhs.particleID == -1 ) {
std::cout << "In operator< S/I this=" << this->toString() << " rhs=" << rhs.toString() << std::endl;
// }
  return numericalSmallerWithTolerance(rhs, positionTolerance);
}


// This takes Particle !Search! Identifier as argument
bool toolbox::particles::assignmentchecks::internal::ParticleIdentifier::
  operator<(
    const toolbox::particles::assignmentchecks::internal::
      ParticleSearchIdentifier& rhs
  ) const {
// if (particleID == -1 or rhs.particleID == -1 ) {
std::cout << "In operator< I/S this=" << this->toString() << " rhs=" << rhs.toString() << std::endl;
// }
  return numericalSmallerWithTolerance(rhs, rhs.positionTolerance);
}


bool operator==(
  const toolbox::particles::assignmentchecks::internal::ParticleIdentifier& lhs,
  const toolbox::particles::assignmentchecks::internal::ParticleIdentifier& rhs
) {
// if (lhs.particleID == -1 or rhs.particleID == -1 ) {
std::cout << "In operator== I/I this=" << lhs.toString() << " rhs=" << rhs.toString() << std::endl;
// }
  return lhs.numericalEquals(rhs);
}


bool operator!=(
  const toolbox::particles::assignmentchecks::internal::ParticleIdentifier& lhs,
  const toolbox::particles::assignmentchecks::internal::ParticleIdentifier& rhs
) {
// if (lhs.particleID == -1 or rhs.particleID == -1 ) {
std::cout << "In operator!= I/I this=" << lhs.toString() << " rhs=" << rhs.toString() << std::endl;
// }
  return not lhs.numericalEquals(rhs);
}
