// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FileWriter.h"

tarch::plotter::FileWriter::CellPlotter::CellPlotter(FileWriter& plotter, int subdomainId):
  _plotter(plotter),
  _buffers(plotter._subdomains.at(static_cast<std::size_t>(subdomainId))) {
  _plotter.checkCanOpenPlotter(subdomainId);
}

template <int N>
int tarch::plotter::FileWriter::CellPlotter::plotStandardCell(const int* vertexIndex, std::uint8_t vtkType) {
  std::int32_t buffer[N];
  std::copy_n(vertexIndex, N, buffer);
  return pushCell(buffer, N, vtkType);
}

int tarch::plotter::FileWriter::CellPlotter::pushCell(const std::int32_t* idx, int n, std::uint8_t vtkType) {
  const int cellId = static_cast<int>(_buffers.types.size());
  _buffers.types.push_back(vtkType);

  _buffers.connectivity.insert(_buffers.connectivity.end(), idx, idx + n);
  _currentOffset += n;
  _buffers.offsets.push_back(_currentOffset);

  return cellId;
}

int tarch::plotter::FileWriter::CellPlotter::plotPoint(int vertexIndex) { return plotStandardCell<1>(&vertexIndex, 1); }

int tarch::plotter::FileWriter::CellPlotter::plotLine(int vertexIndex[2]) {
  return plotStandardCell<2>(vertexIndex, 3);
}

int tarch::plotter::FileWriter::CellPlotter::plotTriangle(int vertexIndex[3]) {
  return plotStandardCell<3>(vertexIndex, 5);
}

int tarch::plotter::FileWriter::CellPlotter::plotQuadrangle(int vertexIndex[4]) {
  return plotStandardCell<4>(vertexIndex, 9);
}

int tarch::plotter::FileWriter::CellPlotter::plotHexahedron(int vertexIndex[8]) {
  return plotStandardCell<8>(vertexIndex, 12);
}
