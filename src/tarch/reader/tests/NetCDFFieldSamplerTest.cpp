// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "NetCDFFieldSamplerTest.h"

#ifdef PEANO_WITH_NETCDF

#include "tarch/reader/NetCDFFieldSampler.h"

#include <netcdf.h>

namespace {
  /**
   * Returns a new fixture path in the test directory. The file name contains an identifier that
   * each process computes once, so two ranks never create the same file.
   */
  std::filesystem::path fixturePath(const std::string& stem) {
    static const std::string testRunIdentifier =
      std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-"
      + std::to_string(std::random_device{}());
    static std::atomic<int> counter = 0;

    return tarch::tests::getWritableTestDirectory()
      / std::filesystem::path(stem + "-" + testRunIdentifier + "-" + std::to_string(counter++) + ".nc");
  }

  std::string writeTempNetCDFFile(
    const std::string&         stem,
    const std::vector<double>& xValues,
    const std::vector<double>& yValues,
    const std::vector<double>& zValues,
    const std::string&         valueKey = "z",
    const std::string&         xKey     = "x",
    const std::string&         yKey     = "y"
  ) {
    const std::filesystem::path path = fixturePath(stem);

    auto check = [&](const int status, const std::string& operation) {
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

  /**
   * Writes a 3D fixture that stores its values as value(z, y, x).
   */
  std::string writeTempNetCDF3DFile(
    const std::string&         stem,
    const std::vector<double>& xValues,
    const std::vector<double>& yValues,
    const std::vector<double>& zValues,
    const std::vector<double>& fieldValues,
    const std::string&         valueKey = "value"
  ) {
    const std::filesystem::path path = fixturePath(stem);

    auto check = [&](const int status, const std::string& operation) {
      if (status != NC_NOERR) {
        throw std::runtime_error(operation + ": " + nc_strerror(status));
      }
    };

    if (fieldValues.size() != xValues.size() * yValues.size() * zValues.size()) {
      throw std::runtime_error("Invalid fixture dimensions.");
    }

    int ncid       = -1;
    int xDimId     = -1;
    int yDimId     = -1;
    int zDimId     = -1;
    int xVarId     = -1;
    int yVarId     = -1;
    int zVarId     = -1;
    int fieldVarId = -1;

    check(nc_create(path.string().c_str(), NC_CLOBBER | NC_NETCDF4, &ncid), "Failed to create test NetCDF file");
    check(nc_def_dim(ncid, "x", xValues.size(), &xDimId), "Failed to define x dimension");
    check(nc_def_dim(ncid, "y", yValues.size(), &yDimId), "Failed to define y dimension");
    check(nc_def_dim(ncid, "z", zValues.size(), &zDimId), "Failed to define z dimension");
    check(nc_def_var(ncid, "x", NC_DOUBLE, 1, &xDimId, &xVarId), "Failed to define x variable");
    check(nc_def_var(ncid, "y", NC_DOUBLE, 1, &yDimId, &yVarId), "Failed to define y variable");
    check(nc_def_var(ncid, "z", NC_DOUBLE, 1, &zDimId, &zVarId), "Failed to define z variable");

    const int dimensions[3] = {zDimId, yDimId, xDimId};
    check(nc_def_var(ncid, valueKey.c_str(), NC_DOUBLE, 3, dimensions, &fieldVarId), "Failed to define field variable");

    check(nc_enddef(ncid), "Failed to end NetCDF definition mode");
    check(nc_put_var_double(ncid, xVarId, xValues.data()), "Failed to write x values");
    check(nc_put_var_double(ncid, yVarId, yValues.data()), "Failed to write y values");
    check(nc_put_var_double(ncid, zVarId, zValues.data()), "Failed to write z values");
    check(nc_put_var_double(ncid, fieldVarId, fieldValues.data()), "Failed to write field values");
    check(nc_close(ncid), "Failed to close test NetCDF file");

    return path.string();
  }
} // namespace

tarch::reader::tests::NetCDFFieldSamplerTest::NetCDFFieldSamplerTest() :
  tarch::tests::TestCase("tarch::reader::tests::NetCDFFieldSamplerTest") {}

void
tarch::reader::tests::NetCDFFieldSamplerTest::run() {
  testMethod(testPiecewiseConstantSampleSingleField);
  testMethod(testOutOfDomainReturnsZero);
  testMethod(testMultipleNamedFields);
  testMethod(testDisplacementUsesItsOwnCoordinateRange);
  testMethod(testSamplingWithSimulationOffset);
  testMethod(testSamplingWithFileExtent);
  testMethod(testBilinearSampling);
  testMethod(testThreeDimensionalSampling);
  testMethod(testThreeDimensionalBilinearSampling);
}

void
tarch::reader::tests::NetCDFFieldSamplerTest::testPiecewiseConstantSampleSingleField() {
  const std::string path = writeTempNetCDFFile(
    "peano-netcdf-sampler-single",
    {0.0, 5.0, 10.0},
    {0.0, 10.0},
    {10.0, 11.0, 12.0, 20.0, 21.0, 22.0}
  );

  try {
    tarch::reader::NetCDFFieldSampler<2> sampler({{"bathymetry", path}}, {{10.0, 10.0}}, {{0.0, 0.0}});

    validateNumericalEquals(sampler.samplePiecewiseConstant("bathymetry", {0.0, 0.0}), 10.0);
    validateNumericalEquals(sampler.samplePiecewiseConstant("bathymetry", {5.0, 0.0}), 11.0);
    validateNumericalEquals(sampler.samplePiecewiseConstant("bathymetry", {10.0, 10.0}), 22.0);
  } catch (...) {
    std::filesystem::remove(path);
    throw;
  }

  std::filesystem::remove(path);
}

void
tarch::reader::tests::NetCDFFieldSamplerTest::testOutOfDomainReturnsZero() {
  const std::string path =
    writeTempNetCDFFile("peano-netcdf-sampler-range", {0.0, 10.0}, {0.0, 10.0}, {1.0, 2.0, 3.0, 4.0});

  try {
    tarch::reader::NetCDFFieldSampler<2> sampler({{"bathymetry", path}}, {{10.0, 10.0}}, {{0.0, 0.0}});

    validateNumericalEquals(sampler.samplePiecewiseConstant("bathymetry", {-0.1, 5.0}), 0.0);
    validateNumericalEquals(sampler.samplePiecewiseConstant("bathymetry", {5.0, 10.1}), 0.0);
  } catch (...) {
    std::filesystem::remove(path);
    throw;
  }

  std::filesystem::remove(path);
}

void
tarch::reader::tests::NetCDFFieldSamplerTest::testMultipleNamedFields() {
  const std::string bathymetryPath =
    writeTempNetCDFFile("peano-netcdf-sampler-bathymetry", {0.0, 1.0}, {0.0, 1.0}, {1.0, 2.0, 3.0, 4.0});
  const std::string displacementPath =
    writeTempNetCDFFile("peano-netcdf-sampler-displacement", {0.0, 1.0}, {0.0, 1.0}, {5.0, 6.0, 7.0, 8.0});

  try {
    tarch::reader::NetCDFFieldSampler<2> sampler(
      {{"bathymetry", bathymetryPath}, {"displacement", displacementPath}},
      {{1.0, 1.0}},
      {{0.0, 0.0}}
    );

    validateNumericalEquals(sampler.samplePiecewiseConstant("bathymetry", {1.0, 1.0}), 4.0);
    validateNumericalEquals(sampler.samplePiecewiseConstant("displacement", {1.0, 1.0}), 8.0);
  } catch (...) {
    std::filesystem::remove(bathymetryPath);
    std::filesystem::remove(displacementPath);
    throw;
  }

  std::filesystem::remove(bathymetryPath);
  std::filesystem::remove(displacementPath);
}

void
tarch::reader::tests::NetCDFFieldSamplerTest::testDisplacementUsesItsOwnCoordinateRange() {
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
    tarch::reader::NetCDFFieldSampler<2> sampler(
      {{"bathymetry", bathymetryPath}, {"displacement", displacementPath}},
      {{2.0, 1.0}},
      {{0.0, 0.0}}
    );

    validateNumericalEquals(sampler.samplePiecewiseConstant("displacement", {1.0, 0.0}), 51.0);
  } catch (...) {
    std::filesystem::remove(bathymetryPath);
    std::filesystem::remove(displacementPath);
    throw;
  }

  std::filesystem::remove(bathymetryPath);
  std::filesystem::remove(displacementPath);
}

void
tarch::reader::tests::NetCDFFieldSamplerTest::testSamplingWithSimulationOffset() {
  const std::string path = writeTempNetCDFFile(
    "peano-netcdf-sampler-offset",
    {100.0, 105.0, 110.0},
    {200.0, 210.0, 220.0},
    {10.0, 11.0, 12.0, 20.0, 21.0, 22.0, 30.0, 31.0, 32.0}
  );

  try {
    tarch::reader::NetCDFFieldSampler<2> sampler({{"bathymetry", path}}, {{10.0, 20.0}}, {{50.0, 60.0}});

    validateNumericalEquals(sampler.samplePiecewiseConstant("bathymetry", {55.0, 70.0}), 21.0);
  } catch (...) {
    std::filesystem::remove(path);
    throw;
  }

  std::filesystem::remove(path);
}

void
tarch::reader::tests::NetCDFFieldSamplerTest::testSamplingWithFileExtent() {
  const std::string path = writeTempNetCDFFile(
    "peano-netcdf-sampler-file-extent",
    {100.0, 105.0, 110.0},
    {200.0, 210.0, 220.0},
    {10.0, 11.0, 12.0, 20.0, 21.0, 22.0, 30.0, 31.0, 32.0}
  );

  try {
    const std::vector<tarch::reader::NetCDFFieldSampler<2>::FieldDescription> fields{{"bathymetry", path}};
    const tarch::reader::NetCDFFieldSampler<2>                                sampler(fields);

    validateNumericalEquals(sampler.samplePiecewiseConstant("bathymetry", {105.0, 210.0}), 21.0);
    validateNumericalEquals(sampler.sampleBilinear("bathymetry", {107.5, 215.0}), 26.5);
    validateNumericalEquals(sampler.sampleBilinear("bathymetry", {99.9, 210.0}), 0.0);
  } catch (...) {
    std::filesystem::remove(path);
    throw;
  }

  std::filesystem::remove(path);
}

void
tarch::reader::tests::NetCDFFieldSamplerTest::testBilinearSampling() {
  const std::string path =
    writeTempNetCDFFile("peano-netcdf-sampler-bilinear", {0.0, 10.0}, {0.0, 10.0}, {0.0, 10.0, 20.0, 30.0});

  try {
    tarch::reader::NetCDFFieldSampler<2> sampler({{"bathymetry", path}}, {{10.0, 10.0}}, {{0.0, 0.0}});

    validateNumericalEquals(sampler.samplePiecewiseConstant("bathymetry", {5.0, 5.0}), 0.0);
    validateNumericalEquals(sampler.sampleBilinear("bathymetry", {5.0, 5.0}), 15.0);
    validateNumericalEquals(sampler.sampleBilinear("bathymetry", {10.0, 10.0}), 30.0);
    validateNumericalEquals(sampler.sampleBilinear("bathymetry", {-0.1, 5.0}), 0.0);
  } catch (...) {
    std::filesystem::remove(path);
    throw;
  }

  std::filesystem::remove(path);
}

void
tarch::reader::tests::NetCDFFieldSamplerTest::testThreeDimensionalSampling() {
  // value(z, y, x) over a 2x2x2 grid, so the first coordinate varies fastest.
  const std::string path = writeTempNetCDF3DFile(
    "peano-netcdf-sampler-3d",
    {0.0, 10.0},
    {0.0, 10.0},
    {0.0, 10.0},
    {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0}
  );

  try {
    const tarch::reader::NetCDFFieldSampler<3> sampler({{"material", path, "value", {"x", "y", "z"}}});

    validateNumericalEquals(sampler.samplePiecewiseConstant("material", {0.0, 0.0, 0.0}), 0.0);
    validateNumericalEquals(sampler.samplePiecewiseConstant("material", {10.0, 0.0, 0.0}), 1.0);
    validateNumericalEquals(sampler.samplePiecewiseConstant("material", {0.0, 10.0, 0.0}), 2.0);
    validateNumericalEquals(sampler.samplePiecewiseConstant("material", {0.0, 0.0, 10.0}), 4.0);
    validateNumericalEquals(sampler.samplePiecewiseConstant("material", {10.0, 10.0, 10.0}), 7.0);
    validateNumericalEquals(sampler.samplePiecewiseConstant("material", {-0.1, 0.0, 0.0}), 0.0);
  } catch (...) {
    std::filesystem::remove(path);
    throw;
  }

  std::filesystem::remove(path);
}

void
tarch::reader::tests::NetCDFFieldSamplerTest::testThreeDimensionalBilinearSampling() {
  const std::string path = writeTempNetCDF3DFile(
    "peano-netcdf-sampler-3d-bilinear",
    {0.0, 10.0},
    {0.0, 10.0},
    {0.0, 10.0},
    {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0}
  );

  try {
    const tarch::reader::NetCDFFieldSampler<3> sampler({{"material", path, "value", {"x", "y", "z"}}});

    // The centre averages all eight corners, and a face centre averages the four on it.
    validateNumericalEquals(sampler.sampleBilinear("material", {5.0, 5.0, 5.0}), 3.5);
    validateNumericalEquals(sampler.sampleBilinear("material", {5.0, 0.0, 0.0}), 0.5);
    validateNumericalEquals(sampler.sampleBilinear("material", {0.0, 5.0, 0.0}), 1.0);
    validateNumericalEquals(sampler.sampleBilinear("material", {0.0, 0.0, 5.0}), 2.0);
    validateNumericalEquals(sampler.sampleBilinear("material", {10.0, 10.0, 10.0}), 7.0);
    validateNumericalEquals(sampler.sampleBilinear("material", {0.0, 0.0, -0.1}), 0.0);
  } catch (...) {
    std::filesystem::remove(path);
    throw;
  }

  std::filesystem::remove(path);
}

#else

tarch::reader::tests::NetCDFFieldSamplerTest::NetCDFFieldSamplerTest() :
  tarch::tests::TestCase("tarch::reader::tests::NetCDFFieldSamplerTest") {}

void
tarch::reader::tests::NetCDFFieldSamplerTest::run() {}

#endif
