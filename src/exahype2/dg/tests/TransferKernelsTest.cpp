// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TransferKernelsTest.h"

#include "exahype2/amr/ChildDataStage.h"
#include "exahype2/amr/LevelJumpEvents.h"
#include "exahype2/dg/DGUtils.h"
#include "exahype2/dg/Interpolation.h"
#include "exahype2/dg/Restriction.h"
#include "peano4/dataview/CellMarker.h"
#include "peano4/dataview/FaceMarker.h"
#include "peano4/sfc/ActiveCurve.h"

namespace {
  constexpr int SubdivisionFactor = peano4::sfc::ActiveCurve::SubdivisionFactor;
  constexpr int ChildrenPerCell   = peano4::sfc::ActiveCurve::ChildrenPerCell;
  constexpr int SubfacesPerFace   = exahype2::amr::SubfacesPerFace;

  constexpr int DofsPerAxis   = 2;
  constexpr int ValuesPerNode = 1;

  constexpr int FaceValues    = exahype2::dg::NodesPerFace<DofsPerAxis> * ValuesPerNode;
  constexpr int CellValues    = exahype2::dg::NodesPerCell<DofsPerAxis> * ValuesPerNode;
  constexpr int MatrixEntries = SubdivisionFactor * DofsPerAxis * DofsPerAxis;

  constexpr double ConstantTrace     = 2.25;
  constexpr double RestrictionWeight = 1.0 / SubdivisionFactor;

  /**
   * Returns the factor the operator of one child interval applies, which differs between the
   * intervals.
   */
  double intervalFactor(const int interval, const double scale) { return scale * (interval + 1); }

  /**
   * Returns an operator that maps every node onto the node of the same index with the factor of
   * its child interval.
   */
  std::array<double, MatrixEntries> intervalOperator(const double scale) {
    std::array<double, MatrixEntries> result{};
    for (int interval = 0; interval < SubdivisionFactor; interval++) {
      for (int node = 0; node < DofsPerAxis; node++) {
        result[interval * DofsPerAxis * DofsPerAxis + node * DofsPerAxis + node] = intervalFactor(interval, scale);
      }
    }
    return result;
  }

  /**
   * Returns an operator that maps every node onto the node of the same index in every interval.
   */
  std::array<double, MatrixEntries> identityOperator() {
    std::array<double, MatrixEntries> result{};
    for (int interval = 0; interval < SubdivisionFactor; interval++) {
      for (int node = 0; node < DofsPerAxis; node++) {
        result[interval * DofsPerAxis * DofsPerAxis + node * DofsPerAxis + node] = 1.0;
      }
    }
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

  /**
   * Returns the product of the interval factors along the axes of the child.
   */
  double childFactor(const int child, const double scale) {
    double factor = 1.0;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      factor *= intervalFactor(childPosition(child)(axis), scale);
    }
    return factor;
  }

  /**
   * Returns the product of the interval factors along the tangential axes of the subface.
   */
  double subfaceFactor(const int subface, const double scale) {
    double factor    = 1.0;
    int    remainder = subface;
    for (int tangentialAxis = 0; tangentialAxis < DIMENSIONS - 1; tangentialAxis++) {
      factor *= intervalFactor(remainder % SubdivisionFactor, scale);
      remainder /= SubdivisionFactor;
    }
    return factor;
  }

  peano4::dataview::FaceMarker createFaceMarker(const int faceNumber, const int subfaceIndex) {
    peano4::dataview::FaceMarker marker{};
    const int                    normal    = faceNumber % DIMENSIONS;
    int                          remainder = subfaceIndex;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      marker.h[axis]          = 1.0 / SubdivisionFactor;
      marker.cellCentre[axis] = 0.5;
      if (axis != normal) {
        marker.relativePositionWithinFatherCell(axis) = remainder % SubdivisionFactor;
        remainder /= SubdivisionFactor;
      }
    }
    marker.relativePositionWithinFatherCell(normal) = faceNumber < DIMENSIONS ? 0 : SubdivisionFactor - 1;
    marker.select(faceNumber);
    return marker;
  }

  peano4::dataview::CellMarker createCellMarker(const int child) {
    peano4::dataview::CellMarker marker{};
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      marker.h[axis]      = 1.0 / SubdivisionFactor;
      marker.centre[axis] = 0.5;
    }
    marker.relativePositionWithinFatherCell = childPosition(child);
    return marker;
  }
} // namespace

exahype2::dg::tests::TransferKernelsTest::TransferKernelsTest() :
  tarch::tests::TestCase("exahype2::dg::tests::TransferKernelsTest") {}

void exahype2::dg::tests::TransferKernelsTest::run() {
  testMethod(testFaceProlongationAppliesTheOperatorOfEachSubface);
  testMethod(testFaceRestrictionSumsTheWeightedSubfaces);
  testMethod(testCellProlongationAppliesTheOperatorOfEachChild);
  testMethod(testCellRestrictionSumsTheWeightedChildren);
  testMethod(testNodalCellInterpolationReproducesTheCoarseState);
}

/**
 * Checks that every subface takes the coarse trace through the operators of its own intervals.
 */
void exahype2::dg::tests::TransferKernelsTest::testFaceProlongationAppliesTheOperatorOfEachSubface() {
  const auto                     matrix = intervalOperator(1.0);
  std::array<double, FaceValues> coarseFaceValues{};
  coarseFaceValues.fill(ConstantTrace);

  const exahype2::dg::FaceProlongation<double> prolongFace(matrix.data(), DofsPerAxis, ValuesPerNode);
  for (int subface = 0; subface < SubfacesPerFace; subface++) {
    std::array<double, FaceValues> fineFaceValues{};
    prolongFace(createFaceMarker(0, subface), coarseFaceValues.data(), fineFaceValues.data(), 0);
    for (int value = 0; value < FaceValues; value++) {
      validateNumericalEqualsWithParams2(
        fineFaceValues[value],
        subfaceFactor(subface, 1.0) * ConstantTrace,
        subface,
        value
      );
    }
  }
}

/**
 * Checks that a face sums its subfaces, each weighted through the operators of its intervals.
 */
void exahype2::dg::tests::TransferKernelsTest::testFaceRestrictionSumsTheWeightedSubfaces() {
  const auto                                                  matrix = intervalOperator(RestrictionWeight);
  exahype2::amr::ChildDataStage<double>                       stage(SubfacesPerFace, FaceValues);
  std::array<double, FaceValues>                              coarseFaceValues{};
  std::array<std::array<double, FaceValues>, SubfacesPerFace> subfaceValues{};

  coarseFaceValues.fill(-1.0);
  double expected = 0.0;
  for (int subface = 0; subface < SubfacesPerFace; subface++) {
    subfaceValues[subface].fill(ConstantTrace * (subface + 1));
    expected += subfaceFactor(subface, RestrictionWeight) * ConstantTrace * (subface + 1);
    exahype2::amr::stageSubfaceForRestriction(
      createFaceMarker(0, subface),
      stage,
      coarseFaceValues.data(),
      subfaceValues[subface].data()
    );
  }

  const exahype2::dg::FaceRestriction<double> restrictFace(matrix.data(), DofsPerAxis, ValuesPerNode);
  restrictFace(stage, coarseFaceValues.data(), coarseFaceValues.data());

  validateFalse(stage.hasFrame(coarseFaceValues.data()));
  for (int value = 0; value < FaceValues; value++) {
    validateNumericalEqualsWithParams1(coarseFaceValues[value], expected, value);
  }
}

/**
 * Checks that every child takes the state of its parent through the operators of its intervals.
 */
void exahype2::dg::tests::TransferKernelsTest::testCellProlongationAppliesTheOperatorOfEachChild() {
  const auto                     matrix = intervalOperator(1.0);
  std::array<double, CellValues> coarseCellValues{};
  coarseCellValues.fill(ConstantTrace);

  const exahype2::dg::CellProlongation<double> prolongCell(matrix.data(), DofsPerAxis, ValuesPerNode);
  for (int child = 0; child < ChildrenPerCell; child++) {
    std::array<double, CellValues> fineCellValues{};
    prolongCell(createCellMarker(child), coarseCellValues.data(), fineCellValues.data());
    for (int value = 0; value < CellValues; value++) {
      validateNumericalEqualsWithParams2(fineCellValues[value], childFactor(child, 1.0) * ConstantTrace, child, value);
    }
  }
}

/**
 * Checks that a cell sums its children, each weighted through the operators of its intervals.
 */
void exahype2::dg::tests::TransferKernelsTest::testCellRestrictionSumsTheWeightedChildren() {
  const auto                                                  matrix = intervalOperator(RestrictionWeight);
  exahype2::amr::ChildDataStage<double>                       stage(ChildrenPerCell, CellValues);
  std::array<double, CellValues>                              coarseCellValues{};
  std::array<std::array<double, CellValues>, ChildrenPerCell> childValues{};

  double expected = 0.0;
  for (int child = 0; child < ChildrenPerCell; child++) {
    childValues[child].fill(ConstantTrace * (child + 1));
    expected += childFactor(child, RestrictionWeight) * ConstantTrace * (child + 1);
    exahype2::amr::stageChildForRestriction(
      createCellMarker(child),
      stage,
      coarseCellValues.data(),
      childValues[child].data()
    );
  }

  const exahype2::dg::CellRestriction<double> restrictCell(matrix.data(), DofsPerAxis, ValuesPerNode);
  restrictCell(stage, coarseCellValues.data());

  validateFalse(stage.hasFrame(coarseCellValues.data()));
  for (int value = 0; value < CellValues; value++) {
    validateNumericalEqualsWithParams1(coarseCellValues[value], expected, value);
  }
}

/**
 * Checks that the nodal interpolation carries a constant state onto every child.
 */
void exahype2::dg::tests::TransferKernelsTest::testNodalCellInterpolationReproducesTheCoarseState() {
  const auto                     matrix = identityOperator();
  std::array<double, CellValues> coarseCellValues{};
  coarseCellValues.fill(ConstantTrace);

  const exahype2::dg::NodalCellInterpolation<double> interpolateCell(matrix.data(), DofsPerAxis, ValuesPerNode);
  for (int child = 0; child < ChildrenPerCell; child++) {
    std::array<double, CellValues> fineCellValues{};
    interpolateCell(createCellMarker(child), coarseCellValues.data(), fineCellValues.data());
    for (int value = 0; value < CellValues; value++) {
      validateNumericalEqualsWithParams2(fineCellValues[value], ConstantTrace, child, value);
    }
  }
}
