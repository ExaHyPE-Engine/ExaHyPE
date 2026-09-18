// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FaceIntegralTest.h"

#include "exahype2/dg/FaceIntegral.h"

namespace {
  constexpr int NumberOfDofsPerAxis = 3;
  constexpr int NumberOfUnknowns    = 2;
  constexpr int NodesPerFace = (DIMENSIONS == 3) ? NumberOfDofsPerAxis * NumberOfDofsPerAxis : NumberOfDofsPerAxis;
  constexpr int NodesPerCell = NodesPerFace * NumberOfDofsPerAxis;

  struct Tables final {
    double faceQuadratureWeights[NodesPerFace]{};
    double inverseCellQuadratureWeights[NodesPerCell]{};
    double leftFaceCoefficients[NumberOfDofsPerAxis]{};
    double rightFaceCoefficients[NumberOfDofsPerAxis]{};
  };

  // Distinct, non-symmetric entries, so an index mix-up cannot cancel out.
  Tables makeTables() {
    Tables tables;
    for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
      tables.faceQuadratureWeights[faceNode] = 0.25 + 0.125 * faceNode;
    }
    for (int cellNode = 0; cellNode < NodesPerCell; cellNode++) {
      tables.inverseCellQuadratureWeights[cellNode] = 1.0 + 0.5 * cellNode;
    }
    for (int index = 0; index < NumberOfDofsPerAxis; index++) {
      tables.leftFaceCoefficients[index]  = 1.0 + index;
      tables.rightFaceCoefficients[index] = 4.0 + index;
    }
    return tables;
  }

  // Returns the cell node that carries the given face node at the given index
  // along the normal axis.
  int cellNodeOf(const int direction, const int faceNode, const int normalIndex) {
    int lowerAxesStride = 1;
    for (int dimension = 0; dimension < direction; dimension++) {
      lowerAxesStride *= NumberOfDofsPerAxis;
    }
    const int lowerAxesIndex  = faceNode % lowerAxesStride;
    const int higherAxesIndex = faceNode / lowerAxesStride;
    return (higherAxesIndex * NumberOfDofsPerAxis + normalIndex) * lowerAxesStride + lowerAxesIndex;
  }
} // namespace

exahype2::dg::tests::FaceIntegralTest::FaceIntegralTest() :
  tarch::tests::TestCase("exahype2::dg::tests::FaceIntegralTest") {}

void exahype2::dg::tests::FaceIntegralTest::run() {
  testMethod(testZeroFluxLeavesUpdateUntouched);
  testMethod(testEachCellNodeReceivesItsOwnFaceNode);
  testMethod(testUpdateStridesByTheUnknowns);
  testMethod(testOrientationFlipsSignOnlyForTheNonlinearForm);
  testMethod(testWeightsAndCoefficientsEnterAsAProduct);
}

void exahype2::dg::tests::FaceIntegralTest::testZeroFluxLeavesUpdateUntouched() {
  const Tables tables                                    = makeTables();
  const double faceFlux[NodesPerFace * NumberOfUnknowns] = {};

  for (int direction = 0; direction < DIMENSIONS; direction++) {
    for (int orientation = 0; orientation < 2; orientation++) {
      double cellUpdate[NodesPerCell * NumberOfUnknowns];
      for (int entry = 0; entry < NodesPerCell * NumberOfUnknowns; entry++) {
        cellUpdate[entry] = 1.0 + entry;
      }

      exahype2::dg::addFaceIntegral<NumberOfDofsPerAxis, NumberOfUnknowns, false, double, double>(
        cellUpdate,
        faceFlux,
        tables.faceQuadratureWeights,
        tables.inverseCellQuadratureWeights,
        tables.leftFaceCoefficients,
        tables.rightFaceCoefficients,
        direction,
        orientation,
        2.0,
        0.5
      );

      for (int entry = 0; entry < NodesPerCell * NumberOfUnknowns; entry++) {
        validateNumericalEquals(cellUpdate[entry], 1.0 + entry);
      }
    }
  }
}

void exahype2::dg::tests::FaceIntegralTest::testEachCellNodeReceivesItsOwnFaceNode() {
  Tables tables = makeTables();
  for (int cellNode = 0; cellNode < NodesPerCell; cellNode++) {
    tables.inverseCellQuadratureWeights[cellNode] = 1.0;
  }

  for (int direction = 0; direction < DIMENSIONS; direction++) {
    // Only one face node carries a flux, so the cell nodes that pick it up are
    // exactly the ones the lifting is supposed to reach.
    for (int markedFaceNode = 0; markedFaceNode < NodesPerFace; markedFaceNode++) {
      double    faceFlux[NodesPerFace * NumberOfUnknowns] = {};
      const int markedEntry                               = markedFaceNode * NumberOfUnknowns;
      faceFlux[markedEntry]                               = 1.0;

      double cellUpdate[NodesPerCell * NumberOfUnknowns] = {};

      exahype2::dg::addFaceIntegral<NumberOfDofsPerAxis, NumberOfUnknowns, true, double, double>(
        cellUpdate,
        faceFlux,
        tables.faceQuadratureWeights,
        tables.inverseCellQuadratureWeights,
        tables.leftFaceCoefficients,
        tables.rightFaceCoefficients,
        direction,
        0,
        1.0,
        1.0
      );

      double expectedSum = 0.0;
      for (int normalIndex = 0; normalIndex < NumberOfDofsPerAxis; normalIndex++) {
        const int    cellNode   = cellNodeOf(direction, markedFaceNode, normalIndex);
        const int    firstEntry = cellNode * NumberOfUnknowns;
        const double expectedValue =
          -tables.faceQuadratureWeights[markedFaceNode] * tables.leftFaceCoefficients[normalIndex];
        validateNumericalEquals(cellUpdate[firstEntry], expectedValue);
        validateNumericalEquals(cellUpdate[firstEntry + 1], 0.0);
        expectedSum += expectedValue;
      }

      double sumOfUpdates = 0.0;
      for (int entry = 0; entry < NodesPerCell * NumberOfUnknowns; entry++) {
        sumOfUpdates += cellUpdate[entry];
      }
      validateNumericalEquals(sumOfUpdates, expectedSum);
    }
  }
}

/**
 * The update holds NumberOfUnknowns entries per cell node, so every unknown of
 * every cell node receives its own face value and the entries behind the last
 * cell node stay as the caller left them.
 */
void exahype2::dg::tests::FaceIntegralTest::testUpdateStridesByTheUnknowns() {
  constexpr int    Direction    = DIMENSIONS - 1;
  constexpr int    Orientation  = 1;
  constexpr int    GuardEntries = 4;
  constexpr double GuardValue   = 7.0;

  const Tables tables = makeTables();
  double       faceFlux[NodesPerFace * NumberOfUnknowns];
  for (int entry = 0; entry < NodesPerFace * NumberOfUnknowns; entry++) {
    faceFlux[entry] = 1.0 + entry;
  }

  double cellUpdate[NodesPerCell * NumberOfUnknowns + GuardEntries] = {};
  for (int entry = 0; entry < GuardEntries; entry++) {
    cellUpdate[NodesPerCell * NumberOfUnknowns + entry] = GuardValue;
  }

  exahype2::dg::addFaceIntegral<NumberOfDofsPerAxis, NumberOfUnknowns, false, double, double>(
    cellUpdate,
    faceFlux,
    tables.faceQuadratureWeights,
    tables.inverseCellQuadratureWeights,
    tables.leftFaceCoefficients,
    tables.rightFaceCoefficients,
    Direction,
    Orientation,
    1.0,
    1.0
  );

  constexpr double Scaling = 2.0 * Orientation - 1.0;
  for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
    for (int normalIndex = 0; normalIndex < NumberOfDofsPerAxis; normalIndex++) {
      const int cellNode = cellNodeOf(Direction, faceNode, normalIndex);
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        const double expectedValue = -tables.inverseCellQuadratureWeights[cellNode] * Scaling
          * tables.faceQuadratureWeights[faceNode] * tables.rightFaceCoefficients[normalIndex]
          * faceFlux[faceNode * NumberOfUnknowns + unknown];
        validateNumericalEqualsWithParams2(
          cellUpdate[cellNode * NumberOfUnknowns + unknown],
          expectedValue,
          cellNode,
          unknown
        );
      }
    }
  }

  for (int entry = 0; entry < GuardEntries; entry++) {
    validateNumericalEqualsWithParams1(cellUpdate[NodesPerCell * NumberOfUnknowns + entry], GuardValue, entry);
  }
}

void exahype2::dg::tests::FaceIntegralTest::testOrientationFlipsSignOnlyForTheNonlinearForm() {
  Tables tables = makeTables();
  // Both faces use the same coefficient table here, so the only difference left
  // between the orientations is the scaling.
  for (int index = 0; index < NumberOfDofsPerAxis; index++) {
    tables.rightFaceCoefficients[index] = tables.leftFaceCoefficients[index];
  }

  double faceFlux[NodesPerFace * NumberOfUnknowns];
  for (int entry = 0; entry < NodesPerFace * NumberOfUnknowns; entry++) {
    faceFlux[entry] = 1.0 + entry;
  }

  for (int direction = 0; direction < DIMENSIONS; direction++) {
    double nonlinearLowerFace[NodesPerCell * NumberOfUnknowns] = {};
    double nonlinearUpperFace[NodesPerCell * NumberOfUnknowns] = {};
    double linearLowerFace[NodesPerCell * NumberOfUnknowns]    = {};
    double linearUpperFace[NodesPerCell * NumberOfUnknowns]    = {};

    for (int orientation = 0; orientation < 2; orientation++) {
      exahype2::dg::addFaceIntegral<NumberOfDofsPerAxis, NumberOfUnknowns, false, double, double>(
        (orientation == 0) ? nonlinearLowerFace : nonlinearUpperFace,
        faceFlux,
        tables.faceQuadratureWeights,
        tables.inverseCellQuadratureWeights,
        tables.leftFaceCoefficients,
        tables.rightFaceCoefficients,
        direction,
        orientation,
        1.0,
        1.0
      );
      exahype2::dg::addFaceIntegral<NumberOfDofsPerAxis, NumberOfUnknowns, true, double, double>(
        (orientation == 0) ? linearLowerFace : linearUpperFace,
        faceFlux,
        tables.faceQuadratureWeights,
        tables.inverseCellQuadratureWeights,
        tables.leftFaceCoefficients,
        tables.rightFaceCoefficients,
        direction,
        orientation,
        1.0,
        1.0
      );
    }

    for (int entry = 0; entry < NodesPerCell * NumberOfUnknowns; entry++) {
      validateNumericalEquals(nonlinearLowerFace[entry], -nonlinearUpperFace[entry]);
      validateNumericalEquals(linearLowerFace[entry], linearUpperFace[entry]);
      validateNumericalEquals(linearUpperFace[entry], nonlinearUpperFace[entry]);
    }
  }
}

void exahype2::dg::tests::FaceIntegralTest::testWeightsAndCoefficientsEnterAsAProduct() {
  constexpr int    Direction       = 0;
  constexpr int    Orientation     = 1;
  constexpr double InverseCellSize = 3.0;
  constexpr double TimeStepSize    = 0.25;

  const Tables tables = makeTables();
  double       faceFlux[NodesPerFace * NumberOfUnknowns];
  for (int entry = 0; entry < NodesPerFace * NumberOfUnknowns; entry++) {
    faceFlux[entry] = 0.5 + 0.25 * entry;
  }

  double cellUpdate[NodesPerCell * NumberOfUnknowns] = {};

  exahype2::dg::addFaceIntegral<NumberOfDofsPerAxis, NumberOfUnknowns, false, double, double>(
    cellUpdate,
    faceFlux,
    tables.faceQuadratureWeights,
    tables.inverseCellQuadratureWeights,
    tables.leftFaceCoefficients,
    tables.rightFaceCoefficients,
    Direction,
    Orientation,
    InverseCellSize,
    TimeStepSize
  );

  const double scaling = (2.0 * Orientation - 1.0) * InverseCellSize;
  for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
    const double scaledWeight = scaling * TimeStepSize * tables.faceQuadratureWeights[faceNode];
    for (int normalIndex = 0; normalIndex < NumberOfDofsPerAxis; normalIndex++) {
      const int cellNode = cellNodeOf(Direction, faceNode, normalIndex);
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        const double expectedValue = -tables.inverseCellQuadratureWeights[cellNode] * scaledWeight
          * tables.rightFaceCoefficients[normalIndex] * faceFlux[faceNode * NumberOfUnknowns + unknown];
        validateNumericalEquals(cellUpdate[cellNode * NumberOfUnknowns + unknown], expectedValue);
      }
    }
  }
}
