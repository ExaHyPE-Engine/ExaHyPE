// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LimiterTestData.h"

namespace {
  const double QuadratureNodes[exahype2::dg::limiting::tests::StubLayout::NodesPerAxis] = {
    0.5 - 0.5 / std::sqrt(3.0),
    0.5 + 0.5 / std::sqrt(3.0)
  };

  /** Returns the Lagrange basis function of node at x. */
  double getBasisValue(const double x, const int node) {
    const int other = 1 - node;
    return (x - QuadratureNodes[other]) / (QuadratureNodes[node] - QuadratureNodes[other]);
  }
} // namespace

exahype2::dg::limiting::tests::SubcellProjections::SubcellProjections() {
  constexpr int NodesPerAxis    = StubLayout::NodesPerAxis;
  constexpr int SubcellsPerAxis = StubLayout::SubcellsPerAxis;
  for (int node = 0; node < NodesPerAxis; node++) {
    quadraturePoints[static_cast<std::size_t>(node)] = QuadratureNodes[node];
    // A node takes the subcell it lies in, so a constant subgrid returns the constant.
    const int subcellOfNode = static_cast<int>(QuadratureNodes[node] * SubcellsPerAxis);
    for (int subcell = 0; subcell < SubcellsPerAxis; subcell++) {
      nodesToSubcells[static_cast<std::size_t>(node) * SubcellsPerAxis + subcell] =
        getBasisValue((subcell + 0.5) / SubcellsPerAxis, node);
      subcellsToNodes[static_cast<std::size_t>(subcell) * NodesPerAxis + node] = subcell == subcellOfNode ? 1.0 : 0.0;
    }
    nodesToLobatto[static_cast<std::size_t>(node) * NodesPerAxis]     = getBasisValue(0.0, node);
    nodesToLobatto[static_cast<std::size_t>(node) * NodesPerAxis + 1] = getBasisValue(1.0, node);
  }
}
