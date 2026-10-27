// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LimiterTestData.h"

#include "tarch/Assertions.h"

namespace {
  const double QuadratureNodes[exahype2::dg::limiting::tests::StubLayout::NodesPerAxis] = {
    0.5 - 0.5 / std::sqrt(3.0),
    0.5 + 0.5 / std::sqrt(3.0)
  };

  double getBasisValue(const double x, const int node) {
    const int other = 1 - node;
    return (x - QuadratureNodes[other]) / (QuadratureNodes[node] - QuadratureNodes[other]);
  }

  /**
   * Overwrites every entry of table and its point mirror with their mean. The generator of the
   * projection tables does the same. The butterfly projections of the limiter require that symmetry.
   */
  void averageMirrorPairs(std::vector<double>& table, const int rows, const int columns) {
    assertion(table.size() == static_cast<std::size_t>(rows) * columns, table.size(), rows, columns);
    for (int row = 0; row < rows; row++) {
      for (int column = 0; column < columns; column++) {
        const std::size_t entry  = static_cast<std::size_t>(row) * columns + column;
        const std::size_t mirror = static_cast<std::size_t>(rows - 1 - row) * columns + (columns - 1 - column);
        const double      mean   = 0.5 * (table[entry] + table[mirror]);
        table[entry]             = mean;
        table[mirror]            = mean;
      }
    }
  }
} // namespace

exahype2::dg::limiting::tests::SubcellProjections::SubcellProjections() {
  constexpr int NodesPerAxis    = StubLayout::NodesPerAxis;
  constexpr int SubcellsPerAxis = StubLayout::SubcellsPerAxis;
  for (int node = 0; node < NodesPerAxis; node++) {
    quadraturePoints[static_cast<std::size_t>(node)] = QuadratureNodes[node];
    // A node takes the subcell it lies in. A constant subgrid then returns the constant.
    const int subcellOfNode = static_cast<int>(QuadratureNodes[node] * SubcellsPerAxis);
    for (int subcell = 0; subcell < SubcellsPerAxis; subcell++) {
      nodesToSubcells[static_cast<std::size_t>(node) * SubcellsPerAxis + subcell] =
        getBasisValue((subcell + 0.5) / SubcellsPerAxis, node);
      subcellsToNodes[static_cast<std::size_t>(subcell) * NodesPerAxis + node] = subcell == subcellOfNode ? 1.0 : 0.0;
    }
    nodesToLobatto[static_cast<std::size_t>(node) * NodesPerAxis]     = getBasisValue(0.0, node);
    nodesToLobatto[static_cast<std::size_t>(node) * NodesPerAxis + 1] = getBasisValue(1.0, node);
  }
  averageMirrorPairs(nodesToSubcells, NodesPerAxis, SubcellsPerAxis);
  averageMirrorPairs(subcellsToNodes, SubcellsPerAxis, NodesPerAxis);
  averageMirrorPairs(nodesToLobatto, NodesPerAxis, NodesPerAxis);
}
