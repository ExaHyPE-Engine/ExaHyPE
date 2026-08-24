// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FileWriter.h"

std::size_t tarch::plotter::FileWriter::VertexHash::operator()(
  const std::tuple<long long, long long, long long>& key
) const {
  // Mix each coordinate (boost hash_combine); a plain XOR of shifts collides badly for
  // the structured integer coordinates and lengthens the dedup probe chains.
  std::size_t seed = 0;
  auto        mix  = [&seed](long long value) {
    seed ^= std::hash<long long>()(value) + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
  };
  mix(std::get<0>(key));
  mix(std::get<1>(key));
  mix(std::get<2>(key));
  return seed;
}

tarch::plotter::FileWriter::VertexPlotter::VertexPlotter(
  FileWriter&         plotter,
  const int           subdomainId,
  const VertexSharing vertexSharing
) :
  _plotter(plotter),
  _buffers(plotter._subdomains.at(static_cast<std::size_t>(subdomainId))),
  _vertexSharing(vertexSharing) {
  _plotter.checkCanOpenPlotter(subdomainId);
}

int tarch::plotter::FileWriter::VertexPlotter::plotVertex(const tarch::la::Vector<2, double>& position) {
  tarch::la::Vector<3, double> position3d;
  position3d(0) = position(0);
  position3d(1) = position(1);
  position3d(2) = 0.0;
  return plotVertex(position3d);
}

int tarch::plotter::FileWriter::VertexPlotter::plotVertex(const tarch::la::Vector<3, double>& position) {
  const auto appendVertex = [&]() {
    const int vertexIndex = static_cast<int>(_buffers.pointsXYZ.size() / 3);
    _buffers.pointsXYZ.push_back(position(0));
    _buffers.pointsXYZ.push_back(position(1));
    _buffers.pointsXYZ.push_back(position(2));
    return vertexIndex;
  };

  if (_vertexSharing == VertexSharing::Distinct) {
    return appendVertex();
  }

  constexpr double InverseTolerance = 1.0 / 1e-8;

  const auto key = std::make_tuple(
    static_cast<long long>(std::round(position(0) * InverseTolerance)),
    static_cast<long long>(std::round(position(1) * InverseTolerance)),
    static_cast<long long>(std::round(position(2) * InverseTolerance))
  );

  auto&      vertexMap = _buffers.vertexMap;
  const auto vertex    = vertexMap.find(key);
  if (vertex != vertexMap.end()) {
    return vertex->second;
  }

  const int vertexIndex = appendVertex();
  vertexMap.emplace(key, vertexIndex);
  return vertexIndex;
}
