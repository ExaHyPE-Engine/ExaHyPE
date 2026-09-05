// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CellDataLoaderTest.h"

#ifdef PEANO_WITH_HDF5

#include "peano4/checkpoint/CellDataLoader.h"
#include "peano4/checkpoint/Lattice.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/HDF5File.h"

namespace {
  constexpr int     RecordLevel     = 2;
  constexpr int     ValuesPerRecord = 3;
  constexpr double  CheckpointTime  = 0.25;
  const std::string SolverGroup     = "/solvers/Test";

  std::string testFilePath() { return (tarch::tests::getWritableTestDirectory() / "CellDataLoaderTest.h5").string(); }

  /**
   * Writes what a solver checkpoint holds for one level: The per-block record
   * counts, one record's key and values, the solver scalars and the time
   * stamp. Collective over every rank.
   */
  void writeFixture(const std::string& path) {
    const auto file = tarch::plotter::HDF5File::openForDataHoldingRanks(
      path, tarch::mpi::Rank::getInstance().isGlobalMaster()
    );
    if (file == nullptr) {
      return;
    }

    const std::string         prefix = SolverGroup + "/cells/level" + std::to_string(RecordLevel);
    std::vector<std::int64_t> counts(
      static_cast<std::size_t>(peano4::checkpoint::numberOfBlocksOnLevel(RecordLevel)), 0
    );
    counts.front() = 1;
    file->writeTable(prefix + "/count", counts.size(), 0, counts.data());

    const std::int64_t key = 0;
    file->writeTable(prefix + "/key", 1, 0, &key);
    const double values[ValuesPerRecord] = {1.5, -2.5, 3.5};
    file->writeTable(prefix + "/values", 1, ValuesPerRecord, values);

    const double scalars[2] = {0.125, 4.0};
    file->writeTable(SolverGroup + "/scalars", 2, 0, scalars);
    const double checkpointTime = CheckpointTime;
    file->writeTable(SolverGroup + "/time", 1, 0, &checkpointTime);
  }

  void removeFixture(const std::string& path) {
    tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());
    if (tarch::mpi::Rank::getInstance().isGlobalMaster() and std::filesystem::exists(path)) {
      std::filesystem::remove(path);
    }
    tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());
  }
} // namespace

#endif

peano4::checkpoint::tests::CellDataLoaderTest::CellDataLoaderTest() :
  TestCase("peano4::checkpoint::tests::CellDataLoaderTest") {}

void peano4::checkpoint::tests::CellDataLoaderTest::testTimeStampAndScalars() {
#ifdef PEANO_WITH_HDF5
  const std::string path = testFilePath();
  writeFixture(path);
  tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());

  {
    const peano4::checkpoint::CellDataLoader loader(path, SolverGroup, ValuesPerRecord);
    validateNumericalEquals(loader.getTimeStamp(), CheckpointTime);
    validateEquals(loader.getSolverScalars().size(), 2);
    validateNumericalEquals(loader.getSolverScalars()[0], 0.125);
    validateNumericalEquals(loader.getSolverScalars()[1], 4.0);
  }

  removeFixture(path);
#endif
}

void peano4::checkpoint::tests::CellDataLoaderTest::testRecordWithoutResidentBlocks() {
#ifdef PEANO_WITH_HDF5
  const std::string path = testFilePath();
  writeFixture(path);
  tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());

  {
    const peano4::checkpoint::CellDataLoader loader(path, SolverGroup, ValuesPerRecord);
    const tarch::la::Vector<DIMENSIONS, int> latticeIndex(0);
    validate(loader.record(RecordLevel, latticeIndex) == nullptr);
    validate(loader.record(0, latticeIndex) == nullptr);
  }

  removeFixture(path);
#endif
}

void peano4::checkpoint::tests::CellDataLoaderTest::run() {
  testMethod(testTimeStampAndScalars);
  testMethod(testRecordWithoutResidentBlocks);
}
