// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "VertexLookupTableTest.h"

#include "peano4/grid/PeanoCurve.h"
#include "peano4/grid/StackTopology.h"
#include "peano4/grid/VertexLookupTable.h"
#include "peano4/utils/Linearised.h"
#include "tarch/la/Vector.h"

namespace {
  peano4::grid::AutomatonState makeState() {
    peano4::grid::AutomatonState state;
    state.level = 3;

    for (int d = 0; d < DIMENSIONS; ++d) {
      state.x[d] = 0.25 * (d + 1);
      state.h[d] = 0.5;
    }

    for (int i = 0; i < TWO_TIMES_D; ++i) {
      state.accessNumber[i] = 0;
    }

    return state;
  }

  tarch::la::Vector<DIMENSIONS, int> makeRelativePosition(int seed) {
    tarch::la::Vector<DIMENSIONS, int> result;
    for (int d = 0; d < DIMENSIONS; ++d) {
      result[d] = (seed + d) % 3;
    }
    return result;
  }

} // namespace

peano4::grid::tests::VertexLookupTableTest::VertexLookupTableTest():
  TestCase("peano4::grid::tests::VertexLookupTableTest") {}

void peano4::grid::tests::VertexLookupTableTest::run() {
  testMethod(testCreateVertexLookupTableMatchesPeanoCurve);
  testMethod(testCreateVertexLookupTableCachesByStructuralState);
}

void peano4::grid::tests::VertexLookupTableTest::validateLookupTableMatchesPeanoCurve(
  const peano4::grid::AutomatonState&       state,
  const tarch::la::Vector<DIMENSIONS, int>& relativePosition,
  const peano4::grid::VertexLookupTable&    lut
) {
  const int firstVertex = static_cast<int>(peano4::grid::PeanoCurve::getFirstVertexIndex(state).to_ulong());

  for (int i = 0; i < TWO_POWER_D; ++i) {
    const int expectedVertexIndex = firstVertex ^ i;

    validateEquals(lut.vIdx[i], expectedVertexIndex);

    tarch::la::Vector<DIMENSIONS, int> vertexOffset;
    for (int d = 0; d < DIMENSIONS; ++d) {
      vertexOffset[d] = (expectedVertexIndex >> d) & 1;
      validateEquals(lut.vPos[i][d], relativePosition[d] + vertexOffset[d]);
    }

    validateEquals(expectedVertexIndex, peano4::utils::dLinearised(vertexOffset, 2));

    const int expectedReadStack  = peano4::grid::PeanoCurve::getVertexReadStackNumber(state, expectedVertexIndex);
    const int expectedWriteStack = peano4::grid::PeanoCurve::getVertexWriteStackNumber(state, expectedVertexIndex);
    validateEquals(lut.readStack[i], expectedReadStack);
    validateEquals(lut.writeStack[i], expectedWriteStack);
    validateEquals(lut.inOutRead[i], peano4::grid::StackTopology::isInOutStack(expectedReadStack));
    validateEquals(lut.inOutWrite[i], peano4::grid::StackTopology::isInOutStack(expectedWriteStack));
  }

  for (int i = 0; i < TWO_TIMES_D; ++i) {
    const int expectedFaceIndex = peano4::grid::PeanoCurve::getFaceNumberAlongCurve(state, i);
    validateEquals(lut.faceIndex[i], expectedFaceIndex);
    validateEquals(lut.faceReadStack[i], peano4::grid::PeanoCurve::getFaceReadStackNumber(state, expectedFaceIndex));
    validateEquals(lut.faceWriteStack[i], peano4::grid::PeanoCurve::getFaceWriteStackNumber(state, expectedFaceIndex));
  }

  validateEquals(lut.cellReadStack, peano4::grid::PeanoCurve::getCellReadStackNumber(state));
  validateEquals(lut.cellWriteStack, peano4::grid::PeanoCurve::getCellWriteStackNumber(state));
}

void peano4::grid::tests::VertexLookupTableTest::testCreateVertexLookupTableMatchesPeanoCurve() {
  auto        stateA = makeState();
  auto        posA   = makeRelativePosition(0);
  const auto& lutA   = peano4::grid::createVertexLookupTable(stateA, posA);
  validateLookupTableMatchesPeanoCurve(stateA, posA, lutA);

  auto stateB = makeState();
  stateB.setInverted(true);
  if (DIMENSIONS > 1) {
    stateB.setEvenFlag(1, true);
  }
  for (int i = 0; i < TWO_TIMES_D; ++i) {
    stateB.accessNumber[i] = (i % 2 == 0) ? -1 : 1;
  }
  auto        posB = makeRelativePosition(1);
  const auto& lutB = peano4::grid::createVertexLookupTable(stateB, posB);
  validateLookupTableMatchesPeanoCurve(stateB, posB, lutB);
}

void peano4::grid::tests::VertexLookupTableTest::testCreateVertexLookupTableCachesByStructuralState() {
  auto state = makeState();
  state.setInverted(true);
  auto position = makeRelativePosition(2);

  const auto& reference = peano4::grid::createVertexLookupTable(state, position);

  auto sameStructure  = state;
  sameStructure.level = 7;
  for (int d = 0; d < DIMENSIONS; ++d) {
    sameStructure.x[d] += 1.0;
    sameStructure.h[d] *= 0.5;
  }
  const auto& sameReference = peano4::grid::createVertexLookupTable(sameStructure, position);
  validateEquals(&reference, &sameReference);

  auto differentFlags = state;
  differentFlags.setInverted(false);
  const auto& differentFlagsReference = peano4::grid::createVertexLookupTable(differentFlags, position);
  validate(&reference != &differentFlagsReference);
  validateLookupTableMatchesPeanoCurve(differentFlags, position, differentFlagsReference);

  auto differentAccessNumbers            = state;
  differentAccessNumbers.accessNumber[0] = -1;
  const auto& differentAccessReference   = peano4::grid::createVertexLookupTable(differentAccessNumbers, position);
  validate(&reference != &differentAccessReference);
  validateLookupTableMatchesPeanoCurve(differentAccessNumbers, position, differentAccessReference);

  auto        differentPosition          = makeRelativePosition(0);
  const auto& differentPositionReference = peano4::grid::createVertexLookupTable(state, differentPosition);
  validate(&reference != &differentPositionReference);
  validateLookupTableMatchesPeanoCurve(state, differentPosition, differentPositionReference);
}
