// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "HDF5FileWriter.h"

#ifdef PEANO_WITH_HDF5

#include "tarch/Termination.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Handles.h"
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/CellTypes.h"

#include <hdf5.h>

namespace {
  int toXdmfType(const std::uint8_t vtkType) {
    switch (static_cast<tarch::plotter::CellType>(vtkType)) {
    case tarch::plotter::CellType::Vertex:
      return 1;
    case tarch::plotter::CellType::Line:
      return 2;
    case tarch::plotter::CellType::Quadrangle:
      return 5;
    case tarch::plotter::CellType::Hexahedron:
      return 9;
    }
    ASSERTION(
      false,
      "The HDF5 plotter writes vertices, lines, quadrangles and hexahedra only.",
      static_cast<int>(vtkType)
    );
    return 0;
  }

  /**
   * Returns whether xdmfType names a polyvertex, a polyline or a polygon. A mixed XDMF topology lists
   * the node count of such a cell.
   */
  bool xdmfTypeNeedsNodeCount(const int xdmfType) { return xdmfType >= 1 and xdmfType <= 3; }
} // namespace

tarch::plotter::Format
tarch::plotter::HDF5FileWriter::getFormat() const {
  return Format::HDF5;
}

void
tarch::plotter::HDF5FileWriter::write(
  const Mesh&        mesh,
  const std::string& outputDirectory,
  const std::string& filename,
  const double       time
) {
  validateMesh(mesh);
  createOutputDirectory(outputDirectory);

  const std::filesystem::path directory(outputDirectory);
  const std::string           snapshotName = getSnapshotName(filename, time);
  const std::filesystem::path h5FilePath   = directory / (snapshotName + ".h5");

  const auto check = [&h5FilePath](const herr_t status) {
    if (status < 0) {
      tarch::terminateWithError("The plotter cannot write the file '" + h5FilePath.string() + "'.");
    }
  };

  const unsigned long long localNumberOfPoints = mesh.pointsXYZ.size() / 3;
  const unsigned long long localNumberOfCells  = mesh.types.size();

  // The XDMF mixed topology lists the type, the node count of some types and the nodes of every cell.
  std::vector<unsigned long long> topologyEntriesBeforeRange(
    (localNumberOfCells + EntriesPerRange - 1) / EntriesPerRange + 1,
    0
  );
  forEachRangeInParallel(localNumberOfCells, [&](const std::size_t firstCell, const std::size_t endCell) {
    unsigned long long entries = 0;
    for (std::size_t cell = firstCell; cell < endCell; cell++) {
      entries += xdmfTypeNeedsNodeCount(toXdmfType(mesh.types[cell])) ? 2 : 1;
    }
    const std::int32_t firstNode = firstCell == 0 ? 0 : mesh.offsets[firstCell - 1];
    entries += static_cast<unsigned long long>(mesh.offsets[endCell - 1] - firstNode);
    topologyEntriesBeforeRange[firstCell / EntriesPerRange + 1] = entries;
  });
  std::partial_sum(
    topologyEntriesBeforeRange.begin(),
    topologyEntriesBeforeRange.end(),
    topologyEntriesBeforeRange.begin()
  );
  const unsigned long long localTopologySize = topologyEntriesBeforeRange.back();

  unsigned long long offsetPoints   = 0;
  unsigned long long offsetCells    = 0;
  unsigned long long offsetTopology = 0;
  unsigned long long totalPoints    = localNumberOfPoints;
  unsigned long long totalCells     = localNumberOfCells;
  unsigned long long totalTopology  = localTopologySize;

  const auto&    rank          = tarch::mpi::Rank::getInstance();
  const MPI_Comm comm          = rank.getCommunicator();
  const bool     useParallelIO = rank.getNumberOfRanks() > 1;
  if (useParallelIO) {
    const unsigned long long local[3]  = {localNumberOfPoints, localNumberOfCells, localTopologySize};
    unsigned long long       prefix[3] = {0ULL, 0ULL, 0ULL};
    unsigned long long       totals[3] = {0ULL, 0ULL, 0ULL};
    tarch::mpi::scan(local, prefix, 3, MPI_UNSIGNED_LONG_LONG, MPI_SUM, comm);
    offsetPoints   = prefix[0] - local[0];
    offsetCells    = prefix[1] - local[1];
    offsetTopology = prefix[2] - local[2];
    tarch::mpi::allreduce(local, totals, 3, MPI_UNSIGNED_LONG_LONG, MPI_SUM, comm);
    totalPoints   = totals[0];
    totalCells    = totals[1];
    totalTopology = totals[2];
  }

  if (offsetPoints + localNumberOfPoints > static_cast<unsigned long long>(std::numeric_limits<int>::max()) + 1) {
    tarch::terminateWithError("The HDF5 plotter numbers the points of a plot with 32-bit integers only.");
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
    H5Pset_dxpl_mpio(dataTransferProperties, H5FD_MPIO_COLLECTIVE);
  }

  const hid_t fileId = H5Fcreate(h5FilePath.string().c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, fileAccessProperties);
  if (fileAccessProperties != H5P_DEFAULT) {
    H5Pclose(fileAccessProperties);
  }
  if (fileId < 0) {
    tarch::terminateWithError("The plotter cannot create the file '" + h5FilePath.string() + "'.");
  }
  const hid_t linkCreationProperties = H5Pcreate(H5P_LINK_CREATE);
  H5Pset_create_intermediate_group(linkCreationProperties, 1);

  const bool        writeDouble     = getDoublePrecisionOutput();
  const hid_t       realStorageType = writeDouble ? H5T_NATIVE_DOUBLE : H5T_NATIVE_FLOAT;
  const std::size_t realBytes       = writeDouble ? sizeof(double) : sizeof(float);
  std::size_t scratchBytes = std::max<std::size_t>(localNumberOfPoints * realBytes, localTopologySize * sizeof(int));
  for (const auto& field : mesh.pointFields) {
    scratchBytes = std::max<std::size_t>(
      scratchBytes,
      localNumberOfPoints * classifyField(field.components).writtenComponents * realBytes
    );
  }
  for (const auto& field : mesh.cellFields) {
    scratchBytes = std::max<std::size_t>(
      scratchBytes,
      localNumberOfCells * classifyField(field.components).writtenComponents * realBytes
    );
  }
  MeshArray<std::byte> scratch(scratchBytes);

  // Every rank writes its rows at data to a dataset at offset. A dataset of zero rows stays absent.
  const auto writeDataset = [&](
                              const std::string&   name,
                              const int            numberOfDimensions,
                              const hsize_t* const globalDimensions,
                              const hsize_t* const localDimensions,
                              const hsize_t* const offset,
                              const hid_t          type,
                              const void* const    data
                            ) {
    if (globalDimensions[0] == 0) {
      return;
    }

    // Parallel HDF5 writes a chunked and deflated dataset collectively.
    constexpr hsize_t ChunkByteTarget    = 1U << 20;
    const hsize_t     columns            = numberOfDimensions > 1 ? globalDimensions[1] : 1;
    const hsize_t     bytesPerRow        = std::max<hsize_t>(1, columns * static_cast<hsize_t>(H5Tget_size(type)));
    const hsize_t     targetRows         = std::max<hsize_t>(1, ChunkByteTarget / bytesPerRow);
    const hsize_t     chunkDimensions[2] = {std::max<hsize_t>(1, std::min(globalDimensions[0], targetRows)), columns};
    const hid_t       datasetCreationProperties = H5Pcreate(H5P_DATASET_CREATE);
    H5Pset_chunk(datasetCreationProperties, numberOfDimensions, chunkDimensions);
    // The slabs of the ranks write every element of the dataset. HDF5 skips the fill value of its
    // chunks.
    H5Pset_fill_time(datasetCreationProperties, H5D_FILL_TIME_NEVER);
    H5Pset_shuffle(datasetCreationProperties);
    H5Pset_deflate(datasetCreationProperties, 1);

    const hid_t fileSpace = H5Screate_simple(numberOfDimensions, globalDimensions, nullptr);
    const hid_t dataset =
      H5Dcreate(fileId, name.c_str(), type, fileSpace, linkCreationProperties, datasetCreationProperties, H5P_DEFAULT);
    H5Pclose(datasetCreationProperties);
    if (dataset < 0) {
      tarch::terminateWithError(
        "The plotter cannot create the dataset '" + name + "' in the file '" + h5FilePath.string() + "'."
      );
    }

    if (localDimensions[0] > 0) {
      const hid_t  memorySpace = H5Screate_simple(numberOfDimensions, localDimensions, nullptr);
      const herr_t selectStatus =
        H5Sselect_hyperslab(fileSpace, H5S_SELECT_SET, offset, nullptr, localDimensions, nullptr);
      check(
        selectStatus >= 0 ? H5Dwrite(dataset, type, memorySpace, fileSpace, dataTransferProperties, data) : selectStatus
      );
      H5Sclose(memorySpace);
    } else if (useParallelIO) {
      // An empty rank joins the collective write with an empty selection. Every rank issues the
      // same number of collective writes.
      H5Sselect_none(fileSpace);
      check(H5Dwrite(dataset, type, H5S_ALL, fileSpace, dataTransferProperties, nullptr));
    }
    H5Sclose(fileSpace);
    H5Dclose(dataset);
  };

  const auto copyIntoScratch = [&](
                                 const double* const source,
                                 const std::size_t   tuples,
                                 const int           sourceStride,
                                 const int           copiedComponents,
                                 const int           targetStride
                               ) {
    ASSERTION(tuples * static_cast<std::size_t>(targetStride) * realBytes <= scratch.size(), tuples, scratch.size());
    if (writeDouble) {
      copyTuples(
        source,
        sourceStride,
        copiedComponents,
        targetStride,
        tuples,
        reinterpret_cast<double*>(scratch.data())
      );
    } else {
      copyTuples(
        source,
        sourceStride,
        copiedComponents,
        targetStride,
        tuples,
        reinterpret_cast<float*>(scratch.data())
      );
    }
  };

  for (const char axis : {'x', 'y', 'z'}) {
    const auto    axisIndex               = static_cast<std::size_t>(axis - 'x');
    const hsize_t coordinateDimensions[1] = {totalPoints};
    const hsize_t localDimensions[1]      = {localNumberOfPoints};
    const hsize_t offset[1]               = {offsetPoints};
    copyIntoScratch(
      localNumberOfPoints > 0 ? mesh.pointsXYZ.data() + axisIndex : nullptr,
      localNumberOfPoints,
      3,
      1,
      1
    );
    writeDataset(
      std::string("/geometry/") + axis,
      1,
      coordinateDimensions,
      localDimensions,
      offset,
      realStorageType,
      scratch.data()
    );
  }

  {
    auto* const        topology  = reinterpret_cast<std::int32_t*>(scratch.data());
    const std::int32_t pointBase = static_cast<std::int32_t>(offsetPoints);
    forEachRangeInParallel(localNumberOfCells, [&](const std::size_t firstCell, const std::size_t endCell) {
      std::size_t entry = topologyEntriesBeforeRange[firstCell / EntriesPerRange];
      for (std::size_t cell = firstCell; cell < endCell; cell++) {
        const int          xdmfType  = toXdmfType(mesh.types[cell]);
        const std::int32_t firstNode = cell == 0 ? 0 : mesh.offsets[cell - 1];
        topology[entry]              = xdmfType;
        entry++;
        if (xdmfTypeNeedsNodeCount(xdmfType)) {
          topology[entry] = mesh.offsets[cell] - firstNode;
          entry++;
        }
        for (std::int32_t node = firstNode; node < mesh.offsets[cell]; node++) {
          topology[entry] = mesh.connectivity[static_cast<std::size_t>(node)] + pointBase;
          entry++;
        }
      }
      ASSERTION(entry == topologyEntriesBeforeRange[firstCell / EntriesPerRange + 1], entry, firstCell);
    });
    const hsize_t globalTopologyDimensions[1] = {totalTopology};
    const hsize_t localTopologyDimensions[1]  = {localTopologySize};
    const hsize_t topologyOffset[1]           = {offsetTopology};
    writeDataset(
      "/topology",
      1,
      globalTopologyDimensions,
      localTopologyDimensions,
      topologyOffset,
      H5T_NATIVE_INT,
      topology
    );
  }

  const auto writeFields = [&](
                             const std::vector<Mesh::Field>& fields,
                             const std::string&              group,
                             const unsigned long long        localEntities,
                             const unsigned long long        totalEntities,
                             const unsigned long long        offsetEntities
                           ) {
    for (const auto& field : fields) {
      const int     written                 = classifyField(field.components).writtenComponents;
      const hsize_t fieldDimensions[2]      = {totalEntities, static_cast<hsize_t>(written)};
      const hsize_t localFieldDimensions[2] = {localEntities, static_cast<hsize_t>(written)};
      const hsize_t fieldOffset[2]          = {offsetEntities, 0};
      const bool    storesValues            = writeDouble and field.components == written;
      if (not storesValues) {
        copyIntoScratch(field.values.data(), localEntities, field.components, field.components, written);
      }
      writeDataset(
        group + field.name,
        2,
        fieldDimensions,
        localFieldDimensions,
        fieldOffset,
        realStorageType,
        storesValues ? static_cast<const void*>(field.values.data()) : scratch.data()
      );
    }
  };
  writeFields(mesh.pointFields, "/data/", localNumberOfPoints, totalPoints, offsetPoints);
  writeFields(mesh.cellFields, "/cell_data/", localNumberOfCells, totalCells, offsetCells);

  H5Pclose(linkCreationProperties);
  if (dataTransferProperties != H5P_DEFAULT) {
    H5Pclose(dataTransferProperties);
  }
  check(H5Fclose(fileId));

  if (not rank.isGlobalMaster()) {
    return;
  }

  std::ostringstream grid;
  const std::string  h5FileName    = h5FilePath.filename().string();
  const char* const  realPrecision = writeDouble ? "8" : "4";
  grid << "\n"
       << "   <Topology TopologyType=\"Mixed\" NumberOfElements=\"" << totalCells << "\">\n"
       << "    <DataItem Format=\"HDF\" DataType=\"Int\" Dimensions=\"" << totalTopology << "\">\n"
       << "      " << h5FileName << ":/topology\n"
       << "    </DataItem>\n"
       << "   </Topology>\n"
       << "   <Geometry GeometryType=\"X_Y_Z\">\n";
  for (const char* const axis : {"x", "y", "z"}) {
    grid << "    <DataItem Format=\"HDF\" DataType=\"Float\" Precision=\"" << realPrecision << "\" Dimensions=\""
         << totalPoints << "\">\n"
         << "      " << h5FileName << ":/geometry/" << axis << "\n"
         << "    </DataItem>\n";
  }
  grid << "   </Geometry>\n";
  const auto writeAttributes = [&](
                                 const std::vector<Mesh::Field>& fields,
                                 const std::string&              centre,
                                 const std::string&              group,
                                 const unsigned long long        totalEntities
                               ) {
    for (const auto& field : fields) {
      const FieldLayout layout = classifyField(field.components);
      std::string       type   = "Matrix";
      switch (layout.kind) {
      case FieldKind::Scalar:
        type = "Scalar";
        break;
      case FieldKind::Vector:
        type = "Vector";
        break;
      case FieldKind::Tensor:
        type = "Tensor";
        break;
      case FieldKind::Generic:
        type = "Matrix";
        break;
      }
      grid << "   <Attribute Name=\"" << field.name << "\" AttributeType=\"" << type << "\" Center=\"" << centre
           << "\">\n"
           << "    <DataItem Format=\"HDF\" DataType=\"Float\" Precision=\"" << realPrecision << "\" Dimensions=\""
           << totalEntities << " " << layout.writtenComponents << "\">\n"
           << "      " << h5FileName << ":" << group << field.name << "\n"
           << "    </DataItem>\n"
           << "   </Attribute>\n";
    }
  };
  writeAttributes(mesh.pointFields, "Node", "/data/", totalPoints);
  writeAttributes(mesh.cellFields, "Cell", "/cell_data/", totalCells);
  grid << "  ";

  const std::filesystem::path xmfFilePath = directory / (snapshotName + ".xmf");
  std::ofstream               out(xmfFilePath);
  if (not out) {
    tarch::terminateWithError("The plotter cannot open the file '" + xmfFilePath.string() + "'.");
  }
  out << "<?xml version=\"1.0\" ?>\n"
      << "<!DOCTYPE Xdmf SYSTEM \"Xdmf.dtd\" []>\n"
      << "<Xdmf Version=\"2.0\">\n"
      << " <Domain>\n"
      << "  <Grid Name=\"PeanoGrid\" GridType=\"Uniform\">" << grid.str() << "</Grid>\n"
      << " </Domain>\n"
      << "</Xdmf>\n";
  out.close();
  if (not out) {
    tarch::terminateWithError("The plotter cannot write the file '" + xmfFilePath.string() + "'.");
  }

  const Snapshot snapshot{time, snapshotName + ".h5", grid.str()};
  // getSnapshotName() returns one name for time stamps that agree to six decimals. The series lists
  // that file once.
  if (not _snapshots.empty() and _snapshots.back().file == snapshot.file) {
    _snapshots.back() = snapshot;
  } else {
    _snapshots.push_back(snapshot);
  }
  writeSeries(directory / (filename + ".xmf"));
}

void
tarch::plotter::HDF5FileWriter::writeSeries(const std::filesystem::path& filePath) const {
  std::ofstream out(filePath);
  if (not out) {
    tarch::terminateWithError("The plotter cannot open the file '" + filePath.string() + "'.");
  }

  out << "<?xml version=\"1.0\" ?>\n"
      << "<!DOCTYPE Xdmf SYSTEM \"Xdmf.dtd\" []>\n"
      << "<Xdmf Version=\"2.0\">\n"
      << " <Domain>\n"
      << "  <Grid Name=\"TimeSeries\" GridType=\"Collection\" CollectionType=\"Temporal\">\n";
  for (const auto& snapshot : _snapshots) {
    const std::string timeStamp = getExactTimeStamp(snapshot.time);
    out << "   <Grid Name=\"Step_" << timeStamp << "\" GridType=\"Uniform\">\n"
        << "    <Time Value=\"" << timeStamp << "\"/>\n"
        << snapshot.grid << "   </Grid>\n";
  }
  out << "  </Grid>\n"
      << " </Domain>\n"
      << "</Xdmf>\n";
  out.close();
  if (not out) {
    tarch::terminateWithError("The plotter cannot write the file '" + filePath.string() + "'.");
  }
}

#endif
