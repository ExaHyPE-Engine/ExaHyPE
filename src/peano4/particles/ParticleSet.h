// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/particles/SieveParticles.h"
#include "tarch/mpi/Collectives.h"

namespace peano4::particles {
  template <typename T>
  class ParticleSet;
} // namespace peano4::particles

/**
 * Collects the sort statistics and the sieve particles of one particle type.
 *
 * The counters and the sieve set are static, so every tree of a rank adds into
 * the same data.
 */
template <typename T>
class peano4::particles::ParticleSet {
public:
  typedef T             DoFType;
  typedef std::list<T*> ParticleList;

#if PEANO_DEBUG
  void                                  setDebugX(const tarch::la::Vector<DIMENSIONS, double>& data);
  void                                  setDebugH(const tarch::la::Vector<DIMENSIONS, double>& data);
  tarch::la::Vector<DIMENSIONS, double> getDebugX() const;
  tarch::la::Vector<DIMENSIONS, double> getDebugH() const;
#endif

  std::string toString(bool addBrackets = true) const;

  static void clearReassignmentStatistics();

  static void clearParticleStateStatistics();

  static void        reduceParticleStateStatistics();
  static std::string printParticleStateStatistics();

  static void reduceNumberOfParticles();
  static int  getTotalNumberOfParticles();

  static void        reduceReassignmentStatistics();
  static std::string printReassignmentStatistics();

  static void updateLiftDropStatistics(
    int numberOfLiftsInThisTree,
    int numberOfDropsInThisTree,
    int numberOfReassignmentsInThisTree,
    int numberOfLiftsIntoSieveSetInThisTree,
    int numberOfDropsFromSieveSetInThisTree,
    int numberOfDropsIntoHorizontalTreeDecomposition
  );

  static void updateNumberOfLocalAndExpiredParticles(
    int                                 numberOfRemainingLocalParticles,
    int                                 numberOfExpiredHaloParticles,
    int                                 numberOfParticlesThatHaveLeftTheirDomain,
    const std::set<std::array<int, 2>>& remainingLocalParticleNumbers = {}
  );

  static int getNumberOfDropsIntoHorizontalTreeDecomposition();

  static bool registeredAnyResorting();

  static void finishedTraversal(
    const tarch::la::Vector<DIMENSIONS, double> domainOffset,
    const tarch::la::Vector<DIMENSIONS, double> domainSize,
    const std::bitset<DIMENSIONS>               periodicBC
  );

  static SieveParticles<T> cloneParticlesToBeSieved();

  static ParticleList getParticlesToBeSievedIntoVertex(const peano4::dataview::VertexMarker& marker);
  static bool         hasParticlesToBeSievedIntoVertices();

  static int getNumberOfParticlesThatHaveBeSievedIntoVertices();

  static int getNumberOfRemainingLocalParticles();

protected:
  static std::mutex _statisticsMutex;

  // Reassignment counters accumulated by updateLiftDropStatistics().
  static int _numberOfLifts;
  static int _numberOfDrops;
  static int _numberOfReassignments;
  static int _numberOfLiftsIntoSieveSet;
  static int _numberOfDropsFromSieveSet;
  static int _numberOfDropsIntoHorizontalTreeDecomposition;

  // Particle-state counters accumulated by updateNumberOfLocalAndExpiredParticles().
  static int _numberOfRemainingLocalParticles;

  /**
   * Identity of every local particle of the traversal, given as the pair of
   * inserting tree and tree-local particle number. This set holds one entry per
   * particle, while _numberOfRemainingLocalParticles counts one per vertex copy.
   */
  static std::set<std::array<int, 2>> _remainingLocalParticleNumbers;

  static int _numberOfRemainingGlobalParticles;
  static int _numberOfExpiredHaloParticles;
  static int _numberOfParticlesThatHaveLeftTheirDomain;

  // Boundary-merge counters.
  static std::atomic<int> _numberOfDroppedIncomingVirtualParticles;
  static std::atomic<int> _numberOfRedundantlyOwnedLocalParticles;
  static std::atomic<int> _numberOfReplacedVirtualParticlesAlongBoundary;
  static std::atomic<int> _numberOfAddedVirtualParticlesAlongBoundary;
  static std::atomic<int> _numberOfAddedLocalParticlesAlongBoundary;

  // Persistent particles awaiting reassignment to a mesh vertex.
  static SieveParticles<T> _sieveParticles;

#if PEANO_DEBUG
  tarch::la::Vector<DIMENSIONS, double> _debugX;
  tarch::la::Vector<DIMENSIONS, double> _debugH;
#endif
};

#include "peano4/particles/ParticleSet.cpph"
