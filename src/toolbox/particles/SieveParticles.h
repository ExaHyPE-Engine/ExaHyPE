// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include <list>
#include <map>

#include "tarch/logging/Log.h"
#include "tarch/multicore/BooleanSemaphore.h"

#include "peano4/datamanagement/VertexMarker.h"


namespace toolbox {
  namespace particles {
    template <typename T>
    class SieveParticles;
  } // namespace particles
} // namespace toolbox


/**
 * Utility class for all flavours of particle sets
 *
 * Particle sets differ in the way they actually manage the particles. However,
 * they all share common stats, operations, ... This is covered by this class.
 * The class is used by toolbox.particles.ParticleSet which usually holds
 * multiple of these classes for different purposes.
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
 *
 */
template <typename T>
class toolbox::particles::SieveParticles {
public:
  typedef T DoFType;

  typedef std::list<T*> ParticleList;

  static constexpr int GlobalSort = -1;

  /**
   * Get all the global sieve data consistent
   *
   * Has to be called after the traversal has finished on each rank by the
   * main, i.e. do not call it within the action sets. You may not call
   * this routine more than once per rank per traversal. The core purpose
   * of this routine is to get the global sieve data all consistent: If a
   * tree cannot resort its local particles, and, hence, cannot even
   * synchronise with its neighbours as particles moved too fast, it dumps
   * the particle into the global sieve list. After the traversal, all
   * ranks (and eventually trees) should see the same global sieve list and
   * start to sieve particles into their respective domain.
   *
   *
   * ## Algorithmic steps
   *
   * - Delete all the sieve particles which have not been sieved. We note
   *   that only particles within the domain are taken from the sieve set
   *   and inserted into the mesh. For particles close to domain
   *   boundaries, we create copies and insert these copies. The preimage
   *   consequently has to be deleted at one point, i.e. here in this
   *   routine.
   * - Inform all other ranks about my number of local particles that I
   *   could not sort on the local rank.
   * - Per other rank:
   *   - Find out how many particles will be received from this partner.
   *   - Send out my own copy of particles with an unblocking send.
   *   - Receive the guys from the neighbour. We can append these received
   *     data directly to our local data. We will use these local buffers
   *     to send out stuff to other neighbours, but as we append, we will
   *     only send out the first few particles which really stem from the
   *     local node.
   *   - Wrap up the unblocking send via a wait.
   *   It is important that we allow the rank to handle other (unexpected)
   *   message while we wait for the particle exchange to terminate. This
   *   is possible, as we use a tag of its own for the global particle
   *   exchange. Without a wait for unexpected messages, it could for
   *   example be that another rank requires a tree booking due to load
   *   balancing, and we cannot serve the tree request in our rank, as we
   *   are already stuck in the global particle exchange.
   * - We roll over the globally lifted particles into the sieve set for
   *   the next mesh traversal.
   * - Remove duplicates from the sieve list. The sieve list might contain
   *   redundant particles for those guys which ended up exactly on the
   *   boundary between two trees. Such guys are considered to be local in
   *   multiple subdomains and hence might also end up redundantly within
   *   the sieve list.
   * - Clear the container _particlesThatCanNotBeLiftedWithinTheirTree
   *   which will be used in the future to gather the newly sieved
   *   particles.
   */
  void exchangeSieveListsGlobally();

  /**
   * Add a particle to the list of particles which cannot be lifted
   *
   * You hand over the pointer to the sieve list. That is, nothing is
   * copied, but the responsibility for this object, from now on, resides
   * with the sieve list. It will eventually delete the particle. Therefore
   * you should not maintain any pointer to the particle anymore once you
   * hand it over to the sieve list.
   *
   * The routine is thread-safe. The object does not keep any kind of
   * statistics. You have to maintain stats on your own if you need them.
   * However, it only uses the lock, i.e. is thread-safe for
   * originatingTree==GlobalSort.
   */
  void addParticleThatCanNotBeLiftedWithinItsTree(T*, int originatingTree);

  /**
   * Get particles that are to be sieved into one vertex
   *
   * Takes the current particle set and finds those particles which can be
   * sieved into current vertex. The operation alters the object state of
   * particleList, as it returns the particles that are to be sieved and
   * removes them from the current set.
   *
   * In line with peano4.toolbox.particles.Particle, we do not alter the
   * parallel state. That's the job of peano4.toolbox.particles.api.UpdateParticle.
   *
   * All the particles in the result set are assigned the correct mesh
   * size h of the corresponding mesh into which we sieve it.
   *
   *
   * ## Thread safety
   *
   * This routine is not static, i.e. we assume it works on a thread-local
   * list of particles. This thread-local list is typically constructed as
   * deep copy of the global sieve list. getSievedParticlesForNextGridTraversal()
   * provides these deep copy semantics. Since we work with a copy of the
   * global particles per thread, there's no need to add semaphores. We are
   * inherently thread-safe. However, we have to delete all left-over
   * particles which have not been sieved by a local thread. As the list
   * had been a deep copy, we'd otherwise create a memory leak.
   *
   * The routine is thread-safe, i.e. uses a lock, i.e. if
   * originatingTree==GlobalSort.
   *
   * @see getSievedParticlesForNextGridTraversal() for the deep copy and the
   *   roll-over of particles that cannot be lifted into particles that have
   *   to be sieved.
   * @see ::toolbox::particles::sieveParticle() for the sieve logic.
   * @see deleteParticles() for the deletion of deep copies.
   *
   * @param originatingTree Number of tree (positive integer) or GlobalSort. It
   *   indicates where the particles should come from, i.e. are they form a
   *   specific tree or some particles that we have to sort globally.
   * @param callingTree Number of tree calling this routine. We need this
   *   information to find out if particles that we deposit are local or
   *   virtual.
   */
  ParticleList getParticlesToBeSievedIntoVertex(const peano4::datamanagement::VertexMarker& marker, int originatingTree, int callingTree);

  /**
   * Fine out how many particles still to sieve
   *
   * If you ask how many particles we have to sieve, you will receive the ones
   * that have not been sieved so far. Therefore, this routine can not be used
   * for any statistics after a grid sweep to find out how many sieves have
   * been done.
   *
   * The routine is thread-safe, i.e. uses a lock, i.e. if
   * originatingTree==GlobalSort.
   *
   * @param originatingTree Number of tree (positive integer) or GlobalSort.
   */
  bool hasParticlesToBeSievedIntoVertices(int originatingTree) const;


private:
  static tarch::logging::Log _log;

  static void deleteParticles(ParticleList& list);

  /**
   * Reduction tag
   *
   * We one of these per subclass, i.e. I cannot move it to the superclass
   */
  static int _SieveReductionTag;

  /**
   * This is for the assembly of the set of particles which require global
   * (sieve) data exchange. When we later on sieve the particles into their
   * target vertices, we need another semaphore of its own.
   */
  static tarch::multicore::BooleanSemaphore _particlesThatCanNotBeLiftedWithinTheirTreeSemaphore;
  static tarch::multicore::BooleanSemaphore _particlesToBeSievedSemaphore;

  std::map<int, ParticleList> _particlesThatCanNotBeLiftedWithinTheirTree;

  /**
   * Particles that should be sieved
   *
   * This is a set of particles which should be sieved in the next traversal.
   * However, nobody takes particles directly from this list. Instead, tasks
   * will work with their own clones of this list.
   *
   * @see getSievedParticlesForNextGridTraversal()
   *
   * The list is populated by finishedTraversal().
   */
  std::map<int, ParticleList> _particlesThatHaveToBeSieved;
};


#include "SieveParticles.cpph"
