// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "HDF5FileTest.h"

#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/HDF5File.h"

#ifdef PEANO_WITH_HDF5

#include <hdf5.h>

namespace {
  void barrier() { tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator()); }

  void cleanup(const std::string& path) {
    if (std::filesystem::exists(path)) {
      std::filesystem::remove(path);
    }
  }

  /** Reads a whole dataset of doubles through the serial HDF5 API. */
  std::vector<double> readDoubles(hid_t file, const char* name, const hsize_t expectedElements) {
    std::vector<double> result(expectedElements, 0.0);
    const hid_t         dataset = H5Dopen(file, name, H5P_DEFAULT);
    if (dataset < 0) {
      return {};
    }
    H5Dread(dataset, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, result.data());
    H5Dclose(dataset);
    return result;
  }

  std::vector<long long> readInt64s(hid_t file, const char* name, const hsize_t expectedElements) {
    std::vector<long long> result(expectedElements, 0);
    const hid_t            dataset = H5Dopen(file, name, H5P_DEFAULT);
    if (dataset < 0) {
      return {};
    }
    H5Dread(dataset, H5T_NATIVE_LLONG, H5S_ALL, H5S_ALL, H5P_DEFAULT, result.data());
    H5Dclose(dataset);
    return result;
  }

  std::vector<int> readInt32s(hid_t file, const char* name, const hsize_t expectedElements) {
    std::vector<int> result(expectedElements, 0);
    const hid_t      dataset = H5Dopen(file, name, H5P_DEFAULT);
    if (dataset < 0) {
      return {};
    }
    H5Dread(dataset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, result.data());
    H5Dclose(dataset);
    return result;
  }
} // namespace

tarch::plotter::tests::HDF5FileTest::HDF5FileTest() :
  TestCase("tarch::plotter::tests::HDF5FileTest") {}

void tarch::plotter::tests::HDF5FileTest::testDatasetsAndTables() {
  const std::string path = (tarch::tests::getWritableTestDirectory() / "HDF5FileTestDatasets.h5").string();
  const int         rank = tarch::mpi::Rank::getInstance().getRank();
  const int         ranks = tarch::mpi::Rank::getInstance().getNumberOfRanks();

  {
    const auto file = HDF5File::openForDataHoldingRanks(path, true);
    validate(file != nullptr);

    const unsigned long long localValueRows  = rank + 1;
    const unsigned long long localSparseRows = rank % 2 == 0 ? 1 : 0;
    const unsigned long long localRows[2]    = {localValueRows, localSparseRows};
    unsigned long long       firstRows[2]    = {0ULL, 0ULL};
    unsigned long long       totalRows[2]    = {0ULL, 0ULL};
    file->deriveRowRanges(localRows, firstRows, totalRows, 2);
    validateEquals(totalRows[0], static_cast<unsigned long long>(ranks) * (ranks + 1) / 2);
    validateEquals(firstRows[0], static_cast<unsigned long long>(rank) * (rank + 1) / 2);

    {
      HDF5File::Dataset values(*file, "/values", totalRows[0], 2, HDF5File::ElementType::Float64);
      std::vector<double> localValues;
      localValues.reserve(localValueRows * 2);
      for (unsigned long long row = 0; row < localValueRows; row++) {
        localValues.push_back(100.0 * rank + static_cast<double>(row));
        localValues.push_back(-1.0 * rank);
      }
      values.writeRows(firstRows[0], localValueRows, localValues.data(), HDF5File::ElementType::Float64);
    }
    {
      HDF5File::Dataset keys(*file, "/key", totalRows[0], 0, HDF5File::ElementType::Int64);
      std::vector<std::int64_t> localKeys;
      localKeys.reserve(localValueRows);
      for (unsigned long long row = 0; row < localValueRows; row++) {
        localKeys.push_back(1000LL * rank + static_cast<std::int64_t>(row));
      }
      keys.writeRows(firstRows[0], localValueRows, localKeys.data(), HDF5File::ElementType::Int64);
    }
    {
      // The odd ranks pass zero rows here and still join the collective write.
      HDF5File::Dataset sparse(*file, "/sparse", totalRows[1], 0, HDF5File::ElementType::Float64);
      const double      sparseValue = 0.5 + rank;
      sparse.writeRows(firstRows[1], localSparseRows, &sparseValue, HDF5File::ElementType::Float64);
    }

    const std::int32_t intTable[6] = {1, 2, 3, 4, 5, 6};
    file->writeTable("/table/int", 3, 2, intTable);
    const double realTable[2] = {2.5, -3.5};
    file->writeTable("/table/real", 2, 0, realTable);
  }

  barrier();
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    const unsigned long long totalValueRows = static_cast<unsigned long long>(ranks) * (ranks + 1) / 2;
    const hid_t              file           = H5Fopen(path.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    validate(file >= 0);

    const auto values = readDoubles(file, "/values", totalValueRows * 2);
    const auto keys   = readInt64s(file, "/key", totalValueRows);
    validateEquals(values.size(), totalValueRows * 2);
    unsigned long long row = 0;
    for (int writingRank = 0; writingRank < ranks; writingRank++) {
      for (int localRow = 0; localRow < writingRank + 1; localRow++) {
        validateNumericalEquals(values[row * 2], 100.0 * writingRank + localRow);
        validateNumericalEquals(values[row * 2 + 1], -1.0 * writingRank);
        validateEquals(keys[row], 1000LL * writingRank + localRow);
        row++;
      }
    }

    const auto sparse = readDoubles(file, "/sparse", (ranks + 1) / 2);
    for (int writingRank = 0; writingRank < ranks; writingRank += 2) {
      validateNumericalEquals(sparse[writingRank / 2], 0.5 + writingRank);
    }

    const auto intTable = readInt32s(file, "/table/int", 6);
    for (int i = 0; i < 6; i++) {
      validateEquals(intTable[i], i + 1);
    }
    const auto realTable = readDoubles(file, "/table/real", 2);
    validateNumericalEquals(realTable[0], 2.5);
    validateNumericalEquals(realTable[1], -3.5);

    H5Fclose(file);
    cleanup(path);
  }
  barrier();
}

void tarch::plotter::tests::HDF5FileTest::testWithoutDataHoldingRanks() {
  const std::string path = (tarch::tests::getWritableTestDirectory() / "HDF5FileTestEmpty.h5").string();

  {
    const auto file = HDF5File::openForDataHoldingRanks(path, false);
    if (file != nullptr) {
      validate(file->writesTables());
      const double tableData[3] = {1.0, 2.0, 3.0};
      file->writeTable("/table", 3, 0, tableData);
    }
  }

  barrier();
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    const hid_t file = H5Fopen(path.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    validate(file >= 0);
    const auto table = readDoubles(file, "/table", 3);
    validateNumericalEquals(table[0], 1.0);
    validateNumericalEquals(table[2], 3.0);
    H5Fclose(file);
    cleanup(path);
  }
  barrier();
}

void tarch::plotter::tests::HDF5FileTest::testSubsetOfDataHoldingRanks() {
  const std::string path     = (tarch::tests::getWritableTestDirectory() / "HDF5FileTestSubset.h5").string();
  const bool        isMaster = tarch::mpi::Rank::getInstance().isGlobalMaster();

  {
    const auto file           = HDF5File::openForDataHoldingRanks(path, isMaster);
    const bool receivedTheFile = file != nullptr;
    validateEquals(receivedTheFile, isMaster);
    if (file != nullptr) {
      const unsigned long long localRows[1] = {4ULL};
      unsigned long long       firstRows[1] = {0ULL};
      unsigned long long       totalRows[1] = {0ULL};
      file->deriveRowRanges(localRows, firstRows, totalRows, 1);
      validateEquals(totalRows[0], 4ULL);
      validateEquals(firstRows[0], 0ULL);

      HDF5File::Dataset values(*file, "/values", totalRows[0], 0, HDF5File::ElementType::Float64);
      const double      localValues[4] = {4.0, 3.0, 2.0, 1.0};
      values.writeRows(firstRows[0], localRows[0], localValues, HDF5File::ElementType::Float64);
    }
  }

  barrier();
  if (isMaster) {
    const hid_t file = H5Fopen(path.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    validate(file >= 0);
    const auto values = readDoubles(file, "/values", 4);
    validateNumericalEquals(values[0], 4.0);
    validateNumericalEquals(values[3], 1.0);
    H5Fclose(file);
    cleanup(path);
  }
  barrier();
}

void tarch::plotter::tests::HDF5FileTest::run() {
  testMethod(testDatasetsAndTables);
  testMethod(testWithoutDataHoldingRanks);
  testMethod(testSubsetOfDataHoldingRanks);
}

#else

tarch::plotter::tests::HDF5FileTest::HDF5FileTest() :
  TestCase("tarch::plotter::tests::HDF5FileTest") {}

void tarch::plotter::tests::HDF5FileTest::run() {}

#endif
