// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_PLOTTER_VTU_TIME_SERIES_WRITER_H_
#define _TARCH_PLOTTER_VTU_TIME_SERIES_WRITER_H_


#include <sstream>
#include "tarch/logging/Log.h"


namespace tarch {
  namespace plotter {
    class VTUTimeSeriesWriter;
  }
}


/**
 * Writes time series in vtk/vtu format. According to the VTK spec, time series
 * are solely written serially, i.e. you may use this class on any rank, but it
 * only writes out data on Peano's global master.
 */
class tarch::plotter::VTUTimeSeriesWriter {
  public:
    enum class IndexFileMode {
      /**
       * Create new meta file.
       */
      CreateNew,
      /**
       * Create a new data set within an existing index file. This is usually
       * used to add a new snapshot (for a new time step for example).
       */
      AppendNewDataSet,
      /**
       * Add new data. This usually adds a new file to the current snapshot
       * (time step).
       */
      AppendNewData,
      NoIndexFile
    };

    /**
     * @see IndexFileMode
     */
    static void createEmptyNewFile( const std::string& snapshotFileName );
    static void appendNewDataSet( const std::string& snapshotFileName, double t );
    static void appendNewData(const std::string& snapshotFileName, const std::string& dataFile);
};


#endif

