// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "NetCDFFileWriter.h"

#ifdef PEANO_WITH_NETCDF

#include "tarch/Assertions.h"
#include "tarch/Termination.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Handles.h"
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/CellTypes.h"

#include <netcdf.h>
#include <netcdf_par.h>

namespace {
  constexpr int          MaxFaceNodes       = 4;
  constexpr int          MaxVolumeNodes     = 8;
  constexpr std::int32_t FillValue          = -1;
  constexpr int          UgridHexahedronTag = 2;
} // namespace

tarch::plotter::Format
tarch::plotter::NetCDFFileWriter::getFormat() const {
  return Format::NetCDF;
}

void
tarch::plotter::NetCDFFileWriter::write(
  const Mesh&        mesh,
  const std::string& outputDirectory,
  const std::string& filename,
  const double       time
) {
  validateMesh(mesh);
  createOutputDirectory(outputDirectory);

  const std::filesystem::path directory(outputDirectory);
  const std::string           snapshotName = getSnapshotName(filename, time);
  const std::filesystem::path filePath     = directory / (snapshotName + ".nc");

  const auto check = [&filePath](const int status) {
    if (status != NC_NOERR) {
      tarch::terminateWithError(
        "The plotter cannot write the file '" + filePath.string() + "'. NetCDF reports \"" + nc_strerror(status) + "\"."
      );
    }
  };

  const unsigned long long localNumberOfNodes    = mesh.pointsXYZ.size() / 3;
  const unsigned long long localNumberOfElements = mesh.types.size();

  std::atomic<bool> facesFound{false};
  std::atomic<bool> volumesFound{false};
  std::atomic<bool> zCoordinateFound{false};
  std::atomic<int>  unsupportedType{-1};
  forEachRangeInParallel(localNumberOfElements, [&](const std::size_t firstCell, const std::size_t endCell) {
    for (std::size_t cell = firstCell; cell < endCell; cell++) {
      const auto type = static_cast<CellType>(mesh.types[cell]);
      if (type == CellType::Quadrangle) {
        facesFound.store(true, std::memory_order_relaxed);
      } else if (type == CellType::Hexahedron) {
        volumesFound.store(true, std::memory_order_relaxed);
      } else {
        unsupportedType.store(static_cast<int>(type), std::memory_order_relaxed);
      }
    }
  });
  forEachRangeInParallel(localNumberOfNodes, [&](const std::size_t firstPoint, const std::size_t endPoint) {
    for (std::size_t point = firstPoint; point < endPoint; point++) {
      if (mesh.pointsXYZ[point * 3 + 2] != 0.0) {
        zCoordinateFound.store(true, std::memory_order_relaxed);
      }
    }
  });
  if (unsupportedType.load() >= 0) {
    tarch::terminateWithError(
      "The NetCDF plotter writes quadrangles and hexahedra only (got the VTK cell type "
      + std::to_string(unsupportedType.load()) + ")."
    );
  }

  const auto&    rank          = tarch::mpi::Rank::getInstance();
  const MPI_Comm comm          = rank.getCommunicator();
  const bool     useParallelIO = rank.getNumberOfRanks() > 1;

  // The allreduce sends the mesh kind to the ranks without cells.
  int found[3] = {facesFound.load() ? 1 : 0, volumesFound.load() ? 1 : 0, zCoordinateFound.load() ? 1 : 0};
  if (useParallelIO) {
    const int localFound[3] = {found[0], found[1], found[2]};
    tarch::mpi::allreduce(localFound, found, 3, MPI_INT, MPI_MAX, comm);
  }
  if (found[0] != 0 and found[1] != 0) {
    tarch::terminateWithError("The plotter cannot write 2D faces and 3D volumes to one NetCDF file.");
  }

  const bool is3DTopology     = found[1] != 0;
  const bool writeZCoordinate = is3DTopology or found[2] != 0;
  const int  maxElementNodes  = is3DTopology ? MaxVolumeNodes : MaxFaceNodes;

  unsigned long long offsetNodes    = 0;
  unsigned long long offsetElements = 0;
  unsigned long long totalNodes     = localNumberOfNodes;
  unsigned long long totalElements  = localNumberOfElements;
  if (useParallelIO) {
    const unsigned long long localCounts[2] = {localNumberOfNodes, localNumberOfElements};
    unsigned long long       prefix[2]      = {0, 0};
    unsigned long long       totals[2]      = {0, 0};
    tarch::mpi::scan(localCounts, prefix, 2, MPI_UNSIGNED_LONG_LONG, MPI_SUM, comm);
    offsetNodes    = prefix[0] - localNumberOfNodes;
    offsetElements = prefix[1] - localNumberOfElements;
    tarch::mpi::allreduce(localCounts, totals, 2, MPI_UNSIGNED_LONG_LONG, MPI_SUM, comm);
    totalNodes    = totals[0];
    totalElements = totals[1];
  }

  constexpr auto LargestIndex = static_cast<unsigned long long>(std::numeric_limits<int>::max());
  if (totalNodes > LargestIndex or totalElements > LargestIndex) {
    tarch::terminateWithError(
      "The NetCDF plotter numbers the points and the cells of a plot with 32-bit integers only."
    );
  }

  int ncid = -1;
  if (useParallelIO) {
    MPI_Info  hints  = createMPIFileHints();
    const int status = nc_create_par(filePath.string().c_str(), NC_NETCDF4 | NC_CLOBBER, comm, hints, &ncid);
    tarch::mpi::freeInfo(&hints);
    check(status);
  } else {
    check(nc_create(filePath.string().c_str(), NC_NETCDF4 | NC_CLOBBER, &ncid));
  }

  // The writes below cover every entry of the payload variables.
  int oldFill = 0;
  check(nc_set_fill(ncid, NC_NOFILL, &oldFill));

  {
    const char* const conventions = "CF-1.10 UGRID-1.0";
    check(nc_put_att_text(ncid, NC_GLOBAL, "Conventions", std::strlen(conventions), conventions));
  }

  const char* const meshName            = is3DTopology ? "mesh3d" : "mesh2d";
  const char* const nodeDimName         = is3DTopology ? "nMesh3d_node" : "nMesh2d_node";
  const char* const elementDimName      = is3DTopology ? "nMesh3d_volume" : "nMesh2d_face";
  const char* const maxElementDimName   = is3DTopology ? "nMaxMesh3d_volume_nodes" : "nMax_face_nodes";
  const char* const elementNodesName    = is3DTopology ? "volume_nodes" : "face_nodes";
  const char* const elementNodesRole    = is3DTopology ? "volume_node_connectivity" : "face_node_connectivity";
  const char* const elementDimensionAtt = is3DTopology ? "volume_dimension" : "face_dimension";
  const char* const elementLocation     = is3DTopology ? "volume" : "face";

  constexpr std::size_t ChunkByteTarget    = 1U << 20;
  const bool            writeDouble        = getDoublePrecisionOutput();
  const nc_type         realStorageType    = writeDouble ? NC_DOUBLE : NC_FLOAT;
  const std::size_t     bytesPerBulkScalar = writeDouble ? sizeof(double) : sizeof(float);

  const auto configureVariable =
    [&](const int variableId, const unsigned long long globalRows, const std::size_t columns) {
      if (globalRows == 0) {
        return;
      }

      const std::size_t bytesPerRow = std::max<std::size_t>(1, columns * bytesPerBulkScalar);
      const std::size_t targetRows  = std::max<std::size_t>(1, ChunkByteTarget / bytesPerRow);
      const std::size_t chunkRows =
        std::max<std::size_t>(1, std::min<std::size_t>(static_cast<std::size_t>(globalRows), targetRows));
      const std::size_t chunks[2] = {chunkRows, columns};
      check(nc_def_var_chunking(ncid, variableId, NC_CHUNKED, chunks));
      check(nc_def_var_deflate(ncid, variableId, 1, 1, 1));
      nc_set_var_chunk_cache(
        ncid,
        variableId,
        std::max<std::size_t>(chunkRows * bytesPerRow, ChunkByteTarget),
        1009,
        0.75F
      );
    };

  int dimOne = -1;
  check(nc_def_dim(ncid, "one", 1, &dimOne));
  int dimTime = -1;
  check(nc_def_dim(ncid, "time", NC_UNLIMITED, &dimTime));
  int dimNode = -1;
  check(nc_def_dim(ncid, nodeDimName, static_cast<std::size_t>(totalNodes), &dimNode));
  int dimElement = -1;
  check(nc_def_dim(ncid, elementDimName, static_cast<std::size_t>(totalElements), &dimElement));
  int dimMaxElementNodes = -1;
  check(nc_def_dim(ncid, maxElementDimName, maxElementNodes, &dimMaxElementNodes));

  // Parallel netCDF-4 stores an uncompressed dimension scale of full size for a dimension without a
  // variable of its name.
  int varNodeIndex = -1;
  check(nc_def_var(ncid, nodeDimName, NC_INT, 1, &dimNode, &varNodeIndex));
  configureVariable(varNodeIndex, totalNodes, 1);
  int varElementIndex = -1;
  check(nc_def_var(ncid, elementDimName, NC_INT, 1, &dimElement, &varElementIndex));
  configureVariable(varElementIndex, totalElements, 1);

  int varMesh = -1;
  {
    const int meshDims[1] = {dimOne};
    check(nc_def_var(ncid, meshName, NC_INT, 1, meshDims, &varMesh));
    const char* const role = "mesh_topology";
    check(nc_put_att_text(ncid, varMesh, "cf_role", std::strlen(role), role));
    const int topologyDimension = is3DTopology ? 3 : 2;
    check(nc_put_att_int(ncid, varMesh, "topology_dimension", NC_INT, 1, &topologyDimension));
    const char* const nodeCoords = writeZCoordinate ? "x y z" : "x y";
    check(nc_put_att_text(ncid, varMesh, "node_coordinates", std::strlen(nodeCoords), nodeCoords));
    check(nc_put_att_text(ncid, varMesh, elementNodesRole, std::strlen(elementNodesName), elementNodesName));
    check(nc_put_att_text(ncid, varMesh, elementDimensionAtt, std::strlen(elementDimName), elementDimName));
    if (is3DTopology) {
      const char* const volumeShapeType = "volume_shape_type";
      check(nc_put_att_text(ncid, varMesh, "volume_shape_type", std::strlen(volumeShapeType), volumeShapeType));
    }
  }

  int varTime = -1;
  {
    const int timeDims[1] = {dimTime};
    check(nc_def_var(ncid, "time", NC_DOUBLE, 1, timeDims, &varTime));
    const char* const units = "s";
    check(nc_put_att_text(ncid, varTime, "units", std::strlen(units), units));
    const char* const standardName = "time";
    check(nc_put_att_text(ncid, varTime, "standard_name", std::strlen(standardName), standardName));
  }

  int varX = -1;
  int varY = -1;
  int varZ = -1;
  {
    const int nodeDimensions[1] = {dimNode};
    check(nc_def_var(ncid, "x", realStorageType, 1, nodeDimensions, &varX));
    check(nc_def_var(ncid, "y", realStorageType, 1, nodeDimensions, &varY));
    if (writeZCoordinate) {
      check(nc_def_var(ncid, "z", realStorageType, 1, nodeDimensions, &varZ));
    }

    const char* const standardNameX = "projection_x_coordinate";
    const char* const standardNameY = "projection_y_coordinate";
    check(nc_put_att_text(ncid, varX, "standard_name", std::strlen(standardNameX), standardNameX));
    check(nc_put_att_text(ncid, varY, "standard_name", std::strlen(standardNameY), standardNameY));
    if (writeZCoordinate) {
      const char* const standardNameZ = "height";
      check(nc_put_att_text(ncid, varZ, "standard_name", std::strlen(standardNameZ), standardNameZ));
    }

    configureVariable(varX, totalNodes, 1);
    configureVariable(varY, totalNodes, 1);
    if (writeZCoordinate) {
      configureVariable(varZ, totalNodes, 1);
    }
  }

  int varElementNodes = -1;
  {
    const int elementNodeDimensions[2] = {dimElement, dimMaxElementNodes};
    check(nc_def_var(ncid, elementNodesName, NC_INT, 2, elementNodeDimensions, &varElementNodes));
    check(nc_put_att_text(ncid, varElementNodes, "cf_role", std::strlen(elementNodesRole), elementNodesRole));
    const char* const longName =
      is3DTopology ? "Maps every volume to its corner nodes." : "Maps every face to its corner nodes.";
    check(nc_put_att_text(ncid, varElementNodes, "long_name", std::strlen(longName), longName));
    const int startIndex = 0;
    check(nc_put_att_int(ncid, varElementNodes, "start_index", NC_INT, 1, &startIndex));
    const int fillValueAttribute = static_cast<int>(FillValue);
    check(nc_put_att_int(ncid, varElementNodes, "_FillValue", NC_INT, 1, &fillValueAttribute));

    // UGRID-1.0 fixes the corner order of a 2D face. The node_ordering attribute records the corner
    // order of a 3D volume.
    const char* const nodeOrdering = is3DTopology
      ? "Corner order follows VTK_HEXAHEDRON (cell type 12): nodes 0-3 form one quadrilateral face "
        "listed counter-clockwise, nodes 4-7 the opposite face in the same orientation, with node 4 "
        "adjacent to node 0, 5 to 1, 6 to 2 and 7 to 3."
      : "Face corner nodes are listed counter-clockwise.";
    check(nc_put_att_text(ncid, varElementNodes, "node_ordering", std::strlen(nodeOrdering), nodeOrdering));

    configureVariable(varElementNodes, totalElements, static_cast<std::size_t>(maxElementNodes));
  }

  int varVolumeShapeType = -1;
  if (is3DTopology) {
    const int shapeTypeDims[1] = {dimElement};
    check(nc_def_var(ncid, "volume_shape_type", NC_INT, 1, shapeTypeDims, &varVolumeShapeType));
    const char* const role = "volume_shape_type";
    check(nc_put_att_text(ncid, varVolumeShapeType, "cf_role", std::strlen(role), role));
    const char* const longName = "Specifies the shape of the individual volumes.";
    check(nc_put_att_text(ncid, varVolumeShapeType, "long_name", std::strlen(longName), longName));
    const int flagRange[2]  = {0, UgridHexahedronTag};
    const int flagValues[3] = {0, 1, UgridHexahedronTag};
    check(nc_put_att_int(ncid, varVolumeShapeType, "flag_range", NC_INT, 2, flagRange));
    check(nc_put_att_int(ncid, varVolumeShapeType, "flag_values", NC_INT, 3, flagValues));
    const char* const meanings = "tetrahedron wedge hexahedron";
    check(nc_put_att_text(ncid, varVolumeShapeType, "flag_meanings", std::strlen(meanings), meanings));
    configureVariable(varVolumeShapeType, totalElements, 1);
  }

  // Lists one variable per component of every point field and then of every cell field.
  std::vector<int> fieldVariables;
  const auto       defineFieldVariables = [&](const std::vector<Mesh::Field>& fields, const bool onNodes) {
    for (const auto& field : fields) {
      for (int component = 0; component < field.components; ++component) {
        std::string name = field.name;
        if (field.components > 1) {
          name += "_" + std::to_string(component);
        }

        int       varId              = -1;
        const int fieldDimensions[1] = {onNodes ? dimNode : dimElement};
        check(nc_def_var(ncid, name.c_str(), realStorageType, 1, fieldDimensions, &varId));

        std::string longName = field.name;
        if (field.components > 1) {
          longName += " (component " + std::to_string(component) + ")";
        } else {
          longName += " (scalar)";
        }
        check(nc_put_att_text(ncid, varId, "long_name", longName.length(), longName.c_str()));
        check(nc_put_att_text(ncid, varId, "mesh", std::strlen(meshName), meshName));
        const char* const location = onNodes ? "node" : elementLocation;
        check(nc_put_att_text(ncid, varId, "location", std::strlen(location), location));
        configureVariable(varId, onNodes ? totalNodes : totalElements, 1);
        fieldVariables.push_back(varId);
      }
    }
  };
  defineFieldVariables(mesh.pointFields, true);
  defineFieldVariables(mesh.cellFields, false);

  check(nc_enddef(ncid));

  if (useParallelIO) {
    // Parallel netCDF deadlocks with independent access to the unlimited time dimension and in the
    // metadata flush of nc_close(). A rank without values joins every collective write with count 0.
    check(nc_var_par_access(ncid, varMesh, NC_COLLECTIVE));
    check(nc_var_par_access(ncid, varTime, NC_COLLECTIVE));
    check(nc_var_par_access(ncid, varNodeIndex, NC_COLLECTIVE));
    check(nc_var_par_access(ncid, varElementIndex, NC_COLLECTIVE));
    check(nc_var_par_access(ncid, varX, NC_COLLECTIVE));
    check(nc_var_par_access(ncid, varY, NC_COLLECTIVE));
    if (writeZCoordinate) {
      check(nc_var_par_access(ncid, varZ, NC_COLLECTIVE));
    }
    check(nc_var_par_access(ncid, varElementNodes, NC_COLLECTIVE));
    if (is3DTopology) {
      check(nc_var_par_access(ncid, varVolumeShapeType, NC_COLLECTIVE));
    }
    for (const int variable : fieldVariables) {
      check(nc_var_par_access(ncid, variable, NC_COLLECTIVE));
    }
  }

  {
    // Every rank writes the same time value. A collective write with count 0 to a record variable
    // deadlocks parallel netCDF on some MPI implementations.
    const std::size_t start[1] = {0};
    const std::size_t count[1] = {1};
    check(nc_put_vara_double(ncid, varTime, start, count, &time));
  }

  // Every rank writes the same scalar into the mesh topology variable.
  const int meshTopologyValue = 0;
  check(nc_put_var_int(ncid, varMesh, &meshTopologyValue));

  const std::size_t realBytes    = writeDouble ? sizeof(double) : sizeof(float);
  const std::size_t scratchBytes = std::max<std::size_t>(
    std::max<std::size_t>(localNumberOfNodes, localNumberOfElements) * realBytes,
    localNumberOfElements * static_cast<std::size_t>(maxElementNodes) * sizeof(int)
  );
  MeshArray<std::byte> scratch(scratchBytes);

  const auto writeIndices = [&](const int varId, const unsigned long long offset, const unsigned long long count) {
    auto* const indices = reinterpret_cast<int*>(scratch.data());
    forEachRangeInParallel(count, [&](const std::size_t firstIndex, const std::size_t endIndex) {
      std::iota(indices + firstIndex, indices + endIndex, static_cast<int>(offset + firstIndex));
    });
    const int         placeholder   = 0;
    const std::size_t start[1]      = {static_cast<std::size_t>(offset)};
    const std::size_t writeCount[1] = {static_cast<std::size_t>(count)};
    check(nc_put_vara_int(ncid, varId, start, writeCount, count > 0 ? indices : &placeholder));
  };
  writeIndices(varNodeIndex, offsetNodes, localNumberOfNodes);
  writeIndices(varElementIndex, offsetElements, localNumberOfElements);

  // Writes one component of the tuples at values to the rows from offset on.
  const auto writeComponent = [&](
                                const int                varId,
                                const double* const      values,
                                const int                stride,
                                const int                component,
                                const unsigned long long offset,
                                const unsigned long long tuples
                              ) {
    const std::size_t start[1] = {static_cast<std::size_t>(offset)};
    const std::size_t count[1] = {static_cast<std::size_t>(tuples)};
    if (writeDouble and stride == 1) {
      const double placeholder = 0.0;
      check(nc_put_vara_double(ncid, varId, start, count, tuples > 0 ? values : &placeholder));
    } else if (writeDouble) {
      auto* const target = reinterpret_cast<double*>(scratch.data());
      copyTuples(tuples > 0 ? values + component : nullptr, stride, 1, 1, tuples, target);
      check(nc_put_vara_double(ncid, varId, start, count, target));
    } else {
      auto* const target = reinterpret_cast<float*>(scratch.data());
      copyTuples(tuples > 0 ? values + component : nullptr, stride, 1, 1, tuples, target);
      check(nc_put_vara_float(ncid, varId, start, count, target));
    }
  };

  writeComponent(varX, mesh.pointsXYZ.data(), 3, 0, offsetNodes, localNumberOfNodes);
  writeComponent(varY, mesh.pointsXYZ.data(), 3, 1, offsetNodes, localNumberOfNodes);
  if (writeZCoordinate) {
    writeComponent(varZ, mesh.pointsXYZ.data(), 3, 2, offsetNodes, localNumberOfNodes);
  }

  {
    auto* const elementNodes = reinterpret_cast<int*>(scratch.data());
    forEachRangeInParallel(localNumberOfElements, [&](const std::size_t firstCell, const std::size_t endCell) {
      for (std::size_t cell = firstCell; cell < endCell; cell++) {
        int* const         nodesOfCell = elementNodes + cell * static_cast<std::size_t>(maxElementNodes);
        const std::int32_t firstNode   = cell == 0 ? 0 : mesh.offsets[cell - 1];
        const std::int32_t endNode     = mesh.offsets[cell];
        ASSERTION(endNode - firstNode == maxElementNodes, cell, firstNode, endNode, maxElementNodes);
        for (std::int32_t node = firstNode; node < endNode; node++) {
          const std::int32_t point = mesh.connectivity[static_cast<std::size_t>(node)];
          ASSERTION(point >= 0 and static_cast<unsigned long long>(point) < localNumberOfNodes, cell, point);
          nodesOfCell[node - firstNode] = static_cast<int>(offsetNodes + static_cast<unsigned long long>(point));
        }
      }
    });
    const int         placeholderConnectivity = FillValue;
    const std::size_t start[2]                = {static_cast<std::size_t>(offsetElements), 0};
    const std::size_t count[2]                = {
      static_cast<std::size_t>(localNumberOfElements),
      static_cast<std::size_t>(maxElementNodes)
    };
    check(nc_put_vara_int(
      ncid,
      varElementNodes,
      start,
      count,
      localNumberOfElements > 0 ? elementNodes : &placeholderConnectivity
    ));
  }

  if (is3DTopology) {
    auto* const shapeTypes = reinterpret_cast<int*>(scratch.data());
    forEachRangeInParallel(localNumberOfElements, [&](const std::size_t firstCell, const std::size_t endCell) {
      std::fill(shapeTypes + firstCell, shapeTypes + endCell, UgridHexahedronTag);
    });
    const int         placeholderShapeType = UgridHexahedronTag;
    const std::size_t start[1]             = {static_cast<std::size_t>(offsetElements)};
    const std::size_t count[1]             = {static_cast<std::size_t>(localNumberOfElements)};
    check(nc_put_vara_int(
      ncid,
      varVolumeShapeType,
      start,
      count,
      localNumberOfElements > 0 ? shapeTypes : &placeholderShapeType
    ));
  }

  std::size_t variable = 0;
  const auto  writeFields =
    [&](const std::vector<Mesh::Field>& fields, const unsigned long long offset, const unsigned long long tuples) {
      for (const auto& field : fields) {
        for (int component = 0; component < field.components; ++component) {
          writeComponent(fieldVariables[variable], field.values.data(), field.components, component, offset, tuples);
          variable++;
        }
      }
    };
  writeFields(mesh.pointFields, offsetNodes, localNumberOfNodes);
  writeFields(mesh.cellFields, offsetElements, localNumberOfElements);
  ASSERTION(variable == fieldVariables.size(), variable, fieldVariables.size());

  check(nc_close(ncid));

  if (rank.isGlobalMaster()) {
    const Snapshot snapshot{time, snapshotName + ".nc"};
    // getSnapshotName() returns one name for time stamps that agree to six decimals. The series
    // lists that file once.
    if (not _snapshots.empty() and _snapshots.back().file == snapshot.file) {
      _snapshots.back() = snapshot;
    } else {
      _snapshots.push_back(snapshot);
    }
    writeSeries(directory / (filename + ".nc.series"));
  }
}

void
tarch::plotter::NetCDFFileWriter::writeSeries(const std::filesystem::path& filePath) const {
  std::ofstream out(filePath);
  if (not out) {
    tarch::terminateWithError("The plotter cannot open the file '" + filePath.string() + "'.");
  }

  out << "{\n"
      << "  \"file-series-version\" : \"1.0\",\n"
      << "  \"files\" : [\n";
  for (std::size_t snapshot = 0; snapshot < _snapshots.size(); ++snapshot) {
    out << R"(    { "name" : ")" << _snapshots[snapshot].file << R"(", "time" : )"
        << getExactTimeStamp(_snapshots[snapshot].time) << " }";
    if (snapshot + 1 < _snapshots.size()) {
      out << ",";
    }
    out << "\n";
  }
  out << "  ]\n"
      << "}\n";
  out.close();
  if (not out) {
    tarch::terminateWithError("The plotter cannot write the file '" + filePath.string() + "'.");
  }
}

#endif
