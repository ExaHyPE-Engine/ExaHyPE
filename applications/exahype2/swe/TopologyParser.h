// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "NetCDFReader.h"

namespace applications::exahype2::swe {
  class TopologyParser;
} // namespace applications::exahype2::swe

class applications::exahype2::swe::TopologyParser {
private:
  NetCDFReader _netCDFReader;

  double* _bathymetry   = nullptr;
  double* _displacement = nullptr;

  const char* _ncBathymetryKey  = nullptr;
  const char* _ncBathymetryKeyX = nullptr;
  const char* _ncBathymetryKeyY = nullptr;

  const char* _ncDisplacementKey  = nullptr;
  const char* _ncDisplacementKeyX = nullptr;
  const char* _ncDisplacementKeyY = nullptr;

  std::size_t _lengthOfBathymetryInX   = 0;
  std::size_t _lengthOfBathymetryInY   = 0;
  std::size_t _lengthOfDisplacementInX = 0;
  std::size_t _lengthOfDisplacementInY = 0;

  double _lengthOfSimulationDomainInX = -1;
  double _lengthOfSimulationDomainInY = -1;

  double _minXCoordinateInBathymetry = 1;
  double _minYCoordinateInBathymetry = 1;
  double _maxXCoordinateInBathymetry = -1;
  double _maxYCoordinateInBathymetry = -1;

  double _minXCoordinateInDisplacement = 1;
  double _minYCoordinateInDisplacement = 1;
  double _maxXCoordinateInDisplacement = -1;
  double _maxYCoordinateInDisplacement = -1;

  void parseBathymetryFile(const char* bathymetryFilePath);
  void parseDisplacementFile(const char* displacementFilePath);

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
    double maxIndexOfCDFspaceInZDirection
  );

  /**
   * @brief Transforms coordinate pair from netCDF plane to 1D row-wise array
   * index.
   *
   * @return index in [0, ]
   */
  int transformIndexCDFRangeToArray(
    double      xCoordinateInCDFMesh,
    double      yCoordinateInCDFMesh,
    double      minXCoordinateInCDFMesh,
    double      minYCoordinateInCDFMesh,
    double      maxXCoordinateInCDFMesh,
    double      maxYCoordinateInCDFMesh,
    std::size_t lengthOfXDimensionInCDFMesh,
    std::size_t lengthOfYDimensionInCDFMesh
  );

public:
  TopologyParser(
    const char* bathymetryFilePath,
    const char* displacementFilePath,
    const char* ncBathymetryKey    = "z",
    const char* ncBathymetryKeyX   = "x",
    const char* ncBathymetryKeyY   = "y",
    const char* ncDisplacementKey  = "z",
    const char* ncDisplacementKeyX = "x",
    const char* ncDisplacementKeyY = "y"
  );

  TopologyParser(
    const char* bathymetryFilePath,
    double      lengthOfSimulationDomainInY,
    double      lengthOfSimulationDomainInX,
    const char* ncBathymetryKey  = "z",
    const char* ncBathymetryKeyX = "x",
    const char* ncBathymetryKeyY = "y"
  );

  ~TopologyParser();

  double sampleBathymetry(double x, double y);
  double sampleDisplacement(double x, double y);
};
