// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/reader/tests/NetCDFFieldSamplerTest.h"

#ifdef PEANO_WITH_NETCDF

#include "tarch/reader/NetCDFFieldSampler.h"

#include <netcdf.h>

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
      = tarch::tests::getWritableTestDirectory()
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

tarch::reader::tests::NetCDFFieldSamplerTest::NetCDFFieldSamplerTest():
  TestCase("tarch::reader::tests::NetCDFFieldSamplerTest") {}

void tarch::reader::tests::NetCDFFieldSamplerTest::run() {
  testMethod(testSampleSingleField);
  testMethod(testOutOfDomainReturnsZero);
  testMethod(testMultipleNamedFields);
  testMethod(testDisplacementUsesItsOwnCoordinateRange);
  testMethod(testSamplingWithSimulationOffset);
}

void tarch::reader::tests::NetCDFFieldSamplerTest::testSampleSingleField() {
  const std::string path = writeTempNetCDFFile(
    "peano-netcdf-sampler-single",
    {0.0, 5.0, 10.0},
    {0.0, 10.0},
    {10.0, 11.0, 12.0, 20.0, 21.0, 22.0}
  );

  try {
    tarch::reader::NetCDFFieldSampler sampler({{"bathymetry", path}}, 10.0, 10.0, 0.0, 0.0);

    validateNumericalEquals(sampler.sample("bathymetry", 0.0, 0.0), 10.0);
    validateNumericalEquals(sampler.sample("bathymetry", 5.0, 0.0), 11.0);
    validateNumericalEquals(sampler.sample("bathymetry", 10.0, 10.0), 22.0);
  } catch (...) {
    std::filesystem::remove(path);
    throw;
  }

  std::filesystem::remove(path);
}

void tarch::reader::tests::NetCDFFieldSamplerTest::testOutOfDomainReturnsZero() {
  const std::string path = writeTempNetCDFFile(
    "peano-netcdf-sampler-range",
    {0.0, 10.0},
    {0.0, 10.0},
    {1.0, 2.0, 3.0, 4.0}
  );

  try {
    tarch::reader::NetCDFFieldSampler sampler({{"bathymetry", path}}, 10.0, 10.0, 0.0, 0.0);

    validateNumericalEquals(sampler.sample("bathymetry", -0.1, 5.0), 0.0);
    validateNumericalEquals(sampler.sample("bathymetry", 5.0, 10.1), 0.0);
  } catch (...) {
    std::filesystem::remove(path);
    throw;
  }

  std::filesystem::remove(path);
}

void tarch::reader::tests::NetCDFFieldSamplerTest::testMultipleNamedFields() {
  const std::string bathymetryPath = writeTempNetCDFFile(
    "peano-netcdf-sampler-bathymetry",
    {0.0, 1.0},
    {0.0, 1.0},
    {1.0, 2.0, 3.0, 4.0}
  );
  const std::string displacementPath = writeTempNetCDFFile(
    "peano-netcdf-sampler-displacement",
    {0.0, 1.0},
    {0.0, 1.0},
    {5.0, 6.0, 7.0, 8.0}
  );

  try {
    tarch::reader::NetCDFFieldSampler
      sampler({{"bathymetry", bathymetryPath}, {"displacement", displacementPath}}, 1.0, 1.0, 0.0, 0.0);

    validateNumericalEquals(sampler.sample("bathymetry", 1.0, 1.0), 4.0);
    validateNumericalEquals(sampler.sample("displacement", 1.0, 1.0), 8.0);
  } catch (...) {
    std::filesystem::remove(bathymetryPath);
    std::filesystem::remove(displacementPath);
    throw;
  }

  std::filesystem::remove(bathymetryPath);
  std::filesystem::remove(displacementPath);
}

void tarch::reader::tests::NetCDFFieldSamplerTest::testDisplacementUsesItsOwnCoordinateRange() {
  const std::string bathymetryPath = writeTempNetCDFFile(
    "peano-netcdf-sampler-topology-range",
    {0.0, 1.0, 2.0},
    {0.0, 1.0},
    {1.0, 2.0, 3.0, 4.0, 5.0, 6.0}
  );
  const std::string displacementPath = writeTempNetCDFFile(
    "peano-netcdf-sampler-displacement-range",
    {10.0, 11.0, 12.0},
    {100.0, 101.0},
    {50.0, 51.0, 52.0, 60.0, 61.0, 62.0}
  );

  try {
    tarch::reader::NetCDFFieldSampler
      sampler({{"bathymetry", bathymetryPath}, {"displacement", displacementPath}}, 2.0, 1.0, 0.0, 0.0);

    validateNumericalEquals(sampler.sample("displacement", 1.0, 0.0), 51.0);
  } catch (...) {
    std::filesystem::remove(bathymetryPath);
    std::filesystem::remove(displacementPath);
    throw;
  }

  std::filesystem::remove(bathymetryPath);
  std::filesystem::remove(displacementPath);
}

void tarch::reader::tests::NetCDFFieldSamplerTest::testSamplingWithSimulationOffset() {
  const std::string path = writeTempNetCDFFile(
    "peano-netcdf-sampler-offset",
    {100.0, 105.0, 110.0},
    {200.0, 210.0, 220.0},
    {10.0, 11.0, 12.0, 20.0, 21.0, 22.0, 30.0, 31.0, 32.0}
  );

  try {
    tarch::reader::NetCDFFieldSampler sampler({{"bathymetry", path}}, 10.0, 20.0, 50.0, 60.0);

    validateNumericalEquals(sampler.sample("bathymetry", 55.0, 70.0), 21.0);
  } catch (...) {
    std::filesystem::remove(path);
    throw;
  }

  std::filesystem::remove(path);
}

#else

tarch::reader::tests::NetCDFFieldSamplerTest::NetCDFFieldSamplerTest():
  TestCase("tarch::reader::tests::NetCDFFieldSamplerTest") {}

void tarch::reader::tests::NetCDFFieldSamplerTest::run() {}

#endif
