// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/reader/tests/NetCDFFileReaderTest.h"

#ifdef PEANO_WITH_NETCDF

#include "tarch/reader/NetCDFFileReader.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <netcdf.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
  std::string writeTempNetCDFFile(
    const std::string&         stem,
    const std::vector<double>& xValues,
    const std::vector<double>& yValues,
    const std::vector<double>& zValues,
    const std::string&         valueKey = "z",
    const std::string&         xKey     = "x",
    const std::string&         yKey     = "y"
  ) {
    static std::atomic<int> counter = 0;

    const std::filesystem::path path
      = std::filesystem::temp_directory_path()
        / std::filesystem::path(
          stem + "-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-"
          + std::to_string(counter++) + ".nc"
        );

    auto check = [&](int status, const std::string& operation) {
      if (status != NC_NOERR) {
        throw std::runtime_error(operation + ": " + nc_strerror(status));
      }
    };

    if (zValues.size() != xValues.size() * yValues.size()) {
      throw std::runtime_error("Invalid fixture dimensions.");
    }

    int ncid   = -1;
    int xDimId = -1;
    int yDimId = -1;
    int xVarId = -1;
    int yVarId = -1;
    int zVarId = -1;

    check(nc_create(path.string().c_str(), NC_CLOBBER | NC_NETCDF4, &ncid), "Failed to create test NetCDF file");
    check(nc_def_dim(ncid, xKey.c_str(), xValues.size(), &xDimId), "Failed to define x dimension");
    check(nc_def_dim(ncid, yKey.c_str(), yValues.size(), &yDimId), "Failed to define y dimension");
    check(nc_def_var(ncid, xKey.c_str(), NC_DOUBLE, 1, &xDimId, &xVarId), "Failed to define x variable");
    check(nc_def_var(ncid, yKey.c_str(), NC_DOUBLE, 1, &yDimId, &yVarId), "Failed to define y variable");

    const int dimensions[2] = {yDimId, xDimId};
    check(nc_def_var(ncid, valueKey.c_str(), NC_DOUBLE, 2, dimensions, &zVarId), "Failed to define field variable");

    check(nc_enddef(ncid), "Failed to end NetCDF definition mode");
    check(nc_put_var_double(ncid, xVarId, xValues.data()), "Failed to write x values");
    check(nc_put_var_double(ncid, yVarId, yValues.data()), "Failed to write y values");
    check(nc_put_var_double(ncid, zVarId, zValues.data()), "Failed to write field values");
    check(nc_close(ncid), "Failed to close test NetCDF file");

    return path.string();
  }
} // namespace

tarch::reader::tests::NetCDFFileReaderTest::NetCDFFileReaderTest():
  TestCase("tarch::reader::tests::NetCDFFileReaderTest") {}

void tarch::reader::tests::NetCDFFileReaderTest::run() {
  testMethod(testOpenReadAndClose);
  testMethod(testMissingFileFails);
}

void tarch::reader::tests::NetCDFFileReaderTest::testOpenReadAndClose() {
  const std::string path = writeTempNetCDFFile(
    "peano-netcdf-reader",
    {1.0, 3.0, 5.0},
    {-2.0, 4.0},
    {10.0, 11.0, 12.0, 20.0, 21.0, 22.0}
  );

  try {
    tarch::reader::NetCDFFileReader reader;
    const int                       ncid = reader.open(path);

    validate(ncid != -1);
    validateEquals(reader.getDimension(ncid, "x"), static_cast<std::size_t>(3));
    validateEquals(reader.getDimension(ncid, "y"), static_cast<std::size_t>(2));

    double xValues[3] = {};
    double yValues[2] = {};
    double zValues[6] = {};

    validateEquals(reader.readVariable1D(ncid, "x", xValues), 0);
    validateEquals(reader.readVariable1D(ncid, "y", yValues), 0);
    validateEquals(reader.readVariable2D(ncid, "z", zValues), 0);

    validateNumericalEquals(xValues[0], 1.0);
    validateNumericalEquals(xValues[2], 5.0);
    validateNumericalEquals(yValues[0], -2.0);
    validateNumericalEquals(yValues[1], 4.0);
    validateNumericalEquals(zValues[0], 10.0);
    validateNumericalEquals(zValues[2], 12.0);
    validateNumericalEquals(zValues[3], 20.0);
    validateNumericalEquals(zValues[5], 22.0);

    validateEquals(reader.close(ncid), 0);
  } catch (...) {
    std::filesystem::remove(path);
    throw;
  }

  std::filesystem::remove(path);
}

void tarch::reader::tests::NetCDFFileReaderTest::testMissingFileFails() {
  tarch::reader::NetCDFFileReader reader;
  validateEquals(reader.open("/tmp/this-netcdf-fixture-should-not-exist.nc"), -1);
}

#else

tarch::reader::tests::NetCDFFileReaderTest::NetCDFFileReaderTest():
  TestCase("tarch::reader::tests::NetCDFFileReaderTest") {}

void tarch::reader::tests::NetCDFFileReaderTest::run() {}

#endif
