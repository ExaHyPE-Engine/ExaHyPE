// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <iomanip>
#include <list>
#include <map>
#include <string>

#include "peano4/utils/Globals.h"
#include "tarch/la/Vector.h"
#include "tarch/logging/Log.h"
#include "tarch/multicore/BooleanSemaphore.h"

namespace toolbox {
  namespace particles {
    class TrajectoryDatabase;
  } // namespace particles
} // namespace toolbox

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

  double _timeDelta;

  const int _deltaBetweenTwoDatabaseFlushes;
  int       _thresholdForNextDatabaseFlush;
  int       _precision;

  /**
   * Flag that indicates if we erase the database after a flush
   *
   * If not set, each dump consists all particle data ever recorded. If set,
   * you will potentially get a huge set of data files, and you will have to
   * concatenate them before you interpret the data.
   *
   * @see TrajectoryDatabase()
   * @see clearDatabaseAfterFlush()
   */
  bool _clearDatabaseAfterFlush;

  bool _deltasAreRelative;

  static constexpr double _deltaCutOffThreshold = 1e-6;

  /**
   * This is a hack: Usually, I'd just ask the rank what its number is.
   * However, the database dump often is called in the very end, after
   * the MPI rank is already down. So it will return -1. So what I do is
   * that I store this piece of data whenever I insert a new entry.
   */
  int _rank;

  double _maxTimestamp;

  struct Entry {
    tarch::la::Vector<Dimensions, double> x;
    double                                timestamp;
    double*                               data;
    /**
     * Have to memorise this guy to be able to write a proper copy
     * constructor.
     */
    const int numberOfDataEntries;

    Entry(const Entry&);
    ~Entry();
    Entry(const TrajectoryDatabase& database, const tarch::la::Vector<Dimensions, double>& x_, double timestamp_);
  };

  std::map<std::pair<int, int>, std::list<Entry>> _data;

  tarch::multicore::BooleanSemaphore _semaphore;

  enum class AddSnapshotAction { Ignore, Append, Replace };

  /**
   * Determine what to do with new entry
   *
   * Can't be const as it might augment the database with default entries.
   *
   * The routine is not thread-safe, i.e. you have to protect it from outside.
   */
  AddSnapshotAction getAction(
    const std::pair<int, int>& number, const tarch::la::Vector<Dimensions, double>& x, double timestamp
  );

  /**
   * Determine what to do with new entry
   *
   * First of all, invoke other getAction() routine. This one will determine
   * if the entry is new or if the positions have changed significantly or if
   * the time stamp means that this entry is due to be plotted. If and only
   * if the other getAction() yields Ignore, then study the data if the data
   * deltas are big enough.
   */
  AddSnapshotAction getAction(
    const std::pair<int, int>&                   number,
    const tarch::la::Vector<Dimensions, double>& x,
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
   *
   * ## Flushing the database
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
   *   will go into a separate file, and the code will clear the database after the
   *   flush. As a consequence, the database will never exceed the memory. However,
   *   you'll get a lot of files that you have to merge afterwards.
   *
   * @param growthBetweenTwoDatabaseFlushes Number of entries that we dump into
   *   the database before it is flushed the next time. Set it to max of an
   *   integer or zero to disable on-the-fly dumps. In this case, the database
   *   is only flushed when the simulation terminates. So, the thresholds
   *   for the data and position deltas determine how often entries and up in
   *   the database, and growthBetweenTwoDatabaseFlushes determines how often
   *   this database is written into a file.
   *
   * @param positionDelta The code dumps a new particle if and only if it is
   *   not in the database or if its position differs from the last position
   *   tracked by more than positionDelta.  Therefore, the flag is similar to
   *   dataDelta but this time it is not the difference in the data that
   *   triggers a dump into the database, but a change of position. Obviously,
   *   these things can be totally independent (and also can be combined), as
   *   particles that move with the domain might not change their value, as
   *   they are tracers, while stationary seismograms usually do not change
   *   their position at all.
   *
   *   If you work with stationary particles, you can set this parameter to
   *   literally anything, as it will never become relevant. If you want the
   *   dump to write a snapshot only after fixed time intervals, set this value
   *   to a very high threshold, so it never kicks in and triggers a data dump.
   *   In this case, only the time stamp differences will issue data writes.
   *
   *   This flag has no impact whatsoever how often the data is dumped into a
   *   file. The frequency of datafile writes is solely controlled via
   *   growthBetweenTwoDatabaseFlushes.
   *
   * @param dataDelta Compare to positionDelta, but this time we analyse the
   *   data held by the particle. A particle is dumped into the database if it
   *   has switched the domain partition or its position has changed more then
   *   delta_between_two_snapshots. See
   *   toolbox::particles::TrajectoryDatabase::getAction() for a description
   *   how the C++ code interprets this threshold, but it is usually the max
   *   norm that is used here. If you set it to a very small number, you'll
   *   get a lot of entries in your database.
   *
   *   This flag has no impact whatsoever how often the data is dumped into a
   *   file. The frequency of datafile writes is solely controlled via
   *   growthBetweenTwoDatabaseFlushes.
   *
   * @param timeDelta this parameter ask the code to dump particle into database after certain time
   *   interval of time_delta_between_two_snapsots, even data and position do not change
   *   during this time interval. You can set the two parameter above to be extremely big
   *   to enforce code dump particle with (roughly) regular time interval.
   *
   *
   *   This flag has no impact whatsoever how often the data is dumped into a
   *   file. The frequency of datafile writes is solely controlled via
   *   growthBetweenTwoDatabaseFlushes.
   *
   *
   * @param deltasAreRelative By default (flag is false), we take the absolute deltas
   *   of the position or data to make a decision if to dump a particle or not. If
   *   this flag is set however, we track the maximum of the deltas, and we dump
   *   data if and only if it exceeds positionDelta times this maximum. So we use a
   *   relative quantity.
   */
  TrajectoryDatabase(
    int    growthBetweenTwoDatabaseFlushes,
    double positionDelta           = 1e-8,
    double dataDelta               = 1e-8,
    double timeDelta               = 0.0,
    bool   clearDatabaseAfterFlush = true,
    bool   deltasAreRelative       = false
  );
  ~TrajectoryDatabase();

  /**
   * <h2> Thread-safety </h2>
   *
   * The clear() operation is thread-safe if you set lockSemaphore. In this
   * case, it first locks the sempahore and then it continues.
   */
  void clear(bool lockSemaphore = true);

  /**
   * This call does not throw away all particles, but it throws away all the history
   * behind the particles.
   */
  void clearHistory(bool lockSemaphore = true);

  /**
   * Dump data into CSV file
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

  void setOutputFileName(const std::string& filename);
  void setOutputPrecision(int precision);
  void setDataDeltaBetweenTwoSnapshots(double value, bool deltasAreRelative = false);
  void setPositionDeltaBetweenTwoSnapshots(double value, bool deltasAreRelative = false);
  void setTimeDeltaBetweenTwoSnapshots(double value);

  void clearDatabaseAfterFlush(bool value);

  /**
   * Add particle snapshot
   *
   * A particle is always uniquely identified by two integers (an
   * integer pair). This way, we can initialise (hand out) particle
   * numbers without any semaphore.
   *
   * I do not really care how many attributes one tracks per particle.
   * We can track none at all (like in this function) or an arbitrary
   * number. This is the reason why this operation is heavily overloaded.
   * It is the user's responsibility to use the addParticleSnapshot()
   * routines in a consistent way.
   *
   * ## Thread-safety
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
    const std::pair<int, int>& number, double timestamp, const tarch::la::Vector<Dimensions, double>& x
  );

  void addParticleSnapshot(int number0, int number1, double timestamp, const tarch::la::Vector<Dimensions, double>& x);

  void addParticleSnapshot(
    const std::pair<int, int>&                   number,
    double                                       timestamp,
    const tarch::la::Vector<Dimensions, double>& x,
    int                                          numberOfDataEntries,
    double*                                      data
  );

  void addParticleSnapshot(
    int                                          number0,
    int                                          number1,
    double                                       timestamp,
    const tarch::la::Vector<Dimensions, double>& x,
    int                                          numberOfDataEntries,
    double*                                      data
  );
};
