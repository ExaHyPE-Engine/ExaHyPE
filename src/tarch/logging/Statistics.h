// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_LOGGING_STATISTICS_H_
#define _TARCH_LOGGING_STATISTICS_H_


#include <string>
#include <map>
#include <vector>
#include <tuple>


#include "Log.h"
#include "tarch/timing/Watch.h"
#include "tarch/multicore/BooleanSemaphore.h"




namespace tarch {
  namespace logging {
    class Statistics;
  }
}


/**
 * Global statistics interface
 *
 * This interface only logs data if you have translated with -DTrackStatistics.
 *
 * To get the stats right, you might want to invoke the clear() operation 
 * explicitly when you start up your code. This way, you ensure that the 
 * internal timer (_globalWatch) is properly initialised.
 */
class tarch::logging::Statistics {
  public:
    /**
     * This is not the canonical realisation of singletons as I use it usually
     * for stats in Peano.
     */
    static Statistics& getInstance();

    /**
     * Log one particular value
     *
     * @param identifier Unique name (string) for this event
     */
    #ifdef TrackStatistics
    void log( const std::string& identifier, double value, bool disableSampling = false );
    void inc( const std::string& identifier, double value = 1.0, bool disableSampling = false );
    #else
    void log( const std::string& identifier, double value, bool disableSampling = false ) {}
    void inc( const std::string& identifier, double value = 1.0, bool disableSampling = false ) {}
    #endif

    /**
     * I do append the extension (csv) and a rank identifier myself.
     */
    void writeToCSV( std::string  filename = "statistics" );

    void clear();
  private:
    static Statistics   _singleton;

    static Log          _log;

    static tarch::multicore::BooleanSemaphore  _semaphore;

    int                   _maxCountInBetweenTwoMeasurements;
    double                _maxTimeInBetweenTwoMeasurements;

    tarch::timing::Watch  _globalWatch;

    /**
     * One data set for one type (identifier) of statistics
     *
     * The watch is used to track time (obviously) and the counter counts the
     * number of hits. If you enable sampling, the counter can trigger writes,
     * i.e. you can specify after how many hits you wanna dump data. The actual
     * data is a five-tuple of time stamp, current value, minimal value over
     * the tracked period and maximum value. The last entry holds the number of
     * writes to the bucket.
     */
    struct DataSet {
      /**
       * Not used for time stamps but only for sampling.
       */
      tarch::timing::Watch  _watch;
      int                   _counter;
      std::vector< std::tuple<double,double,double,double,int> >   _data;
      /**
       * As we hold data sets within a map, we need a default constructor.
       * However, this one yields an invalid data set.
       */
      DataSet();
      DataSet(double time);
      void createNewSnapshot(double t);
    };

    std::map< std::string, DataSet >  _map;

    Statistics();

    /**
     * Not const, as it also updates the internal counters.
     *
     * A new snapshot is written if the number of writes for this data set
     * exceeds _maxCountInBetweenTwoMeasurements or if the time since the
     * last write exceeds _maxTimeInBetweenTwoMeasurements. Furthermore,
     * a new data set is written if the sampling is disabled. In this case,
     * all the local counters are still reset, as you might want to combine
     * manual disabled stats with sampled ones.
     *
     * If the data has not changed since the last snapshot, I don't write
     * anything.
     *
     * @return Write a new snapshot.
     */
    bool acceptNewData(const std::string& identifier, bool disableSampling);

    /**
     * Ensures that dataset is there. If not, it creates a new entry.
     * If there's already an entry for identifier, it degenerates to
     * nop.
     */
    void initData(const std::string& identifier);

    /**
     * Updates snapshot
     */
    void updateDataSnapshot(const std::string& identifier, double value);
};



#endif
