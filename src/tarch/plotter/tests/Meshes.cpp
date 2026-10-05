// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Meshes.h"

#include "tarch/plotter/CellTypes.h"
#include "tarch/plotter/Mesh.h"

/**
 * Appends numberOfSquares quadrangles of unit size with their lower left corners at x, x + 2, x + 4
 * and so forth on the axis y = 0.
 */
void
tarch::plotter::tests::appendUnitSquares(tarch::plotter::Mesh& mesh, const double x, const int numberOfSquares) {
  assertion(numberOfSquares >= 0, numberOfSquares);
  for (int square = 0; square < numberOfSquares; square++) {
    const double       left       = x + 2.0 * square;
    const std::int32_t firstPoint = static_cast<std::int32_t>(mesh.pointsXYZ.size() / 3);
    mesh.pointsXYZ.insert(
      mesh.pointsXYZ.end(),
      {left, 0.0, 0.0, left + 1.0, 0.0, 0.0, left + 1.0, 1.0, 0.0, left, 1.0, 0.0}
    );
    for (int corner = 0; corner < 4; corner++) {
      mesh.connectivity.push_back(firstPoint + corner);
    }
    mesh.offsets.push_back(static_cast<std::int32_t>(mesh.connectivity.size()));
    mesh.types.push_back(static_cast<std::uint8_t>(tarch::plotter::CellType::Quadrangle));
  }
}

/**
 * Appends one hexahedron of unit size with its lower corner at x on the axis. The bottom face comes
 * first and the top face follows in the same orientation.
 */
void
tarch::plotter::tests::appendUnitCube(tarch::plotter::Mesh& mesh, const double x) {
  const std::int32_t firstPoint = static_cast<std::int32_t>(mesh.pointsXYZ.size() / 3);
  mesh.pointsXYZ.insert(mesh.pointsXYZ.end(), {x, 0, 0, x + 1, 0, 0, x + 1, 1, 0, x, 1, 0,
                                               x, 0, 1, x + 1, 0, 1, x + 1, 1, 1, x, 1, 1});
  for (int corner = 0; corner < 8; corner++) {
    mesh.connectivity.push_back(firstPoint + corner);
  }
  mesh.offsets.push_back(static_cast<std::int32_t>(mesh.connectivity.size()));
  mesh.types.push_back(static_cast<std::uint8_t>(tarch::plotter::CellType::Hexahedron));
}
