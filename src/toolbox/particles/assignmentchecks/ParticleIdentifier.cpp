// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

#include "ParticleIdentifier.h"

toolbox::particles::assignmentchecks::internal::ParticleIdentifier::
  ParticleIdentifier(
    const std::string&                           particleName__,
    const tarch::la::Vector<Dimensions, double>&    particleX__,
    const tarch::la::Vector<Dimensions, double>&    vertexX,
    const tarch::la::Vector<Dimensions, double>&    vertexH,
    const int                                       particleID__,
    const int                                       treeID__
  ):
  particleName(particleName__),
  particleX(particleX__),
  vertexInd(getVertexInd(vertexX, vertexH)),
  vertexDepth(getVertexDepth(vertexX, vertexH)),
  particleID(particleID__),
  treeID(treeID__) {}

toolbox::particles::assignmentchecks::internal::ParticleIdentifier::
  ParticleIdentifier(
    const std::string&                           particleName__,
    const tarch::la::Vector<Dimensions, double>&    particleX__,
    const tarch::la::Vector<Dimensions, long long>& vertexInd__,
    const int                                       vertexDepth__,
    const int                                       particleID__,
    const int                                       treeID__
  ):
  particleName(particleName__),
  particleX(particleX__),
  vertexInd(vertexInd__),
  vertexDepth(vertexDepth__),
  particleID(particleID__),
  treeID(treeID__) {}


tarch::la::Vector<Dimensions, long long> toolbox::particles::assignmentchecks::internal::ParticleIdentifier::getVertexInd(
            const tarch::la::Vector<Dimensions, double>& vertexX,
            const tarch::la::Vector<Dimensions, double>& vertexH
            ){

  tarch::la::Vector<Dimensions, long long>  ind;

  for (size_t i = 0; i<Dimensions; i++){
    // Round to 12 significant digits
    double indf = std::round(vertexX(i)/vertexH(i) * 1e12) * 1e-12;
    ind(i) = static_cast<long long>(indf);
  }

  return ind;
}


int toolbox::particles::assignmentchecks::internal::ParticleIdentifier::getVertexDepth(
            const tarch::la::Vector<Dimensions, double>& vertexX,
            const tarch::la::Vector<Dimensions, double>& vertexH
            ){

  constexpr double boxsize = 243.;

  double maxX = tarch::la::normMax(vertexX);
  assertion3(maxX < boxsize, maxX, boxsize, "Hardcoded bosize too small.");

  double maxH = tarch::la::normMax(vertexH);

  int result = static_cast<int>(boxsize / maxH);

  return result;
}


bool toolbox::particles::assignmentchecks::internal::ParticleIdentifier::
  numericalEquals(const ParticleIdentifier& rhs) const {

  for (size_t i = 0; i < Dimensions; i++){
    if (vertexInd(i) != rhs.vertexInd(i))
      return false;
  }

  return (particleName == rhs.particleName) and (particleID == rhs.particleID) and (treeID == rhs.treeID);
}


bool toolbox::particles::assignmentchecks::internal::ParticleIdentifier::
  numericalSmaller(const ParticleIdentifier& rhs) const {

  // DO NOT USE EQUALITY CHECKS IN HERE.
  // That aborts the checks too early.
  // Only return false at the very end.

  if (*this == rhs)
    return false;

  if (particleName < rhs.particleName)
    return true;
  if (particleName > rhs.particleName)
    return false;

  if (particleID < rhs.particleID)
    return true;
  if (particleID > rhs.particleID)
    return false;

  if (treeID < rhs.treeID)
    return true;
  if (treeID > rhs.treeID)
    return false;

  for (int d = 0; d < Dimensions; d++) {

    if (vertexInd(d) < rhs.vertexInd(d))
      return true;
    if (vertexInd(d) > rhs.vertexInd(d))
      return false;
  }

  if (vertexDepth < rhs.vertexDepth)
    return true;
  if (vertexDepth > rhs.vertexDepth)
    return true;

  // If everthing is equal so far, then one is not smaller than the other.
  return false;
}



std::string toolbox::particles::assignmentchecks::internal::ParticleIdentifier::
  toString() const {

  std::stringstream sstr;
  sstr << "(" << particleName <<
          ",ID=" <<particleID <<
          ",VertexInd=" << ::toString(vertexInd) <<
          ",treeID=" << treeID <<
          ",particleX=" << ::toString(particleX)
          << ")";

  return sstr.str();
  }


bool toolbox::particles::assignmentchecks::internal::ParticleIdentifier::
  operator<(
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
