// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#ifdef UseHDF5

#include "PatchWriter.h"
#include "tarch/Filesystem.h"
#include "tarch/logging/Log.h"

#include <cmath>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <H5Cpp.h>
using namespace H5;

namespace tarch {
  namespace plotter {
    namespace griddata {
      namespace blockstructured {
        class PeanoHDF5FileWriter;
      } // namespace blockstructured
    } // namespace griddata
  } // namespace plotter
} // namespace tarch

/**
 * XMF file construction modes (Similar to PeanoTextPatchFileWriter)
 *
 * There are different configuration modes. They are passed as an argument to
 * the constructor:
 *
 * - IndexFileMode::CreateNew Create a new XMF file with the given timestamp value.
 *
 * - IndexFileMode::NoIndexFile Ignore the whole XMF file stuff. Every
 *   instance of the plotter dumps its own HDF5 data only.
 *
 * - IndexFileMode::AppendNewData Try to append a new time stamp (and subdomains) to an
 *   XMF file.
 *
 * The last one is the most sophisticated variant and deserves further
 * explanation. First of all, the code searches for the XMF file.
 * If there is none, the code basically falls back to IndexFileMode::CreateNew,
 * i.e. creates a new XMF file with initial time stamp 0.0
 *
 * If there is a file, we open it and look what the latest time stamp in
 * there is. For this, we call getLatestTimeStepInXMFFile(). If that time
 * stamp is bigger than the current time stamp, we know that this XMF file is
 * from a previous run. We remove this file and create a new XMF file with the
 * current given time stamp.
 *
 * If our own time stamp matches the last time stamp in the index file, we know
 * that the current dump is one (of many) dumps by a spacetree which contributes
 * towards the overall picture. That is, we add our own data set to the existing
 * time stamp.
 *
 * If our own time stamp is bigger than the last one, we know that we are the
 * first plotter of all the plotters out there which started to write a subdomain
 * for the next time stamp. We append a new section for a new dump and add our own
 * file there.
 *
 */
class tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter:
  public tarch::plotter::griddata::blockstructured::PatchWriter {

protected:
  static tarch::logging::Log _log;
  static const std::string   HEADER;

  std::string _hdf5FileName;
  std::string _xmfFile;

  bool _hasMapping; /**< Actual value is set by the cellDataWriter constructor*/
  bool _writtenToFile;
  bool _noIndexFileModeOff;

  int _dofsPerAxis;
  int _numUnknowns;

  int _dimensions;
  int _patchCounter;

  std::stringstream _xmfFileOut;

  /**HDF5 type definitions, that are used for writing datasets overall.
   * TODO: this has to be variable for e.g. float to work
   */
  FloatType _doubleType;
  IntType   _intType;
  FloatType _floatType;

  /**
   * Function that takes care of having a valid incremental XMF file.
   * The closing tags are added every time we write a subdomain,
   * because we do not know when the last subdomain will be written.
   * The tags are actually only needed at the end.
   * This function takes care of removing the closing tags that were added in the middle, to have a valid final XMF.
   * Depending on if we are still writing in the same time step, it removes the closing tag for the temporal grid.
   * If we received a new time step, that means the previous one was complete, and we do not need to remove its closing
   * tag.
   */
  void removeClosingTags(bool removeTemporalGrid);

  /**
   * Creates an empty XMF file with given time stamp, and adds the first temporal grid.
   */
  void createEmptyXMFFile(double initialTimeStamp);

  /**
   * This function is used to determine if we should start a new temporal grid for a new timestep.
   * We look for all the <Time Value=\ tags and return the lastly written double value.
   * Returns -1.0 if there is no timestep yet
   * (which is not possible in correct behaviour because XMF file is always created with an inital timestep.)
   */
  double getLatestTimeStepInXMFFile();

  /**
   * This is used in writeToFile() for writing the XMF.
   * Metadata like number of unknowns and dofs per axis is given to the cell data writer constructor.
   * These values are then written into the corresponding HDF5 file.
   * We can only access these values by reading the HDF5 file.
   */
  void readMetaData();

  /**
   * Similar to readMetaData() it reads the mapping array from HDF5 file, if given.
   * Handles both 2D and 3D case by internally setting the size of the mapping array, according to formula:
   * ((number of cells per axis)^dimensions)x dimensions
   */
  std::vector<double> readMapping();

  /**
   * Similar to readMetaData(), depending on if we have used the 2D or 3D plotPatch(),
   *  we read the offset and size values from the HDF5 for x, y, (z) axis
   */
  std::pair<double, double>          readOffsetAndSize2(const std::string& datasetPath);
  std::tuple<double, double, double> readOffsetAndSize3(const std::string& datasetPath);

  /**
   * Returns access to the RAII-managed HDF5 file handle
   */
  H5File* getH5File() const;

private:
  /**
   * Pointer to avoid redundant call/close calls on the hdf5 file. Initialize pointer in PeanoHDF5FileWriter
   * constructor. Close the pointer in writeToFile()
   */
  std::unique_ptr<H5::H5File> _h5File;

public:
  /**
   * Creates an HDF5 file which will contain the subdomains data. Similar to a patch file.
   * If HDF5 exists it will be overwritten.
   * Writes the metadata for the whole file. Handles both dimensions.
   */
  class CellDataWriter: public tarch::plotter::griddata::blockstructured::PatchWriter::CellDataWriter {
  protected:
    tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter& _writer;

    const std::string _identifier;
    const int         _numberOfCellsPerAxis;
    const int         _numberOfUnknowns;
    bool              _metadataWritten;
    int _cardinality; /**< Size of the unknowns array. ((_numberOfCellsPerAxis)^(_dimensions)) x _numberOfUnknowns */
    mutable bool _unknownsWritten;

  public:
    CellDataWriter(
      const std::string&                                              identifier,
      int                                                             unknownsPerAxis,
      int                                                             numberOfUnknowns,
      const std::string&                                              description,
      const std::string&                                              metaData,
      double*                                                         mapping,
      tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter& writer
    );
    virtual ~CellDataWriter();

    int getCellsPerPatch() const;

    /**
     * @see https://www.cplusplus.com/reference/iomanip/setprecision/
     */
    void setPrecision(int precision);

    virtual void plotCell(int index, double value) override; /*Unimplemented and unused*/

    /**
     * Writes the unknowns arrays into their own dataset in order of appearance.
     * Takes the one values array, and splits this array into separate unknowns.
     * Handles 2D and 3D
     * Technically needs to be called once, or write the unknowns array once ( _unknownsWritten)
     *
     * @param index Index of the cell. Unused
     * @param values Values array
     */
    virtual void plotCell(int index, double* values) override;

    template <typename T> void plotCell(int index, T value) {
      // This function does not make sense for HDF5. No op.
    }
    template <typename T> void plotCell(int index, T* values);

    virtual void close() override;

    virtual void assignRemainingCellsDefaultValues() override; /*Unimplemented*/

    virtual int getFirstCellWithinPatch(int index) const override;
  };

  /**
   * VertexDataWriter is unimplemented and unused. The compiler insisted in having this in the header file.
   * This is unused. The CellDataWriter can take care of everything. We could copy all the logic of
   * CellDataWriter in here, because they would not do anything different. All the XMF writing logic
   * is in the PeanoHDF5FileWriter writeToFile().
   * Maybe we can use the VertexDataWriter, when a mapping is given. That could be a difference to CellDataWriter.
   */
  class VertexDataWriter: public tarch::plotter::griddata::blockstructured::PatchWriter::VertexDataWriter {
  protected:
    tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter& _writer;

    const std::string _identifier;
    const int         _numberOfVerticesPerAxis;
    const int         _numberOfUnknowns;

    int               _entryCounter;
    std::stringstream _out;

  public:
    VertexDataWriter(
      const std::string&                                              identifier,
      int                                                             unknownsPerAxis,
      int                                                             numberOfUnknowns,
      const std::string&                                              description,
      const std::string&                                              metaData,
      double*                                                         mapping,
      tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter& writer
    );

    ~VertexDataWriter();

    void setPrecision(int precision);

    int getVerticesPerPatch() const;

    virtual void plotVertex(int index, double value) override;
    virtual void plotVertex(int index, double* values) override;

    template <typename T> void plotVertex(int index, T* values);

    virtual void close() override;

    virtual void assignRemainingVerticesDefaultValues() override;

    virtual int getFirstVertexWithinPatch(int index) const override;
  };

  enum class IndexFileMode { CreateNew, AppendNewData, NoIndexFile };

  /**
   * Create a new Peano XMF file output
   *
   * An instance of this class has to be owned by each action set that wants
   * to dump data.
   *
   */
  PeanoHDF5FileWriter(
    int                dimension,
    const std::string& hdf5FileName,
    const std::string& xmfFileName,
    IndexFileMode      appendToIndexFile,
    double             timeStamp
  );

  /**
   * Destructor
   *
   * The destructor is empty, i.e. if you destroy this object, your data are
   * lost. You have to dump the data beforehand manually by calling
   * writeToFile().
   */
  virtual ~PeanoHDF5FileWriter();

  /**
   * Caller has to destroy this instance manually.
   */
  virtual CellDataWriter* createCellDataWriter(
    const std::string& identifier,
    int                unknownsPerAxis,
    int                recordsPerCell,
    const std::string& description
  ) override;
  virtual CellDataWriter* createCellDataWriter(
    const std::string& identifier,
    int                unknownsPerAxis,
    int                recordsPerCell,
    const std::string& description,
    const std::string& metaData
  ) override;
  virtual CellDataWriter* createCellDataWriter(
    const std::string& identifier,
    int                unknownsPerAxis,
    int                recordsPerCell,
    const std::string& description,
    const std::string& metaData,
    double*            mapping
  ) override;

  /**
   * Caller has to destroy this instance manually.
   */
  virtual VertexDataWriter* createVertexDataWriter(
    const std::string& identifier,
    int                unknownsPerAxis,
    int                recordsPerVertex,
    const std::string& description
  ) override;
  virtual VertexDataWriter* createVertexDataWriter(
    const std::string& identifier,
    int                unknownsPerAxis,
    int                recordsPerVertex,
    const std::string& description,
    const std::string& metaData
  ) override;
  virtual VertexDataWriter* createVertexDataWriter(
    const std::string& identifier,
    int                unknownsPerAxis,
    int                recordsPerVertex,
    const std::string& description,
    const std::string& metaData,
    double*            mapping
  ) override;

  /**
   * Opens the HDF5 file, which should have been created in the data writer.
   * Creates an HDF5 group for the current patch with _patchCounter.
   * Writes the patch offset and size into a dataset.
   * Updates _patchCounter to keep track of the amount of patches in the subdomain.
   * The unknowns will be written py plotCell().
   *
   * @return the id of the patch that has just been written
   */
  virtual int plotPatch(const tarch::la::Vector<2, double>& offset, const tarch::la::Vector<2, double>& size) override;

  virtual int plotPatch(const tarch::la::Vector<3, double>& offset, const tarch::la::Vector<3, double>& size) override;

  /**
   * Adds one subdomain to the XMF file.
   * Reads the data that should be written, from the corresponding H5 file for that subdomain.
   *
   * Can handle both mapping or no mapping, 2D and 3D.
   *
   * Writes the subdomain patch by patch.
   *
   * Any geometry written to XMF is expected in this order: Z Y X
   *
   * If we have a mapping, we use the topology type 2DSMesh (3DSMesh).
   * We need to provide coordinate arrays in row major order for X and Y (and Z) axis inline.
   * For displaying the attributes aka the unknowns, we need the center type Node.
   *
   * If we do not have mapping, we use the topology type 2DCoRectMesh (3DCoRectMesh).
   * Since we have equidistant patches, with this topology, we do not need to provide
   * any coordinates. We just give the offset and the cell stepping size.
   * For displaying the unknowns as attributes, we use center type Cell.
   *
   * To reference the unknowns, we use a relative path name to HDF5.
   * XMF and the HDF5 files need to be in the same directory.
   * XMF wants only the file name of the HDF5, and no path with directories.
   *
   *
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
   *
   */
  virtual void clear() override;
};

#include "PeanoHDF5FileWriter_CellDataWriter.cpph"

#endif
