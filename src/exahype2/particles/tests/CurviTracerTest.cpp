// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CurviTracerTest.h"

#include "exahype2/particles/CurviTracer.h"
#include "peano4/dataview/CellMarker.h"
#include "peano4/dataview/FaceMarker.h"
#include "peano4/particles/TrajectoryDatabase.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"

namespace {
  constexpr int    Order                       = 1;
  constexpr int    NodesPerAxis                = Order + 1;
  constexpr double CellWidth                   = 0.2;
  constexpr double CellCentre                  = 0.5;
  constexpr double TimeStamp                   = 2.5;
  constexpr double ScaleOfTheInverseProjection = 2.0;

  constexpr double QuadraturePoints1D[] = {0.21132486540518713, 0.7886751345948129};

  constexpr int nodesPerCell() {
    int product = 1;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      product *= NodesPerAxis;
    }
    return product;
  }

  constexpr int nodesPerFace() { return nodesPerCell() / NodesPerAxis; }

  /** Stands in for the generated solver and maps a physical position onto the mesh. */
  struct CurvilinearSolver final {
    [[nodiscard]] tarch::la::Vector<DIMENSIONS, double> invertProjection(
      const tarch::la::Vector<DIMENSIONS, double> position
    ) const {
      tarch::la::Vector<DIMENSIONS, double> result;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        result(axis) = ScaleOfTheInverseProjection * position(axis);
      }
      return result;
    }
  };

  peano4::dataview::CellMarker createCellMarker() {
    peano4::dataview::CellMarker marker{};
    marker.h      = tarch::la::Vector<DIMENSIONS, double>(CellWidth);
    marker.centre = tarch::la::Vector<DIMENSIONS, double>(CellCentre);
    return marker;
  }

  peano4::dataview::FaceMarker createFaceMarker(const int faceNumber) {
    peano4::dataview::FaceMarker marker{};
    marker.h           = tarch::la::Vector<DIMENSIONS, double>(CellWidth);
    marker.cellCentre  = tarch::la::Vector<DIMENSIONS, double>(CellCentre);
    marker.cellIsLocal = true;
    marker.select(faceNumber);
    return marker;
  }

  std::string testFileBaseName(const std::string& name) {
    return (tarch::tests::getWritableTestDirectory() / name).string();
  }

  /** Returns the file name TrajectoryDatabase writes for the calling rank. */
  std::string outputFileName(const std::string& baseName) {
    if (tarch::mpi::Rank::getInstance().getNumberOfRanks() > 1) {
      return baseName + "-rank-" + std::to_string(tarch::mpi::Rank::getInstance().getRank()) + ".csv";
    }
    return baseName + ".csv";
  }

  /** Returns the rows behind the header, each split into its values. */
  std::vector<std::vector<double>> readDataRows(const std::string& fileName) {
    std::vector<std::vector<double>> rows;
    std::ifstream                    file(fileName);
    std::string                      line;
    bool                             isHeader = true;
    while (std::getline(file, line)) {
      if (line.empty()) {
        continue;
      }
      if (isHeader) {
        isHeader = false;
        continue;
      }
      std::vector<double> values;
      std::istringstream  stream(line);
      std::string         cell;
      while (std::getline(stream, cell, ',')) {
        values.push_back(std::stod(cell));
      }
      rows.push_back(values);
    }
    return rows;
  }
} // namespace

exahype2::particles::tests::CurviTracerTest::CurviTracerTest() :
  tarch::tests::TestCase("exahype2::particles::tests::CurviTracerTest") {}

void exahype2::particles::tests::CurviTracerTest::run() {
  testMethod(testInvertCoordinatesAppendsEveryMappedPosition);
  testMethod(testIsFaultFaceSelectsTheFaceOnTheFault);
  testMethod(testRecordsTheSolutionAtEveryCoordinateOfTheCell);
  testMethod(testRecordsTheFaceAverageAndTheJumpOfTheVectorComponents);
}

/** Every raw coordinate reaches the vector as the position the inverse projection returns. */
void exahype2::particles::tests::CurviTracerTest::testInvertCoordinatesAppendsEveryMappedPosition() {
  static constexpr double RawCoordinates[][3] = {{0.1, 0.2, 0.3}, {0.4, 0.5, 0.6}};

  CurvilinearSolver                                  solver;
  std::vector<tarch::la::Vector<DIMENSIONS, double>> coordinates;
  exahype2::particles::invertCoordinates(solver, RawCoordinates, coordinates);

  validateEquals(static_cast<int>(coordinates.size()), 2);
  for (int entry = 0; entry < 2; entry++) {
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      validateNumericalEqualsWithParams2(
        coordinates[entry](axis),
        ScaleOfTheInverseProjection * RawCoordinates[entry][axis],
        entry,
        axis
      );
    }
  }
}

/**
 * A face carries the fault if its normal is the fault normal and its centre lies less than half
 * a mesh width away from the fault position.
 */
void exahype2::particles::tests::CurviTracerTest::testIsFaultFaceSelectsTheFaceOnTheFault() {
  const double lowerFace = CellCentre - CellWidth / 2.0;

  validateTrue(exahype2::particles::isFaultFace(createFaceMarker(0), 0, lowerFace));
  validateFalse(exahype2::particles::isFaultFace(createFaceMarker(0), 0, lowerFace + CellWidth));
  validateFalse(exahype2::particles::isFaultFace(createFaceMarker(1), 0, lowerFace));
  validateTrue(exahype2::particles::isFaultFace(createFaceMarker(DIMENSIONS), 0, lowerFace + CellWidth));
}

/**
 * A coordinate inside the cell reaches the database under its own index, with the first
 * UnknownsToStore values of the polynomial, and a coordinate outside the cell reaches nothing.
 */
void exahype2::particles::tests::CurviTracerTest::testRecordsTheSolutionAtEveryCoordinateOfTheCell() {
  constexpr int UnknownsPerDoF  = 3;
  constexpr int UnknownsToStore = 2;
  constexpr int ValuesOfTheCell = nodesPerCell() * UnknownsPerDoF;

  double solution[ValuesOfTheCell];
  for (int node = 0; node < nodesPerCell(); node++) {
    for (int unknown = 0; unknown < UnknownsPerDoF; unknown++) {
      solution[node * UnknownsPerDoF + unknown] = 10.0 * (unknown + 1);
    }
  }

  std::vector<tarch::la::Vector<DIMENSIONS, double>> coordinates;
  coordinates.emplace_back(0.9);
  coordinates.emplace_back(CellCentre);

  const std::string baseName = testFileBaseName("CurviTracerTest_cell");
  const std::string fileName = outputFileName(baseName);
  std::filesystem::remove(fileName);

  {
    peano4::particles::TrajectoryDatabase database;
    database.setOutputFileName(baseName);

    exahype2::particles::recordCurvilinearCellSolution<Order, UnknownsPerDoF, UnknownsToStore>(
      createCellMarker(),
      QuadraturePoints1D,
      solution,
      coordinates,
      tarch::la::Vector<DIMENSIONS, double>(0.0),
      TimeStamp,
      database
    );

    const std::vector<std::vector<double>> rows = readDataRows(fileName);
    validateEqualsWithParams1(static_cast<int>(rows.size()), 1, fileName);
    validateEquals(static_cast<int>(rows[0].size()), 3 + DIMENSIONS + UnknownsToStore);
    validateNumericalEquals(rows[0][0], 1.0);
    validateNumericalEquals(rows[0][1], 1.0);
    validateNumericalEquals(rows[0][2], TimeStamp);
    validateNumericalEquals(rows[0][3], CellCentre);
    validateNumericalEquals(rows[0][3 + DIMENSIONS], 10.0);
    validateNumericalEquals(rows[0][4 + DIMENSIONS], 20.0);
  }
  std::filesystem::remove(fileName);
}

/**
 * A fault coordinate on the face reaches the database with the average of the two estimates,
 * except for the three velocity and the three displacement components, which carry the
 * difference of the right estimate and the left one.
 */
void exahype2::particles::tests::CurviTracerTest::testRecordsTheFaceAverageAndTheJumpOfTheVectorComponents() {
  constexpr int UnknownsPerDoF       = 12;
  constexpr int UnknownsToStore      = 12;
  constexpr int FirstVelocity        = 0;
  constexpr int FirstDisplacement    = 9;
  constexpr int ComponentsPerVector  = 3;
  constexpr int BasisElementsPerFace = nodesPerFace() * UnknownsPerDoF;
  constexpr int FirstSnapshotId      = 4;

  double faceEstimates[2 * BasisElementsPerFace];
  for (int node = 0; node < nodesPerFace(); node++) {
    for (int unknown = 0; unknown < UnknownsPerDoF; unknown++) {
      const double leftValue                                                = unknown + 1;
      faceEstimates[node * UnknownsPerDoF + unknown]                        = leftValue;
      faceEstimates[BasisElementsPerFace + node * UnknownsPerDoF + unknown] = 2.0 * leftValue;
    }
  }

  std::vector<tarch::la::Vector<DIMENSIONS, double>> faultCoordinates;
  faultCoordinates.push_back(createFaceMarker(0).getX());

  const std::string baseName = testFileBaseName("CurviTracerTest_fault");
  const std::string fileName = outputFileName(baseName);
  std::filesystem::remove(fileName);

  {
    peano4::particles::TrajectoryDatabase database;
    database.setOutputFileName(baseName);

    exahype2::particles::
      recordCurvilinearFaultSolution<Order, UnknownsPerDoF, UnknownsToStore, FirstDisplacement, FirstVelocity>(
        createFaceMarker(0),
        QuadraturePoints1D,
        faceEstimates,
        BasisElementsPerFace,
        faultCoordinates,
        FirstSnapshotId,
        TimeStamp,
        database
      );

    const std::vector<std::vector<double>> rows = readDataRows(fileName);
    validateEqualsWithParams1(static_cast<int>(rows.size()), 1, fileName);
    validateEquals(static_cast<int>(rows[0].size()), 3 + DIMENSIONS + UnknownsToStore);
    validateNumericalEquals(rows[0][0], static_cast<double>(FirstSnapshotId));
    validateNumericalEquals(rows[0][2], TimeStamp);
    for (int unknown = 0; unknown < UnknownsToStore; unknown++) {
      const bool holdsJump = (unknown >= FirstVelocity and unknown < FirstVelocity + ComponentsPerVector)
        or (unknown >= FirstDisplacement and unknown < FirstDisplacement + ComponentsPerVector);
      const double leftValue = unknown + 1;
      const double expected  = holdsJump ? leftValue : 1.5 * leftValue;
      validateNumericalEqualsWithParams1(rows[0][3 + DIMENSIONS + unknown], expected, unknown);
    }
  }
  std::filesystem::remove(fileName);
}
