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
  namespace blockstructured {
    class GlobalDatabase;
  } // namespace blockstructured
} // namespace toolbox

/**
 * A simple global database
 *
 * The database can be configured with various thresholds such that we write
 * out snapshots whenever a certain maximum size is reached.
 *
 * The database is thread-safe. It was built after the model of the particle Database.
 */
class toolbox::blockstructured::GlobalDatabase {
private:
  static tarch::logging::Log _log;

  std::string _fileName;
  std::string _dataName;

  double  _dataDelta;
  double  _maxDataDelta;
  int     _numberOfGlobalDataPoints;

  double  _timeDelta;

  int     _deltaBetweenTwoDatabaseFlushes;
  int     _thresholdForNextDatabaseFlush;
  int     _precision;

  /**
   * Flag that indicates if we erase the database after a flush
   *
   * If not set, each dump consists of all data ever recorded. If set,
   * you will potentially get a huge set of data files, and you will have to
   * concatenate them before you interpret the data.
   *
   * @see GlobalDatabase()
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
    double                                timestamp;
    double*                               data;
    /**
     * Have to memorise this guy to be able to write a proper copy
     * constructor.
     */
    const int numberOfDataEntries;

    Entry(const Entry&);
    ~Entry();
    Entry(const GlobalDatabase& database, double timestamp_);
  };

  std::list<Entry> _data;

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
    double timestamp
  );

  /**
   * Determine what to do with new entry
   *
   * First of all, invoke other getAction() routine. This one will determine
   * if the entry is the first entry, if it's timestamp is such that it should be added
   * to the database or discarded, and if it is at the same timestamp as the
   * last one (in which case it might need to replace it.)
   * In the latest case, we only replace it if the data actually differs from the
   * existing data.
   */
  AddSnapshotAction getAction(
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
   * Global database
   *
   * The database dumps/stores data if the data is either for a new timestamp which
   * is a sufficient delta after the latest one. If the new data has the same
   * timestamp as a previous one, it will replace it if the data is different by a
   * sufficiently large margin.
   * We always work with the max norm.
   *
   * ## Flushing the database
   *
   * Please read the documentation of clearDatabaseAfterFlush and
   * growthBetweenTwoDatabaseFlushes below first.
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
   * @param dataDelta If two pieces of data are defined for the same timestamp,
   *   this parameter is used to determine whether this is re-defining the data
   *   or whether it is just a duplicate. If the former is true, the existing data
   *   will be replaced with the new definition. In the later case, the data is not
   *   added to the database and is simply discarded.
   *   toolbox::blockstructured::GlobalDatabase::getAction() for a description
   *   how the C++ code interprets this threshold, but it is usually the max
   *   norm that is used here.
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
   *   This flag has no impact whatsoever how often the data is dumped into a
   *   file. The frequency of datafile writes is solely controlled via
   *   growthBetweenTwoDatabaseFlushes.
   *
   * @param deltasAreRelative By default (flag is false), we take the absolute deltas
   *   of the data to decide whether to replace data with duplicates or simply assume
   *   these to be identical.
   *   If this flag is set however, we track the maximum of the deltas, and we dump
   *   data if and only if it exceeds dataDelta times this maximum. So we use a
   *   relative quantity.
   *   This might make sense if one value in your database hovers around a size of 10^-3
   *   and another is in the range of 10^10, in which case using the same absolute delta
   *   for both is not the ideal choice.
   */
  GlobalDatabase(
    int    growthBetweenTwoDatabaseFlushes = 0,
    double dataDelta               = 1e-8,
    double timeDelta               = 0.0,
    bool   clearDatabaseAfterFlush = true,
    bool   deltasAreRelative       = false
  );
  ~GlobalDatabase();

  /**
   * <h2> Thread-safety </h2>
   *
   * The clear() operation is thread-safe if you set lockSemaphore. In this
   * case, it first locks the sempahore and then it continues.
   */
  void clear(bool lockSemaphore = true);


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

  void setDeltaBetweenTwoDatabaseFlushes(const int deltaBetweenTwoDatabaseFlushes);
  void setOutputFileName(const std::string& filename);
  void setDataName(const std::string& dataname);
  void setOutputPrecision(int precision);
  void setDataDeltaBetweenTwoSnapshots(double value, bool deltasAreRelative = false);
  void setTimeDeltaBetweenTwoSnapshots(double value);

  void clearDatabaseAfterFlush(bool value);

  /**
   * Add snapshot
   *
   * The data is assumed to be global, and therefore uniquely identified
   * by its timestamp.
   * We can track any amount of data, such as none at all (like in this function) 
   * or an arbitrary number. Hence the overload.
   * It is the user's responsibility to use the addGlobalSnapshot()
   * routines in a consistent way. If you've added six pieces of data once and you try
   * adding eight pieces in a next call, the code will do what we in the business like to
   * call an "oopsie-woopsie", by which we mean an assertion will fail and your program
   * will crash.
   *
   * ## Thread-safety
   *
   * The routine is thread-safe. It actually locks the database before it
   * invokes getAction() and thus will not mess up either the database
   * analysis or any writes.
   *
   * If the database is configured to write snapshots, the routine will
   * also invoke the dump. However, it is important that we free the lock
   * before, as I do not use recursive locks and as the dumping itself
   * is thread-safe.
   */
  void addGlobalSnapshot(
    double timestamp
  );

  void addGlobalSnapshot(
    double                                       timestamp,
    int                                          numberOfDataEntries,
    double*                                      data
  );

};
