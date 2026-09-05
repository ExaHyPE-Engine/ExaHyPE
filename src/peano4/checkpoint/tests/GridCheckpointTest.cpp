// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GridCheckpointTest.h"

#ifdef PEANO_WITH_HDF5

#include "peano4/checkpoint/GridCheckpoint.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/HDF5File.h"

namespace {
  constexpr int ExtraTableColumns = 3;

  std::string testDirectory() {
    return (tarch::tests::getWritableTestDirectory() / "GridCheckpointTest").string();
  }

  void removeFixture(const std::string& directory) {
    tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());
    if (tarch::mpi::Rank::getInstance().isGlobalMaster() and std::filesystem::exists(directory)) {
      std::filesystem::remove_all(directory);
    }
    tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());
  }
} // namespace

#endif

peano4::checkpoint::tests::GridCheckpointTest::GridCheckpointTest() :
  TestCase("peano4::checkpoint::tests::GridCheckpointTest") {}

void peano4::checkpoint::tests::GridCheckpointTest::testCreateDirectoryFor() {
#ifdef PEANO_WITH_HDF5
  const std::string directory = testDirectory();
  removeFixture(directory);

  peano4::checkpoint::createDirectoryFor(directory + "/nested/grid.h5");
  validate(std::filesystem::is_directory(directory + "/nested"));

  removeFixture(directory);
#endif
}

void peano4::checkpoint::tests::GridCheckpointTest::testReadExtraTable() {
#ifdef PEANO_WITH_HDF5
  const std::string directory = testDirectory();
  const std::string path      = directory + "/checkpoint-0.h5";
  removeFixture(directory);
  peano4::checkpoint::createDirectoryFor(path);

  const std::int32_t rows[2 * ExtraTableColumns] = {1, 2, 3, 4, 5, 6};
  {
    const auto file = tarch::plotter::HDF5File::openForDataHoldingRanks(
      path, tarch::mpi::Rank::getInstance().isGlobalMaster()
    );
    if (file != nullptr) {
      file->writeTable(std::string(peano4::checkpoint::GridGroup) + "/extra", 2, ExtraTableColumns, rows);
    }
  }
  tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());

  int        columns = 0;
  const auto table   = peano4::checkpoint::readExtraTable(path, columns);
  validateEquals(columns, ExtraTableColumns);
  validateEquals(table.size(), static_cast<std::size_t>(2) * ExtraTableColumns);
  for (std::size_t entry = 0; entry < table.size(); entry++) {
    validateEqualsWithParams1(table[entry], rows[entry], entry);
  }

  removeFixture(directory);
#endif
}

void peano4::checkpoint::tests::GridCheckpointTest::run() {
  testMethod(testCreateDirectoryFor);
  testMethod(testReadExtraTable);
}
