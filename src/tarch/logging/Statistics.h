// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_LOGGING_STATISTICS_H_
#define _TARCH_LOGGING_STATISTICS_H_


#include <string>
#include <map>
#include <vector>

#include "Log.h"
#include "tarch/timing/Watch.h"
#include "tarch/multicore/BooleanSemaphore.h"


namespace tarch {
  namespace logging {
    class Statistics;
  }
}


class tarch::logging::Statistics {
  public:
    /**
     * This is not the canonical realisation of singletons as I use it usually
     * for stats in Peano.
     */
    static Statistics& getInstance();

    /**
     * Log one particular value
     */
    #ifdef TrackStatistics
    void log( const std::string& identifier, double value );
    #else
    void log( const std::string& identifier, double value ) {}
    #endif

    void writeToCSV( const std::string& filename = "statistics.csv" );

  private:
    static Statistics   _singleton;

    static Log          _log;

    static tarch::multicore::BooleanSemaphore  _semaphore;

    int                   _minCountInBetweenTwoMeasurements;
    double                _minTimeInBetweenTwoMeasurements;

    struct DataSet {
      tarch::timing::Watch  _watch;
      int                   _counter;
      std::vector< std::pair<double,double> >   _data;
      DataSet();
    };

    std::map< std::string, DataSet >  _map;

    Statistics();

    /**
     * Not const, as it also updates the internal counters.
     *
     * @return Something bigger than 0 if new data should be accepted
     */
    double acceptNewData(const std::string& identifier);
};



#endif
