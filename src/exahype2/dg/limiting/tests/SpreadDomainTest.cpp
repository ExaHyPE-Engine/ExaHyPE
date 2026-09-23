// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SpreadDomainTest.h"

#include "exahype2/dg/LimiterStatus.h"
#include "exahype2/dg/limiting/SpreadDomain.h"
#include "exahype2/dg/limiting/SubcellLayout.h"
#include "exahype2/dg/limiting/tests/LimiterFixtures.h"
#include "peano4/Constants.h"

namespace {
  using Solver = exahype2::dg::limiting::tests::TestSolver<true>;
  using Layout = exahype2::dg::limiting::SubcellLayout<Solver>;

  constexpr int HaloWidth          = 1;
  constexpr int TangentialSubcells = peano4::power(Layout::NumberOfSubcellsPerAxis, DIMENSIONS - 1);
  constexpr int FaceCardinality    = 2 * HaloWidth * TangentialSubcells;

  using CellLabel     = exahype2::dg::limiting::tests::TestCellLabel;
  using FaceLabel     = exahype2::dg::limiting::tests::TestFaceLabel;
  using FaceLabels    = exahype2::dg::limiting::tests::TestEnumerator<FaceLabel, peano4::FacesPerCell>;
  using SubcellFace   = exahype2::dg::limiting::tests::TestPatch<double, FaceCardinality>;
  using SubcellFaces  = exahype2::dg::limiting::tests::TestEnumerator<SubcellFace, peano4::FacesPerCell>;
  using NodalPatch    = exahype2::dg::limiting::tests::TestPatch<double, Layout::NumberOfNodes>;
  using SubcellPatch  = exahype2::dg::limiting::tests::TestPatch<double, Layout::NumberOfSubcells>;
  using LimiterStatus = exahype2::dg::LimiterStatus;

  // Every column sums to one, so a constant polynomial keeps its value on the subgrid.
  constexpr double
    DgToFvMatrix[Layout::NumberOfDofsPerAxis * Layout::NumberOfSubcellsPerAxis]{1.0, 0.5, 0.0, 0.0, 0.5, 1.0};

  constexpr double CellCentre          = 0.5;
  constexpr double CellWidth           = 1.0;
  constexpr double TimeStepSize        = 0.25;
  constexpr double AcceptedValue       = 2.0;
  constexpr double SampledSubcellValue = 7.0;
  constexpr double UntouchedValue      = 9.0;

  /**
   * Holds the entities of one cell that the spreading sweep reads and writes.
   */
  struct Cell final {
    CellLabel    cellLabel{};
    FaceLabel    faces[peano4::FacesPerCell]{};
    SubcellFace  subcellFaceStorage[peano4::FacesPerCell]{};
    NodalPatch   acceptedSolution{};
    SubcellPatch subcellPatch{};
  };

  /**
   * Fills the cell with the given stamp, an accepted constant polynomial, a sampled constant
   * subgrid and halo faces the sweep has not written.
   */
  void fill(Cell& cell, const double timeStamp) {
    cell.cellLabel.timeStamp    = timeStamp;
    cell.cellLabel.timeStepSize = TimeStepSize;
    std::fill_n(cell.acceptedSolution.data(), Layout::NumberOfNodes, AcceptedValue);
    std::fill_n(cell.subcellPatch.data(), Layout::NumberOfSubcells, SampledSubcellValue);
    for (auto& face : cell.subcellFaceStorage) {
      std::fill_n(face.data(), FaceCardinality, UntouchedValue);
    }
  }

  void spread(Cell& cell) {
    Solver             solver;
    const FaceLabels   faceLabels{cell.faces};
    const SubcellFaces subcellFaces{cell.subcellFaceStorage};
    exahype2::dg::limiting::spreadDomain<Solver, HaloWidth>(
      exahype2::dg::limiting::tests::TestGeometry::cellMarker(CellCentre, CellWidth),
      solver,
      cell.cellLabel,
      faceLabels,
      subcellFaces,
      cell.acceptedSolution,
      cell.subcellPatch,
      DgToFvMatrix
    );
  }
} // namespace

exahype2::dg::limiting::tests::SpreadDomainTest::SpreadDomainTest() :
  tarch::tests::TestCase("exahype2::dg::limiting::tests::SpreadDomainTest") {}

void exahype2::dg::limiting::tests::SpreadDomainTest::run() {
  testMethod(testCellWithoutTroubledNeighbourKeepsItsSubgrid);
  testMethod(testTroubledNeighbourProjectsTheAcceptedPolynomialOntoTheSubgrid);
  testMethod(testCellEnteringWithinTheFirstStepKeepsItsSampledSubcells);
  testMethod(testBoundaryFaceHaloComesFromTheBoundaryConditions);
  testMethod(testALimitedCellKeepsItsEvolvedSubgrid);
  testMethod(testALimitedNeighbourSpreadsTheHaloStatus);
  testMethod(testAPeriodicBoundaryFaceKeepsItsOuterLayer);
}

/**
 * Checks that a cell whose neighbours all run the Galerkin scheme leaves its subgrid alone.
 */
void exahype2::dg::limiting::tests::SpreadDomainTest::testCellWithoutTroubledNeighbourKeepsItsSubgrid() {
  Cell cell;
  fill(cell, 4.0 * TimeStepSize);

  spread(cell);

  validateEquals(static_cast<int>(cell.cellLabel.getLimiterStatus()), static_cast<int>(LimiterStatus::Regular));
  validateFalse(cell.cellLabel.getWasTroubledInPreviousStep());
  for (int subcell = 0; subcell < Layout::NumberOfSubcells; subcell++) {
    validateNumericalEqualsWithParams1(cell.subcellPatch.data()[subcell], SampledSubcellValue, subcell);
  }
}

/**
 * Checks that a cell next to a troubled one enters the subcell scheme and starts from the accepted
 * polynomial.
 */
void exahype2::dg::limiting::tests::SpreadDomainTest::
  testTroubledNeighbourProjectsTheAcceptedPolynomialOntoTheSubgrid() {
  Cell cell;
  fill(cell, 4.0 * TimeStepSize);
  cell.faces[0].setLimiterStatusSnapshot(0, LimiterStatus::Troubled);

  spread(cell);

  validateEquals(static_cast<int>(cell.cellLabel.getLimiterStatus()), static_cast<int>(LimiterStatus::Limited));
  validateTrue(cell.cellLabel.getWasTroubledInPreviousStep());
  for (int subcell = 0; subcell < Layout::NumberOfSubcells; subcell++) {
    validateNumericalEqualsWithParams1(cell.subcellPatch.data()[subcell], AcceptedValue, subcell);
  }
  // The lower face of the first axis holds the cell's own layer in its upper half.
  validateNumericalEquals(cell.subcellFaceStorage[0].data()[HaloWidth], AcceptedValue);
}

/**
 * Checks that a cell entering the scheme within the first time step keeps the subcells the grid
 * initialisation sampled.
 */
void exahype2::dg::limiting::tests::SpreadDomainTest::testCellEnteringWithinTheFirstStepKeepsItsSampledSubcells() {
  Cell cell;
  fill(cell, TimeStepSize);
  cell.faces[0].setLimiterStatusSnapshot(0, LimiterStatus::Troubled);

  spread(cell);

  for (int subcell = 0; subcell < Layout::NumberOfSubcells; subcell++) {
    validateNumericalEqualsWithParams1(cell.subcellPatch.data()[subcell], SampledSubcellValue, subcell);
  }
  validateNumericalEquals(cell.subcellFaceStorage[0].data()[HaloWidth], SampledSubcellValue);
}

/**
 * Checks that the outer halo layer of a boundary face comes from the solver's boundary condition.
 */
void exahype2::dg::limiting::tests::SpreadDomainTest::testBoundaryFaceHaloComesFromTheBoundaryConditions() {
  Cell cell;
  fill(cell, 4.0 * TimeStepSize);
  cell.faces[0].setLimiterStatusSnapshot(0, LimiterStatus::Troubled);
  cell.faces[0].boundary = true;

  spread(cell);

  // The boundary condition copies the inner layer, which the projection filled with the subgrid.
  validateNumericalEquals(cell.subcellFaceStorage[0].data()[0], AcceptedValue);
}

/**
 * Checks that a cell that ran the subcell scheme in the previous step keeps its evolved subgrid
 * and writes it onto its halo faces.
 */
void exahype2::dg::limiting::tests::SpreadDomainTest::testALimitedCellKeepsItsEvolvedSubgrid() {
  Cell cell;
  fill(cell, 4.0 * TimeStepSize);
  cell.cellLabel.setPreviousLimiterStatus(LimiterStatus::Limited);
  cell.faces[0].setLimiterStatusSnapshot(0, LimiterStatus::Troubled);

  spread(cell);

  for (int subcell = 0; subcell < Layout::NumberOfSubcells; subcell++) {
    validateNumericalEqualsWithParams1(cell.subcellPatch.data()[subcell], SampledSubcellValue, subcell);
  }
  validateNumericalEquals(cell.subcellFaceStorage[0].data()[HaloWidth], SampledSubcellValue);
}

/**
 * Checks that a limited neighbour spreads the halo status, which writes the halo faces but keeps
 * the cell off the subcell scheme in the next step.
 */
void exahype2::dg::limiting::tests::SpreadDomainTest::testALimitedNeighbourSpreadsTheHaloStatus() {
  Cell cell;
  fill(cell, 4.0 * TimeStepSize);
  cell.faces[0].setLimiterStatusSnapshot(0, LimiterStatus::Limited);

  spread(cell);

  validateEquals(static_cast<int>(cell.cellLabel.getLimiterStatus()), static_cast<int>(LimiterStatus::Halo));
  validateFalse(cell.cellLabel.getWasTroubledInPreviousStep());
  validateNumericalEquals(cell.subcellFaceStorage[0].data()[HaloWidth], AcceptedValue);
}

/**
 * Checks that the boundary face of a periodic axis keeps the outer layer the periodic exchange
 * fills.
 */
void exahype2::dg::limiting::tests::SpreadDomainTest::testAPeriodicBoundaryFaceKeepsItsOuterLayer() {
  Cell cell;
  fill(cell, 4.0 * TimeStepSize);
  cell.faces[0].setLimiterStatusSnapshot(0, LimiterStatus::Troubled);
  cell.faces[0].boundary = true;

  Solver::PeriodicBoundaries.set(0);
  spread(cell);
  Solver::PeriodicBoundaries.reset();

  validateNumericalEquals(cell.subcellFaceStorage[0].data()[0], UntouchedValue);
  validateNumericalEquals(cell.subcellFaceStorage[0].data()[HaloWidth], AcceptedValue);
}
