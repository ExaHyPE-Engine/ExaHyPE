// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "VTKHDFFileWriter.h"

#ifdef PEANO_WITH_HDF5

#include "tarch/Termination.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Handles.h"
#include "tarch/mpi/Rank.h"

#include <hdf5.h>

namespace {
  // Paths of the .vtkhdf files this process has created. The first write to a path
  // truncates the file and lays out its groups; every later write reopens and appends.
  std::set<std::string>& getCreatedPaths() {
    static std::set<std::string> paths;
    return paths;
  }
  std::mutex& getCreatedPathsMutex() {
    static std::mutex mutex;
    return mutex;
  }
} // namespace

tarch::plotter::VTKHDFFileWriter::VTKHDFFileWriter(const std::string& outputDirectory) :
  tarch::plotter::FileWriter(outputDirectory) {}

bool
tarch::plotter::VTKHDFFileWriter::isOpen() const {
  return true;
}
std::string
tarch::plotter::VTKHDFFileWriter::getFileExtension() const {
  return ".vtkhdf";
}

bool
tarch::plotter::VTKHDFFileWriter::writeToFile(const std::string& filename, const double time) {
  createOutputDirectory();
  const std::string filePath = (std::filesystem::path(_outputDirectory) / (filename + ".vtkhdf")).string();

  bool firstWrite;
  {
    const std::lock_guard<std::mutex> lock(getCreatedPathsMutex());
    firstWrite = getCreatedPaths().insert(filePath).second;
  }

  const auto localData = getSubdomainBuffersSequenceForWriting();
  // Every rank of the writing communicator holds data, so the schema comes from the
  // local reference.
  const FieldSchema schema = getFieldSchema(localData.reference);

  // Each rank merges its subdomains into one contiguous grid, and the grids of all
  // ranks form one VTKHDF part per step. Each dataset takes one collective write.
  unsigned long long localPoints = 0, localCells = 0, localConnectivity = 0;
  for (const SubdomainBuffers* const subdomain : localData.payloads) {
    localPoints += subdomain->pointsXYZ.size() / 3;
    localCells += subdomain->types.size();
    localConnectivity += subdomain->connectivity.size();
  }

  unsigned long long offsetPoints = 0, offsetCells = 0, offsetConnectivity = 0;
  unsigned long long totalPoints = localPoints, totalCells = localCells, totalConnectivity = localConnectivity;

  const auto& rank          = tarch::mpi::Rank::getInstance();
  MPI_Comm    comm          = rank.getCommunicator();
  bool        useParallelIO = rank.getNumberOfRanks() > 1;
  bool        isMaster      = rank.isGlobalMaster();

  // The collective HDF5 calls run on a sub-communicator of the ranks that hold data.
  // An empty rank that joins a collective write to a filtered dataset with a zero-sized
  // selection deadlocks the parallel chunk redistribution of some MPI-IO stacks.
  MPI_Comm ioComm = MPI_COMM_NULL;
  if (useParallelIO) {
    const bool hasData    = (localData.reference != nullptr);
    const int  localHas   = hasData ? 1 : 0;
    int        anyHasData = 0;
    tarch::mpi::allreduce(&localHas, &anyHasData, 1, MPI_INT, MPI_MAX, comm);

    if (anyHasData == 0) {
      // Every rank is empty: The master alone writes a valid empty file, serially.
      if (not rank.isGlobalMaster()) {
        return true;
      }
      useParallelIO = false;
    } else {
      tarch::mpi::splitCommunicator(comm, hasData ? 0 : MPI_UNDEFINED, rank.getRank(), &ioComm);
      if (not hasData) {
        return true; // The data-holding ranks write the file
      }
      comm       = ioComm;
      int ioRank = 0;
      tarch::mpi::getRank(ioComm, &ioRank);
      isMaster = (ioRank == 0);
    }
  }

  if (useParallelIO) {
    const unsigned long long local[3]  = {localPoints, localCells, localConnectivity};
    unsigned long long       prefix[3] = {0, 0, 0};
    unsigned long long       totals[3] = {0, 0, 0};
    tarch::mpi::scan(local, prefix, 3, MPI_UNSIGNED_LONG_LONG, MPI_SUM, comm);
    offsetPoints       = prefix[0] - local[0];
    offsetCells        = prefix[1] - local[1];
    offsetConnectivity = prefix[2] - local[2];
    tarch::mpi::allreduce(local, totals, 3, MPI_UNSIGNED_LONG_LONG, MPI_SUM, comm);
    totalPoints       = totals[0];
    totalCells        = totals[1];
    totalConnectivity = totals[2];
  }

  hid_t fileAccessProperties   = H5P_DEFAULT;
  hid_t dataTransferProperties = H5P_DEFAULT;
  if (useParallelIO) {
    fileAccessProperties = H5Pcreate(H5P_FILE_ACCESS);
    MPI_Info   info      = MPI_INFO_NULL;
    const auto hints     = getMPIFileHints();
    if (not hints.empty()) {
      tarch::mpi::createInfo(&info);
      for (const auto& hint : hints) {
        tarch::mpi::setInfo(info, hint.first.c_str(), hint.second.c_str());
      }
    }
    H5Pset_fapl_mpio(fileAccessProperties, comm, info);
    if (info != MPI_INFO_NULL) {
      tarch::mpi::freeInfo(&info);
    }
#if H5_VERSION_GE(1, 10, 0)
    H5Pset_all_coll_metadata_ops(fileAccessProperties, 1);
    H5Pset_coll_metadata_write(fileAccessProperties, 1);
#endif
    dataTransferProperties = H5Pcreate(H5P_DATASET_XFER);
    // The bulk datasets are deflate-filtered, and parallel HDF5 writes to a filtered
    // dataset collectively.
    H5Pset_dxpl_mpio(dataTransferProperties, H5FD_MPIO_COLLECTIVE);
  }

  const hid_t fileId = firstWrite ? H5Fcreate(filePath.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, fileAccessProperties)
                                  : H5Fopen(filePath.c_str(), H5F_ACC_RDWR, fileAccessProperties);
  if (fileAccessProperties != H5P_DEFAULT) {
    H5Pclose(fileAccessProperties);
  }
  // H5Pset_fapl_mpio duplicated the communicator into the file, and the file keeps that
  // copy for the collective writes and the close.
  if (ioComm != MPI_COMM_NULL) {
    tarch::mpi::freeCommunicator(&ioComm);
  }
  if (fileId < 0) {
    if (dataTransferProperties != H5P_DEFAULT) {
      H5Pclose(dataTransferProperties);
    }
    tarch::terminateWithError("Failed to open '" + filePath + "'.");
  }
  const hid_t linkCreationProperties = H5Pcreate(H5P_LINK_CREATE);
  H5Pset_create_intermediate_group(linkCreationProperties, 1);

  class HandleCloser final {
  public:
    HandleCloser(const hid_t fileId, const hid_t linkCreationProperties, const hid_t dataTransferProperties) :
      _fileId(fileId),
      _linkCreationProperties(linkCreationProperties),
      _dataTransferProperties(dataTransferProperties) {}
    HandleCloser(const HandleCloser&)            = delete;
    HandleCloser(HandleCloser&&)                 = delete;
    HandleCloser& operator=(const HandleCloser&) = delete;
    HandleCloser& operator=(HandleCloser&&)      = delete;

    ~HandleCloser() {
      H5Pclose(_linkCreationProperties);
      if (_dataTransferProperties != H5P_DEFAULT) {
        H5Pclose(_dataTransferProperties);
      }
      H5Fclose(_fileId);
    }

  private:
    const hid_t _fileId;
    const hid_t _linkCreationProperties;
    const hid_t _dataTransferProperties;
  } handleCloser(fileId, linkCreationProperties, dataTransferProperties);

  // Each dataset opens once per traversal, and its handle serves the extent, extend
  // and write calls below. This map is declared after handleCloser, so the datasets
  // close before the file.
  std::map<std::string, hid_t> openDatasets;
  class DatasetCloser final {
  public:
    explicit DatasetCloser(const std::map<std::string, hid_t>& datasets) :
      _datasets(datasets) {}
    DatasetCloser(const DatasetCloser&)            = delete;
    DatasetCloser(DatasetCloser&&)                 = delete;
    DatasetCloser& operator=(const DatasetCloser&) = delete;
    DatasetCloser& operator=(DatasetCloser&&)      = delete;

    ~DatasetCloser() {
      for (const auto& entry : _datasets) {
        if (entry.second >= 0) {
          H5Dclose(entry.second);
        }
      }
    }

  private:
    const std::map<std::string, hid_t>& _datasets;
  } datasetCloser(openDatasets);
  const auto openDataset = [&](const std::string& name) -> hid_t {
    const auto existing = openDatasets.find(name);
    if (existing != openDatasets.end()) {
      return existing->second;
    }
    const hid_t handle = H5Dopen(fileId, name.c_str(), H5P_DEFAULT);
    openDatasets.emplace(name, handle);
    return handle;
  };

  const auto createExtendible = [&](
                                  const std::string& name,
                                  const int          numberOfDimensions,
                                  const hsize_t      numberOfComponents,
                                  const hid_t        type,
                                  const bool         compress
                                ) {
    const hsize_t dimensions[2]    = {0, numberOfComponents};
    const hsize_t maxDimensions[2] = {H5S_UNLIMITED, numberOfComponents};
    // The bulk geometry and data datasets take a large chunk and deflate, while the
    // one-row-per-step index datasets stay uncompressed.
    const hsize_t chunkDimensions[2]        = {static_cast<hsize_t>(compress ? 65536 : 1024), numberOfComponents};
    const hid_t   dataSpace                 = H5Screate_simple(numberOfDimensions, dimensions, maxDimensions);
    const hid_t   datasetCreationProperties = H5Pcreate(H5P_DATASET_CREATE);
    H5Pset_chunk(datasetCreationProperties, numberOfDimensions, chunkDimensions);
    if (compress) {
      // The rank slabs tile each grown chunk of a compressed dataset, so every element
      // is written and the pre-fill is skipped.
      H5Pset_fill_time(datasetCreationProperties, H5D_FILL_TIME_NEVER);
      H5Pset_shuffle(datasetCreationProperties);
      H5Pset_deflate(datasetCreationProperties, 1);
    }
    const hid_t dataset =
      H5Dcreate(fileId, name.c_str(), type, dataSpace, linkCreationProperties, datasetCreationProperties, H5P_DEFAULT);
    H5Pclose(datasetCreationProperties);
    H5Sclose(dataSpace);
    if (dataset < 0) {
      tarch::terminateWithError("Failed to create VTKHDF dataset '" + name + "'.");
    }
    H5Dclose(dataset);
  };
  const auto rowsOf = [&](const std::string& name) -> hsize_t {
    const hid_t dataSpace     = H5Dget_space(openDataset(name));
    hsize_t     dimensions[2] = {0, 0};
    H5Sget_simple_extent_dims(dataSpace, dimensions, nullptr);
    H5Sclose(dataSpace);
    return dimensions[0];
  };
  // Extend a dataset to (currentRows + globalAdd) rows and write this rank's slab at
  // the exclusive-prefix offset within the new block. With collective IO an empty
  // rank issues a balanced empty write so the collective does not deadlock.
  const auto appendSlab = [&](
                            const std::string& name,
                            const int          numberOfDimensions,
                            const hsize_t      numberOfComponents,
                            const hid_t        type,
                            const hsize_t      currentRows,
                            const hsize_t      globalAdd,
                            const hsize_t      localRows,
                            const hsize_t      localOffset,
                            const void* const  data
                          ) {
    const hid_t   dataset          = openDataset(name);
    const hsize_t newDimensions[2] = {currentRows + globalAdd, numberOfComponents};
    H5Dset_extent(dataset, newDimensions);
    const hid_t fileSpace = H5Dget_space(dataset);
    if (localRows > 0) {
      const hsize_t start[2] = {currentRows + localOffset, 0};
      const hsize_t count[2] = {localRows, numberOfComponents};
      H5Sselect_hyperslab(fileSpace, H5S_SELECT_SET, start, nullptr, count, nullptr);
      const hid_t memorySpace = H5Screate_simple(numberOfDimensions, count, nullptr);
      if (H5Dwrite(dataset, type, memorySpace, fileSpace, dataTransferProperties, data) < 0) {
        H5Sclose(memorySpace);
        H5Sclose(fileSpace);
        tarch::terminateWithError("Failed to write VTKHDF hyperslab '" + name + "'.");
      }
      H5Sclose(memorySpace);
    } else if (dataTransferProperties != H5P_DEFAULT) {
      // Empty rank: It joins the collective write with an empty selection, so every
      // rank issues the same number of collective writes.
      H5Sselect_none(fileSpace);
      H5Dwrite(dataset, type, H5S_ALL, fileSpace, dataTransferProperties, nullptr);
    }
    H5Sclose(fileSpace);
  };
  // Appends one row to a 1D index dataset at the row the caller passes. The extend and
  // the write are collective, the master supplies the value and every other rank joins
  // with an empty selection.
  const auto appendStepRow =
    [&](const std::string& name, const hid_t type, const void* const value, const hsize_t row) {
      const hid_t   dataset          = openDataset(name);
      const hsize_t newDimensions[1] = {row + 1};
      H5Dset_extent(dataset, newDimensions);
      const hid_t fileSpace = H5Dget_space(dataset);
      if (isMaster) {
        const hsize_t start[1] = {row};
        const hsize_t count[1] = {1};
        H5Sselect_hyperslab(fileSpace, H5S_SELECT_SET, start, nullptr, count, nullptr);
        const hid_t memorySpace = H5Screate_simple(1, count, nullptr);
        H5Dwrite(dataset, type, memorySpace, fileSpace, dataTransferProperties, value);
        H5Sclose(memorySpace);
      } else if (dataTransferProperties != H5P_DEFAULT) {
        H5Sselect_none(fileSpace);
        H5Dwrite(dataset, type, H5S_ALL, fileSpace, dataTransferProperties, nullptr);
      }
      H5Sclose(fileSpace);
    };
  // Write the trailing Offsets sentinel. The preceding appendSlab already grew the extent
  // collectively, so only the value is still missing.
  const auto writeInt64At = [&](const std::string& name, const hsize_t position, const long long value) {
    const hid_t dataset   = openDataset(name);
    const hid_t fileSpace = H5Dget_space(dataset);
    if (isMaster) {
      const hsize_t start[1] = {position};
      const hsize_t count[1] = {1};
      H5Sselect_hyperslab(fileSpace, H5S_SELECT_SET, start, nullptr, count, nullptr);
      const hid_t memorySpace = H5Screate_simple(1, count, nullptr);
      H5Dwrite(dataset, H5T_NATIVE_LLONG, memorySpace, fileSpace, dataTransferProperties, &value);
      H5Sclose(memorySpace);
    } else if (dataTransferProperties != H5P_DEFAULT) {
      H5Sselect_none(fileSpace);
      H5Dwrite(dataset, H5T_NATIVE_LLONG, H5S_ALL, fileSpace, dataTransferProperties, nullptr);
    }
    H5Sclose(fileSpace);
  };

  if (firstWrite) {
    const hid_t group = H5Gcreate(fileId, "/VTKHDF", linkCreationProperties, H5P_DEFAULT, H5P_DEFAULT);
    {
      // A reader rejects a file whose declared version is newer than the version it
      // supports. This is the lowest version that covers a temporal unstructured grid.
      const hsize_t   dimensions[1] = {2};
      const long long version[2]    = {2, 0};
      const hid_t     dataSpace     = H5Screate_simple(1, dimensions, nullptr);
      const hid_t     attribute     = H5Acreate(group, "Version", H5T_STD_I64LE, dataSpace, H5P_DEFAULT, H5P_DEFAULT);
      H5Awrite(attribute, H5T_NATIVE_LLONG, version);
      H5Aclose(attribute);
      H5Sclose(dataSpace);
    }
    {
      const char* const type       = "UnstructuredGrid";
      const hid_t       stringType = H5Tcopy(H5T_C_S1);
      H5Tset_size(stringType, std::strlen(type));
      const hid_t dataSpace = H5Screate(H5S_SCALAR);
      const hid_t attribute = H5Acreate(group, "Type", stringType, dataSpace, H5P_DEFAULT, H5P_DEFAULT);
      H5Awrite(attribute, stringType, type);
      H5Aclose(attribute);
      H5Sclose(dataSpace);
      H5Tclose(stringType);
    }
    H5Gclose(group);

    const hid_t realStorageType = getDoublePrecisionOutput() ? H5T_IEEE_F64LE : H5T_IEEE_F32LE;
    createExtendible("/VTKHDF/Points", 2, 3, realStorageType, true);
    createExtendible("/VTKHDF/Connectivity", 1, 1, H5T_STD_I64LE, true);
    createExtendible("/VTKHDF/Offsets", 1, 1, H5T_STD_I64LE, true);
    createExtendible("/VTKHDF/Types", 1, 1, H5T_STD_U8LE, true);
    createExtendible("/VTKHDF/NumberOfPoints", 1, 1, H5T_STD_I64LE, false);
    createExtendible("/VTKHDF/NumberOfCells", 1, 1, H5T_STD_I64LE, false);
    createExtendible("/VTKHDF/NumberOfConnectivityIds", 1, 1, H5T_STD_I64LE, false);
    // Vectors are stored with 3 components and tensors with 9 so ParaView reads
    // them back as vectors and tensors; see FileWriter::classifyField.
    for (const auto& field : schema.pointFields) {
      createExtendible(
        "/VTKHDF/PointData/" + field.name,
        2,
        classifyField(field.components).writtenComponents,
        realStorageType,
        true
      );
    }
    for (const auto& field : schema.cellFields) {
      createExtendible(
        "/VTKHDF/CellData/" + field.name,
        2,
        classifyField(field.components).writtenComponents,
        realStorageType,
        true
      );
    }

    H5Gclose(H5Gcreate(fileId, "/VTKHDF/Steps", linkCreationProperties, H5P_DEFAULT, H5P_DEFAULT));
    createExtendible("/VTKHDF/Steps/Values", 1, 1, realStorageType, false);
    createExtendible("/VTKHDF/Steps/PartOffsets", 1, 1, H5T_STD_I64LE, false);
    createExtendible("/VTKHDF/Steps/PointOffsets", 1, 1, H5T_STD_I64LE, false);
    createExtendible("/VTKHDF/Steps/CellOffsets", 1, 1, H5T_STD_I64LE, false);
    createExtendible("/VTKHDF/Steps/ConnectivityIdOffsets", 1, 1, H5T_STD_I64LE, false);
    for (const auto& field : schema.pointFields) {
      createExtendible("/VTKHDF/Steps/PointDataOffsets/" + field.name, 1, 1, H5T_STD_I64LE, false);
    }
    for (const auto& field : schema.cellFields) {
      createExtendible("/VTKHDF/Steps/CellDataOffsets/" + field.name, 1, 1, H5T_STD_I64LE, false);
    }
    const hid_t     stepsGroup = H5Gopen(fileId, "/VTKHDF/Steps", H5P_DEFAULT);
    const hid_t     dataSpace  = H5Screate(H5S_SCALAR);
    const long long zero       = 0;
    const hid_t     attribute  = H5Acreate(stepsGroup, "NSteps", H5T_STD_I64LE, dataSpace, H5P_DEFAULT, H5P_DEFAULT);
    H5Awrite(attribute, H5T_NATIVE_LLONG, &zero);
    H5Aclose(attribute);
    H5Sclose(dataSpace);
    H5Gclose(stepsGroup);
  }

  // Current extents before this step (cumulative over all prior steps) - the offsets
  // the VTKHDF reader expects in /VTKHDF/Steps.
  const long long currentPoints       = static_cast<long long>(rowsOf("/VTKHDF/Points"));
  const long long currentConnectivity = static_cast<long long>(rowsOf("/VTKHDF/Connectivity"));
  const long long currentCells        = static_cast<long long>(rowsOf("/VTKHDF/Types"));
  const hsize_t   currentOffsets      = rowsOf("/VTKHDF/Offsets");
  const long long currentStep         = static_cast<long long>(rowsOf("/VTKHDF/Steps/Values"));

  // Merge this rank's subdomains into one contiguous grid for the step. Points,
  // connectivity and offsets are rebased to step-global indices so that the whole
  // step is a single part (NumberOf*[step] = step totals, one entry per step).
  std::vector<double>       pointsBuffer;
  std::vector<std::int64_t> connectivityBuffer, beforeOffsets;
  std::vector<std::uint8_t> typesBuffer;
  pointsBuffer.reserve(localPoints * 3);
  connectivityBuffer.reserve(localConnectivity);
  beforeOffsets.reserve(localCells);
  typesBuffer.reserve(localCells);
  unsigned long long pointCursor        = 0;
  unsigned long long connectivityCursor = 0;
  for (const SubdomainBuffers* const subdomain : localData.payloads) {
    const std::int64_t pointBase = static_cast<std::int64_t>(offsetPoints + pointCursor);
    pointsBuffer.insert(pointsBuffer.end(), subdomain->pointsXYZ.begin(), subdomain->pointsXYZ.end());
    for (const auto connectivityEntry : subdomain->connectivity) {
      connectivityBuffer.push_back(static_cast<std::int64_t>(connectivityEntry) + pointBase);
    }
    std::int32_t previousEnd = 0;
    for (std::size_t i = 0; i < subdomain->types.size(); i++) {
      beforeOffsets.push_back(static_cast<std::int64_t>(offsetConnectivity + connectivityCursor) + previousEnd);
      previousEnd = subdomain->offsets[i];
    }
    typesBuffer.insert(typesBuffer.end(), subdomain->types.begin(), subdomain->types.end());
    pointCursor += subdomain->pointsXYZ.size() / 3;
    connectivityCursor += subdomain->connectivity.size();
  }

  // The buffers accumulate in double precision; single-precision output narrows
  // them right before the write.
  const bool writeDouble    = getDoublePrecisionOutput();
  const auto appendRealSlab = [&](
                                const std::string&         name,
                                const int                  components,
                                const hsize_t              current,
                                const hsize_t              total,
                                const hsize_t              local,
                                const hsize_t              offset,
                                const std::vector<double>& buffer
                              ) {
    if (writeDouble) {
      appendSlab(name, 2, components, H5T_NATIVE_DOUBLE, current, total, local, offset, buffer.data());
    } else {
      const std::vector<float> narrowed = narrowToFloat(buffer);
      appendSlab(name, 2, components, H5T_NATIVE_FLOAT, current, total, local, offset, narrowed.data());
    }
  };

  appendRealSlab("/VTKHDF/Points", 3, currentPoints, totalPoints, localPoints, offsetPoints, pointsBuffer);
  appendSlab(
    "/VTKHDF/Connectivity",
    1,
    1,
    H5T_NATIVE_LLONG,
    currentConnectivity,
    totalConnectivity,
    localConnectivity,
    offsetConnectivity,
    connectivityBuffer.data()
  );
  appendSlab(
    "/VTKHDF/Types",
    1,
    1,
    H5T_NATIVE_UINT8,
    currentCells,
    totalCells,
    localCells,
    offsetCells,
    typesBuffer.data()
  );
  // Offsets per step: One entry per cell plus a trailing sentinel that carries the
  // connectivity total of the step, so the step block holds totalCells + 1 entries.
  appendSlab(
    "/VTKHDF/Offsets",
    1,
    1,
    H5T_NATIVE_LLONG,
    currentOffsets,
    totalCells + 1,
    localCells,
    offsetCells,
    beforeOffsets.data()
  );
  writeInt64At("/VTKHDF/Offsets", currentOffsets + totalCells, static_cast<long long>(totalConnectivity));

  // validateSubdomainsForWriting() establishes the schema's field order in every
  // subdomain, so the schema index also indexes a subdomain's field list.
  for (std::size_t fieldIndex = 0; fieldIndex < schema.pointFields.size(); ++fieldIndex) {
    const auto&         field   = schema.pointFields[fieldIndex];
    const int           written = classifyField(field.components).writtenComponents;
    std::vector<double> fieldBuffer;
    fieldBuffer.reserve(localPoints * field.components);
    for (const SubdomainBuffers* const subdomain : localData.payloads) {
      const auto& values = subdomain->pointFields[fieldIndex].values;
      fieldBuffer.insert(fieldBuffer.end(), values.begin(), values.end());
    }
    if (written != field.components) {
      fieldBuffer = expandFieldComponents(fieldBuffer, field.components, written);
    }
    // A point field grows by the same number of rows as /VTKHDF/Points.
    appendRealSlab(
      "/VTKHDF/PointData/" + field.name,
      written,
      static_cast<hsize_t>(currentPoints),
      totalPoints,
      localPoints,
      offsetPoints,
      fieldBuffer
    );
    appendStepRow(
      "/VTKHDF/Steps/PointDataOffsets/" + field.name,
      H5T_NATIVE_LLONG,
      &currentPoints,
      static_cast<hsize_t>(currentStep)
    );
  }
  for (std::size_t fieldIndex = 0; fieldIndex < schema.cellFields.size(); ++fieldIndex) {
    const auto&         field   = schema.cellFields[fieldIndex];
    const int           written = classifyField(field.components).writtenComponents;
    std::vector<double> fieldBuffer;
    fieldBuffer.reserve(localCells * field.components);
    for (const SubdomainBuffers* const subdomain : localData.payloads) {
      const auto& values = subdomain->cellFields[fieldIndex].values;
      fieldBuffer.insert(fieldBuffer.end(), values.begin(), values.end());
    }
    if (written != field.components) {
      fieldBuffer = expandFieldComponents(fieldBuffer, field.components, written);
    }
    // A cell field grows by the same number of rows as /VTKHDF/Types.
    appendRealSlab(
      "/VTKHDF/CellData/" + field.name,
      written,
      static_cast<hsize_t>(currentCells),
      totalCells,
      localCells,
      offsetCells,
      fieldBuffer
    );
    appendStepRow(
      "/VTKHDF/Steps/CellDataOffsets/" + field.name,
      H5T_NATIVE_LLONG,
      &currentCells,
      static_cast<hsize_t>(currentStep)
    );
  }

  const long long totalPointsSigned       = static_cast<long long>(totalPoints);
  const long long totalCellsSigned        = static_cast<long long>(totalCells);
  const long long totalConnectivitySigned = static_cast<long long>(totalConnectivity);
  const float     timeAsFloat             = static_cast<float>(time);
  const hsize_t   stepRow                 = static_cast<hsize_t>(currentStep);
  appendStepRow("/VTKHDF/NumberOfPoints", H5T_NATIVE_LLONG, &totalPointsSigned, stepRow);
  appendStepRow("/VTKHDF/NumberOfCells", H5T_NATIVE_LLONG, &totalCellsSigned, stepRow);
  appendStepRow("/VTKHDF/NumberOfConnectivityIds", H5T_NATIVE_LLONG, &totalConnectivitySigned, stepRow);
  if (writeDouble) {
    appendStepRow("/VTKHDF/Steps/Values", H5T_NATIVE_DOUBLE, &time, stepRow);
  } else {
    appendStepRow("/VTKHDF/Steps/Values", H5T_NATIVE_FLOAT, &timeAsFloat, stepRow);
  }
  appendStepRow("/VTKHDF/Steps/PartOffsets", H5T_NATIVE_LLONG, &currentStep, stepRow);
  appendStepRow("/VTKHDF/Steps/PointOffsets", H5T_NATIVE_LLONG, &currentPoints, stepRow);
  appendStepRow("/VTKHDF/Steps/CellOffsets", H5T_NATIVE_LLONG, &currentCells, stepRow);
  appendStepRow("/VTKHDF/Steps/ConnectivityIdOffsets", H5T_NATIVE_LLONG, &currentConnectivity, stepRow);

  const hid_t     stepsGroup    = H5Gopen(fileId, "/VTKHDF/Steps", H5P_DEFAULT);
  const hid_t     attribute     = H5Aopen(stepsGroup, "NSteps", H5P_DEFAULT);
  const long long numberOfSteps = currentStep + 1;
  H5Awrite(attribute, H5T_NATIVE_LLONG, &numberOfSteps);
  H5Aclose(attribute);
  H5Gclose(stepsGroup);

  return true;
}

#endif
