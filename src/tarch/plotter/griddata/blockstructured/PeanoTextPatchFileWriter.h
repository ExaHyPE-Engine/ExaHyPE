// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_PLOTTER_GRID_DATA_BLOCK_STRUCTURED_PEANO_TEXT_PATCH_FILE_WRITER_H_
#define _TARCH_PLOTTER_GRID_DATA_BLOCK_STRUCTURED_PEANO_TEXT_PATCH_FILE_WRITER_H_


#include "tarch/logging/Log.h"
#include "tarch/mpi/BooleanSemaphore.h"
#include "PatchWriter.h"


#include <fstream>
#include <vector>


namespace tarch {
  namespace plotter {
    namespace griddata {
      namespace blockstructured {
        class PeanoTextPatchFileWriter;
      }
    }
  }
}


class tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter: public tarch::plotter::griddata::blockstructured::PatchWriter {
  protected:
    static tarch::logging::Log _log;
    static const std::string HEADER;

    /**
     * This is a global (mpi) semaphore that I use to update/modify the
     * index file over all files. The individual data writes are totally
     * concurrent.
     */
    static tarch::mpi::BooleanSemaphore _sempahore;

    const std::string  _fileName;
    const std::string  _indexFile;

    bool _writtenToFile;

    int  _dimensions;

    int _patchCounter;

    std::stringstream _snapshotFileOut;
    bool              _haveWrittenAtLeastOnePatch;

    void writeMetaData(const std::string& metaData);
    void writeMapping(int totalEntries, double* values);

    void createEmptyIndexFile();
    void addNewDatasetToIndexFile();
    void addNewFileToCurrentDataSetInIndexFile( const std::string&  filename );

    /**
     * Adds a new dataset
     */
//    static void addDataFileToIndexFile( const std::string& filename );

    /**
     * Parallel counterpart of addDataFileToIndexFile. Here, you pass a whole
     * set of files that logically belong together. They typically are written
     * by a number of trees concurrently.
     */
//    static void addDatasetToIndexFile( const std::vector< std::string >& filenames );

  public:
    class CellDataWriter: public tarch::plotter::griddata::blockstructured::PatchWriter::CellDataWriter {
      protected:
        tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter& _writer;

        const std::string _identifier;
        const int         _numberOfCellsPerAxis;
        const int         _numberOfUnknowns;
        int               _entryCounter;
        std::stringstream _out;

        void flushIfPatchIsComplete();
      public:
        CellDataWriter(
          const std::string& identifier,
          int                unknownsPerAxis,
          int                numberOfUnknowns,
          const std::string& metaData,
          double*            mapping,
          tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter& writer
        );
        virtual ~CellDataWriter();

        int getCellsPerPatch() const;

        /**
          * Write data for one cell.
          *
          * @param index Index of the cell. This index has to equal the index
          *              used for the cell within the VTKWriter class
          *              interface.
          * @param value Value for the cell.
          */
         void plotCell( int index, double value ) override;
         void plotCell( int index, double* values ) override;

         void close() override;

         void assignRemainingCellsDefaultValues() override;

         int getFirstCellWithinPatch(int index) const override;
     };

    class VertexDataWriter: public tarch::plotter::griddata::blockstructured::PatchWriter::VertexDataWriter {
       protected:
         tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter& _writer;

         const std::string _identifier;
         const int         _numberOfVerticesPerAxis;
         const int         _numberOfUnknowns;

         /**
          * Number of entries written within a patch.
          */
         int               _entryCounter;
         std::stringstream _out;

         void flushIfPatchIsComplete();
       public:
         VertexDataWriter(
           const std::string& identifier,
           int                unknownsPerAxis,
           int                numberOfUnknowns,
           const std::string& metaData,
           double*            mapping,
           tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter& writer
         );
         ~VertexDataWriter();

         int getVerticesPerPatch() const;

         /**
          * Write data for one cell.
          *
          * @param index Index of the vertex. This index has to equal the index
          *              used for the cell within the VTKWriter class
          *              interface.
          * @param value Value for the cell.
          */
         void plotVertex( int index, double value ) override;
         void plotVertex( int index, double* values ) override;

         void close() override;

         /**
          * @see close()
          */
         void assignRemainingVerticesDefaultValues() override;

         int getFirstVertexWithinPatch(int index) const override;
    };

    enum class IndexFileMode {
      CreateNew,
      AppendNewDataSet,
      AppendNewData,
      NoIndexFile
    };

    /**
     * @param fileName      Name of the file to write. Has to be unique per
     *                      dump.
     * @param indexFileName Name of the index file. Can we empty if you select
     *                      NoIndexFile.
     */
    PeanoTextPatchFileWriter(int dimension, const std::string&  fileName, const std::string&  indexFileName, IndexFileMode appendToIndexFile);
    virtual ~PeanoTextPatchFileWriter();

    /**
     * Caller has to destroy this instance manually.
     */
    CellDataWriter*    createCellDataWriter( const std::string& identifier, int unknownsPerAxis, int recordsPerCell ) override;
    CellDataWriter*    createCellDataWriter( const std::string& identifier, int unknownsPerAxis, int recordsPerCell, const std::string& metaData ) override;
    CellDataWriter*    createCellDataWriter( const std::string& identifier, int unknownsPerAxis, int recordsPerCell, const std::string& metaData, double* mapping ) override;

    /**
     * Caller has to destroy this instance manually.
     */
    VertexDataWriter*  createVertexDataWriter( const std::string& identifier, int unknownsPerAxis, int recordsPerVertex ) override;
    VertexDataWriter*  createVertexDataWriter( const std::string& identifier, int unknownsPerAxis, int recordsPerVertex, const std::string& metaData  ) override;
    VertexDataWriter*  createVertexDataWriter( const std::string& identifier, int unknownsPerAxis, int recordsPerVertex, const std::string& metaData, double* mapping ) override;

    int plotPatch(
      const tarch::la::Vector<2,double>& offset,
      const tarch::la::Vector<2,double>& size
    ) override;

    int plotPatch(
      const tarch::la::Vector<3,double>& offset,
      const tarch::la::Vector<3,double>& size
    ) override;

    /**
     * @return Write has been successful
     */
    bool writeToFile() override;

    /**
     * @return Whether writer is ready to accept data.
     */
    bool isOpen() override;

    /**
     * Clear the writer, i.e. erase all the data. However, as the writer does
     * not track how many vertex and cell writers you've created, it's up to
     * you to ensure that none of these instances is left.
     */
    void clear() override;
};


#endif
