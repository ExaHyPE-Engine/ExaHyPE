// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "VTUFileWriter.h"

#include "tarch/mpi/Rank.h"

#ifdef PEANO_WITH_ZLIB
#include <zlib.h>
#endif

tarch::plotter::VTUFileWriter::VTUFileWriter(const std::string& outputDirectory, int numberOfSubdomains) :
  FileWriter(outputDirectory, numberOfSubdomains) {}

std::string tarch::plotter::VTUFileWriter::getFileExtension() const { return ".pvtu"; }

bool tarch::plotter::VTUFileWriter::writeToFile(const std::string& filename, double time) {
  if (_written) {
    throw std::runtime_error("Cannot write to file. File already written.");
  }

  createOutputDirectory();

  writeVTU(filename);

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    writePVTU(filename);
  }

  _written = true;
  return true;
}

void tarch::plotter::VTUFileWriter::writeVTU(const std::string& filename) {
  const auto  rank    = tarch::mpi::Rank::getInstance().getRank();
  std::string           vtuFile = filename + "-rank-" + std::to_string(rank) + ".vtu";
  std::filesystem::path p(_outputDirectory);
  p /= vtuFile;

  std::ofstream out(p, std::ios::binary | std::ios::trunc);
  if (not out) {
    throw std::runtime_error("Failed to open '" + p.string() + "'");
  }

  const auto localData = getSubdomainBuffersSequenceForWriting();

  // All subdomains empty: write a minimal valid empty VTU and return
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
    return;
  }

  struct Block {
    std::uint64_t             sizeBytes = 0;
    std::vector<std::uint8_t> binaryData;
  };
  std::vector<Block> appendedBlocks;

  std::uint64_t currentFileOffset = 0;

  auto addBlock = [&](
                    const std::string& tagName,
                    const std::string& type,
                    const std::string& name,
                    int                components,
                    const auto&        dataVec
                  ) -> std::string {
    std::ostringstream ss;
    ss << "        <" << tagName << " type=\"" << type << "\" Name=\"" << name << "\"";
    if (components > 1) {
      ss << " NumberOfComponents=\"" << components << "\"";
    }
    ss << " format=\"appended\" offset=\"" << currentFileOffset << "\"/>\n";
    const std::string xmlHeader = ss.str();

    using T                       = typename std::decay_t<decltype(dataVec)>::value_type;
    const std::uint64_t dataBytes = static_cast<std::uint64_t>(dataVec.size()) * sizeof(T);

#ifdef PEANO_WITH_ZLIB
    {
      static_assert(
        sizeof(uLong) >= 8,
        "ZLib's 'uLong' is only 32-bit on this platform (likely Windows). "
        "This makes it impossible to safely compress >4GB buffers without chunking. "
        "Refusing to compile to prevent heap corruption. Disable ZLib in the CMake configuration."
      );

      Block b;

      uLongf destLen = compressBound(static_cast<uLong>(dataBytes));

      std::uint64_t header[4];
      header[0] = 1;         // Number of blocks
      header[1] = dataBytes; // Block size
      header[2] = dataBytes; // Last block size

      const std::size_t headerBytes = 4 * sizeof(std::uint64_t);

      b.binaryData.resize(headerBytes + static_cast<std::size_t>(destLen));
      std::memcpy(b.binaryData.data(), header, 3 * sizeof(std::uint64_t));

      int res = compress(
        reinterpret_cast<Bytef*>(b.binaryData.data() + headerBytes),
        &destLen,
        reinterpret_cast<const Bytef*>(dataVec.data()),
        static_cast<uLong>(dataBytes)
      );
      if (res != Z_OK) {
        throw std::runtime_error("Zlib compression failed");
      }

      std::uint64_t compressedSize64 = destLen;
      std::memcpy(b.binaryData.data() + 3 * sizeof(std::uint64_t), &compressedSize64, sizeof(std::uint64_t));

      b.binaryData.resize(headerBytes + static_cast<std::size_t>(destLen));

      currentFileOffset += static_cast<std::uint64_t>(b.binaryData.size());
      appendedBlocks.push_back(std::move(b));
      return xmlHeader;
    }
#else
    // The block copies the raw bytes because dataVec may be a temporary
    // (expandFieldComponents returns by value) destroyed before the appended-data
    // section is streamed; the length prefix is added at write time.
    Block b;
    b.sizeBytes = dataBytes;
    b.binaryData.resize(static_cast<std::size_t>(dataBytes));
    if (dataBytes > 0) {
      std::memcpy(b.binaryData.data(), dataVec.data(), static_cast<std::size_t>(dataBytes));
    }

    currentFileOffset += sizeof(std::uint64_t) + dataBytes;
    appendedBlocks.push_back(std::move(b));
    return xmlHeader;
#endif
  };

  // Raw memory is written, so VTK must be told the byte order.
  const std::uint16_t endianCheck    = 1;
  const bool          isLittleEndian = *reinterpret_cast<const std::uint8_t*>(&endianCheck) == 1;
  const std::string   byteOrder      = isLittleEndian ? "LittleEndian" : "BigEndian";

  std::string compressorAttr = "";
#ifdef PEANO_WITH_ZLIB
  compressorAttr = " compressor=\"vtkZLibDataCompressor\"";
#endif

  // The buffers accumulate in double precision; single-precision output narrows
  // them right before the write.
  const bool writeDouble  = getDoublePrecisionOutput();
  auto       addRealBlock = [&](const std::string& name, int components, const std::vector<double>& data) {
    return writeDouble ? addBlock("DataArray", "Float64", name, components, data)
                             : addBlock("DataArray", "Float32", name, components, narrowToFloat(data));
  };

  auto writePiece = [&](const SubdomainBuffers& piece) {
    out << "    <Piece NumberOfPoints=\"" << (piece.pointsXYZ.size() / 3) << "\" NumberOfCells=\"" << piece.types.size()
        << "\">\n";

    // -- Point data -- (vectors padded to 3 components, tensors to 9, so they
    // read back as real vectors / tensors; see FileWriter::classifyField)
    out << "      <PointData>\n";
    for (const auto& f : piece.pointFields) {
      const FieldLayout layout = classifyField(f.components);
      out << addRealBlock(
        f.name,
        layout.writtenComponents,
        expandFieldComponents(f.values, f.components, layout.writtenComponents)
      );
    }
    out << "      </PointData>\n";

    // -- Cell data --
    out << "      <CellData>\n";
    for (const auto& f : piece.cellFields) {
      const FieldLayout layout = classifyField(f.components);
      out << addRealBlock(
        f.name,
        layout.writtenComponents,
        expandFieldComponents(f.values, f.components, layout.writtenComponents)
      );
    }
    out << "      </CellData>\n";

    // -- Points --
    out << "      <Points>\n";
    out << addRealBlock("Points", 3, piece.pointsXYZ);
    out << "      </Points>\n";

    // -- Cells --
    out << "      <Cells>\n";
    out << addBlock("DataArray", "Int32", "connectivity", 1, piece.connectivity);
    out << addBlock("DataArray", "Int32", "offsets", 1, piece.offsets);
    out << addBlock("DataArray", "UInt8", "types", 1, piece.types);
    out << "      </Cells>\n";

    out << "    </Piece>\n";
  };

  out << "<?xml version=\"1.0\"?>\n";
  out << "<VTKFile type=\"UnstructuredGrid\" version=\"1.0\" byte_order=\"" << byteOrder << "\" header_type=\"UInt64\""
      << compressorAttr << ">\n";
  out << "  <UnstructuredGrid>\n";
  if (localData.payloads.empty()) {
    writePiece(*localData.reference);
  } else {
    for (const SubdomainBuffers* piece : localData.payloads) {
      writePiece(*piece);
    }
  }
  out << "  </UnstructuredGrid>\n";

  out << "  <AppendedData encoding=\"raw\">\n";
  out << "_"; // Separator

  for (const auto& b : appendedBlocks) {
#ifdef PEANO_WITH_ZLIB
    out.write(reinterpret_cast<const char*>(b.binaryData.data()), b.binaryData.size());
#else
    out.write(reinterpret_cast<const char*>(&b.sizeBytes), sizeof(std::uint64_t));
    if (b.sizeBytes > 0) {
      out.write(reinterpret_cast<const char*>(b.binaryData.data()), b.binaryData.size());
    }
#endif
  }

  out << "\n  </AppendedData>\n";
  out << "</VTKFile>\n";
}

void tarch::plotter::VTUFileWriter::writePVTU(const std::string& filename) {
  std::string           pvtuFile = filename + ".pvtu";
  std::filesystem::path p(_outputDirectory);
  p /= pvtuFile;

  std::ofstream out(p);
  if (not out) {
    throw std::runtime_error("Failed to open '" + p.string() + "'");
  }

  const std::uint16_t endianCheck = 1;
  const std::string   byteOrder =
    (*reinterpret_cast<const std::uint8_t*>(&endianCheck) == 1) ? "LittleEndian" : "BigEndian";

  const SubdomainBuffers* ref          = findReferenceSubdomain();
  const std::string       realTypeName = getDoublePrecisionOutput() ? "Float64" : "Float32";

  out << "<?xml version=\"1.0\"?>\n";
  out << "<VTKFile type=\"PUnstructuredGrid\" version=\"1.0\" byte_order=\"" << byteOrder
      << "\" header_type=\"UInt64\">\n";
  out << "  <PUnstructuredGrid GhostLevel=\"0\">\n";

  // -- PPointData --
  out << "    <PPointData>\n";
  if (ref) {
    for (const auto& f : ref->pointFields) {
      const int written = classifyField(f.components).writtenComponents;
      out << "      <PDataArray type=\"" << realTypeName << "\" Name=\"" << f.name << "\"";
      if (written > 1) {
        out << " NumberOfComponents=\"" << written << "\"";
      }
      out << "/>\n";
    }
  }
  out << "    </PPointData>\n";

  // -- PCellData --
  out << "    <PCellData>\n";
  if (ref) {
    for (const auto& f : ref->cellFields) {
      const int written = classifyField(f.components).writtenComponents;
      out << "      <PDataArray type=\"" << realTypeName << "\" Name=\"" << f.name << "\"";
      if (written > 1) {
        out << " NumberOfComponents=\"" << written << "\"";
      }
      out << "/>\n";
    }
  }
  out << "    </PCellData>\n";

  // -- PPoints --
  out << "    <PPoints>\n";
  out << "      <PDataArray type=\"" << realTypeName << "\" NumberOfComponents=\"3\"/>\n";
  out << "    </PPoints>\n";

  // -- Pieces --
  int totalRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  for (int r = 0; r < totalRanks; ++r) {
    std::string pieceName = filename + "-rank-" + std::to_string(r) + ".vtu";
    // Source is relative to the .pvtu file, so use just the filename
    out << "    <Piece Source=\"" << pieceName << "\"/>\n";
  }

  out << "  </PUnstructuredGrid>\n";
  out << "</VTKFile>\n";
}
