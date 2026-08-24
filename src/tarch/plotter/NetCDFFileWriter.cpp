// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "NetCDFFileWriter.h"

#ifdef PEANO_WITH_NETCDF

#include "CellTypes.h"

#include "tarch/Termination.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"

#include <netcdf.h>
#include <netcdf_par.h>

namespace {
  // Largest 2D face / 3D volume corner counts, taken from the CellTypes table
  // (CellTypes.h) so the fixed-width connectivity padding cannot drift.
  constexpr int MaxFaceNodes   = tarch::plotter::nodesOf(tarch::plotter::CellType::Quadrangle); // 4: triangles/quads
  constexpr int MaxVolumeNodes = tarch::plotter::nodesOf(tarch::plotter::CellType::Hexahedron); // 8: hexahedra
  constexpr std::int32_t FillValue          = -1; // for unused nodes in padded connectivities
  constexpr int          UgridHexahedronTag = 2;  // UGRID flag_values: 0=tetrahedron, 1=wedge, 2=hexahedron

  enum class MeshTopologyDimension { TwoD = 2, ThreeD = 3 };

  // UGRID meshes carry 2D faces (triangle/quadrangle) or 3D volumes (hexahedron).
  inline bool is2DCell(const int vtkType) {
    const tarch::plotter::CellTypeInfo* const info = tarch::plotter::lookupCellType(vtkType);
    return info != nullptr and info->dimension == 2;
  }

  inline bool is3DCell(const int vtkType) {
    const tarch::plotter::CellTypeInfo* const info = tarch::plotter::lookupCellType(vtkType);
    return info != nullptr and info->dimension == 3;
  }

  inline bool hasNonZeroZCoordinate(const std::vector<double>& pointsXYZ) {
    for (std::size_t i = 2; i < pointsXYZ.size(); i += 3) {
      if (pointsXYZ[i] != 0.0) {
        return true;
      }
    }
    return false;
  }
} // namespace

tarch::plotter::NetCDFFileWriter::NetCDFFileWriter(const std::string& outputDirectory, const int numberOfSubdomains) :
  FileWriter(outputDirectory, numberOfSubdomains) {}

void tarch::plotter::NetCDFFileWriter::check(const int status, const std::string& operation) {
  if (status != NC_NOERR) {
    tarch::terminateWithError("NetCDF operation '" + operation + "' failed: " + std::string(nc_strerror(status)) + ".");
  }
}

std::string tarch::plotter::NetCDFFileWriter::getFileExtension() const { return ".nc"; }

bool tarch::plotter::NetCDFFileWriter::writeToFile(const std::string& filename, const double time) {
  if (_written) {
    throw std::runtime_error("Cannot write a plotter file more than once.");
  }

  createOutputDirectory();

  const std::filesystem::path outputDirectory(_outputDirectory);
  const std::filesystem::path filePath = outputDirectory / (filename + ".nc");

  const auto localData = getSubdomainBuffersSequenceForWriting();

  unsigned long long localNumNodes    = 0;
  unsigned long long localNumElements = 0;
  for (const SubdomainBuffers* const subdomain : localData.payloads) {
    localNumNodes += subdomain->pointsXYZ.size() / 3;
    localNumElements += subdomain->types.size();
  }

  bool localHasFaces   = false;
  bool localHasVolumes = false;
  for (const SubdomainBuffers* const subdomain : localData.payloads) {
    for (const std::uint8_t cellType : subdomain->types) {
      const int vtkType = static_cast<int>(cellType);
      if (is2DCell(vtkType)) {
        localHasFaces = true;
      } else if (is3DCell(vtkType)) {
        localHasVolumes = true;
      } else {
        throw std::runtime_error(
          "NetCDFFileWriter (UGRID) supports triangle/quadrangle faces and hexahedron volumes. Encountered VTK cell type "
          + std::to_string(vtkType) + "."
        );
      }
    }
  }
  bool localHas3DCoordinates = false;
  for (const SubdomainBuffers* const subdomain : localData.payloads) {
    localHas3DCoordinates = localHas3DCoordinates or hasNonZeroZCoordinate(subdomain->pointsXYZ);
  }

  const auto& rank          = tarch::mpi::Rank::getInstance();
  MPI_Comm    comm          = rank.getCommunicator();
  bool        useParallelIO = rank.getNumberOfRanks() > 1;
  bool        isMasterRank  = rank.isGlobalMaster();

  // Do the I/O on a sub-communicator of the ranks that hold data. An empty rank's
  // count-0 collective write to a compressed variable deadlocks parallel netCDF (the
  // MPI implementations disagree on whether it engages), so we exclude such ranks.
  MPI_Comm ioComm = MPI_COMM_NULL;
  if (useParallelIO) {
    const bool hasData    = localData.reference != nullptr;
    const int  localHas   = hasData ? 1 : 0;
    int        anyHasData = 0;
    tarch::mpi::allreduce(&localHas, &anyHasData, 1, MPI_INT, MPI_MAX, comm);

    if (anyHasData == 0) {
      // Every rank is empty: the master alone writes a (serial) valid empty file.
      if (not isMasterRank) {
        _written = true;
        return true;
      }
      useParallelIO = false;
    } else {
      MPI_Comm_split(comm, hasData ? 0 : MPI_UNDEFINED, rank.getRank(), &ioComm);
      if (not hasData) {
        _written = true;
        return true; // nothing to contribute; the ranks with data write the file
      }
      comm       = ioComm;
      int ioRank = 0;
      MPI_Comm_rank(ioComm, &ioRank);
      isMasterRank = (ioRank == 0);
    }
  }

  bool globalHasFaces         = localHasFaces;
  bool globalHasVolumes       = localHasVolumes;
  bool globalHas3DCoordinates = localHas3DCoordinates;

  if (useParallelIO) {
    // One max-allreduce so every participating rank agrees on the global mesh
    // character: [0] any rank has 2D faces, [1] any rank has 3D volumes, [2] any rank
    // has a non-zero z coordinate.
    const int localReduce[3]  = {localHasFaces ? 1 : 0, localHasVolumes ? 1 : 0, localHas3DCoordinates ? 1 : 0};
    int       globalReduce[3] = {0, 0, 0};
    tarch::mpi::allreduce(localReduce, globalReduce, 3, MPI_INT, MPI_MAX, comm);

    globalHasFaces         = globalReduce[0] != 0;
    globalHasVolumes       = globalReduce[1] != 0;
    globalHas3DCoordinates = globalReduce[2] != 0;
  }

  if (globalHasFaces and globalHasVolumes) {
    throw std::runtime_error(
      "NetCDFFileWriter does not support mixed 2D face cells and 3D volume cells in one UGRID mesh."
    );
  }

  const MeshTopologyDimension topologyDimension =
    globalHasVolumes ? MeshTopologyDimension::ThreeD : MeshTopologyDimension::TwoD;
  const bool is3DTopology     = topologyDimension == MeshTopologyDimension::ThreeD;
  const bool writeZCoordinate = is3DTopology or globalHas3DCoordinates;
  const int  maxElementNodes  = is3DTopology ? MaxVolumeNodes : MaxFaceNodes;

  // Append the subdomain's element-node table to the merged buffer (padded to
  // maxElementNodes per cell with FillValue), rebasing node indices to global.
  const auto appendElementNodes =
    [&](std::vector<int>& elementNodes, const SubdomainBuffers& subdomain, const unsigned long long pointBase) {
      const std::size_t base = elementNodes.size();
      elementNodes.resize(base + subdomain.types.size() * static_cast<std::size_t>(maxElementNodes), FillValue);

      const std::size_t numberOfPoints    = subdomain.pointsXYZ.size() / 3;
      std::int32_t      connectivityBegin = 0;
      for (std::size_t c = 0; c < subdomain.types.size(); ++c) {
        const int           vtkType = static_cast<int>(subdomain.types[c]);
        const CellTypeInfo* info    = lookupCellType(vtkType);

        if ((is3DTopology and not is3DCell(vtkType)) or (not is3DTopology and not is2DCell(vtkType))) {
          throw std::runtime_error(
            "NetCDFFileWriter encountered a cell type that is inconsistent with the mesh topology."
          );
        }
        // The consistency check above guarantees a known 2D/3D cell, so info is non-null here.

        const std::int32_t connectivityEnd = subdomain.offsets[c];
        const std::int32_t numberOfNodes   = connectivityEnd - connectivityBegin;
        const int          expectedNodes   = info->nodes;

        if (numberOfNodes != expectedNodes) {
          throw std::runtime_error(
            std::string(info->name) + " cell has " + std::to_string(numberOfNodes) + " nodes (expected "
            + std::to_string(expectedNodes) + ")."
          );
        }

        for (int k = 0; k < numberOfNodes; ++k) {
          const std::int32_t localIndex = subdomain.connectivity[connectivityBegin + k];
          if (localIndex < 0 or static_cast<std::size_t>(localIndex) >= numberOfPoints) {
            throw std::runtime_error("NetCDFFileWriter encountered a cell connectivity index outside its subdomain.");
          }

          const unsigned long long globalIndex = pointBase + static_cast<unsigned long long>(localIndex);
          if (globalIndex > static_cast<unsigned long long>(std::numeric_limits<int>::max())) {
            throw std::runtime_error("NetCDFFileWriter cannot write node indices that exceed 32-bit integer range.");
          }

          elementNodes[base + c * static_cast<std::size_t>(maxElementNodes) + k] = static_cast<int>(globalIndex);
        }

        connectivityBegin = connectivityEnd;
      }
    };

  unsigned long long offsetNodes    = 0;
  unsigned long long offsetElements = 0;
  unsigned long long totalNodes     = localNumNodes;
  unsigned long long totalElements  = localNumElements;

  if (useParallelIO) {
    // One scan and one allreduce over a 2-element array, rather than two of each.
    const unsigned long long localCounts[2] = {localNumNodes, localNumElements};

    unsigned long long prefix[2] = {0, 0};
    tarch::mpi::scan(localCounts, prefix, 2, MPI_UNSIGNED_LONG_LONG, MPI_SUM, comm);
    offsetNodes    = prefix[0] - localNumNodes;
    offsetElements = prefix[1] - localNumElements;

    unsigned long long totals[2] = {0, 0};
    tarch::mpi::allreduce(localCounts, totals, 2, MPI_UNSIGNED_LONG_LONG, MPI_SUM, comm);
    totalNodes    = totals[0];
    totalElements = totals[1];
  }

  int ncid = -1;

  if (useParallelIO) {
    MPI_Info   info  = MPI_INFO_NULL;
    const auto hints = mpiIoHints();
    if (not hints.empty()) {
      MPI_Info_create(&info);
      for (const auto& hint : hints) {
        MPI_Info_set(info, hint.first.c_str(), hint.second.c_str());
      }
    }
    const int status = nc_create_par(filePath.string().c_str(), NC_NETCDF4 | NC_CLOBBER, comm, info, &ncid);
    if (info != MPI_INFO_NULL) {
      MPI_Info_free(&info);
    }
    check(status, "nc_create_par(" + filePath.string() + ")");
  } else {
    check(nc_create(filePath.string().c_str(), NC_NETCDF4 | NC_CLOBBER, &ncid), "nc_create(" + filePath.string() + ")");
  }

  // Close the open file if a check() throws before the explicit nc_close below.
  // Disarmed on the success path.
  class NcFileCloser {
  public:
    explicit NcFileCloser(const int id) :
      _id(id) {}
    NcFileCloser(const NcFileCloser&) = delete;
    NcFileCloser(NcFileCloser&&)      = delete;
    ~NcFileCloser() {
      if (_armed and _id >= 0) {
        nc_close(_id);
      }
    }
    NcFileCloser& operator=(const NcFileCloser&) = delete;
    NcFileCloser& operator=(NcFileCloser&&)      = delete;

    void release() { _armed = false; }

  private:
    const int _id;
    bool      _armed = true;
  } ncFileCloser(ncid);

  // Disable pre-filling: the payload variables are written whole. The connectivity index
  // variables are the exception and stay undefined on read, which their own definition
  // records.
  int oldFill = 0;
  check(nc_set_fill(ncid, NC_NOFILL, &oldFill), "nc_set_fill(NO_FILL)");

  // Global attributes
  {
    const char* const conventions = "CF-1.10 UGRID-1.0";
    check(nc_put_att_text(ncid, NC_GLOBAL, "Conventions", std::strlen(conventions), conventions), "put Conventions");
  }

  const char* const meshName            = is3DTopology ? "mesh3d" : "mesh2d";
  const char* const nodeDimName         = is3DTopology ? "nMesh3d_node" : "nMesh2d_node";
  const char* const elementDimName      = is3DTopology ? "nMesh3d_volume" : "nMesh2d_face";
  const char* const maxElementDimName   = is3DTopology ? "nMaxMesh3d_volume_nodes" : "nMax_face_nodes";
  const char* const elementNodesName    = is3DTopology ? "volume_nodes" : "face_nodes";
  const char* const elementNodesRole    = is3DTopology ? "volume_node_connectivity" : "face_node_connectivity";
  const char* const elementDimensionAtt = is3DTopology ? "volume_dimension" : "face_dimension";
  const char* const elementLocation     = is3DTopology ? "volume" : "face";

  // Size deflated chunks to a ~1 MiB byte target, which the element count derives from
  // through the width the bulk variables are stored in. Tiny chunks multiply the per-chunk
  // filter cost; larger ones reach across more rank boundaries.
  constexpr std::size_t ChunkByteTarget    = 1u << 20;
  const std::size_t     bytesPerBulkScalar = getDoublePrecisionOutput() ? sizeof(double) : sizeof(float);

  const auto configure1DVariable =
    [&](const int variableId, const unsigned long long globalLength, const std::string& name) {
      if (globalLength == 0) {
        return;
      }

      const std::size_t targetElems = ChunkByteTarget / bytesPerBulkScalar;
      const std::size_t chunk0 =
        std::max<std::size_t>(1, std::min<std::size_t>(static_cast<std::size_t>(globalLength), targetElems));
      const std::size_t chunks[1] = {chunk0};
      check(nc_def_var_chunking(ncid, variableId, NC_CHUNKED, chunks), "chunking " + name);
      check(nc_def_var_deflate(ncid, variableId, 1, 1, 1), "deflate " + name);
      // Size the cache to hold the chunk, so each slab is deflated once.
      nc_set_var_chunk_cache(
        ncid,
        variableId,
        std::max<std::size_t>(chunk0 * bytesPerBulkScalar, ChunkByteTarget),
        1009,
        0.75f
      );
    };

  const auto configure2DVariable =
    [&](const int variableId, const unsigned long long globalRows, const std::size_t columns, const std::string& name) {
      if (globalRows == 0) {
        return;
      }

      const std::size_t bytesPerRow = std::max<std::size_t>(1, columns * bytesPerBulkScalar);
      const std::size_t targetRows  = std::max<std::size_t>(1, ChunkByteTarget / bytesPerRow);
      const std::size_t chunkRows =
        std::max<std::size_t>(1, std::min<std::size_t>(static_cast<std::size_t>(globalRows), targetRows));
      const std::size_t chunks[2] = {chunkRows, columns};
      check(nc_def_var_chunking(ncid, variableId, NC_CHUNKED, chunks), "chunking " + name);
      check(nc_def_var_deflate(ncid, variableId, 1, 1, 1), "deflate " + name);
      nc_set_var_chunk_cache(
        ncid,
        variableId,
        std::max<std::size_t>(chunkRows * bytesPerRow, ChunkByteTarget),
        1009,
        0.75f
      );
    };

  // Dimensions
  int dimOne = -1;
  check(nc_def_dim(ncid, "one", 1, &dimOne), "def dim one");
  int dimTime = -1;
  check(nc_def_dim(ncid, "time", NC_UNLIMITED, &dimTime), "def dim time");
  int dimNode = -1;
  check(
    nc_def_dim(ncid, nodeDimName, static_cast<std::size_t>(totalNodes), &dimNode),
    std::string("def dim ") + nodeDimName
  );
  int dimElement = -1;
  check(
    nc_def_dim(ncid, elementDimName, static_cast<std::size_t>(totalElements), &dimElement),
    std::string("def dim ") + elementDimName
  );
  int dimMaxElementNodes = -1;
  check(
    nc_def_dim(ncid, maxElementDimName, maxElementNodes, &dimMaxElementNodes),
    std::string("def dim ") + maxElementDimName
  );

  // A dimension with no same-named coordinate variable gets a phony dimension scale that
  // parallel netCDF-4 stores uncompressed and fully pre-allocated (megabytes per large
  // mesh dimension). A chunked, deflated coordinate variable named after the dimension
  // replaces it; left unwritten it costs no I/O, and readers treat a variable named after
  // its dimension as a coordinate, not a field.
  int varNodeIndex = -1;
  check(
    nc_def_var(ncid, nodeDimName, NC_INT, 1, &dimNode, &varNodeIndex),
    std::string("def dimension-scale ") + nodeDimName
  );
  configure1DVariable(varNodeIndex, totalNodes, nodeDimName);
  int varElementIndex = -1;
  check(
    nc_def_var(ncid, elementDimName, NC_INT, 1, &dimElement, &varElementIndex),
    std::string("def dimension-scale ") + elementDimName
  );
  configure1DVariable(varElementIndex, totalElements, elementDimName);

  // Mesh topology dummy variable
  int varMesh = -1;
  {
    const int meshDims[1] = {dimOne};
    check(nc_def_var(ncid, meshName, NC_INT, 1, meshDims, &varMesh), std::string("def var ") + meshName);
    const char* const role = "mesh_topology";
    check(nc_put_att_text(ncid, varMesh, "cf_role", std::strlen(role), role), "mesh cf_role");
    const int topoDim = static_cast<int>(topologyDimension);
    check(nc_put_att_int(ncid, varMesh, "topology_dimension", NC_INT, 1, &topoDim), "mesh topology_dimension");
    const char* const nodeCoords = writeZCoordinate ? "x y z" : "x y";
    check(
      nc_put_att_text(ncid, varMesh, "node_coordinates", std::strlen(nodeCoords), nodeCoords),
      "mesh node_coordinates"
    );
    check(
      nc_put_att_text(ncid, varMesh, elementNodesRole, std::strlen(elementNodesName), elementNodesName),
      std::string("mesh ") + elementNodesRole
    );
    check(
      nc_put_att_text(ncid, varMesh, elementDimensionAtt, std::strlen(elementDimName), elementDimName),
      std::string("mesh ") + elementDimensionAtt
    );
    if (is3DTopology) {
      const char* const volumeShapeType = "volume_shape_type";
      check(
        nc_put_att_text(ncid, varMesh, "volume_shape_type", std::strlen(volumeShapeType), volumeShapeType),
        "mesh volume_shape_type"
      );
    }
  }

  // Time
  int varTime = -1;
  {
    const int timeDims[1] = {dimTime};
    check(nc_def_var(ncid, "time", NC_DOUBLE, 1, timeDims, &varTime), "def var time");
    const char* const units = "s";
    check(nc_put_att_text(ncid, varTime, "units", std::strlen(units), units), "time units");
    const char* const standardName = "time";
    check(
      nc_put_att_text(ncid, varTime, "standard_name", std::strlen(standardName), standardName),
      "time standard_name"
    );
  }

  // Coordinates. Single-precision output narrows the double buffers at write time.
  const bool    writeDouble     = getDoublePrecisionOutput();
  const nc_type realStorageType = writeDouble ? NC_DOUBLE : NC_FLOAT;
  int           varX            = -1;
  int           varY            = -1;
  int           varZ            = -1;
  {
    const int dims1[1] = {dimNode};
    check(nc_def_var(ncid, "x", realStorageType, 1, dims1, &varX), "def var x");
    check(nc_def_var(ncid, "y", realStorageType, 1, dims1, &varY), "def var y");
    if (writeZCoordinate) {
      check(nc_def_var(ncid, "z", realStorageType, 1, dims1, &varZ), "def var z");
    }

    // Minimal CF metadata
    const char* const standardNameX = "projection_x_coordinate";
    const char* const standardNameY = "projection_y_coordinate";
    check(nc_put_att_text(ncid, varX, "standard_name", std::strlen(standardNameX), standardNameX), "x standard_name");
    check(nc_put_att_text(ncid, varY, "standard_name", std::strlen(standardNameY), standardNameY), "y standard_name");
    if (writeZCoordinate) {
      const char* const standardNameZ = "height";
      check(nc_put_att_text(ncid, varZ, "standard_name", std::strlen(standardNameZ), standardNameZ), "z standard_name");
    }

    configure1DVariable(varX, totalNodes, "x");
    configure1DVariable(varY, totalNodes, "y");
    if (writeZCoordinate) {
      configure1DVariable(varZ, totalNodes, "z");
    }
  }

  // Face-node or volume-node connectivity
  int varElementNodes = -1;
  {
    const int dims2[2] = {dimElement, dimMaxElementNodes};
    check(
      nc_def_var(ncid, elementNodesName, NC_INT, 2, dims2, &varElementNodes),
      std::string("def var ") + elementNodesName
    );
    check(
      nc_put_att_text(ncid, varElementNodes, "cf_role", std::strlen(elementNodesRole), elementNodesRole),
      std::string(elementNodesName) + " cf_role"
    );
    const char* const longName =
      is3DTopology ? "Maps every volume to its corner nodes." : "Maps every face to its corner nodes.";
    check(
      nc_put_att_text(ncid, varElementNodes, "long_name", std::strlen(longName), longName),
      std::string(elementNodesName) + " long_name"
    );
    const int startIndex = 0;
    check(
      nc_put_att_int(ncid, varElementNodes, "start_index", NC_INT, 1, &startIndex),
      std::string(elementNodesName) + " start_index"
    );
    const int fillValueAttribute = static_cast<int>(FillValue);
    check(
      nc_put_att_int(ncid, varElementNodes, "_FillValue", NC_INT, 1, &fillValueAttribute),
      std::string(elementNodesName) + " _FillValue"
    );

    // UGRID-1.0 does not standardise the corner ordering of 3D volume
    // connectivity, so document the convention we write. 2D faces follow the
    // UGRID counter-clockwise face convention.
    const char* const nodeOrdering = is3DTopology
      ? "Corner order follows VTK_HEXAHEDRON (cell type 12): nodes 0-3 form one quadrilateral face "
        "listed counter-clockwise, nodes 4-7 the opposite face in the same orientation, with node 4 "
        "adjacent to node 0, 5 to 1, 6 to 2 and 7 to 3."
      : "Face corner nodes are listed counter-clockwise.";
    check(
      nc_put_att_text(ncid, varElementNodes, "node_ordering", std::strlen(nodeOrdering), nodeOrdering),
      std::string(elementNodesName) + " node_ordering"
    );

    configure2DVariable(varElementNodes, totalElements, static_cast<std::size_t>(maxElementNodes), elementNodesName);
  }

  int varVolumeShapeType = -1;
  if (is3DTopology) {
    const int shapeTypeDims[1] = {dimElement};
    check(
      nc_def_var(ncid, "volume_shape_type", NC_INT, 1, shapeTypeDims, &varVolumeShapeType),
      "def var volume_shape_type"
    );
    const char* const role = "volume_shape_type";
    check(nc_put_att_text(ncid, varVolumeShapeType, "cf_role", std::strlen(role), role), "volume_shape_type cf_role");
    const char* const longName = "Specifies the shape of the individual volumes.";
    check(
      nc_put_att_text(ncid, varVolumeShapeType, "long_name", std::strlen(longName), longName),
      "volume_shape_type long_name"
    );
    const int flagRange[2]  = {0, UgridHexahedronTag};
    const int flagValues[3] = {0, 1, UgridHexahedronTag};
    check(nc_put_att_int(ncid, varVolumeShapeType, "flag_range", NC_INT, 2, flagRange), "volume_shape_type flag_range");
    check(
      nc_put_att_int(ncid, varVolumeShapeType, "flag_values", NC_INT, 3, flagValues),
      "volume_shape_type flag_values"
    );
    const char* const meanings = "tetrahedron wedge hexahedron";
    check(
      nc_put_att_text(ncid, varVolumeShapeType, "flag_meanings", std::strlen(meanings), meanings),
      "volume_shape_type flag_meanings"
    );
    configure1DVariable(varVolumeShapeType, totalElements, "volume_shape_type");
  }

  // Empty ranks were dropped from the writing communicator, so the schema comes from the
  // local reference without any exchange.
  const FieldSchema             schema      = fieldSchemaFromReference(localData.reference);
  const std::vector<DataField>& pointFields = schema.pointFields;
  const std::vector<DataField>& cellFields  = schema.cellFields;

  std::vector<std::vector<int>> pointVarIds(pointFields.size());
  std::vector<std::vector<int>> faceVarIds(cellFields.size());

  const auto defineDataVariable = [&](const DataField& field, const bool onNodes, std::vector<int>& variableIds) {
    const int                dimensionId  = onNodes ? dimNode : dimElement;
    const unsigned long long globalLength = onNodes ? totalNodes : totalElements;

    for (int c = 0; c < field.components; ++c) {
      std::string name = field.name;
      if (field.components > 1) {
        name += "_" + std::to_string(c);
      }

      int       varId              = -1;
      const int fieldDimensions[1] = {dimensionId};
      check(nc_def_var(ncid, name.c_str(), realStorageType, 1, fieldDimensions, &varId), "def var " + name);

      std::string longName = field.name;
      if (field.components > 1) {
        longName += " (component " + std::to_string(c) + ")";
      } else {
        longName += " (scalar)";
      }
      check(
        nc_put_att_text(ncid, varId, "long_name", longName.length(), longName.c_str()),
        "attr long_name for " + name
      );

      check(nc_put_att_text(ncid, varId, "mesh", std::strlen(meshName), meshName), "attr mesh for " + name);

      const char* const location = onNodes ? "node" : elementLocation;
      check(nc_put_att_text(ncid, varId, "location", std::strlen(location), location), "attr location for " + name);

      configure1DVariable(varId, globalLength, name);

      variableIds.push_back(varId);
    }
  };

  for (std::size_t i = 0; i < pointFields.size(); ++i) {
    defineDataVariable(pointFields[i], true, pointVarIds[i]);
  }
  for (std::size_t i = 0; i < cellFields.size(); ++i) {
    defineDataVariable(cellFields[i], false, faceVarIds[i]);
  }

  check(nc_enddef(ncid), "nc_enddef");

  if (useParallelIO) {
    // Collective access (independent deadlocks because the file has an unlimited time
    // dimension). A rank that registered field names but plotted nothing joins with a
    // zero-length count, which the write sites below pass explicitly.
    // The mesh-topology scalar is collective too: if a single rank wrote it independently
    // the ranks' HDF5 metadata caches would diverge, and the collective metadata flush in
    // nc_close then deadlocks, because the ranks end up waiting in different collective
    // operations of the cache synchronisation.
    check(nc_var_par_access(ncid, varMesh, NC_COLLECTIVE), "par_access mesh topology");
    check(nc_var_par_access(ncid, varTime, NC_COLLECTIVE), "par_access time");
    check(nc_var_par_access(ncid, varX, NC_COLLECTIVE), "par_access x");
    check(nc_var_par_access(ncid, varY, NC_COLLECTIVE), "par_access y");
    if (writeZCoordinate) {
      check(nc_var_par_access(ncid, varZ, NC_COLLECTIVE), "par_access z");
    }
    check(nc_var_par_access(ncid, varElementNodes, NC_COLLECTIVE), std::string("par_access ") + elementNodesName);
    if (is3DTopology) {
      check(nc_var_par_access(ncid, varVolumeShapeType, NC_COLLECTIVE), "par_access volume_shape_type");
    }
    for (const auto& ids : pointVarIds) {
      for (const int id : ids) {
        check(nc_var_par_access(ncid, id, NC_COLLECTIVE), "par_access point field");
      }
    }
    for (const auto& ids : faceVarIds) {
      for (const int id : ids) {
        check(nc_var_par_access(ncid, id, NC_COLLECTIVE), "par_access face field");
      }
    }
  }

  // Write time scalar
  {
    // Every rank writes the same single time value, so this collective write has no
    // count-0 participant (a count-0 write to a record variable deadlocks parallel
    // netCDF on some MPI implementations).
    const std::size_t startT[1] = {0};
    const std::size_t countT[1] = {1};
    check(nc_put_vara_double(ncid, varTime, startT, countT, &time), "write time value");
  }

  const int dummyI = 0;

  // Write a defined value for the mesh topology dummy variable. All participating ranks
  // write the same scalar collectively: a master-only write would diverge the ranks'
  // metadata caches and deadlock the collective nc_close (see par_access note above).
  check(nc_put_var_int(ncid, varMesh, &dummyI), "write mesh topology");

  const auto writeVara1DReal = [&](
                                 const int                  varId,
                                 const std::vector<double>& data,
                                 const unsigned long long   offset,
                                 const unsigned long long   count,
                                 const std::string&         where
                               ) {
    const std::size_t start[1]  = {static_cast<std::size_t>(offset)};
    const std::size_t countA[1] = {static_cast<std::size_t>(count)};
    if (writeDouble) {
      const double dummyD = 0.0;
      check(nc_put_vara_double(ncid, varId, start, countA, count > 0 ? data.data() : &dummyD), where);
    } else {
      const std::vector<float> narrowed = narrowToFloat(data);
      const float              dummyF   = 0.0f;
      check(nc_put_vara_float(ncid, varId, start, countA, count > 0 ? narrowed.data() : &dummyF), where);
    }
  };

  // Coordinates: de-interleave each subdomain's XYZ into contiguous per-axis
  // buffers, then one collective write per axis (an empty rank writes count 0).
  std::vector<double> mergedX, mergedY, mergedZ;
  mergedX.reserve(localNumNodes);
  mergedY.reserve(localNumNodes);
  if (writeZCoordinate) {
    mergedZ.reserve(localNumNodes);
  }
  for (const SubdomainBuffers* const subdomain : localData.payloads) {
    const unsigned long long numberOfPoints = subdomain->pointsXYZ.size() / 3;
    for (unsigned long long p = 0; p < numberOfPoints; ++p) {
      mergedX.push_back(subdomain->pointsXYZ[3 * p + 0]);
      mergedY.push_back(subdomain->pointsXYZ[3 * p + 1]);
      if (writeZCoordinate) {
        mergedZ.push_back(subdomain->pointsXYZ[3 * p + 2]);
      }
    }
  }
  writeVara1DReal(varX, mergedX, offsetNodes, localNumNodes, "write x");
  writeVara1DReal(varY, mergedY, offsetNodes, localNumNodes, "write y");
  if (writeZCoordinate) {
    writeVara1DReal(varZ, mergedZ, offsetNodes, localNumNodes, "write z");
  }

  // Connectivity: merge every subdomain's element-node table into one buffer (node
  // indices rebased to global) and write it once.
  {
    std::vector<int> mergedElementNodes;
    mergedElementNodes.reserve(localNumElements * static_cast<std::size_t>(maxElementNodes));
    unsigned long long pointCursor = 0;
    for (const SubdomainBuffers* const subdomain : localData.payloads) {
      appendElementNodes(mergedElementNodes, *subdomain, offsetNodes + pointCursor);
      pointCursor += subdomain->pointsXYZ.size() / 3;
    }
    const int         dummyConnectivity = FillValue;
    const std::size_t start[2]          = {static_cast<std::size_t>(offsetElements), 0};
    const std::size_t count[2]          = {
      static_cast<std::size_t>(localNumElements),
      static_cast<std::size_t>(maxElementNodes)
    };
    check(
      nc_put_vara_int(
        ncid,
        varElementNodes,
        start,
        count,
        localNumElements > 0 ? mergedElementNodes.data() : &dummyConnectivity
      ),
      std::string("write ") + elementNodesName
    );
  }

  if (is3DTopology) {
    const std::vector<int> mergedShapeTypes(localNumElements, UgridHexahedronTag);
    const int              dummyShapeType = UgridHexahedronTag;
    const std::size_t      start[1]       = {static_cast<std::size_t>(offsetElements)};
    const std::size_t      count[1]       = {static_cast<std::size_t>(localNumElements)};
    check(
      nc_put_vara_int(
        ncid,
        varVolumeShapeType,
        start,
        count,
        localNumElements > 0 ? mergedShapeTypes.data() : &dummyShapeType
      ),
      "write volume_shape_type"
    );
  }

  // Field data: gather each component into one contiguous per-rank buffer (the
  // subdomain values are component-major) and write it.
  const auto writeMergedField =
    [&](const std::vector<int>& variableIds, const bool onNodes, const std::size_t fieldIndex) {
      const unsigned long long count  = onNodes ? localNumNodes : localNumElements;
      const unsigned long long offset = onNodes ? offsetNodes : offsetElements;
      const int components = onNodes ? pointFields[fieldIndex].components : cellFields[fieldIndex].components;
      for (int c = 0; c < components; ++c) {
        std::vector<double> buffer;
        buffer.reserve(count);
        for (const SubdomainBuffers* const subdomain : localData.payloads) {
          const DataField&  field = onNodes ? subdomain->pointFields[fieldIndex] : subdomain->cellFields[fieldIndex];
          const std::size_t numberOfTuples = field.values.size() / static_cast<std::size_t>(components);
          for (std::size_t t = 0; t < numberOfTuples; ++t) {
            buffer.push_back(field.values[t * components + c]);
          }
        }
        writeVara1DReal(variableIds[c], buffer, offset, count, "write field");
      }
    };

  for (std::size_t i = 0; i < pointFields.size(); ++i) {
    writeMergedField(pointVarIds[i], true, i);
  }
  for (std::size_t i = 0; i < cellFields.size(); ++i) {
    writeMergedField(faceVarIds[i], false, i);
  }

  ncFileCloser.release();
  check(nc_close(ncid), "nc_close");

  if (ioComm != MPI_COMM_NULL) {
    MPI_Comm_free(&ioComm);
  }

  _written = true;
  return true;
}

#endif
