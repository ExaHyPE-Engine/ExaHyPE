// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "exahype2/particles/TrajectoryRecording.h"

#include "exahype2/particles/ParticleSet.h"
#include "exahype2/particles/TrajectoryDatabase.h"
#include "peano4/Spacetree.h"
#include "tarch/mpi/Rank.h"
#include "tarch/testing/UnitTest.h"
#include "unittests/peano4/TestGrids.h"

namespace {
  constexpr int Depth        = 1;
  constexpr int NumberOfData = 1;

  int getNumberOfRowsOf(const std::filesystem::path& path) {
    std::ifstream stream(path);
    std::string   line;
    int           rows = 0;
    while (std::getline(stream, line)) {
      if (line.find("number(0)") == std::string::npos and not line.empty()) {
        rows++;
      }
    }
    return rows;
  }
} // namespace

UNIT_TEST("A recording writes one row per particle of the rank and per time stamp inside the record window") {
  GIVEN("Three particles in a regular grid over every rank and a database that stores every snapshot") {
    const int rank          = tarch::mpi::Rank::getInstance().getRank();
    const int numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
    peano4::unittests::buildRegularGrid(Depth);
    const auto& cellConnectivity = peano4::Spacetree::getInstance().getCellConnectivity();

    exahype2::particles::ParticleSet<NumberOfData> particles;
    std::vector<double>                            candidates;
    for (const double coordinate : {0.2, 0.5, 0.8}) {
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        candidates.push_back(coordinate);
      }
      candidates.push_back(10.0 * coordinate);
    }
    particles.insert(cellConnectivity, candidates);

    const std::string fileName = (tarch::testing::getWritableTestDirectory() / "TrajectoryRecordingTest").string();
    const std::string writtenFile =
      numberOfRanks > 1 ? fileName + "-rank-" + std::to_string(rank) + ".csv" : fileName + ".csv";
    std::filesystem::remove(writtenFile);
    exahype2::particles::TrajectoryDatabase database(0.0);
    database.setOutputFileName(fileName);

    WHEN("The recording takes the times five, one half and three quarters with the record window from zero to one") {
      exahype2::particles::recordSnapshots(particles, 5.0, 0.0, 1.0, database);
      const bool fileExistsAfterTheTimeOutsideTheWindow = std::filesystem::exists(writtenFile);
      exahype2::particles::recordSnapshots(particles, 0.5, 0.0, 1.0, database);
      exahype2::particles::recordSnapshots(particles, 0.75, 0.0, 1.0, database);
      const bool fileExists   = std::filesystem::exists(writtenFile);
      const int  numberOfRows = getNumberOfRowsOf(writtenFile);

      THEN("The recording skips the time five and a rank with particles writes two rows per particle") {
        VALIDATE(not fileExistsAfterTheTimeOutsideTheWindow);
        if (particles.getSize() > 0) {
          VALIDATE(fileExists, writtenFile);
          VALIDATE_EQUALS(numberOfRows, 2 * particles.getSize(), writtenFile);
        }
      }
    }
    std::filesystem::remove(writtenFile);
  }
}

UNIT_TEST("A snapshot due on the rank that owns the particle is due on every rank") {
  GIVEN("One particle in a regular grid over every rank and a database with the time delta 0.1") {
    const int rank          = tarch::mpi::Rank::getInstance().getRank();
    const int numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
    peano4::unittests::buildRegularGrid(Depth);
    const auto& cellConnectivity = peano4::Spacetree::getInstance().getCellConnectivity();

    exahype2::particles::ParticleSet<NumberOfData> particles;
    std::vector<double>                            candidate(DIMENSIONS, 0.2);
    candidate.push_back(1.0);
    particles.insert(cellConnectivity, candidate);
    const bool ownsTheParticle = particles.getSize() > 0;

    const std::string fileName = (tarch::testing::getWritableTestDirectory() / "TrajectoryRecordingDueTest").string();
    const std::string writtenFile =
      numberOfRanks > 1 ? fileName + "-rank-" + std::to_string(rank) + ".csv" : fileName + ".csv";
    exahype2::particles::TrajectoryDatabase database(1.0e-8, 0.1);
    database.setOutputFileName(fileName);

    WHEN("Every rank asks at the times 5, 0.5, 0.55 and 0.65 in the window from zero to one, with a record at 0.5") {
      const bool isDueOutsideTheWindow =
        exahype2::particles::isSnapshotDueOnSomeRank(particles, 5.0, 0.0, 1.0, database);
      const bool isDueOnThisRank = exahype2::particles::isSnapshotDue(particles, 0.5, 0.0, 1.0, database);
      const bool isDueInsideTheWindow =
        exahype2::particles::isSnapshotDueOnSomeRank(particles, 0.5, 0.0, 1.0, database);
      exahype2::particles::recordSnapshots(particles, 0.5, 0.0, 1.0, database);
      const bool isDueWithinTheTimeDelta =
        exahype2::particles::isSnapshotDueOnSomeRank(particles, 0.55, 0.0, 1.0, database);
      const bool isDueAfterTheTimeDelta =
        exahype2::particles::isSnapshotDueOnSomeRank(particles, 0.65, 0.0, 1.0, database);

      THEN("The ranks own one particle and a snapshot is due at 0.5 and 0.65 only and locally on its rank") {
        VALIDATE_EQUALS(particles.getGlobalNumberOfParticles(), 1);
        VALIDATE_EQUALS(isDueOutsideTheWindow, false);
        VALIDATE_EQUALS(isDueInsideTheWindow, true);
        VALIDATE_EQUALS(isDueWithinTheTimeDelta, false);
        VALIDATE_EQUALS(isDueAfterTheTimeDelta, true);
        VALIDATE_EQUALS(isDueOnThisRank, ownsTheParticle);
      }
    }
    std::filesystem::remove(writtenFile);
  }
}
