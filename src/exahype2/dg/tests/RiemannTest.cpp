// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RiemannTest.h"

#include "exahype2/dg/Riemann.h"

namespace {
  constexpr int NumberOfDofsPerAxis = 2;
  constexpr int NumberOfUnknowns    = 2;
  constexpr int NodesPerFace = (DIMENSIONS == 3) ? NumberOfDofsPerAxis * NumberOfDofsPerAxis : NumberOfDofsPerAxis;

  // Reports the first entry of the state as the wave speed, so a test can steer
  // the eigenvalue of each side from the data it hands in.
  struct FirstEntryEigenvalueSolver final {
    static double maxEigenvalue(
      const double* NOALIAS const                                   state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal
    ) {
      return state[0];
    }

    // Accumulation verifies that the kernel clears callback outputs at every node.
    static void flux(
      const double* NOALIAS const                                   state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal,
      double* NOALIAS const                                         fluxValues
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        fluxValues[unknown] += state[unknown];
      }
    }

    static void nonconservativeProduct(
      [[maybe_unused]] const double* NOALIAS const                  state,
      const double* NOALIAS const                                   stateJump,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal,
      double* NOALIAS const                                         productValues
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        productValues[unknown] += stateJump[unknown];
      }
    }

    // Doubles whatever it is handed, which is enough to show the matrix ran.
    static void multiplyMaterialParameterMatrix(
      [[maybe_unused]] const double* NOALIAS const                  state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal,
      double* NOALIAS const                                         values
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        values[unknown] *= 2.0;
      }
    }
  };

  // Reads the jump of the first auxiliary variable, the way the bathymetry
  // enters the shallow-water equations.
  struct AuxiliaryJumpSolver final {
    static double maxEigenvalue(
      const double* NOALIAS const                                   state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal
    ) {
      return state[0];
    }

    static void nonconservativeProduct(
      [[maybe_unused]] const double* NOALIAS const                  state,
      const double* NOALIAS const                                   stateJump,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal,
      double* NOALIAS const                                         productValues
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        productValues[unknown] += (1.0 + unknown) * stateJump[NumberOfUnknowns];
      }
    }
  };

  // Weights that sum to one, so a uniform state averages to itself.
  void fillNormalisedWeights(double (&faceQuadratureWeights)[NodesPerFace]) {
    for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
      faceQuadratureWeights[faceNode] = 1.0 / NodesPerFace;
    }
  }
} // namespace

exahype2::dg::tests::RiemannTest::RiemannTest() :
  tarch::tests::TestCase("exahype2::dg::tests::RiemannTest") {}

void exahype2::dg::tests::RiemannTest::run() {
  testMethod(testEqualStatesAverageTheTwoFluxes);
  testMethod(testJumpTermScalesWithTheLargerWaveSpeed);
  testMethod(testBothSidesSeeTheSameFluxWithoutANonconservativeProduct);
  testMethod(testNonconservativeProductSplitsTheTwoSides);
  testMethod(testNonconservativeProductReadsTheAuxiliaryJump);
  testMethod(testLinearFormBuildsTheFluxFromTheStateJump);
  testMethod(testLinearFormAddsTheNonconservativeProduct);
  testMethod(testLinearFormAppliesTheMaterialParameterMatrix);
}

void exahype2::dg::tests::RiemannTest::testEqualStatesAverageTheTwoFluxes() {
  constexpr int NumberOfAuxiliaryVariables = 0;
  constexpr int NumberOfData               = NumberOfUnknowns + NumberOfAuxiliaryVariables;

  double faceQuadratureWeights[NodesPerFace];
  fillNormalisedWeights(faceQuadratureWeights);

  double leftFlux[NodesPerFace * NumberOfUnknowns];
  double rightFlux[NodesPerFace * NumberOfUnknowns];
  double leftState[NodesPerFace * NumberOfData];
  double rightState[NodesPerFace * NumberOfData];
  for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      leftFlux[faceNode * NumberOfUnknowns + unknown]  = 1.0 + unknown;
      rightFlux[faceNode * NumberOfUnknowns + unknown] = 3.0 + unknown;
    }
    for (int value = 0; value < NumberOfData; value++) {
      leftState[faceNode * NumberOfData + value]  = 2.0;
      rightState[faceNode * NumberOfData + value] = 2.0;
    }
  }

  const tarch::la::Vector<DIMENSIONS, double> faceCentre(0.5);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(1.0);

  exahype2::dg::solveRiemannProblem<
    FirstEntryEigenvalueSolver,
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    false,
    double>(leftFlux, rightFlux, leftState, rightState, faceQuadratureWeights, faceCentre, cellSize, 0.0, 1.0, 0);

  for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      validateNumericalEquals(leftFlux[faceNode * NumberOfUnknowns + unknown], 2.0 + unknown);
      validateNumericalEquals(rightFlux[faceNode * NumberOfUnknowns + unknown], 2.0 + unknown);
    }
  }
}

void exahype2::dg::tests::RiemannTest::testJumpTermScalesWithTheLargerWaveSpeed() {
  constexpr int NumberOfAuxiliaryVariables = 0;
  constexpr int NumberOfData               = NumberOfUnknowns + NumberOfAuxiliaryVariables;

  double faceQuadratureWeights[NodesPerFace];
  fillNormalisedWeights(faceQuadratureWeights);

  double leftFlux[NodesPerFace * NumberOfUnknowns]  = {};
  double rightFlux[NodesPerFace * NumberOfUnknowns] = {};
  double leftState[NodesPerFace * NumberOfData];
  double rightState[NodesPerFace * NumberOfData];
  for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
    const int firstEntry       = faceNode * NumberOfData;
    leftState[firstEntry]      = 3.0;
    leftState[firstEntry + 1]  = 1.0;
    rightState[firstEntry]     = 5.0;
    rightState[firstEntry + 1] = 4.0;
  }

  const tarch::la::Vector<DIMENSIONS, double> faceCentre(0.5);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(1.0);

  exahype2::dg::solveRiemannProblem<
    FirstEntryEigenvalueSolver,
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    false,
    double>(leftFlux, rightFlux, leftState, rightState, faceQuadratureWeights, faceCentre, cellSize, 0.0, 1.0, 0);

  // The right side averages to 5, the left one to 3, so the larger speed wins.
  constexpr double ExpectedMaxEigenvalue = 5.0;
  for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
    const int firstEntry = faceNode * NumberOfUnknowns;
    validateNumericalEquals(leftFlux[firstEntry], 0.5 * ExpectedMaxEigenvalue * (3.0 - 5.0));
    validateNumericalEquals(leftFlux[firstEntry + 1], 0.5 * ExpectedMaxEigenvalue * (1.0 - 4.0));
  }
}

void exahype2::dg::tests::RiemannTest::testBothSidesSeeTheSameFluxWithoutANonconservativeProduct() {
  constexpr int NumberOfAuxiliaryVariables = 1;
  constexpr int NumberOfData               = NumberOfUnknowns + NumberOfAuxiliaryVariables;

  double faceQuadratureWeights[NodesPerFace];
  fillNormalisedWeights(faceQuadratureWeights);

  double leftFlux[NodesPerFace * NumberOfUnknowns];
  double rightFlux[NodesPerFace * NumberOfUnknowns];
  double leftState[NodesPerFace * NumberOfData];
  double rightState[NodesPerFace * NumberOfData];
  for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      leftFlux[faceNode * NumberOfUnknowns + unknown]  = 0.5 * faceNode + unknown;
      rightFlux[faceNode * NumberOfUnknowns + unknown] = 1.5 * faceNode - unknown;
    }
    for (int value = 0; value < NumberOfData; value++) {
      leftState[faceNode * NumberOfData + value]  = 1.0 + 0.25 * value;
      rightState[faceNode * NumberOfData + value] = 2.0 - 0.25 * value;
    }
  }

  const tarch::la::Vector<DIMENSIONS, double> faceCentre(0.5);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(1.0);

  exahype2::dg::solveRiemannProblem<
    FirstEntryEigenvalueSolver,
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    false,
    double>(leftFlux, rightFlux, leftState, rightState, faceQuadratureWeights, faceCentre, cellSize, 0.0, 1.0, 0);

  for (int entry = 0; entry < NodesPerFace * NumberOfUnknowns; entry++) {
    validateNumericalEquals(leftFlux[entry], rightFlux[entry]);
  }
}

void exahype2::dg::tests::RiemannTest::testNonconservativeProductSplitsTheTwoSides() {
  constexpr int NumberOfAuxiliaryVariables = 0;
  constexpr int NumberOfData               = NumberOfUnknowns + NumberOfAuxiliaryVariables;

  double faceQuadratureWeights[NodesPerFace];
  fillNormalisedWeights(faceQuadratureWeights);

  double leftFlux[NodesPerFace * NumberOfUnknowns]  = {};
  double rightFlux[NodesPerFace * NumberOfUnknowns] = {};
  double leftState[NodesPerFace * NumberOfData];
  double rightState[NodesPerFace * NumberOfData];
  for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
    for (int value = 0; value < NumberOfData; value++) {
      leftState[faceNode * NumberOfData + value]  = 1.0;
      rightState[faceNode * NumberOfData + value] = 1.0 + value;
    }
  }

  const tarch::la::Vector<DIMENSIONS, double> faceCentre(0.5);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(1.0);

  exahype2::dg::solveRiemannProblem<
    FirstEntryEigenvalueSolver,
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    true,
    double>(leftFlux, rightFlux, leftState, rightState, faceQuadratureWeights, faceCentre, cellSize, 0.0, 1.0, 0);

  // The stub returns the state jump, which the kernel adds on one side and
  // subtracts on the other, so the two sides differ by exactly that jump.
  for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      const double stateJump =
        rightState[faceNode * NumberOfData + unknown] - leftState[faceNode * NumberOfData + unknown];
      validateNumericalEquals(
        leftFlux[faceNode * NumberOfUnknowns + unknown] - rightFlux[faceNode * NumberOfUnknowns + unknown],
        stateJump
      );
    }
  }
}

/**
 * The buffer the nonconservative product reads carries the jump of every
 * auxiliary variable next to the jump of the unknowns.
 */
void exahype2::dg::tests::RiemannTest::testNonconservativeProductReadsTheAuxiliaryJump() {
  constexpr int NumberOfAuxiliaryVariables = 1;
  constexpr int NumberOfData               = NumberOfUnknowns + NumberOfAuxiliaryVariables;

  double faceQuadratureWeights[NodesPerFace];
  fillNormalisedWeights(faceQuadratureWeights);

  double leftFlux[NodesPerFace * NumberOfUnknowns]  = {};
  double rightFlux[NodesPerFace * NumberOfUnknowns] = {};
  double leftState[NodesPerFace * NumberOfData];
  double rightState[NodesPerFace * NumberOfData];
  for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
    const int firstEntry = faceNode * NumberOfData;
    // The unknowns carry no jump, so only the auxiliary variable can reach the flux.
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      leftState[firstEntry + unknown]  = 1.0;
      rightState[firstEntry + unknown] = 1.0;
    }
    leftState[firstEntry + NumberOfUnknowns]  = 2.0 + faceNode;
    rightState[firstEntry + NumberOfUnknowns] = 5.0 + 2.0 * faceNode;
  }

  const tarch::la::Vector<DIMENSIONS, double> faceCentre(0.5);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(1.0);

  exahype2::dg::solveRiemannProblem<
    AuxiliaryJumpSolver,
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    true,
    double>(leftFlux, rightFlux, leftState, rightState, faceQuadratureWeights, faceCentre, cellSize, 0.0, 1.0, 0);

  for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
    const double auxiliaryJump =
      rightState[faceNode * NumberOfData + NumberOfUnknowns] - leftState[faceNode * NumberOfData + NumberOfUnknowns];
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      const double halfProduct = 0.5 * (1.0 + unknown) * auxiliaryJump;
      validateNumericalEqualsWithParams2(
        leftFlux[faceNode * NumberOfUnknowns + unknown],
        halfProduct,
        faceNode,
        unknown
      );
      validateNumericalEqualsWithParams2(
        rightFlux[faceNode * NumberOfUnknowns + unknown],
        -halfProduct,
        faceNode,
        unknown
      );
    }
  }
}

void exahype2::dg::tests::RiemannTest::testLinearFormBuildsTheFluxFromTheStateJump() {
  constexpr int NumberOfAuxiliaryVariables = 0;
  constexpr int NumberOfData               = NumberOfUnknowns + NumberOfAuxiliaryVariables;

  double faceQuadratureWeights[NodesPerFace];
  fillNormalisedWeights(faceQuadratureWeights);

  // Nonzero incoming values prove the linear form starts from a clean slate.
  double leftFlux[NodesPerFace * NumberOfUnknowns];
  double rightFlux[NodesPerFace * NumberOfUnknowns];
  double leftState[NodesPerFace * NumberOfData];
  double rightState[NodesPerFace * NumberOfData];
  for (int entry = 0; entry < NodesPerFace * NumberOfUnknowns; entry++) {
    leftFlux[entry]  = 9.0;
    rightFlux[entry] = -9.0;
  }
  for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
    for (int value = 0; value < NumberOfData; value++) {
      leftState[faceNode * NumberOfData + value]  = 1.0;
      rightState[faceNode * NumberOfData + value] = 3.0;
    }
  }

  const tarch::la::Vector<DIMENSIONS, double> faceCentre(0.5);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(1.0);

  exahype2::dg::solveLinearRiemannProblem<
    FirstEntryEigenvalueSolver,
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    true,
    false,
    false,
    double>(leftFlux, rightFlux, leftState, rightState, faceQuadratureWeights, faceCentre, cellSize, 0.0, 1.0, 0);

  // Half the jump is one, the stub flux returns it unchanged, and the wave
  // speed is the averaged first entry, which is three.
  constexpr double ExpectedFlux          = 1.0;
  constexpr double ExpectedMaxEigenvalue = 3.0;
  constexpr double ExpectedHalfStateJump = 1.0;
  for (int entry = 0; entry < NodesPerFace * NumberOfUnknowns; entry++) {
    validateNumericalEquals(leftFlux[entry], ExpectedFlux - ExpectedMaxEigenvalue * ExpectedHalfStateJump);
    validateNumericalEquals(rightFlux[entry], ExpectedFlux + ExpectedMaxEigenvalue * ExpectedHalfStateJump);
  }
}

void exahype2::dg::tests::RiemannTest::testLinearFormAddsTheNonconservativeProduct() {
  constexpr int NumberOfAuxiliaryVariables = 0;
  constexpr int NumberOfData               = NumberOfUnknowns + NumberOfAuxiliaryVariables;

  double faceQuadratureWeights[NodesPerFace];
  fillNormalisedWeights(faceQuadratureWeights);

  double leftFlux[NodesPerFace * NumberOfUnknowns]  = {};
  double rightFlux[NodesPerFace * NumberOfUnknowns] = {};
  double leftState[NodesPerFace * NumberOfData];
  double rightState[NodesPerFace * NumberOfData];
  for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
    for (int value = 0; value < NumberOfData; value++) {
      leftState[faceNode * NumberOfData + value]  = 1.0;
      rightState[faceNode * NumberOfData + value] = 3.0;
    }
  }

  const tarch::la::Vector<DIMENSIONS, double> faceCentre(0.5);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(1.0);

  exahype2::dg::solveLinearRiemannProblem<
    FirstEntryEigenvalueSolver,
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    true,
    true,
    false,
    double>(leftFlux, rightFlux, leftState, rightState, faceQuadratureWeights, faceCentre, cellSize, 0.0, 1.0, 0);

  // Flux and nonconservative product both return half the jump, so the central
  // part doubles while the dissipation stays as it was.
  constexpr double ExpectedCentralFlux   = 2.0;
  constexpr double ExpectedMaxEigenvalue = 3.0;
  constexpr double ExpectedHalfStateJump = 1.0;
  for (int entry = 0; entry < NodesPerFace * NumberOfUnknowns; entry++) {
    validateNumericalEquals(leftFlux[entry], ExpectedCentralFlux - ExpectedMaxEigenvalue * ExpectedHalfStateJump);
    validateNumericalEquals(rightFlux[entry], ExpectedCentralFlux + ExpectedMaxEigenvalue * ExpectedHalfStateJump);
  }
}

void exahype2::dg::tests::RiemannTest::testLinearFormAppliesTheMaterialParameterMatrix() {
  constexpr int NumberOfAuxiliaryVariables = 0;
  constexpr int NumberOfData               = NumberOfUnknowns + NumberOfAuxiliaryVariables;

  double faceQuadratureWeights[NodesPerFace];
  fillNormalisedWeights(faceQuadratureWeights);

  double leftFlux[NodesPerFace * NumberOfUnknowns]  = {};
  double rightFlux[NodesPerFace * NumberOfUnknowns] = {};
  double leftState[NodesPerFace * NumberOfData];
  double rightState[NodesPerFace * NumberOfData];
  for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
    for (int value = 0; value < NumberOfData; value++) {
      leftState[faceNode * NumberOfData + value]  = 1.0;
      rightState[faceNode * NumberOfData + value] = 3.0;
    }
  }

  const tarch::la::Vector<DIMENSIONS, double> faceCentre(0.5);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(1.0);

  exahype2::dg::solveLinearRiemannProblem<
    FirstEntryEigenvalueSolver,
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    true,
    false,
    true,
    double>(leftFlux, rightFlux, leftState, rightState, faceQuadratureWeights, faceCentre, cellSize, 0.0, 1.0, 0);

  // The material parameter matrix doubles the flux the stub produced.
  constexpr double ExpectedCentralFlux   = 2.0;
  constexpr double ExpectedMaxEigenvalue = 3.0;
  constexpr double ExpectedHalfStateJump = 1.0;
  for (int entry = 0; entry < NodesPerFace * NumberOfUnknowns; entry++) {
    validateNumericalEquals(leftFlux[entry], ExpectedCentralFlux - ExpectedMaxEigenvalue * ExpectedHalfStateJump);
    validateNumericalEquals(rightFlux[entry], ExpectedCentralFlux + ExpectedMaxEigenvalue * ExpectedHalfStateJump);
  }
}
