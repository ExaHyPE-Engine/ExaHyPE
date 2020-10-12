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
     * @return Tuple of snapshot counter (time step), groups, and lines within the file. The lines
     *         do not comprise the two closing tags.
     */
    static std::tuple< int, int, std::vector<std::string> > parseFile( const std::string& filename );

    /**
     * @param lines content without the tail of the format, i.e. the closing brackets
     */
    static void writeFile( const std::string& filename, const std::vector<std::string>& lines );

    static void addFileTail( std::vector<std::string>& lines );
    static void removeFileTail( std::vector<std::string>& lines );

    static std::string createFileEntry( const std::string& snapshotFileName, int snapshotCounter, int partCounter );

    /**
     * Ensure the file we want to add to the pvd file is a fit to Paraview. If
     * not, this routine writes a warning.
     */
    static void validateFile( const std::string& filename );
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
    static void appendNewDataSet( const std::string& snapshotFileName, const std::string& dataFile );
    static void appendNewData(const std::string& snapshotFileName, const std::string& dataFile);
};


#endif

