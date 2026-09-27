// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FormatTest.h"

#include "tarch/plotter/FileWriter.h"
#include "tarch/plotter/Format.h"
#include "tarch/plotter/TimeSeriesWriter.h"

tarch::plotter::tests::FormatTest::FormatTest() :
  TestCase("tarch::plotter::tests::FormatTest") {}

void
tarch::plotter::tests::FormatTest::run() {
  testMethod(testEveryNameParsesToItsFormatAndBack);
  testMethod(testTheDefaultFormatIsAvailable);
  testMethod(testTheWritersOfAnAvailableFormatExist);
}

void
tarch::plotter::tests::FormatTest::testEveryNameParsesToItsFormatAndBack() {
  validateEquals(static_cast<int>(tarch::plotter::parseFormat("vtu")), static_cast<int>(tarch::plotter::Format::VTU));
  validateEquals(static_cast<int>(tarch::plotter::parseFormat("hdf5")), static_cast<int>(tarch::plotter::Format::HDF5));
  validateEquals(
    static_cast<int>(tarch::plotter::parseFormat("netcdf")),
    static_cast<int>(tarch::plotter::Format::NetCDF)
  );
  validateEquals(
    static_cast<int>(tarch::plotter::parseFormat("vtkhdf")),
    static_cast<int>(tarch::plotter::Format::VTKHDF)
  );
  for (const auto format :
       {tarch::plotter::Format::VTU,
        tarch::plotter::Format::HDF5,
        tarch::plotter::Format::NetCDF,
        tarch::plotter::Format::VTKHDF}) {
    validateEquals(
      static_cast<int>(tarch::plotter::parseFormat(tarch::plotter::toString(format))),
      static_cast<int>(format)
    );
  }
}

void
tarch::plotter::tests::FormatTest::testTheDefaultFormatIsAvailable() {
  validate(tarch::plotter::isAvailable(tarch::plotter::getDefaultFormat()));
  validate(tarch::plotter::isAvailable(tarch::plotter::Format::VTU));
  const bool hdf5Available   = tarch::plotter::isAvailable(tarch::plotter::Format::HDF5);
  const bool vtkhdfAvailable = tarch::plotter::isAvailable(tarch::plotter::Format::VTKHDF);
  validateEquals(hdf5Available, vtkhdfAvailable);
  const bool defaultIsVtkhdf = tarch::plotter::getDefaultFormat() == tarch::plotter::Format::VTKHDF;
  validateEquals(defaultIsVtkhdf, hdf5Available);
}

/** Every available format hands out a file writer and a time series writer. */
void
tarch::plotter::tests::FormatTest::testTheWritersOfAnAvailableFormatExist() {
  const std::string directory = (tarch::tests::getWritableTestDirectory() / "FormatTest").string() + "/";
  for (const auto format :
       {tarch::plotter::Format::VTU,
        tarch::plotter::Format::HDF5,
        tarch::plotter::Format::NetCDF,
        tarch::plotter::Format::VTKHDF}) {
    if (tarch::plotter::isAvailable(format)) {
      const auto fileWriter = tarch::plotter::createFileWriter(format, directory);
      validateWithParams1(fileWriter != nullptr, tarch::plotter::toString(format));
      validateWithParams1(fileWriter->isOpen(), tarch::plotter::toString(format));
      const auto timeSeriesWriter = tarch::plotter::createTimeSeriesWriter(format);
      validateWithParams1(timeSeriesWriter != nullptr, tarch::plotter::toString(format));
    }
  }
}
