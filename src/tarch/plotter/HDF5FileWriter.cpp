// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "HDF5FileWriter.h"

#ifdef PEANO_WITH_HDF5

#include "tarch/Termination.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/CellTypes.h"

#include <hdf5.h>

tarch::plotter::HDF5FileWriter::HDF5FileWriter(const std::string& outputDirectory) :
  tarch::plotter::FileWriter(outputDirectory) {}

int
tarch::plotter::HDF5FileWriter::toXdmfType(const int vtkType) {
  // Mapping based on XDMF Model and VTK File Formats; see CellTypes.h for the table.
  const CellTypeInfo* const info = lookupCellType(vtkType);
  return info != nullptr ? info->xdmf : 0; // 0 == unknown/unstructured
}

std::string
tarch::plotter::HDF5FileWriter::getFileExtension() const {
  return ".xmf";
}

bool
tarch::plotter::HDF5FileWriter::writeToFile(const std::string& filename, [[maybe_unused]] const double time) {
  if (_written) {
    throw std::runtime_error("Cannot write a plotter file more than once.");
  }

  createOutputDirectory();

  const std::filesystem::path outputDirectory(_outputDirectory);
  const std::filesystem::path h5FilePath  = outputDirectory / (filename + ".h5");
  const std::filesystem::path xmfFilePath = outputDirectory / (filename + ".xmf");

  const auto localData = getSubdomainBuffersSequenceForWriting();

  unsigned long long localNumberOfPoints = 0;
  unsigned long long localNumberOfCells  = 0;
  for (const SubdomainBuffers* const subdomain : localData.payloads) {
    localNumberOfPoints += subdomain->pointsXYZ.size() / 3;
    localNumberOfCells += subdomain->types.size();
  }

  // Number of entries of the XDMF mixed-topology stream.
  unsigned long long localTopologySize       = 0;
  const auto         computeXdmfTopologySize = [](const SubdomainBuffers& subdomain) {
    unsigned long long result            = 0;
    int                connectivityIndex = 0;
    for (std::size_t i = 0; i < subdomain.types.size(); i++) {
      const int vtkType       = subdomain.types[i];
      const int xdmfType      = toXdmfType(vtkType);
      const int numberOfNodes = subdomain.offsets[i] - connectivityIndex;
      connectivityIndex       = subdomain.offsets[i];

      result += 1; // The type id
      if (xdmfTypeNeedsNodeCount(xdmfType)) {
        result += 1; // The node count
      }
      result += numberOfNodes; // The node indices
    }
    return result;
  };

  for (const SubdomainBuffers* const subdomain : localData.payloads) {
    localTopologySize += computeXdmfTopologySize(*subdomain);
  }

  unsigned long long offsetPoints   = 0;
  unsigned long long offsetCells    = 0;
  unsigned long long offsetTopology = 0;

  unsigned long long totalPoints   = localNumberOfPoints;
  unsigned long long totalCells    = localNumberOfCells;
  unsigned long long totalTopology = localTopologySize;

  const auto& rank           = tarch::mpi::Rank::getInstance();
  MPI_Comm    comm           = rank.getCommunicator();
  bool        useParallelIO  = rank.getNumberOfRanks() > 1;
  bool        writesMetadata = rank.isGlobalMaster();

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
        _written = true;
        return true;
      }
      useParallelIO = false;
    } else {
      MPI_Comm_split(comm, hasData ? 0 : MPI_UNDEFINED, rank.getRank(), &ioComm);
      if (not hasData) {
        _written = true;
        return true; // The data-holding ranks write the file
      }
      comm       = ioComm;
      int ioRank = 0;
      MPI_Comm_rank(ioComm, &ioRank);
      writesMetadata = (ioRank == 0);
    }
  }

  if (useParallelIO) {
    const unsigned long long local[3]  = {localNumberOfPoints, localNumberOfCells, localTopologySize};
    unsigned long long       prefix[3] = {0ULL, 0ULL, 0ULL};
    unsigned long long       totals[3] = {0ULL, 0ULL, 0ULL};

    // Inclusive prefix sums over the three size fields; subtract local to get exclusive offsets.
    tarch::mpi::scan(local, prefix, 3, MPI_UNSIGNED_LONG_LONG, MPI_SUM, comm);
    offsetPoints   = prefix[0] - local[0];
    offsetCells    = prefix[1] - local[1];
    offsetTopology = prefix[2] - local[2];

    // Global file sizes (the metadata rank needs these for the XMF, everyone for the
    // HDF5 dataspaces).
    tarch::mpi::allreduce(local, totals, 3, MPI_UNSIGNED_LONG_LONG, MPI_SUM, comm);
    totalPoints   = totals[0];
    totalCells    = totals[1];
    totalTopology = totals[2];
  }

  hid_t fileAccessProperties   = H5P_DEFAULT;
  hid_t dataTransferProperties = H5P_DEFAULT;

  if (useParallelIO) {
    fileAccessProperties = H5Pcreate(H5P_FILE_ACCESS);

    MPI_Info   info  = MPI_INFO_NULL;
    const auto hints = getMPIFileHints();
    if (not hints.empty()) {
      MPI_Info_create(&info);
      for (const auto& hint : hints) {
        MPI_Info_set(info, hint.first.c_str(), hint.second.c_str());
      }
    }
    H5Pset_fapl_mpio(fileAccessProperties, comm, info);
    if (info != MPI_INFO_NULL) {
      MPI_Info_free(&info);
    }

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
    tarch::terminateWithError("Failed to create '" + h5FilePath.string() + "'.");
  }

  const hid_t linkCreationProperties = H5Pcreate(H5P_LINK_CREATE);
  H5Pset_create_intermediate_group(linkCreationProperties, 1);

  // Closes the file and the property lists when a dataset write throws before the
  // explicit closes at the end of this function. Disarmed on the success path.
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

    void release() { _armed = false; }

    ~HandleCloser() {
      if (_armed) {
        H5Pclose(_linkCreationProperties);
        if (_dataTransferProperties != H5P_DEFAULT) {
          H5Pclose(_dataTransferProperties);
        }
        H5Fclose(_fileId);
      }
    }

  private:
    const hid_t _fileId;
    const hid_t _linkCreationProperties;
    const hid_t _dataTransferProperties;
    bool        _armed = true;
  } handleCloser(fileId, linkCreationProperties, dataTransferProperties);

  const auto createDataset = [&](
                               const std::string&   name,
                               const int            numberOfDimensions,
                               const hsize_t* const globalDimensions,
                               const hid_t          type
                             ) {
    if (globalDimensions[0] == 0) {
      return static_cast<hid_t>(-1); // Empty dataset globally
    }

    const hid_t fileSpace = H5Screate_simple(numberOfDimensions, globalDimensions, nullptr);

    // The bulk datasets are chunked and deflated, and parallel HDF5 writes to a
    // filtered dataset collectively.
    const hid_t datasetCreationProperties = H5Pcreate(H5P_DATASET_CREATE);

    constexpr hsize_t ChunkByteTarget    = 1U << 20;
    const hsize_t     columns            = (numberOfDimensions > 1) ? globalDimensions[1] : 1;
    const hsize_t     bytesPerRow        = std::max<hsize_t>(1, columns * static_cast<hsize_t>(H5Tget_size(type)));
    const hsize_t     targetRows         = std::max<hsize_t>(1, ChunkByteTarget / bytesPerRow);
    const hsize_t     chunkDimensions[2] = {std::max<hsize_t>(1, std::min(globalDimensions[0], targetRows)), columns};
    H5Pset_chunk(datasetCreationProperties, numberOfDimensions, chunkDimensions);

    // The rank slabs tile the dataset exactly, so skip the default pre-write chunk fill.
    H5Pset_fill_time(datasetCreationProperties, H5D_FILL_TIME_NEVER);

    H5Pset_shuffle(datasetCreationProperties);
    H5Pset_deflate(datasetCreationProperties, 1);

    const hid_t dataset =
      H5Dcreate(fileId, name.c_str(), type, fileSpace, linkCreationProperties, datasetCreationProperties, H5P_DEFAULT);
    H5Pclose(datasetCreationProperties);
    H5Sclose(fileSpace);
    if (dataset < 0) {
      tarch::terminateWithError("Failed to create HDF5 dataset '" + name + "'.");
    }
    return dataset;
  };

  const auto writeDatasetChunk = [&](
                                   const hid_t          dataset,
                                   const int            numberOfDimensions,
                                   const hsize_t* const localDimensions,
                                   const hsize_t* const offsets,
                                   const void* const    data,
                                   const hid_t          type
                                 ) {
    if (dataset < 0) {
      return;
    }

    const hid_t fileSpace = H5Dget_space(dataset);
    if (localDimensions[0] > 0) {
      const hid_t  memorySpace = H5Screate_simple(numberOfDimensions, localDimensions, nullptr);
      const herr_t selectStatus =
        H5Sselect_hyperslab(fileSpace, H5S_SELECT_SET, offsets, nullptr, localDimensions, nullptr);
      const herr_t writeStatus =
        selectStatus >= 0 ? H5Dwrite(dataset, type, memorySpace, fileSpace, dataTransferProperties, data) : -1;
      H5Sclose(memorySpace);
      if (selectStatus < 0 or writeStatus < 0) {
        H5Sclose(fileSpace);
        tarch::terminateWithError("Failed to write an HDF5 dataset hyperslab.");
      }
    } else if (dataTransferProperties != H5P_DEFAULT) {
      // Empty rank: It joins the collective write with an empty selection, so every
      // rank issues the same number of collective writes.
      H5Sselect_none(fileSpace);
      if (H5Dwrite(dataset, type, H5S_ALL, fileSpace, dataTransferProperties, nullptr) < 0) {
        H5Sclose(fileSpace);
        tarch::terminateWithError("Failed to write an empty HDF5 dataset selection.");
      }
    }
    H5Sclose(fileSpace);
  };

  const auto closeDataset = [](const hid_t dataset) {
    if (dataset >= 0) {
      H5Dclose(dataset);
    }
  };

  // Append the subdomain's cells to the XDMF mixed-topology stream, rebasing
  // connectivity to global point indices.
  const auto appendXdmfTopology =
    [&](std::vector<std::int32_t>& topology, const SubdomainBuffers& subdomain, const unsigned long long pointBase) {
      std::int32_t connectivityIndex = 0;
      for (std::size_t i = 0; i < subdomain.types.size(); i++) {
        const int vtkType  = subdomain.types[i];
        const int xdmfType = toXdmfType(vtkType);

        const std::int32_t endIndex      = subdomain.offsets[i];
        const std::int32_t numberOfNodes = endIndex - connectivityIndex;

        topology.push_back(xdmfType);

        if (xdmfTypeNeedsNodeCount(xdmfType)) {
          topology.push_back(numberOfNodes);
        }

        for (int node = 0; node < numberOfNodes; node++) {
          const long long globalIndex = subdomain.connectivity[connectivityIndex++] + pointBase;
          // The /topology dataset is H5T_NATIVE_INT and XDMF readers expect that width, so a
          // mesh beyond the 32-bit range would wrap into a silently wrong connectivity.
          if (globalIndex > static_cast<long long>(std::numeric_limits<int>::max())) {
            tarch::terminateWithError("HDF5FileWriter cannot write node indices that exceed 32-bit integer range.");
          }
          topology.push_back(static_cast<std::int32_t>(globalIndex));
        }
      }
    };

  // The buffers accumulate in double precision; single-precision output narrows
  // them right before the write.
  const bool  writeDouble     = getDoublePrecisionOutput();
  const hid_t realStorageType = writeDouble ? H5T_NATIVE_DOUBLE : H5T_NATIVE_FLOAT;
  const auto  writeRealChunk  = [&](
                                  const hid_t                dataset,
                                  const int                  numberOfDimensions,
                                  const hsize_t* const       dimensions,
                                  const hsize_t* const       offset,
                                  const std::vector<double>& buffer
                                ) {
    if (writeDouble) {
      writeDatasetChunk(dataset, numberOfDimensions, dimensions, offset, buffer.data(), H5T_NATIVE_DOUBLE);
    } else {
      const std::vector<float> narrowed = narrowToFloat(buffer);
      writeDatasetChunk(dataset, numberOfDimensions, dimensions, offset, narrowed.data(), H5T_NATIVE_FLOAT);
    }
  };

  // The geometry goes into one dataset per axis, so each deflate stream holds one
  // coordinate range.
  {
    const hsize_t       coordinateDimensions[1] = {totalPoints};
    const hid_t         datasetX = createDataset("/geometry/x", 1, coordinateDimensions, realStorageType);
    const hid_t         datasetY = createDataset("/geometry/y", 1, coordinateDimensions, realStorageType);
    const hid_t         datasetZ = createDataset("/geometry/z", 1, coordinateDimensions, realStorageType);
    std::vector<double> coordinatesX, coordinatesY, coordinatesZ;
    coordinatesX.reserve(localNumberOfPoints);
    coordinatesY.reserve(localNumberOfPoints);
    coordinatesZ.reserve(localNumberOfPoints);
    for (const SubdomainBuffers* const subdomain : localData.payloads) {
      const auto& points = subdomain->pointsXYZ;
      for (std::size_t i = 0; i + 2 < points.size(); i += 3) {
        coordinatesX.push_back(points[i]);
        coordinatesY.push_back(points[i + 1]);
        coordinatesZ.push_back(points[i + 2]);
      }
    }
    const hsize_t localDimensions[1] = {localNumberOfPoints};
    const hsize_t offset[1]          = {offsetPoints};
    writeRealChunk(datasetX, 1, localDimensions, offset, coordinatesX);
    writeRealChunk(datasetY, 1, localDimensions, offset, coordinatesY);
    writeRealChunk(datasetZ, 1, localDimensions, offset, coordinatesZ);
    closeDataset(datasetX);
    closeDataset(datasetY);
    closeDataset(datasetZ);
  }

  const hsize_t globalTopologyDimensions[1] = {totalTopology};
  const hid_t   topologyDataset             = createDataset("/topology", 1, globalTopologyDimensions, H5T_NATIVE_INT);
  {
    std::vector<std::int32_t> mergedTopology;
    mergedTopology.reserve(localTopologySize);
    unsigned long long pointCursor = 0;
    for (const SubdomainBuffers* const subdomain : localData.payloads) {
      appendXdmfTopology(mergedTopology, *subdomain, offsetPoints + pointCursor);
      pointCursor += subdomain->pointsXYZ.size() / 3;
    }
    const hsize_t localTopologyDimensions[1] = {localTopologySize};
    const hsize_t topologyOffset[1]          = {offsetTopology};
    writeDatasetChunk(
      topologyDataset,
      1,
      localTopologyDimensions,
      topologyOffset,
      mergedTopology.data(),
      H5T_NATIVE_INT
    );
  }
  closeDataset(topologyDataset);

  // Every rank of the writing communicator holds data, so the schema comes from the
  // local reference.
  const FieldSchema             schema      = getFieldSchema(localData.reference);
  const std::vector<DataField>& pointFields = schema.pointFields;
  const std::vector<DataField>& cellFields  = schema.cellFields;

  // One merged collective write per point field. A vector is padded to 3 components
  // and a tensor to 9, as classifyField() prescribes.
  for (std::size_t fieldIndex = 0; fieldIndex < pointFields.size(); ++fieldIndex) {
    const auto&         field              = pointFields[fieldIndex];
    const FieldLayout   layout             = classifyField(field.components);
    const auto          written            = static_cast<hsize_t>(layout.writtenComponents);
    hsize_t             fieldDimensions[2] = {totalPoints, written};
    const hid_t         dataset            = createDataset("/data/" + field.name, 2, fieldDimensions, realStorageType);
    std::vector<double> mergedField;
    mergedField.reserve(localNumberOfPoints * static_cast<unsigned long long>(field.components));
    for (const SubdomainBuffers* const subdomain : localData.payloads) {
      const auto& values = subdomain->pointFields[fieldIndex].values;
      mergedField.insert(mergedField.end(), values.begin(), values.end());
    }
    if (layout.writtenComponents != field.components) {
      mergedField = expandFieldComponents(mergedField, field.components, layout.writtenComponents);
    }
    hsize_t localFieldDimensions[2] = {localNumberOfPoints, written};
    hsize_t fieldOffset[2]          = {offsetPoints, 0};
    writeRealChunk(dataset, 2, localFieldDimensions, fieldOffset, mergedField);
    closeDataset(dataset);
  }

  for (std::size_t fieldIndex = 0; fieldIndex < cellFields.size(); ++fieldIndex) {
    const auto&         field              = cellFields[fieldIndex];
    const FieldLayout   layout             = classifyField(field.components);
    const auto          written            = static_cast<hsize_t>(layout.writtenComponents);
    hsize_t             fieldDimensions[2] = {totalCells, written};
    const hid_t         dataset = createDataset("/cell_data/" + field.name, 2, fieldDimensions, realStorageType);
    std::vector<double> mergedField;
    mergedField.reserve(localNumberOfCells * static_cast<unsigned long long>(field.components));
    for (const SubdomainBuffers* const subdomain : localData.payloads) {
      const auto& values = subdomain->cellFields[fieldIndex].values;
      mergedField.insert(mergedField.end(), values.begin(), values.end());
    }
    if (layout.writtenComponents != field.components) {
      mergedField = expandFieldComponents(mergedField, field.components, layout.writtenComponents);
    }
    hsize_t localFieldDimensions[2] = {localNumberOfCells, written};
    hsize_t fieldOffset[2]          = {offsetCells, 0};
    writeRealChunk(dataset, 2, localFieldDimensions, fieldOffset, mergedField);
    closeDataset(dataset);
  }

  handleCloser.release();
  H5Pclose(linkCreationProperties);
  if (dataTransferProperties != H5P_DEFAULT) {
    H5Pclose(dataTransferProperties);
  }
  H5Fclose(fileId);

  if (ioComm != MPI_COMM_NULL) {
    MPI_Comm_free(&ioComm);
  }

  if (writesMetadata) {
    std::ofstream out(xmfFilePath);
    if (not out.is_open()) {
      tarch::terminateWithError("Failed to open '" + xmfFilePath.string() + "'.");
    } else {
      out << "<?xml version=\"1.0\" ?>\n"
          << "<!DOCTYPE Xdmf SYSTEM \"Xdmf.dtd\" []>\n"
          << "<Xdmf Version=\"2.0\">\n"
          << " <Domain>\n"
          << "  <Grid Name=\"PeanoGrid\" GridType=\"Uniform\">\n"
          << "   <Topology TopologyType=\"Mixed\" NumberOfElements=\"" << totalCells << "\">\n"
          << "    <DataItem Format=\"HDF\" DataType=\"Int\" Dimensions=\"" << totalTopology << "\">\n"
          << "      " << h5FilePath.filename().string() << ":/topology\n"
          << "    </DataItem>\n"
          << "   </Topology>\n"
          << "   <Geometry GeometryType=\"X_Y_Z\">\n";
      const char* const realPrecision = writeDouble ? "8" : "4";
      for (const char* const axis : {"x", "y", "z"}) {
        out << "    <DataItem Format=\"HDF\" DataType=\"Float\" Precision=\"" << realPrecision << "\" Dimensions=\""
            << totalPoints << "\">\n"
            << "      " << h5FilePath.filename().string() << ":/geometry/" << axis << "\n"
            << "    </DataItem>\n";
      }
      out << "   </Geometry>\n";

      const auto writeAttributes =
        [&](const std::vector<DataField>& fields, const std::string& centre, const std::string& path) {
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

            out << "   <Attribute Name=\"" << field.name << "\" AttributeType=\"" << type << "\" Center=\"" << centre
                << "\">\n"
                << "    <DataItem Format=\"HDF\" DataType=\"Float\" Precision=\"" << realPrecision << "\" Dimensions=\""
                << (centre == "Node" ? totalPoints : totalCells) << " " << layout.writtenComponents << "\">\n"
                << "      " << h5FilePath.filename().string() << ":" << path << field.name << "\n"
                << "    </DataItem>\n"
                << "   </Attribute>\n";
          }
        };

      writeAttributes(pointFields, "Node", "/data/");
      writeAttributes(cellFields, "Cell", "/cell_data/");

      out << "  </Grid>\n"
          << " </Domain>\n"
          << "</Xdmf>\n";
      if (not out) {
        tarch::terminateWithError("Failed to write '" + xmfFilePath.string() + "'.");
      }
    }
  }

  _written = true;
  return true;
}

#endif
