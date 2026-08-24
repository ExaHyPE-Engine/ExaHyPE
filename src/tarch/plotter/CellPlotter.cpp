// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CellTypes.h"
#include "FileWriter.h"

namespace {
  // The public plotXXX(int[N]) signatures promise a fixed corner count; lock
  // those promises to the CellTypes table so the two cannot drift.
  static_assert(tarch::plotter::nodesOf(tarch::plotter::CellType::Vertex) == 1);
  static_assert(tarch::plotter::nodesOf(tarch::plotter::CellType::Line) == 2);
  static_assert(tarch::plotter::nodesOf(tarch::plotter::CellType::Triangle) == 3);
  static_assert(tarch::plotter::nodesOf(tarch::plotter::CellType::Quadrangle) == 4);
  static_assert(tarch::plotter::nodesOf(tarch::plotter::CellType::Hexahedron) == 8);
} // namespace

tarch::plotter::FileWriter::CellPlotter::CellPlotter(FileWriter& plotter, const int subdomainId) :
  _plotter(plotter),
  _buffers(plotter._subdomains.at(static_cast<std::size_t>(subdomainId))) {
  _plotter.checkCanOpenPlotter(subdomainId);
  _currentOffset = _buffers.offsets.empty() ? 0 : _buffers.offsets.back();
}

template <int N>
int tarch::plotter::FileWriter::CellPlotter::plotStandardCell(
  const int* const   vertexIndices,
  const std::uint8_t vtkType
) {
  std::int32_t buffer[N];
  std::copy_n(vertexIndices, N, buffer);
  return pushCell(buffer, N, vtkType);
}

int tarch::plotter::FileWriter::CellPlotter::pushCell(
  const std::int32_t* const vertexIndices,
  const int                 numberOfVertices,
  const std::uint8_t        vtkType
) {
  const int cellId = static_cast<int>(_buffers.types.size());
  _buffers.types.push_back(vtkType);

  _buffers.connectivity.insert(_buffers.connectivity.end(), vertexIndices, vertexIndices + numberOfVertices);

  // The connectivity offset is a 32-bit value (it is stored and written as such
  // for VTU and HDF5). Guard against silent overflow on very large subdomains.
  if (_currentOffset > std::numeric_limits<std::int32_t>::max() - numberOfVertices) {
    throw std::runtime_error(
      "CellPlotter connectivity offset exceeds the 32-bit range; the subdomain has too many connectivity entries."
    );
  }
  _currentOffset += numberOfVertices;
  _buffers.offsets.push_back(_currentOffset);

  return cellId;
}

int tarch::plotter::FileWriter::CellPlotter::plotPoint(const int vertexIndex) {
  return plotStandardCell<nodesOf(CellType::Vertex)>(&vertexIndex, static_cast<std::uint8_t>(CellType::Vertex));
}

int tarch::plotter::FileWriter::CellPlotter::plotLine(const int* const vertexIndices) {
  return plotStandardCell<nodesOf(CellType::Line)>(vertexIndices, static_cast<std::uint8_t>(CellType::Line));
}

int tarch::plotter::FileWriter::CellPlotter::plotTriangle(const int* const vertexIndices) {
  return plotStandardCell<nodesOf(CellType::Triangle)>(vertexIndices, static_cast<std::uint8_t>(CellType::Triangle));
}

int tarch::plotter::FileWriter::CellPlotter::plotQuadrangle(const int* const vertexIndices) {
  return plotStandardCell<nodesOf(CellType::Quadrangle)>(
    vertexIndices,
    static_cast<std::uint8_t>(CellType::Quadrangle)
  );
}

int tarch::plotter::FileWriter::CellPlotter::plotHexahedron(const int* const vertexIndices) {
  return plotStandardCell<nodesOf(CellType::Hexahedron)>(
    vertexIndices,
    static_cast<std::uint8_t>(CellType::Hexahedron)
  );
}
