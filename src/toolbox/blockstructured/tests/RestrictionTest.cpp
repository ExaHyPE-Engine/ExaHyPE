// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RestrictionTest.h"

#include "peano4/Loop.h"
#include "peano4/grid/GridTraversalEvent.h"
#include "toolbox/blockstructured/AMRRoutines.h"

namespace {
  // A two-point Gauss-Legendre basis on the unit interval, the matrices a solver generates
  // for polynomial order one.
  constexpr int    NodesPerAxis = 2;
  constexpr double Node0        = 0.5 - 0.5 / 1.7320508075688772;
  constexpr double Node1        = 0.5 + 0.5 / 1.7320508075688772;

  double shapeFunction(const int node, const double x) {
    return node == 0 ? (Node1 - x) / (Node1 - Node0) : (x - Node0) / (Node1 - Node0);
  }

  std::array<double, 3 * NodesPerAxis * NodesPerAxis> interpolationMatrix() {
    std::array<double, 3 * NodesPerAxis * NodesPerAxis> result{};
    for (int block = 0; block < 3; block++) {
      for (int fine = 0; fine < NodesPerAxis; fine++) {
        const double fineNode = ((fine == 0 ? Node0 : Node1) + block) / 3.0;
        for (int coarse = 0; coarse < NodesPerAxis; coarse++) {
          result[block * NodesPerAxis * NodesPerAxis + fine * NodesPerAxis + coarse] = shapeFunction(coarse, fineNode);
        }
      }
    }
    return result;
  }

  std::array<double, 3 * NodesPerAxis * NodesPerAxis> restrictionMatrix() {
    std::array<double, 3 * NodesPerAxis * NodesPerAxis> result{};
    for (int block = 0; block < 3; block++) {
      for (int coarse = 0; coarse < NodesPerAxis; coarse++) {
        for (int fine = 0; fine < NodesPerAxis; fine++) {
          const double fineNode = ((fine == 0 ? Node0 : Node1) + block) / 3.0;
          result[block * NodesPerAxis * NodesPerAxis + coarse * NodesPerAxis + fine] =
            shapeFunction(coarse, fineNode) / 3.0;
        }
      }
    }
    return result;
  }

  constexpr std::array<double, NodesPerAxis> QuadratureWeights = {0.5, 0.5};

  double nodePosition(const int node) { return node == 0 ? Node0 : Node1; }

  // An affine field is representable in the order-one basis, so the interpolate-then-restrict
  // round trip has to reproduce it exactly up to roundoff.
  double affineField(const tarch::la::Vector<DIMENSIONS, double>& x) {
    double result = 1.0;
    for (int d = 0; d < DIMENSIONS; d++) {
      result += (2.0 + d) * x(d);
    }
    return result;
  }
} // namespace

toolbox::blockstructured::tests::RestrictionTest::RestrictionTest() :
  TestCase("toolbox::blockstructured::tests::RestrictionTest") {}

void toolbox::blockstructured::tests::RestrictionTest::run() {
  testMethod(testRestrictHaloLayer);
  testMethod(testRestrictCell);
  testMethod(testRestrictCellWithMatrixReproducesPolynomials);
}

void toolbox::blockstructured::tests::RestrictionTest::testRestrictCellWithMatrixReproducesPolynomials() {
  constexpr int Unknowns = 2;

  int nodesPerCell = 1;
  for (int d = 0; d < DIMENSIONS; d++) {
    nodesPerCell *= NodesPerAxis;
  }

  const auto interpolation = interpolationMatrix();
  const auto restriction   = restrictionMatrix();

  std::vector<double> coarse(static_cast<std::size_t>(nodesPerCell) * Unknowns, 0.0);
  peano4::dForEach(NodesPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& node) {
    tarch::la::Vector<DIMENSIONS, double> x;
    for (int d = 0; d < DIMENSIONS; d++) {
      x(d) = nodePosition(node(d));
    }
    const int index   = peano4::dLinearised(node, NodesPerAxis) * Unknowns;
    coarse[index]     = affineField(x);
    coarse[index + 1] = 4.0;
  });

  // Interpolate onto every child, restrict every child back: the round trip is the identity
  // for every field the coarse basis represents.
  peano4::grid::GridTraversalEvent event;
  std::vector<double>              restricted(static_cast<std::size_t>(nodesPerCell) * Unknowns, 0.0);
  peano4::dForEach(3, [&](const tarch::la::Vector<DIMENSIONS, int>& child) {
    event.setRelativePositionToFather(child);
    const auto cellMarker = event.getCellMarker();

    std::vector<double> fine(static_cast<std::size_t>(nodesPerCell) * Unknowns, 0.0);
    toolbox::blockstructured::interpolateCell(
      cellMarker,
      coarse.data(),
      fine.data(),
      interpolation.data(),
      NodesPerAxis,
      Unknowns
    );

    peano4::dForEach(NodesPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& node) {
      tarch::la::Vector<DIMENSIONS, double> x;
      for (int d = 0; d < DIMENSIONS; d++) {
        x(d) = (nodePosition(node(d)) + child(d)) / 3.0;
      }
      const int index = peano4::dLinearised(node, NodesPerAxis) * Unknowns;
      validateNumericalEqualsWithEps(fine[index], affineField(x), 1e-12);
      validateNumericalEqualsWithEps(fine[index + 1], 4.0, 1e-12);
    });

    toolbox::blockstructured::restrictCell(
      cellMarker,
      restricted.data(),
      fine.data(),
      restriction.data(),
      QuadratureWeights.data(),
      NodesPerAxis,
      Unknowns
    );
  });

  for (std::size_t i = 0; i < coarse.size(); i++) {
    validateNumericalEqualsWithEps(restricted[i], coarse[i], 1e-12);
  }
}

void toolbox::blockstructured::tests::RestrictionTest::testRestrictHaloLayer() {
  // Unknowns == DIMENSIONS lets each axis carry a distinct linear ramp.
  constexpr int NumberOfUnknowns    = DIMENSIONS;
  constexpr int NumberOfDofsPerAxis = 7;

#if DIMENSIONS == 2
  constexpr int FaceSize = 2 * NumberOfUnknowns * NumberOfDofsPerAxis;
#else
  constexpr int FaceSize = 2 * NumberOfUnknowns * NumberOfDofsPerAxis * NumberOfDofsPerAxis;
#endif
  double coarseGridFaceValues[FaceSize];
  double fineGridFaceValues[FaceSize];

  for (int normal = 0; normal < DIMENSIONS; normal++) {
    auto serialiseIndex = [normal](tarch::la::Vector<DIMENSIONS, int> volume) {
      int base  = NumberOfUnknowns;
      int index = 0;
      for (int d = 0; d < DIMENSIONS; d++) {
        index += volume(d) * base;
        if (d == normal) {
          base *= 2;
        } else {
          base *= NumberOfDofsPerAxis;
        }
      }
      return index;
    };

    for (int side = 0; side < 2; side++) {
      // Pre-zero the coarse face: the averaging restriction accumulates onto it.
      std::fill_n(coarseGridFaceValues, FaceSize, 0.);

      peano4::grid::GridTraversalEvent event;
      peano4::dForEach(3, [&](const tarch::la::Vector<DIMENSIONS, int>& subCellIndex) {
        event.setRelativePositionToFather(subCellIndex);
        auto faceMarker = event.getFaceMarker();
        faceMarker.select(normal + side * DIMENSIONS);

        if (not faceMarker.isInteriorFaceWithinPatch()) {
          peano4::dForEachExclusive(
            NumberOfDofsPerAxis,
            normal,
            side,
            [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
              const auto index = serialiseIndex(volume);
              for (int u = 0; u < NumberOfUnknowns; u++) {
                fineGridFaceValues[index + u] = volume[u] + (u == normal ? 0 : NumberOfDofsPerAxis * subCellIndex[u]);
              }
            }
          );

          toolbox::blockstructured::restrictHaloLayer(
            faceMarker,
            coarseGridFaceValues,
            fineGridFaceValues,
            side,
            NumberOfDofsPerAxis,
            NumberOfUnknowns
          );
        }
      });

      peano4::dForEachExclusive(
        NumberOfDofsPerAxis,
        normal,
        (1 - side),
        [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
          const auto index = serialiseIndex(volume);
          for (int u = 0; u < NumberOfUnknowns; u++) {
            validateNumericalEquals(coarseGridFaceValues[index + u], 0.);
          }
        }
      );
      peano4::dForEachExclusive(
        NumberOfDofsPerAxis,
        normal,
        side,
        [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
          const auto index = serialiseIndex(volume);
          for (int u = 0; u < NumberOfUnknowns; u++) {
            const auto expectedValue = (u == normal ? side : (1 + 3 * volume[u]));
            validateNumericalEquals(coarseGridFaceValues[index + u], expectedValue);
          }
        }
      );
    }
  }
}

void toolbox::blockstructured::tests::RestrictionTest::testRestrictCell() {
  // Unknowns == DIMENSIONS lets each axis carry a distinct linear ramp.
  constexpr int NumberOfUnknowns    = DIMENSIONS;
  constexpr int NumberOfDofsPerAxis = 7;

#if DIMENSIONS == 2
  constexpr int CellSize = NumberOfUnknowns * NumberOfDofsPerAxis * NumberOfDofsPerAxis;
#else
  constexpr int CellSize = NumberOfUnknowns * NumberOfDofsPerAxis * NumberOfDofsPerAxis * NumberOfDofsPerAxis;
#endif
  double coarseGridCellValues[CellSize] = {0.};
  double fineGridCellValues[CellSize];

  peano4::grid::GridTraversalEvent event;
  peano4::dForEach(3, [&](const tarch::la::Vector<DIMENSIONS, int>& subCellIndex) {
    event.setRelativePositionToFather(subCellIndex);
    const auto cellMarker = event.getCellMarker();

    peano4::dForEach(NumberOfDofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
      const auto index = NumberOfUnknowns * peano4::dLinearised(volume, NumberOfDofsPerAxis);
      for (int u = 0; u < NumberOfUnknowns; u++) {
        fineGridCellValues[index + u] = volume[u] + NumberOfDofsPerAxis * subCellIndex[u];
      }
    });

    toolbox::blockstructured::restrictCell(
      cellMarker,
      coarseGridCellValues,
      fineGridCellValues,
      NumberOfDofsPerAxis,
      NumberOfUnknowns
    );
  });

  peano4::dForEach(NumberOfDofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
    const auto index = NumberOfUnknowns * peano4::dLinearised(volume, NumberOfDofsPerAxis);
    for (int u = 0; u < NumberOfUnknowns; u++) {
      validateNumericalEquals(coarseGridCellValues[index + u], (1 + 3 * volume[u]));
    }
  });
}
