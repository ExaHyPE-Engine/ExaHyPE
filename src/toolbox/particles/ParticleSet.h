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
 * ## Statistics output
 *
 * The particle species delivers a couple of generic stats which are explained
 * below
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * toolbox::particles::ParticleSet::reduceAndPrintStatistics()   subdomain-local sorting statistics: #lifts=0, #drops=0,
 * #sieves=0, #reassignments=0
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * Lift means that a particle was associated to a vertex but left the area
 * around this vertex. The resorting algorithm was not able or not configured
 * to assign it directly to its new owner vertex on the correct level and
 * therefore has pushed the particle to the next coarser level. A particle
 * can be pushed multiple times (if it is very fast and jumps through cells).
 * In this case - which is unlikely in many physical systems - it contributes
 * to the counter multiple times.
 *
 * Drop means that a particle was found on a certain level, but its search
 * radius was so small that the sorting decided to move it to the next finer
 * level.
 *
 * Whenever a tree decides that it cannot sort a particle locally, it removes
 * the particle from its local data structure and puts it into a global sieve
 * list. This sieve list is aggregated between all ranks and shared. In the
 * subsequent iteration, all the trees will sieve the particles into their
 * mesh if they can accommodate it. The sieve counter tells you how often this
 * was the case.
 *
 * Reassignments means that a particle has left the catchment area of its
 * vertex, but the code successfully reassigned it to a neighbouring vertex
 * on the same resolution level.
 *
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * toolbox::particles::ParticleSet::reduceAndPrintStatistics()   global sorting statistics:
 * #lifts(assignments)-into-sieve-set=0
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * This is the counterpart of of the sieve operation. Note that a particle
 * might be lifted into the sieve set once, but then multiple ranks/threads
 * might sieve it into their domain. This happens if the object ends up
 * exactly at the interface of two subdomains and hence is held redundantly.
 *
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * toolbox::particles::ParticleSet::reduceAndPrintStatistics()   particle state updates: #remaining=121, #incoming=0,
 * #leaving=0, #expired-and-dropped-halos=51
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * After each step where particles move, we might want to update their state.
 * These statistics summarise how many particles have stayed within their
 * local subdomain, i.e. within their tree, and how many had been merged into
 * the local domain and then we found out that they actually entered the domain.
 * Those are incoming particles. The other way round, we also track how many
 * particles have left our local domain.
 *
 * Finally, the statistics keep track of halo (virtual) particles. After each
 * step, we exchange a halo layer of particles. Next time we look at these
 * virtual particles, they might be totally outdated. Therefore, we throw them
 * away. They are expired.
 *
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * toolbox::particles::ParticleSet::reduceAndPrintStatistics()   boundary merge statistics: #added new virtual particles
 * (new halo particles)=51, #replaced virtual particles (halo particle updates)=0, #dropped virtual particles (redundant
 * halo information)=0
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * When we merge data sent in from another tree, we ever only consider data
 * that is marked as local on the other tree. We are only interested in updated
 * virtual data. The incoming data might be a new virtual particle, or we
 * might already know about this one, so we also don't use it. The latter
 * thing happens if a particle sits at the boundary of two partitions, i.e.
 * both update it concurrently. Now they might send this info to a third
 * partition which now has two copies of the halo data but should really only
 * host one.
 *
 * Usually, virtual copies on a rank expire after one step, are subsequently
 * dropped and hence not sent out. However, if a code decides not to drop a
 * particle, for whatever reason, then we still don't work it in here. We
 * however make a note in. This should usually not happen, i.e. the last
 * counter should be zero.
 *
 * Particles can move in touchVertexLastTime() and hence be marked as local
 * even though they have just left their domain. They will enter here as
 * virtual particles, but subsequently be reset to local by toolbox.particles.UpdateParallelState().
 * In this context, it is important to reiterate that this local/virtual
 * property really never should be used for some programming logic. Particle
 * codes should solely use the particles' position in space, i.e. are they
 * inside a cell, to make a decision if to update them or not.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * toolbox::particles::ParticleSet::reduceAndPrintStatistics()   replica deletion statistics: #eliminated local spatial
 * duplicates=0, #eliminated virtual spatial duplicates=0
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * Some codes explicitly check for redundant particles in teh mesh and remove
 * them. The last counters keep track of such redundant data. It is usually
 * suspicious if you have to many redundant particles. Some data might be
 * inconsistent. We have however seen redundant data when two subtrees for
 * example hold a local particle which sits exactly on the boundary between
 * their domains. Both then update the particle and make it fly into a third
 * domain via the sieve mechanism (as it is a very fast particle). In such a
 * case, particles can enter a domain multiple times.
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
   * peano4.toolbox.particles.AbstractUpdateParticleGridAssociation.get_body_of_prepareTraversal().
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
    int numberOfDropsFromSieveSetInThisTree
  );

  static void updateNumberOfLocalParticleStatistics(
    int numberOfRemainingLocalParticles,
    int numberOfIncomingLocalParticles,
    int numberOfLeavingLocalParticles,
    int numberOfExpiredHaloParticles
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
   * Make routine from SieveParticles public
   *
   * Delegate through. This way, we don't have to expose the actual
   * sieve object.
   */
  static ParticleList getParticlesToBeSievedIntoVertex(
    const peano4::datamanagement::VertexMarker& marker, int originatingTree = SieveParticles<T>::GlobalSort
  );

  /**
   * Make routine from SieveParticles public
   *
   * Delegate through. This way, we don't have to expose the actual
   * sieve object.
   */
  static bool hasParticlesToBeSievedIntoVertices(int originatingTree = SieveParticles<T>::GlobalSort);

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

  /**
   * Set by parallel state analysis
   *
   * See class documentation of toolbox::particles::ParticleSet for
   * an overview of the statistics tracked.
   */
  static int _numberOfRemainingLocalParticles;
  static int _numberOfIncomingLocalParticles;
  static int _numberOfLeavingLocalParticles;
  static int _numberOfExpiredHaloParticles;

  /**
   * Number of updated halo particles
   *
   * Updated by boundary merge operations.
   *
   * @see merge()
   *
   * See class documentation of toolbox::particles::ParticleSet for
   * an overview of the statistics tracked.
   */
  static int _numberOfAddedVirtualParticles;
  static int _numberOfReplacedVirtualParticles;
  static int _numberOfDroppedVirtualParticles;

  /**
   * Stats on duplicate elimination
   *
   * This one is updated internally by eliminateSpatialDuplicates() which you
   * might call or you might not call. It is never updated anywhere else.
   *
   * See class documentation of toolbox::particles::ParticleSet for
   * an overview of the statistics tracked.
   */
  static int _numberOfVirtualEliminatedSpatialDuplicates;
  static int _numberOfLocalEliminatedSpatialDuplicates;

  /**
   * Map of persistent particles that have to be sieved
   */
  static SieveParticles<T> _sieveParticles;

#if PeanoDebug >= 1
  tarch::la::Vector<Dimensions, double> _debugX;
  tarch::la::Vector<Dimensions, double> _debugH;
#endif
};


#include "ParticleSet.cpph"
