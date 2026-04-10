// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#ifdef WITH_NETCDF

#include "NetCDFFileReader.h"
#include "tarch/logging/Log.h"

#include <initializer_list>
#include <string>
#include <unordered_map>
#include <vector>

namespace tarch::reader {
  class NetCDFFieldSampler;
} // namespace tarch::reader

class tarch::reader::NetCDFFieldSampler {
public:
  struct FieldDescription {
    std::string name;
    std::string filePath;
    std::string valueKey = "z";
    std::string xKey     = "x";
    std::string yKey     = "y";
  };

private:
  static tarch::logging::Log _log;

  struct Field {
    std::vector<double> values;
    std::size_t         lengthInX = 0;
    std::size_t         lengthInY = 0;
    double              firstX    = 0.0;
    double              lastX     = 0.0;
    double              firstY    = 0.0;
    double              lastY     = 0.0;
  };

  NetCDFFileReader                       _netCDFFileReader;
  std::unordered_map<std::string, Field> _fields;

  double _lengthOfSimulationDomainInX = -1;
  double _lengthOfSimulationDomainInY = -1;
  double _offsetOfSimulationDomainInX = -1;
  double _offsetOfSimulationDomainInY = -1;

  void parseFieldFile(const FieldDescription& fieldDescription);

  /**
   * @brief Transforms coordinate from simulation plane to netCDF plane.
   *
   * @return index in netCDF space [minIndexOfCDFspaceInZDirection,
   * maxIndexOfCDFspaceInZDirection]
   */
  static double transformSimulationCoordinateToCDFRange(
    double coordinateOfDimensionZInSimulationSpace,
    double sizeOfSimulationSpaceInZDirection,
    double minIndexOfCDFspaceInZDirection,
    double maxIndexOfCDFspaceInZDirection,
    double offsetOfSimulationSpaceInZDirection
  );

  /**
   * @brief Transforms coordinate pair from netCDF plane to 1D row-wise array
   * index.
   *
   * @return index in [0, ]
   */
  static int transformIndexCDFRangeToArray(
    double      xCoordinateInCDFMesh,
    double      yCoordinateInCDFMesh,
    double      firstXCoordinateInCDFMesh,
    double      firstYCoordinateInCDFMesh,
    double      lastXCoordinateInCDFMesh,
    double      lastYCoordinateInCDFMesh,
    std::size_t lengthOfXDimensionInCDFMesh,
    std::size_t lengthOfYDimensionInCDFMesh
  );

  static bool isCoordinateInRange(double coordinate, double firstCoordinate, double lastCoordinate);

public:
  NetCDFFieldSampler(
    std::initializer_list<FieldDescription> fields,
    double                                  lengthOfSimulationDomainInX,
    double                                  lengthOfSimulationDomainInY,
    double                                  offsetOfSimulationDomainInX,
    double                                  offsetOfSimulationDomainInY
  );

  NetCDFFieldSampler(
    const std::vector<FieldDescription>& fields,
    double                               lengthOfSimulationDomainInX,
    double                               lengthOfSimulationDomainInY,
    double                               offsetOfSimulationDomainInX,
    double                               offsetOfSimulationDomainInY
  );

  ~NetCDFFieldSampler() = default;

  double sample(const std::string& fieldName, double x, double y) const;
};

#endif
