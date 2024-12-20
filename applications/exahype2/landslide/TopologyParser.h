// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "NetCDFReader.h"

namespace applications::exahype2::landslide {
  class TopologyParser;
} // namespace applications::exahype2::landslide

class applications::exahype2::landslide::TopologyParser {
private:
  NetCDFReader _netCDFReader;

  double* _elevation = nullptr;
  double* _initV = nullptr;

  const char* _ncElevationKey = nullptr;
  const char* _ncElevationKeyX = nullptr;
  const char* _ncElevationKeyY = nullptr;

  const char* _ncInitVKey = nullptr;
  const char* _ncInitVKeyX = nullptr;
  const char* _ncInitVKeyY = nullptr;

  std::size_t _lengthOfElevationInX = 0;
  std::size_t _lengthOfElevationInY = 0;
  std::size_t _lengthOfInitVInX = 0;
  std::size_t _lengthOfInitVInY = 0;

  double _lengthOfSimulationDomainInX = -1;
  double _lengthOfSimulationDomainInY = -1;

  double _minXCoordinateInElevation = 1;
  double _minYCoordinateInElevation = 1;
  double _maxXCoordinateInElevation = -1;
  double _maxYCoordinateInElevation = -1;

  double _minXCoordinateInInitV = 1;
  double _minYCoordinateInInitV = 1;
  double _maxXCoordinateInInitV = -1;
  double _maxYCoordinateInInitV = -1;

  void parseElevationFile(const char* elevationFilePath);
  void parseInitVFile(const char* initVFilePath);

  /**
   * @brief Transforms coordinate from simulation plane to netCDF plane.
   *
   * @return index in netCDF space [minIndexOfCDFspaceInZDirection,
   * maxIndexOfCDFspaceInZDirection]
   */
  double transformIndexSimulationToCDFRange(
    double coordinateOfDimensionZInSimulationSpace,
    double sizeOfSimulationSpaceInZDirection,
    double minIndexOfCDFspaceInZDirection,
    double maxIndexOfCDFspaceInZDirection,
    double domainOffsetInZDirection
  );

  /**
   * @brief Transforms coordinate pair from netCDF plane to 1D row-wise array
   * index.
   *
   * @return index in [0, ]
   */
  int transformIndexCDFRangeToArray(
    double xCoordinateInCDFMesh,
    double yCoordinateInCDFMesh,
    double minXCoordinateInCDFMesh,
    double minYCoordinateInCDFMesh,
    double maxXCoordinateInCDFMesh,
    double maxYCoordinateInCDFMesh,
    std::size_t lengthOfXDimensionInCDFMesh,
    std::size_t lengthOfYDimensionInCDFMesh
  );

public:
  TopologyParser(
    const char* ElevationFilePath,
    const char* initVFilePath,
    const char* ncElevationKey = "Band1",
    const char* ncElevationKeyX = "x",
    const char* ncElevationKeyY = "y",
    const char* ncInitVKey = "Band1",
    const char* ncInitVKeyX = "x",
    const char* ncInitVKeyY = "y"
  );

  TopologyParser(
    const char* elevationFilePath,
    double lengthOfSimulationDomainInY,
    double lengthOfSimulationDomainInX,
    const char* ncElevationKey = "Band1",
    const char* ncElevationKeyX = "x",
    const char* ncElevationKeyY = "y"
  );

  ~TopologyParser();

  double sampleElevation(double x, double y);
  double sampleInitV(double x, double y);
};
