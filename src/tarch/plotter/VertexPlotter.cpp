// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/Termination.h"
#include "tarch/plotter/FileWriter.h"

std::size_t tarch::plotter::FileWriter::VertexHash::operator()(const VertexKey& key) const {
  // Mix the three coordinates with the constants of boost hash_combine, which
  // spreads the structured integer coordinates over the buckets.
  std::size_t seed = 0;
  auto        mix  = [&seed](const std::int64_t value) {
    seed ^= std::hash<std::int64_t>()(value) + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
  };
  mix(std::get<0>(key));
  mix(std::get<1>(key));
  mix(std::get<2>(key));
  return seed;
}

tarch::plotter::FileWriter::VertexPlotter::VertexPlotter(
  FileWriter&         plotter,
  const int           subdomainId,
  const VertexMerging vertexMerging
) :
  _buffers(plotter.pieceBuffers(subdomainId)),
  _vertexMerging(vertexMerging) {}

int tarch::plotter::FileWriter::VertexPlotter::plotVertex(const tarch::la::Vector<2, double>& position) {
  tarch::la::Vector<3, double> position3D;
  position3D(0) = position(0);
  position3D(1) = position(1);
  position3D(2) = 0.0;
  return plotVertex(position3D);
}

int tarch::plotter::FileWriter::VertexPlotter::plotVertex(const tarch::la::Vector<3, double>& position) {
  const auto appendVertex = [&]() {
    const auto numberOfVertices = _buffers.pointsXYZ.size() / 3;
    if (numberOfVertices >= static_cast<std::size_t>(std::numeric_limits<int>::max())) {
      tarch::terminateWithError("The plot piece has more vertices than a 32-bit vertex index can address.");
    }
    const int vertexIndex = static_cast<int>(numberOfVertices);
    _buffers.pointsXYZ.push_back(position(0));
    _buffers.pointsXYZ.push_back(position(1));
    _buffers.pointsXYZ.push_back(position(2));
    return vertexIndex;
  };

  // Distinct vertices keep separate point-data values at one position, as a
  // discontinuous field requires.
  if (_vertexMerging == VertexMerging::Distinct) {
    return appendVertex();
  }

  // Two adjacent patches reach a corner both of them hold through different
  // arithmetic, so their coordinates agree only to roundoff. Quantising before
  // the lookup collapses the two into one plotted point and maps both zeros
  // onto the same key.
  const auto quantisedCoordinate = [](const double coordinate) {
    constexpr double InverseTolerance           = 1.0 / 1e-8;
    constexpr double MinimumQuantisedCoordinate = static_cast<double>(std::numeric_limits<std::int64_t>::lowest());
    constexpr double MaximumQuantisedCoordinate = -MinimumQuantisedCoordinate;
    const auto       scaledCoordinate           = coordinate * InverseTolerance;
    if (
      not std::isfinite(scaledCoordinate) or scaledCoordinate < MinimumQuantisedCoordinate
      or scaledCoordinate >= MaximumQuantisedCoordinate
    ) {
      tarch::terminateWithError("A plot vertex coordinate exceeds the quantised key range.");
    }
    return static_cast<std::int64_t>(std::llround(scaledCoordinate));
  };

  const auto key = std::make_tuple(
    quantisedCoordinate(position(0)),
    quantisedCoordinate(position(1)),
    quantisedCoordinate(position(2))
  );

  auto& map = _buffers.vertexMap;

  // The deduplication covers one subdomain, so hanging vertices can still leave
  // duplicates in the global mesh.
  auto iter = map.find(key);
  if (iter != map.end()) {
    return iter->second;
  }

  const int vertexIndex = appendVertex();
  map.emplace(key, vertexIndex);
  return vertexIndex;
}
