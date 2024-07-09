// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <functional> // needed for std::less
#include <map>
#include <vector>

#include "MeshSweepData.h"
#include "ParticleIdentifier.h"
#include "tarch/la/Vector.h"
#include "tarch/multicore/MultiReadSingleWriteSemaphore.h"


#if !defined(AssignmentChecks) and !defined(noAssignmentChecks) \
  and PeanoDebug > 0
#define AssignmentChecks
#endif


// TODO in documentation:
// - checks will fail if particles move too much. They can't have arbitrary
// motions in the grid. THey're limited by the vertexH.
// - database dump and particle history dump will print out last event first

namespace toolbox {
  namespace particles {
    namespace assignmentchecks {
      namespace internal {

        using ParticleEvents = std::vector<Event>;

        /**
         * @class The Database class stores the actual particle histories.
         * Particles are traced via a map, where the keys are
         * instances of the ParticleIdentifier class. The database itself
         * is implemented as a singleton. To access it, use the getInstance()
         * method.
         *
         * The database will track all events on all spacetrees on this
         * MPI rank. Per particle, we store up to
         _maxParticleSnapshotsToKeepTrackOf
         * particle events. Once that number is exceeded, the past history will
         * be condensed into a single event, signifying that it had been
         compacted.

         * Additionally, something we come across very often is that particles,
         * once assigned to a vertex, very often stay assigned to the same
         * vertex for a long time (=many sweeps) and only experiences small
         movements.
         * Rather than tracing every single move event individually, the
         database
         * will mark consecutive move events using a special event type,
         * Event::Type::ConsecutiveMoveEvent. It will also keep track the
         original
         * position of the particle, before the first move event of that
         sequence
         * has occurred, as well as the current particle position.
         */

        class Database {

          // Singleton-related stuff first, actual class content later.
        public:
          static Database& getInstance() {

            static Database _database;
            return _database;
          }

          // C++ 11 way of doing things. Explicitly delete the methods
          // we don't want.
          Database(const Database&)       = delete;
          void operator=(const Database&) = delete;

        private:
          // Make constructor private so nobody can make one by accident.
          Database() {
            setMaxParticleSnapshotsToKeepTrackOf(16);
            this->reset();
          };


        private:
          tarch::multicore::MultiReadSingleWriteSemaphore _semaphore;

          std::vector<MeshSweepData> _meshSweepData;
          int                        _currentMeshSweepIndex;

          // tell the map to use std::less<> as comparator instead of the
          // default std::less<key>. std::less<> invokes the `<` operator.
          // We want that to be able to use our bespoke `<` operator
          // for different search and ID key identifier objects.
          std::map<ParticleIdentifier, ParticleEvents, std::less<>> _data;

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
          size_t _maxParticleSnapshotsToKeepTrackOf;

        public:
          static constexpr int SearchWholeDatabase                    = 0;
          static constexpr int AnyTree                                = -1;
          static constexpr int DoNotFollowParticleMovementsInDatabase = -2;

          /**
           * Sets the number of snapshots to keep in the database.
           */
          void setMaxParticleSnapshotsToKeepTrackOf(const size_t n);

          /**
           * Add an entry for a new mesh sweep
           */
          void startMeshSweep(const std::string& meshSweepName);

          /**
           * TODO: fix docu here.
           * Return last entry matching identifier
           *
           * This routine runs backwards (from new to old) through the
           * database and searches for a particle entry with the specified
           * position and name. If there's none or the last logged entry for
           * this particle is an erase event, then the routine returns an
           * invalid event.
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
           * @param spacetreeId Either a valid spacetree or AnyTree if you don't
           * care
           *
           * @return tuple of event and identifier. In most cases, the
           *   identifier returned will equal the parameter identifier.
           *   However, if the particle has moved, the result identifier
           *   will identify the spatial position corresponding to the
           *   returned event.
           *
           *   --------------------
           *   Will alloc a new ParticleEvents and add a single
           *   event of type NotFound if there is no history.
           *   Make sure to delete that after you use it.
           */
          ParticleEvents& getParticleHistory(
            const ParticleSearchIdentifier& identifier
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
           *
           * --------------------------------
           * Events of type NotFound will not be added to the history.
           * TODO: rename
           */
          std::string particleHistory(const ParticleSearchIdentifier& identifier
          );

          /**
           * Returns the recorded events of the latest mesh sweep.
           */
          std::string lastMeshSweepSnapshot();

          /**
           * Count total number of entries for a particle
           *
           * Cannot be const as we have a semaphore to be thread-safe.
           */
          int getTotalParticleEntries(const ParticleSearchIdentifier& identifier
          );

          /**
           * Return number of snapshots
           *
           * These are the active snapshots still held in the database.
           * addEvent()'s garbage collection might have removed some of
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
           *
           * ------------------------------------------
           *  - will refresh identifier if moved past tolerance
           *  - will delete events if there are too many
           *  - returns ref to particleEvents
           */
          ParticleEvents& addEvent(
            ParticleSearchIdentifier identifier,
            Event&                   event
          );

          /**
           * Change the particle's coordinates if it moved too much
           * compared to the coordinates stored in its identifier in
           * the database.
           */
          void shiftIdentifierCoordinates(
            ParticleSearchIdentifier              identifier,
            tarch::la::Vector<Dimensions, double> newParticleX
          );

          /**
           * Dump the whole database
           *
           * Cannot be const as we have a semaphore to be thread-safe.
           */
          std::string toString();

          /**
           * Delete all entries in the database and reset it to the initial
           * state. Then re-initialize the database.
           */
          void reset();

          /**
           * Grab the current index of mesh sweeps.
           */
          size_t getCurrentMeshSweepIndex() const;

          /**
           * Get the number of currently traced particles in the database.
           */
          int getNumberOfTracedParticles() const;

          /**
           * Get a handle on the mesh sweep data of the database.
           * You shouldn't ever need this in your code. This only exists to
           * verify the database integrity in unit tests.
           */
          std::vector<MeshSweepData>& getMeshSweepData();

          /**
           * Get the history of stored mesh sweep (names).
           */
          std::string sweepHistory() const;
        };

        /**
         * get the previous event from a particle history.
         *
         * nFirstEventsToSkip: skip this many events. Useful if you've just
         * added something to the database using addEvent and are trying to
         * re-use the history it returns.
         */
        Event getPreviousEvent(
          ParticleEvents& particleHistory,
          int             spacetreeId,
          size_t          nFirstEventsToSkip = 0
        );

      } // namespace internal
    }   // namespace assignmentchecks
  }     // namespace particles
} // namespace toolbox
