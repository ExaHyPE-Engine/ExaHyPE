// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ParticleIdentifier.h"

#include "tarch/Assertions.h"

peano4::particles::assignmentchecks::ParticleIdentifier::ParticleIdentifier(
  const std::string&                           particleName,
  const tarch::la::Vector<DIMENSIONS, double>& particleX,
  const tarch::la::Vector<DIMENSIONS, double>& vertexX,
  const tarch::la::Vector<DIMENSIONS, double>& vertexH,
  const int                                    particleId,
  const int                                    treeId
) :
  particleName(particleName),
  particleX(particleX),
  vertexIndex(getVertexIndex(vertexX, vertexH)),
  vertexDepth(getVertexDepth(vertexX, vertexH)),
  particleId(particleId),
  treeId(treeId) {}

peano4::particles::assignmentchecks::ParticleIdentifier::ParticleIdentifier(
  const std::string&                              particleName,
  const tarch::la::Vector<DIMENSIONS, double>&    particleX,
  const tarch::la::Vector<DIMENSIONS, long long>& vertexIndex,
  const int                                       vertexDepth,
  const int                                       particleId,
  const int                                       treeId
) :
  particleName(particleName),
  particleX(particleX),
  vertexIndex(vertexIndex),
  vertexDepth(vertexDepth),
  particleId(particleId),
  treeId(treeId) {}

/**
 * Returns the index of the vertex at position vertexX with size vertexH. The
 * quotient of the two is rounded to twelve significant digits.
 */
tarch::la::Vector<DIMENSIONS, long long> peano4::particles::assignmentchecks::ParticleIdentifier::getVertexIndex(
  const tarch::la::Vector<DIMENSIONS, double>& vertexX,
  const tarch::la::Vector<DIMENSIONS, double>& vertexH
) {
  tarch::la::Vector<DIMENSIONS, long long> index;

  for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
    assertion(vertexH(dimension) > 0.0, vertexX, vertexH);
    const double roundedQuotient = std::round(vertexX(dimension) / vertexH(dimension) * 1e12) * 1e-12;
    index(dimension)             = static_cast<long long>(roundedQuotient);
  }

  return index;
}

/**
 * Returns the depth of the vertex with size vertexH. The domain size is a
 * constant of this function, and an assertion fires when vertexX lies outside
 * that domain.
 */
int peano4::particles::assignmentchecks::ParticleIdentifier::getVertexDepth(
  const tarch::la::Vector<DIMENSIONS, double>& vertexX,
  const tarch::la::Vector<DIMENSIONS, double>& vertexH
) {
  constexpr double              BoxSize = 243.0;
  [[maybe_unused]] const double maxX    = tarch::la::normMax(vertexX);
  assertion(maxX < BoxSize, "The vertex lies outside the box this routine assumes.", maxX, BoxSize);
  const double maxH = tarch::la::normMax(vertexH);
  assertion(maxH > 0.0, vertexH);
  return static_cast<int>(BoxSize / maxH);
}

bool peano4::particles::assignmentchecks::ParticleIdentifier::numericalEquals(const ParticleIdentifier& rhs) const {
  for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
    if (vertexIndex(dimension) != rhs.vertexIndex(dimension)) {
      return false;
    }
  }

  return (particleName == rhs.particleName) and (particleId == rhs.particleId) and (treeId == rhs.treeId)
    and (vertexDepth == rhs.vertexDepth);
}

/**
 * Returns whether this identifier orders before rhs. The comparison runs over
 * the particle name, the particle id, the tree id, the vertex index and the
 * vertex depth, in this order.
 */
bool peano4::particles::assignmentchecks::ParticleIdentifier::numericalSmaller(const ParticleIdentifier& rhs) const {
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
    if (vertexIndex(d) < rhs.vertexIndex(d)) {
      return true;
    }
    if (vertexIndex(d) > rhs.vertexIndex(d)) {
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

std::string peano4::particles::assignmentchecks::ParticleIdentifier::toString() const {
  std::ostringstream out;
  out << "(" << particleName << ",Id=" << particleId << ",VertexIndex=" << ::toString(vertexIndex)
      << ",VertexDepth=" << vertexDepth << ",treeId=" << treeId << ",particleX=" << ::toString(particleX) << ")";

  return out.str();
}

bool peano4::particles::assignmentchecks::ParticleIdentifier::operator<(
  const peano4::particles::assignmentchecks::ParticleIdentifier& rhs
) const {
  return numericalSmaller(rhs);
}

bool operator==(
  const peano4::particles::assignmentchecks::ParticleIdentifier& lhs,
  const peano4::particles::assignmentchecks::ParticleIdentifier& rhs
) {
  return lhs.numericalEquals(rhs);
}

bool operator!=(
  const peano4::particles::assignmentchecks::ParticleIdentifier& lhs,
  const peano4::particles::assignmentchecks::ParticleIdentifier& rhs
) {
  return not lhs.numericalEquals(rhs);
}
