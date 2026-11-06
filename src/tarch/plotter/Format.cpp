// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Format.h"

#include "tarch/Termination.h"
#include "tarch/plotter/VTUFileWriter.h"

#if defined(PEANO_WITH_HDF5)
#include "tarch/plotter/HDF5FileWriter.h"
#include "tarch/plotter/VTKHDFFileWriter.h"
#endif

#if defined(PEANO_WITH_NETCDF)
#include "tarch/plotter/NetCDFFileWriter.h"
#endif

namespace {
  constexpr std::array<std::pair<tarch::plotter::Format, const char*>, 4> FormatNames{
    {{tarch::plotter::Format::VTU, "vtu"},
     {tarch::plotter::Format::HDF5, "hdf5"},
     {tarch::plotter::Format::NetCDF, "netcdf"},
     {tarch::plotter::Format::VTKHDF, "vtkhdf"}}
  };

  std::string getKnownFormatNames() {
    std::string names;
    for (const auto& [format, name] : FormatNames) {
      names += names.empty() ? name : std::string(", ") + name;
    }
    return names;
  }
} // namespace

/**
 * Returns the format of name. Terminates for an unknown name.
 */
tarch::plotter::Format
tarch::plotter::parseFormat(const std::string& name) {
  for (const auto& [format, formatName] : FormatNames) {
    if (name == formatName) {
      return format;
    }
  }
  tarch::terminateWithError(
    "The plotter supports the formats " + getKnownFormatNames() + " only (got \"" + name + "\")."
  );
}

std::string
tarch::plotter::toString(const Format format) {
  for (const auto& [knownFormat, name] : FormatNames) {
    if (knownFormat == format) {
      return name;
    }
  }
  tarch::terminateWithError("The plotter supports the formats " + getKnownFormatNames() + " only.");
}

/**
 * Returns whether the build links the library the format writes with.
 */
bool
tarch::plotter::isAvailable(const Format format) {
  switch (format) {
  case Format::VTU:
    return true;
  case Format::HDF5:
  case Format::VTKHDF:
#if defined(PEANO_WITH_HDF5)
    return true;
#else
    return false;
#endif
  case Format::NetCDF:
#if defined(PEANO_WITH_NETCDF)
    return true;
#else
    return false;
#endif
  }
  tarch::terminateWithError("The plotter supports the formats " + getKnownFormatNames() + " only.");
}

/**
 * Returns VTKHDF where the build links HDF5 and VTU otherwise.
 */
tarch::plotter::Format
tarch::plotter::getDefaultFormat() {
  return isAvailable(Format::VTKHDF) ? Format::VTKHDF : Format::VTU;
}

std::unique_ptr<tarch::plotter::FileWriter>
tarch::plotter::createFileWriter(const Format format) {
  switch (format) {
  case Format::VTU:
    return std::make_unique<VTUFileWriter>();
#if defined(PEANO_WITH_HDF5)
  case Format::HDF5:
    return std::make_unique<HDF5FileWriter>();
  case Format::VTKHDF:
    return std::make_unique<VTKHDFFileWriter>();
#endif
#if defined(PEANO_WITH_NETCDF)
  case Format::NetCDF:
    return std::make_unique<NetCDFFileWriter>();
#endif
  default:
    break;
  }
  tarch::terminateWithError(
    "This build cannot write the plot format \"" + toString(format) + "\". Rebuild with the library of this format."
  );
}
