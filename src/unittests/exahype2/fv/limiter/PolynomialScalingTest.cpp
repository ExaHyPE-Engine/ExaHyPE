// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "exahype2/fv/GaussLegendreBasis.h"
#include "exahype2/fv/limiter/PolynomialScaling.h"
#include "tarch/testing/UnitTest.h"

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
    struct RuntimeParameters final {};

    static bool isPhysicallyAdmissible(
      const double* const Q,
      const int           numberOfStates,
      const tarch::la::Vector<DIMENSIONS, double>&,
      const tarch::la::Vector<DIMENSIONS, double>&,
      const RuntimeParameters&
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
   * Admits the states whose depth is not negative and, on the lower face of the first axis, at
   * least MinimumDepthOnLowerFace.
   */
  struct DeeperOnLowerFace {
    struct RuntimeParameters final {};

    static constexpr double MinimumDepthOnLowerFace = 0.5;

    static bool isPhysicallyAdmissible(
      const double* const                          Q,
      const int                                    numberOfStates,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>&,
      const RuntimeParameters&
    ) {
      const double minimumDepth = x(0) < 1e-9 ? MinimumDepthOnLowerFace : 0.0;
      for (int state = 0; state < numberOfStates; state++) {
        if (not(Q[state * NumberOfData] >= minimumDepth)) {
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
    return Scaling::limit<NonnegativeDepth, NumberOfData>(averageState, nodalState.data(), centre, size, NonnegativeDepth::RuntimeParameters{});
  }
} // namespace

UNIT_TEST("Admissible polynomial is unchanged") {
  auto       nodalState = linearDepth(0.5, 0.8);
  const auto original   = nodalState;
  VALIDATE_NUMERICAL_EQUALS(limit(0.5, nodalState), 1.0);
  for (int entry = 0; entry < Basis::NodesPerCell * NumberOfData; entry++) {
    VALIDATE_NUMERICAL_EQUALS(nodalState[entry], original[entry]);
  }
}

/**
 * Every node of the slope 2.2 around the average 1 is positive, but the extrapolation to the lower
 * face reaches -0.1. The largest admissible scaling 1 / 1.1 vanishes there.
 */
UNIT_TEST("Face undershoot is scaled") {
  auto nodalState = linearDepth(1.0, 2.2);
  for (int node = 0; node < Basis::NodesPerCell; node++) {
    VALIDATE(nodalState[node * NumberOfData] > 0.0, node);
  }
  VALIDATE(std::abs((limit(1.0, nodalState)) - (1.0 / 1.1)) <= 1e-6, limit(1.0, nodalState), 1.0 / 1.1);

  double minimum = 1.0;
  double values[NumberOfData];
  for (int point = 0; point < Scaling::Points; point++) {
    Scaling::evaluate<NumberOfData>(nodalState.data(), point, values);
    VALIDATE(values[0] >= 0.0, point, values[0]);
    VALIDATE_NUMERICAL_EQUALS(values[0] + values[1], Level, point);
    minimum = std::min(minimum, values[0]);
  }
  VALIDATE(std::abs((minimum) - (0.0)) <= 1e-6, minimum, 0.0);

  double average = 0.0;
  for (int node = 0; node < Basis::NodesPerCell; node++) {
    double weight = 1.0;
    for (int axis = 0, remainder = node; axis < DIMENSIONS; axis++, remainder /= Basis::NumberOfNodes) {
      weight *= Basis::weights()[remainder % Basis::NumberOfNodes];
    }
    average += weight * nodalState[node * NumberOfData];
  }
  VALIDATE_NUMERICAL_EQUALS(average, 1.0);
}

/**
 * The slope 1.2 around the average 1 reaches the depth 0.4 on the lower face of the first axis,
 * where the term asks for 0.5. The largest admissible scaling 5 / 6 meets it there.
 */
UNIT_TEST("Admissibility is tested at the position of every point") {
  auto                                        nodalState                 = linearDepth(1.0, 1.2);
  const double                                averageState[NumberOfData] = {1.0, Level - 1.0};
  const tarch::la::Vector<DIMENSIONS, double> centre(0.5);
  const tarch::la::Vector<DIMENSIONS, double> size(1.0);
  const double theta = Scaling::limit<DeeperOnLowerFace, NumberOfData>(
    averageState,
    nodalState.data(),
    centre,
    size,
    DeeperOnLowerFace::RuntimeParameters{}
  );
  VALIDATE(std::abs(theta - 5.0 / 6.0) <= 1e-6, theta, 5.0 / 6.0);
}

UNIT_TEST("Inadmissible average gives constant") {
  auto nodalState = linearDepth(-0.1, 1.0);
  VALIDATE_NUMERICAL_EQUALS(limit(-0.1, nodalState), 0.0);
  for (int node = 0; node < Basis::NodesPerCell; node++) {
    VALIDATE_NUMERICAL_EQUALS(nodalState[node * NumberOfData], -0.1);
  }
}
