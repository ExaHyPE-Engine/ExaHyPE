// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LevelJumpEventsTest.h"

#include "exahype2/amr/LevelJumpEvents.h"
#include "peano4/sfc/ActiveCurve.h"

namespace {
  constexpr int SubdivisionFactor = peano4::sfc::ActiveCurve::SubdivisionFactor;
  constexpr int ChildrenPerCell   = peano4::sfc::ActiveCurve::ChildrenPerCell;
  constexpr int SubfacesPerFace   = exahype2::amr::SubfacesPerFace;

  constexpr int DofsPerAxis      = 2;
  constexpr int NumberOfUnknowns = 1;
  constexpr int ValuesPerCellPatch =
    NumberOfUnknowns * (DIMENSIONS == 2 ? DofsPerAxis * DofsPerAxis : DofsPerAxis * DofsPerAxis * DofsPerAxis);

  peano4::dataview::FaceMarker createFaceMarker(
    const int                                 faceNumber,
    const tarch::la::Vector<DIMENSIONS, int>& relativePosition
  ) {
    peano4::dataview::FaceMarker marker{};
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      marker.h[axis]          = 1.0 / SubdivisionFactor;
      marker.cellCentre[axis] = 0.5 * marker.h[axis];
    }
    marker.relativePositionWithinFatherCell = relativePosition;
    marker.select(faceNumber);
    return marker;
  }

  peano4::dataview::CellMarker createCellMarker(const tarch::la::Vector<DIMENSIONS, int>& relativePosition) {
    peano4::dataview::CellMarker marker{};
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      marker.h[axis]      = 1.0 / SubdivisionFactor;
      marker.centre[axis] = 0.5;
    }
    marker.relativePositionWithinFatherCell = relativePosition;
    return marker;
  }

  tarch::la::Vector<DIMENSIONS, int> positionAlongFirstAxis(const int position) {
    tarch::la::Vector<DIMENSIONS, int> result(0);
    result(0) = position;
    return result;
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

  class RecordingFaceInterpolation final {
  public:
    RecordingFaceInterpolation(int* calls, int* lastSide) :
      _calls(calls),
      _lastSide(lastSide) {}

    void operator()(
      const peano4::dataview::FaceMarker& marker,
      const double*                       coarseFaceValues,
      double*                             fineFaceValues,
      const int                           side
    ) const {
      *_calls += 1;
      *_lastSide        = side;
      fineFaceValues[0] = coarseFaceValues[0] + marker.getSelectedFaceNumber();
    }

  private:
    int* const _calls;
    int* const _lastSide;
  };

  class SummingFaceRestriction final {
  public:
    explicit SummingFaceRestriction(int* calls) :
      _calls(calls) {}

    void operator()(
      exahype2::amr::ChildDataStage<double>& stage,
      const void*                            coarseFaceKey,
      double*                                coarseFaceValues
    ) const {
      *_calls += 1;
      const double* stagedSubfaces[SubfacesPerFace];
      exahype2::amr::collectStagedSubfaces(stage, coarseFaceKey, stagedSubfaces);
      coarseFaceValues[0] = 0.0;
      for (int subface = 0; subface < SubfacesPerFace; subface++) {
        if (stagedSubfaces[subface] != nullptr) {
          coarseFaceValues[0] += stagedSubfaces[subface][0];
        }
      }
      stage.discard(coarseFaceKey);
    }

  private:
    int* const _calls;
  };

  class SummingCellRestriction final {
  public:
    explicit SummingCellRestriction(int* calls) :
      _calls(calls) {}

    void operator()(exahype2::amr::ChildDataStage<double>& stage, double* coarseGridValues) const {
      *_calls += 1;
      const double* stagedChildren[ChildrenPerCell];
      exahype2::amr::collectStagedChildren(stage, coarseGridValues, stagedChildren);
      double sum = 0.0;
      for (int child = 0; child < ChildrenPerCell; child++) {
        sum += stagedChildren[child][0];
      }
      coarseGridValues[0] = sum;
      stage.discard(coarseGridValues);
    }

  private:
    int* const _calls;
  };

  class TestFaceLabel final {
  public:
    double getNewTimeStamp(const int side) const { return _newTimeStamp[side]; }
    double getOldTimeStamp(const int side) const { return _oldTimeStamp[side]; }
    double getUpdatedTimeStamp(const int side) const { return _updatedTimeStamp[side]; }
    bool   getUpdated(const int side) const { return _updated[side]; }
    bool   getIsHanging() const { return _isHanging; }
    bool   getRequiresDataTransfer(const int side) const { return _requiresDataTransfer[side]; }

    void setNewTimeStamp(const int side, const double value) { _newTimeStamp[side] = value; }
    void setOldTimeStamp(const int side, const double value) { _oldTimeStamp[side] = value; }
    void setUpdatedTimeStamp(const int side, const double value) { _updatedTimeStamp[side] = value; }
    void setUpdated(const int side, const bool value) { _updated[side] = value; }
    void setIsHanging(const bool value) { _isHanging = value; }
    void setRequiresDataTransfer(const int side, const bool value) { _requiresDataTransfer[side] = value; }

  private:
    double _newTimeStamp[2]{0.0, 0.0};
    double _oldTimeStamp[2]{0.0, 0.0};
    double _updatedTimeStamp[2]{0.0, 0.0};
    bool   _updated[2]{false, false};
    bool   _isHanging{false};
    bool   _requiresDataTransfer[2]{false, false};
  };

  class TestCellLabel final {
  public:
    double getTimeStamp() const { return _timeStamp; }
    double getTimeStepSize() const { return _timeStepSize; }
    bool   getHasUpdated() const { return _hasUpdated; }
    bool   getRequiresCriterionEvaluation() const { return _requiresCriterionEvaluation; }

    void setTimeStamp(const double value) { _timeStamp = value; }
    void setTimeStepSize(const double value) { _timeStepSize = value; }
    void setHasUpdated(const bool value) { _hasUpdated = value; }
    void setRequiresCriterionEvaluation(const bool value) { _requiresCriterionEvaluation = value; }

  private:
    double _timeStamp{0.0};
    double _timeStepSize{0.0};
    bool   _hasUpdated{false};
    bool   _requiresCriterionEvaluation{false};
  };

  class TestFaceLabelEnumerator final {
  public:
    explicit TestFaceLabelEnumerator(TestFaceLabel* labels) :
      _labels(labels) {}

    TestFaceLabel& operator()(const int face) const { return _labels[face]; }

  private:
    TestFaceLabel* const _labels;
  };

  /**
   * Carries Cardinality values plus one entry behind them that a clear running too far
   * overwrites.
   */
  class TestRecord final {
  public:
    static constexpr int Cardinality = 3;

    double*              data() { return _values.data(); }
    [[nodiscard]] double entryBehindTheRecord() const { return _values[Cardinality]; }
    void                 fill(const double value) { _values.fill(value); }

  private:
    std::array<double, Cardinality + 1> _values{};
  };
} // namespace

exahype2::amr::tests::LevelJumpEventsTest::LevelJumpEventsTest() :
  tarch::tests::TestCase("exahype2::amr::tests::LevelJumpEventsTest") {}

void exahype2::amr::tests::LevelJumpEventsTest::run() {
  testMethod(testClearTransferBuffer);
  testMethod(testClearTransferRecord);
  testMethod(testSubfaceIndexWithinFatherFace);
  testMethod(testChildIndexWithinFatherCell);
  testMethod(testIsOnDomainBoundary);
  testMethod(testInterpolateFaceSkipsSubfaceInsideThePatch);
  testMethod(testRestrictFaceFromStagedSubfaces);
  testMethod(testCollectStagedSubfacesReportsAnAbsentSubface);
  testMethod(testRestrictChildrenIntoCellWaitsForTheSiblingSet);
  testMethod(testProlongCellIntoChild);
  testMethod(testFaceLabelBookkeeping);
  testMethod(testCellLabelBookkeeping);
  testMethod(testDataTransferFlags);
  testMethod(testCellAveragingKernelsAverageTheChildren);
}

/**
 * Checks that the clear writes zeroes over the whole buffer and nothing beyond it.
 */
void exahype2::amr::tests::LevelJumpEventsTest::testClearTransferBuffer() {
  std::array<double, 5> values{};
  values.fill(7.0);
  exahype2::amr::clearTransferBuffer(values.data(), 4);
  for (int entry = 0; entry < 4; entry++) {
    validateNumericalEquals(values[entry], 0.0);
  }
  validateNumericalEquals(values[4], 7.0);
}

/**
 * Checks that the clear of a data record covers the Cardinality entries the record declares and
 * nothing behind them.
 */
void exahype2::amr::tests::LevelJumpEventsTest::testClearTransferRecord() {
  TestRecord record;
  record.fill(7.0);
  exahype2::amr::clearTransferRecord(record);
  for (int entry = 0; entry < TestRecord::Cardinality; entry++) {
    validateNumericalEquals(record.data()[entry], 0.0);
  }
  validateNumericalEquals(record.entryBehindTheRecord(), 7.0);
}

/**
 * Checks that the subface index enumerates the tangential positions of the father face.
 */
void exahype2::amr::tests::LevelJumpEventsTest::testSubfaceIndexWithinFatherFace() {
  for (int subface = 0; subface < SubfacesPerFace; subface++) {
    const auto marker = createFaceMarker(0, subfacePosition(0, subface));
    validateEquals(exahype2::amr::subfaceIndexWithinFatherFace(marker), subface);
  }
}

/**
 * Checks that the child index enumerates the children of the father cell.
 */
void exahype2::amr::tests::LevelJumpEventsTest::testChildIndexWithinFatherCell() {
  for (int position = 0; position < SubdivisionFactor; position++) {
    const auto marker = createCellMarker(positionAlongFirstAxis(position));
    validateEquals(exahype2::amr::childIndexWithinFatherCell(marker), position);
  }
  tarch::la::Vector<DIMENSIONS, int> lastChild(SubdivisionFactor - 1);
  validateEquals(exahype2::amr::childIndexWithinFatherCell(createCellMarker(lastChild)), ChildrenPerCell - 1);
}

/**
 * Checks that a face on a wall of the domain is recognised and an interior face is not.
 */
void exahype2::amr::tests::LevelJumpEventsTest::testIsOnDomainBoundary() {
  const tarch::la::Vector<DIMENSIONS, double> offset(0.0);
  const tarch::la::Vector<DIMENSIONS, double> size(1.0);

  auto onWall = createFaceMarker(0, positionAlongFirstAxis(0));
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    onWall.cellCentre[axis] = 0.5 * onWall.h[axis];
  }
  validateTrue(exahype2::amr::isOnDomainBoundary(onWall, offset, size));

  auto inside = onWall;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    inside.cellCentre[axis] = 0.5;
  }
  validateFalse(exahype2::amr::isOnDomainBoundary(inside, offset, size));
}

/**
 * Checks that only a subface on a face of the coarse cell takes data from that face, and that
 * the marker and the side reach the kernel.
 */
void exahype2::amr::tests::LevelJumpEventsTest::testInterpolateFaceSkipsSubfaceInsideThePatch() {
  int    calls    = 0;
  int    lastSide = -1;
  double coarse   = 3.0;
  double fine     = 0.0;

  const auto onCoarseFace = createFaceMarker(0, subfacePosition(0, 0));
  validateFalse(onCoarseFace.isInteriorFaceWithinPatch());
  exahype2::amr::interpolateFaceFromCoarseFace(
    onCoarseFace,
    &coarse,
    &fine,
    0,
    RecordingFaceInterpolation(&calls, &lastSide)
  );
  validateEquals(calls, 1);
  validateEquals(lastSide, 0);
  validateNumericalEquals(fine, 3.0);

  const auto insidePatch = createFaceMarker(0, positionAlongFirstAxis(1));
  validateTrue(insidePatch.isInteriorFaceWithinPatch());
  exahype2::amr::interpolateFaceFromCoarseFace(
    insidePatch,
    &coarse,
    &fine,
    1,
    RecordingFaceInterpolation(&calls, &lastSide)
  );
  validateEquals(calls, 1);

  const auto onUpperCoarseFace = createFaceMarker(DIMENSIONS, subfacePosition(DIMENSIONS, 0));
  exahype2::amr::interpolateFaceFromCoarseFace(
    onUpperCoarseFace,
    &coarse,
    &fine,
    1,
    RecordingFaceInterpolation(&calls, &lastSide)
  );
  validateEquals(calls, 2);
  validateEquals(lastSide, 1);
  validateNumericalEquals(fine, 3.0 + DIMENSIONS);
}

/**
 * Checks that a face reduces its staged subfaces once and reports an empty frame.
 */
void exahype2::amr::tests::LevelJumpEventsTest::testRestrictFaceFromStagedSubfaces() {
  constexpr double                      ExpectedSum = 0.5 * SubfacesPerFace * (SubfacesPerFace + 1.0);
  exahype2::amr::ChildDataStage<double> stage(SubfacesPerFace, 1);
  double                                coarseFaceValues = 0.0;
  std::array<double, SubfacesPerFace>   subfaceValues{};

  for (int subface = 0; subface < SubfacesPerFace; subface++) {
    subfaceValues[subface] = subface + 1.0;
    const auto marker      = createFaceMarker(0, subfacePosition(0, subface));
    exahype2::amr::stageSubfaceForRestriction(marker, stage, &coarseFaceValues, &subfaceValues[subface]);
  }

  int calls = 0;
  validateTrue(
    exahype2::amr::restrictFaceFromStagedSubfaces(
      stage,
      &coarseFaceValues,
      &coarseFaceValues,
      SummingFaceRestriction(&calls)
    )
  );
  validateEquals(calls, 1);
  validateNumericalEquals(coarseFaceValues, ExpectedSum);

  validateFalse(
    exahype2::amr::restrictFaceFromStagedSubfaces(
      stage,
      &coarseFaceValues,
      &coarseFaceValues,
      SummingFaceRestriction(&calls)
    )
  );
  validateEquals(calls, 1);
  validateNumericalEquals(coarseFaceValues, ExpectedSum);
}

/**
 * Checks that a subface another tree holds reaches the kernel as a null entry.
 */
void exahype2::amr::tests::LevelJumpEventsTest::testCollectStagedSubfacesReportsAnAbsentSubface() {
  exahype2::amr::ChildDataStage<double> stage(SubfacesPerFace, 1);
  double                                coarseFaceValues = 0.0;
  double                                subfaceValue     = 4.0;

  const auto marker = createFaceMarker(0, subfacePosition(0, 0));
  exahype2::amr::stageSubfaceForRestriction(marker, stage, &coarseFaceValues, &subfaceValue);

  const double* stagedSubfaces[SubfacesPerFace];
  exahype2::amr::collectStagedSubfaces(stage, &coarseFaceValues, stagedSubfaces);
  validateNumericalEquals(*stagedSubfaces[0], 4.0);
  for (int subface = 1; subface < SubfacesPerFace; subface++) {
    validateTrue(stagedSubfaces[subface] == nullptr);
  }
  stage.discard(&coarseFaceValues);
}

/**
 * Checks that the reduction runs on the last child of the sibling set and on no other.
 */
void exahype2::amr::tests::LevelJumpEventsTest::testRestrictChildrenIntoCellWaitsForTheSiblingSet() {
  exahype2::amr::ChildDataStage<double> stage(ChildrenPerCell, 1);
  double                                coarseGridValues = 0.0;
  std::array<double, ChildrenPerCell>   childValues{};
  int                                   calls = 0;

  for (int child = 0; child < ChildrenPerCell; child++) {
    childValues[child] = child + 1.0;
    tarch::la::Vector<DIMENSIONS, int> position;
    int                                remainder = child;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      position(axis) = remainder % SubdivisionFactor;
      remainder /= SubdivisionFactor;
    }
    const bool reduced = exahype2::amr::restrictChildrenIntoCell(
      createCellMarker(position),
      stage,
      &coarseGridValues,
      &childValues[child],
      SummingCellRestriction(&calls)
    );
    validateEquals(reduced, (child == ChildrenPerCell - 1));
  }
  validateEquals(calls, 1);
  validateNumericalEquals(coarseGridValues, 0.5 * ChildrenPerCell * (ChildrenPerCell + 1.0));
}

/**
 * Checks that the prolongation hands the kernel the coarse value of the child it fills.
 */
void exahype2::amr::tests::LevelJumpEventsTest::testProlongCellIntoChild() {
  const auto                                              cellMarker = createCellMarker(positionAlongFirstAxis(1));
  const exahype2::amr::CellAveragingInterpolation<double> prolongCell(1, 1);
  double                                                  coarseCell = 5.0;
  double                                                  fineCell   = 0.0;
  exahype2::amr::prolongCellIntoChild(cellMarker, &coarseCell, &fineCell, prolongCell);
  validateNumericalEquals(fineCell, 5.0);
}

/**
 * Checks the time stamp transfers between a face and the face above or below it.
 */
void exahype2::amr::tests::LevelJumpEventsTest::testFaceLabelBookkeeping() {
  TestFaceLabel coarseFaceLabel;
  TestFaceLabel fineFaceLabel;
  coarseFaceLabel.setNewTimeStamp(0, 4.0);
  coarseFaceLabel.setOldTimeStamp(0, 3.0);
  exahype2::amr::copyFaceTimeStampsFromCoarseFace(coarseFaceLabel, fineFaceLabel, 0);
  validateNumericalEquals(fineFaceLabel.getNewTimeStamp(0), 4.0);
  validateNumericalEquals(fineFaceLabel.getOldTimeStamp(0), 3.0);
  validateNumericalEquals(fineFaceLabel.getNewTimeStamp(1), 0.0);

  fineFaceLabel.setNewTimeStamp(1, 9.0);
  exahype2::amr::raiseFaceTimeStampsFromCoarseFace(coarseFaceLabel, fineFaceLabel);
  validateNumericalEquals(fineFaceLabel.getNewTimeStamp(0), 4.0);
  validateNumericalEquals(fineFaceLabel.getNewTimeStamp(1), 9.0);

  TestFaceLabel destroyedFaceLabel;
  destroyedFaceLabel.setUpdatedTimeStamp(1, 6.0);
  exahype2::amr::raiseFaceTimeStampsOntoCoarseFace(coarseFaceLabel, destroyedFaceLabel);
  validateNumericalEquals(coarseFaceLabel.getUpdatedTimeStamp(1), 6.0);
  validateNumericalEquals(coarseFaceLabel.getNewTimeStamp(0), 4.0);

  TestFaceLabel hangingFaceLabel;
  hangingFaceLabel.setUpdatedTimeStamp(0, 8.0);
  exahype2::amr::raiseUpdatedTimeStampOntoCoarseFace(coarseFaceLabel, hangingFaceLabel, 0);
  validateTrue(coarseFaceLabel.getUpdated(0));
  validateNumericalEquals(coarseFaceLabel.getUpdatedTimeStamp(0), 8.0);

  hangingFaceLabel.setUpdatedTimeStamp(0, 2.0);
  exahype2::amr::copyUpdatedTimeStampOntoCoarseFace(coarseFaceLabel, hangingFaceLabel, 0);
  validateNumericalEquals(coarseFaceLabel.getUpdatedTimeStamp(0), 2.0);

  TestCellLabel coarseCellLabel;
  coarseCellLabel.setTimeStamp(1.5);
  TestFaceLabel createdFaceLabel;
  exahype2::amr::setFaceTimeStampsFromCoarseCell(coarseCellLabel, createdFaceLabel);
  validateNumericalEquals(createdFaceLabel.getNewTimeStamp(0), 1.5);
  validateNumericalEquals(createdFaceLabel.getOldTimeStamp(1), 1.5);
}

/**
 * Checks the time stepping transfers between a cell and the cell above or below it.
 */
void exahype2::amr::tests::LevelJumpEventsTest::testCellLabelBookkeeping() {
  TestCellLabel coarseCellLabel;
  TestCellLabel fineCellLabel;
  coarseCellLabel.setTimeStamp(2.0);
  coarseCellLabel.setTimeStepSize(0.25);
  coarseCellLabel.setHasUpdated(true);

  exahype2::amr::copyCellLabelIntoChild(coarseCellLabel, fineCellLabel);
  validateNumericalEquals(fineCellLabel.getTimeStamp(), 2.0);
  validateNumericalEquals(fineCellLabel.getTimeStepSize(), 0.25);
  validateTrue(fineCellLabel.getHasUpdated());
  validateTrue(fineCellLabel.getRequiresCriterionEvaluation());

  TestCellLabel parentLabel;
  TestCellLabel childLabel;
  childLabel.setTimeStamp(3.0);
  childLabel.setTimeStepSize(0.5);
  childLabel.setHasUpdated(true);
  exahype2::amr::copyCellTimeStampsFromChild(parentLabel, childLabel);
  validateNumericalEquals(parentLabel.getTimeStamp(), 3.0);
  validateFalse(parentLabel.getHasUpdated());

  parentLabel.setTimeStamp(4.0);
  exahype2::amr::raiseCellTimeStampsFromChild(parentLabel, childLabel);
  validateNumericalEquals(parentLabel.getTimeStamp(), 4.0);

  exahype2::amr::mergeCellLabelFromChild(parentLabel, childLabel);
  validateNumericalEquals(parentLabel.getTimeStamp(), 3.0);
  validateTrue(parentLabel.getHasUpdated());
  validateTrue(parentLabel.getRequiresCriterionEvaluation());
}

/**
 * Checks that the transfer flags travel over the side of each face that the cell writes.
 */
void exahype2::amr::tests::LevelJumpEventsTest::testDataTransferFlags() {
  std::array<TestFaceLabel, peano4::FacesPerCell> labels{};
  const TestFaceLabelEnumerator                   faceLabels(labels.data());

  exahype2::amr::requestDataTransferOnOwnSides(faceLabels);
  for (int face = 0; face < peano4::FacesPerCell; face++) {
    const int ownSide = face < DIMENSIONS ? 1 : 0;
    validateTrue(faceLabels(face).getRequiresDataTransfer(ownSide));
    validateFalse(faceLabels(face).getRequiresDataTransfer(1 - ownSide));
  }

  const auto leafMarker = createCellMarker(positionAlongFirstAxis(0));
  validateTrue(exahype2::amr::awaitsDataTransfer(leafMarker, faceLabels));

  exahype2::amr::markOwnSidesUpdated(faceLabels, 1.25);
  for (int face = 0; face < peano4::FacesPerCell; face++) {
    const int ownSide = face < DIMENSIONS ? 1 : 0;
    validateTrue(faceLabels(face).getUpdated(ownSide));
    validateNumericalEquals(faceLabels(face).getUpdatedTimeStamp(ownSide), 1.25);
  }

  labels[0].setIsHanging(true);
  exahype2::amr::releaseDataTransferOnOwnSides(faceLabels);
  validateTrue(faceLabels(0).getRequiresDataTransfer(1));
  validateFalse(exahype2::amr::awaitsDataTransfer(leafMarker, faceLabels));

  labels[0].setIsHanging(false);
  validateTrue(exahype2::amr::awaitsDataTransfer(leafMarker, faceLabels));
  exahype2::amr::releaseDataTransferOnOwnSides(faceLabels);
  validateFalse(exahype2::amr::awaitsDataTransfer(leafMarker, faceLabels));
}

/**
 * Checks that the two averaging kernels reproduce a constant patch through the whole cycle.
 */
void exahype2::amr::tests::LevelJumpEventsTest::testCellAveragingKernelsAverageTheChildren() {
  const exahype2::amr::CellAveragingInterpolation<double> prolongCell(DofsPerAxis, NumberOfUnknowns);
  std::array<double, ValuesPerCellPatch>                  coarseGridValues{};
  coarseGridValues.fill(2.0);

  exahype2::amr::ChildDataStage<double>                               stage(ChildrenPerCell, ValuesPerCellPatch);
  std::array<std::array<double, ValuesPerCellPatch>, ChildrenPerCell> childValues{};
  int                                                                 reductions = 0;

  for (int child = 0; child < ChildrenPerCell; child++) {
    tarch::la::Vector<DIMENSIONS, int> position;
    int                                remainder = child;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      position(axis) = remainder % SubdivisionFactor;
      remainder /= SubdivisionFactor;
    }
    const auto marker = createCellMarker(position);
    exahype2::amr::prolongCellIntoChild(marker, coarseGridValues.data(), childValues[child].data(), prolongCell);
    for (int value = 0; value < ValuesPerCellPatch; value++) {
      validateNumericalEquals(childValues[child][value], 2.0);
    }
    if (exahype2::amr::stageChildForRestriction(marker, stage, coarseGridValues.data(), childValues[child].data())) {
      reductions++;
    }
  }
  validateEquals(reductions, 1);

  coarseGridValues.fill(0.0);
  exahype2::amr::restrictStagedChildrenIntoCell(
    stage,
    coarseGridValues.data(),
    exahype2::amr::CellAveragingRestriction<double>(DofsPerAxis, NumberOfUnknowns)
  );
  for (int value = 0; value < ValuesPerCellPatch; value++) {
    validateNumericalEquals(coarseGridValues[value], 2.0);
  }
}
