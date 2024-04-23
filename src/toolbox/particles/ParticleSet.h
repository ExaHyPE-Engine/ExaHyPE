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
 * All particle types (species) are also tied to a sieve set, which is also
 * class attribute of this abstract particle supertype.
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
 *
 * ## Sieve semantics
 *
 * Whenever a particle has to be added to the sieve set, the underlying action
 * set will inform the particle set or vertex, respectively, to move the
 * particle into the sieve set. This step has to be protected by a semaphore,
 * as relaised in SieveParticles. After the mesh sweep, all the particles are
 * exchanged between ranks, and then we have to sort them into the mesh again.
 *
 * There are two different modes how to use this sieve set: We can either work
 * against a global sieve set, or we can make each thread first create a copy
 * of all the particles to be sieved and then hand out thread-local copies.
 * Which variant to pick is a choice to be made by the sorting action set.
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
   * Clear stats of all counters that have to do with resorting
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
  static void reduceParticleStateStatistics();
  static std::string printParticleStateStatistics();

  static void reduceReassignmentStatistics();
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
    int numberOfRemainingLocalParticles,
    int numberOfExpiredHaloParticles,
    int numberOfParticlesThatHaveLeftTheirDomain
  );

  /**
   * Return number of drops into cuts recorded
   *
   * This counter is typically updated by the action sets which update the
   * particle-mesh association. The field should guide the simulation main
   * whether to rerun this dropping action set again or continue with next
   * step in simulation pipeline.
   */
  static int getNumberOfDropsIntoHorizontalTreeDecomposition();

  /**
   * Returns true if there have been any lifts, drops, reassignments, ...
   */
  static bool registeredAnyResorting();

#ifdef Parallel
  static void initDatatype();

  static void shutdownDatatype();
#endif

  /**
   * Invoke exchangeSieveListsGlobally().
   */
  static void finishedTraversal(
      const tarch::la::Vector<Dimensions,double>   domainOffset,
      const tarch::la::Vector<Dimensions,double>   domainSize,
      const std::bitset<Dimensions>                periodicBC
  );

  /**
   * @return A copy of all the particles that have to be sieved.
   */
  static SieveParticles cloneParticlesToBeSieved();

  /**
   * Make routine from SieveParticles public
   *
   * Delegate through. This way, we don't have to expose the actual
   * sieve object. Usually combined with hasParticlesToBeSievedIntoVertices()
   * as guard, which makes the querying faster.
   * 
   * This operation always removes particles from the sieve set, and it 
   * sort exclusively local particles into the domain. We use it within
   * the lift-drop sorting approach, where it is used by the dummy mesh
   * sweeps following each sweep that moves around particles. Details on
   * this scheme are provided in peano4.toolbox.particles.api.UpdateParticleGridAssociation_LiftDrop.
   *
   * All action sets in this scheme share one common sieve set. Therefore, we
   * do not run risk that one tree grabs a particle, sorts it into its mesh,
   * labels it as outside and then this particle disappears. This is avoided,
   * as we only sieve in particles if they become local.
   *
   * When we take a particle that's local, we have to remove it from the
   * sieve set. This is triggered by a hard-coded boolean that we pass on.
   * If we don't remove it, we can run into situations where a particle sits
   * right in-between two vertices and is hence sieved in twice. It appears
   * two times.
   *
   * Consult the @ref page_toolbox_particles_mesh_consistency
   *
   * @see SieveParticles::getParticlesToBeSievedIntoVertex()
   */
  static ParticleList getParticlesToBeSievedIntoVertex(
    const peano4::datamanagement::VertexMarker& marker
 );

  /**
   * Make routine from SieveParticles public
   *
   * Delegate through. This way, we don't have to expose the actual
   * sieve object. This predicate is usually always used as guard before
   * actually invoking getParticlesToBeSievedIntoVertex().
   */
  static bool hasParticlesToBeSievedIntoVertices();

  /**
   * Return total number of particles to be sieved
   *
   * Delegates to SieveParticles::getNumberOfParticlesThatHaveBeSievedIntoVertices()
   */
  static int getNumberOfParticlesThatHaveBeSievedIntoVertices();

  /**
   * Return global sum of the particles that did remain within their partition at a time.
   *
   * Should only be called on global master after the reduction.
   */
  static int getNumberOfRemainingLocalParticles();

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
  static int _numberOfDropsIntoHorizontalTreeDecomposition;

  /**
   * Set by parallel state analysis
   *
   * See documentation of action set peano4.toolbox.particles.api.UpdateParallelState.
   */
  static int _numberOfRemainingLocalParticles;

  /**
   * Set by parallel state analysis
   *
   * See documentation of action set peano4.toolbox.particles.api.UpdateParallelState.
   */
  static int _numberOfExpiredHaloParticles;

  /**
   * Set by parallel state analysis
   *
   * See documentation of action set peano4.toolbox.particles.api.UpdateParallelState.
   */
  static int _numberOfParticlesThatHaveLeftTheirDomain;

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
  static int _numberOfReplacedVirtualParticlesAlongBoundary;

  /**
   * Newly "discovered" virtual particles
   *
   * If a particle is far away from "our" boundary and then approaches this
   * boundary on the neighbouring rank, it will, at one point, become visible
   * here and we add it to our set of virtual particles.
   *
   * Updated by boundary merge operations.
   */
  static int _numberOfAddedVirtualParticlesAlongBoundary;
  static int _numberOfAddedLocalParticlesAlongBoundary;

  /**
   * Map of persistent particles that have to be sieved
   *
   * Every particle set is tied to one global sieve set realised through this
   * attribute. Sieving is briefly also mentioned on @ref page_toolbox_particles_mesh_consistency.
   * Access to this set is provided through two routines:
   *
   * - getParticlesToBeSievedIntoVertex() pipes your request directly through
   *   to the set's getParticlesToBeSievedIntoVertex(), i.e. it grants you
   *   access to the global sieve set.
   * - Alternatively, you can clone the whole sieve set for your algorithm via
   *   cloneParticlesToBeSieved() and then sieve from thereon.
   *
   * Different particle sorting strategies these different options the way they
   * see fit.
   *
   */
  static SieveParticles _sieveParticles;

#if PeanoDebug >= 1
  tarch::la::Vector<Dimensions, double> _debugX;
  tarch::la::Vector<Dimensions, double> _debugH;
#endif
};


#include "ParticleSet.cpph"
