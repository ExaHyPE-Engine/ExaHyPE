// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "HDF5FileWriter.h"

#ifdef PEANO_WITH_HDF5

#include "tarch/Termination.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/CellTypes.h"

#include <hdf5.h>

tarch::plotter::HDF5FileWriter::HDF5FileWriter(const std::string& outputDirectory, const int numberOfSubdomains) :
  FileWriter(outputDirectory, numberOfSubdomains) {}

int tarch::plotter::HDF5FileWriter::toXdmfType(const int vtkType) {
  // Mapping based on XDMF Model and VTK File Formats; see CellTypes.h for the table.
  const CellTypeInfo* const info = lookupCellType(vtkType);
  return info != nullptr ? info->xdmf : 0; // 0 == unknown/unstructured
}

std::string tarch::plotter::HDF5FileWriter::getFileExtension() const { return ".xmf"; }

bool tarch::plotter::HDF5FileWriter::writeToFile(const std::string& filename, [[maybe_unused]] const double time) {
  if (_written) {
    throw std::runtime_error("Cannot write a plotter file more than once.");
  }

  createOutputDirectory();

  const std::filesystem::path outputDirectory(_outputDirectory);
  const std::filesystem::path h5FilePath  = outputDirectory / (filename + ".h5");
  const std::filesystem::path xmfFilePath = outputDirectory / (filename + ".xmf");

  const auto localData = getSubdomainBuffersSequenceForWriting();

  unsigned long long localNumPoints = 0;
  unsigned long long localNumCells  = 0;
  for (const SubdomainBuffers* const subdomain : localData.payloads) {
    localNumPoints += subdomain->pointsXYZ.size() / 3;
    localNumCells += subdomain->types.size();
  }

  // localTopoSize: entry count of the XDMF mixed-topology stream.
  unsigned long long localTopoSize           = 0;
  const auto         computeXdmfTopologySize = [](const SubdomainBuffers& subdomain) {
    unsigned long long result  = 0;
    int                connIdx = 0;
    for (std::size_t i = 0; i < subdomain.types.size(); i++) {
      const int vtkType  = subdomain.types[i];
      const int xdmfType = toXdmfType(vtkType);
      const int numNodes = subdomain.offsets[i] - connIdx;
      connIdx            = subdomain.offsets[i];

      result += 1; // the type ID
      if (xdmfTypeNeedsNodeCount(xdmfType)) {
        result += 1; // the count integer
      }
      result += numNodes; // the node indices
    }
    return result;
  };

  for (const SubdomainBuffers* const subdomain : localData.payloads) {
    localTopoSize += computeXdmfTopologySize(*subdomain);
  }

  unsigned long long offsetPoints = 0;
  unsigned long long offsetCells  = 0;
  unsigned long long offsetTopo   = 0;

  unsigned long long totalPoints = localNumPoints;
  unsigned long long totalCells  = localNumCells;
  unsigned long long totalTopo   = localTopoSize;

  const auto& rank           = tarch::mpi::Rank::getInstance();
  MPI_Comm    comm           = rank.getCommunicator();
  bool        useParallelIO  = rank.getNumberOfRanks() > 1;
  bool        writesMetadata = rank.isGlobalMaster();

  // Do the collective HDF5 I/O on a sub-communicator of only the ranks that hold data.
  // An empty rank that joins a collective write to a chunked + deflated (filtered) dataset
  // with a zero-sized selection deadlocks HDF5's parallel filtered-collective chunk
  // redistribution on some MPI-IO stacks (MPICH/ROMIO on a parallel filesystem). Dropping
  // the empty ranks keeps every participant's collective sequence identical, so the
  // compressed parallel write stays well-defined.
  MPI_Comm ioComm = MPI_COMM_NULL;
  if (useParallelIO) {
    const bool hasData    = (localData.reference != nullptr);
    const int  localHas   = hasData ? 1 : 0;
    int        anyHasData = 0;
    tarch::mpi::allreduce(&localHas, &anyHasData, 1, MPI_INT, MPI_MAX, comm);

    if (anyHasData == 0) {
      // Every rank is empty: the master alone writes a (serial) valid empty file.
      if (not rank.isGlobalMaster()) {
        _written = true;
        return true;
      }
      useParallelIO = false;
    } else {
      MPI_Comm_split(comm, hasData ? 0 : MPI_UNDEFINED, rank.getRank(), &ioComm);
      if (not hasData) {
        _written = true;
        return true; // nothing to contribute; the data-holding ranks write the file
      }
      comm       = ioComm;
      int ioRank = 0;
      MPI_Comm_rank(ioComm, &ioRank);
      writesMetadata = (ioRank == 0);
    }
  }

  if (useParallelIO) {
    const unsigned long long local[3]  = {localNumPoints, localNumCells, localTopoSize};
    unsigned long long       prefix[3] = {0ull, 0ull, 0ull};
    unsigned long long       totals[3] = {0ull, 0ull, 0ull};

    // Inclusive prefix sums over the three size fields; subtract local to get exclusive offsets.
    tarch::mpi::scan(local, prefix, 3, MPI_UNSIGNED_LONG_LONG, MPI_SUM, comm);
    offsetPoints = prefix[0] - local[0];
    offsetCells  = prefix[1] - local[1];
    offsetTopo   = prefix[2] - local[2];

    // Global file sizes (the metadata rank needs these for the XMF, everyone for the
    // HDF5 dataspaces).
    tarch::mpi::allreduce(local, totals, 3, MPI_UNSIGNED_LONG_LONG, MPI_SUM, comm);
    totalPoints = totals[0];
    totalCells  = totals[1];
    totalTopo   = totals[2];
  }

  hid_t fapl = H5P_DEFAULT;
  hid_t dxpl = H5P_DEFAULT; // Data transfer property list

  if (useParallelIO) {
    // Property list for parallel file access
    fapl = H5Pcreate(H5P_FILE_ACCESS);

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

    // The file keeps the default alignment and metadata block size. Raising either
    // reserves roughly 1 MiB of mostly empty space per file for deflate-compressed
    // chunks.
#if H5_VERSION_GE(1, 10, 0)
    // Handle metadata collectively instead of per-rank small I/O.
    H5Pset_all_coll_metadata_ops(fapl, 1);
    H5Pset_coll_metadata_write(fapl, 1);
#endif

    // Collective transfer: one balanced write per dataset per rank, which lets ROMIO
    // aggregate the per-rank slabs into large file writes.
    dxpl = H5Pcreate(H5P_DATASET_XFER);
    H5Pset_dxpl_mpio(dxpl, H5FD_MPIO_COLLECTIVE);
  }

  const hid_t fileId = H5Fcreate(h5FilePath.string().c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, fapl);

  if (fapl != H5P_DEFAULT) {
    H5Pclose(fapl);
  }

  if (fileId < 0) {
    tarch::terminateWithError("Failed to create '" + h5FilePath.string() + "'.");
  }

  const hid_t lcpl = H5Pcreate(H5P_LINK_CREATE);
  H5Pset_create_intermediate_group(lcpl, 1);

  // Close the file and property lists if a dataset write throws before the explicit
  // closes below. Disarmed on the success path.
  class HandleCloser {
  public:
    HandleCloser(const hid_t fileId, const hid_t lcpl, const hid_t dxpl) :
      _fileId(fileId),
      _lcpl(lcpl),
      _dxpl(dxpl) {}
    HandleCloser(const HandleCloser&) = delete;
    HandleCloser(HandleCloser&&)      = delete;
    ~HandleCloser() {
      if (_armed) {
        H5Pclose(_lcpl);
        if (_dxpl != H5P_DEFAULT) {
          H5Pclose(_dxpl);
        }
        H5Fclose(_fileId);
      }
    }
    HandleCloser& operator=(const HandleCloser&) = delete;
    HandleCloser& operator=(HandleCloser&&)      = delete;

    void release() { _armed = false; }

  private:
    const hid_t _fileId;
    const hid_t _lcpl;
    const hid_t _dxpl;
    bool        _armed = true;
  } handleCloser(fileId, lcpl, dxpl);

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

    // Chunk + deflate the bulk datasets; readers decompress transparently. Parallel HDF5
    // needs collective writes to filtered datasets, which these already are.
    const hid_t datasetCreationProperties = H5Pcreate(H5P_DATASET_CREATE);

    // ~1 MiB chunk byte target. cols uses the 1D-safe form (globalDims[1] is only valid
    // when dims > 1).
    constexpr hsize_t ChunkByteTarget    = 1u << 20;
    const hsize_t     columns            = (numberOfDimensions > 1) ? globalDimensions[1] : 1;
    const hsize_t     bytesPerRow        = std::max<hsize_t>(1, columns * static_cast<hsize_t>(H5Tget_size(type)));
    const hsize_t     targetRows         = std::max<hsize_t>(1, ChunkByteTarget / bytesPerRow);
    const hsize_t     chunkDimensions[2] = {std::max<hsize_t>(1, std::min(globalDimensions[0], targetRows)), columns};
    H5Pset_chunk(datasetCreationProperties, numberOfDimensions, chunkDimensions);

    // The rank slabs tile the dataset exactly, so skip the default pre-write chunk fill.
    H5Pset_fill_time(datasetCreationProperties, H5D_FILL_TIME_NEVER);

    // Byte-shuffle before deflate improves the ratio of the homogeneous data; reversed on read.
    H5Pset_shuffle(datasetCreationProperties);
    H5Pset_deflate(datasetCreationProperties, 1);

    const hid_t dataset =
      H5Dcreate(fileId, name.c_str(), type, fileSpace, lcpl, datasetCreationProperties, H5P_DEFAULT);
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
      const herr_t writeStatus = selectStatus >= 0 ? H5Dwrite(dataset, type, memorySpace, fileSpace, dxpl, data) : -1;
      H5Sclose(memorySpace);
      if (selectStatus < 0 or writeStatus < 0) {
        H5Sclose(fileSpace);
        tarch::terminateWithError("Failed to write an HDF5 dataset hyperslab.");
      }
    } else if (dxpl != H5P_DEFAULT) {
      // Empty rank: join the collective write with an empty selection so all ranks
      // issue the same number of collective writes.
      H5Sselect_none(fileSpace);
      if (H5Dwrite(dataset, type, H5S_ALL, fileSpace, dxpl, nullptr) < 0) {
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
      std::int32_t connIdx = 0;
      for (std::size_t i = 0; i < subdomain.types.size(); i++) {
        const int vtkType  = subdomain.types[i];
        const int xdmfType = toXdmfType(vtkType);

        const std::int32_t endIdx   = subdomain.offsets[i];
        const std::int32_t numNodes = endIdx - connIdx;

        topology.push_back(xdmfType);

        if (xdmfTypeNeedsNodeCount(xdmfType)) {
          topology.push_back(numNodes);
        }

        for (int k = 0; k < numNodes; k++) {
          const long long globalIndex = subdomain.connectivity[connIdx++] + pointBase;
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

  // Geometry as separate x/y/z arrays. An interleaved {N,3} array would mix three
  // different coordinate ranges into one deflate stream, which the compressor cannot
  // match well.
  {
    const hsize_t       coordinateDimensions[1] = {totalPoints};
    const hid_t         datasetX = createDataset("/geometry/x", 1, coordinateDimensions, realStorageType);
    const hid_t         datasetY = createDataset("/geometry/y", 1, coordinateDimensions, realStorageType);
    const hid_t         datasetZ = createDataset("/geometry/z", 1, coordinateDimensions, realStorageType);
    std::vector<double> cx, cy, cz;
    cx.reserve(localNumPoints);
    cy.reserve(localNumPoints);
    cz.reserve(localNumPoints);
    for (const SubdomainBuffers* const subdomain : localData.payloads) {
      const auto& p = subdomain->pointsXYZ;
      for (std::size_t i = 0; i + 2 < p.size(); i += 3) {
        cx.push_back(p[i]);
        cy.push_back(p[i + 1]);
        cz.push_back(p[i + 2]);
      }
    }
    const hsize_t localDimensions[1] = {localNumPoints};
    const hsize_t offset[1]          = {offsetPoints};
    writeRealChunk(datasetX, 1, localDimensions, offset, cx);
    writeRealChunk(datasetY, 1, localDimensions, offset, cy);
    writeRealChunk(datasetZ, 1, localDimensions, offset, cz);
    closeDataset(datasetX);
    closeDataset(datasetY);
    closeDataset(datasetZ);
  }

  // Write the topology (XDMF mixed stream), merged with connectivity rebased to
  // global point indices.
  const hsize_t globalTopologyDimensions[1] = {totalTopo};
  const hid_t   topologyDataset             = createDataset("/topology", 1, globalTopologyDimensions, H5T_NATIVE_INT);
  {
    std::vector<std::int32_t> mergedTopo;
    mergedTopo.reserve(localTopoSize);
    unsigned long long pointCursor = 0;
    for (const SubdomainBuffers* const subdomain : localData.payloads) {
      appendXdmfTopology(mergedTopo, *subdomain, offsetPoints + pointCursor);
      pointCursor += subdomain->pointsXYZ.size() / 3;
    }
    const hsize_t localTopologyDimensions[1] = {localTopoSize};
    const hsize_t topologyOffset[1]          = {offsetTopo};
    writeDatasetChunk(topologyDataset, 1, localTopologyDimensions, topologyOffset, mergedTopo.data(), H5T_NATIVE_INT);
  }
  closeDataset(topologyDataset);

  // Empty ranks were dropped from the writing communicator, so every rank here holds
  // data and the schema comes from the local reference (a broadcast would deadlock).
  const FieldSchema             schema      = fieldSchemaFromReference(localData.reference);
  const std::vector<DataField>& pointFields = schema.pointFields;
  const std::vector<DataField>& cellFields  = schema.cellFields;

  // Write point data fields (one merged collective write per field). Vectors are
  // padded to 3 components and tensors to 9 so readers interpret them as vectors
  // and tensors; see FileWriter::classifyField.
  for (std::size_t fieldIndex = 0; fieldIndex < pointFields.size(); ++fieldIndex) {
    const auto&         field    = pointFields[fieldIndex];
    const FieldLayout   layout   = classifyField(field.components);
    const auto          written  = static_cast<hsize_t>(layout.writtenComponents);
    hsize_t             fDims[2] = {totalPoints, written};
    const hid_t         dataset  = createDataset("/data/" + field.name, 2, fDims, realStorageType);
    std::vector<double> mergedField;
    mergedField.reserve(localNumPoints * static_cast<unsigned long long>(field.components));
    for (const SubdomainBuffers* const subdomain : localData.payloads) {
      const auto& values = subdomain->pointFields[fieldIndex].values;
      mergedField.insert(mergedField.end(), values.begin(), values.end());
    }
    // Only vectors/tensors are widened; scalars skip the copy.
    if (layout.writtenComponents != field.components) {
      mergedField = expandFieldComponents(mergedField, field.components, layout.writtenComponents);
    }
    hsize_t flDims[2] = {localNumPoints, written};
    hsize_t fOff[2]   = {offsetPoints, 0};
    writeRealChunk(dataset, 2, flDims, fOff, mergedField);
    closeDataset(dataset);
  }

  // Write cell data fields (one merged collective write per field).
  for (std::size_t fieldIndex = 0; fieldIndex < cellFields.size(); ++fieldIndex) {
    const auto&         field    = cellFields[fieldIndex];
    const FieldLayout   layout   = classifyField(field.components);
    const auto          written  = static_cast<hsize_t>(layout.writtenComponents);
    hsize_t             fDims[2] = {totalCells, written};
    const hid_t         dataset  = createDataset("/cell_data/" + field.name, 2, fDims, realStorageType);
    std::vector<double> mergedField;
    mergedField.reserve(localNumCells * static_cast<unsigned long long>(field.components));
    for (const SubdomainBuffers* const subdomain : localData.payloads) {
      const auto& values = subdomain->cellFields[fieldIndex].values;
      mergedField.insert(mergedField.end(), values.begin(), values.end());
    }
    if (layout.writtenComponents != field.components) {
      mergedField = expandFieldComponents(mergedField, field.components, layout.writtenComponents);
    }
    hsize_t flDims[2] = {localNumCells, written};
    hsize_t fOff[2]   = {offsetCells, 0};
    writeRealChunk(dataset, 2, flDims, fOff, mergedField);
    closeDataset(dataset);
  }

  handleCloser.release();
  H5Pclose(lcpl);
  if (dxpl != H5P_DEFAULT) {
    H5Pclose(dxpl);
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
          << "    <DataItem Format=\"HDF\" DataType=\"Int\" Dimensions=\"" << totalTopo << "\">\n"
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

      // Emit one XDMF Attribute per field.
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
