// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "VTKHDFFileWriter.h"

#ifdef PEANO_WITH_HDF5

#include "tarch/Termination.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Handles.h"
#include "tarch/mpi/Rank.h"

#include <hdf5.h>

tarch::plotter::Format
tarch::plotter::VTKHDFFileWriter::getFormat() const {
  return Format::VTKHDF;
}

void
tarch::plotter::VTKHDFFileWriter::write(
  const Mesh&        mesh,
  const std::string& outputDirectory,
  const std::string& filename,
  const double       time
) {
  validateMesh(mesh);
  createOutputDirectory(outputDirectory);

  const std::string filePath   = (std::filesystem::path(outputDirectory) / (filename + ".vtkhdf")).string();
  const bool        firstWrite = _createdFiles.insert(filePath).second;

  const auto check = [&filePath](const herr_t status) {
    if (status < 0) {
      tarch::terminateWithError("The plotter cannot write the file '" + filePath + "'.");
    }
  };

  // The meshes of all ranks form one VTKHDF part per step.
  const unsigned long long localPoints        = mesh.pointsXYZ.size() / 3;
  const unsigned long long localCells         = mesh.types.size();
  const unsigned long long localConnectivity  = mesh.connectivity.size();
  unsigned long long       offsetPoints       = 0;
  unsigned long long       offsetCells        = 0;
  unsigned long long       offsetConnectivity = 0;
  unsigned long long       totalPoints        = localPoints;
  unsigned long long       totalCells         = localCells;
  unsigned long long       totalConnectivity  = localConnectivity;

  const auto&    rank          = tarch::mpi::Rank::getInstance();
  const MPI_Comm comm          = rank.getCommunicator();
  const bool     useParallelIO = rank.getNumberOfRanks() > 1;
  const bool     isMaster      = rank.isGlobalMaster();
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
    MPI_Info hints       = createMPIFileHints();
    H5Pset_fapl_mpio(fileAccessProperties, comm, hints);
    tarch::mpi::freeInfo(&hints);
#if H5_VERSION_GE(1, 10, 0)
    H5Pset_all_coll_metadata_ops(fileAccessProperties, 1);
    H5Pset_coll_metadata_write(fileAccessProperties, 1);
#endif
    dataTransferProperties = H5Pcreate(H5P_DATASET_XFER);
    // Parallel HDF5 writes a deflated dataset collectively.
    H5Pset_dxpl_mpio(dataTransferProperties, H5FD_MPIO_COLLECTIVE);
  }

  const hid_t fileId = firstWrite ? H5Fcreate(filePath.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, fileAccessProperties)
                                  : H5Fopen(filePath.c_str(), H5F_ACC_RDWR, fileAccessProperties);
  if (fileAccessProperties != H5P_DEFAULT) {
    H5Pclose(fileAccessProperties);
  }
  if (fileId < 0) {
    tarch::terminateWithError("The plotter cannot open the file '" + filePath + "'.");
  }
  const hid_t linkCreationProperties = H5Pcreate(H5P_LINK_CREATE);
  H5Pset_create_intermediate_group(linkCreationProperties, 1);

  std::map<std::string, hid_t> openDatasets;
  const auto                   openDataset = [&](const std::string& name) -> hid_t {
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
    const hsize_t dimensions[2]             = {0, numberOfComponents};
    const hsize_t maxDimensions[2]          = {H5S_UNLIMITED, numberOfComponents};
    const hsize_t chunkDimensions[2]        = {static_cast<hsize_t>(compress ? 65536 : 1024), numberOfComponents};
    const hid_t   dataSpace                 = H5Screate_simple(numberOfDimensions, dimensions, maxDimensions);
    const hid_t   datasetCreationProperties = H5Pcreate(H5P_DATASET_CREATE);
    H5Pset_chunk(datasetCreationProperties, numberOfDimensions, chunkDimensions);
    if (compress) {
      // The slabs of the ranks write every element of a compressed dataset. HDF5 writes no fill
      // value into its chunks.
      H5Pset_fill_time(datasetCreationProperties, H5D_FILL_TIME_NEVER);
      H5Pset_shuffle(datasetCreationProperties);
      H5Pset_deflate(datasetCreationProperties, 1);
    }
    const hid_t dataset =
      H5Dcreate(fileId, name.c_str(), type, dataSpace, linkCreationProperties, datasetCreationProperties, H5P_DEFAULT);
    H5Pclose(datasetCreationProperties);
    H5Sclose(dataSpace);
    if (dataset < 0) {
      tarch::terminateWithError("The plotter cannot create the dataset '" + name + "' in the file '" + filePath + "'.");
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
      check(H5Dwrite(dataset, type, memorySpace, fileSpace, dataTransferProperties, data));
      H5Sclose(memorySpace);
    } else if (useParallelIO) {
      // An empty rank joins the collective write with an empty selection. Every rank issues the
      // same number of collective writes.
      H5Sselect_none(fileSpace);
      check(H5Dwrite(dataset, type, H5S_ALL, fileSpace, dataTransferProperties, nullptr));
    }
    H5Sclose(fileSpace);
  };
  // The master writes value into the entry position of a one-dimensional dataset.
  const auto writeAt = [&](const std::string& name, const hid_t type, const void* const value, const hsize_t position) {
    const hid_t dataset   = openDataset(name);
    const hid_t fileSpace = H5Dget_space(dataset);
    if (isMaster) {
      const hsize_t start[1] = {position};
      const hsize_t count[1] = {1};
      H5Sselect_hyperslab(fileSpace, H5S_SELECT_SET, start, nullptr, count, nullptr);
      const hid_t memorySpace = H5Screate_simple(1, count, nullptr);
      check(H5Dwrite(dataset, type, memorySpace, fileSpace, dataTransferProperties, value));
      H5Sclose(memorySpace);
    } else if (useParallelIO) {
      H5Sselect_none(fileSpace);
      check(H5Dwrite(dataset, type, H5S_ALL, fileSpace, dataTransferProperties, nullptr));
    }
    H5Sclose(fileSpace);
  };
  const auto appendStepRow =
    [&](const std::string& name, const hid_t type, const void* const value, const hsize_t row) {
      const hsize_t newDimensions[1] = {row + 1};
      H5Dset_extent(openDataset(name), newDimensions);
      writeAt(name, type, value, row);
    };

  if (firstWrite) {
    const hid_t group = H5Gcreate(fileId, "/VTKHDF", linkCreationProperties, H5P_DEFAULT, H5P_DEFAULT);
    {
      // A reader rejects a file whose declared version is newer than the version it supports.
      // Version 2.0 is the lowest version that covers a temporal unstructured grid.
      const hsize_t   dimensions[1] = {2};
      const long long version[2]    = {2, 0};
      const hid_t     dataSpace     = H5Screate_simple(1, dimensions, nullptr);
      const hid_t     attribute     = H5Acreate(group, "Version", H5T_STD_I64LE, dataSpace, H5P_DEFAULT, H5P_DEFAULT);
      check(H5Awrite(attribute, H5T_NATIVE_LLONG, version));
      H5Aclose(attribute);
      H5Sclose(dataSpace);
    }
    {
      const char* const type       = "UnstructuredGrid";
      const hid_t       stringType = H5Tcopy(H5T_C_S1);
      H5Tset_size(stringType, std::strlen(type));
      const hid_t dataSpace = H5Screate(H5S_SCALAR);
      const hid_t attribute = H5Acreate(group, "Type", stringType, dataSpace, H5P_DEFAULT, H5P_DEFAULT);
      check(H5Awrite(attribute, stringType, type));
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
    // ParaView reads three components as a vector and nine as a tensor.
    for (const auto& field : mesh.pointFields) {
      const int written = classifyField(field.components).writtenComponents;
      createExtendible("/VTKHDF/PointData/" + field.name, 2, written, realStorageType, true);
    }
    for (const auto& field : mesh.cellFields) {
      const int written = classifyField(field.components).writtenComponents;
      createExtendible("/VTKHDF/CellData/" + field.name, 2, written, realStorageType, true);
    }

    H5Gclose(H5Gcreate(fileId, "/VTKHDF/Steps", linkCreationProperties, H5P_DEFAULT, H5P_DEFAULT));
    createExtendible("/VTKHDF/Steps/Values", 1, 1, realStorageType, false);
    createExtendible("/VTKHDF/Steps/PartOffsets", 1, 1, H5T_STD_I64LE, false);
    createExtendible("/VTKHDF/Steps/PointOffsets", 1, 1, H5T_STD_I64LE, false);
    createExtendible("/VTKHDF/Steps/CellOffsets", 1, 1, H5T_STD_I64LE, false);
    createExtendible("/VTKHDF/Steps/ConnectivityIdOffsets", 1, 1, H5T_STD_I64LE, false);
    for (const auto& field : mesh.pointFields) {
      createExtendible("/VTKHDF/Steps/PointDataOffsets/" + field.name, 1, 1, H5T_STD_I64LE, false);
    }
    for (const auto& field : mesh.cellFields) {
      createExtendible("/VTKHDF/Steps/CellDataOffsets/" + field.name, 1, 1, H5T_STD_I64LE, false);
    }
    const hid_t     stepsGroup = H5Gopen(fileId, "/VTKHDF/Steps", H5P_DEFAULT);
    const hid_t     dataSpace  = H5Screate(H5S_SCALAR);
    const long long zero       = 0;
    const hid_t     attribute  = H5Acreate(stepsGroup, "NSteps", H5T_STD_I64LE, dataSpace, H5P_DEFAULT, H5P_DEFAULT);
    check(H5Awrite(attribute, H5T_NATIVE_LLONG, &zero));
    H5Aclose(attribute);
    H5Sclose(dataSpace);
    H5Gclose(stepsGroup);
  }

  // The extents of the datasets before this step give the offsets of the step in /VTKHDF/Steps.
  const long long currentPoints       = static_cast<long long>(rowsOf("/VTKHDF/Points"));
  const long long currentConnectivity = static_cast<long long>(rowsOf("/VTKHDF/Connectivity"));
  const long long currentCells        = static_cast<long long>(rowsOf("/VTKHDF/Types"));
  const hsize_t   currentOffsets      = rowsOf("/VTKHDF/Offsets");
  const long long currentStep         = static_cast<long long>(rowsOf("/VTKHDF/Steps/Values"));

  const bool        writeDouble  = getDoublePrecisionOutput();
  const hid_t       realType     = writeDouble ? H5T_NATIVE_DOUBLE : H5T_NATIVE_FLOAT;
  const std::size_t realBytes    = writeDouble ? sizeof(double) : sizeof(float);
  std::size_t       scratchBytes = std::max<std::size_t>(
    localPoints * 3 * realBytes,
    std::max<std::size_t>(localConnectivity, localCells) * sizeof(std::int64_t)
  );
  for (const auto& field : mesh.pointFields) {
    scratchBytes =
      std::max<std::size_t>(scratchBytes, localPoints * classifyField(field.components).writtenComponents * realBytes);
  }
  for (const auto& field : mesh.cellFields) {
    scratchBytes =
      std::max<std::size_t>(scratchBytes, localCells * classifyField(field.components).writtenComponents * realBytes);
  }
  MeshArray<std::byte> scratch(scratchBytes);

  // Returns values in the stored precision with written components per tuple. The lambda returns the
  // array values itself if the file stores doubles and written equals components.
  const auto getStoredValues = [&](const MeshArray<double>& values, const int components, const int written) {
    const std::size_t tuples = values.size() / static_cast<std::size_t>(components);
    assertion(tuples * static_cast<std::size_t>(written) * realBytes <= scratch.size(), tuples, written);
    if (writeDouble and components == written) {
      return static_cast<const void*>(values.data());
    }
    if (writeDouble) {
      copyTuples(values.data(), components, components, written, tuples, reinterpret_cast<double*>(scratch.data()));
    } else {
      copyTuples(values.data(), components, components, written, tuples, reinterpret_cast<float*>(scratch.data()));
    }
    return static_cast<const void*>(scratch.data());
  };

  appendSlab(
    "/VTKHDF/Points",
    2,
    3,
    realType,
    currentPoints,
    totalPoints,
    localPoints,
    offsetPoints,
    getStoredValues(mesh.pointsXYZ, 3, 3)
  );

  // The point indices in Connectivity count from the first point of the step.
  auto* const integers = reinterpret_cast<std::int64_t*>(scratch.data());
  forEachRangeInParallel(localConnectivity, [&](const std::size_t firstEntry, const std::size_t endEntry) {
    for (std::size_t entry = firstEntry; entry < endEntry; entry++) {
      integers[entry] = static_cast<std::int64_t>(mesh.connectivity[entry]) + static_cast<std::int64_t>(offsetPoints);
    }
  });
  appendSlab(
    "/VTKHDF/Connectivity",
    1,
    1,
    H5T_NATIVE_INT64,
    currentConnectivity,
    totalConnectivity,
    localConnectivity,
    offsetConnectivity,
    integers
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
    mesh.types.data()
  );

  // The Offsets block of a step holds totalCells + 1 entries. The entry of a cell lists the first
  // connectivity entry of the cell. The last entry counts the connectivity entries of the step.
  forEachRangeInParallel(localCells, [&](const std::size_t firstCell, const std::size_t endCell) {
    for (std::size_t cell = firstCell; cell < endCell; cell++) {
      integers[cell] = static_cast<std::int64_t>(offsetConnectivity) + (cell == 0 ? 0 : mesh.offsets[cell - 1]);
    }
  });
  appendSlab(
    "/VTKHDF/Offsets",
    1,
    1,
    H5T_NATIVE_INT64,
    currentOffsets,
    totalCells + 1,
    localCells,
    offsetCells,
    integers
  );
  const long long numberOfConnectivityIds = static_cast<long long>(totalConnectivity);
  writeAt("/VTKHDF/Offsets", H5T_NATIVE_LLONG, &numberOfConnectivityIds, currentOffsets + totalCells);

  const auto appendFields = [&](
                              const std::vector<Mesh::Field>& fields,
                              const std::string&              dataGroup,
                              const std::string&              offsetsGroup,
                              const long long&                currentEntities,
                              const unsigned long long        totalEntities,
                              const unsigned long long        localEntities,
                              const unsigned long long        offsetEntities
                            ) {
    for (const auto& field : fields) {
      const int written = classifyField(field.components).writtenComponents;
      appendSlab(
        dataGroup + field.name,
        2,
        written,
        realType,
        static_cast<hsize_t>(currentEntities),
        totalEntities,
        localEntities,
        offsetEntities,
        getStoredValues(field.values, field.components, written)
      );
      appendStepRow(offsetsGroup + field.name, H5T_NATIVE_LLONG, &currentEntities, static_cast<hsize_t>(currentStep));
    }
  };
  appendFields(
    mesh.pointFields,
    "/VTKHDF/PointData/",
    "/VTKHDF/Steps/PointDataOffsets/",
    currentPoints,
    totalPoints,
    localPoints,
    offsetPoints
  );
  appendFields(
    mesh.cellFields,
    "/VTKHDF/CellData/",
    "/VTKHDF/Steps/CellDataOffsets/",
    currentCells,
    totalCells,
    localCells,
    offsetCells
  );

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
  check(H5Awrite(attribute, H5T_NATIVE_LLONG, &numberOfSteps));
  H5Aclose(attribute);
  H5Gclose(stepsGroup);

  for (const auto& entry : openDatasets) {
    H5Dclose(entry.second);
  }
  H5Pclose(linkCreationProperties);
  if (dataTransferProperties != H5P_DEFAULT) {
    H5Pclose(dataTransferProperties);
  }
  check(H5Fclose(fileId));
}

#endif
