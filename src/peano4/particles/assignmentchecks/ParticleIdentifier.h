// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"

namespace peano4::particles::assignmentchecks {
  struct ParticleIdentifier;
} // namespace peano4::particles::assignmentchecks

/**
 * Key of the map that stores the history of a particle. A periodic boundary
 * replicates a particle, so two particles carry the same name and the same
 * integer number, and the key adds the index and the depth of the vertex that
 * holds the particle plus the number of the spacetree.
 */
struct peano4::particles::assignmentchecks::ParticleIdentifier final {
  std::string                              particleName;
  tarch::la::Vector<DIMENSIONS, double>    particleX;
  tarch::la::Vector<DIMENSIONS, long long> vertexIndex;
  int                                      vertexDepth;
  int                                      particleId;
  int                                      treeId;

  [[nodiscard]] std::string toString() const;

  [[nodiscard]] bool numericalEquals(const ParticleIdentifier& rhs) const;
  [[nodiscard]] bool numericalSmaller(const ParticleIdentifier& rhs) const;

  bool operator<(const ParticleIdentifier& rhs) const;

  ParticleIdentifier(
    const std::string&                           particleName,
    const tarch::la::Vector<DIMENSIONS, double>& particleX,
    const tarch::la::Vector<DIMENSIONS, double>& vertexX,
    const tarch::la::Vector<DIMENSIONS, double>& vertexH,
    int                                          particleId,
    int                                          treeId
  );

  ParticleIdentifier(
    const std::string&                              particleName,
    const tarch::la::Vector<DIMENSIONS, double>&    particleX,
    const tarch::la::Vector<DIMENSIONS, long long>& vertexIndex,
    int                                             vertexDepth,
    int                                             particleId,
    int                                             treeId
  );

  static tarch::la::Vector<DIMENSIONS, long long> getVertexIndex(
    const tarch::la::Vector<DIMENSIONS, double>& vertexX,
    const tarch::la::Vector<DIMENSIONS, double>& vertexH
  );

  static int getVertexDepth(
    const tarch::la::Vector<DIMENSIONS, double>& vertexX,
    const tarch::la::Vector<DIMENSIONS, double>& vertexH
  );
};

bool operator==(
  const peano4::particles::assignmentchecks::ParticleIdentifier& lhs,
  const peano4::particles::assignmentchecks::ParticleIdentifier& rhs
);

bool operator!=(
  const peano4::particles::assignmentchecks::ParticleIdentifier& lhs,
  const peano4::particles::assignmentchecks::ParticleIdentifier& rhs
);
