// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "HDF5FileReaderTest.h"

#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/HDF5File.h"
#include "tarch/reader/HDF5FileReader.h"

#ifdef PEANO_WITH_HDF5

namespace {
  std::string testFilePath() { return (tarch::tests::getWritableTestDirectory() / "HDF5FileReaderTest.h5").string(); }

  /** The master writes the fixture; the others reuse it after the barrier. */
  void writeFixture(const std::string& path) {
    const auto file =
      tarch::plotter::HDF5File::openForDataHoldingRanks(path, tarch::mpi::Rank::getInstance().isGlobalMaster());
    if (file == nullptr) {
      return;
    }
    const double       realTable[6] = {0.5, 1.5, 2.5, 3.5, 4.5, 5.5};
    const std::int32_t intTable[4]  = {3, 1, 4, 1};
    const std::int64_t keyTable[3]  = {1LL << 40, 2, 3};
    file->writeTable("/cells/values", 3, 2, realTable);
    file->writeTable("/marks", 4, 0, intTable);
    file->writeTable("/keys", 3, 0, keyTable);
  }
} // namespace

tarch::reader::tests::HDF5FileReaderTest::HDF5FileReaderTest() :
  TestCase("tarch::reader::tests::HDF5FileReaderTest") {}

void tarch::reader::tests::HDF5FileReaderTest::testReadRowRanges() {
  const std::string path = testFilePath();
  writeFixture(path);
  tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());

  {
    const tarch::reader::HDF5FileReader reader(path);

    const auto valueDimensions = reader.getDatasetDimensions("/cells/values");
    validateEquals(valueDimensions.first, 3U);
    validateEquals(valueDimensions.second, 2U);

    const auto allValues = reader.readDoubleRows("/cells/values", 0, 3);
    validateEquals(allValues.size(), 6U);
    validateNumericalEquals(allValues[0], 0.5);
    validateNumericalEquals(allValues[5], 5.5);

    const auto middleRow = reader.readDoubleRows("/cells/values", 1, 1);
    validateEquals(middleRow.size(), 2U);
    validateNumericalEquals(middleRow[0], 2.5);
    validateNumericalEquals(middleRow[1], 3.5);

    const auto marks = reader.readInt32Rows("/marks", 1, 2);
    validateEquals(marks.size(), 2U);
    validateEquals(marks[0], 1);
    validateEquals(marks[1], 4);

    const auto keys = reader.readInt64Rows("/keys", 0, 1);
    validateEquals(keys.size(), 1U);
    validateEquals(keys[0], 1LL << 40);

    validate(reader.readDoubleRows("/cells/values", 0, 0).empty());
  }

  tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    std::filesystem::remove(path);
  }
  tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());
}

void tarch::reader::tests::HDF5FileReaderTest::testDatasetExists() {
  const std::string path = testFilePath();
  writeFixture(path);
  tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());

  {
    const tarch::reader::HDF5FileReader reader(path);
    validate(reader.datasetExists("/cells/values"));
    validate(reader.datasetExists("/marks"));
    validate(not reader.datasetExists("/absent"));
    validate(not reader.datasetExists("/cells/absent"));
    validate(not reader.datasetExists("/absent/group/dataset"));
    validateEquals(reader.getFilename(), path);
  }

  tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    std::filesystem::remove(path);
  }
  tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());
}

void tarch::reader::tests::HDF5FileReaderTest::run() {
  testMethod(testReadRowRanges);
  testMethod(testDatasetExists);
}

#else

tarch::reader::tests::HDF5FileReaderTest::HDF5FileReaderTest() :
  TestCase("tarch::reader::tests::HDF5FileReaderTest") {}

void tarch::reader::tests::HDF5FileReaderTest::run() {}

#endif
