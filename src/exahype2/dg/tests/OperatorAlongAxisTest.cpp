// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "OperatorAlongAxisTest.h"

#include "exahype2/dg/OperatorAlongAxis.h"

namespace {
  constexpr int NumberOfEntries     = 3;
  constexpr int NumberOfTargetNodes = 4;
  constexpr int NumberOfSourceNodes = 2;

  // Distinct, non-symmetric entries, so an index mix-up cannot cancel out.
  void fillOperands(
    double (&values)[NumberOfSourceNodes * NumberOfEntries],
    double (&operatorMatrix)[NumberOfTargetNodes * NumberOfSourceNodes]
  ) {
    for (int entry = 0; entry < NumberOfSourceNodes * NumberOfEntries; entry++) {
      values[entry] = 1.0 + 0.25 * entry;
    }
    for (int entry = 0; entry < NumberOfTargetNodes * NumberOfSourceNodes; entry++) {
      operatorMatrix[entry] = 2.0 - 0.125 * entry;
    }
  }
} // namespace

exahype2::dg::tests::OperatorAlongAxisTest::OperatorAlongAxisTest() :
  tarch::tests::TestCase("exahype2::dg::tests::OperatorAlongAxisTest") {}

void exahype2::dg::tests::OperatorAlongAxisTest::run() {
  testMethod(testProductMatchesTheDefinition);
  testMethod(testResultIsAccumulatedOrOverwritten);
  testMethod(testScalingMultipliesEveryProduct);
  testMethod(testStridesAddressTheIntendedSlices);
  testMethod(testANarrowerResultIsRoundedOnce);
}

void exahype2::dg::tests::OperatorAlongAxisTest::testProductMatchesTheDefinition() {
  double values[NumberOfSourceNodes * NumberOfEntries];
  double operatorMatrix[NumberOfTargetNodes * NumberOfSourceNodes];
  fillOperands(values, operatorMatrix);

  double result[NumberOfTargetNodes * NumberOfEntries] = {};

  exahype2::dg::applyOperatorAlongAxis<
    NumberOfEntries,
    NumberOfTargetNodes,
    NumberOfSourceNodes,
    NumberOfEntries,
    NumberOfSourceNodes,
    NumberOfEntries,
    true,
    double,
    double>(result, values, operatorMatrix, 1.0);

  for (int targetNode = 0; targetNode < NumberOfTargetNodes; targetNode++) {
    for (int entry = 0; entry < NumberOfEntries; entry++) {
      double expected = 0.0;
      for (int sourceNode = 0; sourceNode < NumberOfSourceNodes; sourceNode++) {
        expected +=
          values[sourceNode * NumberOfEntries + entry] * operatorMatrix[targetNode * NumberOfSourceNodes + sourceNode];
      }
      validateNumericalEquals(result[targetNode * NumberOfEntries + entry], expected);
    }
  }
}

void exahype2::dg::tests::OperatorAlongAxisTest::testResultIsAccumulatedOrOverwritten() {
  double values[NumberOfSourceNodes * NumberOfEntries];
  double operatorMatrix[NumberOfTargetNodes * NumberOfSourceNodes];
  fillOperands(values, operatorMatrix);

  double accumulated[NumberOfTargetNodes * NumberOfEntries];
  double overwritten[NumberOfTargetNodes * NumberOfEntries];
  for (int entry = 0; entry < NumberOfTargetNodes * NumberOfEntries; entry++) {
    accumulated[entry] = 5.0 + entry;
    overwritten[entry] = 5.0 + entry;
  }

  exahype2::dg::applyOperatorAlongAxis<
    NumberOfEntries,
    NumberOfTargetNodes,
    NumberOfSourceNodes,
    NumberOfEntries,
    NumberOfSourceNodes,
    NumberOfEntries,
    false,
    double,
    double>(accumulated, values, operatorMatrix, 1.0);
  exahype2::dg::applyOperatorAlongAxis<
    NumberOfEntries,
    NumberOfTargetNodes,
    NumberOfSourceNodes,
    NumberOfEntries,
    NumberOfSourceNodes,
    NumberOfEntries,
    true,
    double,
    double>(overwritten, values, operatorMatrix, 1.0);

  for (int entry = 0; entry < NumberOfTargetNodes * NumberOfEntries; entry++) {
    validateNumericalEquals(accumulated[entry], overwritten[entry] + 5.0 + entry);
  }
}

void exahype2::dg::tests::OperatorAlongAxisTest::testScalingMultipliesEveryProduct() {
  double values[NumberOfSourceNodes * NumberOfEntries];
  double operatorMatrix[NumberOfTargetNodes * NumberOfSourceNodes];
  fillOperands(values, operatorMatrix);

  double unscaled[NumberOfTargetNodes * NumberOfEntries] = {};
  double scaled[NumberOfTargetNodes * NumberOfEntries]   = {};

  exahype2::dg::applyOperatorAlongAxis<
    NumberOfEntries,
    NumberOfTargetNodes,
    NumberOfSourceNodes,
    NumberOfEntries,
    NumberOfSourceNodes,
    NumberOfEntries,
    true,
    double,
    double>(unscaled, values, operatorMatrix, 1.0);
  exahype2::dg::applyOperatorAlongAxis<
    NumberOfEntries,
    NumberOfTargetNodes,
    NumberOfSourceNodes,
    NumberOfEntries,
    NumberOfSourceNodes,
    NumberOfEntries,
    true,
    double,
    double>(scaled, values, operatorMatrix, -3.0);

  for (int entry = 0; entry < NumberOfTargetNodes * NumberOfEntries; entry++) {
    validateNumericalEquals(scaled[entry], -3.0 * unscaled[entry]);
  }
}

void exahype2::dg::tests::OperatorAlongAxisTest::testStridesAddressTheIntendedSlices() {
  constexpr int ValueStride    = NumberOfEntries + 2;
  constexpr int OperatorStride = NumberOfSourceNodes + 1;
  constexpr int ResultStride   = NumberOfEntries + 3;

  double values[NumberOfSourceNodes * ValueStride]            = {};
  double operatorMatrix[NumberOfTargetNodes * OperatorStride] = {};
  for (int sourceNode = 0; sourceNode < NumberOfSourceNodes; sourceNode++) {
    for (int entry = 0; entry < NumberOfEntries; entry++) {
      values[sourceNode * ValueStride + entry] = 1.0 + 0.25 * (sourceNode * NumberOfEntries + entry);
    }
  }
  for (int targetNode = 0; targetNode < NumberOfTargetNodes; targetNode++) {
    for (int sourceNode = 0; sourceNode < NumberOfSourceNodes; sourceNode++) {
      operatorMatrix[targetNode * OperatorStride + sourceNode] =
        2.0 - 0.125 * (targetNode * NumberOfSourceNodes + sourceNode);
    }
  }

  double result[NumberOfTargetNodes * ResultStride];
  for (int entry = 0; entry < NumberOfTargetNodes * ResultStride; entry++) {
    result[entry] = 7.0;
  }

  exahype2::dg::applyOperatorAlongAxis<
    NumberOfEntries,
    NumberOfTargetNodes,
    NumberOfSourceNodes,
    ValueStride,
    OperatorStride,
    ResultStride,
    true,
    double,
    double>(result, values, operatorMatrix, 1.0);

  for (int targetNode = 0; targetNode < NumberOfTargetNodes; targetNode++) {
    for (int entry = 0; entry < NumberOfEntries; entry++) {
      double expected = 0.0;
      for (int sourceNode = 0; sourceNode < NumberOfSourceNodes; sourceNode++) {
        expected += values[sourceNode * ValueStride + entry] * operatorMatrix[targetNode * OperatorStride + sourceNode];
      }
      validateNumericalEquals(result[targetNode * ResultStride + entry], expected);
    }
    // The padding between two result nodes is never touched.
    for (int entry = NumberOfEntries; entry < ResultStride; entry++) {
      validateNumericalEquals(result[targetNode * ResultStride + entry], 7.0);
    }
  }
}

void exahype2::dg::tests::OperatorAlongAxisTest::testANarrowerResultIsRoundedOnce() {
  // A product that a float cannot represent shows whether the accumulation runs
  // in the wider type before it is stored.
  constexpr double Operand = 1.0 + 1.0 / 1048576.0;

  const double values[1]         = {Operand};
  const double operatorMatrix[1] = {Operand};
  float        result[1]         = {0.0F};

  exahype2::dg::applyOperatorAlongAxis<1, 1, 1, 1, 1, 1, false, float, double>(result, values, operatorMatrix, 1.0);

  const auto expected = static_cast<double>(static_cast<float>(Operand * Operand));
  validateNumericalEquals(static_cast<double>(result[0]), expected);
}
