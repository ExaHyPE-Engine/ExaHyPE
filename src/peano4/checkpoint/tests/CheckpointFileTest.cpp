// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CheckpointFileTest.h"

#ifdef PEANO_WITH_HDF5

#include "peano4/checkpoint/CheckpointFile.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"

namespace {
  std::string testDirectory() { return (tarch::tests::getWritableTestDirectory() / "CheckpointFileTest").string(); }

  void removeFixture(const std::string& directory) {
    tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());
    if (tarch::mpi::Rank::getInstance().isGlobalMaster() and std::filesystem::exists(directory)) {
      std::filesystem::remove_all(directory);
    }
    tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());
  }
} // namespace

#endif

peano4::checkpoint::tests::CheckpointFileTest::CheckpointFileTest() :
  TestCase("peano4::checkpoint::tests::CheckpointFileTest") {}

void peano4::checkpoint::tests::CheckpointFileTest::testPath() {
#ifdef PEANO_WITH_HDF5
  validateEquals(peano4::checkpoint::CheckpointFile::getPath("checkpoints", 0), "checkpoints/checkpoint-0.h5");
  validateEquals(peano4::checkpoint::CheckpointFile::getPath("a/b", 12), "a/b/checkpoint-12.h5");
#endif
}

void peano4::checkpoint::tests::CheckpointFileTest::testWritersHoldOneFile() {
#ifdef PEANO_WITH_HDF5
  const std::string directory = testDirectory();
  removeFixture(directory);

  auto&     checkpointFile = peano4::checkpoint::CheckpointFile::getInstance();
  const int firstNumber    = checkpointFile.getNumberOfCheckpoints();

  // Two writers of one checkpoint sweep: The file carries both and closes once
  // the second of them has left.
  checkpointFile.open(directory);
  checkpointFile.open(directory);
  const double firstTable[2] = {1.0, 2.0};
  checkpointFile.getFile().writeTable("/solvers/first/time", 2, 0, firstTable);
  checkpointFile.close();
  validateEquals(checkpointFile.getNumberOfCheckpoints(), firstNumber);
  const double secondTable[1] = {3.0};
  checkpointFile.getFile().writeTable("/solvers/second/time", 1, 0, secondTable);
  checkpointFile.close();
  validateEquals(checkpointFile.getNumberOfCheckpoints(), firstNumber + 1);

  tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());
  validate(std::filesystem::exists(peano4::checkpoint::CheckpointFile::getPath(directory, firstNumber)));
  validate(not std::filesystem::exists(peano4::checkpoint::CheckpointFile::getPath(directory, firstNumber + 1)));

  removeFixture(directory);
#endif
}

void peano4::checkpoint::tests::CheckpointFileTest::run() {
  testMethod(testPath);
  testMethod(testWritersHoldOneFile);
}
