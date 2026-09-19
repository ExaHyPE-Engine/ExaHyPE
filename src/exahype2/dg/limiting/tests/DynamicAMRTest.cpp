// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DynamicAMRTest.h"

#include "exahype2/amr/ChildDataStage.h"
#include "exahype2/dg/LimiterStatus.h"
#include "exahype2/dg/limiting/DynamicAMR.h"
#include "exahype2/dg/limiting/SubcellLayout.h"
#include "exahype2/dg/limiting/tests/LimiterFixtures.h"
#include "peano4/dataview/CellMarker.h"
#include "peano4/sfc/ActiveCurve.h"

namespace {
  using Solver        = exahype2::dg::limiting::tests::TestSolver<false>;
  using CellLabel     = exahype2::dg::limiting::tests::TestCellLabel;
  using Layout        = exahype2::dg::limiting::SubcellLayout<Solver>;
  using LimiterStatus = exahype2::dg::LimiterStatus;

  constexpr int SubdivisionFactor = peano4::sfc::ActiveCurve::SubdivisionFactor;
  constexpr int ChildrenPerCell   = peano4::sfc::ActiveCurve::ChildrenPerCell;

  constexpr int NodalValues = Layout::NumberOfData * Layout::NumberOfNodes;
  constexpr int SubcellValues =
    exahype2::dg::limiting::ValuesPerSubcellPatch<Layout::NumberOfSubcellsPerAxis, Layout::NumberOfData>;
  constexpr int ProjectionEntries = Layout::NumberOfDofsPerAxis * Layout::NumberOfSubcellsPerAxis;

  constexpr double NodalValue   = 1.5;
  constexpr double SubcellValue = 4.25;
  constexpr double StaleValue   = -3.0;

  /**
   * Returns a projection whose entries average the nodes, so a constant nodal solution projects
   * onto the same constant on every subcell.
   */
  std::array<double, ProjectionEntries> averagingProjection() {
    std::array<double, ProjectionEntries> result{};
    result.fill(1.0 / Layout::NumberOfDofsPerAxis);
    return result;
  }

  tarch::la::Vector<DIMENSIONS, int> childPosition(const int child) {
    tarch::la::Vector<DIMENSIONS, int> result;
    int                                remainder = child;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      result(axis) = remainder % SubdivisionFactor;
      remainder /= SubdivisionFactor;
    }
    return result;
  }

  peano4::dataview::CellMarker createCellMarker(const int child, const bool hasBeenRefined, const bool willBeRefined) {
    peano4::dataview::CellMarker marker{};
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      marker.h[axis]      = 1.0 / SubdivisionFactor;
      marker.centre[axis] = 0.5;
    }
    marker.relativePositionWithinFatherCell = childPosition(child);
    if (hasBeenRefined) {
      marker.flags |= peano4::dataview::CellMarker::HasBeenRefined;
    }
    if (willBeRefined) {
      marker.flags |= peano4::dataview::CellMarker::WillBeRefined;
    }
    return marker;
  }

  /**
   * Runs the creation of the first child of a limited parent in the given precision and returns
   * whether both the subgrid and the label of the child carry the values of the parent.
   */
  template <typename Value>
  bool childOfALimitedParentTakesItsSubgrid() {
    const auto                       projection = averagingProjection();
    std::array<Value, SubcellValues> coarseSubcells{};
    std::array<Value, SubcellValues> fineSubcells{};
    std::array<Value, NodalValues>   fineNodalSolution{};
    coarseSubcells.fill(static_cast<Value>(SubcellValue));
    fineNodalSolution.fill(static_cast<Value>(NodalValue));

    CellLabel coarseCellLabel;
    CellLabel fineCellLabel;
    coarseCellLabel.setLimiterStatus(LimiterStatus::Limited);
    coarseCellLabel.setPredictorDiverged(true);

    exahype2::dg::limiting::createCell<Solver>(
      createCellMarker(0, false, false),
      projection.data(),
      coarseSubcells.data(),
      fineSubcells.data(),
      fineNodalSolution.data(),
      coarseCellLabel,
      fineCellLabel
    );

    bool childTakesTheParentsValues =
      fineCellLabel.getLimiterStatus() == LimiterStatus::Limited and fineCellLabel.getPredictorDiverged();
    for (int value = 0; value < SubcellValues; value++) {
      childTakesTheParentsValues =
        childTakesTheParentsValues and fineSubcells[value] == static_cast<Value>(SubcellValue);
    }
    return childTakesTheParentsValues;
  }
} // namespace

exahype2::dg::limiting::tests::DynamicAMRTest::DynamicAMRTest() :
  tarch::tests::TestCase("exahype2::dg::limiting::tests::DynamicAMRTest") {}

void exahype2::dg::limiting::tests::DynamicAMRTest::run() {
  testMethod(testResetCoarseningParentLabelActsOnACoarseningParentOnly);
  testMethod(testCreateCellProlongsTheSubcellsOfALimitedParent);
  testMethod(testCreateCellProjectsTheNodalSolutionOfARegularParent);
  testMethod(testDestroyCellRestrictsAMixedSiblingSetAndMergesTheLimiterState);
}

/**
 * Checks that only a cell that loses its children returns to the regular state.
 */
void exahype2::dg::limiting::tests::DynamicAMRTest::testResetCoarseningParentLabelActsOnACoarseningParentOnly() {
  CellLabel cellLabel;
  cellLabel.setLimiterStatus(LimiterStatus::Troubled);
  cellLabel.setPreviousLimiterStatus(LimiterStatus::Limited);
  cellLabel.setWasTroubledInPreviousStep(true);
  cellLabel.setPredictorDiverged(true);

  exahype2::dg::limiting::resetCoarseningParentLabel(createCellMarker(0, false, false), cellLabel);
  validateEquals(static_cast<int>(cellLabel.getLimiterStatus()), static_cast<int>(LimiterStatus::Troubled));

  exahype2::dg::limiting::resetCoarseningParentLabel(createCellMarker(0, true, true), cellLabel);
  validateEquals(static_cast<int>(cellLabel.getLimiterStatus()), static_cast<int>(LimiterStatus::Troubled));

  exahype2::dg::limiting::resetCoarseningParentLabel(createCellMarker(0, true, false), cellLabel);
  validateEquals(static_cast<int>(cellLabel.getLimiterStatus()), static_cast<int>(LimiterStatus::Regular));
  validateEquals(static_cast<int>(cellLabel.getPreviousLimiterStatus()), static_cast<int>(LimiterStatus::Regular));
  validateFalse(cellLabel.getWasTroubledInPreviousStep());
  validateFalse(cellLabel.getPredictorDiverged());
}

/**
 * Checks that a child of a limited parent takes the subgrid and the label of that parent in
 * double and in single precision.
 */
void exahype2::dg::limiting::tests::DynamicAMRTest::testCreateCellProlongsTheSubcellsOfALimitedParent() {
  validateTrue(childOfALimitedParentTakesItsSubgrid<double>());
  validateTrue(childOfALimitedParentTakesItsSubgrid<float>());
}

/**
 * Checks that a child of a regular parent takes the projection of its own nodal solution.
 */
void exahype2::dg::limiting::tests::DynamicAMRTest::testCreateCellProjectsTheNodalSolutionOfARegularParent() {
  const auto                        projection = averagingProjection();
  std::array<double, SubcellValues> coarseSubcells{};
  std::array<double, SubcellValues> fineSubcells{};
  std::array<double, NodalValues>   fineNodalSolution{};
  coarseSubcells.fill(SubcellValue);
  fineNodalSolution.fill(NodalValue);

  CellLabel coarseCellLabel;
  CellLabel fineCellLabel;

  exahype2::dg::limiting::createCell<Solver>(
    createCellMarker(0, false, false),
    projection.data(),
    coarseSubcells.data(),
    fineSubcells.data(),
    fineNodalSolution.data(),
    coarseCellLabel,
    fineCellLabel
  );

  for (int value = 0; value < SubcellValues; value++) {
    validateNumericalEqualsWithParams1(fineSubcells[value], NodalValue, value);
  }
}

/**
 * Checks that a sibling set of one regular and several limited children restricts the projected
 * nodal solution and the subgrids onto the parent and merges every field of their labels.
 */
void exahype2::dg::limiting::tests::DynamicAMRTest::testDestroyCellRestrictsAMixedSiblingSetAndMergesTheLimiterState() {
  const auto                            projection = averagingProjection();
  exahype2::amr::ChildDataStage<double> subcellStage(ChildrenPerCell, SubcellValues);
  std::array<double, SubcellValues>     coarseSubcells{};

  CellLabel coarseCellLabel;
  for (int child = 0; child < ChildrenPerCell; child++) {
    std::array<double, SubcellValues> fineSubcells{};
    std::array<double, NodalValues>   fineNodalSolution{};
    fineNodalSolution.fill(NodalValue);

    CellLabel fineCellLabel;
    if (child == 0) {
      // A regular child holds a stale subgrid, which its projection overwrites.
      fineSubcells.fill(StaleValue);
      fineCellLabel.setPreviousLimiterStatus(LimiterStatus::Halo);
      fineCellLabel.setPredictorDiverged(true);
    } else {
      fineSubcells.fill(SubcellValue);
      fineCellLabel.setLimiterStatus(LimiterStatus::Limited);
      fineCellLabel.setWasTroubledInPreviousStep(child == 1);
    }

    exahype2::dg::limiting::destroyCell<Solver>(
      createCellMarker(child, false, false),
      subcellStage,
      projection.data(),
      coarseSubcells.data(),
      fineSubcells.data(),
      fineNodalSolution.data(),
      coarseCellLabel,
      fineCellLabel
    );
  }

  // Every coarse subcell averages the same number of fine subcells, so the mean over the parent's
  // subgrid equals the mean over the subgrids of its children. The first coarse subcell lies
  // inside the first child, which carries the projection of its nodal solution.
  double coarseSum = 0.0;
  for (int subcell = 0; subcell < Layout::NumberOfSubcells; subcell++) {
    coarseSum += coarseSubcells[subcell * Layout::NumberOfData];
  }
  const double expectedMean = (NodalValue + (ChildrenPerCell - 1) * SubcellValue) / ChildrenPerCell;
  validateNumericalEquals(coarseSum / Layout::NumberOfSubcells, expectedMean);
  validateNumericalEquals(coarseSubcells[0], NodalValue);
  validateEquals(static_cast<int>(coarseCellLabel.getLimiterStatus()), static_cast<int>(LimiterStatus::Limited));
  validateEquals(static_cast<int>(coarseCellLabel.getPreviousLimiterStatus()), static_cast<int>(LimiterStatus::Halo));
  validateTrue(coarseCellLabel.getWasTroubledInPreviousStep());
  validateTrue(coarseCellLabel.getPredictorDiverged());
}
