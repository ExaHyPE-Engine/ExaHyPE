// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "NetCDFFileWriter.h"

#ifdef PEANO_WITH_NETCDF

#include "tarch/mpi/Rank.h"
#include "tarch/mpi/mpi.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <netcdf.h>
#include <stdexcept>
#include <string>
#include <vector>

#if defined(PEANO_WITH_MPI)
#include <netcdf_par.h>
#endif

namespace fs = std::filesystem;

namespace {
  constexpr int VTK_TRIANGLE = 5;
  constexpr int VTK_QUAD     = 9;

  constexpr int          MAX_FACE_NODES = 4;  // triangles/quads
  constexpr std::int32_t FILL_VALUE     = -1; // for unused nodes in a face

  inline void require2DCell(int vtkType) {
    if (vtkType != VTK_TRIANGLE and vtkType != VTK_QUAD) {
      throw std::runtime_error(
        "NetCDFFileWriter (UGRID 2D) only supports triangle (VTK=5) and quadrangle (VTK=9) cells. "
        "Encountered VTK cell type " +
        std::to_string(vtkType) + "."
      );
    }
  }
} // namespace

tarch::plotter::NetCDFFileWriter::NetCDFFileWriter(const std::string& outputDirectory, int numberOfSubdomains) :
  FileWriter(outputDirectory, numberOfSubdomains) {}

void tarch::plotter::NetCDFFileWriter::check(int status, const std::string& where) {
  if (status != NC_NOERR) {
    throw std::runtime_error("NetCDF error in " + where + ": " + std::string(nc_strerror(status)));
  }
}

std::string tarch::plotter::NetCDFFileWriter::getFileExtension() const { return ".nc"; }

bool tarch::plotter::NetCDFFileWriter::writeToFile(const std::string& filename, double time) {
  if (_written) {
    throw std::runtime_error("Cannot write to file. File already written.");
  }

  createOutputDirectory();

  fs::path outDir(_outputDirectory);
  fs::path filePath = outDir / (filename + ".nc");

  SubdomainBuffers localData = mergeSubdomains();

  const unsigned long long localNumNodes = localData.pointsXYZ.size() / 3;
  const unsigned long long localNumFaces = localData.types.size();

  // Validate cell types and build the (nFace x 4) UGRID face-node connectivity.
  // We store a padded connectivity with _FillValue for triangles.
  std::vector<std::int32_t> localFaceNodes;
  localFaceNodes.resize(static_cast<std::size_t>(localNumFaces) * MAX_FACE_NODES, FILL_VALUE);

  std::int32_t connBegin = 0;
  for (std::size_t c = 0; c < localData.types.size(); ++c) {
    const int vtkType = static_cast<int>(localData.types[c]);
    require2DCell(vtkType);

    const std::int32_t connEnd = localData.offsets[c];
    const std::int32_t nNodes  = connEnd - connBegin;

    if (vtkType == VTK_TRIANGLE and nNodes != 3) {
      throw std::runtime_error("Triangle cell has " + std::to_string(nNodes) + " nodes (expected 3).");
    }
    if (vtkType == VTK_QUAD and nNodes != 4) {
      throw std::runtime_error("Quadrangle cell has " + std::to_string(nNodes) + " nodes (expected 4).");
    }

    for (int k = 0; k < nNodes; ++k) {
      localFaceNodes[c * MAX_FACE_NODES + k] = localData.connectivity[connBegin + k];
    }

    connBegin = connEnd;
  }

  unsigned long long offsetNodes = 0;
  unsigned long long offsetFaces = 0;
  unsigned long long totalNodes  = localNumNodes;
  unsigned long long totalFaces  = localNumFaces;

#if defined(PEANO_WITH_MPI)
  auto&    rank          = tarch::mpi::Rank::getInstance();
  MPI_Comm comm          = rank.getCommunicator();
  bool     useParallelIO = rank.getNumberOfRanks() > 1;
  bool     isMasterRank  = rank.isGlobalMaster();
#else
  bool useParallelIO = false;
  bool isMasterRank  = true;
#endif

#if defined(PEANO_WITH_MPI)
  if (useParallelIO) {
    tarch::mpi::scan(&localNumNodes, &offsetNodes, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, comm);
    tarch::mpi::scan(&localNumFaces, &offsetFaces, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, comm);
    offsetNodes -= localNumNodes;
    offsetFaces -= localNumFaces;
    tarch::mpi::allreduce(&localNumNodes, &totalNodes, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, comm);
    tarch::mpi::allreduce(&localNumFaces, &totalFaces, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, comm);
  }
#endif

  // Shift connectivity to global node indices
  if (offsetNodes != 0 and not localFaceNodes.empty()) {
    for (auto& idx : localFaceNodes) {
      if (idx != FILL_VALUE) {
        idx += static_cast<std::int32_t>(offsetNodes);
      }
    }
  }

  int ncid = -1;

#if defined(PEANO_WITH_MPI)
  if (useParallelIO) {
    check(
      nc_create_par(filePath.string().c_str(), NC_NETCDF4 | NC_CLOBBER, comm, MPI_INFO_NULL, &ncid),
      "nc_create_par(" + filePath.string() + ")"
    );
  } else {
    check(nc_create(filePath.string().c_str(), NC_NETCDF4 | NC_CLOBBER, &ncid), "nc_create(" + filePath.string() + ")");
  }
#else
  check(nc_create(filePath.string().c_str(), NC_NETCDF4 | NC_CLOBBER, &ncid), "nc_create(" + filePath.string() + ")");
#endif

  // Disable pre-filling for performance (we always write all values we expose)
  int oldFill = 0;
  check(nc_set_fill(ncid, NC_NOFILL, &oldFill), "nc_set_fill(NO_FILL)");

  // Global attributes
  {
    const char* conv = "CF-1.10 UGRID-1.0";
    check(nc_put_att_text(ncid, NC_GLOBAL, "Conventions", std::strlen(conv), conv), "put Conventions");
  }

  // Dimensions
  int dimOne = -1;
  check(nc_def_dim(ncid, "one", 1, &dimOne), "def dim one");
  int dimTime = -1;
  check(nc_def_dim(ncid, "time", NC_UNLIMITED, &dimTime), "def dim time");
  int dimNode = -1;
  check(nc_def_dim(ncid, "nMesh2d_node", static_cast<std::size_t>(totalNodes), &dimNode), "def dim nMesh2d_node");
  int dimFace = -1;
  check(nc_def_dim(ncid, "nMesh2d_face", static_cast<std::size_t>(totalFaces), &dimFace), "def dim nMesh2d_face");
  int dimMaxFaceNodes = -1;
  check(nc_def_dim(ncid, "nMax_face_nodes", MAX_FACE_NODES, &dimMaxFaceNodes), "def dim nMax_face_nodes");

  // Mesh topology dummy variable
  int varMesh = -1;
  {
    int meshDims[1] = {dimOne};
    check(nc_def_var(ncid, "mesh2d", NC_INT, 1, meshDims, &varMesh), "def var mesh2d");
    const char* role = "mesh_topology";
    check(nc_put_att_text(ncid, varMesh, "cf_role", std::strlen(role), role), "mesh cf_role");
    const int topoDim = 2;
    check(nc_put_att_int(ncid, varMesh, "topology_dimension", NC_INT, 1, &topoDim), "mesh topology_dimension");
    const char* nodeCoords = "x y";
    check(
      nc_put_att_text(ncid, varMesh, "node_coordinates", std::strlen(nodeCoords), nodeCoords),
      "mesh node_coordinates"
    );
    const char* faceConn = "face_nodes";
    check(
      nc_put_att_text(ncid, varMesh, "face_node_connectivity", std::strlen(faceConn), faceConn),
      "mesh face_node_connectivity"
    );
    const char* faceDimName = "nMesh2d_face";
    check(
      nc_put_att_text(ncid, varMesh, "face_dimension", std::strlen(faceDimName), faceDimName),
      "mesh face_dimension"
    );
  }

  // Time
  int varTime = -1;
  {
    int d[1] = {dimTime};
    check(nc_def_var(ncid, "time", NC_DOUBLE, 1, d, &varTime), "def var time");
    const char* units = "s";
    check(nc_put_att_text(ncid, varTime, "units", std::strlen(units), units), "time units");
    const char* stdname = "time";
    check(nc_put_att_text(ncid, varTime, "standard_name", std::strlen(stdname), stdname), "time standard_name");
  }

  // Coordinates
  int varX = -1;
  int varY = -1;
  {
    int dims1[1] = {dimNode};
    check(nc_def_var(ncid, "x", NC_FLOAT, 1, dims1, &varX), "def var x");
    check(nc_def_var(ncid, "y", NC_FLOAT, 1, dims1, &varY), "def var y");

    // Minimal CF metadata
    const char* sx = "projection_x_coordinate";
    const char* sy = "projection_y_coordinate";
    check(nc_put_att_text(ncid, varX, "standard_name", std::strlen(sx), sx), "x standard_name");
    check(nc_put_att_text(ncid, varY, "standard_name", std::strlen(sy), sy), "y standard_name");

    // Chunking + compression for storage efficiency (skip if empty)
    if (totalNodes > 0) {
      const std::size_t chunk0 =
        std::max<std::size_t>(1, std::min<std::size_t>(static_cast<std::size_t>(totalNodes), 4096));
      std::size_t chunks[1] = {chunk0};
      check(nc_def_var_chunking(ncid, varX, NC_CHUNKED, chunks), "chunking x");
      check(nc_def_var_chunking(ncid, varY, NC_CHUNKED, chunks), "chunking y");
      check(nc_def_var_deflate(ncid, varX, 1, 1, 1), "deflate x");
      check(nc_def_var_deflate(ncid, varY, 1, 1, 1), "deflate y");
    }
  }

  // Face-node connectivity
  int varFaceNodes = -1;
  {
    int dims2[2] = {dimFace, dimMaxFaceNodes};
    check(nc_def_var(ncid, "face_nodes", NC_INT, 2, dims2, &varFaceNodes), "def var face_nodes");
    const char* role = "face_node_connectivity";
    check(nc_put_att_text(ncid, varFaceNodes, "cf_role", std::strlen(role), role), "face_nodes cf_role");
    const int startIndex = 0;
    check(nc_put_att_int(ncid, varFaceNodes, "start_index", NC_INT, 1, &startIndex), "face_nodes start_index");
    // Fill value for padding (triangles)
    const int fv = static_cast<int>(FILL_VALUE);
    check(nc_put_att_int(ncid, varFaceNodes, "_FillValue", NC_INT, 1, &fv), "face_nodes _FillValue");

    // Chunking + compression for storage efficiency.
    // Avoid chunk setup when the corresponding dimension is zero.
    if (totalFaces > 0) {
      const std::size_t chunkFaces =
        std::max<std::size_t>(1, std::min<std::size_t>(static_cast<std::size_t>(totalFaces), 1024));
      std::size_t chunks[2] = {chunkFaces, static_cast<std::size_t>(MAX_FACE_NODES)};
      check(nc_def_var_chunking(ncid, varFaceNodes, NC_CHUNKED, chunks), "chunking face_nodes");
      check(nc_def_var_deflate(ncid, varFaceNodes, 1, 1, 1), "deflate face_nodes");
    }
  }

  // Data fields
  std::vector<std::vector<int>> pointVarIds(localData.pointFields.size());
  std::vector<std::vector<int>> faceVarIds(localData.cellFields.size());

  auto defDataVar = [&](const DataField& f, bool onNodes, std::vector<int>& outVarIds) {
    int         dimId     = onNodes ? dimNode : dimFace;
    std::size_t globalLen = onNodes ? totalNodes : totalFaces;

    for (int c = 0; c < f.components; ++c) {
      std::string name = f.name;
      if (f.components > 1) {
        name += "_" + std::to_string(c);
      }

      int varId = -1;
      int d[1]  = {dimId};
      check(nc_def_var(ncid, name.c_str(), NC_FLOAT, 1, d, &varId), "def var " + name);

      std::string longName = f.name;
      if (f.components > 1) {
        longName += " (component " + std::to_string(c) + ")";
      } else {
        longName += " (scalar)";
      }
      check(
        nc_put_att_text(ncid, varId, "long_name", longName.length(), longName.c_str()),
        "attr long_name for " + name
      );

      // Standard UGRID Attributes
      const char* meshName = "mesh2d";
      check(nc_put_att_text(ncid, varId, "mesh", std::strlen(meshName), meshName), "attr mesh for " + name);

      const char* location = onNodes ? "node" : "face";
      check(nc_put_att_text(ncid, varId, "location", std::strlen(location), location), "attr location for " + name);

      // Chunking & Compression
      if (globalLen > 0) {
        const std::size_t chunkLen  = std::max<std::size_t>(1, std::min<std::size_t>(globalLen, 4096));
        std::size_t       chunks[1] = {chunkLen};
        check(nc_def_var_chunking(ncid, varId, NC_CHUNKED, chunks), "chunking " + name);
        check(nc_def_var_deflate(ncid, varId, 1, 1, 1), "deflate " + name);
      }

      outVarIds.push_back(varId);
    }
  };

  for (std::size_t i = 0; i < localData.pointFields.size(); ++i) {
    defDataVar(localData.pointFields[i], true, pointVarIds[i]);
  }
  for (std::size_t i = 0; i < localData.cellFields.size(); ++i) {
    defDataVar(localData.cellFields[i], false, faceVarIds[i]);
  }

  check(nc_enddef(ncid), "nc_enddef");

#if defined(PEANO_WITH_MPI)
  if (useParallelIO) {
    // Scalar metadata variables are written collectively below. Non-master ranks
    // contribute an empty selection, which keeps parallel NetCDF/HDF5 backends in
    // collective data mode.
    check(nc_var_par_access(ncid, varMesh, NC_COLLECTIVE), "par_access mesh2d");
    check(nc_var_par_access(ncid, varTime, NC_COLLECTIVE), "par_access time");
    check(nc_var_par_access(ncid, varX, NC_COLLECTIVE), "par_access x");
    check(nc_var_par_access(ncid, varY, NC_COLLECTIVE), "par_access y");
    check(nc_var_par_access(ncid, varFaceNodes, NC_COLLECTIVE), "par_access face_nodes");
    for (const auto& ids : pointVarIds) {
      for (int id : ids) {
        check(nc_var_par_access(ncid, id, NC_COLLECTIVE), "par_access point field");
      }
    }
    for (const auto& ids : faceVarIds) {
      for (int id : ids) {
        check(nc_var_par_access(ncid, id, NC_COLLECTIVE), "par_access face field");
      }
    }
  }
#endif

  // Write coordinates (extract x/y from interlaced XYZ buffer)
  std::vector<float> x;
  std::vector<float> y;
  x.reserve(static_cast<std::size_t>(localNumNodes));
  y.reserve(static_cast<std::size_t>(localNumNodes));
  for (std::size_t i = 0; i < localData.pointsXYZ.size(); i += 3) {
    x.push_back(localData.pointsXYZ[i + 0]);
    y.push_back(localData.pointsXYZ[i + 1]);
  }

  // Write time scalar
  {
    std::size_t startT[1] = {0};
    std::size_t countT[1] = {useParallelIO ? static_cast<std::size_t>(isMasterRank ? 1 : 0) : 1};
    check(nc_put_vara_double(ncid, varTime, startT, countT, &time), "write time value");
  }

  const float dummyF = 0.0f;
  const int   dummyI = 0;

  // Write a defined value for the mesh topology dummy variable
  {
    std::size_t startMesh[1] = {0};
    std::size_t countMesh[1] = {useParallelIO ? static_cast<std::size_t>(isMasterRank ? 1 : 0) : 1};
    check(nc_put_vara_int(ncid, varMesh, startMesh, countMesh, &dummyI), "write mesh2d");
  }

  {
    std::size_t start[1] = {static_cast<std::size_t>(offsetNodes)};
    std::size_t count[1] = {static_cast<std::size_t>(localNumNodes)};
    check(nc_put_vara_float(ncid, varX, start, count, localNumNodes ? x.data() : &dummyF), "write x");
    check(nc_put_vara_float(ncid, varY, start, count, localNumNodes ? y.data() : &dummyF), "write y");
  }

  // Write connectivity
  {
    std::size_t start[2] = {static_cast<std::size_t>(offsetFaces), 0};
    std::size_t count[2] = {static_cast<std::size_t>(localNumFaces), static_cast<std::size_t>(MAX_FACE_NODES)};
    check(
      nc_put_vara_int(ncid, varFaceNodes, start, count, localNumFaces ? localFaceNodes.data() : &dummyI),
      "write face_nodes"
    );
  }

  // Write fields
  auto writeField =
    [&](const std::vector<int>& varIds, const DataField& f, unsigned long long offset, unsigned long long nTuples) {
    if (nTuples == 0) {
      return;
    }

    // We need to split the interlaced data in f.values [nTuples * nComponents]
    // into separate buffers for each component [nTuples].
    int                numComponents = f.components;
    std::vector<float> buffer(nTuples); // Temp buffer for one component

    std::size_t start[1] = {static_cast<std::size_t>(offset)};
    std::size_t count[1] = {static_cast<std::size_t>(nTuples)};

    for (int c = 0; c < numComponents; ++c) {
      // Extract component 'c' from the interlaced array
      for (std::size_t i = 0; i < nTuples; ++i) {
        buffer[i] = static_cast<float>(f.values[i * numComponents + c]);
      }

      check(nc_put_vara_float(ncid, varIds[c], start, count, buffer.data()), "write field " + f.name);
    }
  };

  for (std::size_t i = 0; i < localData.pointFields.size(); ++i) {
    writeField(pointVarIds[i], localData.pointFields[i], offsetNodes, localNumNodes);
  }
  for (std::size_t i = 0; i < localData.cellFields.size(); ++i) {
    writeField(faceVarIds[i], localData.cellFields[i], offsetFaces, localNumFaces);
  }

  check(nc_close(ncid), "nc_close");

  _written = true;
  return true;
}

#endif
