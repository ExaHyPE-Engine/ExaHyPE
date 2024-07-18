// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "tarch/la/Vector.h"

namespace toolbox {
  namespace particles {
    /**
     * @namespace toolbox::particles::assignmentchecks Correctness checks of
     * particle-mesh assignment
     *
     * This API is inspired by SWIFT's swift2::dependencychecks as introduced
     * by Mladen: We keep some history of particle-mesh associations, trace new
     * mesh assignments and validate them against the recorded history. All the
     * tracing degenerates to empty routines if we work in release mode, so
     * there should not be any overhead.
     *
     * Most toolbox routines do automatically report against this API. However,
     * to make the checks work, you have to report particle movements manually.
     *
     * Furthermore, you might want to invoke
     * toolbox::particles::assignmentchecks::startMeshSweep() prior to your
     * traversals to ensure that you get a nice break-down of all mesh
     * transitions.
     *
     * ## Enable assignment checks
     *
     * By default, the assignment checks are enabled as soon as PeanoDebug is
     * set to a value greater than 0. However, you can explicitly disable it
     * by adding
     *
     * -DnoAssignmentChecks
     *
     * to your compile flags. This is important as assignment checks are very
     * expensive. The checks also might yield invalid errors once you run them
     * over multiple ranks, i.e with -DParallel.
     */

    namespace assignmentchecks {

      /**
       * Get the history of stored sweeps.
       */
      std::string sweepHistory();

      /**
       * Inform API that this is the end of a grid run-throuch.
       *
       * Delegates to the corresponding method in the database.
       */
      void startMeshSweep(const std::string& meshSweepName);

      /**
       * Log that a particle is erased
       *
       * This should solely happen for virtual particles, i.e. halo particles.
       * Before this function is called, the corresponding particle has to be
       * removed from its associated vertex.
       */
      void eraseParticle(
        const std::string&                           particleName,
        const tarch::la::Vector<Dimensions, double>& particleX,
        const int                                    particleID,
        bool                                         isLocal,
        const tarch::la::Vector<Dimensions, double>& vertexH,
        int                                          treeId,
        const std::string&                           trace
      );

      /**
       * Record the movement of a particle
       *
       * We insert a new entry which memorises the particle movements. However,
       * something we come across very often is that particles, once assigned to
       * a vertex, very often stay assigned to the same vertex for a long time
       * (=many sweeps) and only experiences small movements. Rather than
       * tracing every single move event individually, the database will mark
       * consecutive move events using a special event type,
       * Event::Type::ConsecutiveMoveEvent. It will also keep track the original
       * position of the particle, before the first move event of that sequence
       * has occurred, as well as the current particle position.
       */
      void moveParticle(
        const std::string&                           particleName,
        const tarch::la::Vector<Dimensions, double>& oldParticleX,
        const tarch::la::Vector<Dimensions, double>& newParticleX,
        const int                                    particleID,
        const tarch::la::Vector<Dimensions, double>& vertexX,
        const tarch::la::Vector<Dimensions, double>& vertexH,
        int                                          treeId,
        const std::string&                           trace
      );

      /**
       * Assign a particle to a vertex
       *
       * Take the particle, perform some consistency checks, and then add the
       * particle event to our history database. The consistency checks differ
       * for local from virtual particles.
       *
       * ## Consistency checks
       *
       * ### Consistency checks for local particles
       *
       * - A new particle might have been inserted into the domain for the very
       *   first time. In this case, the particle has to be "free", i.e. it may
       *   not have been assigned to any other vertex yet. The search should
       *   reduce a NotFound, or find that in a previous sweep it had been a
       *   particle that has been erased.
       * - The particle might have been dropped from a coarser level. See
       *   corresponding discussion for virtual particles below including some
       *   sketches.
       * - The particle might have been lifted. Lifting can only occur for
       *   local particles, as it is a result of a move and we may only move
       *   local particles. If the assignment is due to a lift, this local
       *   particle has been assigned to a finer mesh resolution before.
       * - If we lift a particle into the global sieve set (as the particle
       *   tunnels or there's a horizontal cut through the spacetree, i.e. the
       *   coarser cell is held by another rank), we will sieve the particle
       *   into the local domain in the next step. In this case, the previous
       *   particle's state could be local or virtual: If the particle has moved
       *   and therefore ended up in the sieve set, we will not have updated
       *   its parallel flag yet. Therefore, it might have changed.
       * - A particle might change from remote to local. This can only happen
       *   in UpdateParallelState after a merge, which first detaches a particle
       *   and then re-adds it with the local flag set.
       *
       *
       * ### Consistency check for virtual particles
       *
       * - The particle comes in through the boundary as part of the boundary
       *   exchange. In this case, the particle has to be "free", i.e. it may
       *   not be assigned to any other vertex yet. The particle has to be new
       *   (NotFound in database), or virtual in a previous sweep as well and
       *   having been erased.
       * - If it is an update of the halo copy, we would expect the
       *   corresponding merge() to replace this copy. A replacement manifests
       *   in an event sequence of detach->erase->assign, so we still get a
       *   NotFound or Erase throughout the assign.
       * - A virtual particle might also be subject of a mesh drop: If we lift
       *   a particle, exchange it along the boundary, we have to drop it on
       *   both sides again. We update its state
       *   ***after*** we have assigned it to a vertex, but we also set its
       *   state in the merge, so here the state has to be virtual.
       * - A local particle might have changed from local to virtual as it
       *   had moved. In this case, the update should detach the particle
       *   before it adds it again.
       * - We might drop due to a global sieve. In this case, the previous
       *   particle's state could be local or virtual: If the particle has moved
       *   and therefore ended up in the sieve set, we will not have updated
       *   its parallel flag yet. Therefore, it might have changed.
       *
       * In the scenario below, we illustrate the last scenario by means of an
       * interface between two ranks. It
       * is denoted below as grey partition (one left rank) and a blueish
       * partition (on the right rank). They have kind of a halo layer of
       * width one, which is a logical construct, i.e. not maintained manually.
       *
       * @image html scenario00_particle-through-domain-wall.png
       *
       * Study the central setup: In step N, yellow moves, while red will be
       * deleted after the traversal, as each step moving particles has to
       * update states as well (UpdateParallelState action set). Yellow now
       * might be lifted to the next level. So far, its state is local on the
       * grey rank, no matter if it stays within this subdomain or not. We
       * exchange the data after step N.
       *
       * In step N+1, either rank sets the state to local or virtual. This
       * happens either in touchVertexFirstTime() in the UpdateParallelState
       * action set, or it happens straight in merge(). No matter what, it
       * happens on the coarser level before we even touch the fine mesh
       * level. When we drop it, the particle has the correct parallel state.
       *
       *
       * Another special situation arises from a setup, where the particle of
       * interest is located exactly along the subdomain boundary. In this
       * case, we move both copies, as the particle is considered to be local
       * on both ranks. Let one rank (tree 0) subsequently lift the particle,
       * as it has moved quite a bit. The other rank (tree 1) therefore also
       * has to lift yet cannot do so, as the next coarser mesh level is
       * actually owned by tree 0. It adds its particle to the sieve set.
       * After the movement, the particle is on the coarser level on tree 0
       * and in the sieve set of tree 1.
       *
       * This sieve set is now exchanged between the trees. Tree 1 seems to be
       * kind of simple: The particle will be taken from the sieve set,
       * inserted into the domain (given that it hasn't totally left this
       * tree) and will then either be set to local or virtual. However, we
       * will also get a copy of this particle from tree 0 through the
       * boundary. This copy will then be sieved. We end up with two copies:
       * One from the global sieve set and one from the boundary.
       *
       * Tree 0's behaviour exhibits the same behaviour: We will drop the
       * particle which we have temporarily lifted to the next coarser level.
       * After we have dropped it, we will also insert the copy from the
       * global sieve set.
       *
       * @param particleIsNew If we know that a particle is new, there's no
       *   need to search through the database.
       * @param reassignmentOnSameTreeDepthAllowed If true, assume our
       *   particle sorting/bookkeeping algorithm allows for a particle to be
       *   reassigned from one vertex to another vertex on the same depth in
       *   the tree directly (due to its movement). Otherwise, expect the
       *   particle to be sorted either via sieves or a sequence of lifts and
       *   drops.
       */
      void assignParticleToVertex(
        const std::string&                           particleName,
        const tarch::la::Vector<Dimensions, double>& particleX,
        const int                                    particleID,
        bool                                         isLocal,
        const tarch::la::Vector<Dimensions, double>& vertexX,
        const tarch::la::Vector<Dimensions, double>& vertexH,
        int                                          treeId,
        const std::string&                           trace,
        bool                                         particleIsNew = false,
        bool reassignmentOnSameTreeDepthAllowed                    = false
      );


      /**
       * Remove particle from vertex
       *
       * To make this API call valid, the particle has to be assigned to a
       * vertex beforehand. So there has to be a assignParticleToVertex()
       * before this one.
       *
       * An exception may occur during mergeWithParticle() if we're using
       * periodic boundary conditions: Once a particle is mirrored along the
       * boundary (for the first time), we won't be able to identify its
       * history in the database due to its change in position. This can
       * happen during a first mergeWithParticle() in the simulation run.
       * In that case, permit the previous event to be of type NotFound via
       * the @param inMergeWithPeriodicBoundarySwap.
       *
       * TODO: we may need to extend this to non-periodic boundary swaps
       * too. If the exchange is via MPI, then a particle may not exists
       * in the database of the other rank too, and we have the same problem.
       * Then we should permit an event of type NotFound for any merge event,
       * not just for periodic boundary data swaps.
       */
      void detachParticleFromVertex(
        const std::string&                           particleName,
        const tarch::la::Vector<Dimensions, double>& particleX,
        const int                                    particleID,
        bool                                         isLocal,
        const tarch::la::Vector<Dimensions, double>& vertexX,
        const tarch::la::Vector<Dimensions, double>& vertexH,
        int                                          treeId,
        const bool                                   inMergeWithPeriodicBoundarySwap,
        const std::string&                           trace
      );

      /**
       * Assign a particle to a sieve set
       *
       * To make this routine valid, the particle may not be assigned to a
       * vertex anymore, i.e. detachParticleFromVertex() has to be called
       * before.
       */
      void assignParticleToSieveSet(
        const std::string&                           particleName,
        const tarch::la::Vector<Dimensions, double>& particleX,
        const int                                    particleID,
        bool                                         isLocal,
        const tarch::la::Vector<Dimensions, double>& vertexH,
        int                                          treeId,
        const std::string&                           trace
      );

      /**
       * Find out particle name
       *
       * In some places, it is convenient to construct the particle name from
       * the type. However, some compilers only give you something similar to
       *
       * ~~~~~~~~~~~~~~~~~~~~
       * N10benchmarks6swift23noh10globaldata9HydroPartE
       * ~~~~~~~~~~~~~~~~~~~~
       *
       * which is really not what you want. This routine tries to reconstruct a
       * meaningful result.
       */
      template <typename Particle> std::string pruneTypeName() {
        std::string result    = typeid(Particle).name();
        std::size_t lastDigit = result.find_last_of("0123456789");
        if (lastDigit != std::string::npos) {
          result = result.substr(lastDigit + 1);
        }
        assertion2(result.size() > 1, typeid(Particle).name(), result);
        return result.size() > 1 ? result.substr(0, result.size() - 1) : result;
      }

      /**
       * Eliminate existing particles, so only those that are "forgotten"
       * remain
       *
       * This routine runs through the particles for which we have
       * recorded events. If the last event
       *
       * - affects a remote particle;
       * - is a move;
       * - is an assignment to vertex,
       *
       * we know that it cannot eliminate existing particles. So we
       * remote their trajectories via removeTrajectory(). If the
       * last snapshot is empty, we remove it completely.
       *
       * Once this routine has terminated, we get the stuff in the
       * database for particles that seem to have been left over. We also
       * get a lot of historic data, but if we plot the database and
       * study solely the last snapshot, we typically get a good idea of
       * which particles got lost.
       */
      void eliminateExistingParticles();

      /**
       * Ensure that database is empty
       *
       * This one is usually called after eliminateExistingParticles() and will
       * leave the user only with particles which have "magically" disappeared.
       *
       * This routine uses exit(-1) if no assertions are enabled.
       */
      void ensureDatabaseIsEmpty();

      using ParticlePosition = tarch::la::Vector<Dimensions, double>;

      /**
       * Record the current particle positions in a vector.
       */
      template <typename ParticleContainer>
      std::vector<ParticlePosition> recordParticlePositions(
        const ParticleContainer& container
      );

      /**
       * Trace the particle movements.
       *
       * This routine runs through the particles in container and invokes
       * moveParticle() for each and every particle which has moved. That is,
       * we assume that recordedPositions has been obtained through
       * recordParticlePositions() before, and both recordedPositions and
       * container hold the same particles in the same order (even though they
       * might have moved in-between).
       *
       * We only record particles that have moved, but actually the comparison
       * here is only an optimisation, as the moveParticle itself will once
       * again check that the particle has actually changed its position.
       */
      template <typename ParticleContainer>
      void traceParticleMovements(
        const ParticleContainer&                     container,
        const std::vector<ParticlePosition>&         recordedPositions,
        const tarch::la::Vector<Dimensions, double>& vertexX,
        const tarch::la::Vector<Dimensions, double>& vertexH,
        int                                          spacetreeId
      );

    } // namespace assignmentchecks
  }   // namespace particles
} // namespace toolbox


#include "TracingAPI.cpph"
