// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FacePlotterTest.h"

#include "exahype2/plotter/FacePlotter.h"
#include "exahype2/plotter/tests/RecordingFileWriter.h"
#include "tarch/la/Vector.h"

namespace {
  constexpr int PointsPerAxis    = 3;
  constexpr int NumberOfUnknowns = 2;

  constexpr std::array<double, PointsPerAxis> Mapping{0.0, 0.5, 1.0};

  peano4::dataview::FaceMarker createFaceOfUnitCell(const int faceNumber) {
    peano4::dataview::FaceMarker marker{};
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      marker.h(axis)          = 1.0;
      marker.cellCentre(axis) = 0.5;
    }
    marker.select(faceNumber);
    return marker;
  }

  std::vector<double> createFace(const int points) {
    std::vector<double> face(static_cast<std::size_t>(2 * points * NumberOfUnknowns));
    for (int point = 0; point < points; point++) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        const auto entry = static_cast<std::size_t>(point * NumberOfUnknowns + unknown);
        face[entry]      = point * 100.0 + unknown;
        face[entry + static_cast<std::size_t>(points * NumberOfUnknowns)] = 1000.0 + point * 100.0 + unknown;
      }
    }
    return face;
  }
} // namespace

exahype2::plotter::tests::FacePlotterTest::FacePlotterTest() :
  tarch::tests::TestCase("exahype2::plotter::tests::FacePlotterTest") {}

void exahype2::plotter::tests::FacePlotterTest::run() {
  testMethod(testPlotsTheTwoTracesOfOneFace);
  testMethod(testSpansTheAxesOtherThanTheNormalAxis);
}

/**
 * Checks that a face plots one point per node of the face, writes the values of the left cell
 * and of the right cell into two fields per field, and connects the neighbouring nodes.
 */
void exahype2::plotter::tests::FacePlotterTest::testPlotsTheTwoTracesOfOneFace() {
  const int totalPoints = peano4::power(PointsPerAxis, DIMENSIONS - 1);
  const int totalCells  = peano4::power(PointsPerAxis - 1, DIMENSIONS - 1);

  Snapshot snapshot;
  openRecordingSnapshot(snapshot);

  FacePlotter plotter;
  plotter.open(snapshot, 6, {{"q", 1}, {"r", 1}});

  const std::vector<double> face = createFace(totalPoints);
  plotter.plotFace<PointsPerAxis, NumberOfUnknowns>(createFaceOfUnitCell(0), Mapping, face.data());

  const auto& writer = getRecordingFileWriter(snapshot);
  validateEquals(writer.getNumberOfPoints(0), totalPoints);
  validateEquals(writer.getNumberOfCells(0), totalCells);

  const std::vector<std::string> expectedPointFields{"qL", "qR", "rL", "rR"};
  validateTrue(writer.getPointFieldNames(0) == expectedPointFields);

  for (int point = 0; point < totalPoints; point++) {
    const auto entry = static_cast<std::size_t>(point);
    validateNumericalEquals(writer.getPointField(0, "qL")[entry], point * 100.0);
    validateNumericalEquals(writer.getPointField(0, "rL")[entry], point * 100.0 + 1.0);
    validateNumericalEquals(writer.getPointField(0, "qR")[entry], 1000.0 + point * 100.0);
    validateNumericalEquals(writer.getPointField(0, "rR")[entry], 1000.0 + point * 100.0 + 1.0);
  }

#if DIMENSIONS == 2
  const std::vector<std::int32_t> expectedFirstCell{0, 1};
#else
  const std::vector<std::int32_t> expectedFirstCell{0, 1, 4, 3};
#endif
  const std::vector<std::int32_t> connectivity = writer.getConnectivity(0);
  for (int corner = 0; corner < peano4::VerticesPerCell / 2; corner++) {
    validateEquals(connectivity[static_cast<std::size_t>(corner)], expectedFirstCell[static_cast<std::size_t>(corner)]);
  }

  const std::vector<double> levels = writer.getCellField(0, "RefinementLevel");
  validateEquals(static_cast<int>(levels.size()), totalCells);
  for (const double level : levels) {
    validateNumericalEquals(level, 0.0);
  }
  validateNumericalEquals(writer.getCellField(0, "Subtree")[0], 6.0);
}

/**
 * Checks that the plot points of a face run along the axes other than its normal axis, and that
 * the normal axis carries the lower corner of the cell.
 */
void exahype2::plotter::tests::FacePlotterTest::testSpansTheAxesOtherThanTheNormalAxis() {
  const int totalPoints = peano4::power(PointsPerAxis, DIMENSIONS - 1);

  for (int normalAxis = 0; normalAxis < DIMENSIONS; normalAxis++) {
    Snapshot snapshot;
    openRecordingSnapshot(snapshot);

    FacePlotter plotter;
    plotter.open(snapshot, 0, {{"q", 1}, {"r", 1}});

    const auto                marker = createFaceOfUnitCell(normalAxis);
    const std::vector<double> face   = createFace(totalPoints);
    plotter.plotFace<PointsPerAxis, NumberOfUnknowns>(marker, Mapping, face.data());

    const auto& writer = getRecordingFileWriter(snapshot);
    for (int point = 0; point < totalPoints; point++) {
      int tangentialIndex = 0;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        if (axis == normalAxis) {
          validateNumericalEquals(
            writer.getPointCoordinate(0, point, axis),
            marker.getX()(axis) - 0.5 * marker.getH()(axis)
          );
        } else {
          validateNumericalEquals(
            writer.getPointCoordinate(0, point, axis),
            Mapping[digitAlongAxis(point, tangentialIndex, PointsPerAxis)]
          );
          tangentialIndex++;
        }
      }
    }
  }
}
