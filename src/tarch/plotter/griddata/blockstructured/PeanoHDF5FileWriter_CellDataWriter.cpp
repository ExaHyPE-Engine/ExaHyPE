// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "PeanoHDF5FileWriter.h"

#ifdef WITH_HDF5

#include "tarch/la/ScalarOperations.h"

using namespace H5;

tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter::CellDataWriter::CellDataWriter(
  const std::string&                                              identifier,
  int                                                             unknownsPerAxis,
  int                                                             numberOfUnknowns,
  const std::string&                                              /*description*/,
  const std::string&                                              /*metaData*/,
  double*                                                         mapping,
  tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter& writer
):
  _writer(writer),
  _identifier(identifier),
  _numberOfCellsPerAxis(unknownsPerAxis),
  _numberOfUnknowns(numberOfUnknowns),
  _metadataWritten(false),
  _unknownsWritten(false) {

  if (_metadataWritten == false) {
    try {
      Group metadataGroup = _writer.getH5File()->createGroup("Metadata");

      hsize_t   dim = 1;
      DataSpace scalarSpace(1, &dim);

      DataSet unknownsDataset = metadataGroup.createDataSet("number_of_unknowns", _writer._intType, scalarSpace);
      DataSet dofsDataset     = metadataGroup.createDataSet("dofs_per_axis", _writer._intType, scalarSpace);

      int tmpUnknowns    = _numberOfUnknowns;
      int tmpDofsPerAxis = _numberOfCellsPerAxis;

      unknownsDataset.write(&tmpUnknowns, _writer._intType);
      dofsDataset.write(&tmpDofsPerAxis, _writer._intType);

      if (mapping == nullptr) {
        _writer._hasMapping = false;

      } else if (mapping != nullptr) {
        _writer._hasMapping = true;

        hsize_t   dimsMapping = getCellsPerPatch() * _writer._dimensions;
        DataSpace dataspaceMapping(1, &dimsMapping);

        // DSetCreatPropList property_list;

        // hsize_t chunkSize = dimsMapping;
        // property_list.setChunk(1, &chunkSize);
        // property_list.setDeflate(9);

        DataSet mappingDataset = metadataGroup.createDataSet("mapping", _writer._doubleType, dataspaceMapping);
        mappingDataset.write(mapping, _writer._doubleType);
      }
      _metadataWritten = true;
    } catch (std::exception& e) {
      logError("CellDataWriter()", "File " << _writer._hdf5FileName << " could not be opened: " << e.what());
    }
  }

  _cardinality = getCellsPerPatch() * _numberOfUnknowns;
}

tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter::CellDataWriter::~CellDataWriter() {}

int tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter::CellDataWriter::getCellsPerPatch() const {
  return tarch::la::aPowI(_writer._dimensions, _numberOfCellsPerAxis);
}

void tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter::CellDataWriter::setPrecision(int precision) {
  if (precision > 0) {
    _writer._xmfFileOut << std::setprecision(precision);
  }
}

void tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter::CellDataWriter::plotCell(
  int     /*index*/,
  double  /*value*/
) {
  // This function does not make sense for HDF5. No op.
}

void tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter::CellDataWriter::plotCell(
  int     /*index*/,
  double* values
) {
  if (_unknownsWritten == false) {
    try {
      std::string groupName  = "Patch_" + std::to_string(_writer._patchCounter - 1);
      Group       patchGroup = _writer.getH5File()->openGroup(groupName);
      for (int unknown = 0; unknown < _numberOfUnknowns; ++unknown) {
        std::vector<double> separated_values;

        for (int i = unknown; i < _cardinality; i += _numberOfUnknowns) {
          separated_values.push_back(values[i]);
        }

        std::string datasetName = "unknown_" + std::to_string(unknown);
        hsize_t     dimsValues  = separated_values.size();

        // DSetCreatPropList property_list;

        // hsize_t chunkSize = dimsValues;
        // property_list.setChunk(1, &chunkSize);
        // property_list.setDeflate(9);

        DataSpace dataspaceValues(1, &dimsValues);
        DataSet   valuesDataset = patchGroup.createDataSet(datasetName, _writer._doubleType, dataspaceValues);
        valuesDataset.write(separated_values.data(), _writer._doubleType);
      }

      //=====ALTERNATIVE APPROACH: FILE SIZE
      // OPTIMIZATION=========================================================================
      /*Storing all unknowns in a multidimensional dataset, instead of individual seperate,
      reduces number of datasets. This eliminates the chunking overhead we have, and allows for compression benefits.
      Then the HDF5 writer results in smaller file sizes.
      */

      /*if(_writer._dimensions == 2){
          hsize_t dims[3];
          dims[0] = _numberOfCellsPerAxis;
          dims[1] = _numberOfCellsPerAxis;
          dims[2] = _numberOfUnknowns;

          DataSpace dataspaceValues(3, dims);

          DataSet valuesDataset = patchGroup.createDataSet("unknowns", _writer._doubleType, dataspaceValues);
          valuesDataset.write(values, _writer._doubleType);

        } else if(_writer._dimensions == 3){
          hsize_t dims[4];
          dims[0] = _numberOfCellsPerAxis;
          dims[1] = _numberOfCellsPerAxis;
          dims[2] = _numberOfCellsPerAxis;
          dims[3] = _numberOfUnknowns;

          DataSpace dataspaceValues(4, dims);

          DataSet valuesDataset = patchGroup.createDataSet("unknowns", _writer._doubleType, dataspaceValues);
          valuesDataset.write(values, _writer._doubleType);
        }*/

      _unknownsWritten = true;
    } catch (std::exception& e) {
      logError("plotCell()", "File " << _writer._hdf5FileName << " could not be opened: " << e.what());
    }
  }
}

void tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter::CellDataWriter::close() {}

void tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter::CellDataWriter::assignRemainingCellsDefaultValues() {}

int tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter::CellDataWriter::getFirstCellWithinPatch(
  int index
) const {
  _unknownsWritten = false;
  return index * getCellsPerPatch();
}

#endif
