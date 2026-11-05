// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "PolynomialScalingTest.h"

#include "exahype2/fv/GaussLegendreBasis.h"
#include "exahype2/fv/limiter/PolynomialScaling.h"

namespace {
  using Basis   = exahype2::fv::GaussLegendreBasis<4>;
  using Scaling = exahype2::fv::limiter::PolynomialScaling<Basis>;

  constexpr int    NumberOfData = 2;
  constexpr double Level        = 1.0;

  /**
   * Admits the states whose first entry, a depth, is not negative. The second entry is a bed with
   * depth plus bed at Level.
   */
  struct NonnegativeDepth {
    static bool isPhysicallyAdmissible(
      const double* const Q,
      const int           numberOfStates,
      const tarch::la::Vector<DIMENSIONS, double>&,
      const tarch::la::Vector<DIMENSIONS, double>&
    ) {
      for (int state = 0; state < numberOfStates; state++) {
        if (not(Q[state * NumberOfData] >= 0.0)) {
          return false;
        }
      }
      return true;
    }
  };

  /**
   * Returns the nodal state of the depth average + slope (x - 1/2) along the first axis.
   */
  std::array<double, Basis::NodesPerCell * NumberOfData> linearDepth(const double average, const double slope) {
    std::array<double, Basis::NodesPerCell * NumberOfData> nodalState{};
    for (int node = 0; node < Basis::NodesPerCell; node++) {
      const double depth                    = average + slope * (Basis::nodes()[node % Basis::NumberOfNodes] - 0.5);
      nodalState[node * NumberOfData]     = depth;
      nodalState[node * NumberOfData + 1] = Level - depth;
    }
    return nodalState;
  }

  double limit(const double average, std::array<double, Basis::NodesPerCell * NumberOfData>& nodalState) {
    const double                          averageState[NumberOfData] = {average, Level - average};
    const tarch::la::Vector<DIMENSIONS, double> centre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> size(1.0);
    return Scaling::limit<NonnegativeDepth, NumberOfData>(averageState, nodalState.data(), centre, size);
  }
} // namespace

exahype2::fv::limiter::tests::PolynomialScalingTest::PolynomialScalingTest() :
  tarch::tests::TestCase("exahype2::fv::limiter::tests::PolynomialScalingTest") {}

void
exahype2::fv::limiter::tests::PolynomialScalingTest::run() {
  testMethod(testAdmissiblePolynomialIsUnchanged);
  testMethod(testFaceUndershootIsScaled);
  testMethod(testInadmissibleAverageGivesConstant);
}

void
exahype2::fv::limiter::tests::PolynomialScalingTest::testAdmissiblePolynomialIsUnchanged() {
  auto       nodalState = linearDepth(0.5, 0.8);
  const auto original   = nodalState;
  validateNumericalEquals(limit(0.5, nodalState), 1.0);
  for (int entry = 0; entry < Basis::NodesPerCell * NumberOfData; entry++) {
    validateNumericalEquals(nodalState[entry], original[entry]);
  }
}

/**
 * Every node of the slope 2.2 around the average 1 is positive, but the extrapolation to the lower
 * face reaches -0.1. The largest admissible scaling 1 / 1.1 vanishes there.
 */
void
exahype2::fv::limiter::tests::PolynomialScalingTest::testFaceUndershootIsScaled() {
  auto nodalState = linearDepth(1.0, 2.2);
  for (int node = 0; node < Basis::NodesPerCell; node++) {
    validateWithParams1(nodalState[node * NumberOfData] > 0.0, node);
  }
  validateNumericalEqualsWithEps(limit(1.0, nodalState), 1.0 / 1.1, 1e-6);

  double minimum = 1.0;
  double values[NumberOfData];
  for (int point = 0; point < Scaling::Points; point++) {
    Scaling::evaluate<NumberOfData>(nodalState.data(), point, values);
    validateWithParams2(values[0] >= 0.0, point, values[0]);
    validateNumericalEqualsWithParams1(values[0] + values[1], Level, point);
    minimum = std::min(minimum, values[0]);
  }
  validateNumericalEqualsWithEps(minimum, 0.0, 1e-6);

  double average = 0.0;
  for (int node = 0; node < Basis::NodesPerCell; node++) {
    double weight = 1.0;
    for (int axis = 0, remainder = node; axis < DIMENSIONS; axis++, remainder /= Basis::NumberOfNodes) {
      weight *= Basis::weights()[remainder % Basis::NumberOfNodes];
    }
    average += weight * nodalState[node * NumberOfData];
  }
  validateNumericalEquals(average, 1.0);
}

void
exahype2::fv::limiter::tests::PolynomialScalingTest::testInadmissibleAverageGivesConstant() {
  auto nodalState = linearDepth(-0.1, 1.0);
  validateNumericalEquals(limit(-0.1, nodalState), 0.0);
  for (int node = 0; node < Basis::NodesPerCell; node++) {
    validateNumericalEquals(nodalState[node * NumberOfData], -0.1);
  }
}
