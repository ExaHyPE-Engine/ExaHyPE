// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "VertexExtremaTest.h"

#include "exahype2/dg/limiting/VertexExtrema.h"
#include "exahype2/dg/limiting/tests/LimiterFixtures.h"

namespace {
  constexpr int NumberOfObservables = 2;

  using VertexLabel  = exahype2::dg::limiting::tests::TestVertexLabel<NumberOfObservables>;
  using VertexLabels = exahype2::dg::limiting::tests::TestEnumerator<VertexLabel, peano4::VerticesPerCell>;

  /**
   * Returns the position of a hanging vertex on the lower boundary facet of the first axis, so the
   * coarse vertices with an even number carry it.
   */
  tarch::la::Vector<DIMENSIONS, int> hangingVertexOnFirstAxis() {
    tarch::la::Vector<DIMENSIONS, int> position(1);
    position(0) = 0;
    return position;
  }
} // namespace

exahype2::dg::limiting::tests::VertexExtremaTest::VertexExtremaTest() :
  tarch::tests::TestCase("exahype2::dg::limiting::tests::VertexExtremaTest") {}

void exahype2::dg::limiting::tests::VertexExtremaTest::run() {
  testMethod(testResetSeedsInfinities);
  testMethod(testCellExtremaReachEveryAdjacentVertex);
  testMethod(testVertexExtremaWidenTheCellRange);
  testMethod(testHangingVertexTakesTheExtremaOfItsFacetVertices);
  testMethod(testHangingVertexExtremaReachTheFacetVertices);
  testMethod(testMergeReducesTwoCopiesOfOneVertex);
}

/**
 * Checks that a vertex no cell writes keeps an empty range.
 */
void exahype2::dg::limiting::tests::VertexExtremaTest::testResetSeedsInfinities() {
  VertexLabel vertexLabel;

  exahype2::dg::limiting::resetVertexObservableExtrema<NumberOfObservables>(vertexLabel);

  for (int observable = 0; observable < NumberOfObservables; observable++) {
    validateTrue(std::isinf(vertexLabel.getDmpMinimum(observable)));
    validateTrue(vertexLabel.getDmpMinimum(observable) > 0.0);
    validateTrue(std::isinf(vertexLabel.getDmpMaximum(observable)));
    validateTrue(vertexLabel.getDmpMaximum(observable) < 0.0);
  }
}

/**
 * Checks that a cell's range reaches every vertex adjacent to it.
 */
void exahype2::dg::limiting::tests::VertexExtremaTest::testCellExtremaReachEveryAdjacentVertex() {
  VertexLabel vertices[peano4::VerticesPerCell];
  for (auto& vertexLabel : vertices) {
    exahype2::dg::limiting::resetVertexObservableExtrema<NumberOfObservables>(vertexLabel);
  }
  const VertexLabels vertexLabels{vertices};
  const double       minimum[NumberOfObservables]{-1.0, 2.0};
  const double       maximum[NumberOfObservables]{3.0, 4.0};

  exahype2::dg::limiting::reduceCellObservableExtremaIntoVertices<NumberOfObservables>(minimum, maximum, vertexLabels);

  for (const auto& vertexLabel : vertices) {
    validateNumericalEquals(vertexLabel.getDmpMinimum(0), -1.0);
    validateNumericalEquals(vertexLabel.getDmpMinimum(1), 2.0);
    validateNumericalEquals(vertexLabel.getDmpMaximum(0), 3.0);
    validateNumericalEquals(vertexLabel.getDmpMaximum(1), 4.0);
  }
}

/**
 * Checks that the widened range covers every cell adjacent to a node of this cell.
 */
void exahype2::dg::limiting::tests::VertexExtremaTest::testVertexExtremaWidenTheCellRange() {
  VertexLabel vertices[peano4::VerticesPerCell];
  for (auto& vertexLabel : vertices) {
    for (int observable = 0; observable < NumberOfObservables; observable++) {
      vertexLabel.setDmpMinimum(observable, 0.0);
      vertexLabel.setDmpMaximum(observable, 1.0);
    }
  }
  vertices[0].setDmpMinimum(0, -5.0);
  vertices[peano4::VerticesPerCell - 1].setDmpMaximum(1, 7.0);
  const VertexLabels vertexLabels{vertices};
  double             minimum[NumberOfObservables]{0.5, 0.5};
  double             maximum[NumberOfObservables]{0.5, 0.5};

  exahype2::dg::limiting::widenRangeByVertexObservableExtrema<NumberOfObservables>(minimum, maximum, vertexLabels);

  validateNumericalEquals(minimum[0], -5.0);
  validateNumericalEquals(minimum[1], 0.0);
  validateNumericalEquals(maximum[0], 1.0);
  validateNumericalEquals(maximum[1], 7.0);
}

/**
 * Checks that a hanging vertex holds the range of the coarse vertices on its facet only.
 */
void exahype2::dg::limiting::tests::VertexExtremaTest::testHangingVertexTakesTheExtremaOfItsFacetVertices() {
  VertexLabel coarseVertices[peano4::VerticesPerCell];
  for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
    const double offset = static_cast<double>(vertexNumber);
    for (int observable = 0; observable < NumberOfObservables; observable++) {
      coarseVertices[vertexNumber].setDmpMinimum(observable, -offset);
      coarseVertices[vertexNumber].setDmpMaximum(observable, offset);
    }
  }
  const VertexLabels coarseVertexLabels{coarseVertices};
  VertexLabel        vertexLabel;

  exahype2::dg::limiting::interpolateHangingVertexObservableExtrema<NumberOfObservables>(
    hangingVertexOnFirstAxis(),
    vertexLabel,
    coarseVertexLabels
  );

  // The even coarse vertices lie on the lower facet of the first axis, so the last of them decides.
  const double largestEvenVertexNumber = static_cast<double>(peano4::VerticesPerCell - 2);
  validateNumericalEquals(vertexLabel.getDmpMinimum(0), -largestEvenVertexNumber);
  validateNumericalEquals(vertexLabel.getDmpMaximum(0), largestEvenVertexNumber);
}

/**
 * Checks that a hanging vertex reduces its range into the coarse vertices on its facet only.
 */
void exahype2::dg::limiting::tests::VertexExtremaTest::testHangingVertexExtremaReachTheFacetVertices() {
  VertexLabel coarseVertices[peano4::VerticesPerCell];
  for (auto& coarseVertexLabel : coarseVertices) {
    exahype2::dg::limiting::resetVertexObservableExtrema<NumberOfObservables>(coarseVertexLabel);
  }
  const VertexLabels coarseVertexLabels{coarseVertices};
  VertexLabel        vertexLabel;
  for (int observable = 0; observable < NumberOfObservables; observable++) {
    vertexLabel.setDmpMinimum(observable, -2.0);
    vertexLabel.setDmpMaximum(observable, 6.0);
  }

  exahype2::dg::limiting::restrictHangingVertexObservableExtrema<NumberOfObservables>(
    hangingVertexOnFirstAxis(),
    vertexLabel,
    coarseVertexLabels
  );

  for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
    if (vertexNumber % 2 == 0) {
      validateNumericalEquals(coarseVertices[vertexNumber].getDmpMinimum(0), -2.0);
      validateNumericalEquals(coarseVertices[vertexNumber].getDmpMaximum(0), 6.0);
    } else {
      validateTrue(std::isinf(coarseVertices[vertexNumber].getDmpMinimum(0)));
      validateTrue(std::isinf(coarseVertices[vertexNumber].getDmpMaximum(0)));
    }
  }
}

/**
 * Checks that the merge of two copies of one vertex holds the union of their ranges.
 */
void exahype2::dg::limiting::tests::VertexExtremaTest::testMergeReducesTwoCopiesOfOneVertex() {
  VertexLabel vertexLabel;
  VertexLabel neighbour;
  for (int observable = 0; observable < NumberOfObservables; observable++) {
    vertexLabel.setDmpMinimum(observable, 0.0);
    vertexLabel.setDmpMaximum(observable, 1.0);
    neighbour.setDmpMinimum(observable, -3.0);
    neighbour.setDmpMaximum(observable, 0.5);
  }

  exahype2::dg::limiting::mergeVertexObservableExtrema<NumberOfObservables>(
    peano4::grid::SendReceiveContext::InterRank,
    vertexLabel,
    neighbour
  );

  for (int observable = 0; observable < NumberOfObservables; observable++) {
    validateNumericalEquals(vertexLabel.getDmpMinimum(observable), -3.0);
    validateNumericalEquals(vertexLabel.getDmpMaximum(observable), 1.0);
  }
}
