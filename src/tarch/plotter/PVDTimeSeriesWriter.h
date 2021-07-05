// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_PLOTTER_PVD_TIME_SERIES_WRITER_H_
#define _TARCH_PLOTTER_PVD_TIME_SERIES_WRITER_H_


#include <string>
#include <sstream>
#include <vector>

#include "tarch/logging/Log.h"


namespace tarch {
  namespace plotter {
    class PVDTimeSeriesWriter;
  }
}


/**
 * Writes time series in vtk/vtu format. According to the VTK spec, time series
 * are solely written serially, i.e. you may use this class on any rank, but it
 * only writes out data on Peano's global master.
 *
 * Please note that this file format does not work with VTK files. It only works
 * with the XML variants of the writer.
 */
class tarch::plotter::PVDTimeSeriesWriter {
  private:
    /**
     * @return double Latest timestamp
     * @return int    Counter of partitions in last snapshot
     * @return lines  Set of lines
     */
    static std::tuple< double, int, std::vector<std::string> > parseFile( const std::string& filename );

    /**
     * @param lines content without the tail of the format, i.e. the closing brackets
     */
    static void writeFile( const std::string& filename, const std::vector<std::string>& lines );

    static void addFileTail( std::vector<std::string>& lines );
    static void removeFileTail( std::vector<std::string>& lines );

    static std::string createFileEntry( const std::string& snapshotFileName, double timestamp, int partCounter );
  public:
    enum class IndexFileMode {
      /**
       * Create new meta file. If a meta data file exists already, we overwrite
       * it.
       */
      CreateNew,
      /**
       * Add new data. This usually adds a new file to the current snapshot
       * (time step).
       */
      AppendNewData,
      /**
       * Don't use a meta data file.
       */
      NoIndexFile
    };

    /**
     * @param dataFile  First file entry to insert. Pass an empty string if you don't want Peano to
     *   add a file into the original index file.
     *
     * @see IndexFileMode
     */
    static void createEmptyIndexFile( const std::string& dataFile );
    static void appendNewData(const std::string& snapshotFileName, const std::string& dataFile, double timeStamp);

    static double getLatestTimeStepInIndexFile( std::string dataFile );
};


#endif

