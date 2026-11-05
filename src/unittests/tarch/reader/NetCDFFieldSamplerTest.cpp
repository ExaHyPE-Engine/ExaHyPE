// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/testing/UnitTest.h"

#ifdef PEANO_WITH_NETCDF

#include "tarch/Assertions.h"
#include "tarch/reader/NetCDFFieldSampler.h"

#include <netcdf.h>

namespace {
  /**
   * Returns a new fixture path in the test directory. The file name contains an identifier that
   * each process computes once. Two ranks never create the same file.
   */
  std::filesystem::path getFixturePath(const std::string& stem) {
    static const std::string testRunIdentifier =
      std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-"
      + std::to_string(std::random_device{}());
    static std::atomic<int> counter = 0;

    return tarch::testing::getWritableTestDirectory()
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
    const std::filesystem::path path = getFixturePath(stem);

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
    const std::filesystem::path path = getFixturePath(stem);

    const auto check = [&](const int status, const std::string& operation) {
      VALIDATE_EQUALS(status, NC_NOERR, operation);
    };

    ASSERTION(
      fieldValues.size() == xValues.size() * yValues.size() * zValues.size(),
      fieldValues.size(),
      xValues.size(),
      yValues.size(),
      zValues.size()
    );

    int fileId          = -1;
    int xDimensionId    = -1;
    int yDimensionId    = -1;
    int zDimensionId    = -1;
    int xVariableId     = -1;
    int yVariableId     = -1;
    int zVariableId     = -1;
    int fieldVariableId = -1;

    check(nc_create(path.string().c_str(), NC_CLOBBER | NC_NETCDF4, &fileId), "create the NetCDF file");
    check(nc_def_dim(fileId, "x", xValues.size(), &xDimensionId), "define the x dimension");
    check(nc_def_dim(fileId, "y", yValues.size(), &yDimensionId), "define the y dimension");
    check(nc_def_dim(fileId, "z", zValues.size(), &zDimensionId), "define the z dimension");
    check(nc_def_var(fileId, "x", NC_DOUBLE, 1, &xDimensionId, &xVariableId), "define the x variable");
    check(nc_def_var(fileId, "y", NC_DOUBLE, 1, &yDimensionId, &yVariableId), "define the y variable");
    check(nc_def_var(fileId, "z", NC_DOUBLE, 1, &zDimensionId, &zVariableId), "define the z variable");

    const int dimensions[3] = {zDimensionId, yDimensionId, xDimensionId};
    check(
      nc_def_var(fileId, valueKey.c_str(), NC_DOUBLE, 3, dimensions, &fieldVariableId),
      "define the field variable"
    );

    check(nc_enddef(fileId), "end the definition mode");
    check(nc_put_var_double(fileId, xVariableId, xValues.data()), "write the x values");
    check(nc_put_var_double(fileId, yVariableId, yValues.data()), "write the y values");
    check(nc_put_var_double(fileId, zVariableId, zValues.data()), "write the z values");
    check(nc_put_var_double(fileId, fieldVariableId, fieldValues.data()), "write the field values");
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

UNIT_TEST("Piecewise-constant sampling returns the value at the lower grid point in each coordinate direction") {
  GIVEN("A sampler of the values 10 to 12 and 20 to 22 on the grid x = 0, 5, 10 and y = 0, 10 of a ten by ten domain") {
    const std::string path = writeTempNetCDFFile(
      "peano-netcdf-sampler-single",
      {0.0, 5.0, 10.0},
      {0.0, 10.0},
      {10.0, 11.0, 12.0, 20.0, 21.0, 22.0}
    );
    const RemoveFileOnReturn                   removal(path);
    const tarch::reader::NetCDFFieldSampler<2> sampler({{"bathymetry", path}}, {{10.0, 10.0}}, {{0.0, 0.0}});

    WHEN("The sampler samples the positions (0, 0), (5, 0) and (10, 10)") {
      const double atOrigin      = sampler.samplePiecewiseConstant("bathymetry", {0.0, 0.0});
      const double atSecondPoint = sampler.samplePiecewiseConstant("bathymetry", {5.0, 0.0});
      const double atLastPoint   = sampler.samplePiecewiseConstant("bathymetry", {10.0, 10.0});

      THEN("The samples are the values 10, 11 and 22 stored at these grid points") {
        VALIDATE_NUMERICAL_EQUALS(atOrigin, 10.0);
        VALIDATE_NUMERICAL_EQUALS(atSecondPoint, 11.0);
        VALIDATE_NUMERICAL_EQUALS(atLastPoint, 22.0);
      }
    }
  }
}

UNIT_TEST("A position outside the domain samples to zero") {
  GIVEN("A sampler of the values 1 to 4 on the grid x = 0, 10 and y = 0, 10 of a ten by ten domain") {
    const std::string path =
      writeTempNetCDFFile("peano-netcdf-sampler-range", {0.0, 10.0}, {0.0, 10.0}, {1.0, 2.0, 3.0, 4.0});
    const RemoveFileOnReturn                   removal(path);
    const tarch::reader::NetCDFFieldSampler<2> sampler({{"bathymetry", path}}, {{10.0, 10.0}}, {{0.0, 0.0}});

    WHEN("The sampler samples the positions (-0.1, 5) and (5, 10.1)") {
      const double leftOfTheDomain = sampler.samplePiecewiseConstant("bathymetry", {-0.1, 5.0});
      const double aboveTheDomain  = sampler.samplePiecewiseConstant("bathymetry", {5.0, 10.1});

      THEN("Both samples are zero") {
        VALIDATE_NUMERICAL_EQUALS(leftOfTheDomain, 0.0);
        VALIDATE_NUMERICAL_EQUALS(aboveTheDomain, 0.0);
      }
    }
  }
}

UNIT_TEST("covers() accepts the positions that map into the stored extent of a field and rejects the others") {
  GIVEN("A file on [100, 110] x [200, 220], a sampler in its coordinates and a sampler of a 1 by 2 domain") {
    const std::string path =
      writeTempNetCDFFile("peano-netcdf-sampler-covers", {100.0, 110.0}, {200.0, 220.0}, {1.0, 2.0, 3.0, 4.0});
    const RemoveFileOnReturn                                                  removal(path);
    const std::vector<tarch::reader::NetCDFFieldSampler<2>::FieldDescription> fields{{"bathymetry", path}};

    const tarch::reader::NetCDFFieldSampler<2> fileCoordinates(fields);
    const tarch::reader::NetCDFFieldSampler<2> mappedCoordinates(fields, {1.0, 2.0}, {0.0, 0.0});

    WHEN("Both samplers check the corners of the file, positions beyond them and positions of the domain") {
      const bool coversTheLowerCorner   = fileCoordinates.covers("bathymetry", {100.0, 200.0});
      const bool coversTheUpperCorner   = fileCoordinates.covers("bathymetry", {110.0, 220.0});
      const bool coversLeftOfTheFile    = fileCoordinates.covers("bathymetry", {99.9, 210.0});
      const bool coversAboveTheFile     = fileCoordinates.covers("bathymetry", {105.0, 220.1});
      const bool coversInsideTheDomain  = mappedCoordinates.covers("bathymetry", {0.5, 2.0});
      const bool coversOutsideTheDomain = mappedCoordinates.covers("bathymetry", {1.1, 1.0});

      THEN("Both samplers accept the positions inside the stored extent and reject the others") {
        VALIDATE(coversTheLowerCorner);
        VALIDATE(coversTheUpperCorner);
        VALIDATE(not coversLeftOfTheFile);
        VALIDATE(not coversAboveTheFile);
        VALIDATE(coversInsideTheDomain);
        VALIDATE(not coversOutsideTheDomain);
      }
    }
  }
}

UNIT_TEST("Every named field samples its own file") {
  GIVEN("A sampler of a bathymetry with the values 1 to 4 and a displacement with the values 5 to 8 on one grid") {
    const std::string bathymetryPath =
      writeTempNetCDFFile("peano-netcdf-sampler-bathymetry", {0.0, 1.0}, {0.0, 1.0}, {1.0, 2.0, 3.0, 4.0});
    const std::string displacementPath =
      writeTempNetCDFFile("peano-netcdf-sampler-displacement", {0.0, 1.0}, {0.0, 1.0}, {5.0, 6.0, 7.0, 8.0});
    const RemoveFileOnReturn                   bathymetryRemoval(bathymetryPath);
    const RemoveFileOnReturn                   displacementRemoval(displacementPath);
    const tarch::reader::NetCDFFieldSampler<2> sampler(
      {{"bathymetry", bathymetryPath}, {"displacement", displacementPath}},
      {{1.0, 1.0}},
      {{0.0, 0.0}}
    );

    WHEN("The sampler samples both fields at the position (1, 1)") {
      const double bathymetry   = sampler.samplePiecewiseConstant("bathymetry", {1.0, 1.0});
      const double displacement = sampler.samplePiecewiseConstant("displacement", {1.0, 1.0});

      THEN("The samples are the last values 4 and 8 of the two files") {
        VALIDATE_NUMERICAL_EQUALS(bathymetry, 4.0);
        VALIDATE_NUMERICAL_EQUALS(displacement, 8.0);
      }
    }
  }
}

UNIT_TEST("The displacement maps the domain onto its own coordinate range") {
  GIVEN("A sampler of a 2 by 1 domain with bathymetry on [0, 2] x [0, 1] and displacement on [10, 12] x [100, 101]") {
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
    const RemoveFileOnReturn                   bathymetryRemoval(bathymetryPath);
    const RemoveFileOnReturn                   displacementRemoval(displacementPath);
    const tarch::reader::NetCDFFieldSampler<2> sampler(
      {{"bathymetry", bathymetryPath}, {"displacement", displacementPath}},
      {{2.0, 1.0}},
      {{0.0, 0.0}}
    );

    WHEN("The sampler samples the displacement at the position (1, 0)") {
      const double displacement = sampler.samplePiecewiseConstant("displacement", {1.0, 0.0});

      THEN("The sample is the value 51 at the displacement coordinate (11, 100)") {
        VALIDATE_NUMERICAL_EQUALS(displacement, 51.0);
      }
    }
  }
}

UNIT_TEST("A sampler maps the offset simulation domain onto the coordinate range of the file") {
  GIVEN("A sampler of a 10 by 20 domain at the offset (50, 60) with a file on [100, 110] x [200, 220]") {
    const std::string path = writeTempNetCDFFile(
      "peano-netcdf-sampler-offset",
      {100.0, 105.0, 110.0},
      {200.0, 210.0, 220.0},
      {10.0, 11.0, 12.0, 20.0, 21.0, 22.0, 30.0, 31.0, 32.0}
    );
    const RemoveFileOnReturn                   removal(path);
    const tarch::reader::NetCDFFieldSampler<2> sampler({{"bathymetry", path}}, {{10.0, 20.0}}, {{50.0, 60.0}});

    WHEN("The sampler samples the centre (55, 70) of the domain") {
      const double atCentre = sampler.samplePiecewiseConstant("bathymetry", {55.0, 70.0});

      THEN("The sample is the value 21 at the centre (105, 210) of the file") {
        VALIDATE_NUMERICAL_EQUALS(atCentre, 21.0);
      }
    }
  }
}

UNIT_TEST("A sampler without a domain samples at the coordinates of the file") {
  GIVEN("A sampler without a domain of a file on [100, 110] x [200, 220]") {
    const std::string path = writeTempNetCDFFile(
      "peano-netcdf-sampler-file-extent",
      {100.0, 105.0, 110.0},
      {200.0, 210.0, 220.0},
      {10.0, 11.0, 12.0, 20.0, 21.0, 22.0, 30.0, 31.0, 32.0}
    );
    const RemoveFileOnReturn                                                  removal(path);
    const std::vector<tarch::reader::NetCDFFieldSampler<2>::FieldDescription> fields{{"bathymetry", path}};
    const tarch::reader::NetCDFFieldSampler<2>                                sampler(fields);

    WHEN("The sampler samples (105, 210) piecewise constant and (107.5, 215) and (99.9, 210) bilinearly") {
      const double atGridPoint       = sampler.samplePiecewiseConstant("bathymetry", {105.0, 210.0});
      const double betweenGridPoints = sampler.sampleBilinear("bathymetry", {107.5, 215.0});
      const double leftOfTheFile     = sampler.sampleBilinear("bathymetry", {99.9, 210.0});

      THEN("The samples are the stored value 21, the average 26.5 of the four values around and zero outside") {
        VALIDATE_NUMERICAL_EQUALS(atGridPoint, 21.0);
        VALIDATE_NUMERICAL_EQUALS(betweenGridPoints, 26.5);
        VALIDATE_NUMERICAL_EQUALS(leftOfTheFile, 0.0);
      }
    }
  }
}

UNIT_TEST("Bilinear sampling interpolates between the four surrounding grid points") {
  GIVEN("A sampler of the values 0, 10, 20 and 30 on the grid x = 0, 10 and y = 0, 10 of a ten by ten domain") {
    const std::string path =
      writeTempNetCDFFile("peano-netcdf-sampler-bilinear", {0.0, 10.0}, {0.0, 10.0}, {0.0, 10.0, 20.0, 30.0});
    const RemoveFileOnReturn                   removal(path);
    const tarch::reader::NetCDFFieldSampler<2> sampler({{"bathymetry", path}}, {{10.0, 10.0}}, {{0.0, 0.0}});

    WHEN("The sampler samples (5, 5) piecewise constant and (5, 5), (10, 10) and (-0.1, 5) bilinearly") {
      const double piecewiseConstantAtCentre = sampler.samplePiecewiseConstant("bathymetry", {5.0, 5.0});
      const double bilinearAtCentre          = sampler.sampleBilinear("bathymetry", {5.0, 5.0});
      const double bilinearAtLastPoint       = sampler.sampleBilinear("bathymetry", {10.0, 10.0});
      const double bilinearLeftOfTheDomain   = sampler.sampleBilinear("bathymetry", {-0.1, 5.0});

      THEN("The piecewise-constant sample is the lower value 0 and the bilinear ones are 15, 30 and zero outside") {
        VALIDATE_NUMERICAL_EQUALS(piecewiseConstantAtCentre, 0.0);
        VALIDATE_NUMERICAL_EQUALS(bilinearAtCentre, 15.0);
        VALIDATE_NUMERICAL_EQUALS(bilinearAtLastPoint, 30.0);
        VALIDATE_NUMERICAL_EQUALS(bilinearLeftOfTheDomain, 0.0);
      }
    }
  }
}

UNIT_TEST("Piecewise-constant sampling of a 3D field returns the corner values, the first coordinate varying fastest") {
  GIVEN("A sampler of the values 0 to 7, stored as value(z, y, x), at the corners of the cube from 0 to 10") {
    const std::string path = writeTempNetCDF3DFile(
      "peano-netcdf-sampler-3d",
      {0.0, 10.0},
      {0.0, 10.0},
      {0.0, 10.0},
      {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0}
    );
    const RemoveFileOnReturn                   removal(path);
    const tarch::reader::NetCDFFieldSampler<3> sampler({{"material", path, "value", {"x", "y", "z"}}});

    WHEN("The sampler samples the origin, the corner on each axis, the opposite corner and a point left of the cube") {
      const double atOrigin         = sampler.samplePiecewiseConstant("material", {0.0, 0.0, 0.0});
      const double atCornerOnX      = sampler.samplePiecewiseConstant("material", {10.0, 0.0, 0.0});
      const double atCornerOnY      = sampler.samplePiecewiseConstant("material", {0.0, 10.0, 0.0});
      const double atCornerOnZ      = sampler.samplePiecewiseConstant("material", {0.0, 0.0, 10.0});
      const double atOppositeCorner = sampler.samplePiecewiseConstant("material", {10.0, 10.0, 10.0});
      const double leftOfTheCube    = sampler.samplePiecewiseConstant("material", {-0.1, 0.0, 0.0});

      THEN("The samples are the corner values 0, 1, 2, 4 and 7 and zero left of the cube") {
        VALIDATE_NUMERICAL_EQUALS(atOrigin, 0.0);
        VALIDATE_NUMERICAL_EQUALS(atCornerOnX, 1.0);
        VALIDATE_NUMERICAL_EQUALS(atCornerOnY, 2.0);
        VALIDATE_NUMERICAL_EQUALS(atCornerOnZ, 4.0);
        VALIDATE_NUMERICAL_EQUALS(atOppositeCorner, 7.0);
        VALIDATE_NUMERICAL_EQUALS(leftOfTheCube, 0.0);
      }
    }
  }
}

UNIT_TEST("Bilinear sampling of a 3D field averages the corners around the position") {
  GIVEN("A sampler of the values 0 to 7, stored as value(z, y, x), at the corners of the cube from 0 to 10") {
    const std::string path = writeTempNetCDF3DFile(
      "peano-netcdf-sampler-3d-bilinear",
      {0.0, 10.0},
      {0.0, 10.0},
      {0.0, 10.0},
      {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0}
    );
    const RemoveFileOnReturn                   removal(path);
    const tarch::reader::NetCDFFieldSampler<3> sampler({{"material", path, "value", {"x", "y", "z"}}});

    WHEN("The sampler samples the centre, the edge midpoint on each axis, the opposite corner and a position below") {
      const double atCentre          = sampler.sampleBilinear("material", {5.0, 5.0, 5.0});
      const double atEdgeMidpointOnX = sampler.sampleBilinear("material", {5.0, 0.0, 0.0});
      const double atEdgeMidpointOnY = sampler.sampleBilinear("material", {0.0, 5.0, 0.0});
      const double atEdgeMidpointOnZ = sampler.sampleBilinear("material", {0.0, 0.0, 5.0});
      const double atOppositeCorner  = sampler.sampleBilinear("material", {10.0, 10.0, 10.0});
      const double belowTheCube      = sampler.sampleBilinear("material", {0.0, 0.0, -0.1});

      THEN("A point below is zero, the centre averages all eight corners and an edge midpoint the ends of its edge") {
        VALIDATE_NUMERICAL_EQUALS(atCentre, 3.5);
        VALIDATE_NUMERICAL_EQUALS(atEdgeMidpointOnX, 0.5);
        VALIDATE_NUMERICAL_EQUALS(atEdgeMidpointOnY, 1.0);
        VALIDATE_NUMERICAL_EQUALS(atEdgeMidpointOnZ, 2.0);
        VALIDATE_NUMERICAL_EQUALS(atOppositeCorner, 7.0);
        VALIDATE_NUMERICAL_EQUALS(belowTheCube, 0.0);
      }
    }
  }
}

#endif
