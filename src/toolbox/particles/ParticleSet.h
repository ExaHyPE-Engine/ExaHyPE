// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include <list>

#include "SieveParticles.h"
#include "tarch/multicore/BooleanSemaphore.h"

namespace toolbox {
  namespace particles {
    template <typename T>
    class ParticleSet;
  } // namespace particles
} // namespace toolbox


/**
 * Abstract base class for all flavours of particle sets
 *
 * Particle sets differ in the way they actually manage the particles. However,
 * they all share common stats, operations, ... This is covered by this class.
 *
 * There's a problem here however: Most of these fields are static fields and
 * hence cannot be modelled as static. If we have them static, they are tied
 * to the base object, not the the particular particle type. However, the fact
 * that we work with templates rides to our rescue. We can assume that this
 * base class is instantiated per particle type. Therefore, all of its static
 * fields are unique again. Along these discussions, this base class is more of
 * an aspect rather than a real base class. The number of real functions and
 * attributes is negligible.
 *
 * An alternative implementation would make this a real class and let the
 * actual particles sets holds a static attribute of this class. I think that
 * would be a valid implementation, too.
 *
 * Eventually, I ended up with a compromise, where the sieving is delegated
 * into a class of its own called toolbox::particles::SieveParticles. The
 * rationale here is simple:
 *
 * All the stuff that has to do with the management of the sieved particles is
 * outsourced into a separate class. All the stats, initialisation and whatever
 * generic stuff is out there is collocated in this very routine.
 *
 */
template <typename T>
class toolbox::particles::ParticleSet {
public:
  typedef T                 DoFType;
  typedef std::list<T*>     ParticleList;
  typedef SieveParticles<T> SieveParticles;

#if PeanoDebug >= 1
  void                                  setDebugX(const tarch::la::Vector<Dimensions, double>& data);
  void                                  setDebugH(const tarch::la::Vector<Dimensions, double>& data);
  tarch::la::Vector<Dimensions, double> getDebugX() const;
  tarch::la::Vector<Dimensions, double> getDebugH() const;
#endif

  /**
   * Clear stats
   *
   * Has to be called per rank prior to any mesh traversal assembling stats.
   * I typically realise this in the prepare traversal event. See
   * peano4.toolbox.particles.AbstractUpdateParticleGridAssociation.get_body_of_prepareTraversal().
   */
  static void clearReassignmentStatistics();

  /**
   * Clear stats
   *
   * Has to be called per rank prior to any mesh traversal assembling stats.
   * I typically realise this in the prepare traversal event. See
   * peano4.toolbox.particles.UpdateParallelState.get_body_of_prepareTraversal().
   */
  static void clearParticleStateStatistics();

  /**
   * Reduces and prints the global statistics
   */
  static void reduceAndPrintStatistics();

  static void updateLiftDropStatistics(
    int numberOfLiftsInThisTree,
    int numberOfDropsInThisTree,
    int numberOfReassignmentsInThisTree,
    int numberOfSievesInThisTree
  );

  static void updateNumberOfLocalParticleStatistics(
    int numberOfRemainingLocalParticles, int numberOfIncomingLocalParticles, int numberOfLeavingLocalParticles
  );

#ifdef Parallel
  static void initDatatype();

  static void shutdownDatatype();
#endif

  /**
   * Invoke exchangeSieveListsGlobally().
   */
  static void finishedTraversal();

  /**
   * Delegate through. This way, we don't have to expose the actual
   * sieve object.
   */
  static ParticleList getParticlesToBeSievedIntoVertex(const peano4::datamanagement::VertexMarker& marker);

  static bool hasParticlesToBeSievedIntoVertices();

  //    static ParticleList getSievedParticlesForNextGridTraversal();
private:
  static tarch::logging::Log _log;

protected:
  static tarch::multicore::BooleanSemaphore _statisticsSemaphore;

  /**
   * Sorting statistics
   *
   * Used by sorting algorithm of choice. The attribute is updated
   * through updateLiftDropStatistics() by each and every tree
   * traversal. The five fields should be cleared through a call of
   * clearReassignmentStatistics() at the begin of a time step, i.e.
   * the first grid sweep starting a time step.
   */
  static int _numberOfLifts;

  /**
   * Sorting statistics
   *
   * Used by sorting algorithm of choice. The attribute is updated
   * through updateLiftDropStatistics() by each and every tree
   * traversal. The five fields should be cleared through a call of
   * clearReassignmentStatistics() at the begin of a time step, i.e.
   * the first grid sweep starting a time step.
   */
  static int _numberOfDrops;

  /**
   * Sorting statistics
   *
   * Used by sorting algorithm of choice. The attribute is updated
   * through updateLiftDropStatistics() by each and every tree
   * traversal. The five fields should be cleared through a call of
   * clearReassignmentStatistics() at the begin of a time step, i.e.
   * the first grid sweep starting a time step.
   */
  static int _numberOfReassignments;

  /**
   * Sorting statistics
   *
   * Used by sorting algorithm of choice. The attribute is updated
   * through updateLiftDropStatistics() by each and every tree
   * traversal. The five fields should be cleared through a call of
   * clearReassignmentStatistics() at the begin of a time step, i.e.
   * the first grid sweep starting a time step.
   */
  static int _numberOfSieves;

  /**
   * Set by parallel state analysis
   */
  static int _numberOfRemainingLocalParticles;
  static int _numberOfIncomingLocalParticles;
  static int _numberOfLeavingLocalParticles;

  /**
   * Number of updated halo particles
   *
   * Updated by boundary merge operations.
   *
   * @see merge()
   */
  static int _numberOfAddedVirtualParticles;
  static int _numberOfReplacedVirtualParticles;
  static int _numberOfDroppedVirtualParticles;

  /**
   * Stats on duplicate elimination
   *
   * This one is updated internally by eliminateSpatialDuplicates() which you
   * might call or you might not call. It is never updated anywhere else.
   */
  static int _numberOfVirtualEliminatedSpatialDuplicates;
  static int _numberOfLocalEliminatedSpatialDuplicates;

  static SieveParticles sieveParticles;

#if PeanoDebug >= 1
  tarch::la::Vector<Dimensions, double> _debugX;
  tarch::la::Vector<Dimensions, double> _debugH;
#endif
};


#include "ParticleSet.cpph"
