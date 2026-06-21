// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "HDF5FileWriter.h"

#ifdef PEANO_WITH_HDF5

#include "tarch/mpi/Rank.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <hdf5.h>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace fs = std::filesystem;

tarch::plotter::HDF5FileWriter::HDF5FileWriter(const std::string& outputDirectory, int numberOfSubdomains):
  FileWriter(outputDirectory, numberOfSubdomains) {}

int tarch::plotter::HDF5FileWriter::toXdmfType(int vtkType) {
  // Mapping based on XDMF Model and VTK File Formats
  switch (vtkType) {
  case 1:
    return 1; // Polyvertex
  case 3:
    return 2; // Polyline
  case 5:
    return 4; // Triangle
  case 9:
    return 5; // Quadrilateral
  case 10:
    return 6; // Tetrahedron
  case 12:
    return 9; // Hexahedron
  case 13:
    return 8; // Wedge
  case 14:
    return 7; // Pyramid
  default:
    return 0; // Unknown/Unstructured
  }
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

  SubdomainBuffers localData = mergeSubdomains();

  const unsigned long long localNumPoints = localData.pointsXYZ.size() / 3;
  const unsigned long long localNumCells  = localData.types.size();

  // Compute localTopoSize for XDMF Mixed format
  unsigned long long localTopoSize = 0;
  {
    int connIdx = 0;
    for (size_t i = 0; i < localData.types.size(); i++) {
      const int vtkType  = localData.types[i];
      const int xdmfType = toXdmfType(vtkType);
      const int numNodes = localData.offsets[i] - connIdx;
      connIdx            = localData.offsets[i];

      localTopoSize += 1; // The Type ID
      if (xdmfType >= 1 and xdmfType <= 3) {
        localTopoSize += 1; // The Count integer
      }
      localTopoSize += numNodes; // The Node Indices
    }
  }

  unsigned long long offsetPoints = 0;
  unsigned long long offsetCells  = 0;
  unsigned long long offsetTopo   = 0;

  unsigned long long totalPoints = localNumPoints;
  unsigned long long totalCells  = localNumCells;
  unsigned long long totalTopo   = localTopoSize;

#if defined(PEANO_WITH_MPI)
  auto&    rank          = tarch::mpi::Rank::getInstance();
  MPI_Comm comm          = rank.getCommunicator();
  bool     useParallelIO = rank.getNumberOfRanks() > 1;

  if (useParallelIO) {
    unsigned long long local[3]  = {localNumPoints, localNumCells, localTopoSize};
    unsigned long long prefix[3] = {0ull, 0ull, 0ull};
    unsigned long long totals[3] = {0ull, 0ull, 0ull};

    // Inclusive prefix sums; subtract local to get exclusive offsets.
    MPI_Scan(local, prefix, 3, MPI_UNSIGNED_LONG_LONG, MPI_SUM, comm);
    offsetPoints = prefix[0] - local[0];
    offsetCells  = prefix[1] - local[1];
    offsetTopo   = prefix[2] - local[2];

    // TODO: Use tarch MPI routines
    // Use Allreduce to get global file sizes (Rank 0 needs this for XMF, everyone for HDF5 dataspace)
    MPI_Allreduce(local, totals, 3, MPI_UNSIGNED_LONG_LONG, MPI_SUM, comm);
    totalPoints = totals[0];
    totalCells  = totals[1];
    totalTopo   = totals[2];
  }
#else
  bool useParallelIO = false;
#endif

  // Prepare Topology (Convert VTK-style to XDMF Mixed format)
  // VTK: [Type, Type] + [Conn, Conn] + [Offset, Offset]
  // XDMF Mixed: Single stream [Type1, Node0, Node1... Type2, Node0...]
  std::vector<std::int32_t> localXdmfTopology;
  // Reservation estimation: connectivity size + number of cells
  localXdmfTopology.reserve(localData.connectivity.size() + localData.types.size());

  std::int32_t connIdx = 0;
  for (std::size_t i = 0; i < localData.types.size(); i++) {
    const int vtkType  = localData.types[i];
    const int xdmfType = toXdmfType(vtkType);

    // Calculate number of nodes for this cell
    // The offsets array in SubdomainBuffers contains the end index of connectivity for cell i
    const std::int32_t endIdx   = localData.offsets[i];
    const std::int32_t numNodes = endIdx - connIdx;

    localXdmfTopology.push_back(xdmfType);

    // Insert Count for variable size types
    if (xdmfType >= 1 and xdmfType <= 3) {
      localXdmfTopology.push_back(numNodes);
    }

    for (int k = 0; k < numNodes; k++) {
      const long long globalIndex = localData.connectivity[connIdx++] + offsetPoints;
      localXdmfTopology.push_back(static_cast<std::int32_t>(globalIndex));
    }
  }

  hid_t fapl = H5P_DEFAULT;
  hid_t dxpl = H5P_DEFAULT; // Data transfer property list

#if defined(PEANO_WITH_MPI)
  if (useParallelIO) {
    // Property list for parallel file access
    fapl = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_fapl_mpio(fapl, comm, MPI_INFO_NULL);

    // Data transfer property list for collective MPI-I/O.
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

  // Helper lambda to write a dataset
  auto writeDataset =
    [&](
      const std::string& name,
      int                dims,
      hsize_t*           globalDims,
      hsize_t*           localDims,
      hsize_t*           offsets,
      const void*        data,
      hid_t              type
    ) {
      if (globalDims[0] == 0) {
        return; // Empty dataset globally
      }

      hid_t filespace = H5Screate_simple(dims, globalDims, NULL);
      hid_t dataset   = H5Dcreate(fileId, name.c_str(), type, filespace, lcpl, H5P_DEFAULT, H5P_DEFAULT);

      if (localDims[0] > 0) {
        hid_t memspace = H5Screate_simple(dims, localDims, NULL);
        H5Sselect_hyperslab(filespace, H5S_SELECT_SET, offsets, NULL, localDims, NULL);
        H5Dwrite(dataset, type, memspace, filespace, dxpl, data);
        H5Sclose(memspace);
      } else if (dxpl != H5P_DEFAULT) {
        H5Sselect_none(filespace);
        H5Dwrite(dataset, type, H5S_ALL, filespace, dxpl, NULL);
      }
      H5Dclose(dataset);
      H5Sclose(filespace);
    };

  // Write Geometry
  hsize_t gDims[2] = {totalPoints, 3};
  hsize_t lDims[2] = {localNumPoints, 3};
  hsize_t off[2]   = {offsetPoints, 0};
  writeDataset("/geometry", 2, gDims, lDims, off, localData.pointsXYZ.data(), H5T_NATIVE_FLOAT);

  // Write Topology
  hsize_t gTDims[1] = {totalTopo};
  hsize_t lTDims[1] = {localTopoSize};
  hsize_t offT[1]   = {offsetTopo};
  writeDataset("/topology", 1, gTDims, lTDims, offT, localXdmfTopology.data(), H5T_NATIVE_INT);

  // Write Point Data Fields
  for (const auto& field : localData.pointFields) {
    hsize_t fDims[2]  = {totalPoints, static_cast<hsize_t>(field.components)};
    hsize_t flDims[2] = {localNumPoints, static_cast<hsize_t>(field.components)};
    hsize_t fOff[2]   = {offsetPoints, 0};
    writeDataset("/data/" + field.name, 2, fDims, flDims, fOff, field.values.data(), H5T_NATIVE_FLOAT);
  }

  // Write Cell Data Fields
  for (const auto& field : localData.cellFields) {
    hsize_t fDims[2]  = {totalCells, static_cast<hsize_t>(field.components)};
    hsize_t flDims[2] = {localNumCells, static_cast<hsize_t>(field.components)};
    hsize_t fOff[2]   = {offsetCells, 0};
    writeDataset("/cell_data/" + field.name, 2, fDims, flDims, fOff, field.values.data(), H5T_NATIVE_FLOAT);
  }

  H5Pclose(lcpl);
  if (dxpl != H5P_DEFAULT) {
    H5Pclose(dxpl);
  }
  H5Fclose(fileId);

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    std::ofstream out(xmfFilePath);
    if (not out.is_open()) {
      throw std::runtime_error("Could not write XDMF file " + xmfFilePath.string());
    } else {
      out
        << "<?xml version=\"1.0\" ?>\n"
        << "<!DOCTYPE Xdmf SYSTEM \"Xdmf.dtd\" []>\n"
        << "<Xdmf Version=\"2.0\">\n"
        << " <Domain>\n"
        << "  <Grid Name=\"PeanoGrid\" GridType=\"Uniform\">\n"
        << "   <Topology TopologyType=\"Mixed\" NumberOfElements=\"" << totalCells << "\">\n"
        << "    <DataItem Format=\"HDF\" DataType=\"Int\" Dimensions=\"" << totalTopo << "\">\n"
        << "      " << h5FilePath.filename().string() << ":/topology\n"
        << "    </DataItem>\n"
        << "   </Topology>\n"
        << "   <Geometry GeometryType=\"XYZ\">\n"
        << "    <DataItem Format=\"HDF\" DataType=\"Float\" Precision=\"4\" Dimensions=\"" << totalPoints << " 3\">\n"
        << "      " << h5FilePath.filename().string() << ":/geometry\n"
        << "    </DataItem>\n"
        << "   </Geometry>\n";

      // Helper to write attributes
      auto writeAttrs = [&](const std::vector<DataField>& fields, const std::string& center, const std::string& path) {
        for (const auto& field : fields) {
          std::string type = "Scalar";
          if (field.components == 3) {
            type = "Vector";
          } else if (field.components != 1) {
            type = "Matrix";
          }

          out
            << "   <Attribute Name=\"" << field.name << "\" AttributeType=\"" << type << "\" Center=\"" << center
            << "\">\n"
            << "    <DataItem Format=\"HDF\" DataType=\"Float\" Precision=\"4\" Dimensions=\""
            << (center == "Node" ? totalPoints : totalCells) << " " << field.components << "\">\n"
            << "      " << h5FilePath.filename().string() << ":" << path << field.name << "\n"
            << "    </DataItem>\n"
            << "   </Attribute>\n";
        }
      };

      writeAttrs(localData.pointFields, "Node", "/data/");
      writeAttrs(localData.cellFields, "Cell", "/cell_data/");

      out
        << "  </Grid>\n"
        << " </Domain>\n"
        << "</Xdmf>\n";
    }
  }

  _written = true;
  return true;
}

#endif
