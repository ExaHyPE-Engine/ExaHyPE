// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "VTUFileWriter.h"

#include "tarch/Termination.h"
#include "tarch/mpi/Rank.h"

#ifdef PEANO_WITH_ZLIB
#include <zlib.h>
#endif

tarch::plotter::VTUFileWriter::VTUFileWriter(const std::string& outputDirectory) :
  tarch::plotter::FileWriter(outputDirectory) {}

std::string
tarch::plotter::VTUFileWriter::getFileExtension() const {
  return ".pvtu";
}

bool
tarch::plotter::VTUFileWriter::writeToFile(const std::string& filename, [[maybe_unused]] const double time) {
  if (_written) {
    throw std::runtime_error("Cannot write a plotter file more than once.");
  }

  createOutputDirectory();

  writeVTU(filename);

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    writePVTU(filename);
  }

  _written = true;
  return true;
}

void
tarch::plotter::VTUFileWriter::writeVTU(const std::string& filename) const {
  const auto            rank    = tarch::mpi::Rank::getInstance().getRank();
  const std::string     vtuFile = filename + "-rank-" + std::to_string(rank) + ".vtu";
  std::filesystem::path filePath(_outputDirectory);
  filePath /= vtuFile;

  std::ofstream out(filePath, std::ios::binary | std::ios::trunc);
  if (not out) {
    tarch::terminateWithError("Failed to open '" + filePath.string() + "'.");
  }

  const auto localData = getSubdomainBuffersSequenceForWriting();

  // Every subdomain is empty: The file holds one piece without points and cells.
  if (localData.reference == nullptr) {
    out << "<?xml version=\"1.0\"?>\n"
        << "<VTKFile type=\"UnstructuredGrid\" version=\"1.0\" byte_order=\"LittleEndian\" header_type=\"UInt64\">\n"
        << "  <UnstructuredGrid>\n"
        << "    <Piece NumberOfPoints=\"0\" NumberOfCells=\"0\">\n"
        << "      <Points></Points>\n"
        << "      <Cells></Cells>\n"
        << "    </Piece>\n"
        << "  </UnstructuredGrid>\n"
        << "</VTKFile>\n";
    if (not out) {
      tarch::terminateWithError("Failed to write '" + filePath.string() + "'.");
    }
    return;
  }

  struct Block {
    std::uint64_t             sizeBytes = 0;
    std::vector<std::uint8_t> binaryData;
  };
  std::vector<Block> appendedBlocks;

  std::uint64_t currentFileOffset = 0;

  const auto addBlock = [&](
                          const std::string& tagName,
                          const std::string& type,
                          const std::string& name,
                          const int          components,
                          const auto&        values
                        ) -> std::string {
    std::ostringstream stream;
    stream << "        <" << tagName << " type=\"" << type << "\" Name=\"" << name << "\"";
    if (components > 1) {
      stream << " NumberOfComponents=\"" << components << "\"";
    }
    stream << " format=\"appended\" offset=\"" << currentFileOffset << "\"/>\n";
    const std::string xmlHeader = stream.str();

    using T                       = typename std::decay_t<decltype(values)>::value_type;
    const std::uint64_t dataBytes = static_cast<std::uint64_t>(values.size()) * sizeof(T);

#ifdef PEANO_WITH_ZLIB
    {
      static_assert(
        sizeof(uLong) >= 8,
        "ZLib's 'uLong' holds only 32 bits on this platform, so a buffer above four "
        "gigabytes cannot be compressed in one call. Disable ZLib in the CMake configuration."
      );

      Block block;

      uLongf destinationLength = compressBound(static_cast<uLong>(dataBytes));

      std::uint64_t header[4];
      header[0] = 1;         // Number of blocks
      header[1] = dataBytes; // Block size
      header[2] = dataBytes; // Last block size

      const std::size_t headerBytes = 4 * sizeof(std::uint64_t);

      block.binaryData.resize(headerBytes + static_cast<std::size_t>(destinationLength));
      std::memcpy(block.binaryData.data(), header, 3 * sizeof(std::uint64_t));

      const int status = compress(
        reinterpret_cast<Bytef*>(block.binaryData.data() + headerBytes),
        &destinationLength,
        reinterpret_cast<const Bytef*>(values.data()),
        static_cast<uLong>(dataBytes)
      );
      if (status != Z_OK) {
        tarch::terminateWithError("Zlib compression failed while writing '" + filePath.string() + "'.");
      }

      const std::uint64_t compressedSize = destinationLength;
      std::memcpy(block.binaryData.data() + 3 * sizeof(std::uint64_t), &compressedSize, sizeof(std::uint64_t));

      block.binaryData.resize(headerBytes + static_cast<std::size_t>(destinationLength));

      currentFileOffset += static_cast<std::uint64_t>(block.binaryData.size());
      appendedBlocks.push_back(std::move(block));
      return xmlHeader;
    }
#else
    // The block copies the raw bytes, because expandFieldComponents() returns by value
    // and that temporary dies before the appended-data section is streamed. The length
    // prefix is added at write time.
    Block block;
    block.sizeBytes = dataBytes;
    block.binaryData.resize(static_cast<std::size_t>(dataBytes));
    if (dataBytes > 0) {
      std::memcpy(block.binaryData.data(), values.data(), static_cast<std::size_t>(dataBytes));
    }

    currentFileOffset += sizeof(std::uint64_t) + dataBytes;
    appendedBlocks.push_back(std::move(block));
    return xmlHeader;
#endif
  };

  // Raw memory is written, so VTK must be told the byte order.
  const std::uint16_t endianCheck    = 1;
  const bool          isLittleEndian = *reinterpret_cast<const std::uint8_t*>(&endianCheck) == 1;
  const std::string   byteOrder      = isLittleEndian ? "LittleEndian" : "BigEndian";

  std::string compressorAttribute = "";
#ifdef PEANO_WITH_ZLIB
  compressorAttribute = " compressor=\"vtkZLibDataCompressor\"";
#endif

  // The buffers accumulate in double precision; single-precision output narrows
  // them right before the write.
  const bool writeDouble  = getDoublePrecisionOutput();
  const auto addRealBlock = [&](const std::string& name, const int components, const std::vector<double>& data) {
    return writeDouble ? addBlock("DataArray", "Float64", name, components, data)
                       : addBlock("DataArray", "Float32", name, components, narrowToFloat(data));
  };

  const auto writePiece = [&](const SubdomainBuffers& piece) {
    out << "    <Piece NumberOfPoints=\"" << (piece.pointsXYZ.size() / 3) << "\" NumberOfCells=\"" << piece.types.size()
        << "\">\n";

    // A vector is padded to 3 components and a tensor to 9, as classifyField()
    // prescribes.
    out << "      <PointData>\n";
    for (const auto& field : piece.pointFields) {
      const FieldLayout layout = classifyField(field.components);
      out << addRealBlock(
        field.name,
        layout.writtenComponents,
        expandFieldComponents(field.values, field.components, layout.writtenComponents)
      );
    }
    out << "      </PointData>\n";

    out << "      <CellData>\n";
    for (const auto& field : piece.cellFields) {
      const FieldLayout layout = classifyField(field.components);
      out << addRealBlock(
        field.name,
        layout.writtenComponents,
        expandFieldComponents(field.values, field.components, layout.writtenComponents)
      );
    }
    out << "      </CellData>\n";

    out << "      <Points>\n";
    out << addRealBlock("Points", 3, piece.pointsXYZ);
    out << "      </Points>\n";

    out << "      <Cells>\n";
    out << addBlock("DataArray", "Int32", "connectivity", 1, piece.connectivity);
    out << addBlock("DataArray", "Int32", "offsets", 1, piece.offsets);
    out << addBlock("DataArray", "UInt8", "types", 1, piece.types);
    out << "      </Cells>\n";

    out << "    </Piece>\n";
  };

  out << "<?xml version=\"1.0\"?>\n";
  out << "<VTKFile type=\"UnstructuredGrid\" version=\"1.0\" byte_order=\"" << byteOrder << "\" header_type=\"UInt64\""
      << compressorAttribute << ">\n";
  out << "  <UnstructuredGrid>\n";
  if (localData.payloads.empty()) {
    writePiece(*localData.reference);
  } else {
    for (const SubdomainBuffers* const piece : localData.payloads) {
      writePiece(*piece);
    }
  }
  out << "  </UnstructuredGrid>\n";

  out << "  <AppendedData encoding=\"raw\">\n";
  out << "_"; // Separator

  for (const auto& block : appendedBlocks) {
#ifdef PEANO_WITH_ZLIB
    out.write(reinterpret_cast<const char*>(block.binaryData.data()), block.binaryData.size());
#else
    out.write(reinterpret_cast<const char*>(&block.sizeBytes), sizeof(std::uint64_t));
    if (block.sizeBytes > 0) {
      out.write(reinterpret_cast<const char*>(block.binaryData.data()), block.binaryData.size());
    }
#endif
  }

  out << "\n  </AppendedData>\n";
  out << "</VTKFile>\n";
  if (not out) {
    tarch::terminateWithError("Failed to write '" + filePath.string() + "'.");
  }
}

void
tarch::plotter::VTUFileWriter::writePVTU(const std::string& filename) const {
  const std::string     pvtuFile = filename + ".pvtu";
  std::filesystem::path filePath(_outputDirectory);
  filePath /= pvtuFile;

  std::ofstream out(filePath);
  if (not out) {
    tarch::terminateWithError("Failed to open '" + filePath.string() + "'.");
  }

  const std::uint16_t endianCheck = 1;
  const std::string   byteOrder =
    (*reinterpret_cast<const std::uint8_t*>(&endianCheck) == 1) ? "LittleEndian" : "BigEndian";

  const SubdomainBuffers* const reference    = findReferenceSubdomain();
  const std::string             realTypeName = getDoublePrecisionOutput() ? "Float64" : "Float32";

  out << "<?xml version=\"1.0\"?>\n";
  out << "<VTKFile type=\"PUnstructuredGrid\" version=\"1.0\" byte_order=\"" << byteOrder
      << "\" header_type=\"UInt64\">\n";
  out << "  <PUnstructuredGrid GhostLevel=\"0\">\n";

  out << "    <PPointData>\n";
  if (reference != nullptr) {
    for (const auto& field : reference->pointFields) {
      const int written = classifyField(field.components).writtenComponents;
      out << "      <PDataArray type=\"" << realTypeName << "\" Name=\"" << field.name << "\"";
      if (written > 1) {
        out << " NumberOfComponents=\"" << written << "\"";
      }
      out << "/>\n";
    }
  }
  out << "    </PPointData>\n";

  out << "    <PCellData>\n";
  if (reference != nullptr) {
    for (const auto& field : reference->cellFields) {
      const int written = classifyField(field.components).writtenComponents;
      out << "      <PDataArray type=\"" << realTypeName << "\" Name=\"" << field.name << "\"";
      if (written > 1) {
        out << " NumberOfComponents=\"" << written << "\"";
      }
      out << "/>\n";
    }
  }
  out << "    </PCellData>\n";

  out << "    <PPoints>\n";
  out << "      <PDataArray type=\"" << realTypeName << "\" NumberOfComponents=\"3\"/>\n";
  out << "    </PPoints>\n";

  const int totalRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  for (int rankNumber = 0; rankNumber < totalRanks; ++rankNumber) {
    const std::string pieceName = filename + "-rank-" + std::to_string(rankNumber) + ".vtu";
    // A reader resolves the Source path relative to the .pvtu file.
    out << "    <Piece Source=\"" << pieceName << "\"/>\n";
  }

  out << "  </PUnstructuredGrid>\n";
  out << "</VTKFile>\n";
  if (not out) {
    tarch::terminateWithError("Failed to write '" + filePath.string() + "'.");
  }
}
