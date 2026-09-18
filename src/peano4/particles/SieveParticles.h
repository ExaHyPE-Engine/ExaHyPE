// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/dataview/VertexMarker.h"
#include "peano4/particles/MultiscaleTransitions.h"
#include "tarch/Termination.h"
#include "tarch/instrumentation/Debug.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"

namespace peano4::particles {
  template <typename T>
  class SieveParticles;
} // namespace peano4::particles

/**
 * Owns the particles that await a global redistribution or an insertion into a
 * local vertex.
 */
template <typename T>
class peano4::particles::SieveParticles final {
public:
  using DoFType      = T;
  using ParticleList = std::list<T*>;

  SieveParticles() = default;

  // The destructor does not delete the particles; a caller calls deleteParticles() for that.
  ~SieveParticles() = default;

  void deleteParticles();

  void exchangeSieveListsGlobally(
    tarch::la::Vector<DIMENSIONS, double> domainOffset,
    tarch::la::Vector<DIMENSIONS, double> domainSize,
    std::bitset<DIMENSIONS>               periodicBoundaries
  );

  void addParticleThatCanNotBeLiftedWithinItsTree(T* particle);

  SieveParticles<T> cloneParticlesToBeSieved();

  ParticleList getParticlesToBeSievedIntoVertex(
    const peano4::dataview::VertexMarker& marker,
    bool                                  removeReturnedParticlesFromSet,
    bool                                  onlyReturnParticlesThatWillBeLocal,
    bool                                  lockMutex
  );

  [[nodiscard]] bool hasParticlesToBeSievedIntoVertices() const;

  [[nodiscard]] int getNumberOfParticlesThatHaveBeSievedIntoVertices() const;

  [[nodiscard]] std::string toString() const;

private:
  static void deleteParticles(ParticleList& list);

  /**
   * Guards the assembly of the particles that require a global data exchange.
   * A mutex cannot be copied, so the member is static, and a copy created
   * through cloneParticlesToBeSieved() passes lockMutex as false because it
   * holds thread-local data.
   */
  static std::mutex _particlesToBeSievedMutex;

  ParticleList _particlesThatCanNotBeLiftedWithinTheirTree;

  ParticleList _particlesThatHaveToBeSieved;
};

#include "peano4/particles/SieveParticles.cpph"
