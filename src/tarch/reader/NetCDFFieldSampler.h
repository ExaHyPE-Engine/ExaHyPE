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
 * Samples fields from netCDF files at a point of the simulation domain. A field with the coordinate
 * keys {"x", "y", "z"} has to store its values as value(z, y, x), and nothing checks this order.
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
     * The netCDF variable of the values, which has to differ from every coordinate key.
     */
    std::string valueKey = "z";
    /**
     * The netCDF variables of the coordinates, in the order of the position argument of the
     * samplers.
     */
    std::array<std::string, FieldDimensions> key = getDefaultCoordinateKeys();
  };

  NetCDFFieldSampler(const NetCDFFieldSampler&)            = delete;
  NetCDFFieldSampler(NetCDFFieldSampler&&)                 = delete;
  NetCDFFieldSampler& operator=(const NetCDFFieldSampler&) = delete;
  NetCDFFieldSampler& operator=(NetCDFFieldSampler&&)      = delete;

  explicit NetCDFFieldSampler(std::initializer_list<FieldDescription> fields);

  explicit NetCDFFieldSampler(const std::vector<FieldDescription>& fields);

  NetCDFFieldSampler(
    std::initializer_list<FieldDescription>           fields,
    const tarch::la::Vector<FieldDimensions, double>& lengthOfSimulationDomain,
    const tarch::la::Vector<FieldDimensions, double>& offsetOfSimulationDomain
  );

  NetCDFFieldSampler(
    const std::vector<FieldDescription>&              fields,
    const tarch::la::Vector<FieldDimensions, double>& lengthOfSimulationDomain,
    const tarch::la::Vector<FieldDimensions, double>& offsetOfSimulationDomain
  );

  ~NetCDFFieldSampler() = default;

  double samplePiecewiseConstant(
    const std::string&                         fieldName,
    tarch::la::Vector<FieldDimensions, double> position
  ) const;

  double sampleBilinear(const std::string& fieldName, tarch::la::Vector<FieldDimensions, double> position) const;

private:
  struct Field {
    std::vector<double> values;
    // A plain 0 would select the pointer constructor of Vector.
    tarch::la::Vector<FieldDimensions, std::size_t> length =
      tarch::la::Vector<FieldDimensions, std::size_t>(std::size_t(0));
    tarch::la::Vector<FieldDimensions, double> firstElement = tarch::la::Vector<FieldDimensions, double>(0.0);
    tarch::la::Vector<FieldDimensions, double> lastElement  = tarch::la::Vector<FieldDimensions, double>(0.0);
    // A sampler without an extent maps every field onto the coordinate range of its own file.
    tarch::la::Vector<FieldDimensions, double> lengthOfSimulationDomain =
      tarch::la::Vector<FieldDimensions, double>(0.0);
    tarch::la::Vector<FieldDimensions, double> offsetOfSimulationDomain =
      tarch::la::Vector<FieldDimensions, double>(0.0);
  };

  NetCDFFileReader                       _netCDFFileReader;
  std::unordered_map<std::string, Field> _fields;

  std::optional<tarch::la::Vector<FieldDimensions, double>> _lengthOfSimulationDomain;
  std::optional<tarch::la::Vector<FieldDimensions, double>> _offsetOfSimulationDomain;

  static std::array<std::string, FieldDimensions> getDefaultCoordinateKeys();

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
