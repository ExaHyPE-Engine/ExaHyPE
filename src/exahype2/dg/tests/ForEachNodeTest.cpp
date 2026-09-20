// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ForEachNodeTest.h"

#include "exahype2/dg/ForEachNode.h"

namespace {
  constexpr int    NodesPerAxis = 2;
  constexpr int    Values       = 3;
  constexpr int    Nodes = DIMENSIONS == 2 ? NodesPerAxis * NodesPerAxis : NodesPerAxis * NodesPerAxis * NodesPerAxis;
  constexpr double CellWidth = 2.0;
  constexpr double Points[]  = {0.25, 0.75};
  constexpr double Weights[] = {0.5, 0.5};
} // namespace

exahype2::dg::tests::ForEachNodeTest::ForEachNodeTest() :
  tarch::tests::TestCase("exahype2::dg::tests::ForEachNodeTest") {}

void exahype2::dg::tests::ForEachNodeTest::run() {
  testMethod(testVisitsEveryNodeWithItsQuadratureWeight);
  testMethod(testReadsFloatValuesInPlace);
}

/**
 * Every node carries the weight of the cell volume divided by the node count.
 */
void exahype2::dg::tests::ForEachNodeTest::testVisitsEveryNodeWithItsQuadratureWeight() {
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(CellWidth);
  std::array<double, Nodes * Values>          values{};
  int                                         visits = 0;

  exahype2::dg::forEachNode<NodesPerAxis, Values>(
    cellCentre,
    cellSize,
    Points,
    Weights,
    values.data(),
    [&](double* const value, const auto& x, const auto& h, const double weight, const int index) {
      validateEquals(index, visits);
      validateEquals(value - values.data(), index * Values);
      int remainingIndex = index;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        validateNumericalEqualsWithParams2(x(axis), 0.5 + (remainingIndex % NodesPerAxis) * 1.0, index, axis);
        validateNumericalEqualsWithParams2(h(axis), CellWidth, index, axis);
        remainingIndex /= NodesPerAxis;
      }
      const double cellVolume = DIMENSIONS == 2 ? CellWidth * CellWidth : CellWidth * CellWidth * CellWidth;
      validateNumericalEquals(weight, cellVolume / Nodes);
      visits++;
    }
  );
  validateEquals(visits, Nodes);
}

/**
 * A single-precision buffer reaches the functor as a pointer into that buffer.
 */
void exahype2::dg::tests::ForEachNodeTest::testReadsFloatValuesInPlace() {
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(CellWidth);
  std::array<float, Nodes * Values>           values{};
  for (int entry = 0; entry < Nodes * Values; entry++) {
    values[entry] = static_cast<float>(entry);
  }
  const float* const constValues = values.data();
  double             sum         = 0.0;

  exahype2::dg::forEachNode<NodesPerAxis, Values>(
    cellCentre,
    cellSize,
    Points,
    Weights,
    constValues,
    [&](
      const float* const            value,
      [[maybe_unused]] const auto&  x,
      [[maybe_unused]] const auto&  h,
      [[maybe_unused]] const double weight,
      const int                     index
    ) {
      validateNumericalEquals(value[0], static_cast<float>(index * Values));
      sum += value[Values - 1];
    }
  );
  double expected = 0.0;
  for (int node = 0; node < Nodes; node++) {
    expected += node * Values + Values - 1;
  }
  validateNumericalEquals(sum, expected);
}
