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
         template<typename T>
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
     * @param fileName      Name of the file to write. Has to be unique per
     *                      dump.
     * @param indexFileName Name of the index file. Can we empty if you select
     *                      NoIndexFile.
     */
    PeanoTextPatchFileWriter(int dimension, const std::string&  fileName, const std::string&  indexFileName, IndexFileMode appendToIndexFile, double timeStamp);
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
     * Clear the writer, i.e. erase all the data. However, as the writer does
     * not track how many vertex and cell writers you've created, it's up to
     * you to ensure that none of these instances is left.
     */
    virtual void clear() override;
};


