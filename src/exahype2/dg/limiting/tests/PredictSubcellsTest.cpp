// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "PredictSubcellsTest.h"

#include "exahype2/dg/limiting/PredictSubcells.h"
#include "exahype2/dg/limiting/tests/LimiterFixtures.h"

namespace {
  using Solver = exahype2::dg::limiting::tests::TestSolver<true>;

  constexpr int NumberOfDofsPerAxis     = Solver::NumberOfDofsPerAxis;
  constexpr int NumberOfSubcellsPerAxis = Solver::NumberOfSubcellsPerAxis;
  constexpr int NumberOfNodes           = Solver::NumberOfNodes;

  using VertexLabel  = exahype2::dg::limiting::tests::TestVertexLabel<1>;
  using VertexLabels = exahype2::dg::limiting::tests::TestEnumerator<VertexLabel, peano4::VerticesPerCell>;

  constexpr double DgToLobattoMatrix[NumberOfDofsPerAxis * NumberOfDofsPerAxis]{1.0, 0.0, 0.0, 1.0};
  // Every column sums to one, so the samples stay in the convex hull of the storage nodes.
  constexpr double DgToSubcellMatrix[NumberOfDofsPerAxis * NumberOfSubcellsPerAxis]{1.0, 0.5, 0.0, 0.0, 0.5, 1.0};
} // namespace

exahype2::dg::limiting::tests::PredictSubcellsTest::PredictSubcellsTest() :
  tarch::tests::TestCase("exahype2::dg::limiting::tests::PredictSubcellsTest") {}

void exahype2::dg::limiting::tests::PredictSubcellsTest::run() {
  testMethod(testTroubledHistoryStartsTheStepOnTheSubcellScheme);
  testMethod(testUntroubledCellStartsTheStepOnTheGalerkinScheme);
  testMethod(testAcceptedExtremaReachTheAdjacentVertices);
}

/**
 * Checks that a cell troubled in the closed step opens the new step as a limited cell.
 */
void exahype2::dg::limiting::tests::PredictSubcellsTest::testTroubledHistoryStartsTheStepOnTheSubcellScheme() {
  TestCellLabel cellLabel;
  cellLabel.setLimiterStatus(exahype2::dg::LimiterStatus::Troubled);
  cellLabel.setWasTroubledInPreviousStep(true);

  exahype2::dg::limiting::predictSubcells(cellLabel);

  validateEquals(
    static_cast<int>(cellLabel.getPreviousLimiterStatus()),
    static_cast<int>(exahype2::dg::LimiterStatus::Troubled)
  );
  validateEquals(
    static_cast<int>(cellLabel.getLimiterStatus()),
    static_cast<int>(exahype2::dg::LimiterStatus::Limited)
  );
}

/**
 * Checks that a cell without a troubled history opens the new step on the Galerkin scheme.
 */
void exahype2::dg::limiting::tests::PredictSubcellsTest::testUntroubledCellStartsTheStepOnTheGalerkinScheme() {
  TestCellLabel cellLabel;
  cellLabel.setLimiterStatus(exahype2::dg::LimiterStatus::Limited);
  cellLabel.setWasTroubledInPreviousStep(false);

  exahype2::dg::limiting::predictSubcells(cellLabel);

  validateEquals(
    static_cast<int>(cellLabel.getPreviousLimiterStatus()),
    static_cast<int>(exahype2::dg::LimiterStatus::Limited)
  );
  validateEquals(
    static_cast<int>(cellLabel.getLimiterStatus()),
    static_cast<int>(exahype2::dg::LimiterStatus::Regular)
  );
}

/**
 * Checks that the range of the accepted solution reaches every vertex adjacent to the cell.
 */
void exahype2::dg::limiting::tests::PredictSubcellsTest::testAcceptedExtremaReachTheAdjacentVertices() {
  TestCellLabel cellLabel;
  VertexLabel   vertices[peano4::VerticesPerCell];
  for (auto& vertexLabel : vertices) {
    exahype2::dg::limiting::resetVertexObservableExtrema<1>(vertexLabel);
  }
  const VertexLabels vertexLabels{vertices};
  double             acceptedSolution[NumberOfNodes];
  for (int node = 0; node < NumberOfNodes; node++) {
    acceptedSolution[node] = static_cast<double>(node) - 1.0;
  }

  exahype2::dg::limiting::predictSubcells<Solver, 1>(
    cellLabel,
    acceptedSolution,
    DgToLobattoMatrix,
    DgToSubcellMatrix,
    vertexLabels
  );

  for (const auto& vertexLabel : vertices) {
    validateNumericalEquals(vertexLabel.getDmpMinimum(0), -1.0);
    validateNumericalEquals(vertexLabel.getDmpMaximum(0), static_cast<double>(NumberOfNodes) - 2.0);
  }
}
