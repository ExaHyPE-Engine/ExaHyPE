// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DynamicAMRTest.h"

#include "exahype2/fv/DynamicAMR.h"
#include "exahype2/fv/tests/ActionSetTestData.h"
#include "peano4/sfc/ActiveCurve.h"

namespace {
  constexpr int SubdivisionFactor = peano4::sfc::ActiveCurve::SubdivisionFactor;
  constexpr int ChildrenPerCell   = peano4::sfc::ActiveCurve::ChildrenPerCell;
  constexpr int SubfacesPerFace   = exahype2::amr::SubfacesPerFace;

  constexpr int DofsPerAxis      = 2;
  constexpr int Overlap          = 1;
  constexpr int NumberOfUnknowns = 1;

  constexpr int FaceLayerValues = exahype2::fv::ValuesPerFaceLayer<DofsPerAxis, NumberOfUnknowns>;
  constexpr int FacePatchValues = 2 * Overlap * FaceLayerValues;
  constexpr int CellPatchValues = exahype2::fv::ValuesPerCellPatch<DofsPerAxis, NumberOfUnknowns>;

  constexpr double ConstantField = 3.5;

  peano4::dataview::FaceMarker createFaceMarker(
    const int                                 faceNumber,
    const tarch::la::Vector<DIMENSIONS, int>& relativePosition
  ) {
    peano4::dataview::FaceMarker marker{};
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      marker.h[axis]          = 1.0 / SubdivisionFactor;
      marker.cellCentre[axis] = 0.5;
    }
    marker.relativePositionWithinFatherCell = relativePosition;
    marker.select(faceNumber);
    return marker;
  }

  peano4::dataview::CellMarker createCellMarker(
    const tarch::la::Vector<DIMENSIONS, int>& relativePosition,
    const bool                                hasBeenRefined,
    const bool                                willBeRefined
  ) {
    peano4::dataview::CellMarker marker{};
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      marker.h[axis]      = 1.0 / SubdivisionFactor;
      marker.centre[axis] = 0.5;
    }
    marker.relativePositionWithinFatherCell = relativePosition;
    if (hasBeenRefined) {
      marker.flags |= peano4::dataview::CellMarker::HasBeenRefined;
    }
    if (willBeRefined) {
      marker.flags |= peano4::dataview::CellMarker::WillBeRefined;
    }
    return marker;
  }

  /**
   * Returns the position within the father cell of the subface with the given tangential index.
   */
  tarch::la::Vector<DIMENSIONS, int> subfacePosition(const int faceNumber, const int subfaceIndex) {
    tarch::la::Vector<DIMENSIONS, int> result(0);
    const int                          normal    = faceNumber % DIMENSIONS;
    int                                remainder = subfaceIndex;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      if (axis != normal) {
        result(axis) = remainder % SubdivisionFactor;
        remainder /= SubdivisionFactor;
      }
    }
    result(normal) = faceNumber < DIMENSIONS ? 0 : SubdivisionFactor - 1;
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
} // namespace

exahype2::fv::tests::DynamicAMRTest::DynamicAMRTest() :
  tarch::tests::TestCase("exahype2::fv::tests::DynamicAMRTest") {}

void exahype2::fv::tests::DynamicAMRTest::run() {
  testMethod(testClearFaceAndCellPatch);
  testMethod(testInterpolateHangingFaceFillsTheHaloAndTheTimeStamps);
  testMethod(testRestrictHangingFaceStagesItsHaloAndRaisesTheCoarseLabel);
  testMethod(testRestrictFaceHalvesAveragesTheStagedSubfaces);
  testMethod(testClearCoarseningFaceAndCellActOnAParentOnly);
  testMethod(testRestrictCoarseningFaceHalvesDiscardsACompletedHalf);
  testMethod(testCreateFaceAndDestroyFaceCarryTheTimeStamps);
  testMethod(testCreateCellAndDestroyCellReproduceAConstantPatch);
  testMethod(testProjectPatchAfterTransferWaitsForAFaceThatAwaitsData);
}

/**
 * Checks that the two clears cover the whole patch and nothing beyond it.
 */
void exahype2::fv::tests::DynamicAMRTest::testClearFaceAndCellPatch() {
  std::array<double, FacePatchValues + 1> faceValues{};
  faceValues.fill(1.0);
  exahype2::fv::clearFacePatch<DofsPerAxis, Overlap, NumberOfUnknowns>(faceValues.data());
  for (int value = 0; value < FacePatchValues; value++) {
    validateNumericalEquals(faceValues[value], 0.0);
  }
  validateNumericalEquals(faceValues[FacePatchValues], 1.0);

  std::array<double, CellPatchValues + 1> cellValues{};
  cellValues.fill(1.0);
  exahype2::fv::clearCellPatch<DofsPerAxis, NumberOfUnknowns>(cellValues.data());
  for (int value = 0; value < CellPatchValues; value++) {
    validateNumericalEquals(cellValues[value], 0.0);
  }
  validateNumericalEquals(cellValues[CellPatchValues], 1.0);
}

/**
 * Checks that a subface on a face of its parent takes the coarse halo and its time stamps.
 */
void exahype2::fv::tests::DynamicAMRTest::testInterpolateHangingFaceFillsTheHaloAndTheTimeStamps() {
  std::array<double, FacePatchValues> coarseFaceValues{};
  coarseFaceValues.fill(ConstantField);
  std::array<double, FacePatchValues> fineFaceValues{};

  exahype2::fv::tests::FaceLabel coarseFaceLabel;
  exahype2::fv::tests::FaceLabel fineFaceLabel;
  coarseFaceLabel.setNewTimeStamp(0, 2.0);
  coarseFaceLabel.setOldTimeStamp(0, 1.0);

  const auto marker = createFaceMarker(0, tarch::la::Vector<DIMENSIONS, int>(0));
  exahype2::fv::interpolateHangingFace<DofsPerAxis, NumberOfUnknowns>(
    marker,
    coarseFaceValues.data(),
    fineFaceValues.data(),
    coarseFaceLabel,
    fineFaceLabel
  );

  // A constant coarse halo prolongs to the same constant, whatever the tangential slope adds.
  for (int layer = 0; layer < FaceLayerValues; layer++) {
    validateNumericalEquals(fineFaceValues[2 * layer], ConstantField);
  }
  validateNumericalEquals(fineFaceLabel.getNewTimeStamp(0), 2.0);
  validateNumericalEquals(fineFaceLabel.getOldTimeStamp(0), 1.0);
  validateNumericalEquals(fineFaceLabel.getNewTimeStamp(1), 0.0);
}

/**
 * Checks that a hanging face stages its halo below the coarse face and marks that half updated.
 */
void exahype2::fv::tests::DynamicAMRTest::testRestrictHangingFaceStagesItsHaloAndRaisesTheCoarseLabel() {
  exahype2::amr::ChildDataStage<double> stage(SubfacesPerFace, FaceLayerValues);
  std::array<double, FacePatchValues>   coarseFaceValues{};
  std::array<double, FacePatchValues>   fineFaceValues{};
  fineFaceValues.fill(ConstantField);

  exahype2::fv::tests::FaceLabel coarseFaceLabel;
  exahype2::fv::tests::FaceLabel fineFaceLabel;
  fineFaceLabel.setUpdatedTimeStamp(1, 4.0);

  const auto marker = createFaceMarker(0, tarch::la::Vector<DIMENSIONS, int>(0));
  exahype2::fv::restrictHangingFace<DofsPerAxis, NumberOfUnknowns>(
    marker,
    stage,
    coarseFaceValues.data(),
    fineFaceValues.data(),
    coarseFaceLabel,
    fineFaceLabel
  );

  validateTrue(stage.hasFrame(coarseFaceValues.data() + 1));
  validateTrue(coarseFaceLabel.getUpdated(1));
  validateNumericalEquals(coarseFaceLabel.getUpdatedTimeStamp(1), 4.0);
  stage.discard(coarseFaceValues.data() + 1);
}

/**
 * Checks that a face reduces the halo of every subface staged below either of its halves.
 */
void exahype2::fv::tests::DynamicAMRTest::testRestrictFaceHalvesAveragesTheStagedSubfaces() {
  exahype2::amr::ChildDataStage<double> stage(SubfacesPerFace, FaceLayerValues);
  std::array<double, FacePatchValues>   coarseFaceValues{};
  std::array<double, FacePatchValues>   subfaceValues{};
  subfaceValues.fill(ConstantField);

  for (int subface = 0; subface < SubfacesPerFace; subface++) {
    const auto subfaceMarker = createFaceMarker(0, subfacePosition(0, subface));
    exahype2::amr::stageHaloLayerForGatheredRestriction(
      subfaceMarker,
      stage,
      coarseFaceValues.data() + 1,
      subfaceValues.data(),
      1,
      DofsPerAxis,
      NumberOfUnknowns
    );
  }

  const auto marker = createFaceMarker(0, tarch::la::Vector<DIMENSIONS, int>(0));
  exahype2::fv::restrictFaceHalves<DofsPerAxis, NumberOfUnknowns>(marker, stage, coarseFaceValues.data());

  validateFalse(stage.hasFrame(coarseFaceValues.data() + 1));
  for (int layer = 0; layer < FaceLayerValues; layer++) {
    validateNumericalEquals(coarseFaceValues[2 * layer + 1], ConstantField);
  }
}

/**
 * Checks that only a cell that loses its children clears its patches and flags its faces.
 */
void exahype2::fv::tests::DynamicAMRTest::testClearCoarseningFaceAndCellActOnAParentOnly() {
  std::array<double, FacePatchValues> faceValuesNew{};
  std::array<double, FacePatchValues> faceValuesOld{};
  faceValuesNew.fill(1.0);
  faceValuesOld.fill(1.0);
  exahype2::fv::tests::FaceLabel faceLabel;

  const auto leafFaceMarker = createFaceMarker(0, tarch::la::Vector<DIMENSIONS, int>(0));
  exahype2::fv::clearCoarseningFace<DofsPerAxis, Overlap, NumberOfUnknowns>(
    leafFaceMarker,
    faceLabel,
    faceValuesNew.data(),
    faceValuesOld.data()
  );
  validateNumericalEquals(faceValuesNew[0], 1.0);
  validateFalse(faceLabel.getRequiresDataTransfer(0));

  auto parentFaceMarker               = leafFaceMarker;
  parentFaceMarker.hasBeenRefinedMask = 1U;
  exahype2::fv::clearCoarseningFace<DofsPerAxis, Overlap, NumberOfUnknowns>(
    parentFaceMarker,
    faceLabel,
    faceValuesNew.data(),
    faceValuesOld.data()
  );
  validateNumericalEquals(faceValuesNew[0], 0.0);
  validateNumericalEquals(faceValuesOld[0], 0.0);
  validateTrue(faceLabel.getRequiresDataTransfer(0));
  validateTrue(faceLabel.getRequiresDataTransfer(1));

  std::array<double, CellPatchValues> cellValuesNew{};
  std::array<double, CellPatchValues> cellValuesOld{};
  cellValuesNew.fill(1.0);
  cellValuesOld.fill(1.0);
  const auto leafCellMarker = createCellMarker(tarch::la::Vector<DIMENSIONS, int>(0), false, false);
  exahype2::fv::clearCoarseningCell<DofsPerAxis, NumberOfUnknowns>(
    leafCellMarker,
    cellValuesNew.data(),
    cellValuesOld.data()
  );
  validateNumericalEquals(cellValuesNew[0], 1.0);

  const auto parentCellMarker = createCellMarker(tarch::la::Vector<DIMENSIONS, int>(0), true, false);
  exahype2::fv::clearCoarseningCell<DofsPerAxis, NumberOfUnknowns>(
    parentCellMarker,
    cellValuesNew.data(),
    cellValuesOld.data()
  );
  validateNumericalEquals(cellValuesNew[0], 0.0);
  validateNumericalEquals(cellValuesOld[0], 0.0);
}

/**
 * Checks that a half a projection completed discards its staged subfaces without reducing them.
 */
void exahype2::fv::tests::DynamicAMRTest::testRestrictCoarseningFaceHalvesDiscardsACompletedHalf() {
  exahype2::amr::ChildDataStage<double> stageNew(SubfacesPerFace, FaceLayerValues);
  exahype2::amr::ChildDataStage<double> stageOld(SubfacesPerFace, FaceLayerValues);
  std::array<double, FacePatchValues>   faceValuesNew{};
  std::array<double, FacePatchValues>   faceValuesOld{};
  std::array<double, FacePatchValues>   subfaceValues{};
  subfaceValues.fill(ConstantField);

  const auto subfaceMarker = createFaceMarker(0, tarch::la::Vector<DIMENSIONS, int>(0));
  for (int facePosition = 0; facePosition < 2; facePosition++) {
    exahype2::amr::stageHaloLayerForGatheredRestriction(
      subfaceMarker,
      stageNew,
      faceValuesNew.data() + facePosition,
      subfaceValues.data(),
      facePosition,
      DofsPerAxis,
      NumberOfUnknowns
    );
    exahype2::amr::stageHaloLayerForGatheredRestriction(
      subfaceMarker,
      stageOld,
      faceValuesOld.data() + facePosition,
      subfaceValues.data(),
      facePosition,
      DofsPerAxis,
      NumberOfUnknowns
    );
  }

  exahype2::fv::tests::FaceLabel faceLabel;
  faceLabel.setRequiresDataTransfer(0, true);
  faceLabel.setRequiresDataTransfer(1, false);

  const auto marker = createFaceMarker(0, tarch::la::Vector<DIMENSIONS, int>(0));
  exahype2::fv::restrictCoarseningFaceHalves<DofsPerAxis, NumberOfUnknowns>(
    marker,
    stageNew,
    stageOld,
    faceValuesNew.data(),
    faceValuesOld.data(),
    faceLabel
  );

  validateFalse(stageNew.hasFrame(faceValuesNew.data()));
  validateFalse(stageNew.hasFrame(faceValuesNew.data() + 1));
  validateTrue(faceLabel.getUpdated(0));
  validateFalse(faceLabel.getUpdated(1));
  validateFalse(faceLabel.getRequiresDataTransfer(0));
}

/**
 * Checks that a created face takes the time stamp of its parent and a destroyed face raises it.
 */
void exahype2::fv::tests::DynamicAMRTest::testCreateFaceAndDestroyFaceCarryTheTimeStamps() {
  std::array<double, CellPatchValues> coarseCellValuesOld{};
  std::array<double, CellPatchValues> coarseCellValuesNew{};
  std::array<double, FacePatchValues> coarseFaceValuesOld{};
  std::array<double, FacePatchValues> coarseFaceValuesNew{};
  std::array<double, FacePatchValues> fineFaceValuesOld{};
  std::array<double, FacePatchValues> fineFaceValuesNew{};
  coarseCellValuesOld.fill(ConstantField);
  coarseCellValuesNew.fill(ConstantField);
  coarseFaceValuesOld.fill(ConstantField);
  coarseFaceValuesNew.fill(ConstantField);

  exahype2::fv::tests::CellLabel coarseCellLabel;
  coarseCellLabel.setTimeStamp(1.75);
  exahype2::fv::tests::FaceLabel fineFaceLabel;

  const auto marker = createFaceMarker(0, tarch::la::Vector<DIMENSIONS, int>(0));
  exahype2::fv::createFace<DofsPerAxis, NumberOfUnknowns>(
    marker,
    coarseCellValuesOld.data(),
    coarseCellValuesNew.data(),
    coarseFaceValuesOld.data(),
    coarseFaceValuesNew.data(),
    fineFaceValuesOld.data(),
    fineFaceValuesNew.data(),
    coarseCellLabel,
    fineFaceLabel
  );
  validateNumericalEquals(fineFaceLabel.getNewTimeStamp(0), 1.75);
  validateNumericalEquals(fineFaceLabel.getOldTimeStamp(1), 1.75);
  validateNumericalEquals(fineFaceValuesNew[0], ConstantField);

  exahype2::amr::ChildDataStage<double> stageNew(SubfacesPerFace, FaceLayerValues);
  exahype2::amr::ChildDataStage<double> stageOld(SubfacesPerFace, FaceLayerValues);
  exahype2::fv::tests::FaceLabel        coarseFaceLabel;
  fineFaceLabel.setUpdatedTimeStamp(0, 5.0);

  const tarch::la::Vector<DIMENSIONS, double> domainOffset(0.0);
  const tarch::la::Vector<DIMENSIONS, double> domainSize(1.0);
  exahype2::fv::destroyFace<DofsPerAxis, NumberOfUnknowns>(
    marker,
    stageNew,
    stageOld,
    coarseFaceValuesNew.data(),
    coarseFaceValuesOld.data(),
    fineFaceValuesNew.data(),
    fineFaceValuesOld.data(),
    coarseFaceLabel,
    fineFaceLabel,
    domainOffset,
    domainSize
  );
  validateNumericalEquals(coarseFaceLabel.getUpdatedTimeStamp(0), 5.0);
  validateNumericalEquals(coarseFaceLabel.getNewTimeStamp(0), 1.75);
  validateTrue(stageNew.hasFrame(coarseFaceValuesNew.data()));
  validateTrue(stageOld.hasFrame(coarseFaceValuesOld.data() + 1));
  for (int facePosition = 0; facePosition < 2; facePosition++) {
    stageNew.discard(coarseFaceValuesNew.data() + facePosition);
    stageOld.discard(coarseFaceValuesOld.data() + facePosition);
  }
}

/**
 * Checks that a constant patch survives the prolongation into the children and the restriction back.
 */
void exahype2::fv::tests::DynamicAMRTest::testCreateCellAndDestroyCellReproduceAConstantPatch() {
  std::array<double, CellPatchValues> coarseCellValuesNew{};
  std::array<double, CellPatchValues> coarseCellValuesOld{};
  coarseCellValuesNew.fill(ConstantField);
  coarseCellValuesOld.fill(ConstantField);

  std::array<std::array<double, CellPatchValues>, ChildrenPerCell> childValuesNew{};
  std::array<std::array<double, CellPatchValues>, ChildrenPerCell> childValuesOld{};

  exahype2::fv::tests::CellLabel coarseCellLabel;
  coarseCellLabel.setTimeStamp(2.0);
  coarseCellLabel.setTimeStepSize(0.5);
  coarseCellLabel.setHasUpdated(true);

  for (int child = 0; child < ChildrenPerCell; child++) {
    exahype2::fv::tests::CellLabel fineCellLabel;
    const auto                     marker = createCellMarker(childPosition(child), false, false);
    exahype2::fv::createCell<DofsPerAxis, NumberOfUnknowns>(
      marker,
      coarseCellValuesNew.data(),
      childValuesNew[child].data(),
      coarseCellValuesOld.data(),
      childValuesOld[child].data(),
      coarseCellLabel,
      fineCellLabel
    );
    validateNumericalEquals(childValuesNew[child][0], ConstantField);
    validateNumericalEquals(fineCellLabel.getTimeStamp(), 2.0);
    validateTrue(fineCellLabel.getHasUpdated());
    validateTrue(fineCellLabel.getRequiresCriterionEvaluation());
  }

  exahype2::amr::ChildDataStage<double> stageNew(ChildrenPerCell, CellPatchValues);
  exahype2::amr::ChildDataStage<double> stageOld(ChildrenPerCell, CellPatchValues);
  coarseCellValuesNew.fill(0.0);
  coarseCellValuesOld.fill(0.0);

  exahype2::fv::tests::CellLabel parentLabel;
  for (int child = 0; child < ChildrenPerCell; child++) {
    exahype2::fv::tests::CellLabel fineCellLabel;
    fineCellLabel.setTimeStamp(3.0);
    fineCellLabel.setTimeStepSize(0.25);
    fineCellLabel.setHasUpdated(true);
    const auto marker = createCellMarker(childPosition(child), false, false);
    exahype2::fv::destroyCell<DofsPerAxis, NumberOfUnknowns>(
      marker,
      stageNew,
      stageOld,
      coarseCellValuesNew.data(),
      childValuesNew[child].data(),
      coarseCellValuesOld.data(),
      childValuesOld[child].data(),
      parentLabel,
      fineCellLabel
    );
  }
  for (int value = 0; value < CellPatchValues; value++) {
    validateNumericalEquals(coarseCellValuesNew[value], ConstantField);
    validateNumericalEquals(coarseCellValuesOld[value], ConstantField);
  }
  validateNumericalEquals(parentLabel.getTimeStamp(), 3.0);
  validateTrue(parentLabel.getHasUpdated());
  validateTrue(parentLabel.getRequiresCriterionEvaluation());
}

/**
 * Checks that the projection runs on a cell whose face awaits data and on no other.
 */
void exahype2::fv::tests::DynamicAMRTest::testProjectPatchAfterTransferWaitsForAFaceThatAwaitsData() {
  std::array<double, CellPatchValues>                                           cellValues{};
  std::array<exahype2::fv::tests::Patch<FacePatchValues>, peano4::FacesPerCell> facePatches{};
  std::array<exahype2::fv::tests::FaceLabel, peano4::FacesPerCell>              faceLabels{};
  cellValues.fill(ConstantField);

  const auto faceValues          = exahype2::fv::tests::enumerateFaces(facePatches);
  const auto faceLabelEnumerator = exahype2::fv::tests::enumerateFaces(faceLabels);

  exahype2::fv::tests::CellLabel cellLabel;
  cellLabel.setTimeStamp(1.25);
  cellLabel.setRequiresCriterionEvaluation(true);

  const auto leafMarker = createCellMarker(tarch::la::Vector<DIMENSIONS, int>(0), false, false);
  exahype2::fv::projectPatchAfterTransfer<DofsPerAxis, Overlap, NumberOfUnknowns>(
    leafMarker,
    cellValues.data(),
    faceValues,
    faceLabelEnumerator,
    cellLabel
  );
  validateTrue(cellLabel.getRequiresCriterionEvaluation());
  validateNumericalEquals(facePatches[0].data()[0], 0.0);

  faceLabels[0].setRequiresDataTransfer(1, true);
  exahype2::fv::projectPatchAfterTransfer<DofsPerAxis, Overlap, NumberOfUnknowns>(
    leafMarker,
    cellValues.data(),
    faceValues,
    faceLabelEnumerator,
    cellLabel
  );
  validateFalse(cellLabel.getRequiresCriterionEvaluation());
  validateFalse(faceLabels[0].getRequiresDataTransfer(1));
  validateTrue(faceLabels[0].getUpdated(1));
  validateNumericalEquals(faceLabels[0].getUpdatedTimeStamp(1), 1.25);
  validateNumericalEquals(facePatches[0].data()[1], ConstantField);
}
