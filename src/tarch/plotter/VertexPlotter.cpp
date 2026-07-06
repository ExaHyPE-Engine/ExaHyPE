// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FileWriter.h"

std::size_t
tarch::plotter::FileWriter::VertexHash::operator()(const std::tuple<long long, long long, long long>& k) const {
  // Mix each coordinate (boost hash_combine); a plain XOR of shifts collides badly for
  // the structured integer coordinates and lengthens the dedup probe chains.
  std::size_t seed = 0;
  auto        mix  = [&seed](long long value) {
    seed ^= std::hash<long long>()(value) + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
  };
  mix(std::get<0>(k));
  mix(std::get<1>(k));
  mix(std::get<2>(k));
  return seed;
}

tarch::plotter::FileWriter::VertexPlotter::VertexPlotter(FileWriter& plotter, int subdomainId) :
  _plotter(plotter),
  _buffers(plotter._subdomains.at(static_cast<std::size_t>(subdomainId))) {
  _plotter.checkCanOpenPlotter(subdomainId);
}

int tarch::plotter::FileWriter::VertexPlotter::plotVertex(const tarch::la::Vector<2, double>& p2) {
  tarch::la::Vector<3, double> p;
  p(0) = p2(0);
  p(1) = p2(1);
  p(2) = 0.0;
  return plotVertex(p);
}

int tarch::plotter::FileWriter::VertexPlotter::plotVertex(const tarch::la::Vector<3, double>& p) {
  constexpr double InverseTol = 1.0 / 1e-8;

  const auto key = std::make_tuple(
    static_cast<long long>(std::round(p(0) * InverseTol)),
    static_cast<long long>(std::round(p(1) * InverseTol)),
    static_cast<long long>(std::round(p(2) * InverseTol))
  );

  auto& map = _buffers.vertexMap;

  // Hanging vertices can still leave duplicates in the global mesh; moving the
  // filter to the output-writing stage would remove all duplicate vertices.
  auto iter = map.find(key);
  if (iter != map.end()) {
    return iter->second;
  }

  const int idx = static_cast<int>(_buffers.pointsXYZ.size() / 3);

  _buffers.pointsXYZ.push_back(static_cast<float>(p(0)));
  _buffers.pointsXYZ.push_back(static_cast<float>(p(1)));
  _buffers.pointsXYZ.push_back(static_cast<float>(p(2)));

  map.emplace(key, idx);
  return idx;
}
