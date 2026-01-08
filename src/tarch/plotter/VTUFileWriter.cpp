// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "VTUFileWriter.h"

#include "tarch/mpi/Rank.h"

#include <cassert>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <type_traits>

#ifdef USE_ZLIB
#include <zlib.h>
#endif

namespace fs = std::filesystem;

tarch::plotter::VTUFileWriter::VTUFileWriter(const std::string& outputDirectory, int numberOfSubdomains):
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
  std::string vtuFile = filename + "-rank-" + std::to_string(rank) + ".vtu";
  fs::path    p(_outputDirectory);
  p /= vtuFile;

  std::ofstream out(p, std::ios::binary | std::ios::trunc);
  if (not out) {
    throw std::runtime_error("Failed to open '" + p.string() + "'");
  }

  const auto mergedSubdomains = mergeSubdomains();

  // If all subdomains are empty, write a minimal valid empty VTU and return
  if (mergedSubdomains.pointsXYZ.empty() and mergedSubdomains.types.empty() and mergedSubdomains.pointFields.empty()
      and mergedSubdomains.cellFields.empty()) {
    out
      << "<?xml version=\"1.0\"?>\n"
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
    std::string               xmlHeader;
    std::vector<std::uint8_t> binaryData;
  };
  std::vector<Block> blocks;
  std::size_t        currentFileOffset = 0;

  auto addBlock =
    [&](
      const std::string& tagName,
      const std::string& type,
      const std::string& name,
      int                components,
      const auto&        dataVec
    ) {
      // Do not return early if empty to write the XML header even if the array has size 0
      // if (dataVec.empty()) {
      //   return;
      // }

      Block b;

      // Generate XML Header
      std::ostringstream ss;
      ss << "        <" << tagName << " type=\"" << type << "\" Name=\"" << name << "\"";
      if (components > 1) {
        ss << " NumberOfComponents=\"" << components << "\"";
      }
      ss << " format=\"appended\" offset=\"" << currentFileOffset << "\"/>\n";
      b.xmlHeader = ss.str();

      // Generate binary payload (UInt64 size + raw data)
      using T                        = typename std::decay_t<decltype(dataVec)>::value_type;
      std::uint64_t uncompressedSize = dataVec.size() * sizeof(T);

#ifdef USE_ZLIB
      static_assert(
        sizeof(uLong) >= 8,
        "Critical Safety Error: ZLib's 'uLong' is only 32-bit on this platform (likely Windows). "
        "This makes it impossible to safely compress >4GB buffers without chunking. "
        "Refusing to compile to prevent heap corruption. Disable ZLib in the CMake configuration."
      );

      uLongf destLen = compressBound(uncompressedSize);

      std::uint64_t header[4];
      header[0] = 1;                // Number of blocks
      header[1] = uncompressedSize; // Block size
      header[2] = uncompressedSize; // Last block size

      std::size_t headerBytes = 4 * sizeof(std::uint64_t);

      // Reserve space for Header + Max Compressed Size
      b.binaryData.resize(headerBytes + destLen);

      // Copy the partial header (first 3 uint64s)
      std::memcpy(b.binaryData.data(), header, 3 * sizeof(std::uint64_t));

      int res = compress(
        b.binaryData.data() + headerBytes,
        &destLen,
        reinterpret_cast<const Bytef*>(dataVec.data()),
        uncompressedSize
      );
      if (res != Z_OK) {
        throw std::runtime_error("Zlib compression failed");
      }

      std::uint64_t compressedSize64 = destLen;
      std::memcpy(b.binaryData.data() + 3 * sizeof(std::uint64_t), &compressedSize64, sizeof(std::uint64_t));

      // Shrink vector to actual size (Header + Actual Compressed Size)
      b.binaryData.resize(headerBytes + destLen);
#else
      b.binaryData.reserve(sizeof(std::uint64_t) + uncompressedSize);

      const std::uint8_t* sizePtr = reinterpret_cast<const std::uint8_t*>(&uncompressedSize);
      b.binaryData.insert(b.binaryData.end(), sizePtr, sizePtr + sizeof(std::uint64_t));

      const std::uint8_t* dataPtr = reinterpret_cast<const std::uint8_t*>(dataVec.data());
      b.binaryData.insert(b.binaryData.end(), dataPtr, dataPtr + uncompressedSize);
#endif

      // Update offset and store
      currentFileOffset += b.binaryData.size();
      blocks.push_back(std::move(b));
    };

  // We write raw memory, so we must tell VTK what order the bytes are in.
  const std::uint16_t endianCheck    = 1;
  const bool          isLittleEndian = *reinterpret_cast<const std::uint8_t*>(&endianCheck) == 1;
  const std::string   byteOrder      = isLittleEndian ? "LittleEndian" : "BigEndian";

  std::string compressorAttr = "";
#ifdef USE_ZLIB
  compressorAttr = " compressor=\"vtkZLibDataCompressor\"";
#endif

  // Write file structure
  out << "<?xml version=\"1.0\"?>\n";
  out << "<VTKFile type=\"UnstructuredGrid\" version=\"1.0\" byte_order=\"" << byteOrder << "\" header_type=\"UInt64\""
      << compressorAttr << ">\n";
  out << "  <UnstructuredGrid>\n";
  out << "    <Piece NumberOfPoints=\"" << (mergedSubdomains.pointsXYZ.size() / 3) << "\" NumberOfCells=\""
      << mergedSubdomains.types.size() << "\">\n";

  // -- Point data --
  out << "      <PointData>\n";
  for (const auto& f : mergedSubdomains.pointFields) {
    addBlock("DataArray", "Float32", f.name, f.components, f.values);
    out << blocks.back().xmlHeader;
  }
  out << "      </PointData>\n";

  // -- Cell data --
  out << "      <CellData>\n";
  for (const auto& f : mergedSubdomains.cellFields) {
    addBlock("DataArray", "Float32", f.name, f.components, f.values);
    out << blocks.back().xmlHeader;
  }
  out << "      </CellData>\n";

  // -- Points --
  out << "      <Points>\n";
  addBlock("DataArray", "Float32", "Points", 3, mergedSubdomains.pointsXYZ);
  out << blocks.back().xmlHeader;
  out << "      </Points>\n";

  // -- Cells --
  out << "      <Cells>\n";
  addBlock("DataArray", "Int32", "connectivity", 1, mergedSubdomains.connectivity);
  out << blocks.back().xmlHeader;
  addBlock("DataArray", "Int32", "offsets", 1, mergedSubdomains.offsets);
  out << blocks.back().xmlHeader;
  addBlock("DataArray", "UInt8", "types", 1, mergedSubdomains.types);
  out << blocks.back().xmlHeader;
  out << "      </Cells>\n";

  out << "    </Piece>\n";
  out << "  </UnstructuredGrid>\n";

  // Write raw appended data
  out << "  <AppendedData encoding=\"raw\">\n";
  out << "_"; // Separator

  for (const auto& b : blocks) {
    out.write(reinterpret_cast<const char*>(b.binaryData.data()), b.binaryData.size());
  }

  out << "\n  </AppendedData>\n";
  out << "</VTKFile>\n";
}

void tarch::plotter::VTUFileWriter::writePVTU(const std::string& filename) {
  std::string pvtuFile = filename + ".pvtu";
  fs::path    p(_outputDirectory);
  p /= pvtuFile;

  std::ofstream out(p);
  if (not out) {
    throw std::runtime_error("Failed to open '" + p.string() + "'");
  }

  const std::uint16_t endianCheck = 1;
  const std::string byteOrder = (*reinterpret_cast<const std::uint8_t*>(&endianCheck) == 1) ? "LittleEndian" : "BigEndian";

  const SubdomainBuffers* ref = nullptr;
  for (const auto& s : _subdomains) {
    if (not s.pointFields.empty() or not s.cellFields.empty() or not s.pointsXYZ.empty()) {
      ref = &s;
      break;
    }
  }

  // Write XML structure
  out << "<?xml version=\"1.0\"?>\n";
  out << "<VTKFile type=\"PUnstructuredGrid\" version=\"1.0\" byte_order=\"" << byteOrder
      << "\" header_type=\"UInt64\">\n";
  out << "  <PUnstructuredGrid GhostLevel=\"0\">\n";

  // -- PPointData --
  out << "    <PPointData>\n";
  if (ref) {
    for (const auto& f : ref->pointFields) {
      out << "      <PDataArray type=\"Float32\" Name=\"" << f.name << "\"";
      if (f.components > 1) {
        out << " NumberOfComponents=\"" << f.components << "\"";
      }
      out << "/>\n";
    }
  }
  out << "    </PPointData>\n";

  // -- PCellData --
  out << "    <PCellData>\n";
  if (ref) {
    for (const auto& f : ref->cellFields) {
      out << "      <PDataArray type=\"Float32\" Name=\"" << f.name << "\"";
      if (f.components > 1) {
        out << " NumberOfComponents=\"" << f.components << "\"";
      }
      out << "/>\n";
    }
  }
  out << "    </PCellData>\n";

  // -- PPoints --
  out << "    <PPoints>\n";
  out << "      <PDataArray type=\"Float32\" NumberOfComponents=\"3\"/>\n";
  out << "    </PPoints>\n";

  // -- Pieces --
  int totalRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  for (int r = 0; r < totalRanks; ++r) {
    std::string pieceName = filename + "-rank-" + std::to_string(r) + ".vtu";
    // Source is relative to the .pvtu file, so just the filename
    out << "    <Piece Source=\"" << pieceName << "\"/>\n";
  }

  out << "  </PUnstructuredGrid>\n";
  out << "</VTKFile>\n";
}
