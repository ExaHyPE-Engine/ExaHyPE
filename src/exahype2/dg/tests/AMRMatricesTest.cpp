// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "AMRMatricesTest.h"

#include "exahype2/dg/AMRMatrices.h"
#include "peano4/sfc/ActiveCurve.h"

namespace {
  constexpr int    SubdivisionFactor = peano4::sfc::ActiveCurve::SubdivisionFactor;
  constexpr int    NodesPerAxis      = 4;
  constexpr double Nodes[NodesPerAxis]{
    0.06943184420297371,
    0.33000947820757187,
    0.6699905217924281,
    0.9305681557970262
  };
  constexpr double Weights[NodesPerAxis]{
    0.1739274225687269,
    0.3260725774312731,
    0.3260725774312731,
    0.1739274225687269
  };
  constexpr double Tolerance = 1e-13;

  constexpr auto Interpolation       = exahype2::dg::interpolationMatrix1D<NodesPerAxis>(Nodes);
  constexpr auto Restriction         = exahype2::dg::restrictionMatrix1D<NodesPerAxis>(Nodes);
  constexpr auto WeightedRestriction = exahype2::dg::weightedRestrictionMatrix1D<NodesPerAxis>(Nodes, Weights);

  static_assert(Interpolation.size() == static_cast<std::size_t>(SubdivisionFactor) * NodesPerAxis * NodesPerAxis);

  constexpr int index(const int child, const int row, const int column) {
    return (child * NodesPerAxis + row) * NodesPerAxis + column;
  }

  constexpr int mirrorIndex(const int child, const int row, const int column) {
    return index(SubdivisionFactor - 1 - child, NodesPerAxis - 1 - row, NodesPerAxis - 1 - column);
  }

  double cubic(const double x) { return 1.0 + 2.0 * x - 3.0 * x * x + 4.0 * x * x * x; }
} // namespace

exahype2::dg::tests::AMRMatricesTest::AMRMatricesTest() :
  TestCase("exahype2::dg::tests::AMRMatricesTest") {}

void exahype2::dg::tests::AMRMatricesTest::run() {
  testMethod(testLagrangeBasisFunctionIsCardinal);
  testMethod(testInterpolationEvaluatesTheCoarsePolynomialAtTheChildNodes);
  testMethod(testRestrictionIsTheInterpolationTransposedOverTheChildWidth);
  testMethod(testWeightedRestrictionPreservesConstants);
  testMethod(testMatricesAreMirrorSymmetricToTheLastBit);
  testMethod(testAverageMirrorPairsKeepsSymmetricEntries);
  testMethod(testOrderZeroMatrices);
}

void exahype2::dg::tests::AMRMatricesTest::testLagrangeBasisFunctionIsCardinal() {
  for (int basisFunction = 0; basisFunction < NodesPerAxis; basisFunction++) {
    for (int node = 0; node < NodesPerAxis; node++) {
      const double value    = exahype2::dg::lagrangeBasisFunction1D<NodesPerAxis>(Nodes, basisFunction, Nodes[node]);
      const double expected = node == basisFunction ? 1.0 : 0.0;
      validateNumericalEqualsWithEpsWithParams2(value, expected, Tolerance, basisFunction, node);
    }
  }
  double partitionOfUnity = 0.0;
  for (int basisFunction = 0; basisFunction < NodesPerAxis; basisFunction++) {
    partitionOfUnity += exahype2::dg::lagrangeBasisFunction1D<NodesPerAxis>(Nodes, basisFunction, 0.3);
  }
  validateNumericalEqualsWithEps(partitionOfUnity, 1.0, Tolerance);
}

void exahype2::dg::tests::AMRMatricesTest::testInterpolationEvaluatesTheCoarsePolynomialAtTheChildNodes() {
  for (int child = 0; child < SubdivisionFactor; child++) {
    for (int fine = 0; fine < NodesPerAxis; fine++) {
      double interpolated = 0.0;
      for (int coarse = 0; coarse < NodesPerAxis; coarse++) {
        interpolated += Interpolation[index(child, fine, coarse)] * cubic(Nodes[coarse]);
      }
      const double fineNode = (Nodes[fine] + child) / SubdivisionFactor;
      validateNumericalEqualsWithEpsWithParams2(interpolated, cubic(fineNode), Tolerance, child, fine);
    }
  }
}

void exahype2::dg::tests::AMRMatricesTest::testRestrictionIsTheInterpolationTransposedOverTheChildWidth() {
  for (int child = 0; child < SubdivisionFactor; child++) {
    for (int coarse = 0; coarse < NodesPerAxis; coarse++) {
      for (int fine = 0; fine < NodesPerAxis; fine++) {
        validateNumericalEqualsWithEpsWithParams3(
          Restriction[index(child, coarse, fine)],
          Interpolation[index(child, fine, coarse)] / SubdivisionFactor,
          Tolerance,
          child,
          coarse,
          fine
        );
      }
    }
  }
}

/**
 * The weighted restriction of children that all hold the constant one yields
 * one at every coarse node, because the quadrature integrates every coarse
 * basis function exactly over the child intervals.
 */
void exahype2::dg::tests::AMRMatricesTest::testWeightedRestrictionPreservesConstants() {
  for (int coarse = 0; coarse < NodesPerAxis; coarse++) {
    double restricted = 0.0;
    for (int child = 0; child < SubdivisionFactor; child++) {
      for (int fine = 0; fine < NodesPerAxis; fine++) {
        restricted += WeightedRestriction[index(child, coarse, fine)];
      }
    }
    validateNumericalEqualsWithEpsWithParams1(restricted, 1.0, Tolerance, coarse);
  }
}

void exahype2::dg::tests::AMRMatricesTest::testMatricesAreMirrorSymmetricToTheLastBit() {
  for (int child = 0; child < SubdivisionFactor; child++) {
    for (int row = 0; row < NodesPerAxis; row++) {
      for (int column = 0; column < NodesPerAxis; column++) {
        validateEqualsWithParams1(
          Interpolation[index(child, row, column)],
          Interpolation[mirrorIndex(child, row, column)],
          child
        );
        validateEqualsWithParams1(
          Restriction[index(child, row, column)],
          Restriction[mirrorIndex(child, row, column)],
          child
        );
        validateEqualsWithParams1(
          WeightedRestriction[index(child, row, column)],
          WeightedRestriction[mirrorIndex(child, row, column)],
          child
        );
      }
    }
  }
}

void exahype2::dg::tests::AMRMatricesTest::testAverageMirrorPairsKeepsSymmetricEntries() {
  auto matrices = Interpolation;
  exahype2::dg::averageMirrorPairs<NodesPerAxis>(matrices);
  validate(matrices == Interpolation);

  exahype2::dg::AMRMatrices1D<NodesPerAxis> perturbed{};
  perturbed[index(0, 0, 0)]                                                   = 1.0;
  perturbed[index(SubdivisionFactor - 1, NodesPerAxis - 1, NodesPerAxis - 1)] = 3.0;
  exahype2::dg::averageMirrorPairs<NodesPerAxis>(perturbed);
  validateEquals(perturbed[index(0, 0, 0)], 2.0);
  validateEquals(perturbed[index(SubdivisionFactor - 1, NodesPerAxis - 1, NodesPerAxis - 1)], 2.0);
}

void exahype2::dg::tests::AMRMatricesTest::testOrderZeroMatrices() {
  constexpr double Node{0.5};
  constexpr double Weight{1.0};
  const auto       interpolation = exahype2::dg::interpolationMatrix1D<1>(&Node);
  const auto       restriction   = exahype2::dg::restrictionMatrix1D<1>(&Node);
  const auto       weighted      = exahype2::dg::weightedRestrictionMatrix1D<1>(&Node, &Weight);
  for (int child = 0; child < SubdivisionFactor; child++) {
    validateEquals(interpolation[child], 1.0);
    validateEquals(restriction[child], 1.0 / SubdivisionFactor);
    validateEquals(weighted[child], 1.0 / SubdivisionFactor);
  }
}
