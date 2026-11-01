// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/plotter/Format.h"

#include "tarch/plotter/FileWriter.h"
#include "tarch/testing/UnitTest.h"

UNIT_TEST("Every format name parses to its format and the name of every format parses back to the format") {
  GIVEN("The four formats VTU, HDF5, NetCDF and VTKHDF") {
    static constexpr std::array Formats{
      tarch::plotter::Format::VTU,
      tarch::plotter::Format::HDF5,
      tarch::plotter::Format::NetCDF,
      tarch::plotter::Format::VTKHDF
    };

    WHEN("The test parses the names vtu, hdf5, netcdf and vtkhdf and the name of every format") {
      const tarch::plotter::Format vtu    = tarch::plotter::parseFormat("vtu");
      const tarch::plotter::Format hdf5   = tarch::plotter::parseFormat("hdf5");
      const tarch::plotter::Format netcdf = tarch::plotter::parseFormat("netcdf");
      const tarch::plotter::Format vtkhdf = tarch::plotter::parseFormat("vtkhdf");

      std::vector<tarch::plotter::Format> parsedNames;
      for (const auto format : Formats) {
        parsedNames.push_back(tarch::plotter::parseFormat(tarch::plotter::toString(format)));
      }

      THEN("Each name gives its format and the name of every format gives the format back") {
        VALIDATE_EQUALS(static_cast<int>(vtu), static_cast<int>(tarch::plotter::Format::VTU));
        VALIDATE_EQUALS(static_cast<int>(hdf5), static_cast<int>(tarch::plotter::Format::HDF5));
        VALIDATE_EQUALS(static_cast<int>(netcdf), static_cast<int>(tarch::plotter::Format::NetCDF));
        VALIDATE_EQUALS(static_cast<int>(vtkhdf), static_cast<int>(tarch::plotter::Format::VTKHDF));
        for (std::size_t index = 0; index < Formats.size(); index++) {
          VALIDATE_EQUALS(static_cast<int>(parsedNames[index]), static_cast<int>(Formats[index]));
        }
      }
    }
  }
}

UNIT_TEST("The default format and VTU are available and VTKHDF is the default where the build links HDF5") {
  GIVEN("The default format and whether the build writes HDF5 and VTKHDF") {
    const tarch::plotter::Format defaultFormat   = tarch::plotter::getDefaultFormat();
    const bool                   hdf5Available   = tarch::plotter::isAvailable(tarch::plotter::Format::HDF5);
    const bool                   vtkhdfAvailable = tarch::plotter::isAvailable(tarch::plotter::Format::VTKHDF);

    THEN("The default format and VTU are available, HDF5 comes with VTKHDF and VTKHDF is the default with HDF5") {
      VALIDATE(tarch::plotter::isAvailable(defaultFormat));
      VALIDATE(tarch::plotter::isAvailable(tarch::plotter::Format::VTU));
      VALIDATE_EQUALS(hdf5Available, vtkhdfAvailable);
      VALIDATE_EQUALS(defaultFormat == tarch::plotter::Format::VTKHDF, hdf5Available);
    }
  }
}

UNIT_TEST("For every available format, createFileWriter() returns a writer of this format") {
  GIVEN("The four formats VTU, HDF5, NetCDF and VTKHDF") {
    static constexpr std::array Formats{
      tarch::plotter::Format::VTU,
      tarch::plotter::Format::HDF5,
      tarch::plotter::Format::NetCDF,
      tarch::plotter::Format::VTKHDF
    };

    WHEN("createFileWriter() creates a file writer for every available format") {
      std::vector<tarch::plotter::Format>                      availableFormats;
      std::vector<std::unique_ptr<tarch::plotter::FileWriter>> fileWriters;
      for (const auto format : Formats) {
        if (tarch::plotter::isAvailable(format)) {
          availableFormats.push_back(format);
          fileWriters.push_back(tarch::plotter::createFileWriter(format));
        }
      }

      THEN("Every available format receives a file writer that reports this format") {
        for (std::size_t index = 0; index < availableFormats.size(); index++) {
          VALIDATE(fileWriters[index] != nullptr, tarch::plotter::toString(availableFormats[index]));
          VALIDATE(
            fileWriters[index]->getFormat() == availableFormats[index],
            tarch::plotter::toString(availableFormats[index])
          );
        }
      }
    }
  }
}
