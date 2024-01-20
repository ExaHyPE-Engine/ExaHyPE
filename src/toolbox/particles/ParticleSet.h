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
 *
 *
 * ## Rationale: static attributes vs static aggregation
 *
 * There's a problem here however: Most of these fields are static fields.
 * If we have them static, they are tied
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
  typedef T             DoFType;
  typedef std::list<T*> ParticleList;

  using SieveParticles = SieveParticles<T>;

#if PeanoDebug >= 1
  void                                  setDebugX(const tarch::la::Vector<Dimensions, double>& data);
  void                                  setDebugH(const tarch::la::Vector<Dimensions, double>& data);
  tarch::la::Vector<Dimensions, double> getDebugX() const;
  tarch::la::Vector<Dimensions, double> getDebugH() const;
#endif

  std::string toString(bool addBrackets = true) const;

  /**
   * Clear stats
   *
   * Has to be called per rank prior to any collection of stats.
   * I used to call this routine in the prepare traversal event, i.e. in
   * peano4.toolbox.particles.api.AbstractUpdateParticleGridAssociation.get_body_of_prepareTraversal().
   * However, this is not clever, as the reassignment usually spans multiple
   * grid sweeps. If we clear the stats prior to each of these sweeps, we
   * will only get a state snapshot from what happened in the last sweep.
   *
   * Therefore, I now typically call this clear routine directly after I've
   * written stats to the terminal. This differs to how I treat
   * clearParticleStateStatistics().
   */
  static void clearReassignmentStatistics();

  /**
   * Clear stats
   *
   * Has to be called per rank prior to any mesh traversal assembling stats.
   * I call this routine in the prepare traversal event, i.e. in
   * peano4.toolbox.particles.UpdateParallelState.get_body_of_prepareTraversal().
   * This means I really get only a snapshot from one grid sweep.
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
    int numberOfLiftsIntoSieveSetInThisTree,
    int numberOfDropsFromSieveSetInThisTree,
    int numberOfSkippedDropsFromSieveSetInThisTree
  );

  static void updateNumberOfLocalAndExpiredParticles(int numberOfRemainingLocalParticles, int numberOfExpiredHaloParticles, int numberOfLeavingLocalParticles);


#ifdef Parallel
  static void initDatatype();

  static void shutdownDatatype();
#endif

  /**
   * Invoke exchangeSieveListsGlobally().
   */
  static void finishedTraversal();

  /**
   * Make routine from SieveParticles public
   *
   * Delegate through. This way, we don't have to expose the actual
   * sieve object. Usually combined with hasParticlesToBeSievedIntoVertices()
   * as guard, which makes the querying faster.
   *
   * @see SieveParticles::getParticlesToBeSievedIntoVertex() for parameter
   *   descriptions.
   */
  static ParticleList getParticlesToBeSievedIntoVertex(const peano4::datamanagement::VertexMarker& marker, int originatingTree, int callingTree);

  /**
   * Make routine from SieveParticles public
   *
   * Delegate through. This way, we don't have to expose the actual
   * sieve object. This predicate is usually always used as guard before
   * actually invoking getParticlesToBeSievedIntoVertex().
   *
   * @param originatingTree Either a number associated with a local tree or
   *                        SieveParticles<T>::GlobalSort.
   */
  static bool hasParticlesToBeSievedIntoVertices(int originatingTree);

  /**
   * Return total number of remaining particles
   *
   * Should be invoked on the global master rank only. After each
   * reduceAndPrintStatistics() call, we memorise this quantity and
   * subsequently return it.
   */
  static int getGlobalNumberOfLocalParticles();
  static int getGlobalNumberOfLeavingParticles();
  static int getGlobalNumberOfExpiredHaloParticles();
  static int getGlobalNumberOfNewIncomingParticles();
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
   *
   * See class documentation of toolbox::particles::ParticleSet for
   * an overview of the statistics tracked.
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
   *
   * See class documentation of toolbox::particles::ParticleSet for
   * an overview of the statistics tracked.
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
   *
   * See class documentation of toolbox::particles::ParticleSet for
   * an overview of the statistics tracked.
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
   *
   * See class documentation of toolbox::particles::ParticleSet for
   * an overview of the statistics tracked.
   */
  static int _numberOfLiftsIntoSieveSet;

  static int _numberOfDropsFromSieveSet;
  static int _numberOfSkippedDropsFromSieveSet;

  /**
   * Set by parallel state analysis
   *
   * See class documentation of toolbox::particles::ParticleSet for
   * an overview of the statistics tracked.
   */
  static int _numberOfRemainingLocalParticles;
  static int _numberOfExpiredHaloParticles;
  static int _numberOfLeavingLocalParticles;

  /**
   * Number of incoming halo particles which are not worked in
   *
   * Dropped incoming virtual particles "happen" when a neighbour sends in a
   * particle which is already virtual on the neighbour. In this case, the
   * neighbour hasn't updated this particle at all (has no ownership).
   * Therefore, we can just ignore it.
   *
   * Updated by boundary merge operations.
   */
  static int _numberOfDroppedIncomingVirtualParticles;
  static int _numberOfIncomingParticlesWhichWerePreviouslyKnownAsVirtual;

  /**
   * Particles which are local here and local somewhere else
   *
   * This happens if a particle resides almost exactly on the face between two
   * ranks and we therefore decide that both call it a local particle.
   *
   * Updated by boundary merge operations.
   */
  static int _numberOfRedundantlySharedLocalParticles;

  /**
   * Replace virtual particles are those where we have a copy and receive an update
   *
   * Virtual particles are owned by someone else. So whenever this owner sends
   * in an update, we throw away our local copy and use this updated data.
   *
   * Updated by boundary merge operations.
   */
  static int _numberOfReplacedVirtualParticles;

  /**
   * Newly "discovered" virtual particles
   *
   * If a particle is far away from "our" boundary and then approaches this
   * boundary on the neighbouring rank, it will, at one point, become visible
   * here and we add it to our set of virtual particles.
   *
   * Updated by boundary merge operations.
   */
  static int _numberOfAddedVirtualParticles;

  /**
   * Stats on duplicate elimination
   *
   * This one is updated internally by addParticles() which you
   * might call with duplicate detection or not.
   *
   * See class documentation of toolbox::particles::ParticleSet for
   * an overview of the statistics tracked.
   */
  static int _numberOfVirtualEliminatedSpatialDuplicates;
  static int _numberOfLocalEliminatedSpatialDuplicates;

  /**
   * Map of persistent particles that have to be sieved
   */
  static SieveParticles _sieveParticles;

#if PeanoDebug >= 1
  tarch::la::Vector<Dimensions, double> _debugX;
  tarch::la::Vector<Dimensions, double> _debugH;
#endif

#if PeanoDebug >= 1 and !defined(Parallel) and !defined(SharedMemoryParallelisation)
  /**
   * Counter to ensure no particles get lost
   *
   * We reset the particle counter in each clearParticleStateStatistics().
   * Therefore, we can each (prior to the reset) if the total particle count
   * is still the same. However, this works if and only if no parallelisation
   * is switched on. As soon as the parallelisation kicks in, particles can be
   * hosted redundantly and travel, so counting the number of local/real
   * particles is not that trivial anymore.
   */
  static int _numberOfParticlesInLastClear;
#endif

  static int _globalNumberOfRemainingLocalParticles;
  static int _globalNumberOfExpiredHaloParticles;
  static int _globalNumberOfLeavingLocalParticles;
  static int _globalNumberOfNewIncomingParticles;

};


#include "ParticleSet.cpph"
