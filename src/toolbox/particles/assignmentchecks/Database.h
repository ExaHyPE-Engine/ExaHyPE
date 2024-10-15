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


namespace toolbox {
  namespace particles {
    namespace assignmentchecks {
      namespace internal {

        using ParticleEvents = std::vector<Event>;

        /**
         * @class Database
         * @brief stores the actual particle histories.
         *
         * Particles are traced via a map, where the keys are
         * instances of the ParticleIdentifier class. The database itself
         * is implemented as a singleton. To access it, use the getInstance()
         * method.
         *
         * The database will track all events on all spacetrees on this
         * MPI rank. Per particle, we store up to
         * _maxParticleSnapshotsToKeepTrackOf particle events. Once that number
         * is exceeded, the past history will be condensed into a single event,
         * signifying that it had been compacted.
         *
         * Additionally, something we come across very often is that particles,
         * once assigned to a vertex, very often stay assigned to the same
         * vertex for a long time (=many sweeps) and only experiences small
         * movements. Rather than tracing every single move event individually,
         * the database will mark consecutive move events using a special event
         * type, Event::Type::ConsecutiveMoveEvent. It will also keep track the
         * original position of the particle, before the first move event of
         * that sequence has occurred, as well as the current particle position.
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
          // TODO: is this still used?
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
           * Return the stored event history of a particle
           *
           * If there's none, this routine returns a history containing
           * an invalid event. This history containing an invalid event
           * will alloc a new ParticleEvents and add a singleevent of
           * type NotFound if there is no history. Make sure to delete
           * that after you use it.
           *
           * Cannot be const as we have a semaphore to be
           * thread-safe. Returns a copy, not a reference, so that once
           * semaphore is lifted, the state of the history remains accurate.
           *
           * @return The history of the particle.
           *
           */
          ParticleEvents getParticleHistory(
            const ParticleSearchIdentifier& identifier
          );

          /**
           * Print history of one particle
           *
           * Cannot be const as we have a semaphore to be thread-safe.
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
           * If the number of events for a particle exceeds the limit set
           * by _maxParticleSnapshotsToKeepTrackOf, this routine will
           * squash them together into a single event signifying that
           * this compression has occurred, while keeping the event type
           * of the last recorded event. The other events in the history
           * will be deleted.
           *
           * @return the last event on the spacetree with id treeId before
           * the newly added one.
           */
          Event addEvent(
            ParticleSearchIdentifier identifier,
            Event&                   event,
            const int treeId
          );

          /**
           * Change the particle's coordinates used to identify it if it moved
           * too much compared to the coordinates stored in its identifier in
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
          std::string sweepHistory();

          /**
           * Eliminate existing particles from the database. See documentation
           * in TracingAPI.h:eliminateExistingParticles() for more.
           */
          void eliminateExistingParticles();

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

        }; // class Database
      }    // namespace internal
    }      // namespace assignmentchecks
  }        // namespace particles
} // namespace toolbox
