// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CorrectorTest.h"

#include "exahype2/dg/Basis.h"
#include "exahype2/dg/Kernels.h"
#include "exahype2/dg/ader/Corrector.h"
#include "peano4/Constants.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

namespace {
  constexpr double CellExtent     = 2.0;
  constexpr double TimeStamp      = 0.25;
  constexpr double TimeStepSize   = 0.5;
  constexpr double WaveSpeed      = 3.0;
  constexpr double UntouchedValue = -7.0;

  /**
   * A solver whose wave speed is nonzero only at the end of the time step of the tests.
   */
  struct WaveSpeedSolver final {
    static constexpr int DGOrder                    = 1;
    static constexpr int NumberOfUnknowns           = 1;
    static constexpr int NumberOfAuxiliaryVariables = 0;

    static INLINE_METHOD GPU_CALLABLE_METHOD double maxEigenvalue(
      [[maybe_unused]] const double* NOALIAS const                  state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      const double                                                  timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      [[maybe_unused]] const int                                    direction
    ) {
      return timeStamp == TimeStamp + TimeStepSize ? WaveSpeed : 0.0;
    }
  };

  using CellLayout = exahype2::dg::PaddedLayout<WaveSpeedSolver>;

  constexpr int NodesPerAxis        = CellLayout::NodesPerAxis;
  constexpr int FluxElementsPerFace = CellLayout::NodesPerFace * CellLayout::NumberOfUnknowns;
  constexpr int CellStateSize       = CellLayout::NodesPerCell * CellLayout::NumberOfData;

  /**
   * The nodes lie at the two ends of the reference interval, so a face integral changes only the
   * nodes on that face.
   */
  struct Tables final {
    double faceQuadratureWeights[CellLayout::NodesPerFace]{};
    double inverseCellQuadratureWeights[CellLayout::NodesPerCell]{};
    double quadraturePoints[NodesPerAxis]{0.0, 1.0};
    double leftFaceCoefficients[NodesPerAxis]{1.0, 0.0};
    double rightFaceCoefficients[NodesPerAxis]{0.0, 1.0};
  };

  Tables getTables() {
    Tables tables;
    for (int faceNode = 0; faceNode < CellLayout::NodesPerFace; faceNode++) {
      tables.faceQuadratureWeights[faceNode] = 1.0;
    }
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      tables.inverseCellQuadratureWeights[cellNode] = 1.0;
    }
    return tables;
  }

  /**
   * Returns the cell node of face node faceNode at position normalIndex along direction.
   */
  int getCellNodeOnFace(const int direction, const int faceNode, const int normalIndex) {
    int lowerAxesStride = 1;
    for (int axis = 0; axis < direction; axis++) {
      lowerAxesStride *= NodesPerAxis;
    }
    const int lowerAxesIndex  = faceNode % lowerAxesStride;
    const int higherAxesIndex = faceNode / lowerAxesStride;
    return (higherAxesIndex * NodesPerAxis + normalIndex) * lowerAxesStride + lowerAxesIndex;
  }

  int getStateIndexOf(const int cellNode) { return cellNode * CellLayout::NumberOfData; }

  double getCellStateEntry(const int cellNode) { return 1.0 + 0.25 * cellNode; }

  double getLowerFluxEntry(const int direction, const int faceNode) { return 2.0 + direction + 0.5 * faceNode; }

  double getUpperFluxEntry(const int direction, const int faceNode) { return 8.0 + direction + 0.5 * faceNode; }

  tarch::la::Vector<DIMENSIONS, double> getCellCentre() {
    return tarch::la::Vector<DIMENSIONS, double>(0.5 * CellExtent);
  }

  tarch::la::Vector<DIMENSIONS, double> getCellSize() { return tarch::la::Vector<DIMENSIONS, double>(CellExtent); }

  struct Cell final {
    double        faceFluxes[peano4::FacesPerCell][FluxElementsPerFace]{};
    double        cellState[CellStateSize]{};
    const double* fluxHalves[peano4::FacesPerCell]{};
  };

  void fill(Cell& cell) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      cell.cellState[getStateIndexOf(cellNode)] = getCellStateEntry(cellNode);
    }
    for (int face = 0; face < peano4::FacesPerCell; face++) {
      for (int entry = 0; entry < FluxElementsPerFace; entry++) {
        cell.faceFluxes[face][entry] = UntouchedValue;
      }
      cell.fluxHalves[face] = cell.faceFluxes[face];
    }
    for (int direction = 0; direction < DIMENSIONS; direction++) {
      for (int faceNode = 0; faceNode < CellLayout::NodesPerFace; faceNode++) {
        cell.faceFluxes[direction][faceNode]              = getLowerFluxEntry(direction, faceNode);
        cell.faceFluxes[direction + DIMENSIONS][faceNode] = getUpperFluxEntry(direction, faceNode);
      }
    }
  }

  /**
   * Writes the expected cell state after the correction: Every face subtracts its flux difference
   * times the time step size over the cell extent from the nodes on that face.
   */
  void setExpectedCellState(double (&expected)[CellStateSize]) {
    constexpr double Contribution = TimeStepSize / CellExtent;
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      expected[getStateIndexOf(cellNode)] = getCellStateEntry(cellNode);
    }
    for (int direction = 0; direction < DIMENSIONS; direction++) {
      for (int faceNode = 0; faceNode < CellLayout::NodesPerFace; faceNode++) {
        expected[getStateIndexOf(getCellNodeOnFace(direction, faceNode, 0))] -=
          Contribution * getLowerFluxEntry(direction, faceNode);
        expected[getStateIndexOf(getCellNodeOnFace(direction, faceNode, NodesPerAxis - 1))] -=
          Contribution * getUpperFluxEntry(direction, faceNode);
      }
    }
  }

  template <bool ReportEigenvalues>
  exahype2::dg::MaxEigenvalueStatistics correct(Cell& cell, const Tables& tables, int& postprocessingCalls) {
    return exahype2::dg::ader::Corrector<WaveSpeedSolver, double>::correct<ReportEigenvalues>(
      getCellCentre(),
      getCellSize(),
      cell.fluxHalves,
      cell.cellState,
      tables.faceQuadratureWeights,
      tables.inverseCellQuadratureWeights,
      tables.leftFaceCoefficients,
      tables.rightFaceCoefficients,
      tables.quadraturePoints,
      TimeStamp,
      TimeStepSize,
      [&]() { postprocessingCalls++; }
    );
  }
} // namespace

exahype2::dg::ader::tests::CorrectorTest::CorrectorTest() :
  tarch::tests::TestCase("exahype2::dg::ader::tests::CorrectorTest") {}

void
exahype2::dg::ader::tests::CorrectorTest::run() {
  testMethod(testTheFaceIntegralsReachTheCellState);
  testMethod(testZeroFluxDifferencesLeaveTheCellStateBitwise);
  testMethod(testTheCorrectionReportsTheWaveSpeedsOfTheUpdatedState);
}

/**
 * The face integrals change the nodes on each face by the flux difference of that face.
 */
void
exahype2::dg::ader::tests::CorrectorTest::testTheFaceIntegralsReachTheCellState() {
  const Tables tables = getTables();
  Cell         cell;
  fill(cell);

  double expected[CellStateSize] = {};
  setExpectedCellState(expected);

  int        postprocessingCalls = 0;
  const auto statistics          = correct<false>(cell, tables, postprocessingCalls);
  validateEquals(postprocessingCalls, 1);
  validateNumericalEquals(statistics.maxPhysicalEigenvalue, 0.0);
  validateNumericalEquals(statistics.maxScaledEigenvalue, 0.0);

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    validateNumericalEqualsWithParams1(
      cell.cellState[getStateIndexOf(cellNode)],
      expected[getStateIndexOf(cellNode)],
      cellNode
    );
  }
}

/**
 * Zero flux differences leave the cell state unchanged bitwise.
 */
void
exahype2::dg::ader::tests::CorrectorTest::testZeroFluxDifferencesLeaveTheCellStateBitwise() {
  const Tables tables = getTables();
  Cell         cell;
  fill(cell);
  for (int face = 0; face < peano4::FacesPerCell; face++) {
    for (int entry = 0; entry < FluxElementsPerFace; entry++) {
      cell.faceFluxes[face][entry] = 0.0;
    }
  }
  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    cell.cellState[getStateIndexOf(cellNode)] = 1.0 / 3.0 + 0.1 * cellNode;
  }

  int postprocessingCalls = 0;
  correct<false>(cell, tables, postprocessingCalls);

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    validateEqualsWithParams1(cell.cellState[getStateIndexOf(cellNode)], 1.0 / 3.0 + 0.1 * cellNode, cellNode);
  }
}

/**
 * The corrector reports the wave speeds of the updated state at the new time stamp.
 */
void
exahype2::dg::ader::tests::CorrectorTest::testTheCorrectionReportsTheWaveSpeedsOfTheUpdatedState() {
  const Tables tables = getTables();
  Cell         cell;
  fill(cell);

  int        postprocessingCalls = 0;
  const auto statistics          = correct<true>(cell, tables, postprocessingCalls);

  validateEquals(postprocessingCalls, 1);
  validateNumericalEquals(statistics.maxPhysicalEigenvalue, WaveSpeed);
  validateNumericalEquals(statistics.maxScaledEigenvalue, DIMENSIONS * WaveSpeed / CellExtent);
}
