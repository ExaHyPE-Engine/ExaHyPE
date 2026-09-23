// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "InitialiseSubcellsTest.h"

#include "exahype2/dg/limiting/InitialiseSubcells.h"
#include "exahype2/dg/limiting/tests/LimiterFixtures.h"

namespace {
  using Solver = exahype2::dg::limiting::tests::TestSolver<true>;

  constexpr int NumberOfDofsPerAxis     = Solver::NumberOfDofsPerAxis;
  constexpr int NumberOfSubcellsPerAxis = Solver::NumberOfSubcellsPerAxis;
  constexpr int NumberOfNodes           = Solver::NumberOfNodes;
  constexpr int NumberOfSubcells        = Solver::NumberOfSubcells;

  constexpr double CellCentre = 0.5;
  constexpr double CellWidth  = 1.0;

  constexpr double DgToLobattoMatrix[NumberOfDofsPerAxis * NumberOfDofsPerAxis]{1.0, 0.0, 0.0, 1.0};
  // Every column sums to one, so a constant field keeps its value under the sampling.
  constexpr double DgToSubcellMatrix[NumberOfDofsPerAxis * NumberOfSubcellsPerAxis]{1.0, 0.5, 0.0, 0.0, 0.5, 1.0};

  bool startsOnTheSubcellScheme(const double nodalValue) {
    const auto marker = exahype2::dg::limiting::tests::TestGeometry::cellMarker(CellCentre, CellWidth);
    exahype2::dg::limiting::tests::TestCellLabel cellLabel;
    double                                       nodalSolution[NumberOfNodes];
    std::fill_n(nodalSolution, NumberOfNodes, nodalValue);
    double subcellSolution[NumberOfSubcells]{};

    exahype2::dg::limiting::initialiseSubcells<Solver, 1, false>(
      marker,
      cellLabel,
      nodalSolution,
      subcellSolution,
      DgToLobattoMatrix,
      DgToSubcellMatrix,
      0.0,
      0.0
    );
    return cellLabel.getWasTroubledInPreviousStep();
  }
} // namespace

exahype2::dg::limiting::tests::InitialiseSubcellsTest::InitialiseSubcellsTest() :
  tarch::tests::TestCase("exahype2::dg::limiting::tests::InitialiseSubcellsTest") {}

void exahype2::dg::limiting::tests::InitialiseSubcellsTest::run() {
  testMethod(testSubcellsHoldTheInitialConditionAtTheirCentres);
  testMethod(testInterpolantWithinTheSampledRangeLeavesTheCellUntroubled);
  testMethod(testInterpolantOutsideTheSampledRangeMarksTheCellTroubled);
}

/**
 * Checks that every subcell holds the initial condition evaluated at its own centre.
 */
void exahype2::dg::limiting::tests::InitialiseSubcellsTest::testSubcellsHoldTheInitialConditionAtTheirCentres() {
  const auto marker = TestGeometry::cellMarker(CellCentre, CellWidth);
  double     subcellSolution[NumberOfSubcells]{};

  exahype2::dg::limiting::initialiseSubcells<Solver>(marker, subcellSolution);

  constexpr double SubcellWidth = CellWidth / NumberOfSubcellsPerAxis;
  for (int subcell = 0; subcell < NumberOfSubcells; subcell++) {
    const int    indexAlongFirstAxis = subcell % NumberOfSubcellsPerAxis;
    const double expectedCentre      = CellCentre - 0.5 * CellWidth + (0.5 + indexAlongFirstAxis) * SubcellWidth;
    validateNumericalEquals(subcellSolution[subcell], expectedCentre);
  }
}

/**
 * Checks that a cell whose interpolant stays inside the range of its sampled subcells keeps the
 * Galerkin scheme.
 */
void exahype2::dg::limiting::tests::InitialiseSubcellsTest::
  testInterpolantWithinTheSampledRangeLeavesTheCellUntroubled() {
  validateFalse(startsOnTheSubcellScheme(CellCentre));
}

/**
 * Checks that a cell whose interpolant leaves the range of its sampled subcells starts on the
 * subcell scheme.
 */
void exahype2::dg::limiting::tests::InitialiseSubcellsTest::
  testInterpolantOutsideTheSampledRangeMarksTheCellTroubled() {
  validateTrue(startsOnTheSubcellScheme(2.0 * CellWidth));
}
