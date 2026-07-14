// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "HDF5FileWriter.h"

#ifdef PEANO_WITH_HDF5

#include "CellTypes.h"

#include "tarch/mpi/Rank.h"
#include "tarch/mpi/mpi.h"

#include <hdf5.h>

namespace fs = std::filesystem;

tarch::plotter::HDF5FileWriter::HDF5FileWriter(const std::string& outputDirectory, int numberOfSubdomains) :
  FileWriter(outputDirectory, numberOfSubdomains) {}

int tarch::plotter::HDF5FileWriter::toXdmfType(int vtkType) {
  // Mapping based on XDMF Model and VTK File Formats; see CellTypes.h for the table.
  const CellTypeInfo* info = lookupCellType(vtkType);
  return info != nullptr ? info->xdmf : 0; // 0 == unknown/unstructured
}

std::string tarch::plotter::HDF5FileWriter::getFileExtension() const { return ".xmf"; }

bool tarch::plotter::HDF5FileWriter::writeToFile(const std::string& filename, double time) {
  if (_written) {
    throw std::runtime_error("Cannot write to file. File already written.");
  }

  createOutputDirectory();

  fs::path outDir(_outputDirectory);
  fs::path h5FilePath  = outDir / (filename + ".h5");
  fs::path xmfFilePath = outDir / (filename + ".xmf");

  const auto localData = getSubdomainBuffersSequenceForWriting();

  unsigned long long localNumPoints = 0;
  unsigned long long localNumCells  = 0;
  for (const SubdomainBuffers* subdomain : localData.payloads) {
    localNumPoints += subdomain->pointsXYZ.size() / 3;
    localNumCells += subdomain->types.size();
  }

  // localTopoSize: entry count of the XDMF mixed-topology stream.
  unsigned long long localTopoSize           = 0;
  auto               computeXdmfTopologySize = [&](const SubdomainBuffers& subdomain) {
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

  for (const SubdomainBuffers* subdomain : localData.payloads) {
    localTopoSize += computeXdmfTopologySize(*subdomain);
  }

  unsigned long long offsetPoints = 0;
  unsigned long long offsetCells  = 0;
  unsigned long long offsetTopo   = 0;

  unsigned long long totalPoints = localNumPoints;
  unsigned long long totalCells  = localNumCells;
  unsigned long long totalTopo   = localTopoSize;

#if defined(PEANO_WITH_MPI)
  auto&    rank           = tarch::mpi::Rank::getInstance();
  MPI_Comm comm           = rank.getCommunicator();
  bool     useParallelIO  = rank.getNumberOfRanks() > 1;
  bool     writesMetadata = rank.isGlobalMaster();

  // Do the collective HDF5 I/O on a sub-communicator of only the ranks that hold data.
  // An empty rank that joins a collective write to a chunked + deflated (filtered) dataset
  // with a zero-sized selection deadlocks HDF5's parallel filtered-collective chunk
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
    unsigned long long local[3]  = {localNumPoints, localNumCells, localTopoSize};
    unsigned long long prefix[3] = {0ull, 0ull, 0ull};
    unsigned long long totals[3] = {0ull, 0ull, 0ull};

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
#else
  bool useParallelIO  = false;
  bool writesMetadata = true;
#endif

  hid_t fapl = H5P_DEFAULT;
  hid_t dxpl = H5P_DEFAULT; // Data transfer property list

#if defined(PEANO_WITH_MPI)
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

    // No H5Pset_alignment / H5Pset_meta_block_size: they barely help deflate-compressed
    // chunks but reserve ~1 MiB of mostly-empty space per file. Defaults keep files small.
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
#endif

  hid_t fileId = H5Fcreate(h5FilePath.string().c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, fapl);

  if (fapl != H5P_DEFAULT) {
    H5Pclose(fapl);
  }

  if (fileId < 0) {
    throw std::runtime_error("Failed to create HDF5 file: " + h5FilePath.string());
  }

  hid_t lcpl = H5Pcreate(H5P_LINK_CREATE);
  H5Pset_create_intermediate_group(lcpl, 1);

  // Close the file and property lists if a dataset write throws before the explicit
  // closes below. Disarmed on the success path.
  struct HandleCloser {
    hid_t fileId;
    hid_t lcpl;
    hid_t dxpl;
    bool  armed = true;
    ~HandleCloser() {
      if (armed) {
        H5Pclose(lcpl);
        if (dxpl != H5P_DEFAULT) {
          H5Pclose(dxpl);
        }
        H5Fclose(fileId);
      }
    }
  } handleCloser{fileId, lcpl, dxpl};

  auto createDataset = [&](const std::string& name, int dims, hsize_t* globalDims, hid_t type) {
    if (globalDims[0] == 0) {
      return static_cast<hid_t>(-1); // Empty dataset globally
    }

    hid_t filespace = H5Screate_simple(dims, globalDims, NULL);

    // Chunk + deflate the bulk datasets; readers decompress transparently. Parallel HDF5
    // needs collective writes to filtered datasets, which these already are.
    hid_t dcpl = H5Pcreate(H5P_DATASET_CREATE);

    // ~1 MiB chunk byte target. cols uses the 1D-safe form (globalDims[1] is only valid
    // when dims > 1).
    constexpr hsize_t ChunkByteTarget = 1u << 20;
    const hsize_t     cols            = (dims > 1) ? globalDims[1] : 1;
    const hsize_t     bytesPerRow     = std::max<hsize_t>(1, cols * static_cast<hsize_t>(H5Tget_size(type)));
    const hsize_t     targetRows      = std::max<hsize_t>(1, ChunkByteTarget / bytesPerRow);
    hsize_t           chunk[2]        = {std::max<hsize_t>(1, std::min(globalDims[0], targetRows)), cols};
    H5Pset_chunk(dcpl, dims, chunk);

    // The rank slabs tile the dataset exactly, so skip the default pre-write chunk fill.
    H5Pset_fill_time(dcpl, H5D_FILL_TIME_NEVER);

    // Byte-shuffle before deflate improves the ratio of the homogeneous data; reversed on read.
    H5Pset_shuffle(dcpl);
    H5Pset_deflate(dcpl, 1);

    hid_t dataset = H5Dcreate(fileId, name.c_str(), type, filespace, lcpl, dcpl, H5P_DEFAULT);
    H5Pclose(dcpl);
    H5Sclose(filespace);
    return dataset;
  };

  auto writeDatasetChunk =
    [&](hid_t dataset, int dims, hsize_t* localDims, hsize_t* offsets, const void* data, hid_t type) {
    if (dataset < 0) {
      return;
    }

    hid_t filespace = H5Dget_space(dataset);
    if (localDims[0] > 0) {
      hid_t  memspace     = H5Screate_simple(dims, localDims, NULL);
      herr_t selectStatus = H5Sselect_hyperslab(filespace, H5S_SELECT_SET, offsets, NULL, localDims, NULL);
      herr_t writeStatus  = selectStatus >= 0 ? H5Dwrite(dataset, type, memspace, filespace, dxpl, data) : -1;
      H5Sclose(memspace);
      if (selectStatus < 0 or writeStatus < 0) {
        H5Sclose(filespace);
        throw std::runtime_error("HDF5 hyperslab dataset write failed.");
      }
    } else if (dxpl != H5P_DEFAULT) {
      // Empty rank: join the collective write with an empty selection so all ranks
      // issue the same number of collective writes.
      H5Sselect_none(filespace);
      if (H5Dwrite(dataset, type, H5S_ALL, filespace, dxpl, NULL) < 0) {
        H5Sclose(filespace);
        throw std::runtime_error("HDF5 empty-selection collective dataset write failed.");
      }
    }
    H5Sclose(filespace);
  };

  auto closeDataset = [](hid_t dataset) {
    if (dataset >= 0) {
      H5Dclose(dataset);
    }
  };

  // Append the subdomain's cells to the XDMF mixed-topology stream, rebasing
  // connectivity to global point indices.
  auto appendXdmfTopology =
    [&](std::vector<std::int32_t>& topology, const SubdomainBuffers& subdomain, unsigned long long pointBase) {
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
        topology.push_back(static_cast<std::int32_t>(globalIndex));
      }
    }
  };

  // Geometry as separate x/y/z arrays. Per-component layout compresses much better than
  // an interleaved {N,3} array, whose three coordinate ranges defeat the deflate matching.
  {
    hsize_t            cDims[1] = {totalPoints};
    hid_t              dsX      = createDataset("/geometry/x", 1, cDims, H5T_NATIVE_FLOAT);
    hid_t              dsY      = createDataset("/geometry/y", 1, cDims, H5T_NATIVE_FLOAT);
    hid_t              dsZ      = createDataset("/geometry/z", 1, cDims, H5T_NATIVE_FLOAT);
    std::vector<float> cx, cy, cz;
    cx.reserve(localNumPoints);
    cy.reserve(localNumPoints);
    cz.reserve(localNumPoints);
    for (const SubdomainBuffers* subdomain : localData.payloads) {
      const auto& p = subdomain->pointsXYZ;
      for (std::size_t i = 0; i + 2 < p.size(); i += 3) {
        cx.push_back(p[i]);
        cy.push_back(p[i + 1]);
        cz.push_back(p[i + 2]);
      }
    }
    hsize_t lDims[1] = {localNumPoints};
    hsize_t off[1]   = {offsetPoints};
    writeDatasetChunk(dsX, 1, lDims, off, cx.data(), H5T_NATIVE_FLOAT);
    writeDatasetChunk(dsY, 1, lDims, off, cy.data(), H5T_NATIVE_FLOAT);
    writeDatasetChunk(dsZ, 1, lDims, off, cz.data(), H5T_NATIVE_FLOAT);
    closeDataset(dsX);
    closeDataset(dsY);
    closeDataset(dsZ);
  }

  // Write the topology (XDMF mixed stream), merged with connectivity rebased to
  // global point indices.
  hsize_t gTDims[1]       = {totalTopo};
  hid_t   topologyDataset = createDataset("/topology", 1, gTDims, H5T_NATIVE_INT);
  {
    std::vector<std::int32_t> mergedTopo;
    mergedTopo.reserve(localTopoSize);
    unsigned long long pointCursor = 0;
    for (const SubdomainBuffers* subdomain : localData.payloads) {
      appendXdmfTopology(mergedTopo, *subdomain, offsetPoints + pointCursor);
      pointCursor += subdomain->pointsXYZ.size() / 3;
    }
    hsize_t lTDims[1] = {localTopoSize};
    hsize_t offT[1]   = {offsetTopo};
    writeDatasetChunk(topologyDataset, 1, lTDims, offT, mergedTopo.data(), H5T_NATIVE_INT);
  }
  closeDataset(topologyDataset);

  // Empty ranks were dropped from the writing communicator, so every rank here holds
  // data and the schema comes from the local reference (a broadcast would deadlock).
  const FieldSchema             schema      = fieldSchemaFromReference(localData.reference);
  const std::vector<DataField>& pointFields = schema.pointFields;
  const std::vector<DataField>& cellFields  = schema.cellFields;

  // Write point data fields (one merged collective write per field). Vectors are
  // padded to 3 components and tensors to 9 so they read back as real vectors /
  // tensors; see FileWriter::classifyField.
  for (std::size_t fieldIndex = 0; fieldIndex < pointFields.size(); ++fieldIndex) {
    const auto&        field    = pointFields[fieldIndex];
    const FieldLayout  layout   = classifyField(field.components);
    const auto         written  = static_cast<hsize_t>(layout.writtenComponents);
    hsize_t            fDims[2] = {totalPoints, written};
    hid_t              dataset  = createDataset("/data/" + field.name, 2, fDims, H5T_NATIVE_FLOAT);
    std::vector<float> mergedField;
    mergedField.reserve(localNumPoints * static_cast<unsigned long long>(field.components));
    for (const SubdomainBuffers* subdomain : localData.payloads) {
      const auto& values = subdomain->pointFields[fieldIndex].values;
      mergedField.insert(mergedField.end(), values.begin(), values.end());
    }
    // Only vectors/tensors are widened; scalars skip the copy.
    if (layout.writtenComponents != field.components) {
      mergedField = expandFieldComponents(mergedField, field.components, layout.writtenComponents);
    }
    hsize_t flDims[2] = {localNumPoints, written};
    hsize_t fOff[2]   = {offsetPoints, 0};
    writeDatasetChunk(dataset, 2, flDims, fOff, mergedField.data(), H5T_NATIVE_FLOAT);
    closeDataset(dataset);
  }

  // Write cell data fields (one merged collective write per field).
  for (std::size_t fieldIndex = 0; fieldIndex < cellFields.size(); ++fieldIndex) {
    const auto&        field    = cellFields[fieldIndex];
    const FieldLayout  layout   = classifyField(field.components);
    const auto         written  = static_cast<hsize_t>(layout.writtenComponents);
    hsize_t            fDims[2] = {totalCells, written};
    hid_t              dataset  = createDataset("/cell_data/" + field.name, 2, fDims, H5T_NATIVE_FLOAT);
    std::vector<float> mergedField;
    mergedField.reserve(localNumCells * static_cast<unsigned long long>(field.components));
    for (const SubdomainBuffers* subdomain : localData.payloads) {
      const auto& values = subdomain->cellFields[fieldIndex].values;
      mergedField.insert(mergedField.end(), values.begin(), values.end());
    }
    if (layout.writtenComponents != field.components) {
      mergedField = expandFieldComponents(mergedField, field.components, layout.writtenComponents);
    }
    hsize_t flDims[2] = {localNumCells, written};
    hsize_t fOff[2]   = {offsetCells, 0};
    writeDatasetChunk(dataset, 2, flDims, fOff, mergedField.data(), H5T_NATIVE_FLOAT);
    closeDataset(dataset);
  }

  handleCloser.armed = false;
  H5Pclose(lcpl);
  if (dxpl != H5P_DEFAULT) {
    H5Pclose(dxpl);
  }
  H5Fclose(fileId);

#if defined(PEANO_WITH_MPI)
  if (ioComm != MPI_COMM_NULL) {
    MPI_Comm_free(&ioComm);
  }
#endif

  if (writesMetadata) {
    std::ofstream out(xmfFilePath);
    if (not out.is_open()) {
      throw std::runtime_error("Could not write XDMF file " + xmfFilePath.string());
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
      for (const char* axis : {"x", "y", "z"}) {
        out << "    <DataItem Format=\"HDF\" DataType=\"Float\" Precision=\"4\" Dimensions=\"" << totalPoints << "\">\n"
            << "      " << h5FilePath.filename().string() << ":/geometry/" << axis << "\n"
            << "    </DataItem>\n";
      }
      out << "   </Geometry>\n";

      // Emit one XDMF Attribute per field.
      auto writeAttrs = [&](const std::vector<DataField>& fields, const std::string& center, const std::string& path) {
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

          out << "   <Attribute Name=\"" << field.name << "\" AttributeType=\"" << type << "\" Center=\"" << center
              << "\">\n"
              << "    <DataItem Format=\"HDF\" DataType=\"Float\" Precision=\"4\" Dimensions=\""
              << (center == "Node" ? totalPoints : totalCells) << " " << layout.writtenComponents << "\">\n"
              << "      " << h5FilePath.filename().string() << ":" << path << field.name << "\n"
              << "    </DataItem>\n"
              << "   </Attribute>\n";
        }
      };

      writeAttrs(pointFields, "Node", "/data/");
      writeAttrs(cellFields, "Cell", "/cell_data/");

      out << "  </Grid>\n"
          << " </Domain>\n"
          << "</Xdmf>\n";
    }
  }

  _written = true;
  return true;
}

#endif
