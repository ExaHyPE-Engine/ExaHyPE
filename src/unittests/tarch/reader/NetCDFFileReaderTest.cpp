// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/testing/UnitTest.h"

#ifdef PEANO_WITH_NETCDF

#include "tarch/Assertions.h"
#include "tarch/reader/NetCDFFileReader.h"

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
    // The file name contains a per-run identifier. Two processes never write the same fixture.
    static const std::string testRunIdentifier =
      std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-"
      + std::to_string(std::random_device{}());
    static std::atomic<int> counter = 0;

    const std::filesystem::path path = tarch::testing::getWritableTestDirectory()
      / std::filesystem::path(stem + "-" + testRunIdentifier + "-" + std::to_string(counter++) + ".nc");

    const auto check = [&](const int status, const std::string& operation) {
      VALIDATE_EQUALS(status, NC_NOERR, operation);
    };

    ASSERTION(zValues.size() == xValues.size() * yValues.size(), zValues.size(), xValues.size(), yValues.size());

    int fileId       = -1;
    int xDimensionId = -1;
    int yDimensionId = -1;
    int xVariableId  = -1;
    int yVariableId  = -1;
    int zVariableId  = -1;

    check(nc_create(path.string().c_str(), NC_CLOBBER | NC_NETCDF4, &fileId), "create the NetCDF file");
    check(nc_def_dim(fileId, xKey.c_str(), xValues.size(), &xDimensionId), "define the x dimension");
    check(nc_def_dim(fileId, yKey.c_str(), yValues.size(), &yDimensionId), "define the y dimension");
    check(nc_def_var(fileId, xKey.c_str(), NC_DOUBLE, 1, &xDimensionId, &xVariableId), "define the x variable");
    check(nc_def_var(fileId, yKey.c_str(), NC_DOUBLE, 1, &yDimensionId, &yVariableId), "define the y variable");

    const int dimensions[2] = {yDimensionId, xDimensionId};
    check(nc_def_var(fileId, valueKey.c_str(), NC_DOUBLE, 2, dimensions, &zVariableId), "define the field variable");

    check(nc_enddef(fileId), "end the definition mode");
    check(nc_put_var_double(fileId, xVariableId, xValues.data()), "write the x values");
    check(nc_put_var_double(fileId, yVariableId, yValues.data()), "write the y values");
    check(nc_put_var_double(fileId, zVariableId, zValues.data()), "write the field values");
    check(nc_close(fileId), "close the NetCDF file");

    return path.string();
  }

  /**
   * Removes the file at the end of its scope, after a failed check or an exception too.
   */
  class RemoveFileOnReturn final {
  public:
    explicit RemoveFileOnReturn(const std::string& path) :
      _path(path) {}
    RemoveFileOnReturn(const RemoveFileOnReturn& other)            = delete;
    RemoveFileOnReturn(RemoveFileOnReturn&& other)                 = delete;
    RemoveFileOnReturn& operator=(const RemoveFileOnReturn& other) = delete;
    RemoveFileOnReturn& operator=(RemoveFileOnReturn&& other)      = delete;
    ~RemoveFileOnReturn() {
      std::error_code errorCode;
      std::filesystem::remove(_path, errorCode);
    }

  private:
    const std::string _path;
  };
} // namespace

UNIT_TEST("The reader opens a file, reads its dimensions and variables and closes it") {
  GIVEN("A reader and a file with the coordinates x = 1, 3, 5 and y = -2, 4 and a field z of six values") {
    const std::string path =
      writeTempNetCDFFile("peano-netcdf-reader", {1.0, 3.0, 5.0}, {-2.0, 4.0}, {10.0, 11.0, 12.0, 20.0, 21.0, 22.0});
    const RemoveFileOnReturn        removal(path);
    tarch::reader::NetCDFFileReader reader;

    WHEN("The reader opens the file, reads both dimensions, the three variables and a missing name and closes it") {
      const int         fileId            = reader.open(path);
      const std::size_t xDimension        = reader.getDimension(fileId, "x");
      const std::size_t yDimension        = reader.getDimension(fileId, "y");
      const std::size_t missingDimension  = reader.getDimension(fileId, "missing");
      double            xValues[3]        = {};
      double            yValues[2]        = {};
      double            zValues[6]        = {};
      const int         xReadStatus       = reader.readVariable(fileId, "x", xValues);
      const int         yReadStatus       = reader.readVariable(fileId, "y", yValues);
      const int         zReadStatus       = reader.readVariable(fileId, "z", zValues);
      const int         missingReadStatus = reader.readVariable(fileId, "missing", zValues);
      const int         closeStatus       = reader.close(fileId);

      THEN("The file opens with dimensions three and two, the reads return the stored values and the file closes") {
        VALIDATE(fileId != -1);
        VALIDATE_EQUALS(xDimension, static_cast<std::size_t>(3));
        VALIDATE_EQUALS(yDimension, static_cast<std::size_t>(2));

        VALIDATE_EQUALS(xReadStatus, 0);
        VALIDATE_EQUALS(yReadStatus, 0);
        VALIDATE_EQUALS(zReadStatus, 0);

        VALIDATE_NUMERICAL_EQUALS(xValues[0], 1.0);
        VALIDATE_NUMERICAL_EQUALS(xValues[2], 5.0);
        VALIDATE_NUMERICAL_EQUALS(yValues[0], -2.0);
        VALIDATE_NUMERICAL_EQUALS(yValues[1], 4.0);
        VALIDATE_NUMERICAL_EQUALS(zValues[0], 10.0);
        VALIDATE_NUMERICAL_EQUALS(zValues[2], 12.0);
        VALIDATE_NUMERICAL_EQUALS(zValues[3], 20.0);
        VALIDATE_NUMERICAL_EQUALS(zValues[5], 22.0);

        VALIDATE_EQUALS(closeStatus, 0);
      }

      THEN("The dimension of the missing name is zero and reading it returns minus one") {
        VALIDATE_EQUALS(missingDimension, static_cast<std::size_t>(0));
        VALIDATE_EQUALS(missingReadStatus, -1);
      }
    }
  }
}

UNIT_TEST("The reader fails to open a file that does not exist") {
  GIVEN("A reader and the path of a file that does not exist") {
    tarch::reader::NetCDFFileReader reader;
    const std::string               missingPath = "/tmp/this-netcdf-fixture-should-not-exist.nc";

    WHEN("The reader opens the file") {
      const int fileId = reader.open(missingPath);

      THEN("The open returns minus one") { VALIDATE_EQUALS(fileId, -1); }
    }
  }
}

#endif
