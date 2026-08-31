// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

#include "ParticleIdentifier.h"

toolbox::particles::assignmentchecks::internal::ParticleIdentifier::ParticleIdentifier(
  const std::string&                           particleName__,
  const tarch::la::Vector<DIMENSIONS, double>& particleX__,
  const tarch::la::Vector<DIMENSIONS, double>& vertexX,
  const tarch::la::Vector<DIMENSIONS, double>& vertexH,
  const int                                    particleId__,
  const int                                    treeId__
) :
  particleName(particleName__),
  particleX(particleX__),
  vertexInd(getVertexInd(vertexX, vertexH)),
  vertexDepth(getVertexDepth(vertexX, vertexH)),
  particleId(particleId__),
  treeId(treeId__) {}

toolbox::particles::assignmentchecks::internal::ParticleIdentifier::ParticleIdentifier(
  const std::string&                              particleName__,
  const tarch::la::Vector<DIMENSIONS, double>&    particleX__,
  const tarch::la::Vector<DIMENSIONS, long long>& vertexInd__,
  const int                                       vertexDepth__,
  const int                                       particleId__,
  const int                                       treeId__
) :
  particleName(particleName__),
  particleX(particleX__),
  vertexInd(vertexInd__),
  vertexDepth(vertexDepth__),
  particleId(particleId__),
  treeId(treeId__) {}

/**
 * Returns the index of the vertex at position vertexX with size vertexH. The
 * quotient of the two is rounded to twelve significant digits.
 */
tarch::la::Vector<DIMENSIONS, long long> toolbox::particles::assignmentchecks::internal::ParticleIdentifier::
  getVertexInd(
    const tarch::la::Vector<DIMENSIONS, double>& vertexX,
    const tarch::la::Vector<DIMENSIONS, double>& vertexH
  ) {
  tarch::la::Vector<DIMENSIONS, long long> ind;

  for (size_t i = 0; i < DIMENSIONS; i++) {
    const double index = std::round(vertexX(i) / vertexH(i) * 1e12) * 1e-12;
    ind(i)             = static_cast<long long>(index);
  }

  return ind;
}

/**
 * Returns the depth of the vertex with size vertexH. The domain size is a
 * constant of this function, and an assertion fires when vertexX lies outside
 * that domain.
 */
int toolbox::particles::assignmentchecks::internal::ParticleIdentifier::getVertexDepth(
  const tarch::la::Vector<DIMENSIONS, double>& vertexX,
  const tarch::la::Vector<DIMENSIONS, double>& vertexH
) {
  constexpr double              BoxSize = 243.;
  [[maybe_unused]] const double maxX    = tarch::la::normMax(vertexX);
  assertion(maxX < BoxSize, maxX, BoxSize, "Hardcoded boxsize too small.");
  const double maxH   = tarch::la::normMax(vertexH);
  const int    result = static_cast<int>(BoxSize / maxH);
  return result;
}

bool toolbox::particles::assignmentchecks::internal::ParticleIdentifier::numericalEquals(
  const ParticleIdentifier& rhs
) const {
  for (size_t i = 0; i < DIMENSIONS; i++) {
    if (vertexInd(i) != rhs.vertexInd(i)) {
      return false;
    }
  }

  return (particleName == rhs.particleName) and (particleId == rhs.particleId) and (treeId == rhs.treeId)
    and (vertexDepth == rhs.vertexDepth);
}

/**
 * Returns true if this identifier orders before rhs. The comparison runs to
 * the last member, as an equality check in between ends it too early.
 */
bool toolbox::particles::assignmentchecks::internal::ParticleIdentifier::numericalSmaller(
  const ParticleIdentifier& rhs
) const {
  if (*this == rhs) {
    return false;
  }

  if (particleName < rhs.particleName) {
    return true;
  }
  if (particleName > rhs.particleName) {
    return false;
  }

  if (particleId < rhs.particleId) {
    return true;
  }
  if (particleId > rhs.particleId) {
    return false;
  }

  if (treeId < rhs.treeId) {
    return true;
  }
  if (treeId > rhs.treeId) {
    return false;
  }

  for (int d = 0; d < DIMENSIONS; d++) {
    if (vertexInd(d) < rhs.vertexInd(d)) {
      return true;
    }
    if (vertexInd(d) > rhs.vertexInd(d)) {
      return false;
    }
  }

  if (vertexDepth < rhs.vertexDepth) {
    return true;
  }
  if (vertexDepth > rhs.vertexDepth) {
    return false;
  }

  return false;
}

std::string toolbox::particles::assignmentchecks::internal::ParticleIdentifier::toString() const {
  std::stringstream sstr;
  sstr << "(" << particleName << ",Id=" << particleId << ",VertexInd=" << ::toString(vertexInd)
       << ",VertexDepth=" << vertexDepth << ",treeId=" << treeId << ",particleX=" << ::toString(particleX) << ")";

  return sstr.str();
}

bool toolbox::particles::assignmentchecks::internal::ParticleIdentifier::operator<(
  const toolbox::particles::assignmentchecks::internal::ParticleIdentifier& rhs
) const {
  return numericalSmaller(rhs);
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
