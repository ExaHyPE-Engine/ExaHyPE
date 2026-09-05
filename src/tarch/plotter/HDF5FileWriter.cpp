// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "HDF5FileWriter.h"

#ifdef PEANO_WITH_HDF5

#include "tarch/Termination.h"
#include "tarch/plotter/CellTypes.h"
#include "tarch/plotter/HDF5File.h"

tarch::plotter::HDF5FileWriter::HDF5FileWriter(const std::string& outputDirectory) :
  tarch::plotter::FileWriter(outputDirectory) {}

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

  const auto file =
    tarch::plotter::HDF5File::openForDataHoldingRanks(h5FilePath.string(), localData.reference != nullptr);
  if (file == nullptr) {
    _written = true;
    return true; // The data-holding ranks write the file
  }

  const unsigned long long localRows[3] = {localNumberOfPoints, localNumberOfCells, localTopologySize};
  unsigned long long       firstRows[3] = {0ULL, 0ULL, 0ULL};
  unsigned long long       totalRows[3] = {0ULL, 0ULL, 0ULL};
  file->deriveRowRanges(localRows, firstRows, totalRows, 3);
  const unsigned long long offsetPoints   = firstRows[0];
  const unsigned long long offsetCells    = firstRows[1];
  const unsigned long long offsetTopology = firstRows[2];
  const unsigned long long totalPoints    = totalRows[0];
  const unsigned long long totalCells     = totalRows[1];
  const unsigned long long totalTopology  = totalRows[2];

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
          // The /topology dataset is 32-bit and XDMF readers expect that width, so a
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
  const bool writeDouble = getDoublePrecisionOutput();
  const auto realStorageType =
    writeDouble ? tarch::plotter::HDF5File::ElementType::Float64 : tarch::plotter::HDF5File::ElementType::Float32;
  const auto writeRealRows = [&](
                               tarch::plotter::HDF5File::Dataset& dataset,
                               const unsigned long long           firstRow,
                               const unsigned long long           rows,
                               const std::vector<double>&         buffer
                             ) {
    if (writeDouble) {
      dataset.writeRows(firstRow, rows, buffer.data(), tarch::plotter::HDF5File::ElementType::Float64);
    } else {
      const std::vector<float> narrowed = narrowToFloat(buffer);
      dataset.writeRows(firstRow, rows, narrowed.data(), tarch::plotter::HDF5File::ElementType::Float32);
    }
  };

  // The geometry goes into one dataset per axis, so each deflate stream holds one
  // coordinate range.
  {
    tarch::plotter::HDF5File::Dataset datasetX(*file, "/geometry/x", totalPoints, 0, realStorageType);
    tarch::plotter::HDF5File::Dataset datasetY(*file, "/geometry/y", totalPoints, 0, realStorageType);
    tarch::plotter::HDF5File::Dataset datasetZ(*file, "/geometry/z", totalPoints, 0, realStorageType);
    std::vector<double>               coordinatesX;
    std::vector<double>               coordinatesY;
    std::vector<double>               coordinatesZ;
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
    writeRealRows(datasetX, offsetPoints, localNumberOfPoints, coordinatesX);
    writeRealRows(datasetY, offsetPoints, localNumberOfPoints, coordinatesY);
    writeRealRows(datasetZ, offsetPoints, localNumberOfPoints, coordinatesZ);
  }

  {
    tarch::plotter::HDF5File::Dataset
      topologyDataset(*file, "/topology", totalTopology, 0, tarch::plotter::HDF5File::ElementType::Int32);
    std::vector<std::int32_t> mergedTopology;
    mergedTopology.reserve(localTopologySize);
    unsigned long long pointCursor = 0;
    for (const SubdomainBuffers* const subdomain : localData.payloads) {
      appendXdmfTopology(mergedTopology, *subdomain, offsetPoints + pointCursor);
      pointCursor += subdomain->pointsXYZ.size() / 3;
    }
    topologyDataset.writeRows(
      offsetTopology,
      localTopologySize,
      mergedTopology.data(),
      tarch::plotter::HDF5File::ElementType::Int32
    );
  }

  // Every rank of the writing communicator holds data, so the schema comes from the
  // local reference.
  const FieldSchema             schema      = fieldSchemaFromReference(localData.reference);
  const std::vector<DataField>& pointFields = schema.pointFields;
  const std::vector<DataField>& cellFields  = schema.cellFields;

  // One merged collective write per point field. A vector is padded to 3 components
  // and a tensor to 9, as classifyField() prescribes.
  for (std::size_t fieldIndex = 0; fieldIndex < pointFields.size(); ++fieldIndex) {
    const auto&       field  = pointFields[fieldIndex];
    const FieldLayout layout = classifyField(field.components);
    tarch::plotter::HDF5File::Dataset
                        dataset(*file, "/data/" + field.name, totalPoints, layout.writtenComponents, realStorageType);
    std::vector<double> mergedField;
    mergedField.reserve(localNumberOfPoints * static_cast<unsigned long long>(field.components));
    for (const SubdomainBuffers* const subdomain : localData.payloads) {
      const auto& values = subdomain->pointFields[fieldIndex].values;
      mergedField.insert(mergedField.end(), values.begin(), values.end());
    }
    if (layout.writtenComponents != field.components) {
      mergedField = expandFieldComponents(mergedField, field.components, layout.writtenComponents);
    }
    writeRealRows(dataset, offsetPoints, localNumberOfPoints, mergedField);
  }

  for (std::size_t fieldIndex = 0; fieldIndex < cellFields.size(); ++fieldIndex) {
    const auto&       field  = cellFields[fieldIndex];
    const FieldLayout layout = classifyField(field.components);
    tarch::plotter::HDF5File::Dataset
      dataset(*file, "/cell_data/" + field.name, totalCells, layout.writtenComponents, realStorageType);
    std::vector<double> mergedField;
    mergedField.reserve(localNumberOfCells * static_cast<unsigned long long>(field.components));
    for (const SubdomainBuffers* const subdomain : localData.payloads) {
      const auto& values = subdomain->cellFields[fieldIndex].values;
      mergedField.insert(mergedField.end(), values.begin(), values.end());
    }
    if (layout.writtenComponents != field.components) {
      mergedField = expandFieldComponents(mergedField, field.components, layout.writtenComponents);
    }
    writeRealRows(dataset, offsetCells, localNumberOfCells, mergedField);
  }

  if (file->writesTables()) {
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
