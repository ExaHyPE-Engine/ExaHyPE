// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/logging/Log.h"
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


/**
 * Plot Peano's mesh data in Peano's text file format
 *
 *
 * Consult the @ref tarch_plotting "general plotting" remarks on rationale and
 * patterns behind the realisation of this class. This is a technical view. If
 * you are interested in the file format that is written, you have to read the
 * @ref tarch_patch_files "documentation of the Peano patch file format".
 *
 *
 * ## Meta file construction modes
 *
 * There are different configuration modes. They are passed as an argument to
 * the constructor:
 *
 * - IndexFileMode::CreateNew Create a new index file. The old one is thrown
 *   away.
 * - IndexFileMode::NoIndexFile Ignore the whole index file stuff. Every
 *   instance of the plotter dumps its own data and they do not synchronise in
 *   any way.
 * - IndexFileMode::AppendNewData Try to append a new data set to a given
 *   index file.
 *
 * The last one is the most sophisticated variant and deserves further
 * explanation. First of all, the code searches for the index (or meta) file.
 * If there is none, the code basically falls back to IndexFileMode::CreateNew,
 * i.e. creates a new index file.
 *
 * If there is a file, we open it and look what the latest time stamp in
 * there is. For this, we call getLatestTimeStepInIndexFile(). If that time
 * stamp is bigger than the current time stamp, we know that this index file is
 * from a previous run. We create a backup (some people do appreciate if we
 * do so) and then again create a new index file.
 *
 * If our own time stamp matches the last time stamp in the index file, we know
 * that the current dump is one (of many) dumps by a spacetree which contributes
 * towards the overall picture. That is, we add our own data set to the existing
 * time stamp.
 *
 * If our own time stamp is bigger than the last one, we know that we are the
 * first plotter of all the plotters out there which started to dump the data of
 * the next time stamp. We append a new section for a new dump and add our own
 * file there.
 */
class tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter: public tarch::plotter::griddata::blockstructured::PatchWriter {
  protected:
    static tarch::logging::Log _log;
    static const std::string HEADER;

    std::string  _fileName;
    std::string  _indexFile;

    bool _writtenToFile;

    int  _dimensions;

    int _patchCounter;

    std::stringstream _snapshotFileOut;
    bool              _haveWrittenAtLeastOnePatch;

    void writeMetaData(const std::string& metaData);
    void writeMapping(int totalEntries, double* values);

    void createBackupOfMetaFile();
    void createEmptyIndexFile();
    void addNewDatasetToIndexFile(double timestamp);
    void addNewFileToCurrentDataSetInIndexFile( const std::string&  filename );

    /**
     * Find latest time step in index file
     *
     * This routine runs through the index file and searches for the latest
     * entry in there which looks similar to
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * timestamp  0.000000000000000000000000e+00
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     * It will then return the value of this time stamp.
     */
    double getLatestTimeStepInIndexFile() const;

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
          int                numberOfUnknowns, const std::string& description,
          const std::string& metaData,
          double*            mapping,
          tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter& writer
        );
        virtual ~CellDataWriter();
        
        /**
         * @see https://www.cplusplus.com/reference/iomanip/setprecision/
         */
        void setPrecision(int precision);

        int getCellsPerPatch() const;

        /**
          * Write data for one cell.
          *
          * @param index Index of the cell. This index has to equal the index
          *              used for the cell within the VTKWriter class
          *              interface.
          * @param value Value for the cell.
          */
        virtual void plotCell( int index, double value ) override;
        virtual void plotCell( int index, double* values ) override;

        virtual void close() override;

        virtual void assignRemainingCellsDefaultValues() override;

        virtual int getFirstCellWithinPatch(int index) const override;
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
           int                numberOfUnknowns, const std::string& description,
           const std::string& metaData,
           double*            mapping,
           tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter& writer
         );

         ~VertexDataWriter();

         /**
          * @see https://www.cplusplus.com/reference/iomanip/setprecision/
          */
         void setPrecision(int precision);

         int getVerticesPerPatch() const;

         /**
          * Write data for one cell.
          *
          * @param index Index of the vertex. This index has to equal the index
          *              used for the cell within the VTKWriter class
          *              interface.
          * @param value Value for the cell.
          */
         virtual void plotVertex( int index, double value ) override;
         virtual void plotVertex( int index, double* values ) override;

          /*
          * Somewhat hacky, added to overload the plotVertex function in order to allow plotting of patches with
          * different variable types. Should likely have been handled more cleanly but since template functions cannot
          * be virtual this needed to be handled on the lowest level.
          */
         template <typename T>
         void plotVertex( int index, T* values );

         virtual void close() override;

         /**
          * @see close()
          */
         virtual void assignRemainingVerticesDefaultValues() override;

         virtual int getFirstVertexWithinPatch(int index) const override;
    };

    enum class IndexFileMode {
      CreateNew,
      AppendNewData,
      NoIndexFile
    };

    /**
     * Create a new Peano text file output
     *
     * An instance of this class has to be owned by each action set that wants
     * to dump data.
     *
     * @param dimension     I make this a free parameter, even though most
     *   codes will pass in the compile time parameter Dimensions. However,
     *   there are codes which plot submanifolds for example or want to write
     *   3d data within a 2d code, as they implement a 2.5d model.
     * @param fileName      Name of the file to write. Has to be unique per
     *   dump.
     * @param indexFileName Name of the index file. Can we empty if you select
     *   NoIndexFile.
     */
    PeanoTextPatchFileWriter(int dimension, const std::string&  fileName, const std::string&  indexFileName, IndexFileMode appendToIndexFile, double timeStamp);

    /**
     * Destructor
     *
     * The destructor is empty, i.e. if you destroy this object, your data are
     * lost. You have to dump the data beforehand manually by calling
     * writeToFile().
     */
    virtual ~PeanoTextPatchFileWriter();

    /**
     * Caller has to destroy this instance manually.
     */
    virtual CellDataWriter*    createCellDataWriter( const std::string& identifier, int unknownsPerAxis, int recordsPerCell, const std::string& description ) override;
    virtual CellDataWriter*    createCellDataWriter( const std::string& identifier, int unknownsPerAxis, int recordsPerCell, const std::string& description, const std::string& metaData ) override;
    virtual CellDataWriter*    createCellDataWriter( const std::string& identifier, int unknownsPerAxis, int recordsPerCell, const std::string& description, const std::string& metaData, double* mapping ) override;

    /**
     * Caller has to destroy this instance manually.
     */
    virtual VertexDataWriter*  createVertexDataWriter( const std::string& identifier, int unknownsPerAxis, int recordsPerVertex, const std::string& description ) override;
    virtual VertexDataWriter*  createVertexDataWriter( const std::string& identifier, int unknownsPerAxis, int recordsPerVertex, const std::string& description, const std::string& metaData  ) override;
    virtual VertexDataWriter*  createVertexDataWriter( const std::string& identifier, int unknownsPerAxis, int recordsPerVertex, const std::string& description, const std::string& metaData, double* mapping ) override;

    virtual int plotPatch(
      const tarch::la::Vector<2,double>& offset,
      const tarch::la::Vector<2,double>& size
    ) override;

    virtual int plotPatch(
      const tarch::la::Vector<3,double>& offset,
      const tarch::la::Vector<3,double>& size
    ) override;

    /**
     * @return Write has been successful
     */
    virtual bool writeToFile() override;

    /**
     * @return Whether writer is ready to accept data.
     */
    virtual bool isOpen() override;

    /**
     * Clear local data
     *
     * Clear the writer, i.e. erase all the data. However, as the writer does
     * not track how many vertex and cell writers you've created, it's up to
     * you to ensure that none of these instances is left. So we clear the core
     * data, but we do not clear the data within any data writer which you have
     * created by calling routines such as createCellDataWriter().
     */
    virtual void clear() override;
};

#include "PeanoTextPatchFileWriter_VertexDataWriter.cpph"
