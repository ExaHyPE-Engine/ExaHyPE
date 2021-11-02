// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TOOLBOX_PARTICLE_TRAJECTORY_DATABASE_
#define _TOOLBOX_PARTICLE_TRAJECTORY_DATABASE_


#include "tarch/la/Vector.h"
#include "tarch/logging/Log.h"
#include "tarch/multicore/BooleanSemaphore.h"
#include "peano4/utils/Globals.h"

#include <string>
#include <map>
#include <list>
#include <iomanip>

namespace toolbox {
  namespace particles {
    class TrajectoryDatabase;
  }
}


/**
 * A simple particle database
 *
 * The database can be configured with various thresholds such that we write
 * out snapshots whenever a certain maximum size is reached.
 *
 * The database is thread-safe.
 */
class toolbox::particles::TrajectoryDatabase {
  private:
    static tarch::logging::Log _log;

    std::string _fileName;
    double      _dataDelta;
    double      _positionDelta;
    double      _maxDataDelta;
    double      _maxPositionDelta;
    int         _numberOfDataPointsPerParticle;

    const int   _deltaBetweenTwoDatabaseFlushes;
    int         _thresholdForNextDatabaseFlush;
    int         _precision;
    /**
     * @see TrajectoryDatabase()
     */
    const bool  _clearDatabaseAfterFlush;

    const bool  _deltasAreRelative;

    static constexpr double _deltaCutOffThreshold = 1e-6;

    /**
     * This is a hack: Usually, I'd just ask the rank what its number is.
     * However, the database dump often is called in the very end, after
     * the MPI rank is already down. So it will return -1. So what I do is
     * that I store this piece of data whenever I insert a new entry.
     */
    int         _rank;

    struct Entry {
      tarch::la::Vector<Dimensions,double>  x;
      double                                timestamp;
      double*                               data;

      Entry( const TrajectoryDatabase& database, const tarch::la::Vector<Dimensions,double>&  x_, double  timestamp_ );
    };

    std::map< std::pair<int,int>, std::list<Entry> >  _data;

    tarch::multicore::BooleanSemaphore        _semaphore;

    enum class AddSnapshotAction {
      Ignore,
      Append,
      Replace
    };

    /**
     * Can't be const as it might augment the database with default entries.
     *
     * <h2> Thread-safety </h2>
     *
     * The routine is not thread-safe, i.e. you have to protect it from outside.
     */
    AddSnapshotAction getAction(
      const std::pair<int, int>&                  number,
      const tarch::la::Vector<Dimensions,double>& x,
      double                                      timestamp
    );

    AddSnapshotAction getAction(
      const std::pair<int, int>&                   number,
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       timestamp,
      int                                          numberOfDataEntries,
      double*                                      data
    );

    /**
     *
     * <h2> Thread-safety </h2>
     *
     * The routine is thread-safe, as it locks the database. As it uses the
     * semaphore, it can't be const.
     *
     * @return If the user should write the database into a file. This is the case
     *   if the total number of entries in the database exceeds
     *   _thresholdForNextDatabaseFlush.
     */
    bool dumpDatabaseSnapshot();
  public:
    /**
     * Trajectory database
     *
     * The database dumps/stores data if and only if the delta of two particles is
     * bigger than a threshold. We always work with the max norm. There's two different
     * thresholds: one for the position, one for the data. So whenever a particle
     * moves by more than positionDelta in any component of its position, we write a
     * new snapshot of the particle. Whenever one of the particle's values changes
     * by more than  dataDelta in one of its components, we write a new snapshot of
     * this particle (even though it might not have moved).
     *
     * <h2> Flushing the database </h2>
     *
     * Please read the documentation of clearDatabaseAfterFlush and
     * growthBetweenTwoDatabaseFlushes below first. If you flush a database every
     * now and then and if you clear the database after that, then the following
     * situation can happen: One particle's data or position changes quite a lot.
     * Another particle's data doesn't change at all. We trigger a flush and, after
     * that, clear the database. Again, the one single particle is updated quite a
     * lot. We flush again. The particle that doesn't change et al will not be
     * contained in the second database snapshot.
     *
     *
     *
     * @param clearDatabaseAfterFlush If this flag is set, each flush of the database
     *   will go into a separte file, and the code will clear the database after the
     *   flush. As a consequence, the database will never exceed the memory. However,
     *   you'll get a lot of files that you have to merge afterwards.
     * @param growthBetweenTwoDatabaseFlushes Whenver a database has been added more
     *   than growthBetweenTwoDatabaseFlushes entries, we dump a snapshot. This can be
     *   growthBetweenTwoDatabaseFlushes different particles, or growthBetweenTwoDatabaseFlushes
     *   updates for only one single particle. In both cases, we flush.
     * @param positionDelta The code dumps a new particle if and only if it is not in
     *   the database or if its position differs from the last positon tracked by more
     *   than positionDelta.
     * @param dataDelta See positionDelta, but this time we analyse the data held by
     *   the particle.
     * @param deltasAreRelative By default (flag is false), we take the absolute deltas
     *   of the position or data to make a decision if to dump a particle or not. If
     *   this flag is set however, we track the maximum of the deltas, and we dump
     *   data if and only if it exceeds positionDelta times this maximum. So we use a
     *   relative quantity.
     */
    TrajectoryDatabase( int growthBetweenTwoDatabaseFlushes, double positionDelta = 1e-8, double dataDelta = 1e-8, bool clearDatabaseAfterFlush=true, bool deltasAreRelative=false );
    ~TrajectoryDatabase();

    /**
     * <h2> Thread-safety </h2>
     *
     * The clear() operation is thread-safe if you set lockSemaphore. In this
     * case, it first locks the sempahore and then it continues.
     */
    void clear(bool lockSemaphore=true);

    /**
     * <h2> Thread-safety </h2>
     *
     * The operation is thread-safe, i.e. it first locks the sempahore and then
     * it continues. If we are supposed to clear the database once we've dumped the CSV
     * file, we will call clear(). In this case, it is important that I keep the
     * lock up and call clear(). If I released the lock and then called clear,
     * some other thread might squeeze its particle update in-between and we'd
     * loose the information.
     *
     */
    void dumpCSVFile();

    void setOutputFileName( const std::string& filename );
    void setOutputPrecision( int precision );
    void setDataDeltaBetweenTwoSnapshots( double value );
    void setPositionDeltaBetweenTwoSnapshots( double value );

    /**
     * A particle is always uniquely identified by two integers (an
     * integer pair). This way, we can initialise (hand out) particle
     * numbers without any semaphore.
     *
     *
     * <h2> Thread-safety </h2>
     *
     * The routine is thread-safe. It actually locks the database before it
     * invokes getAction() and thus will not mess up either the database
     * analysis or any writes.
     *
     * If the database is configured to write snapshots, the routine will
     * also invoke the dump. However, it si important that we free the lock
     * before, as I do not use recursive locks and as the dumping itself
     * is thread-safe.
     */
    void addParticleSnapshot(
      const std::pair<int, int>&                   number,
      double                                       timestamp,
      const tarch::la::Vector<Dimensions,double>&  x
    );

    void addParticleSnapshot(
      int                                          number0,
      int                                          number1,
      double                                       timestamp,
      const tarch::la::Vector<Dimensions,double>&  x
    );

    void addParticleSnapshot(
      const std::pair<int, int>&                   number,
      double                                       timestamp,
      const tarch::la::Vector<Dimensions,double>&  x,
      int                                          numberOfDataEntries,
      double*                                      data
    );

    void addParticleSnapshot(
      int                                          number0,
      int                                          number1,
      double                                       timestamp,
      const tarch::la::Vector<Dimensions,double>&  x,
      int                                          numberOfDataEntries,
      double*                                      data
    );
};


#endif
