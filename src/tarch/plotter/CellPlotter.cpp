// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/Termination.h"
#include "tarch/plotter/CellTypes.h"
#include "tarch/plotter/FileWriter.h"

namespace {
  static_assert(tarch::plotter::getNumberOfNodesOf(tarch::plotter::CellType::Vertex) == 1);
  static_assert(tarch::plotter::getNumberOfNodesOf(tarch::plotter::CellType::Line) == 2);
  static_assert(tarch::plotter::getNumberOfNodesOf(tarch::plotter::CellType::Triangle) == 3);
  static_assert(tarch::plotter::getNumberOfNodesOf(tarch::plotter::CellType::Quadrangle) == 4);
  static_assert(tarch::plotter::getNumberOfNodesOf(tarch::plotter::CellType::Hexahedron) == 8);
} // namespace

tarch::plotter::FileWriter::CellPlotter::CellPlotter(FileWriter& plotter, const int subdomainId) :
  _buffers(plotter.getPieceBuffers(subdomainId)),
  // _buffers is declared first, so it is already bound here. A piece that
  // already holds cells continues its connectivity where they left off.
  _currentOffset(_buffers.offsets.empty() ? 0 : _buffers.offsets.back()) {}

template <int N>
int
tarch::plotter::FileWriter::CellPlotter::plotStandardCell(const int* const vertexIndices, const std::uint8_t vtkType) {
  assertion(vertexIndices != nullptr);
  std::int32_t buffer[N];
  std::copy_n(vertexIndices, N, buffer);
  return pushCell(buffer, N, vtkType);
}

int
tarch::plotter::FileWriter::CellPlotter::pushCell(
  const std::int32_t* const vertexIndices,
  const int                 numberOfVertices,
  const std::uint8_t        vtkType
) {
  assertion(vertexIndices != nullptr);
  assertion(numberOfVertices > 0, numberOfVertices);
  assertion(_buffers.types.size() < static_cast<std::size_t>(std::numeric_limits<int>::max()), _buffers.types.size());
#if PEANO_ASSERTIONS
  const auto numberOfPlottedVertices = _buffers.pointsXYZ.size() / 3;
  for (int vertex = 0; vertex < numberOfVertices; vertex++) {
    assertion(
      vertexIndices[vertex] >= 0 and static_cast<std::size_t>(vertexIndices[vertex]) < numberOfPlottedVertices,
      vertexIndices[vertex],
      numberOfPlottedVertices
    );
  }
#endif

  // The connectivity offset is written as a 32-bit value for VTU and HDF5.
  if (numberOfVertices > std::numeric_limits<std::int32_t>::max() - _currentOffset) {
    tarch::terminateWithError(
      "The plot piece has more connectivity entries than the 32-bit connectivity offset can address."
    );
  }

  const int cellId = static_cast<int>(_buffers.types.size());
  _buffers.types.push_back(vtkType);
  _buffers.connectivity.insert(_buffers.connectivity.end(), vertexIndices, vertexIndices + numberOfVertices);
  _currentOffset += numberOfVertices;
  _buffers.offsets.push_back(_currentOffset);

  return cellId;
}

int
tarch::plotter::FileWriter::CellPlotter::plotPoint(const int vertexIndex) {
  return plotStandardCell<getNumberOfNodesOf(CellType::Vertex)>(
    &vertexIndex,
    static_cast<std::uint8_t>(CellType::Vertex)
  );
}

int
tarch::plotter::FileWriter::CellPlotter::plotLine(const int* const vertexIndices) {
  return plotStandardCell<getNumberOfNodesOf(CellType::Line)>(vertexIndices, static_cast<std::uint8_t>(CellType::Line));
}

int
tarch::plotter::FileWriter::CellPlotter::plotTriangle(const int* const vertexIndices) {
  return plotStandardCell<getNumberOfNodesOf(CellType::Triangle)>(
    vertexIndices,
    static_cast<std::uint8_t>(CellType::Triangle)
  );
}

int
tarch::plotter::FileWriter::CellPlotter::plotQuadrangle(const int* const vertexIndices) {
  return plotStandardCell<getNumberOfNodesOf(CellType::Quadrangle)>(
    vertexIndices,
    static_cast<std::uint8_t>(CellType::Quadrangle)
  );
}

int
tarch::plotter::FileWriter::CellPlotter::plotHexahedron(const int* const vertexIndices) {
  return plotStandardCell<getNumberOfNodesOf(CellType::Hexahedron)>(
    vertexIndices,
    static_cast<std::uint8_t>(CellType::Hexahedron)
  );
}
