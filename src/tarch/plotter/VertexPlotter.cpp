// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FileWriter.h"

std::size_t tarch::plotter::FileWriter::VertexHash::operator()(const std::tuple<long long, long long, long long>& k
) const {
  return std::hash<long long>()(std::get<0>(k)) ^ (std::hash<long long>()(std::get<1>(k)) << 1)
         ^ (std::hash<long long>()(std::get<2>(k)) << 2);
}

tarch::plotter::FileWriter::VertexPlotter::VertexPlotter(FileWriter& plotter, int subdomainId):
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

  // O(1) Lookup
  // If we have hanging vertices, we might still have duplicates in the global mesh.
  // We could move the filter to the actual output writing function to get rid of all duplicate vertices.
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
