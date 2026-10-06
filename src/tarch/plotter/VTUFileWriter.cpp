// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "VTUFileWriter.h"

#include "tarch/Termination.h"
#include "tarch/mpi/Rank.h"

#ifdef PEANO_WITH_ZLIB
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/Range.h"

#include <zlib.h>
#endif

namespace {
  constexpr const char* ByteOrder = std::endian::native == std::endian::little ? "LittleEndian" : "BigEndian";

  struct AppendedArray final {
    std::string   type{};
    std::string   name{};
    int           writtenComponents{1};
    const double* values{nullptr};
    int           sourceComponents{1};
    std::size_t   tuples{0};

    const std::byte* data{nullptr};
    std::size_t      bytes{0};

    std::vector<std::uint64_t>                        header{};
    std::vector<tarch::plotter::MeshArray<std::byte>> compressedBlocks{};
  };

  std::uint64_t getAppendedBytes(const AppendedArray& array) {
    std::uint64_t result = array.header.size() * sizeof(std::uint64_t);
    if (array.compressedBlocks.empty()) {
      return result + array.bytes;
    }
    for (const auto& block : array.compressedBlocks) {
      result += block.size();
    }
    return result;
  }

#ifdef PEANO_WITH_ZLIB
  constexpr std::size_t BlockBytes = std::size_t{1} << 16;

  /**
   * Compresses the bytes at data in blocks of BlockBytes bytes in parallel into the zlib format of
   * VTK.
   */
  void compressInBlocks(const std::byte* const data, AppendedArray& array) {
    assertion(array.bytes == 0 or data != nullptr, array.name, array.bytes);
    const std::size_t numberOfBlocks = (array.bytes + BlockBytes - 1) / BlockBytes;
    assertion(numberOfBlocks <= static_cast<std::size_t>(std::numeric_limits<int>::max()), numberOfBlocks);
    array.compressedBlocks.resize(numberOfBlocks);
    tarch::accelerator::parallelForOnHost(
      tarch::accelerator::Range(static_cast<int>(numberOfBlocks)),
      [&](const int block) {
        const std::size_t firstByte       = static_cast<std::size_t>(block) * BlockBytes;
        const std::size_t blockBytes      = std::min(BlockBytes, array.bytes - firstByte);
        auto&             compressedBlock = array.compressedBlocks[static_cast<std::size_t>(block)];
        uLongf            compressedBytes = compressBound(static_cast<uLong>(blockBytes));
        compressedBlock.resize(compressedBytes);
        const int status = compress2(
          reinterpret_cast<Bytef*>(compressedBlock.data()),
          &compressedBytes,
          reinterpret_cast<const Bytef*>(data + firstByte),
          static_cast<uLong>(blockBytes),
          Z_DEFAULT_COMPRESSION
        );
        if (status != Z_OK) {
          tarch::terminateWithError("Zlib cannot compress the plot data '" + array.name + "'.");
        }
        compressedBlock.resize(compressedBytes);
        compressedBlock.shrink_to_fit();
      }
    );
    array.header = {numberOfBlocks, BlockBytes, array.bytes % BlockBytes};
    for (const auto& block : array.compressedBlocks) {
      array.header.push_back(block.size());
    }
  }
#endif
} // namespace

tarch::plotter::Format
tarch::plotter::VTUFileWriter::getFormat() const {
  return Format::VTU;
}

void
tarch::plotter::VTUFileWriter::write(
  const Mesh&        mesh,
  const std::string& outputDirectory,
  const std::string& filename,
  const double       time
) {
  validateMesh(mesh);
  createOutputDirectory(outputDirectory);

  const std::filesystem::path directory(outputDirectory);
  const std::string           snapshotName = getSnapshotName(filename, time);
  const int                   rank         = tarch::mpi::Rank::getInstance().getRank();
  writeVTU(mesh, directory / (snapshotName + "-rank-" + std::to_string(rank) + ".vtu"));

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    const Snapshot snapshot{time, snapshotName + ".pvtu"};
    // getSnapshotName() returns one name for time stamps that agree to six decimals. The series
    // lists that file once.
    if (not _snapshots.empty() and _snapshots.back().file == snapshot.file) {
      _snapshots.back() = snapshot;
    } else {
      _snapshots.push_back(snapshot);
    }
    writePVTU(mesh, directory / (snapshotName + ".pvtu"), snapshotName);
    writePVD(directory / (filename + ".pvd"));
  }
}

void
tarch::plotter::VTUFileWriter::writeVTU(const Mesh& mesh, const std::filesystem::path& filePath) {
  std::ofstream out(filePath, std::ios::binary | std::ios::trunc);
  if (not out) {
    tarch::terminateWithError("The plotter cannot open the file '" + filePath.string() + "'.");
  }

  const bool        writeDouble = getDoublePrecisionOutput();
  const std::string realType    = writeDouble ? "Float64" : "Float32";
  const std::size_t realBytes   = writeDouble ? sizeof(double) : sizeof(float);

  std::vector<AppendedArray> arrays;
  std::size_t                scratchBytes = 0;
  const auto addRealArray = [&](const std::string& name, const int components, const MeshArray<double>& values) {
    assertion(values.size() % static_cast<std::size_t>(components) == 0, name, values.size(), components);
    const int         written = classifyField(components).writtenComponents;
    const std::size_t tuples  = values.size() / static_cast<std::size_t>(components);
    const std::size_t bytes   = tuples * static_cast<std::size_t>(written) * realBytes;
    arrays.push_back({realType, name, written, values.data(), components, tuples, nullptr, bytes});
    if (not writeDouble or components != written) {
      scratchBytes = std::max(scratchBytes, bytes);
    }
  };
  const auto addArray = [&](const std::string& type, const std::string& name, const auto& values) {
    arrays.push_back(
      {type,
       name,
       1,
       nullptr,
       1,
       values.size(),
       reinterpret_cast<const std::byte*>(values.data()),
       values.size() * sizeof(values[0])}
    );
  };
  for (const auto& field : mesh.pointFields) {
    addRealArray(field.name, field.components, field.values);
  }
  for (const auto& field : mesh.cellFields) {
    addRealArray(field.name, field.components, field.values);
  }
  addRealArray("Points", 3, mesh.pointsXYZ);
  addArray("Int32", "connectivity", mesh.connectivity);
  addArray("Int32", "offsets", mesh.offsets);
  addArray("UInt8", "types", mesh.types);

  MeshArray<std::byte> scratch(scratchBytes);
  const auto           getBytes = [&](const AppendedArray& array) {
    if (array.values == nullptr) {
      return array.data;
    }
    if (writeDouble and array.sourceComponents == array.writtenComponents) {
      return reinterpret_cast<const std::byte*>(array.values);
    }
    assertion(array.bytes <= scratch.size(), array.name, array.bytes, scratch.size());
    if (writeDouble) {
      copyTuples(
        array.values,
        array.sourceComponents,
        array.sourceComponents,
        array.writtenComponents,
        array.tuples,
        reinterpret_cast<double*>(scratch.data())
      );
    } else {
      copyTuples(
        array.values,
        array.sourceComponents,
        array.sourceComponents,
        array.writtenComponents,
        array.tuples,
        reinterpret_cast<float*>(scratch.data())
      );
    }
    return static_cast<const std::byte*>(scratch.data());
  };

  for (auto& array : arrays) {
#ifdef PEANO_WITH_ZLIB
    compressInBlocks(getBytes(array), array);
#else
    array.header = {array.bytes};
#endif
  }

  std::string compressorAttribute;
#ifdef PEANO_WITH_ZLIB
  compressorAttribute = " compressor=\"vtkZLibDataCompressor\"";
#endif

  std::uint64_t appendedOffset = 0;
  const auto    writeDataArray = [&](const AppendedArray& array) {
    out << "        <DataArray type=\"" << array.type << "\" Name=\"" << array.name << "\"";
    if (array.writtenComponents > 1) {
      out << " NumberOfComponents=\"" << array.writtenComponents << "\"";
    }
    out << " format=\"appended\" offset=\"" << appendedOffset << "\"/>\n";
    appendedOffset += getAppendedBytes(array);
  };
  const std::size_t numberOfPointFields = mesh.pointFields.size();
  assertion(arrays.size() == numberOfPointFields + mesh.cellFields.size() + 4, arrays.size(), numberOfPointFields);

  out << "<?xml version=\"1.0\"?>\n";
  out << "<VTKFile type=\"UnstructuredGrid\" version=\"1.0\" byte_order=\"" << ByteOrder << "\" header_type=\"UInt64\""
      << compressorAttribute << ">\n";
  out << "  <UnstructuredGrid>\n";
  out << "    <Piece NumberOfPoints=\"" << (mesh.pointsXYZ.size() / 3) << "\" NumberOfCells=\"" << mesh.types.size()
      << "\">\n";
  out << "      <PointData>\n";
  for (std::size_t array = 0; array < numberOfPointFields; array++) {
    writeDataArray(arrays[array]);
  }
  out << "      </PointData>\n";
  out << "      <CellData>\n";
  for (std::size_t array = numberOfPointFields; array < arrays.size() - 4; array++) {
    writeDataArray(arrays[array]);
  }
  out << "      </CellData>\n";
  out << "      <Points>\n";
  writeDataArray(arrays[arrays.size() - 4]);
  out << "      </Points>\n";
  out << "      <Cells>\n";
  for (std::size_t array = arrays.size() - 3; array < arrays.size(); array++) {
    writeDataArray(arrays[array]);
  }
  out << "      </Cells>\n";
  out << "    </Piece>\n";
  out << "  </UnstructuredGrid>\n";

  out << "  <AppendedData encoding=\"raw\">\n";
  out << "_";
  for (const auto& array : arrays) {
    out.write(
      reinterpret_cast<const char*>(array.header.data()),
      static_cast<std::streamsize>(array.header.size() * sizeof(std::uint64_t))
    );
#ifdef PEANO_WITH_ZLIB
    for (const auto& block : array.compressedBlocks) {
      out.write(reinterpret_cast<const char*>(block.data()), static_cast<std::streamsize>(block.size()));
    }
#else
    out.write(reinterpret_cast<const char*>(getBytes(array)), static_cast<std::streamsize>(array.bytes));
#endif
  }
  out << "\n  </AppendedData>\n";
  out << "</VTKFile>\n";
  out.close();
  if (not out) {
    tarch::terminateWithError("The plotter cannot write the file '" + filePath.string() + "'.");
  }
}

void
tarch::plotter::VTUFileWriter::writePVTU(
  const Mesh&                  mesh,
  const std::filesystem::path& filePath,
  const std::string&           snapshotName
) {
  std::ofstream out(filePath);
  if (not out) {
    tarch::terminateWithError("The plotter cannot open the file '" + filePath.string() + "'.");
  }

  const std::string realTypeName = getDoublePrecisionOutput() ? "Float64" : "Float32";

  const auto writeDataArrays = [&](const std::vector<Mesh::Field>& fields) {
    for (const auto& field : fields) {
      const int written = classifyField(field.components).writtenComponents;
      out << "      <PDataArray type=\"" << realTypeName << "\" Name=\"" << field.name << "\"";
      if (written > 1) {
        out << " NumberOfComponents=\"" << written << "\"";
      }
      out << "/>\n";
    }
  };

  out << "<?xml version=\"1.0\"?>\n";
  out << "<VTKFile type=\"PUnstructuredGrid\" version=\"1.0\" byte_order=\"" << ByteOrder
      << "\" header_type=\"UInt64\">\n";
  out << "  <PUnstructuredGrid GhostLevel=\"0\">\n";
  out << "    <PPointData>\n";
  writeDataArrays(mesh.pointFields);
  out << "    </PPointData>\n";
  out << "    <PCellData>\n";
  writeDataArrays(mesh.cellFields);
  out << "    </PCellData>\n";
  out << "    <PPoints>\n";
  out << "      <PDataArray type=\"" << realTypeName << "\" NumberOfComponents=\"3\"/>\n";
  out << "    </PPoints>\n";

  const int numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  for (int rank = 0; rank < numberOfRanks; ++rank) {
    // A reader resolves the Source path relative to the .pvtu file.
    out << "    <Piece Source=\"" << snapshotName << "-rank-" << rank << ".vtu\"/>\n";
  }

  out << "  </PUnstructuredGrid>\n";
  out << "</VTKFile>\n";
  out.close();
  if (not out) {
    tarch::terminateWithError("The plotter cannot write the file '" + filePath.string() + "'.");
  }
}

void
tarch::plotter::VTUFileWriter::writePVD(const std::filesystem::path& filePath) const {
  std::ofstream out(filePath);
  if (not out) {
    tarch::terminateWithError("The plotter cannot open the file '" + filePath.string() + "'.");
  }

  out << "<?xml version=\"1.0\"?>\n"
      << "<VTKFile type=\"Collection\" version=\"0.1\" byte_order=\"LittleEndian\" header_type=\"UInt64\">\n"
      << "  <Collection>\n";

  for (const auto& snapshot : _snapshots) {
    out << R"(    <DataSet timestep=")" << getExactTimeStamp(snapshot.time) << R"(" group="" part="0" file=")"
        << snapshot.file << "\"/>\n";
  }

  out << "  </Collection>\n"
      << "</VTKFile>\n";
  out.close();
  if (not out) {
    tarch::terminateWithError("The plotter cannot write the file '" + filePath.string() + "'.");
  }
}
