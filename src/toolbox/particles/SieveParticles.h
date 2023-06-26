// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "tarch/multicore/BooleanSemaphore.h"
#include <list>


namespace toolbox {
  namespace particles {
    template <typename T>
    class SieveParticles;
  }
}



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
 */
template <typename T>
class toolbox::particles::SieveParticles {
  public:
    typedef T  DoFType;

    typedef std::list<T*> ParticleList;

    /**
     * Deep copy of particles that have to be sieved
     *
     * This routine returns a deep copy of all the particles that are to be
     * sieved. Use this before you start to traverse your local domain. As
     * it is a deep copy, you have to invoke deleteParticles() on the result.
     * The rationale behind the deep copying is as follows: Individual
     * threads run through their subdomain in parallel and they should not
     * compete for a global sieve set. At the same time, you want to
     * eliminate a particle from the sieve set as soon as you have worked it
     * in: No code should check that particle again, and you definitely don't
     * want to insert it multiple times. So I make each thread (subtree) work
     * with a sieve set of its own which it can manipulate as it wants. At
     * the end of the traversal, it will delete all remaining particles, i.e.
     * kill all those deep copies that were not relevant to the local
     * partition.
     *
     * @see deleteParticles() to clean up the local copy.
     * @see _particlesThatHaveToBeSieved which is the container from which we
     *   create a deep copy.
     */
//    ParticleList getSievedParticlesForNextGridTraversal() const;

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
     * The routine is thread-safe. The object does not keep any kind of
     * statistics. You have to maintain stats on your own if you need them.
     */
    void addParticleThatCanNotBeLiftedWithinItsTree( T* );

    /**
     * Get particles that are to be sieved into one vertex
     *
     * Takes the current particle set and finds those particles which can be
     * sieved into current vertex. The operation alters the object state of
     * particleList, as it returns the particles that are to be sieved and
     * removes them from the current set.
     *
     * All the particles in the result set have the parallel state Virtual,
     * as we have to assume that they have been non-local on the current rank
     * so far. The new parallel state is not altered, as this is the job of
     * the action set UpdateParallelSet.
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
     *
     * @see getSievedParticlesForNextGridTraversal() for the deep copy and the
     *   roll-over of particles that cannot be lifted into particles that have
     *   to be sieved.
     * @see ::toolbox::particles::sieveParticle() for the sieve logic.
     * @see deleteParticles() for the deletion of deep copies.
     */
    ParticleList  getParticlesToBeSievedIntoVertex(
      const peano4::datamanagement::VertexMarker&  marker
    );

    bool hasParticlesToBeSievedIntoVertices() const;

    int getNumberOfParticlesToBeSieved() const;
    int getNumberOfParticlesThatCanNotBeLiftedWithinTheirTree() const;
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
    static tarch::multicore::BooleanSemaphore   _particlesThatCanNotBeLiftedWithinTheirTreeSemaphore;
    static tarch::multicore::BooleanSemaphore   _particlesToBeSievedSemaphore;

    ParticleList     _particlesThatCanNotBeLiftedWithinTheirTree;

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
    ParticleList     _particlesThatHaveToBeSieved;
};


#include "SieveParticles.cpph"
