// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "StageScratchTest.h"

#include "exahype2/dg/rk/StageScratch.h"

namespace {
  constexpr int Values      = 64;
  constexpr int OtherValues = 32;
} // namespace

exahype2::dg::rk::tests::StageScratchTest::StageScratchTest() :
  tarch::tests::TestCase("exahype2::dg::rk::tests::StageScratchTest") {}

void exahype2::dg::rk::tests::StageScratchTest::run() {
  testMethod(testEveryCallOfOneThreadReturnsTheSameBuffer);
  testMethod(testTheBufferHoldsEveryValueOfACell);
}

/** One buffer serves every cell of a thread, and each size and precision has its own. */
void exahype2::dg::rk::tests::StageScratchTest::testEveryCallOfOneThreadReturnsTheSameBuffer() {
  double* const firstCell  = exahype2::dg::rk::stageScratch<Values, double>();
  double* const secondCell = exahype2::dg::rk::stageScratch<Values, double>();
  validate(firstCell != nullptr);
  validateEquals(firstCell, secondCell);

  float* const singlePrecision = exahype2::dg::rk::stageScratch<Values, float>();
  validate(static_cast<const void*>(singlePrecision) != static_cast<const void*>(firstCell));

  double* const smallerBuffer = exahype2::dg::rk::stageScratch<OtherValues, double>();
  validate(smallerBuffer != firstCell);
}

/** Checks that the buffer carries every value the caller writes into it. */
void exahype2::dg::rk::tests::StageScratchTest::testTheBufferHoldsEveryValueOfACell() {
  double* const scratch = exahype2::dg::rk::stageScratch<Values, double>();
  for (int value = 0; value < Values; value++) {
    scratch[value] = 1.0 + value;
  }

  const double* const readBack = exahype2::dg::rk::stageScratch<Values, double>();
  for (int value = 0; value < Values; value++) {
    validateNumericalEqualsWithParams1(readBack[value], 1.0 + value, value);
  }
}
