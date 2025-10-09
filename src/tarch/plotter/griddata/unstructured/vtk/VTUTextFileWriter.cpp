#include "tarch/plotter/griddata/unstructured/vtk/VTUTextFileWriter.h"

#include "tarch/Filesystem.h"

#include <fstream>
#include <iomanip>
#include <stdio.h>

#include "tarch/mpi/Rank.h"
#include "../../../PVDTimeSeriesWriter.h"

tarch::logging::Log tarch::plotter::griddata::unstructured::vtk::VTUTextFileWriter::_log("tarch::plotter::griddata::"
                                                                                         "unstructured::vtk::"
                                                                                         "VTUTextFileWriter");

const std::string tarch::plotter::griddata::unstructured::vtk::VTUTextFileWriter::HEADER = "<?xml version=\"1.0\"?>";

tarch::plotter::griddata::unstructured::vtk::VTUTextFileWriter::VTUTextFileWriter(
  const std::string& snapshotFileName,
  const std::string& indexFileName,
  const std::string& outputDirectory,
  tarch::plotter::PVDTimeSeriesWriter::IndexFileMode mode,
  double timeStamp,
  const int precision
):
  //_dataType(precision < 7 ? "Float32" : "Float64"), // TODO: Can we remove this choice?
  _dataType("Float64"),
  _writtenToFile(false),
  _numberOfVertices(0),
  _numberOfCells(0),
  _indexFile(outputDirectory + indexFileName),
  _snapshotFile(outputDirectory + snapshotFileName) {

  if (snapshotFileName.rfind(".vtu") != std::string::npos) {
    logWarning(
      "VTUTextFileWriter()",
      "Snapshot filename should not end with .vtu as routine adds extension automatically. Chosen filename prefix=" << snapshotFileName
    );
  }
  if (mode != tarch::plotter::PVDTimeSeriesWriter::IndexFileMode::NoIndexFile and indexFileName.rfind(".pvd") != std::string::npos) {
    logWarning(
      "VTUTextFileWriter()",
      "Index filename should not end with .pvd as routine adds extension automatically. Chosen filename prefix="
        << indexFileName
    );
  }

  // VTK does not support more precise values
  const double DefaultTimeStampPrecision = 1e-5;

  switch (mode) {
  case tarch::plotter::PVDTimeSeriesWriter::IndexFileMode::CreateNew:
    tarch::plotter::PVDTimeSeriesWriter::createEmptyIndexFile(_indexFile);
    tarch::plotter::PVDTimeSeriesWriter::appendNewData(_indexFile, snapshotFileName + ".vtu", timeStamp);
    break;
  case tarch::plotter::PVDTimeSeriesWriter::IndexFileMode::AppendNewData:
    if (not fs::exists(_indexFile + ".pvd")) {
      logDebug("VTUTextFileWriter()", "No index file " << _indexFile << " found. Create new one.");
      tarch::plotter::PVDTimeSeriesWriter::createEmptyIndexFile(_indexFile);
    } else if (tarch::la::smaller(timeStamp, tarch::plotter::PVDTimeSeriesWriter::getLatestTimeStepInIndexFile(_indexFile),
               tarch::la::relativeEpsNormaledAgainstValueGreaterOne(timeStamp, tarch::plotter::PVDTimeSeriesWriter::getLatestTimeStepInIndexFile(_indexFile), DefaultTimeStampPrecision))) {
      logWarning(
        "VTUTextFileWriter()",
        "There is an index file "
          << _indexFile << " with data for time stamp "
          << tarch::plotter::PVDTimeSeriesWriter::getLatestTimeStepInIndexFile(_indexFile)
          << ". Will be overwritten as we dump data for time " << timeStamp
      );
      tarch::plotter::PVDTimeSeriesWriter::createEmptyIndexFile(_indexFile);
    }
    tarch::plotter::PVDTimeSeriesWriter::appendNewData(_indexFile, snapshotFileName + ".vtu", timeStamp);
    break;
  case tarch::plotter::PVDTimeSeriesWriter::IndexFileMode::NoIndexFile:
    break;
  }
}

tarch::plotter::griddata::unstructured::vtk::VTUTextFileWriter::~VTUTextFileWriter() {
  if (!_writtenToFile) {
    assertionEqualsMsg(
      _numberOfVertices,
      0,
      "Still vertices in vtk writer pipeline. Maybe you forgot to call writeToFile() on a data vtk writer?"
    );
    assertionEqualsMsg(
      _numberOfCells,
      0,
      "Still cells in vtk writer pipeline. Maybe you forgot to call writeToFile() on a data vtk writer?"
    );
  }
}

void tarch::plotter::griddata::unstructured::vtk::VTUTextFileWriter::clear() {
  _writtenToFile                 = false;
  _numberOfVertices              = 0;
  _numberOfCells                 = 0;
  _vertexDescription             = "";
  _cellDescription               = "";
  _vertexDataDescription         = "";
  _cellDataDescription           = "";
  _parallelVertexDataDescription = "";
  _parallelCellDataDescription   = "";
}

bool tarch::plotter::griddata::unstructured::vtk::VTUTextFileWriter::writeToFile() {
  assertion(!_writtenToFile);

  std::ofstream out;
  out.open((_snapshotFile + ".vtu").c_str());
  if ((!out.fail()) and out.is_open()) {
    logDebug("writeToFile()", "Opened data file " + _snapshotFile);

    out
      << HEADER << std::endl
      << "<VTKFile type=\"UnstructuredGrid\" version=\"0.1\" byte_order=\"LittleEndian\">" << std::endl
      << "<UnstructuredGrid>" << std::endl
      << "<Piece NumberOfPoints=\"" << _numberOfVertices << "\" NumberOfCells=\"" << _numberOfCells << "\">" << std::endl
      << _vertexDescription << std::endl
      << _cellDescription << std::endl
      << "<PointData>" << std::endl
      << _vertexDataDescription << std::endl
      << "</PointData>" << std::endl
      << "<CellData>" << std::endl
      << _cellDataDescription << std::endl
      << "</CellData>" << std::endl
      << "</Piece>" << std::endl
      << "</UnstructuredGrid>" << std::endl
      << "</VTKFile>";

    logDebug("writeToFile()", "Data written to " + _snapshotFile);
    _writtenToFile = true;
  } else {
    logError("writeToFile()", "Unable to write output file " + _snapshotFile);
    return false;
  }

  return true;
}

void tarch::plotter::griddata::unstructured::vtk::VTUTextFileWriter::writeMetaDataFileForParallelSnapshot(
  const std::string& metaDataFilename, const std::vector<std::string>& files
) {
  if (metaDataFilename.rfind(".pvtu") != std::string::npos) {
    logWarning(
      "writeMetaDataFileForParallelSnapshot()",
      "meta file name should not end with .pvtu as routine adds extension automatically. Chosen filename prefix="
        << metaDataFilename
    );
  }

  std::ofstream metaOut;
  metaOut.open((metaDataFilename + ".pvtu").c_str());
  if ((!metaOut.fail()) && metaOut.is_open()) {
    logDebug("writeMetaDataFileForParallelSnapshot()", "opened data file " + metaDataFilename);

    metaOut
      << "<?xml version=\"1.0\"?>" << std::endl
      << "<VTKFile type=\"PUnstructuredGrid\" version=\"0.1\" byte_order=\"LittleEndian\">" << std::endl
      << "<PUnstructuredGrid GhostLevel=\"0\">" << std::endl
      << "<PPoints>" << std::endl
      << "<PDataArray type=\"" << _dataType << "\" Name=\"coordinates\" NumberOfComponents=\"3\"/>" << std::endl
      << "</PPoints>" << std::endl
      << "<PCells>" << std::endl
      << "<PDataArray type=\"Int32\" Name=\"connectivity\" NumberOfComponents=\"1\"/>" << std::endl
      << "<PDataArray type=\"Int32\" Name=\"offsets\"      NumberOfComponents=\"1\"/>" << std::endl
      << "<PDataArray type=\"UInt8\" Name=\"types\"        NumberOfComponents=\"1\"/>" << std::endl
      << "</PCells>" << std::endl
      << "<PPointData>" << std::endl
      << _parallelVertexDataDescription << std::endl
      << "</PPointData>" << std::endl
      << "<PCellData>" << std::endl
      << _parallelCellDataDescription << std::endl
      << "</PCellData>" << std::endl;

    for (auto p : files) {
      std::ostringstream referencedFilename;
      if (p.find("/") != std::string::npos) {
        referencedFilename << p.substr(p.rfind("/") + 1);
      } else {
        referencedFilename << p;
      }

      if (p.rfind(".vtu") != std::string::npos) {
        logWarning(
          "writeMetaDataFileForParallelSnapshot()",
          "file name passed to meta file writer should not end with .vtu as routine adds extension automatically. "
          "Chosen filename prefix="
            << p
        );
      }

      referencedFilename << ".vtu";
      metaOut << "<Piece Source=\"" << referencedFilename.str() << "\"/>" << std::endl;
    }

    metaOut << "</PUnstructuredGrid>" << std::endl << "</VTKFile>" << std::endl;

    metaOut.close();
  } else {
    logError("writeMetaDataFileForParallelSnapshot()", "unable to write meta data file " + metaDataFilename);
  }
}

bool tarch::plotter::griddata::unstructured::vtk::VTUTextFileWriter::isOpen() { return !_writtenToFile; }

tarch::plotter::griddata::unstructured::UnstructuredGridWriter::VertexWriter* tarch::plotter::griddata::unstructured::
  vtk::VTUTextFileWriter::createVertexWriter() {
  return new tarch::plotter::griddata::unstructured::vtk::VTUTextFileWriter::VertexWriter(*this, _dataType);
}

tarch::plotter::griddata::unstructured::UnstructuredGridWriter::CellWriter* tarch::plotter::griddata::unstructured::
  vtk::VTUTextFileWriter::createCellWriter() {
  return new tarch::plotter::griddata::unstructured::vtk::VTUTextFileWriter::CellWriter(*this, _dataType);
}

void tarch::plotter::griddata::unstructured::vtk::VTUTextFileWriter::validateDataWriterIdentifier(
  const std::string& identifier
) const {
  if (identifier.empty()) {
    logWarning(
      "validateDataWriterIdentifier(string)",
      "identifier for vtk file is empty. Spaces are not allowed for vtk data field identifiers and some vtk "
      "visualisers might crash."
    );
  }
  if (identifier.find(' ') != std::string::npos) {
    logWarning(
      "validateDataWriterIdentifier(string)",
      "identifier \""
        << identifier
        << "\" contains spaces. Spaces are not allowed for vtk data field identifiers and some vtk visualisers might "
           "crash."
    );
  }
}

tarch::plotter::griddata::unstructured::UnstructuredGridWriter::CellDataWriter* tarch::plotter::griddata::unstructured::
  vtk::VTUTextFileWriter::createCellDataWriter(const std::string& identifier, int recordsPerCell) {
  validateDataWriterIdentifier(identifier);
  return new tarch::plotter::griddata::unstructured::vtk::VTUTextFileWriter::CellDataWriter(
    identifier, *this, recordsPerCell, _dataType
  );
}

tarch::plotter::griddata::unstructured::UnstructuredGridWriter::VertexDataWriter* tarch::plotter::griddata::
  unstructured::vtk::VTUTextFileWriter::createVertexDataWriter(const std::string& identifier, int recordsPerVertex) {
  validateDataWriterIdentifier(identifier);
  return new tarch::plotter::griddata::unstructured::vtk::VTUTextFileWriter::VertexDataWriter(
    identifier, *this, recordsPerVertex, _dataType
  );
}
