// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "PatchPlotterTest.h"

#include "exahype2/plotter/PatchPlotter.h"
#include "exahype2/plotter/tests/RecordingFileWriter.h"
#include "tarch/la/Vector.h"

namespace {
  constexpr int PointsPerAxis    = 3;
  constexpr int VolumesPerAxis   = 2;
  constexpr int NumberOfUnknowns = 2;
  constexpr int PatchLevel       = 3;

  constexpr std::array<double, PointsPerAxis> Mapping{0.0, 0.5, 1.0};

  peano4::dataview::CellMarker createUnitCell() {
    peano4::dataview::CellMarker marker{};
    marker.level = PatchLevel;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      marker.h[axis]      = 1.0;
      marker.centre[axis] = 0.5;
    }
    return marker;
  }

  tarch::la::Vector<DIMENSIONS, double> unitDomainOffset() { return tarch::la::Vector<DIMENSIONS, double>(0.0); }

  tarch::la::Vector<DIMENSIONS, double> unitDomainSize() { return tarch::la::Vector<DIMENSIONS, double>(1.0); }

  std::vector<double> createPatch(const int values) {
    std::vector<double> patch(static_cast<std::size_t>(values * NumberOfUnknowns));
    for (int value = 0; value < values; value++) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        patch[static_cast<std::size_t>(value * NumberOfUnknowns + unknown)] = value * 100.0 + unknown;
      }
    }
    return patch;
  }
} // namespace

exahype2::plotter::tests::PatchPlotterTest::PatchPlotterTest() :
  tarch::tests::TestCase("exahype2::plotter::tests::PatchPlotterTest") {}

void exahype2::plotter::tests::PatchPlotterTest::run() {
  testMethod(testNodalPatchPlotsOnePointPerNode);
  testMethod(testNodalPatchReadsTheAlternativePatch);
  testMethod(testCellPatchPlotsOneCellPerVolume);
  testMethod(testCellPatchRunsTheCellDataPlotsOfTheCaller);
  testMethod(testNodalPatchRunsTheCellDataPlotsOfTheCaller);
}

/**
 * Checks that a nodal patch plots one point per node at the mapped position, writes the values
 * of that node, and connects the neighbouring nodes to one element each.
 */
void exahype2::plotter::tests::PatchPlotterTest::testNodalPatchPlotsOnePointPerNode() {
  const int totalPoints = peano4::power(PointsPerAxis, DIMENSIONS);
  const int totalCells  = peano4::power(PointsPerAxis - 1, DIMENSIONS);

  Snapshot snapshot;
  openRecordingSnapshot(snapshot);

  NodalPatchPlotter plotter;
  plotter.open(snapshot, 2, {{"rho", 1}, {"e", 1}}, []() {});

  const std::vector<double> patch = createPatch(totalPoints);
  plotter.plotCell<PointsPerAxis, NumberOfUnknowns, 1>(
    createUnitCell(),
    unitDomainOffset(),
    unitDomainSize(),
    Mapping,
    patch.data(),
    false,
    nullptr,
    []([[maybe_unused]] const int cellIndex) {}
  );

  const auto& writer = getRecordingFileWriter(snapshot);
  validateEquals(writer.getNumberOfPoints(0), totalPoints);
  validateEquals(writer.getNumberOfCells(0), totalCells);

  for (int point = 0; point < totalPoints; point++) {
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      validateNumericalEquals(
        writer.getPointCoordinate(0, point, axis),
        Mapping[digitAlongAxis(point, axis, PointsPerAxis)]
      );
    }
  }

  const std::vector<double> density = writer.getPointField(0, "rho");
  const std::vector<double> energy  = writer.getPointField(0, "e");
  validateEquals(static_cast<int>(density.size()), totalPoints);
  validateEquals(static_cast<int>(energy.size()), totalPoints);
  for (int point = 0; point < totalPoints; point++) {
    validateNumericalEquals(density[static_cast<std::size_t>(point)], point * 100.0);
    validateNumericalEquals(energy[static_cast<std::size_t>(point)], point * 100.0 + 1.0);
  }

#if DIMENSIONS == 2
  const std::vector<std::int32_t> expectedFirstCell{0, 1, 4, 3};
#else
  const std::vector<std::int32_t> expectedFirstCell{0, 1, 4, 3, 9, 10, 13, 12};
#endif
  const std::vector<std::int32_t> connectivity = writer.getConnectivity(0);
  for (int corner = 0; corner < peano4::VerticesPerCell; corner++) {
    validateEquals(connectivity[static_cast<std::size_t>(corner)], expectedFirstCell[static_cast<std::size_t>(corner)]);
  }

  const std::vector<std::string> expectedCellFields{"Subdomain", "Subtree", "RefinementLevel", "CurvePosition"};
  validateTrue(writer.getCellFieldNames(0) == expectedCellFields);
  const std::vector<double> levels = writer.getCellField(0, "RefinementLevel");
  validateEquals(static_cast<int>(levels.size()), totalCells);
  for (const double level : levels) {
    validateNumericalEquals(level, static_cast<double>(PatchLevel));
  }
}

/**
 * Checks that every plot point of a nodal patch reads the degree of freedom of the alternative
 * patch that lies closest to it.
 */
void exahype2::plotter::tests::PatchPlotterTest::testNodalPatchReadsTheAlternativePatch() {
  constexpr int                            AlternativePointsPerAxis = 2;
  constexpr std::array<int, PointsPerAxis> ExpectedIndexAlongAxis{0, 1, 1};

  const int totalPoints            = peano4::power(PointsPerAxis, DIMENSIONS);
  const int alternativeTotalPoints = peano4::power(AlternativePointsPerAxis, DIMENSIONS);

  Snapshot snapshot;
  openRecordingSnapshot(snapshot);

  NodalPatchPlotter plotter;
  plotter.open(snapshot, 0, {{"rho", 1}, {"e", 1}}, []() {});

  const std::vector<double> patch            = createPatch(totalPoints);
  const std::vector<double> alternativePatch = createPatch(alternativeTotalPoints);
  plotter.plotCell<PointsPerAxis, NumberOfUnknowns, AlternativePointsPerAxis>(
    createUnitCell(),
    unitDomainOffset(),
    unitDomainSize(),
    Mapping,
    patch.data(),
    true,
    alternativePatch.data(),
    []([[maybe_unused]] const int cellIndex) {}
  );

  const std::vector<double> density = getRecordingFileWriter(snapshot).getPointField(0, "rho");
  validateEquals(static_cast<int>(density.size()), totalPoints);
  for (int point = 0; point < totalPoints; point++) {
    int alternativePoint = 0;
    int stride           = 1;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      alternativePoint +=
        ExpectedIndexAlongAxis[static_cast<std::size_t>(digitAlongAxis(point, axis, PointsPerAxis))] * stride;
      stride *= AlternativePointsPerAxis;
    }
    validateNumericalEquals(density[static_cast<std::size_t>(point)], alternativePoint * 100.0);
  }
}

/**
 * Checks that a cell patch plots one element per volume, spanning the volume, and writes the
 * values of that volume.
 */
void exahype2::plotter::tests::PatchPlotterTest::testCellPatchPlotsOneCellPerVolume() {
  const int totalVolumes = peano4::power(VolumesPerAxis, DIMENSIONS);
  const int totalPoints  = peano4::power(VolumesPerAxis + 1, DIMENSIONS);

  Snapshot snapshot;
  openRecordingSnapshot(snapshot);

  CellPatchPlotter plotter;
  plotter.open(snapshot, 4, {{"rho", 1}, {"e", 1}}, []() {});

  const std::vector<double> patch = createPatch(totalVolumes);
  plotter.plotCell<VolumesPerAxis, NumberOfUnknowns>(
    createUnitCell(),
    unitDomainOffset(),
    unitDomainSize(),
    patch.data(),
    []([[maybe_unused]] const int cellIndex) {}
  );

  const auto& writer = getRecordingFileWriter(snapshot);
  validateEquals(writer.getNumberOfCells(0), totalVolumes);
  validateEquals(writer.getNumberOfPoints(0), totalPoints);

  const std::vector<std::int32_t> connectivity = writer.getConnectivity(0);
  for (int corner = 0; corner < peano4::VerticesPerCell; corner++) {
    validateEquals(connectivity[static_cast<std::size_t>(corner)], corner);
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      validateNumericalEquals(
        writer.getPointCoordinate(0, corner, axis),
        0.5 * exahype2::plotter::CellCorners[corner][axis]
      );
    }
  }

  const std::vector<double> density = writer.getCellField(0, "rho");
  const std::vector<double> energy  = writer.getCellField(0, "e");
  validateEquals(static_cast<int>(density.size()), totalVolumes);
  for (int volume = 0; volume < totalVolumes; volume++) {
    validateNumericalEquals(density[static_cast<std::size_t>(volume)], volume * 100.0);
    validateNumericalEquals(energy[static_cast<std::size_t>(volume)], volume * 100.0 + 1.0);
  }
  validateNumericalEquals(writer.getCellField(0, "Subtree")[0], 4.0);
}

/**
 * Checks that the cell plots of the caller run once per plotted element and reach the field the
 * caller created after the plotter opened.
 */
void exahype2::plotter::tests::PatchPlotterTest::testCellPatchRunsTheCellDataPlotsOfTheCaller() {
  const int totalVolumes = peano4::power(VolumesPerAxis, DIMENSIONS);

  Snapshot snapshot;
  openRecordingSnapshot(snapshot);

  CellPatchPlotter                                             plotter;
  std::unique_ptr<tarch::plotter::FileWriter::CellDataPlotter> statusPlotter;
  plotter.open(snapshot, 0, {{"rho", 1}, {"e", 1}}, [&]() {
    statusPlotter = plotter.createCellDataPlotter("Status", 1);
  });

  int                       plots = 0;
  const std::vector<double> patch = createPatch(totalVolumes);
  plotter.plotCell<VolumesPerAxis, NumberOfUnknowns>(
    createUnitCell(),
    unitDomainOffset(),
    unitDomainSize(),
    patch.data(),
    [&](const int cellIndex) {
      statusPlotter->plotCellData(cellIndex, 7.0);
      plots++;
    }
  );

  validateEquals(plots, totalVolumes);

  const auto& writer = getRecordingFileWriter(snapshot);
  const std::vector<std::string>
    expectedCellFields{"Subdomain", "Subtree", "RefinementLevel", "Status", "CurvePosition", "rho", "e"};
  validateTrue(writer.getCellFieldNames(0) == expectedCellFields);
  const std::vector<double> status = writer.getCellField(0, "Status");
  validateEquals(static_cast<int>(status.size()), totalVolumes);
  for (const double value : status) {
    validateNumericalEquals(value, 7.0);
  }
}

/**
 * Checks that the cell plots of the caller of a nodal patch run once per plotted element and
 * that the field the caller creates precedes the curve position.
 */
void exahype2::plotter::tests::PatchPlotterTest::testNodalPatchRunsTheCellDataPlotsOfTheCaller() {
  const int totalPoints = peano4::power(PointsPerAxis, DIMENSIONS);
  const int totalCells  = peano4::power(PointsPerAxis - 1, DIMENSIONS);

  Snapshot snapshot;
  openRecordingSnapshot(snapshot);

  NodalPatchPlotter                                            plotter;
  std::unique_ptr<tarch::plotter::FileWriter::CellDataPlotter> statusPlotter;
  plotter.open(snapshot, 0, {{"rho", 1}, {"e", 1}}, [&]() {
    statusPlotter = plotter.createCellDataPlotter("Status", 1);
  });

  int                       plots = 0;
  const std::vector<double> patch = createPatch(totalPoints);
  plotter.plotCell<PointsPerAxis, NumberOfUnknowns, 1>(
    createUnitCell(),
    unitDomainOffset(),
    unitDomainSize(),
    Mapping,
    patch.data(),
    false,
    nullptr,
    [&](const int cellIndex) {
      statusPlotter->plotCellData(cellIndex, 7.0);
      plots++;
    }
  );

  validateEquals(plots, totalCells);

  const auto& writer = getRecordingFileWriter(snapshot);
  const std::vector<std::string>
    expectedCellFields{"Subdomain", "Subtree", "RefinementLevel", "Status", "CurvePosition"};
  validateTrue(writer.getCellFieldNames(0) == expectedCellFields);
  const std::vector<double> status = writer.getCellField(0, "Status");
  validateEquals(static_cast<int>(status.size()), totalCells);
  for (const double value : status) {
    validateNumericalEquals(value, 7.0);
  }
}
