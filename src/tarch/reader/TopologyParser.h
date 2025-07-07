// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#ifdef UseNetCDF

#include "NetCDFReader.h"
#include "tarch/logging/Log.h"

namespace tarch::reader {
  class TopologyParser;
} // namespace tarch::reader

class tarch::reader::TopologyParser {
private:
  static tarch::logging::Log _log;

  NetCDFReader _netCDFReader;

  double* _topology     = nullptr;
  double* _displacement = nullptr;

  const char* _ncTopologyKey  = nullptr;
  const char* _ncTopologyKeyX = nullptr;
  const char* _ncTopologyKeyY = nullptr;

  const char* _ncDisplacementKey  = nullptr;
  const char* _ncDisplacementKeyX = nullptr;
  const char* _ncDisplacementKeyY = nullptr;

  std::size_t _lengthOfTopologyInX     = 0;
  std::size_t _lengthOfTopologyInY     = 0;
  std::size_t _lengthOfDisplacementInX = 0;
  std::size_t _lengthOfDisplacementInY = 0;

  double _lengthOfSimulationDomainInX = -1;
  double _lengthOfSimulationDomainInY = -1;
  double _offsetOfSimulationDomainInX = -1;
  double _offsetOfSimulationDomainInY = -1;

  double _minXCoordinateInTopology = 1;
  double _minYCoordinateInTopology = 1;
  double _maxXCoordinateInTopology = -1;
  double _maxYCoordinateInTopology = -1;

  double _minXCoordinateInDisplacement = 1;
  double _minYCoordinateInDisplacement = 1;
  double _maxXCoordinateInDisplacement = -1;
  double _maxYCoordinateInDisplacement = -1;

  void parseTopologyFile(const char* bathymetryFilePath);
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
    double maxIndexOfCDFspaceInZDirection,
    double offsetOfSimulationSpaceInZDirection
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
    double      lengthOfSimulationDomainInX,
    double      lengthOfSimulationDomainInY,
    double      offsetOfSimulationDomainInX,
    double      offsetOfSimulationDomainInY,
    const char* ncTopologyKey      = "z",
    const char* ncTopologyKeyX     = "x",
    const char* ncTopologyKeyY     = "y",
    const char* ncDisplacementKey  = "z",
    const char* ncDisplacementKeyX = "x",
    const char* ncDisplacementKeyY = "y"
  );

  TopologyParser(
    const char* bathymetryFilePath,
    double      lengthOfSimulationDomainInX,
    double      lengthOfSimulationDomainInY,
    double      offsetOfSimulationDomainInX,
    double      offsetOfSimulationDomainInY,
    const char* ncTopologyKey  = "z",
    const char* ncTopologyKeyX = "x",
    const char* ncTopologyKeyY = "y"
  );

  ~TopologyParser();

  double sampleTopology(double x, double y);
  double sampleDisplacement(double x, double y);
};

#endif
