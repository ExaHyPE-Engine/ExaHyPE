// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "exahype2/dg/ader/Corrector.h"

#include "exahype2/dg/Basis.h"
#include "exahype2/dg/Kernels.h"
#include "peano4/Constants.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"
#include "tarch/testing/UnitTest.h"

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
    struct RuntimeParameters final {};

    static constexpr int DGOrder                    = 1;
    static constexpr int NumberOfUnknowns           = 1;
    static constexpr int NumberOfAuxiliaryVariables = 0;

    static INLINE_METHOD GPU_CALLABLE_METHOD double maxEigenvalue(
      [[maybe_unused]] const double* NOALIAS const                  state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      const double                                                  timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      [[maybe_unused]] const int                                    direction,
      [[maybe_unused]] const RuntimeParameters&                     parameters
    ) {
      return timeStamp == TimeStamp + TimeStepSize ? WaveSpeed : 0.0;
    }
  };

  using CellLayout                 = exahype2::dg::PaddedLayout<WaveSpeedSolver>;
  using CorrectorOfWaveSpeedSolver = exahype2::dg::ader::Corrector<WaveSpeedSolver, double, double>;

  constexpr int NodesPerAxis        = CellLayout::NodesPerAxis;
  constexpr int FluxElementsPerFace = CellLayout::NodesPerFace * CellLayout::NumberOfUnknowns;
  constexpr int CellStateSize       = CellLayout::NodesPerCell * CellLayout::NumberOfData;

  /**
   * The nodes lie at the two ends of the reference interval. A face integral changes only the nodes
   * on that face.
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
   * Writes the expected cell state after the correction. Every face subtracts its flux difference
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

  /**
   * Corrects every node of cell as a corrector team does: The face integrals of the node and, with
   * ReportEigenvalues, the wave speeds of the node at the end of the time step.
   */
  template <bool ReportEigenvalues>
  exahype2::dg::MaxEigenvalueStatistics correct(Cell& cell, const Tables& tables) {
    double inverseCellSize[DIMENSIONS];
    for (int direction = 0; direction < DIMENSIONS; direction++) {
      inverseCellSize[direction] = 1.0 / CellExtent;
    }
    exahype2::dg::MaxEigenvalueStatistics statistics;
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      CorrectorOfWaveSpeedSolver::correctNode(
        cell.cellState,
        cellNode,
        cell.fluxHalves,
        tables.faceQuadratureWeights,
        tables.inverseCellQuadratureWeights,
        tables.leftFaceCoefficients,
        tables.rightFaceCoefficients,
        inverseCellSize,
        TimeStepSize
      );
    }
    if constexpr (ReportEigenvalues) {
      for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
        statistics = exahype2::dg::MaxEigenvalueStatistics::combine(
          statistics,
          exahype2::dg::getMaxEigenvaluesOfNode<WaveSpeedSolver>(
            cellNode,
            cell.cellState,
            tables.quadraturePoints,
            getCellCentre(),
            getCellSize(),
            TimeStamp + TimeStepSize,
            TimeStepSize,
            WaveSpeedSolver::RuntimeParameters{}
          )
        );
      }
    }
    return statistics;
  }
} // namespace

UNIT_TEST("The face integrals change the nodes on each face by the flux difference of that face") {
  GIVEN("Tables with nodes at both ends of the reference interval and a cell with distinct fluxes on its faces") {
    const Tables tables = getTables();
    Cell         cell;
    fill(cell);

    WHEN("The corrector runs without reporting the wave speeds") {
      const auto statistics = correct<false>(cell, tables);

      THEN("Every face subtracts its flux times the time step over the cell extent from the nodes on that face") {
        double expected[CellStateSize] = {};
        setExpectedCellState(expected);
        for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
          VALIDATE_NUMERICAL_EQUALS(
            cell.cellState[getStateIndexOf(cellNode)],
            expected[getStateIndexOf(cellNode)],
            cellNode
          );
        }
      }

      THEN("Both wave-speed maxima stay zero") {
        VALIDATE_NUMERICAL_EQUALS(statistics.maxPhysicalEigenvalue, 0.0);
        VALIDATE_NUMERICAL_EQUALS(statistics.maxScaledEigenvalue, 0.0);
      }
    }
  }
}

UNIT_TEST("Faces whose flux differences are zero leave every node of the cell untouched bitwise") {
  GIVEN("Tables with nodes at both ends of the reference interval, zero fluxes and a cell state of rounded values") {
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

    WHEN("The corrector runs without reporting the wave speeds") {
      correct<false>(cell, tables);

      THEN("Every node keeps its value bitwise") {
        for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
          VALIDATE_EQUALS(cell.cellState[getStateIndexOf(cellNode)], 1.0 / 3.0 + 0.1 * cellNode, cellNode);
        }
      }
    }
  }
}

UNIT_TEST("The correction evaluates the wave speeds of the updated state at the updated time stamp") {
  GIVEN("Tables with nodes at both ends of the reference interval and a cell with distinct fluxes on its faces") {
    const Tables tables = getTables();
    Cell         cell;
    fill(cell);

    WHEN("The corrector runs and reports the wave speeds") {
      const auto statistics = correct<true>(cell, tables);

      THEN("The maxima equal the wave speed and the wave speed over the cell extent summed over the axes") {
        VALIDATE_NUMERICAL_EQUALS(statistics.maxPhysicalEigenvalue, WaveSpeed);
        VALIDATE_NUMERICAL_EQUALS(statistics.maxScaledEigenvalue, DIMENSIONS * WaveSpeed / CellExtent);
      }
    }
  }
}

UNIT_TEST(
  "A corrector launch runs at least one team, at most one team per cell and at most one team per getTeamThreads() nodes"
) {
  GIVEN("The threads of a corrector team") {
    const int threads = CorrectorOfWaveSpeedSolver::getTeamThreads();

    WHEN("The corrector counts its teams for 1, 7 and 4096 cells") {
      const int cellCounts[3] = {1, 7, 4096};
      int       teams[3]      = {};
      for (int count = 0; count < 3; count++) {
        teams[count] = CorrectorOfWaveSpeedSolver::getNumberOfTeams(cellCounts[count]);
      }

      THEN("A team runs one thread or more and every count lies within the three bounds") {
        VALIDATE(threads >= 1, threads);
        for (int count = 0; count < 3; count++) {
          const int cells = cellCounts[count];
          VALIDATE(teams[count] >= 1 and teams[count] <= cells, teams[count], cells);
          VALIDATE(
            teams[count] == 1 or teams[count] <= (cells * CellLayout::NodesPerCell + threads - 1) / threads,
            teams[count],
            cells,
            threads
          );
        }
      }
    }
  }
}
