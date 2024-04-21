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
 * Utility class for global sorting for all flavours of particle sets
 *
 * Particle sets and different sorting strategies all differ in the way they
 * actually manage their particles and how they are assigned to the mesh
 * vertices. Notably, they all differ in the way when and how they sort
 * globally. Some sorting strategies such as peano4.toolbox.particles.UpdateParticleGridAssociation_LiftDrop
 * try to realise as much sorting as possible locally by shifting particles
 * between levels and only resort to a global sieve set when particles are
 * very fast, while other strategies such as peano4.toolbox.particles.UpdateParticleGridAssociation_BucketSort
 * generally throw all particles that need to be resorted into the global
 * sieve set and then take it from there.
 *
 *
 * ## Relationship to various sorting approaches
 *
 * So sieving means that particles are removed from the global mesh and
 * thrown into a sieve set represented by an instance of SieveParticles.
 * After one mesh traversal, this sieve set is fully populated. After that,
 * the individual subdomains run through their mesh and pick the particles
 * from the global sieve set as they fit. They realise a bucket sort. Again,
 * there are two different ways how to bucket sort:
 *
 * - Some strategies work with one global sieve set, and remove the particles
 *   from this one as they bucket sort into the domain
 * - Other strategies clone the sieve set per thread and pick out only those
 *   particles which are relevant.
 *
 * No matter which strategy you follow, all of them work against the same
 * signature and all of them share the same statistics. These features are
 * provided by this class:
 * The class is used by toolbox.particles.ParticleSet which usually holds
 * one class instance (static attribute) of this class to collect all sieve
 * information. To drop particles from the global sieve set into the respective
 * domains, mesh sweeps either use getParticlesToBeSievedIntoVertex() directly
 * or they create a copy of this sieve set through cloneParticlesToBeSieved()
 * and then hand out particles from this deep copy.
 *
 * There are few key routines that you should study should you try to use this
 * class:
 *
 * 1. addParticleThatCanNotBeLiftedWithinItsTree(). The name is actually not
 *   that good, as the class really doesn't care why you decide to add a
 *   particle to the sieve set. It simply adds it to the set.
 * 2. exchangeSieveListsGlobally(). Exchange all the particles between MPI
 *   ranks, so everybody sees all particles, and also roll them over: particles
 *   that have been added to the sieve set are form hereon in the list that
 *   have to be sieved (bucket sorted), while the list of sieve particles is
 *   cleared such that you can add further new particles to it.
 * 3. getParticlesToBeSievedIntoVertex() picks those particles from the list of
 *   particles that have to be deployed/bucket sorted that fit to one
 *   particular vertex.
 *
 *
 * ## Periodic boundary conditions
 *
 * If particles move around a lot such that the underlying sorting algorithm
 * decides that it would prefer a global sorting step, it adds these particles
 * to the sieve set. If these particles did fly through the domain boundary and
 * now are outside, noone will bucket sort (sieve) them and they will eventually
 * be deleted after the next mesh sweep.
 *
 * If particles did cross a periodic boundary, they should reenter the domain
 * again: They might, for example, be lifted into the sieve set on the far left
 * and of the domain but should be sorted into the far right one in return. To
 * faciliate this, you have to run through all particles within the sieve set
 * once and apply the periodic boundary conditions to them. This can be done
 * either before exchangeSieveListsGlobally() for the rank-local set of
 * _particlesThatCanNotBeLiftedWithinTheirTree, or is has to be done after the
 * exchange on _particlesThatHaveToBeSieved.
 */
template <typename T>
class toolbox::particles::SieveParticles {
public:
  typedef T DoFType;

  typedef std::list<T*> ParticleList;

  SieveParticles() = default;

  /**
   * Destructor
   *
   * The sieve set works with shallow copies, i.e. we assign particles from
   * the mesh and the ownership then goes over into the sieve set. However,
   * when we sort particles into the mesh, each mesh traversal creates its own
   * copy of the particles. The routine cloneParticlesToBeSieved() creates a
   * deep copy.
   *
   * As we might hand around this deep copy, and as the copy constructor also
   * creates shallow copies, the destructor does not(!) free the memory. You
   * have to do this manually via a call to deleteParticles().
   */
  virtual ~SieveParticles();

  /**
   * Deep clear.
   */
  void deleteParticles();

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
  void exchangeSieveListsGlobally(
        const tarch::la::Vector<Dimensions,double>   domainOffset,
        const tarch::la::Vector<Dimensions,double>   domainSize,
        const std::bitset<Dimensions>                periodicBC
      );

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
  void addParticleThatCanNotBeLiftedWithinItsTree(T*);

  /**
   * Clone object, but let it contain only the delivering part, i.e. those
   * particles that have to be sieved.
   */
  SieveParticles<T> cloneParticlesToBeSieved();

  /**
   * Get particles that are to be sieved into one vertex
   *
   * Takes the current particle set and finds those particles which can be
   * sieved into current vertex. The operation alters the object state of
   * particleList, as it returns the particles that are to be sieved and
   * removes them from the current set.
   *
   * In line with peano4.toolbox.particles.Particle, we do not alter the
   * parallel state. That's the job of whoever calls this routine. Most
   * action sets (notably our sorting action sets) will invoke the boundary
   * merger and this merger will then take care of the parallel flag.
   *
   * All the particles in the result set are assigned the correct mesh
   * size h of the corresponding mesh into which we sieve it. So this
   * attribute is changed.
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
   * @see getSievedParticlesForNextGridTraversal() for the deep copy and the
   *   roll-over of particles that cannot be lifted into particles that have
   *   to be sieved.
   * @see ::toolbox::particles::sieveParticle() for the sieve logic.
   * @see deleteParticles() for the deletion of deep copies.
   *
   * @param originatingTree Number of tree (positive integer) or GlobalSort. It
   *   indicates where the particles should come from, i.e. are they form a
   *   specific tree or some particles that we have to sort globally.
   *
   * @param removeReturnedParticlesFromSet If this field is set, we remove the
   *   particles from the underlying set before we return them. That is, the
   *   underlying set on which we call getParticlesToBeSievedIntoVertex()
   *   shrinks monotonously. If you set it to false, the particles will reside
   *   within the set (for this mesh traversal). Therefore, you should not use the returned
   *   particles directly but rather create copies. There is no code at the
   *   moment that does not remove the vertex from the set, as we otherwise
   *   might sieve the same particle multiple times into the mesh (as the
   *   association which particle belongs to which vertex is not absolutely
   *   unique).
   */
  ParticleList getParticlesToBeSievedIntoVertex(
    const peano4::datamanagement::VertexMarker& marker,
    bool  removeReturnedParticlesFromSet,
    bool  onlyReturnParticlesThatWillBeLocal,
    bool  lockSemaphore
  );

  /**
   * Fine out how many particles still to sieve
   *
   * If you ask how many particles we have to sieve, you will receive the ones
   * that have not been sieved so far. Therefore, this routine can not be used
   * for any statistics after a grid sweep to find out how many sieves have
   * been done.
   */
  bool hasParticlesToBeSievedIntoVertices() const;

  /**
   * hasParticlesToBeSievedIntoVertices() is equivalent to checking if this
   * routine returns something greater than 0.
   */
  int getNumberOfParticlesThatHaveBeSievedIntoVertices() const;

  /**
   * @return Overview of set.
   */
  std::string toString() const;

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
   * Semaphores
   *
   * This is for the assembly of the set of particles which require global
   * (sieve) data exchange. When we later on sieve the particles into their
   * target vertices, we need another semaphore of its own.
   *
   * In the present case, the semaphores don't have to be static, as we assume that
   * ParticleSet uses this class as a static class. That is, we implicitly
   * have a static attribute. In return, if someone creates a deep clone
   * of the present particle set through cloneParticlesToBeSieved(), we
   * explicitly don't want the semaphores to lock all other particle sets.
   *
   * Problems arise once we translate these considerations 1:1 into code,
   * as semaphores cannot be copied. That is, we cannot omit the static
   * keyword. To take the synchronisation remark into account (some routines
   * can work with thread-local copies), we therefore have to make the
   * semaphores static and "just" not lock them if we work with thread-local
   * copies.
   */
  static tarch::multicore::BooleanSemaphore _particlesThatCanNotBeLiftedWithinTheirTreeSemaphore;
  static tarch::multicore::BooleanSemaphore _particlesToBeSievedSemaphore;

  ParticleList  _particlesThatCanNotBeLiftedWithinTheirTree;

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
  ParticleList  _particlesThatHaveToBeSieved;
};


#include "SieveParticles.cpph"
