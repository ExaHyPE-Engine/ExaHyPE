// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include <map>
#include <vector>
#include <list>

#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/VertexEnumerator.h"
#include "tarch/la/Vector.h"
#include "tarch/multicore/MultiReadSingleWriteSemaphore.h"


namespace toolbox {
  namespace particles {
    /**
     * @namespace toolbox::particles::assignmentchecks Correctness checks of particle-mesh assignment
     *
     * This API is inspired by SWIFT's swift2::dependencychecks as introduced
     * by Mladen: We keep some history of particle-mesh associations, trace new
     * mesh assignments and validate them against the recorded history. All the
     * tracing degenerates to empty routines if we work in release mode, so
     * there should not be any overhead.
     *
     * Most toolbox routines do automatically report against this API. However, to
     * make the checks work, you have to report particle movements manually.
     *
     * Furthermore, you might want to invoke toolbox::particles::assignmentchecks::startMeshSweep()
     * prior to your traversals to ensure that you get a nice break-down of all
     * mesh transitions.
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
      namespace internal {
        struct ParticleIdentifier;
        struct Event;
        class MeshSweepData;
        class Database;

        /**
         * Key for internal bookkeeping of vertex-particle association
         *
         * We assume that a particle is unique due to its name (species) and
         * its position. Particle information will be stored in a (hash) map
         * and therefore needs a smaller operator.
         *
         * ## Sorting
         *
         * The sorting is very simple: We first use the name as criterion if
         * two particles are the same or one is smaller than the others. For
         * this, we can use the string's < operator. If two particles have the
         * same name, we first sort along the x axis, then (for those particles
         * with the same x value) along the y axis, and so forth.
         *
         * As the keys work with floating point data, we have to take
         * truncation errors into account. Originally, I did attempt to realise
         * the routine as follows:
         *
         * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         * if (particleName < rhs.particleName) {
         *   assertion2( *this!=rhs, toString(), rhs.toString() );
         *   return true;
         * }
         * if (particleName > rhs.particleName) {
         *   assertion2( *this!=rhs, toString(), rhs.toString() );
         *   return false;
         * }
         * for (int d = 0; d < Dimensions; d++) {
         *   if (tarch::la::smaller(particleX(d), rhs.particleX(d))) {
         *     assertion2( *this!=rhs, toString(), rhs.toString() );
         *     return true;
         *   }
         *   if (tarch::la::greater(particleX(d), rhs.particleX(d))) {
         *     assertion2( *this!=rhs, toString(), rhs.toString() );
         *     return false;
         *   }
         * }
         * assertion2( *this==rhs, toString(), rhs.toString() );
         * return false;
         * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         *
         * This variant does not work, as it is too fuzzy. Situations
         * as the one below can arise:
         *
         * - We have A!<B, as A and B overlap.
         * - We have B!<A, as B and A overlap (and B is actually further right).
         * - Consequently, A==B. This follows the C++ map logic.
         * - We have B!<C, as B and C overlap.
         * - We have C!<B, as B and C overlap.
         * - Consequently, B==C.
         *
         * However, we clearly have A!=C which contradicts the statements above.
         *
         * I originally envisaged a very non-elegant solution to this problem:
         *
         * - The < operator which determines the storage order in the map is
         *   bit-wise and neglects the fact that we have floating-point data.
         * - Whenever we construct a new particle identifier, we first check
         *   if there's an entry in the database which is "close". If so, we
         *   use this identifier rather than a completely new one.
         *
         * I found this approach to be extremely expensive. Every time we
         * construct a particle identifier, we might run throught the whole
         * database. In most cases however, we will terminate the database
         * run through early: As soon as getEntry() finds a fit, it returns
         * immediately. So it is unsuccessful entry constructions which really
         * hurt us. These arise for assignParticleToVertex() for new particles.
         * In this case, we cannot avoid that we run through the whole
         * database. If a particle moves however, we don't have to search for
         * existing entries. This movement will yield a new position which is
         * not in there yet. So we disable the search here (though this
         * obviously means we cannot check if two particles coincide
         * spatially).
         */
        struct ParticleIdentifier {
          static double Precision;

          friend class Database;

          std::string                           particleName;
          tarch::la::Vector<Dimensions, double> particleX;

          /**
           * Equals predicate
           *
           * I originally declared this function as
           *
           * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
           * bool operator==(const ParticleIdentifier& rhs) const;
           * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
           *
           * Some variants of C++ however require an external operator (not
           * embedded into class or namespace) to compare two objects. We have
           * seen issues with Intel 2023, e.g., while 2024 was absolutely fine.
           * It might also be due to a lack of C++2x flags. Anyway, I decided
           * to define an equals operation and to define the operator== and
           * operator!= using this function.
           *
           * To some degree, this is actually the cleaner solution, as it
           * highlights the difference to the < operator. This equality
           * routine and the comparison operator are not compatible, as the
           * present routine is purely floating point error-aware, while the
           * comparison operator uses bit-wise inequality.
           */
          bool numericalEquals( const ParticleIdentifier& rhs ) const;
          bool operator<(const ParticleIdentifier& rhs) const;

          std::string toString() const;

          private:
            /**
             * Never create a particle identifier manually
             *
             * Instead, use Database::createParticleIdentifier() to construct
             * one. This routine takes care of floating point in accuracies.
             */
            ParticleIdentifier(const std::string& particleName__, const tarch::la::Vector<Dimensions, double>& particleX__);
        };

        /**
         * Event
         *
         * An event is an assocation or de-association of a particle and a
         * vertex.
         */
        struct Event {
          enum class Type { NotFound, AssignToSieveSet, AssignToVertex, Erase, DetachFromVertex, MoveWhileAssociatedToVertex };

          Type                                  type;
          bool                                  isLocal;
          tarch::la::Vector<Dimensions, double> vertexX;
          tarch::la::Vector<Dimensions, double> previousParticleX;
          tarch::la::Vector<Dimensions, double> vertexH;
          int                                   treeId;
          std::string                           trace;

          /**
           * Construct an event which identifies a vertex assignment or the removal from a vertex.
           */
          Event(
            Type                                          type_,
            bool                                          isLocal_,
            const tarch::la::Vector<Dimensions, double>&  vertexX_,
            const tarch::la::Vector<Dimensions, double>&  vertexH_,
            int                                           treeId_,
            const std::string&                            trace_
          );

          /**
           * Construct a move event
           *
           * This event should always be tied to an identifier in the global map
           * which uses the new position as key.
           */
          Event(
            Type                                          type_,
            const tarch::la::Vector<Dimensions, double>&  previousParticleX,
            int                                           treeId_,
            const std::string&                            trace_
          );

          /**
           * Construct an event that identifies the association to the sieve set or an erase.
           */
          Event(Type type_, bool isLocal_, int treeId_,             const std::string& trace_);

          /**
           * Construct an invalid event. Is used as return value if no data is
           * found in the database.
           */
          Event(Type type_);

          std::string toString() const;
        };

        using ParticleEvents = std::vector<Event>;

        /**
         * Mesh sweep data set
         *
         * All the traces bookkept for one single mesh sweep. This is a plain
         * extension of the std::map which simply adds a unique name on top.
         * So all the original semantics of a map are preserved.
         *
         * Please see ParticleIdentifier for an in-depth discussion of the
         * comparison key to be used here.
         */
        class MeshSweepData: public std::map<ParticleIdentifier, ParticleEvents> {
          public:
            MeshSweepData(const std::string& meshSweepName);

            std::string getName() const;
          private:
            const std::string _meshSweepName;
        };

        class Database {
          private:
            tarch::multicore::MultiReadSingleWriteSemaphore  _semaphore;

            std::list<MeshSweepData> _data;

            /**
             * Of how many previous associations do we want to keep track
             *
             * If we do not clean up the database every now and then, we
             * will run into situations where the memory footprint exceeds the
             * available mem. More importantly, the validation steps will last
             * for ages. So we clean up the database of old data every now and
             * then. This threshold indicates how many old data entries to keep.
             * Set it to std::numerics<int>::max() to disable this type of
             * garbage collection on the history.
             */
            int _maxParticleSnapshotsToKeepTrackOf;

            /**
             * Remove the particle trajectory that ends up in lastIdentifier
             *
             * This is typically invoked by addEvent(). It is not thread-safe,
             * i.e. does not acquire a write lock.
             */
            void removeTrajectory(
              const ParticleIdentifier&  lastIdentifier,
              int                        spacetreeId,
              int                        firstNRecentEntriesToSkip = 0
            );

            /**
             * Not thread-safe
             *
             * Run through the individual snapshots and remove those trajectory
             * entries which are now empty. If a whole snapshot becomes empty, we
             * delete it but if and only if it is not the very last one. If we
             * deleted the very last one, we'd confuse the timeline of some
             * particles as the ordering of the snapshots carries temporal
             * causalities.
             */
            void removeEmptyDatabaseSnapshots();

          public:
            static constexpr int SearchWholeDatabase = 0;
            static constexpr int AnyTree = -1;
            static constexpr int DoNotFollowParticleMovementsInDatabase = -2;

            Database( int maxParticleSnapshotsToKeepTrackOf = 16 );

            /**
             * Create a particle identifier
             *
             * This routine runs through the whole database and looks for a
             * particle identifier which is equal to the parameters. Equal here
             * uses the comparison operator of ParticleIdentifier::operator==()
             * and hence is floating-point aware.
             *
             * If it finds no matching entry, it returns a new identifier with
             * the precise arguments. If it finds one "close enough", it returns
             * this one instead of one matching the precise arguments. See
             * ParticleIdentifier's discussion on map storage for further
             * details.
             *
             * Cannot be const as we have a semaphore to be thread-safe.
             *
             * @param fitToExistingParticles is the search tolerance that we
             *   use when we crawl through the database.
             */
            ParticleIdentifier createParticleIdentifier(
              const std::string&                            particleName,
              const tarch::la::Vector<Dimensions, double>&  particleX,
              double                                        searchTolerance = ParticleIdentifier::Precision
            );

            /**
             * Add a new snapshot for a new mesh sweep
             */
            void startMeshSweep(const std::string&  meshSweepName);

             /**
              * Return last entry matching identifier
             *
             * This routine runs backwards (from new to old) through the
             * database and searches for a particle entry with the specified
             * position and name. If there's none or the last logged entry for
             * this particle is an erase event, then the routine returns an
             * invalid event.
             *
             * We expect that identifier has been constructed through
             * createParticleIdentifier() to accommodate floating-point inaccuracies.
             *
             * Cannot be const as we have a semaphore to be thread-safe.
             *
             * If the routine encounters a move particle, it changes the
             * identifier and invokes itself recursively for this identifier.
             * This way, we can follow a particle through the database even
             * though it has changed its position in space. If the find
             * information on a previous position, this previous position is held
             * as raw floating point data, i.e. might differ from the position
             * use in the database to identify previous particle states by
             * floating point accuracy. So we have to make them match before we
             * continue to search recursively.
             *
             * ## Implementation
             *
             * I originally thought that I have to release the lock before I
             * jump into the recursion. This is however not necessary, as we
             * are working with read/write locks and I can have an arbitrary
             * number of read locks in this case. Actually, it would be
             * outrightly wrong to free it, as someone might alter the data
             * structure in-between a function and its recursive re-invocation.
             * So we have to keep it lock until we return.
             *
             *
             * @param spacetreeId Either a valid spacetree or AnyTree if you don't care
             *
             * @return tuple of event and identifier. In most cases, the
             *   identifier returned will equal the parameter identifier.
             *   However, if the particle has moved, the result identifier
             *   will identify the spatial position corresponding to the
             *   returned event.
             */
            std::pair< Event, ParticleIdentifier > getEntry(
              const ParticleIdentifier&  identifier,
              int                        spacetreeId,
              int                        firstNRecentEntriesToSkip = SearchWholeDatabase
            );

            /**
             * Print history of one particle
             *
             * Cannot be const as we have a semaphore to be thread-safe.
             *
             * The particle runs backwards through the database and plots all
             * fitting identifiers. When we encounter a move, we memorise it.
             * Once at the end of the database, we look if there has been a
             * move before. If so, we call the search again for this previous
             * particle position. When we construct the particle identifier
             * for the tail recursion, we have to take into account that the
             * previous position is stored as raw position data, but the
             * database's entries might be slightly off due to truncation
             * errors.
             *
             * If there are multiple moves, we only take the first one into
             * account. Multiple moves might occur if several particles
             * "hop through" the same spatial position in the database. Only
             * the most recent one however is relevant for identifier.
             */
            std::string particleHistory(const ParticleIdentifier& identifier);

            std::string lastMeshSweepSnapshot();

            /**
             * Count total number of entries for a particle
             *
             * Cannot be const as we have a semaphore to be thread-safe.
             */
            int totalEntries(const ParticleIdentifier& identifier);

            /**
             * Return number of snapshots
             *
             * These are the active snapshots still held in the database.
             * addEntry()'s garbage collection might have removed some of
             * them already.
             */
            int getNumberOfSnapshots() const;

            /**
             * Add a new event to the database
             *
             * If there is not yet an entry for identifier in the current
             * snapshot, we create one. Afterwards, we append event to this
             * new dataset for particle identifier and clean up the database,
             * i.e. throw away irrelevant old data.
             *
             *
             * ## Garbage/history collection
             *
             * Eventually, it is time to clean up the database. This happens if
             * and only if we have a maximum database size set in
             * _maxParticleSnapshotsToKeepTrackOf.
             *
             * - If the new event is an assignment, then we know that now
             *   future request will ever look backwards in history beyond this
             *   assignment. So we can delete all of this historic data.
             * - If we have tons of particle moves switching vertices (and
             *   hence deleting and re-associating particles), we can eliminate
             *   all of those and instead add two events: one that attaches a
             *   particle and one that moves it in one go.
             *
             * I originally thought that I'd also like to remove old data
             * entries whenever we encounter an erase event. However, this is
             * not necessary: If we have an erase event (for an expired halo
             * element), we'll very likely get a new assignment in a second.
             * This assignment will then eliminate previous "expired"
             * information as per the description above.
             */
            void addEvent(ParticleIdentifier identifier, const Event& event);

            /**
             * Dump the whole database
             *
             * Cannot be const as we have a semaphore to be thread-safe.
             */
            std::string toString();

            /**
             * Eliminate existing particles, so only those that are "forgotten" remain
             *
             * This routine runs through the particles for which we have
             * recorded events in the last snapshot. If the event
             *
             * - affects a remote particle;
             * - is a move;
             * - is an assignment to vertex,
             *
             * we know that it cannot eliminate existing particles. So we
             * remote their trajectories via removeTrajectory(). If the
             * last snapshot is empty, we remove it completely.
             *
             * After a sweep through the last snapshot, we run through the
             * algorithm again, maybe eliminating further data (if we got
             * rid of a snapshot).
             *
             * Once this routine has terminated, we get the stuff in the
             * database for particles that seem to have been left over. We also
             * get a lot of historic data, but if we plot the database and
             * study solely the last snapshot, we typically get a good idea of
             * which particles got lost.
             */
            void eliminateExistingParticles();

        };
      } // namespace internal

      /**
       * Inform API that this is the end of a grid run-throuch
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
        const std::string&                            particleName,
        const tarch::la::Vector<Dimensions, double>&  particleX,
        bool                                          isLocal,
        int                                           treeId,
        const std::string&                            trace
      );

      /**
       * Record the movement of a particle
       *
       * We insert a new entry which memorises the particle movements. However,
       * we don't want to hold arbitrary small wiggles. So we construct the old
       * and the new particle identifier (name + position) and if they are
       * different, subject the ParticleIdentifier comparison operator and its
       * precision, then we log the new event.
       * If oldParticleX however equals newParticleX, the function degenerates to nop
       * and nothing is traced.
       *
       * This means that we might skip a few (tiny) particle movements. So if
       * we finally search for the origin particle, we have to pick a bigger
       * tolerance. After all, we might have skipped quite a lot of moves. In
       * theory, it should be enough if we increased the tolerance by a factor
       * of two. In practice, this is still too restrictive. We did
       * make good experiences with increasing the tolerance by one order of
       * magnitude.
       *
       * As we work with this tolerances and therefore skip particles, we have
       * to be careful which old particle x we dump into the database. We
       * could use oldParticleX, but we have just discussed above that we
       * search for a corresponding entry which might be off slightly.
       * Therefore, we do not bookmark oldParticleX -> newParticleX, but
       * instead store found old particleX -> newParticleX.
       */
      void moveParticle(
         const std::string&                            particleName,
         const tarch::la::Vector<Dimensions, double>&  oldParticleX,
         const tarch::la::Vector<Dimensions, double>&  newParticleX,
         int                                           treeId,
         const std::string&                            trace
      );

      /**
       * Eliminate all "live" particles from database
       *
       * This routine can be used to find "lost" particles. It runs through the
       * database and eliminates all those particles which do still exist.
       *
       * This routine is not called routinely, as it can be very expensive.
       *
       * @param numberOfParticles Specify the number of particles you are
       *   searching for or pass in a negative number to indicate that all
       *   particles still have to be in the domain.
       *
       * @param eliminateExistingParticlesFromDatabase This operation prunes
       *   the database and speeds up the validation process significantly.
       */
      void eliminateExistingParticles();

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
       *   reduce a NotFound.
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
       *   and then readds it with the local flag set.
       *
       *
       * ### Consistency check for virtual particles
       *
       * - The particle comes in through the boundary as part of the boundary
       *   exchange. In this case, the particle has to be "free", i.e. it may
       *   not be assigned to any other vertex yet. The particle has to be new
       *   (NotFound in database).
       * - If it is an update of the halo copy, we would expect the
       *   corresponding merge() to replace this copy. A replacement manifests
       *   in an event sequence of detach->erase->assign, so we still get a
       *   NotFound throughout the assign.
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
       *                      need to search through the database.
       */
      void assignParticleToVertex(
        const std::string&                           particleName,
        const tarch::la::Vector<Dimensions, double>& particleX,
        bool                                         isLocal,
        const tarch::la::Vector<Dimensions, double>& vertexX,
        const tarch::la::Vector<Dimensions, double>& vertexH,
        int                                          treeId,
        const std::string&                           trace,
        bool                                         particleIsNew = false
      );


      /**
       * Remove particle from vertex
       *
       * To make this API call valid, the particle has to be assigned to a
       * vertex beforehand. So there has to be a assignParticleToVertex()
       * before this one.
       */
      void detachParticleFromVertex(
        const std::string&                           particleName,
        const tarch::la::Vector<Dimensions, double>& particleX,
        bool                                         isLocal,
        const tarch::la::Vector<Dimensions, double>& vertexX,
        const tarch::la::Vector<Dimensions, double>& vertexH,
        int                                          treeId,
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
        const std::string&                            particleName,
        const tarch::la::Vector<Dimensions, double>&  particleX,
        bool                                          isLocal,
        int                                           treeId,
        const std::string&                            trace
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
      template <typename Particle>
      std::string pruneTypeName() {
        std::string result    = typeid(Particle).name();
        std::size_t lastDigit = result.find_last_of("0123456789");
        if (lastDigit != std::string::npos) {
          result = result.substr(lastDigit + 1);
        }
        assertion2(result.size() > 1, typeid(Particle).name(), result);
        return result.size() > 1 ? result.substr(0, result.size() - 1) : result;
      }

      /**
       * Ensure that database is empty
       *
       * This one is usually called after eliminateExistingParticles() and will
       * leave the user only with particles which have "magically" disappeared.
       *
       * This routine uess exit(-1) if no assertions are enabled.
       */
      void ensureDatabaseIsEmpty();
    } // namespace assignmentchecks
  }   // namespace particles
} // namespace toolbox


/**
 * Comparison operator
 *
 * Some older variants of C++ require an external operator (not embedded
 * into class or namespace) to compare two objects. We have seen issues
 * with Intel 2023, e.g., while 2024 was absolutely fine. Delegates to
 * member function.
 */
bool operator==(const toolbox::particles::assignmentchecks::internal::ParticleIdentifier& lhs, const toolbox::particles::assignmentchecks::internal::ParticleIdentifier& rhs );
bool operator!=(const toolbox::particles::assignmentchecks::internal::ParticleIdentifier& lhs, const toolbox::particles::assignmentchecks::internal::ParticleIdentifier& rhs );

