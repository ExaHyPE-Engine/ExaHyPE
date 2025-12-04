// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "PeanoHDF5FileWriter.h"

#ifdef WITH_HDF5

#include "tarch/Assertions.h"
#include "tarch/mpi/Lock.h"
#include "tarch/mpi/Rank.h"

using namespace H5;

tarch::logging::Log tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter::_log(
  "tarch::plotter::"
  "griddata::"
  "blockstructured::"
  "PeanoHDF5FileWriter"
);

void tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter::removeClosingTags(bool removeTemporalGrid) {
  std::fstream in(_xmfFile, std::ios::in | std::ios::out | std::ios::binary);
  if (!in.is_open()) {
    logWarning("removeClosingTags()", "Could not open " + _xmfFile);
    return;
  }

  // Get file size
  in.seekg(0, std::ios::end);
  std::streampos fileSize = in.tellg();
  if (fileSize < 1) {
    logWarning("removeClosingTags()", "File is empty");
    return;
  }

  // Choose how much to read from the end of the file, big enough to include all closing tags
  static const std::streampos bufferSize = 200;

  // This is the relative reading position. If file size is smaller than bufferSize, we start from reading at the
  // beginning. If file size is bigger than bufferSize, for example 1500. We start reading at fileSize-bufferSize =
  // 1300. This means we do not read the first 1300 files, and look for the closing tags in the rest bufferSize=200
  // section.
  std::streamoff readStart = std::max<std::streampos>(0, fileSize - bufferSize);

  // Move to the readStart position, and read content after that point into the tail
  // We will operate on this smaller tail string for efficiency

  in.seekg(readStart);
  std::string tail;
  tail.resize(static_cast<size_t>(fileSize - readStart));
  in.read(&tail[0], tail.size());
  in.close();

  const std::string tagXdmf   = "</Xdmf>";
  const std::string tagDomain = "  </Domain>";
  const std::string tagGrid   = "    </Grid>";

  // Find position of tags from the end. To "remove" tags, we adjust the newFileSize.
  // readStart is the inital amount from the beginning of the file that we did not consider for the closing tag search.
  // This part stays untouched When we "remove" a tag, the position moves to a higher point. This simulates the word
  // getting removed. We add this new shorter position to readStart, which eventually leaves out the closing tag from
  // the file

  // Remove </Xdmf>
  std::size_t posXdmf = tail.rfind(tagXdmf);
  if (posXdmf == std::string::npos) {
    logWarning("removeClosingTags()", "Could not find closing tag " + tagXdmf);
    return;
  }

  std::streampos newFileSize = readStart + posXdmf;

  // Remove </Domain>
  std::size_t posDomain = tail.rfind(tagDomain, posXdmf);
  if (posDomain != std::string::npos) {
    newFileSize = readStart + posDomain;
  }

  // Optionally remove </Grid>
  if (removeTemporalGrid) {

    std::size_t posGrid = tail.rfind(tagGrid, (posDomain != std::string::npos ? posDomain : posXdmf));
    if (posGrid != std::string::npos) {
      newFileSize = readStart + posGrid;
    }
  }

  // When we truncate the file to the newFileSize, we essentially leave out the closing tags from the end of the file,
  // removing them. With this approach, we do not have to rewrite the entire file
  try {
    fs::resize_file(_xmfFile, newFileSize);
  } catch (const fs::filesystem_error& e) {
    logWarning("removeClosingTags()", "Failed to truncate file: " + std::string(e.what()));
    return;
  }
}

void tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter::createEmptyXMFFile(double initialTimeStamp) {
  logDebug("createEmptyXMFFile()", "Create empty XMF file " << _xmfFile);

  std::ofstream xmfFileOut;

  xmfFileOut.open(_xmfFile, std::ios::trunc);

  if (!xmfFileOut.is_open()) {
    logError("createEmptyXMFFile()", "Have not been able to open file " << _xmfFile);
    return;
  }

  xmfFileOut << R"(<?xml version="1.0"?>)" << "\n";
  xmfFileOut << R"(<Xdmf Version="3.0">)" << "\n";
  xmfFileOut << R"(  <Domain>)" << "\n";
  xmfFileOut
    << "    <Grid Name=\"TimeStep_" << initialTimeStamp << "\" GridType=\"Collection\" CollectionType=\"Temporal\">\n";
  xmfFileOut << "      <Time Value=\"" << std::setprecision(24) << std::scientific << initialTimeStamp << "\"/>\n";

  xmfFileOut.flush();
  xmfFileOut.close();
}

double tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter::getLatestTimeStepInXMFFile() {
  std::ifstream in(_xmfFile);

  if (!in.is_open()) {
    logError("getLatestTimeStepInXMFFile()", "Have not been able to open file " << _xmfFile);
    return -1.0;
  }

  double      lastTime = -1.0;
  std::string line;

  while (std::getline(in, line)) {
    std::size_t pos = line.find("<Time Value=\"");
    if (pos != std::string::npos) {
      pos += std::string("<Time Value=\"").size();
      std::size_t endPos = line.find("\"", pos);
      if (endPos != std::string::npos) {
        std::string numberStr = line.substr(pos, endPos - pos);
        try {
          double timeVal = std::stod(numberStr);
          lastTime       = timeVal;
        } catch (std::exception&) {
          logError(
            "getLatestTimeStepInXMFFile()",
            "File " << _xmfFile << " contained invalid time stamp in line: " << line
          );
        }
      }
    }
  }
  in.close();
  return lastTime;
}

H5::H5File* tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter::getH5File() const { return _h5File.get(); }

tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter::PeanoHDF5FileWriter(
  int                dimension,
  const std::string& hdf5FileName,
  const std::string& xmfFileName,
  IndexFileMode      appendToIndexFile,
  double             timeStamp
):
  _hdf5FileName(hdf5FileName),
  _xmfFile(xmfFileName),
  _hasMapping(false),
  _writtenToFile(false),
  _noIndexFileModeOff(true),
  _dimensions(dimension),
  _patchCounter(0) {

  assertion(dimension >= 2);
  assertion(dimension <= 3);

  if (hdf5FileName.rfind(".h5") != std::string::npos) {
    logWarning(
      "PeanoHDF5FileWriter()",
      "Filename should not end with .h5 as routine adds extension automaticaly. Chosen filename prefix=" << hdf5FileName
    );
  } else {
    _hdf5FileName += ".h5";
  }

  if (xmfFileName.rfind(".xmf") != std::string::npos) {
    logWarning(
      "PeanoHDF5FileWriter()",
      "XMF filename should not end with .xmf as routine adds extension automaticaly. Chosen filename prefix="
        << xmfFileName
    );
  } else {
    _xmfFile += ".xmf";
  }

  _doubleType = FloatType(PredType::NATIVE_DOUBLE);
  _intType    = IntType(PredType::NATIVE_INT);

  const double DefaultTimeStepPrecision = 1e-5;

  switch (appendToIndexFile) {
  case IndexFileMode::CreateNew:
    createEmptyXMFFile(0.0);
    try {
      _h5File = std::make_unique<H5::H5File>(_hdf5FileName, H5F_ACC_TRUNC);
    } catch (std::exception& e) {
      logError("PeanoHDF5FileWriter()", "File pointer to" << _hdf5FileName << " could not be created: " << e.what());
    }
    break;
  case IndexFileMode::NoIndexFile:
    _noIndexFileModeOff = false;
    clear();
    try {
      _h5File = std::make_unique<H5::H5File>(_hdf5FileName, H5F_ACC_TRUNC);
    } catch (std::exception& e) {
      logError("PeanoHDF5FileWriter()", "File pointer to" << _hdf5FileName << " could not be created: " << e.what());
    }
    break;

  case IndexFileMode::AppendNewData:
    if (not fs::exists(_xmfFile)) {
      logDebug("PeanoHDF5FileWriter()", "No XMF file " << _xmfFile << " found. Create new one");
      createEmptyXMFFile(0.0);
    } else {
      double currentTimeVal = getLatestTimeStepInXMFFile();
      if (tarch::la::smaller(currentTimeVal, timeStamp, DefaultTimeStepPrecision)) {
        // they are not equal, we have currentTimeVal < timeStamp, so a new bigger timeStamp. Start new grid.
        removeClosingTags(false);

        _xmfFileOut
          << "    <Grid Name=\"TimeStep_" << timeStamp << "\" GridType=\"Collection\" CollectionType=\"Temporal\">\n";
        _xmfFileOut << "      <Time Value=\"" << std::setprecision(24) << std::scientific << timeStamp << "\"/>\n";

        std::ofstream out(_xmfFile, std::ios::app);
        if (out.is_open()) {

          out << _xmfFileOut.rdbuf();
          out.flush();
          out.close();
        }

        _xmfFileOut.str("");
        _xmfFileOut.clear();
      } else if (tarch::la::smaller(timeStamp, currentTimeVal, DefaultTimeStepPrecision)) {
        // we have currentTimeVal > timeStamp: which should not be allowed, as we always want to increase time
        // increasingly. We delete the xmf, and start a new file with timeStamp
        logError(
          "PeanoHDF5FileWriter()",
          "Given an invalid timeStamp: "
            << timeStamp << " smaller than current timestep: " << currentTimeVal
            << ". Removing contents of XMF and continuing simulation with " << timeStamp
        );
        fs::remove(_xmfFile);
        clear();
        createEmptyXMFFile(timeStamp);
      } else {
        // we have currentTimeval = timeStamp. Keep writing in the same grid
        removeClosingTags(true);
      }
    }
    try {
      _h5File = std::make_unique<H5::H5File>(_hdf5FileName, H5F_ACC_TRUNC);
    } catch (std::exception& e) {
      logError("PeanoHDF5FileWriter()", "File pointer to" << _hdf5FileName << " could not be created: " << e.what());
    }
    break;
  }
}

tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter::~PeanoHDF5FileWriter() {}

tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter::CellDataWriter* tarch::plotter::griddata::
  blockstructured::PeanoHDF5FileWriter::createCellDataWriter(
    const std::string& identifier,
    int                unknownsPerAxis,
    int                recordsPerCell,
    const std::string& description
  ) {
  return new CellDataWriter(identifier, unknownsPerAxis, recordsPerCell, description, "", nullptr, *this);
}

tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter::CellDataWriter* tarch::plotter::griddata::
  blockstructured::PeanoHDF5FileWriter::createCellDataWriter(
    const std::string& identifier,
    int                unknownsPerAxis,
    int                recordsPerCell,
    const std::string& description,
    const std::string& metaData
  ) {
  return new CellDataWriter(identifier, unknownsPerAxis, recordsPerCell, description, metaData, nullptr, *this);
}

tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter::CellDataWriter* tarch::plotter::griddata::
  blockstructured::PeanoHDF5FileWriter::createCellDataWriter(
    const std::string& identifier,
    int                unknownsPerAxis,
    int                recordsPerCell,
    const std::string& description,
    const std::string& metaData,
    double*            mapping
  ) {
  return new CellDataWriter(identifier, unknownsPerAxis, recordsPerCell, description, metaData, mapping, *this);
}

tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter::VertexDataWriter* tarch::plotter::griddata::
  blockstructured::PeanoHDF5FileWriter::createVertexDataWriter(
    const std::string& identifier,
    int                unknownsPerAxis,
    int                recordsPerVertex,
    const std::string& description
  ) {
  return new VertexDataWriter(identifier, unknownsPerAxis, recordsPerVertex, description, "", nullptr, *this);
}

tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter::VertexDataWriter* tarch::plotter::griddata::
  blockstructured::PeanoHDF5FileWriter::createVertexDataWriter(
    const std::string& identifier,
    int                unknownsPerAxis,
    int                recordsPerVertex,
    const std::string& description,
    const std::string& metaData
  ) {
  return new VertexDataWriter(identifier, unknownsPerAxis, recordsPerVertex, description, metaData, nullptr, *this);
}

tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter::VertexDataWriter* tarch::plotter::griddata::
  blockstructured::PeanoHDF5FileWriter::createVertexDataWriter(
    const std::string& identifier,
    int                unknownsPerAxis,
    int                recordsPerVertex,
    const std::string& description,
    const std::string& metaData,
    double*            mapping
  ) {
  return new VertexDataWriter(identifier, unknownsPerAxis, recordsPerVertex, description, metaData, mapping, *this);
}

int tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter::plotPatch(
  const tarch::la::Vector<2, double>& offset,
  const tarch::la::Vector<2, double>& size
) {
  assertion(_dimensions == 2);

  try {
    /*File has to already exist. It should have been created in the constructor*/
    std::string groupName  = "Patch_" + std::to_string(_patchCounter);
    Group       patchGroup = getH5File()->createGroup(groupName);

    hsize_t   dimsPatchGeo = 2;
    DataSpace dataspacePatchGeo(1, &dimsPatchGeo);

    double offsetData[] = {offset(0), offset(1)};
    double sizeData[]   = {size(0), size(1)};

    DataSet offsetDataset = patchGroup.createDataSet("offset", _doubleType, dataspacePatchGeo);
    DataSet sizeDataset   = patchGroup.createDataSet("size", _doubleType, dataspacePatchGeo);

    offsetDataset.write(offsetData, _doubleType);
    sizeDataset.write(sizeData, _doubleType);
  } catch (std::exception& e) {
    logError("plotPatch()", "File " << _hdf5FileName << " could not be opened: " << e.what());
  }

  _patchCounter++;
  return _patchCounter - 1;
}

int tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter::plotPatch(
  const tarch::la::Vector<3, double>& offset,
  const tarch::la::Vector<3, double>& size
) {
  assertion(_dimensions == 3);

  try {
    /*File has to already exist. It should have been created in the constructor*/

    std::string groupName  = "Patch_" + std::to_string(_patchCounter);
    Group       patchGroup = getH5File()->createGroup(groupName);

    hsize_t   dimsPatchGeo = 3;
    DataSpace dataspacePatchGeo(1, &dimsPatchGeo);

    double offsetData[] = {offset(0), offset(1), offset(2)};
    double sizeData[]   = {size(0), size(1), size(2)};

    DataSet offsetDataset = patchGroup.createDataSet("offset", _doubleType, dataspacePatchGeo);
    DataSet sizeDataset   = patchGroup.createDataSet("size", _doubleType, dataspacePatchGeo);

    offsetDataset.write(offsetData, _doubleType);
    sizeDataset.write(sizeData, _doubleType);
  } catch (std::exception& e) {
    logError("plotPatch()", "File " << _hdf5FileName << " could not be opened: " << e.what());
  }

  _patchCounter++;
  return _patchCounter - 1;
}

void tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter::readMetaData() {
  try {
    {
      DataSet dset = getH5File()->openDataSet("/Metadata/dofs_per_axis");
      dset.read(&_dofsPerAxis, _intType);
    }
    {
      DataSet dset = getH5File()->openDataSet("/Metadata/number_of_unknowns");
      dset.read(&_numUnknowns, _intType);
    }
  } catch (std::exception& e) {
    logError("readMetaData()", "File " << _hdf5FileName << " could not be opened: " << e.what());
  }
}

std::vector<double> tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter::readMapping() {
  try {
    int mappingSize = 0;

    if (_dimensions == 2) {
      mappingSize = _dofsPerAxis * _dofsPerAxis * 2;
    } else if (_dimensions == 3) {
      mappingSize = _dofsPerAxis * _dofsPerAxis * _dofsPerAxis * 3;
    }

    std::vector<double> mapping(mappingSize);
    DataSet             dset = getH5File()->openDataSet("/Metadata/mapping");
    dset.read(mapping.data(), _doubleType);
    return mapping;
  } catch (std::exception& e) {
    logError("readMapping()", "File " << _hdf5FileName << " could not be opened: " << e.what());
  }

  return std::vector<double>();
}

std::pair<double, double> tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter::readOffsetAndSize2(
  const std::string& datasetPath
) {
  try {
    H5::DataSet dset = getH5File()->openDataSet(datasetPath);
    double      vals[2];
    dset.read(vals, _doubleType);
    return std::make_pair(vals[0], vals[1]);
  } catch (std::exception& e) {
    logError("readOffsetAndSize2()", "File " << _hdf5FileName << " could not be opened: " << e.what());
  }

  return std::pair<double, double>();
}

std::tuple<double, double, double> tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter::readOffsetAndSize3(
  const std::string& datasetPath
) {
  try {
    DataSet dset = getH5File()->openDataSet(datasetPath);
    double  vals[3];
    dset.read(vals, _doubleType);
    return std::make_tuple(vals[0], vals[1], vals[2]);
  } catch (std::exception& e) {
    logError("readOffsetAndSize3()", "File " << _hdf5FileName << " could not be opened: " << e.what());
  }

  return std::tuple<double, double, double>();
}

bool tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter::writeToFile() {
  assertion(!_writtenToFile);

  if (_noIndexFileModeOff) {
    std::ofstream out;
    out.open(_xmfFile, std::ios::app);

    if ((!out.fail()) && out.is_open()) {
      logDebug("writeToFile()", "Opened data file " + _xmfFile);

      _xmfFileOut << "      <Grid Name=\"subdomain" << "\" GridType=\"Collection\">\n";

      readMetaData();

      int dimsX = _dofsPerAxis;
      int dimsY = _dofsPerAxis;
      int dimsZ = _dofsPerAxis;

      std::vector<double> mapping;

      if (_hasMapping) {
        mapping = readMapping();
      }

      for (int patchIndex = 0; patchIndex < _patchCounter; patchIndex++) {
        std::ostringstream offsetPath, sizePath;
        offsetPath << "/Patch_" << patchIndex << "/offset";
        sizePath << "/Patch_" << patchIndex << "/size";

        double offsetX;
        double offsetY;
        double offsetZ;
        double sizeX;
        double sizeY;
        double sizeZ;

        if (_dimensions == 2) {
          auto off = readOffsetAndSize2(offsetPath.str());
          auto siz = readOffsetAndSize2(sizePath.str());

          offsetX = off.first;
          offsetY = off.second;
          sizeX   = siz.first;
          sizeY   = siz.second;
        } else if (_dimensions == 3) {
          auto off = readOffsetAndSize3(offsetPath.str());
          auto siz = readOffsetAndSize3(sizePath.str());

          offsetX = std::get<0>(off);
          offsetY = std::get<1>(off);
          offsetZ = std::get<2>(off);

          sizeX = std::get<0>(siz);
          sizeY = std::get<1>(siz);
          sizeZ = std::get<2>(siz);
        }

        /*Used in the last loop.
        With mapping, center type node works.
        Without mapping, center type cell.*/
        std::string unknownCenterType;

        if (_hasMapping) {
          unknownCenterType = "Node";

          /* Build the coordinate arrays in row-major order:
           We'll just embed them inline in the XMF (Format="XML") for geometry.*/

          std::ostringstream Xarray, Yarray, Zarray;

          Xarray.precision(12);
          Yarray.precision(12);
          Zarray.precision(12);
          Xarray << std::scientific;
          Yarray << std::scientific;
          Zarray << std::scientific;
          if (_dimensions == 2) {
            for (int j = 0; j < dimsY; ++j) {
              for (int i = 0; i < dimsX; ++i) {

                int idx = j * dimsX + i;

                /* (mapX, mapY) in (0..1) from the global mapping array*/
                double mapX = mapping[2 * idx + 0];
                double mapY = mapping[2 * idx + 1];

                double Xij = offsetX + sizeX * mapX;
                double Yij = offsetY + sizeY * mapY;

                Xarray << " " << Xij;
                Yarray << " " << Yij;
              }
              Xarray << "\n";
              Yarray << "\n";
            }

            _xmfFileOut << "        <Grid Name=\"Patch_" << patchIndex << "\" GridType=\"Uniform\">\n";

            // 2DSMesh => DIMENSIONS="dimsY dimsX"
            _xmfFileOut
              << "          <Topology TopologyType=\"2DSMesh\" DIMENSIONS=\"" << dimsY << " " << dimsX << "\"/>\n";

            // X array
            _xmfFileOut << "          <Geometry GeometryType=\"X_Y\">\n";
            _xmfFileOut
              << "            <DataItem DIMENSIONS=\"" << dimsY << " " << dimsX
              << "\" NumberType=\"Float\" Precision=\"8\" Format=\"XML\">\n";
            _xmfFileOut << Xarray.str();
            _xmfFileOut << "            </DataItem>\n";

            // Y array
            _xmfFileOut
              << "            <DataItem DIMENSIONS=\"" << dimsY << " " << dimsX
              << "\" NumberType=\"Float\" Precision=\"8\" Format=\"XML\">\n";
            _xmfFileOut << Yarray.str();
            _xmfFileOut << "            </DataItem>\n";
            _xmfFileOut << "          </Geometry>\n";
          } else if (_dimensions == 3) {
            for (int k = 0; k < dimsZ; ++k) {
              for (int j = 0; j < dimsY; ++j) {
                for (int i = 0; i < dimsX; ++i) {

                  int idx = k * dimsX * dimsY + j * dimsX + i;

                  // (mapX, mapY, mapZ) in (0..1) from the global mapping array
                  double mapX = mapping[3 * idx + 0];
                  double mapY = mapping[3 * idx + 1];
                  double mapz = mapping[3 * idx + 2];


                  double Xijk = offsetX + sizeX * mapX;
                  double Yijk = offsetY + sizeY * mapY;
                  double Zijk = offsetZ + sizeZ * mapz;

                  Xarray << " " << Xijk;
                  Yarray << " " << Yijk;
                  Zarray << " " << Zijk;
                }
                Xarray << "\n";
                Yarray << "\n";
                Zarray << "\n";
              }
            }

            // 3D geometry => X_Y_Z 3DSMesh
            _xmfFileOut << "        <Grid Name=\"Patch_" << patchIndex << "\" GridType=\"Uniform\">\n";

            _xmfFileOut
              << "          <Topology TopologyType=\"3DSMesh\" DIMENSIONS=\"" << dimsZ << " " << dimsY << " " << dimsX
              << "\"/>\n";

            _xmfFileOut << "          <Geometry GeometryType=\"X_Y_Z\">\n";

            // X array
            _xmfFileOut
              << "            <DataItem DIMENSIONS=\"" << dimsZ << " " << dimsY << " " << dimsX
              << "\" NumberType=\"Float\" Precision=\"8\" Format=\"XML\">\n";
            _xmfFileOut << Xarray.str();
            _xmfFileOut << "            </DataItem>\n";

            // Y array
            _xmfFileOut
              << "            <DataItem DIMENSIONS=\"" << dimsZ << " " << dimsY << " " << dimsX
              << "\" NumberType=\"Float\" Precision=\"8\" Format=\"XML\">\n";
            _xmfFileOut << Yarray.str();
            _xmfFileOut << "            </DataItem>\n";

            // Z array
            _xmfFileOut
              << "            <DataItem DIMENSIONS=\"" << dimsZ << " " << dimsY << " " << dimsX
              << "\" NumberType=\"Float\" Precision=\"8\" Format=\"XML\">\n";
            _xmfFileOut << Zarray.str();
            _xmfFileOut << "            </DataItem>\n";
            _xmfFileOut << "          </Geometry>\n";
          }
        } else {
          unknownCenterType = "Cell";

          if (_dimensions == 2) {
            // if dofsPerAxis=3 => 4 corner nodes in each dimension
            int dims = _dofsPerAxis + 1;
            // spacing
            double dx = sizeX / dimsX;
            double dy = sizeY / dimsY;

            _xmfFileOut << "        <Grid Name=\"Patch_" << patchIndex << "\" GridType=\"Uniform\">\n";

            _xmfFileOut
              << "          <Topology TopologyType=\"2DCoRectMesh\" DIMENSIONS=\"" << dims << " " << dims << "\"/>\n";

            // geometry: expected in order y,x by xdmf
            _xmfFileOut << "          <Geometry GeometryType=\"ORIGIN_DXDY\">\n";
            _xmfFileOut
              << "            <DataItem DIMENSIONS=\"2\" NumberType=\"Float\" Precision=\"8\" Format=\"XML\">"
              << offsetY << " " << offsetX << "</DataItem>\n";
            _xmfFileOut
              << "            <DataItem DIMENSIONS=\"2\" NumberType=\"Float\" Precision=\"8\" Format=\"XML\">" << dy
              << " " << dx << "</DataItem>\n";
            _xmfFileOut << "          </Geometry>\n";
          } else if (_dimensions == 3) {
            int dims = _dofsPerAxis + 1;

            double dx = sizeX / dimsX;
            double dy = sizeY / dimsY;
            double dz = sizeZ / dimsZ;

            _xmfFileOut << "        <Grid Name=\"Patch_" << patchIndex << "\" GridType=\"Uniform\">\n";

            _xmfFileOut
              << "          <Topology TopologyType=\"3DCoRectMesh\" DIMENSIONS=\"" << dims << " " << dims << " " << dims
              << "\"/>\n";

            _xmfFileOut << "          <Geometry GeometryType=\"ORIGIN_DXDYDZ\">\n";
            _xmfFileOut
              << "            <DataItem DIMENSIONS=\"3\" NumberType=\"Float\" Precision=\"8\" Format=\"XML\">"
              << offsetZ << " " << offsetY << " " << offsetX << "</DataItem>\n";
            _xmfFileOut
              << "            <DataItem DIMENSIONS=\"3\" NumberType=\"Float\" Precision=\"8\" Format=\"XML\">" << dz
              << " " << dy << " " << dx << "</DataItem>\n";
            _xmfFileOut << "          </Geometry>\n";
          }
        }

        fs::path file_path(_hdf5FileName);
        std::string           xmfRelativeName = file_path.filename().string();

        for (int unk = 0; unk < _numUnknowns; ++unk) {
          std::ostringstream unkPath;
          unkPath << "/Patch_" << patchIndex << "/unknown_" << unk;

          _xmfFileOut
            << "          <Attribute Name=\"unknown_" << unk << "\" AttributeType=\"Scalar\" Center=\""
            << unknownCenterType << "\">\n";

          if (_dimensions == 2) {
            _xmfFileOut
              << "            <DataItem DIMENSIONS=\"" << _dofsPerAxis << " " << _dofsPerAxis
              << "\" NumberType=\"Float\" Precision=\"8\" Format=\"HDF\">\n";
          } else if (_dimensions == 3) {
            _xmfFileOut
              << "            <DataItem DIMENSIONS=\"" << _dofsPerAxis << " " << _dofsPerAxis << " " << _dofsPerAxis
              << "\" NumberType=\"Float\" Precision=\"8\" Format=\"HDF\">\n";
          }

          _xmfFileOut << "              " << xmfRelativeName << ":" << unkPath.str() << "\n";
          _xmfFileOut << "            </DataItem>\n";
          _xmfFileOut << "          </Attribute>\n";
        }

        //=====================GOOD ALTERNATIVE APPROACH 1 FOR FILE SIZE
        // OPTIMIZATION===================================================================================================================
        /* To use this approach, comment out the attribute generation parts on Line 806-837 and Line 877-894.

        We have one attribute matrix field, referencing the whole multidimensional dataset.
        Works perfect for cell based data with center=cell.
        For vertex based data, we need center=node, paraview fails to read. If we do center=cell, it can read, but since
        the data is vertex based, the simulation does not look correct. We need to find why it does not read the same
        dataset with center=node.
         */

        /*fs::path file_path(_hdf5FileName);
        std::string xmfRelativeName = file_path.filename().string();

        if(_dimensions == 2) {

          _xmfFileOut << "          <Attribute Name=\"AllUnknowns\" " << "AttributeType=\"Matrix\" Center=\"" <<
        unknownCenterType << "\">\n"
                         << "            <DataItem Format=\"HDF\" NumberType=\"Float\" Precision=\"8\" "

                         << "DIMENSIONS=\"" << _dofsPerAxis << " " << _dofsPerAxis << " " << _numUnknowns << "\">\n"
                         << "              " << xmfRelativeName << ":/Patch_" << patchIndex << "/unknowns\n"
                         << "            </DataItem>\n"
                         << "          </Attribute>\n";

        }else if(_dimensions == 3){

         _xmfFileOut << "          <Attribute Name=\"AllUnknowns\" " << "AttributeType=\"Matrix\" Center=\"" <<
        unknownCenterType << "\">\n"
                     << "            <DataItem Format=\"HDF\" NumberType=\"Float\" Precision=\"8\" "

                     << "DIMENSIONS=\"" << _dofsPerAxis << " " << _dofsPerAxis << " " << _dofsPerAxis << " " <<
        _numUnknowns << "\">\n"
                     << "              " << xmfRelativeName << ":/Patch_" << patchIndex << "/unknowns\n"
                     << "            </DataItem>\n"
                     << "          </Attribute>\n";
        }*/

        //=====================ALTERNATIVE APPROACH 2 HYPERSLAB FOR FILE SIZE
        // OPTIMIZATION===================================================================================================================
        /* To use this approach, comment out the attribute generation part on Line 806-837 and Line 849-871.

        This approach always fails with invalid stride error.
         */

        /*fs::path file_path(_hdf5FileName);
        std::string xmfRelativeName = file_path.filename().string();

         for (int i = 0; i < _numUnknowns; i++) {
              _xmfFileOut << "          <Attribute Name=\"unknown_" << i << "\" AttributeType=\"Scalar\" Center=\"" <<
        unknownCenterType << "\">\n"
               << "            <DataItem ItemType=\"HyperSlab\" DIMENSIONS=\"" << _dofsPerAxis * _dofsPerAxis << "\"
        NumberType=\"Float\" Precision=\"8\">\n"

               << "             <DataItem DIMENSIONS=\"3\" Format=\"XML\"> 0 " << i << " </DataItem>\n"  //START: Where
        to start reading in the array
               << "             <DataItem DIMENSIONS=\"3\" Format=\"XML\"> " << _dofsPerAxis * _dofsPerAxis << " 1
        </DataItem>\n"  // COUNT: How many elements you want to read along each axis
               << "             <DataItem DIMENSIONS=\"3\" Format=\"XML\"> 1 " << _numUnknowns << " </DataItem>\n"
        //STRIDE: How many elements to skip between each selection
               << "             <DataItem DIMENSIONS=\"" << _dofsPerAxis * _dofsPerAxis << " " << _numUnknowns << "\"
        Format=\"HDF\"> " << xmfRelativeName << ":/Patch_" << patchIndex << "/unknowns\n" << "</DataItem>\n"

               << "            </DataItem>\n"
               << "          </Attribute>\n";

               //TODO: if it works, adjust to 3d
        }*/

        _xmfFileOut << "        </Grid>\n"; // end Patch
      }
      _xmfFileOut << "      </Grid>\n"; // end subdomain

      _xmfFileOut << "    </Grid>\n"; // end timestep
      _xmfFileOut << "  </Domain>\n";
      _xmfFileOut << "</Xdmf>\n";

      out << _xmfFileOut.rdbuf();
      out.flush();
      out.close();

      _xmfFileOut.str("");
      _xmfFileOut.clear();

      _writtenToFile = true;

      if (_h5File) {
        _h5File->close();
        _h5File.reset();
      }

      return true;

    } else {
      logError("writeToFile()", "Unable to write output file " + _xmfFile);
      return false;
    }

  } else {

    // skip xmf writing because of NoIndexFileMode
    return false;
  }
}

bool tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter::isOpen() { return !_writtenToFile; }

void tarch::plotter::griddata::blockstructured::PeanoHDF5FileWriter::clear() {
  _writtenToFile = false;

  _xmfFileOut.str(std::string());
  _xmfFileOut.clear();

  _patchCounter = 0;
}

#endif
