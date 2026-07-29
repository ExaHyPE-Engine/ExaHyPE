// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "VTKHDFFileWriter.h"

#ifdef PEANO_WITH_HDF5

#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"

#include <hdf5.h>

namespace {
  // Which .vtkhdf paths have already been created in this run. The first write
  // to a path truncates and lays out the skeleton; every later write reopens and
  // appends. Static (per process), guarded for the multithreaded plot step.
  std::set<std::string>& createdPaths() {
    static std::set<std::string> paths;
    return paths;
  }
  std::mutex& createdPathsMutex() {
    static std::mutex mutex;
    return mutex;
  }
} // namespace

tarch::plotter::VTKHDFFileWriter::VTKHDFFileWriter(const std::string& outputDirectory, int numberOfSubdomains) :
  FileWriter(outputDirectory, numberOfSubdomains) {}

bool        tarch::plotter::VTKHDFFileWriter::isOpen() const { return true; }
std::string tarch::plotter::VTKHDFFileWriter::getFileExtension() const { return ".vtkhdf"; }

bool tarch::plotter::VTKHDFFileWriter::writeToFile(const std::string& filename, double time) {
  createOutputDirectory();
  const std::string pathStr = (std::filesystem::path(_outputDirectory) / (filename + ".vtkhdf")).string();

  bool firstWrite;
  {
    std::lock_guard<std::mutex> lock(createdPathsMutex());
    firstWrite = createdPaths().insert(pathStr).second;
  }

  const auto localData = getSubdomainBuffersSequenceForWriting();
  // Empty ranks are dropped from the writing communicator below, so the schema is taken
  // directly from the local reference; a collective broadcast of it would deadlock here,
  // since the excluded ranks would not take part in it.
  const FieldSchema schema = fieldSchemaFromReference(localData.reference);

  // Every rank merges its local subdomains into one contiguous grid; all ranks'
  // grids are then concatenated into a single VTKHDF part per step (no composite
  // dataset, matching the HDF5/XDMF writer). One collective write per dataset, so
  // a heterogeneous rank distribution cannot mismatch collective call counts.
  unsigned long long localPoints = 0, localCells = 0, localConnectivity = 0;
  for (const SubdomainBuffers* subdomain : localData.payloads) {
    localPoints += subdomain->pointsXYZ.size() / 3;
    localCells += subdomain->types.size();
    localConnectivity += subdomain->connectivity.size();
  }

  unsigned long long offsetPoints = 0, offsetCells = 0, offsetConnectivity = 0;
  unsigned long long totalPoints = localPoints, totalCells = localCells, totalConnectivity = localConnectivity;

  auto&    rank          = tarch::mpi::Rank::getInstance();
  MPI_Comm comm          = rank.getCommunicator();
  bool     useParallelIO = rank.getNumberOfRanks() > 1;
  bool     isMaster      = rank.isGlobalMaster();

  // Restrict the collective HDF5 I/O to only the ranks that hold data. An empty rank
  // joining a collective write to a chunked + deflated (filtered) dataset with a
  // zero-sized selection deadlocks HDF5's parallel filtered-collective chunk
  // redistribution on some MPI-IO stacks (MPICH/ROMIO on a parallel filesystem). Dropping
  // the empty ranks keeps every participant's collective sequence identical, so the
  // compressed parallel write stays well-defined.
  MPI_Comm ioComm = MPI_COMM_NULL;
  if (useParallelIO) {
    const bool hasData    = (localData.reference != nullptr);
    int        localHas   = hasData ? 1 : 0;
    int        anyHasData = 0;
    tarch::mpi::allreduce(&localHas, &anyHasData, 1, MPI_INT, MPI_MAX, comm);

    if (anyHasData == 0) {
      // Every rank is empty: the master alone writes a (serial) valid empty file.
      if (not rank.isGlobalMaster()) {
        return true;
      }
      useParallelIO = false;
    } else {
      MPI_Comm_split(comm, hasData ? 0 : MPI_UNDEFINED, rank.getRank(), &ioComm);
      if (not hasData) {
        return true; // nothing to contribute; the data-holding ranks write the file
      }
      comm       = ioComm;
      int ioRank = 0;
      MPI_Comm_rank(ioComm, &ioRank);
      isMaster = (ioRank == 0);
    }
  }

  if (useParallelIO) {
    unsigned long long local[3]  = {localPoints, localCells, localConnectivity};
    unsigned long long prefix[3] = {0, 0, 0};
    unsigned long long totals[3] = {0, 0, 0};
    tarch::mpi::scan(local, prefix, 3, MPI_UNSIGNED_LONG_LONG, MPI_SUM, comm);
    offsetPoints       = prefix[0] - local[0];
    offsetCells        = prefix[1] - local[1];
    offsetConnectivity = prefix[2] - local[2];
    tarch::mpi::allreduce(local, totals, 3, MPI_UNSIGNED_LONG_LONG, MPI_SUM, comm);
    totalPoints       = totals[0];
    totalCells        = totals[1];
    totalConnectivity = totals[2];
  }

  hid_t fapl = H5P_DEFAULT;
  hid_t dxpl = H5P_DEFAULT;
  if (useParallelIO) {
    fapl             = H5Pcreate(H5P_FILE_ACCESS);
    MPI_Info   info  = MPI_INFO_NULL;
    const auto hints = mpiIoHints();
    if (not hints.empty()) {
      MPI_Info_create(&info);
      for (const auto& hint : hints) {
        MPI_Info_set(info, hint.first.c_str(), hint.second.c_str());
      }
    }
    H5Pset_fapl_mpio(fapl, comm, info);
    if (info != MPI_INFO_NULL) {
      MPI_Info_free(&info);
    }
#if H5_VERSION_GE(1, 10, 0)
    H5Pset_all_coll_metadata_ops(fapl, 1);
    H5Pset_coll_metadata_write(fapl, 1);
#endif
    dxpl = H5Pcreate(H5P_DATASET_XFER);
    // The bulk datasets are deflate-filtered, and parallel HDF5 requires
    // collective transfer for filtered datasets.
    H5Pset_dxpl_mpio(dxpl, H5FD_MPIO_COLLECTIVE);
  }

  hid_t fileId = firstWrite ? H5Fcreate(pathStr.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, fapl)
                            : H5Fopen(pathStr.c_str(), H5F_ACC_RDWR, fapl);
  if (fapl != H5P_DEFAULT) {
    H5Pclose(fapl);
  }
  // H5Pset_fapl_mpio duplicated the communicator into the file, so the local handle is
  // no longer needed (the file keeps its own copy for the collective writes and close).
  if (ioComm != MPI_COMM_NULL) {
    MPI_Comm_free(&ioComm);
  }
  if (fileId < 0) {
    if (dxpl != H5P_DEFAULT) {
      H5Pclose(dxpl);
    }
    throw std::runtime_error("Failed to open VTKHDF file: " + pathStr);
  }
  hid_t lcpl = H5Pcreate(H5P_LINK_CREATE);
  H5Pset_create_intermediate_group(lcpl, 1);

  struct HandleCloser {
    hid_t fileId, lcpl, dxpl;
    ~HandleCloser() {
      H5Pclose(lcpl);
      if (dxpl != H5P_DEFAULT) {
        H5Pclose(dxpl);
      }
      H5Fclose(fileId);
    }
  } handleCloser{fileId, lcpl, dxpl};

  // Open each dataset once per traversal and reuse the handle across the
  // read-extent, extend and write calls below (closed together at scope exit,
  // also on throw). Each reopen would otherwise cost a collective metadata
  // operation. Declared after handleCloser so the datasets close before the file.
  std::map<std::string, hid_t> openDatasets;
  struct DatasetCloser {
    std::map<std::string, hid_t>& datasets;
    ~DatasetCloser() {
      for (auto& entry : datasets) {
        if (entry.second >= 0) {
          H5Dclose(entry.second);
        }
      }
    }
  } datasetCloser{openDatasets};
  auto openDataset = [&](const std::string& name) -> hid_t {
    auto existing = openDatasets.find(name);
    if (existing != openDatasets.end()) {
      return existing->second;
    }
    hid_t handle = H5Dopen(fileId, name.c_str(), H5P_DEFAULT);
    openDatasets.emplace(name, handle);
    return handle;
  };

  auto createExtendible =
    [&](const std::string& name, int numberOfDimensions, hsize_t numberOfComponents, hid_t type, bool compress) {
      hsize_t dims[2]          = {0, numberOfComponents};
      hsize_t maxDimensions[2] = {H5S_UNLIMITED, numberOfComponents};
      // Bulk geometry/data datasets get a large chunk + deflate, while the tiny
      // one-row-per-step index datasets stay uncompressed. Deflate stays readable,
      // as HDF5 and ParaView decompress transparently, and is safe in parallel
      // because every write here is collective.
      // The bulk chunk size trades filter overhead against cross-rank redistribution: a
      // smaller chunk raises the former, a larger one the latter. Index datasets stay tiny.
      hsize_t chunk[2]  = {static_cast<hsize_t>(compress ? 65536 : 1024), numberOfComponents};
      hid_t   dataSpace = H5Screate_simple(numberOfDimensions, dims, maxDimensions);
      hid_t   dcpl      = H5Pcreate(H5P_DATASET_CREATE);
      H5Pset_chunk(dcpl, numberOfDimensions, chunk);
      if (compress) {
        // Skip the pre-fill only on the compressed bulk datasets, where the rank slabs tile
        // each grown chunk so every element is written. The uncompressed index and step
        // datasets carry one value per step, leaving the rest of a chunk untouched, so they
        // keep the default fill.
        H5Pset_fill_time(dcpl, H5D_FILL_TIME_NEVER);
        // Shuffle before deflate improves the ratio of the homogeneous data; reversed on read.
        H5Pset_shuffle(dcpl);
        H5Pset_deflate(dcpl, 1);
      }
      hid_t dataset = H5Dcreate(fileId, name.c_str(), type, dataSpace, lcpl, dcpl, H5P_DEFAULT);
      H5Pclose(dcpl);
      H5Sclose(dataSpace);
      if (dataset < 0) {
        throw std::runtime_error("VTKHDF: failed to create dataset " + name);
      }
      H5Dclose(dataset);
    };
  auto rowsOf = [&](const std::string& name) -> hsize_t {
    hid_t   dataSpace = H5Dget_space(openDataset(name));
    hsize_t dims[2]   = {0, 0};
    H5Sget_simple_extent_dims(dataSpace, dims, nullptr);
    H5Sclose(dataSpace);
    return dims[0];
  };
  // Extend a dataset to (currentRows + globalAdd) rows and write this rank's slab at
  // the exclusive-prefix offset within the new block. With collective IO an empty
  // rank issues a balanced empty write so the collective does not deadlock.
  auto appendSlab = [&](
                      const std::string& name,
                      int                numberOfDimensions,
                      hsize_t            numberOfComponents,
                      hid_t              type,
                      hsize_t            currentRows,
                      hsize_t            globalAdd,
                      hsize_t            localRows,
                      hsize_t            localOffset,
                      const void*        data
                    ) {
    hid_t   dataset          = openDataset(name);
    hsize_t newDimensions[2] = {currentRows + globalAdd, numberOfComponents};
    H5Dset_extent(dataset, newDimensions);
    hid_t fileSpace = H5Dget_space(dataset);
    if (localRows > 0) {
      hsize_t start[2] = {currentRows + localOffset, 0};
      hsize_t count[2] = {localRows, numberOfComponents};
      H5Sselect_hyperslab(fileSpace, H5S_SELECT_SET, start, nullptr, count, nullptr);
      hid_t memorySpace = H5Screate_simple(numberOfDimensions, count, nullptr);
      if (H5Dwrite(dataset, type, memorySpace, fileSpace, dxpl, data) < 0) {
        H5Sclose(memorySpace);
        H5Sclose(fileSpace);
        throw std::runtime_error("VTKHDF hyperslab write failed: " + name);
      }
      H5Sclose(memorySpace);
    } else if (dxpl != H5P_DEFAULT) {
      // Collective filtered I/O: an empty rank must still join the write with an
      // empty selection so all ranks issue the collective the same number of times.
      H5Sselect_none(fileSpace);
      H5Dwrite(dataset, type, H5S_ALL, fileSpace, dxpl, nullptr);
    }
    H5Sclose(fileSpace);
  };
  // Append one row to a 1D index dataset. The caller passes the row (= step), so we skip
  // an H5Dget_space per dataset. Extent and write are both collective; the master supplies
  // the value and the other ranks join with an empty selection.
  auto appendStepRow = [&](const std::string& name, hid_t type, const void* value, hsize_t row) {
    hid_t   dataset          = openDataset(name);
    hsize_t newDimensions[1] = {row + 1};
    H5Dset_extent(dataset, newDimensions);
    hid_t fileSpace = H5Dget_space(dataset);
    // Collective write: the master writes the single value, the other ranks join with an
    // empty selection. A master-only write to a dataset accessed by all ranks yields wrong
    // data under parallel HDF5 on a shared filesystem, so every rank participates, as for
    // the bulk data.
    if (isMaster) {
      hsize_t start[1] = {row};
      hsize_t count[1] = {1};
      H5Sselect_hyperslab(fileSpace, H5S_SELECT_SET, start, nullptr, count, nullptr);
      hid_t memorySpace = H5Screate_simple(1, count, nullptr);
      H5Dwrite(dataset, type, memorySpace, fileSpace, dxpl, value);
      H5Sclose(memorySpace);
    } else if (dxpl != H5P_DEFAULT) {
      H5Sselect_none(fileSpace);
      H5Dwrite(dataset, type, H5S_ALL, fileSpace, dxpl, nullptr);
    }
    H5Sclose(fileSpace);
  };
  // Write the trailing Offsets sentinel. The preceding appendSlab already grew the extent
  // collectively, so only the value is still missing.
  auto writeI64At = [&](const std::string& name, hsize_t position, long long value) {
    hid_t dataset   = openDataset(name);
    hid_t fileSpace = H5Dget_space(dataset);
    // Collective: master writes the sentinel, the other ranks join with an empty selection.
    if (isMaster) {
      hsize_t start[1] = {position};
      hsize_t count[1] = {1};
      H5Sselect_hyperslab(fileSpace, H5S_SELECT_SET, start, nullptr, count, nullptr);
      hid_t memorySpace = H5Screate_simple(1, count, nullptr);
      H5Dwrite(dataset, H5T_NATIVE_LLONG, memorySpace, fileSpace, dxpl, &value);
      H5Sclose(memorySpace);
    } else if (dxpl != H5P_DEFAULT) {
      H5Sselect_none(fileSpace);
      H5Dwrite(dataset, H5T_NATIVE_LLONG, H5S_ALL, fileSpace, dxpl, nullptr);
    }
    H5Sclose(fileSpace);
  };

  if (firstWrite) {
    hid_t group = H5Gcreate(fileId, "/VTKHDF", lcpl, H5P_DEFAULT, H5P_DEFAULT);
    {
      // Declare the lowest specification version that covers the features used
      // (temporal unstructured grid). Readers reject files whose declared version
      // is newer than they support, so a higher number would lock out ParaView
      // releases that can read this file.
      hsize_t   dimensions[1] = {2};
      long long version[2]    = {2, 0};
      hid_t     dataSpace     = H5Screate_simple(1, dimensions, nullptr);
      hid_t     attribute     = H5Acreate(group, "Version", H5T_STD_I64LE, dataSpace, H5P_DEFAULT, H5P_DEFAULT);
      H5Awrite(attribute, H5T_NATIVE_LLONG, version);
      H5Aclose(attribute);
      H5Sclose(dataSpace);
    }
    {
      const char* type       = "UnstructuredGrid";
      hid_t       stringType = H5Tcopy(H5T_C_S1);
      H5Tset_size(stringType, std::strlen(type));
      hid_t dataSpace = H5Screate(H5S_SCALAR);
      hid_t attribute = H5Acreate(group, "Type", stringType, dataSpace, H5P_DEFAULT, H5P_DEFAULT);
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

    H5Gclose(H5Gcreate(fileId, "/VTKHDF/Steps", lcpl, H5P_DEFAULT, H5P_DEFAULT));
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
    hid_t     stepsGroup = H5Gopen(fileId, "/VTKHDF/Steps", H5P_DEFAULT);
    hid_t     dataSpace  = H5Screate(H5S_SCALAR);
    long long zero       = 0;
    hid_t     attribute  = H5Acreate(stepsGroup, "NSteps", H5T_STD_I64LE, dataSpace, H5P_DEFAULT, H5P_DEFAULT);
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
  std::vector<double>  pointsBuffer;
  std::vector<int64_t> connectivityBuffer, beforeOffsets;
  std::vector<uint8_t> typesBuffer;
  pointsBuffer.reserve(localPoints * 3);
  connectivityBuffer.reserve(localConnectivity);
  beforeOffsets.reserve(localCells);
  typesBuffer.reserve(localCells);
  unsigned long long pointCursor        = 0;
  unsigned long long connectivityCursor = 0;
  for (const SubdomainBuffers* subdomain : localData.payloads) {
    const int64_t pointBase = static_cast<int64_t>(offsetPoints + pointCursor);
    pointsBuffer.insert(pointsBuffer.end(), subdomain->pointsXYZ.begin(), subdomain->pointsXYZ.end());
    for (auto connectivityEntry : subdomain->connectivity) {
      connectivityBuffer.push_back(static_cast<int64_t>(connectivityEntry) + pointBase);
    }
    int32_t previousEnd = 0;
    for (std::size_t i = 0; i < subdomain->types.size(); i++) {
      beforeOffsets.push_back(static_cast<int64_t>(offsetConnectivity + connectivityCursor) + previousEnd);
      previousEnd = subdomain->offsets[i];
    }
    typesBuffer.insert(typesBuffer.end(), subdomain->types.begin(), subdomain->types.end());
    pointCursor += subdomain->pointsXYZ.size() / 3;
    connectivityCursor += subdomain->connectivity.size();
  }

  // The buffers accumulate in double precision; single-precision output narrows
  // them right before the write.
  const bool writeDouble    = getDoublePrecisionOutput();
  auto       appendRealSlab = [&](
                          const std::string&         name,
                          int                        components,
                          hsize_t                    current,
                          hsize_t                    total,
                          hsize_t                    local,
                          hsize_t                    offset,
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
  // Offsets per step: one before-offset per cell + a trailing sentinel (= step
  // connectivity total), so the step block has totalCells + 1 entries.
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
  writeI64At("/VTKHDF/Offsets", currentOffsets + totalCells, static_cast<long long>(totalConnectivity));

  // Every subdomain shares the schema's field order (validateSubdomainsForWriting),
  // so index by position instead of searching by name.
  for (std::size_t fieldIndex = 0; fieldIndex < schema.pointFields.size(); ++fieldIndex) {
    const auto&         field   = schema.pointFields[fieldIndex];
    const int           written = classifyField(field.components).writtenComponents;
    std::vector<double> fieldBuffer;
    fieldBuffer.reserve(localPoints * field.components);
    for (const SubdomainBuffers* subdomain : localData.payloads) {
      const auto& values = subdomain->pointFields[fieldIndex].values;
      fieldBuffer.insert(fieldBuffer.end(), values.begin(), values.end());
    }
    // Skip the copy for scalars; only vectors/tensors are widened.
    if (written != field.components) {
      fieldBuffer = expandFieldComponents(fieldBuffer, field.components, written);
    }
    // Point fields share the Points extent (currentPoints), so no per-dataset query.
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
    for (const SubdomainBuffers* subdomain : localData.payloads) {
      const auto& values = subdomain->cellFields[fieldIndex].values;
      fieldBuffer.insert(fieldBuffer.end(), values.begin(), values.end());
    }
    if (written != field.components) {
      fieldBuffer = expandFieldComponents(fieldBuffer, field.components, written);
    }
    // Cell fields share the Types extent (currentCells).
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

  hid_t     stepsGroup    = H5Gopen(fileId, "/VTKHDF/Steps", H5P_DEFAULT);
  hid_t     attribute     = H5Aopen(stepsGroup, "NSteps", H5P_DEFAULT);
  long long numberOfSteps = currentStep + 1;
  H5Awrite(attribute, H5T_NATIVE_LLONG, &numberOfSteps);
  H5Aclose(attribute);
  H5Gclose(stepsGroup);

  return true;
}

#endif
