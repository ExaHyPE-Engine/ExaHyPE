// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#ifdef PEANO_WITH_NETCDF

#include "tarch/la/Vector.h"
#include "tarch/reader/NetCDFFileReader.h"

namespace tarch::reader {
  template <int FieldDimensions>
  class NetCDFFieldSampler;
} // namespace tarch::reader

/**
 * Samples one or more fields read from netCDF files at a point of the simulation domain.
 *
 * The sampler assumes that the netCDF variable holding the values is declared with its dimensions
 * in the reverse of the order the coordinate keys are given in, i.e. a 3D field addressed as
 * key = {"x", "y", "z"} has to be stored as value(z, y, x). That is the layout netCDF's own
 * row-major reader hands us, and it is what the flat index computed below assumes. Nothing
 * verifies it, so a file written the other way round samples silently transposed data.
 *
 * Only the 2D and 3D instantiations exist; both are explicitly instantiated in the .cpp.
 */
template <int FieldDimensions = 2>
class tarch::reader::NetCDFFieldSampler {
  static_assert(
    FieldDimensions == 2 or FieldDimensions == 3,
    "NetCDFFieldSampler is only instantiated for two- and three-dimensional fields"
  );

public:
  struct FieldDescription {
    std::string name;
    std::string filePath;
    /**
     * Name of the netCDF variable holding the values. The default collides with the conventional
     * name of the third coordinate, so a 3D field practically always has to name it explicitly.
     * parseFieldFile() rejects a description whose value key is also a coordinate key.
     */
    std::string valueKey = "z";
    /**
     * Names of the netCDF variables holding the coordinates, one per field dimension, in the order
     * sample() expects its argument in.
     */
    std::array<std::string, FieldDimensions> key = defaultCoordinateKeys();
  };

  /**
   * A sampler holds every field it was given in full, which runs to hundreds of megabytes for a
   * topography or material file, and callers construct it in place as a function-local static. A
   * copy would therefore only ever be an accident.
   */
  NetCDFFieldSampler(const NetCDFFieldSampler&)            = delete;
  NetCDFFieldSampler(NetCDFFieldSampler&&)                 = delete;
  NetCDFFieldSampler& operator=(const NetCDFFieldSampler&) = delete;
  NetCDFFieldSampler& operator=(NetCDFFieldSampler&&)      = delete;

  NetCDFFieldSampler(
    std::initializer_list<FieldDescription>    fields,
    tarch::la::Vector<FieldDimensions, double> lengthOfSimulationDomain,
    tarch::la::Vector<FieldDimensions, double> offsetOfSimulationDomain
  );

  NetCDFFieldSampler(
    const std::vector<FieldDescription>&       fields,
    tarch::la::Vector<FieldDimensions, double> lengthOfSimulationDomain,
    tarch::la::Vector<FieldDimensions, double> offsetOfSimulationDomain
  );

  ~NetCDFFieldSampler() = default;

  double sample(const std::string& fieldName, tarch::la::Vector<FieldDimensions, double> position) const;

private:
  struct Field {
    std::vector<double> values;
    // std::size_t(0) rather than a plain 0, which is also a null pointer constant and would make
    // the call ambiguous with Vector's pointer constructor.
    tarch::la::Vector<FieldDimensions, std::size_t> length =
      tarch::la::Vector<FieldDimensions, std::size_t>(std::size_t(0));
    tarch::la::Vector<FieldDimensions, double> firstElement = tarch::la::Vector<FieldDimensions, double>(0.0);
    tarch::la::Vector<FieldDimensions, double> lastElement  = tarch::la::Vector<FieldDimensions, double>(0.0);
  };

  NetCDFFileReader                       _netCDFFileReader;
  std::unordered_map<std::string, Field> _fields;

  tarch::la::Vector<FieldDimensions, double> _lengthOfSimulationDomain =
    tarch::la::Vector<FieldDimensions, double>(-1.0);
  tarch::la::Vector<FieldDimensions, double> _offsetOfSimulationDomain =
    tarch::la::Vector<FieldDimensions, double>(-1.0);

  static std::array<std::string, FieldDimensions> defaultCoordinateKeys();

  void parseFieldFile(const FieldDescription& fieldDescription);

  static double transformSimulationCoordinateToCDFRange(
    double coordinateOfDimensionZInSimulationSpace,
    double sizeOfSimulationSpaceInZDirection,
    double minIndexOfCDFspaceInZDirection,
    double maxIndexOfCDFspaceInZDirection,
    double offsetOfSimulationSpaceInZDirection
  );

  static std::size_t transformIndexCDFRangeToArray(
    tarch::la::Vector<FieldDimensions, double>      coordinatesInCDFMesh,
    tarch::la::Vector<FieldDimensions, double>      firstCoordinatesInCDFMesh,
    tarch::la::Vector<FieldDimensions, double>      lastCoordinatesInCDFMesh,
    tarch::la::Vector<FieldDimensions, std::size_t> lengthOfDimensionsInCDFMesh
  );

  static bool isCoordinateInRange(double coordinate, double firstCoordinate, double lastCoordinate);
};

#endif
