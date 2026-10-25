// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TrajectoryRecordingTest.h"

#include "exahype2/particles/ParticleSet.h"
#include "exahype2/particles/TrajectoryDatabase.h"
#include "exahype2/particles/TrajectoryRecording.h"
#include "peano4/Spacetree.h"
#include "peano4/tests/TestGrids.h"
#include "tarch/mpi/Rank.h"

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

exahype2::particles::tests::TrajectoryRecordingTest::TrajectoryRecordingTest() :
  tarch::tests::TestCase("exahype2::particles::tests::TrajectoryRecordingTest") {}

void
exahype2::particles::tests::TrajectoryRecordingTest::run() {
  testMethod(testEveryParticleOfTheRankBecomesOneRow);
  testMethod(testASnapshotDueOnTheRankOfTheParticleIsDueOnEveryRank);
}

void
exahype2::particles::tests::TrajectoryRecordingTest::testASnapshotDueOnTheRankOfTheParticleIsDueOnEveryRank() {
  const int rank          = tarch::mpi::Rank::getInstance().getRank();
  const int numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  peano4::tests::buildRegularGrid(Depth);
  const auto& cellConnectivity = peano4::Spacetree::getInstance().getCellConnectivity();

  exahype2::particles::ParticleSet<NumberOfData> particles;
  std::vector<double>                            candidate(DIMENSIONS, 0.2);
  candidate.push_back(1.0);
  particles.insert(cellConnectivity, candidate);
  validateEquals(particles.getGlobalNumberOfParticles(), 1);

  const std::string fileName = (tarch::tests::getWritableTestDirectory() / "TrajectoryRecordingDueTest").string();
  const std::string writtenFile =
    numberOfRanks > 1 ? fileName + "-rank-" + std::to_string(rank) + ".csv" : fileName + ".csv";
  exahype2::particles::TrajectoryDatabase database(1.0e-8, 0.1);
  database.setOutputFileName(fileName);

  const bool ownsTheParticle = particles.getSize() > 0;
  validateEquals(exahype2::particles::isSnapshotDueOnSomeRank(particles, 5.0, 0.0, 1.0, database), false);
  validateEquals(exahype2::particles::isSnapshotDue(particles, 0.5, 0.0, 1.0, database), ownsTheParticle);
  validateEquals(exahype2::particles::isSnapshotDueOnSomeRank(particles, 0.5, 0.0, 1.0, database), true);
  exahype2::particles::recordSnapshots(particles, 0.5, 0.0, 1.0, database);
  validateEquals(exahype2::particles::isSnapshotDueOnSomeRank(particles, 0.55, 0.0, 1.0, database), false);
  validateEquals(exahype2::particles::isSnapshotDueOnSomeRank(particles, 0.65, 0.0, 1.0, database), true);
  std::filesystem::remove(writtenFile);
}

void
exahype2::particles::tests::TrajectoryRecordingTest::testEveryParticleOfTheRankBecomesOneRow() {
  const int rank          = tarch::mpi::Rank::getInstance().getRank();
  const int numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  peano4::tests::buildRegularGrid(Depth);
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

  const std::string fileName = (tarch::tests::getWritableTestDirectory() / "TrajectoryRecordingTest").string();
  const std::string writtenFile =
    numberOfRanks > 1 ? fileName + "-rank-" + std::to_string(rank) + ".csv" : fileName + ".csv";
  std::filesystem::remove(writtenFile);
  exahype2::particles::TrajectoryDatabase database(0.0);
  database.setOutputFileName(fileName);

  exahype2::particles::recordSnapshots(particles, 5.0, 0.0, 1.0, database);
  validate(not std::filesystem::exists(writtenFile));

  exahype2::particles::recordSnapshots(particles, 0.5, 0.0, 1.0, database);
  exahype2::particles::recordSnapshots(particles, 0.75, 0.0, 1.0, database);
  if (particles.getSize() > 0) {
    validateWithParams1(std::filesystem::exists(writtenFile), writtenFile);
    validateEqualsWithParams1(getNumberOfRowsOf(writtenFile), 2 * particles.getSize(), writtenFile);
  }
  std::filesystem::remove(writtenFile);
}
